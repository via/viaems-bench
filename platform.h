#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include "decoder.h"
#include "test.h"


void platform_init();
uint32_t current_time_microseconds();
void wait_microseconds(uint32_t);
void wait_seconds(uint32_t);

void start_recording_outputs(struct test_case *tc);
void stop_recording_outputs();

void set_wheel_pattern(struct wheel *w);
void set_wheel_degree_period(uint32_t ns);

#endif

