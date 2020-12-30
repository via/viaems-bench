#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include "decoder.h"
#include "test.h"


void platform_init();
uint32_t current_time();

void start_recording_outputs(struct test_case *tc);
void stop_recording_outputs();

void execute_inputs(struct test_case *tc);

#endif

