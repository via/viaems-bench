#include "platform.h"
#include "decoder.h"
#include "test.h"
#include "validation.h"



static void execute_missing_tooth_startup() {
  uint32_t start_time = current_time() + 4000000;

  struct test_case tc = initialize_static_test_case(start_time);
  struct decoder d = create_decoder_missing_36minus1and1();

  for (int i = 0; i < 500; i++, i++) {
    add_trigger(&tc, d.generate(&d, 1000));
  }

  
  start_recording_outputs(&tc);
  execute_inputs(&tc);
  stop_recording_outputs();

  struct validator v = {
    .decoder = create_decoder_missing_36minus1and1(),
    .test_case = &tc,
  };
  struct validated_change evs[128];
  struct validated_change *ve = &evs[0];
  while (1);
}



int main(void) {
  platform_init();

  execute_missing_tooth_startup();
  while(1);
}
