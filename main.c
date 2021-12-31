#include "platform.h"
#include "decoder.h"
#include "test.h"
#include "validation.h"


static void execute_test() {

  struct wheel mtw = make_wheel_cam24plus1();
  struct validator validator;
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
