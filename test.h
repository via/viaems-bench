#ifndef TEST_H
#define TEST_H

#include <stdint.h>
#include <stdbool.h>
#include "decoder.h"

struct ems_output_event {
  uint32_t time;
  uint16_t outputs;
};

struct ems_input_event {
  uint32_t time;
  uint16_t adc[8];
  int trigger;
};

struct test_case {
  uint32_t start_time;

  struct ems_output_event *outputs;
  uint32_t n_outputs;
  uint32_t max_outputs;

  struct ems_input_event *inputs;
  uint32_t n_inputs;
  uint32_t max_inputs;
};

struct test_case initialize_static_test_case(uint32_t start_time);
bool add_trigger(struct test_case *, struct trigger_event);
bool add_adc(struct test_case *, uint16_t vals[8]);

#endif

