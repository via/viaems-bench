#ifndef DECODER_H
#define DECODER_H

#include <stdint.h>
#include <stdatomic.h>

struct trigger_event {
  uint32_t time;
  uint32_t trigger;
};

struct wheel {
  uint32_t pattern[720];
  float offset;

  volatile uint32_t degree;
  volatile uint32_t revolutions;
};

struct wheel make_wheel_cam24plus1();

void wheel_wait_revolutions(struct wheel *w, uint32_t revs);
float wheel_angle(struct wheel *w);
void wheel_set_rpm(uint32_t);

#endif
