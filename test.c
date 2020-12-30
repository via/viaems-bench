#include "test.h"
#include "decoder.h"

struct ems_input_event inputs[1024];
struct ems_output_event outputs[1024];

struct test_case initialize_static_test_case(uint32_t start_time) {
  return (struct test_case) {
    .start_time = start_time,

    .outputs = outputs,
    .max_outputs = sizeof(outputs) / sizeof(struct ems_output_event),

    .inputs = inputs,
    .max_inputs = sizeof(inputs) / sizeof(struct ems_input_event),
  };
}


bool add_trigger(struct test_case *tc, struct trigger_event e) {
  if (tc->n_inputs >= tc->max_inputs) {
    return false;
  }
  tc->inputs[tc->n_inputs].trigger = e.trigger;
  tc->inputs[tc->n_inputs].time = e.time;
  tc->n_inputs += 1;
  return true;
}

bool add_adc(struct test_case *tc, uint16_t vals[8]) {
  return false;
}
