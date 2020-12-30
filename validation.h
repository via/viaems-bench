#ifndef VALIDATION_H
#define VALIDATION_H

#include <stdint.h>
#include <stdbool.h>

#include "decoder.h"
#include "test.h"

struct validator {
  struct decoder decoder;
  struct test_case *test_case;

  uint16_t last_outputs;
  uint32_t input_pos;
  uint32_t output_pos;
};

struct validated_change {
  bool valid;
  uint32_t time;
  float angle;
  uint32_t pin;
  bool value;
};

bool validate_next(struct validator *v, struct validated_change *);

#endif
