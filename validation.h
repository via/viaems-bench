#ifndef VALIDATION_H
#define VALIDATION_H

#include <stdint.h>
#include <stdbool.h>

#include "decoder.h"
#include "test.h"

struct validator {
  uint16_t last_outputs;
};

bool validate_next(struct validator *v, struct ems_output_event *ev);

#endif
