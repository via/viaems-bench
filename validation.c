#include "platform.h"
#include "validation.h"
#include "decoder.h"

#include <stdint.h>
#include <stdbool.h>

static void validate_falling_edge(struct validator *v, struct ems_output_event *ev, int pin) {
  bool found_event = false;
  for (int i = 0; i < 16; i++) {
    const struct input_configuration *c = &v->configuration[i];
    if (!c->enabled) {
      break;
    }
    if (pin != c->pin) {
      continue;
    }
    int32_t start_adv = (int32_t)c->end_angle - (int32_t)ev->angle;
    if (start_adv < -360) {
      start_adv += 720;
    }
    if (start_adv >= 360) {
      start_adv -= 720;
    }
    if (start_adv < c->end_advance_lower_bound || start_adv > c->end_advance_upper_bound) {
      continue;
    }
    found_event = true;
    v->validation[i].occurances++;
    uint32_t duration = ev->time - v->rising_times[pin];
    v->last_durations[i] = duration;
    if (duration < c->duration_us_lower_bound || duration > c->duration_us_upper_bound) {
      v->validation[i].bad_durations++;
    }
  }
  if (!found_event) {
    v->unknown_events++;
  }
}

bool validate_next(struct validator *v, struct ems_output_event *ev) {
  for (int pin = 0; pin < 16; pin++) {
    bool last = (v->outputs & (1 << pin)) > 0;
    bool now = (ev->outputs & (1 << pin)) > 0;
    if (!last && now) {
      /* Rising edge, record time */
      v->rising_times[pin] = ev->time;
    } else if (last && !now) {
      /* Falling edge */
      validate_falling_edge(v, ev, pin);
    }
  }
  v->outputs = ev->outputs;
  return false;
}
