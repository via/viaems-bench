#include <stdint.h>
#include <stdbool.h>

#include "decoder.h"
#include "platform.h"

struct wheel make_wheel_cam24plus1() {
  struct wheel w = { 0 };
  for (uint32_t i = 0; i < 720; i++) {
    if (i % 10 < 2) {
      w.pattern[i] |= 0x1;
    }
  }
  w.pattern[35] |= 0x2;
  return w;
}

void wheel_wait_revolutions(struct wheel *w, uint32_t revs) {
  while (revs > 0) {
    uint32_t current_rev = w->revolutions;
    while (current_rev == w->revolutions);
    revs--;
  }
}

float wheel_angle(struct wheel *w) {
  return w->degree;
}

void wheel_set_rpm(uint32_t rpm) {
  if (rpm > 0) {
    uint32_t ns_per_degree = 1000000000 / (360 / 60 * rpm);
    set_wheel_degree_period(ns_per_degree);
  } else {
    set_wheel_degree_period(0);
  }
}
