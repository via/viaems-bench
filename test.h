#ifndef TEST_H
#define TEST_H

#include <stdint.h>
#include <stdbool.h>
#include "decoder.h"

struct ems_output_event {
  uint32_t time;
  float angle;
  uint16_t outputs;
};

struct test_case {
  struct wheel *wheel;
  struct validator *validator;
};

#endif

