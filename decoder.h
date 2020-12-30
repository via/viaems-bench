#ifndef DECODER_H
#define DECODER_H

#include <stdint.h>

struct trigger_event {
  uint32_t time;
  uint32_t trigger;
};

struct decoder;
typedef void (*decode_func)(struct decoder *, uint32_t time, int n);
typedef struct trigger_event (*generate_func)(struct decoder *d, float rpm);
typedef float (*angle_func)(struct decoder *d, uint32_t time);

struct decoder {
  decode_func decode;
  generate_func generate;
  angle_func angle;
  uint32_t n_triggers;
  float degrees_per_trigger;
  float offset;

  uint32_t last_trigger_time;
  uint32_t triggers_since_sync;;
  float last_trigger_angle;
  float rpm;
};

struct decoder create_decoder_missing_36minus1and1();

#endif
