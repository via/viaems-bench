#ifndef VALIDATION_H
#define VALIDATION_H

#include <stdint.h>
#include <stdbool.h>

#include "decoder.h"
#include "test.h"

struct input_configuration {
  bool enabled;
  uint8_t pin;
  int32_t end_angle;

  int32_t end_advance_upper_bound;
  int32_t end_advance_lower_bound;

  uint32_t duration_us_lower_bound;
  uint32_t duration_us_upper_bound;
};

struct input_validation {
  uint32_t misses;
  uint32_t occurances;
  uint32_t bad_durations;
  uint32_t duplicates;
};
  
struct validator {
  uint32_t rising_times[16];
  uint32_t last_durations[16];
  bool triggereds[16];
  bool started[16];
  uint16_t outputs;
  struct input_configuration *configuration;
  struct input_validation validation[16];
  uint32_t unknown_events;
};

bool validate_next(struct validator *v, struct ems_output_event *ev);

#endif
