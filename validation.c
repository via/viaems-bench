#include "platform.h"
#include "validation.h"
#include "decoder.h"

#include <stdint.h>
#include <stdbool.h>

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
