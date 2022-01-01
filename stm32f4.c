#include <string.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include "libopencm3/stm32/f4/rcc.h"

#include "validation.h"
#include "test.h"

#define SPI_WRITE_COUNT 9
static uint16_t spi_tx_buf[SPI_WRITE_COUNT] = { 1, 2, 3, 4, 5, 6, 7, 8, 9};

void set_adc_pin_value(int pin, uint16_t value) {
  if (value < 4096) {
    spi_tx_buf[pin] = value;
  }
}

static void prepare_for_spi_tx_dma() {
  dma_stream_reset(DMA1, DMA_STREAM4);
  dma_set_priority(DMA1, DMA_STREAM4, DMA_SxCR_PL_LOW);
  dma_set_memory_size(DMA1, DMA_STREAM4, DMA_SxCR_MSIZE_16BIT);
  dma_set_peripheral_size(DMA1, DMA_STREAM4, DMA_SxCR_PSIZE_16BIT);
  dma_enable_memory_increment_mode(DMA1, DMA_STREAM4);
  dma_set_transfer_mode(DMA1, DMA_STREAM4, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
  dma_set_peripheral_address(DMA1, DMA_STREAM4, (uint32_t)&SPI2_DR);
  dma_set_memory_address(DMA1, DMA_STREAM4, (uint32_t)spi_tx_buf);
  dma_set_number_of_data(
   DMA1, DMA_STREAM4, sizeof(spi_tx_buf) / sizeof(spi_tx_buf[0]));
  dma_channel_select(DMA1, DMA_STREAM4, DMA_SxCR_CHSEL_0);
  dma_enable_direct_mode(DMA1, DMA_STREAM4);
  dma_enable_stream(DMA1, DMA_STREAM4);
}

static void setup_spi() {
  rcc_periph_clock_enable(RCC_DMA1);
  rcc_periph_clock_enable(RCC_SPI2);
  rcc_periph_clock_enable(RCC_GPIOB);
  /* B12-15 spi2 */
  gpio_mode_setup(
    GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO12 | GPIO13 | GPIO14 | GPIO15);
  gpio_set_af(GPIOB, GPIO_AF5, GPIO12 | GPIO13 | GPIO14 | GPIO15);

  spi_disable(SPI2);
  spi_reset(SPI2);
  spi_init_master(SPI2,
                  SPI_CR1_BAUDRATE_FPCLK_DIV_32,
                  SPI_CR1_CPOL_CLK_TO_0_WHEN_IDLE,
                  SPI_CR1_CPHA_CLK_TRANSITION_1,
                  SPI_CR1_DFF_16BIT,
                  SPI_CR1_MSBFIRST);
  spi_disable_software_slave_management(SPI2);
  spi_set_slave_mode(SPI2);

  spi_enable_tx_dma(SPI2);
  spi_enable(SPI2);
}

void exti15_10_isr() {
  uint32_t ints = exti_get_flag_status(0xFFFF);
  prepare_for_spi_tx_dma();
  exti_reset_request(ints);
  __asm__("dsb");
  __asm__("isb");
}

uint32_t current_time_microseconds() {
  return timer_get_counter(TIM5);
}

void wait_seconds(uint32_t seconds) {
  uint32_t current = current_time_microseconds();
  while ((int32_t)(current_time_microseconds() - current) > 1000000 * seconds);
}

static void setup_timer() {

  rcc_periph_clock_enable(RCC_TIM8);
  timer_set_mode(TIM8, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_set_period(TIM8, 167); /* 1 uS */
  timer_set_prescaler(TIM8, 0);
  timer_disable_preload(TIM8);
  timer_continuous_mode(TIM8);
  timer_disable_oc_output(TIM8, TIM_OC1);
  timer_disable_oc_output(TIM8, TIM_OC2);
  timer_disable_oc_output(TIM8, TIM_OC3);
  timer_disable_oc_output(TIM8, TIM_OC4);
  timer_set_master_mode(TIM8, TIM_CR2_MMS_UPDATE);
  timer_enable_counter(TIM8);

  rcc_periph_clock_enable(RCC_TIM5);
  timer_set_mode(TIM5, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_slave_set_mode(TIM5, TIM_SMCR_SMS_ECM1);
  timer_slave_set_trigger(TIM5, TIM_SMCR_TS_ITR3); /* TIM5 slaved off TIM8 */
  timer_set_period(TIM5, 0xFFFFFFFF);
  timer_set_prescaler(TIM5, 0);
  timer_disable_preload(TIM5);
  timer_continuous_mode(TIM5);

  /* Setup output compare registers */
  timer_disable_oc_output(TIM5, TIM_OC1);
  timer_disable_oc_output(TIM5, TIM_OC2);
  timer_disable_oc_output(TIM5, TIM_OC3);
  timer_disable_oc_output(TIM5, TIM_OC4);

  timer_enable_counter(TIM5);
}

static void setup_triggers() {
  rcc_periph_clock_enable(RCC_GPIOA);
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO0);
  gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO1);
}

static struct wheel *current_wheel = NULL;

void set_wheel_pattern(struct wheel *w) {
  rcc_periph_clock_enable(RCC_DMA2);
  rcc_periph_clock_enable(RCC_TIM1);

  current_wheel = w;

  /* dma1 stream 1, channel 7*/
  dma_stream_reset(DMA2, DMA_STREAM5);
  dma_set_priority(DMA2, DMA_STREAM5, DMA_SxCR_PL_HIGH);
  dma_set_memory_size(DMA2, DMA_STREAM5, DMA_SxCR_MSIZE_32BIT);
  dma_set_peripheral_size(DMA2, DMA_STREAM5, DMA_SxCR_PSIZE_32BIT);
  dma_enable_memory_increment_mode(DMA2, DMA_STREAM5);
  dma_set_transfer_mode(DMA2, DMA_STREAM5, DMA_SxCR_DIR_MEM_TO_PERIPHERAL);
  dma_enable_circular_mode(DMA2, DMA_STREAM5);
  dma_set_peripheral_address(DMA2, DMA_STREAM5, (uint32_t)&GPIOA_ODR);
  dma_set_memory_address(DMA2, DMA_STREAM5, (uint32_t)w->pattern);
  dma_set_number_of_data(DMA2, DMA_STREAM5, 720);
  dma_channel_select(DMA2, DMA_STREAM5, DMA_SxCR_CHSEL_6);
  dma_enable_transfer_complete_interrupt(DMA2, DMA_STREAM5);
  dma_enable_stream(DMA2, DMA_STREAM5);

  /* Set up TIM6 to trigger interrupts at a later-determined interval */
  timer_set_counter(TIM1, 0);
  timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_set_period(TIM1, 4000);
  timer_set_prescaler(TIM1, 0); /* 168 MHz */
  timer_enable_preload(TIM1);
  timer_continuous_mode(TIM1);
  timer_enable_update_event(TIM1);
  timer_update_on_overflow(TIM1);
  timer_set_dma_on_update_event(TIM1);
  TIM1_DIER |= TIM_DIER_UDE; /* Enable update dma */

  timer_enable_irq(TIM1, TIM_DIER_UIE);
  nvic_enable_irq(NVIC_TIM1_UP_TIM10_IRQ);
  nvic_set_priority(NVIC_TIM1_UP_TIM10_IRQ, 0);

  nvic_enable_irq(NVIC_DMA2_STREAM5_IRQ);
  nvic_set_priority(NVIC_DMA2_STREAM5_IRQ, 0);
  
}

void tim1_up_tim10_isr() {
  if (timer_get_flag(TIM1, TIM_SR_UIF)) {
    timer_clear_flag(TIM1, TIM_SR_UIF);
    uint32_t current = current_wheel->degree;
    current += 1;
    if (current >= 720) {
      current = 0;
    }
    current_wheel->degree = current;
  }
}

void dma2_stream5_isr() {
  if (dma_get_interrupt_flag(DMA2, DMA_STREAM5, DMA_TCIF)) {
    dma_clear_interrupt_flags(DMA2, DMA_STREAM5, DMA_TCIF);
    current_wheel->revolutions++;
  }
}

void set_wheel_degree_period(uint32_t ns) {
  if (ns > 0) {
    uint32_t clocks = 168 * ns / 1000;
    timer_set_period(TIM1, clocks  - 1);
    timer_enable_counter(TIM1);
  } else {
    timer_disable_counter(TIM1);
  }
}


static struct validator *current_validator = NULL;
void start_recording_outputs(struct test_case *tc) {
  current_validator = tc->validator;
  rcc_periph_clock_enable(RCC_GPIOE);
  /* Enable GPIO E0-7 as input */
  gpio_mode_setup(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, 0x00FF);

  nvic_enable_irq(NVIC_EXTI0_IRQ);
  nvic_enable_irq(NVIC_EXTI1_IRQ);
  nvic_enable_irq(NVIC_EXTI2_IRQ);
  nvic_enable_irq(NVIC_EXTI3_IRQ);
  nvic_enable_irq(NVIC_EXTI4_IRQ);
  nvic_enable_irq(NVIC_EXTI9_5_IRQ);

  nvic_set_priority(NVIC_EXTI0_IRQ, 32);
  nvic_set_priority(NVIC_EXTI1_IRQ, 32);
  nvic_set_priority(NVIC_EXTI2_IRQ, 32);
  nvic_set_priority(NVIC_EXTI3_IRQ, 32);
  nvic_set_priority(NVIC_EXTI4_IRQ, 32);
  nvic_set_priority(NVIC_EXTI9_5_IRQ, 32);

  exti_select_source(0xFF, GPIOE);
  exti_set_trigger(0xFF, EXTI_TRIGGER_BOTH);
  exti_enable_request(0xFF);
}

void stop_recording_outputs() {
  nvic_disable_irq(NVIC_EXTI0_IRQ);
  nvic_disable_irq(NVIC_EXTI1_IRQ);
  nvic_disable_irq(NVIC_EXTI2_IRQ);
  nvic_disable_irq(NVIC_EXTI3_IRQ);
  nvic_disable_irq(NVIC_EXTI4_IRQ);
  nvic_disable_irq(NVIC_EXTI9_5_IRQ);
}


struct bleh {
  uint32_t time;
  uint32_t angle;
  uint16_t values;
};

static void record_gpio_change() {
  uint32_t time = current_time_microseconds();
  uint32_t flag_changes = exti_get_flag_status(0xFF);
  uint32_t gpio = gpio_port_read(GPIOE);
  struct ems_output_event ev = (struct ems_output_event){
    .time = time,
    .angle = current_wheel->degree,
    .outputs = gpio,
  };
  validate_next(current_validator, &ev);
  exti_reset_request(flag_changes);
  __asm__("dsb");
  __asm__("isb");
}

void exti0_isr() {
  record_gpio_change();
}
void exti1_isr() {
  record_gpio_change();
}
void exti2_isr() {
  record_gpio_change();
}
void exti3_isr() {
  record_gpio_change();
}
void exti4_isr() {
  record_gpio_change();
}
void exti9_5_isr() {
  record_gpio_change();
}

void platform_init() {
  rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
  rcc_periph_clock_enable(RCC_SYSCFG);

  scb_set_priority_grouping(SCB_AIRCR_PRIGROUP_GROUP16_NOSUB);

  setup_timer();
  setup_triggers();
}
