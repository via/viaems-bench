#include "platform.h"
#include "validation.h"
#include "decoder.h"

#include <stdint.h>
#include <stdbool.h>

static const struct event expected_events[24] = {
  {.type=IGNITION, .angle=0, .pin=0},
  {.type=IGNITION, .angle=120, .pin=1},
  {.type=IGNITION, .angle=240, .pin=2},
  {.type=IGNITION, .angle=360, .pin=0},
  {.type=IGNITION, .angle=480, .pin=1},
  {.type=IGNITION, .angle=600, .pin=2},

  {.type=FUEL, .angle=700, .pin=3},
  {.type=FUEL, .angle=460, .pin=4},
  {.type=FUEL, .angle=220, .pin=5},
};

static bool angle_matches(float actual, float expected, float expected_minus,
    float expected_plus);

static struct event match_edge_to_event(int pin, float angle, bool is_rising) {
  struct event match;
  for (int i = 0; i < 24; i++) {
    if (expected_events[i].pin != pin) {
      continue;
    }
    switch(expected_events[i].type) {
      case IGNITION:
        if (!is_rising && angle_matches(angle, expected_events[i].angle, -45, 0)) {
          match = expected_events[i];
        }
        break;
      case FUEL:
        break;
      case NONE:
      default:
        continue;
    }

  }
  return match;
}

bool validate_next(struct validator *v, struct validated_change *c) {
  bool triggers_remain = v->input_pos < v->test_case->n_inputs;
  bool outputs_remain = v->output_pos < v->test_case->n_outputs;

  if (!outputs_remain) {
    return false;
  } 

  /* Check if a trigger is next, do it if so */
  while (triggers_remain && 
         (int32_t)(v->test_case->inputs[v->input_pos].time - v->test_case->outputs[v->output_pos].time) < 0) {
    struct ems_input_event *eie = &v->test_case->inputs[v->input_pos];
    v->decoder.decode(&v->decoder, eie->time, eie->trigger);
    v->input_pos += 1;
    triggers_remain = v->input_pos < v->test_case->n_inputs;
  }

  struct ems_output_event *eoe = &v->test_case->outputs[v->output_pos];
  uint16_t high_pins = eoe->outputs & ~v->last_outputs;
  uint16_t low_pins = ~eoe->outputs & v->last_outputs;
  for (int bit = 0; bit < 16; bit++) {
    if (high_pins & (1 << bit)) {
      high_pins &= ~(1 << bit);
      v->last_outputs |= (1 << bit);
      if (!high_pins && !low_pins) {
        v->output_pos += 1;
      }
      *c = (struct validated_change){
        .valid = true,
        .time = eoe->time - v->test_case->start_time,
        .pin = bit,
        .value = true,
        .angle = v->decoder.angle(&v->decoder, eoe->time),
      };
      return true;
    }
    if (low_pins & (1 << bit)) {
      v->last_outputs &= ~(1 << bit);
      low_pins &= ~(1 << bit);
      if (!high_pins && !low_pins) {
        v->output_pos += 1;
      }
      *c = (struct validated_change){
        .valid = true,
        .time = eoe->time - v->test_case->start_time,
        .pin = bit,
        .value = false,
        .angle = v->decoder.angle(&v->decoder, eoe->time),
      };
      return true;
    }
  }
  return false;
}
