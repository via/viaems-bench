#include "platform.h"
#include "decoder.h"
#include "test.h"
#include "validation.h"

static struct input_configuration pinconfig[] = {
  { .enabled = true, .pin = 0, .end_angle = 0,
    .end_advance_upper_bound = 45, .end_advance_lower_bound = 0,
    .duration_us_lower_bound = 2000, .duration_us_upper_bound = 4000,
  },
  { .enabled = true, .pin = 1, .end_angle = 120,
    .end_advance_upper_bound = 45, .end_advance_lower_bound = 0,
    .duration_us_lower_bound = 2000, .duration_us_upper_bound = 4000,
  },
  { .enabled = true, .pin = 2, .end_angle = 240,
    .end_advance_upper_bound = 45, .end_advance_lower_bound = 0,
    .duration_us_lower_bound = 2000, .duration_us_upper_bound = 4000,
  },
  { .enabled = true, .pin = 0, .end_angle = 360,
    .end_advance_upper_bound = 45, .end_advance_lower_bound = 0,
    .duration_us_lower_bound = 2000, .duration_us_upper_bound = 4000,
  },
  { .enabled = true, .pin = 1, .end_angle = 480,
    .end_advance_upper_bound = 45, .end_advance_lower_bound = 0,
    .duration_us_lower_bound = 2000, .duration_us_upper_bound = 4000,
  },
  { .enabled = true, .pin = 2, .end_angle = 600,
    .end_advance_upper_bound = 45, .end_advance_lower_bound = 0,
    .duration_us_lower_bound = 2000, .duration_us_upper_bound = 4000,
  },
  { .enabled = true, .pin = 6, .end_angle = 700,
    .end_advance_upper_bound = 10, .end_advance_lower_bound = -10,
    .duration_us_lower_bound = 1000, .duration_us_upper_bound = 8000,
  },
  { .enabled = true, .pin = 4, .end_angle = 460,
    .end_advance_upper_bound = 10, .end_advance_lower_bound = -10,
    .duration_us_lower_bound = 1000, .duration_us_upper_bound = 8000,
  },
  { .enabled = true, .pin = 5, .end_angle = 220,
    .end_advance_upper_bound = 10, .end_advance_lower_bound = -10,
    .duration_us_lower_bound = 1000, .duration_us_upper_bound = 8000,
  },
  { .enabled = false },
};

static void execute_test() {

  struct wheel mtw = make_wheel_cam24plus1();
  struct validator validator = (struct validator){.configuration = pinconfig};;
  struct test_case tc = (struct test_case){.wheel = &mtw, .validator = &validator};

  set_wheel_pattern(&mtw);
  start_recording_outputs(&tc);
  wheel_set_rpm(1000);
  while(1);
  wheel_wait_revolutions(&mtw, 10);
  wheel_set_rpm(0);

  wait_seconds(1);

  stop_recording_outputs();
}

int main(void) {
  platform_init();

  execute_test();
  while(1);
}
