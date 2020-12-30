#include <stdint.h>
#include <stdbool.h>

#include "decoder.h"

static uint32_t time_from_angle_and_rpm(float angle, float rpm) {
  float ticks_per_degree = (TICKRATE / 6.0f) / rpm;
  return ticks_per_degree * angle;
}

static uint32_t angle_from_time_and_rpm(uint32_t time, float rpm) {
  float ticks_per_degree = (TICKRATE / 6.0f) / rpm;
  return time / ticks_per_degree;
}

float rpm_from_time_and_angle(uint32_t time, float angle) {
  float ticks_per_degree = time / angle;
  return (TICKRATE / 6.0f) / ticks_per_degree;
}

static void decode_missing_with_sync(struct decoder *d, uint32_t time, int n) {
  
  if (n == 0) {
    uint32_t diff = time - d->last_trigger_time;
    float tooth_rpm = rpm_from_time_and_angle(diff, d->degrees_per_trigger);
    if (d->rpm) {
      bool gap = ((tooth_rpm > 1.5f * d->rpm) && (tooth_rpm < 2.5f * d->rpm));
      if (gap) {
        d->last_trigger_angle = 0.0f;
        d->rpm = tooth_rpm * 2.0f; 
      } else {
        d->last_trigger_angle += d->degrees_per_trigger;
        d->rpm = tooth_rpm ;
      }
    } else {
      d->rpm = tooth_rpm;
    }
    d->last_trigger_time = time;
  }
}

static struct trigger_event generate_missing_with_sync(struct decoder *d, float rpm) {
  struct trigger_event ev;
  bool skip = false;

  d->rpm = rpm;

  if (d->last_trigger_angle == 90 || d->last_trigger_angle == 450) {
    skip = true;
  }

  if (d->last_trigger_angle == 180) {
    if (d->triggers_since_sync != 0) {
      d->triggers_since_sync = 0;
      return (struct trigger_event){
        .time = d->last_trigger_time,
        .trigger = 1,
      };
    }
  }

  d->triggers_since_sync += 1;

  ev.time = d->last_trigger_time + (skip ? 2 : 1) * time_from_angle_and_rpm(d->degrees_per_trigger, d->rpm);
  ev.trigger = 0;

  d->last_trigger_time = ev.time;
  d->last_trigger_angle += d->degrees_per_trigger * (skip ? 2 : 1);
  if (d->last_trigger_angle >= 720) {
    d->last_trigger_angle = 0;
  }
  return ev;
}

static float generic_angle(struct decoder *d, uint32_t time) {
  int32_t diff = time - d->last_trigger_time; 
  float angle = angle_from_time_and_rpm(diff, d->rpm);
  angle += d->offset;
  angle += d->last_trigger_angle;
  return angle;
}

struct decoder create_decoder_missing_36minus1and1() {
  return (struct decoder){
    .decode = decode_missing_with_sync,
    .generate = generate_missing_with_sync,
    .angle = generic_angle,
    .n_triggers = 36,
    .degrees_per_trigger = 10,
    .offset = 425,
  };
}

