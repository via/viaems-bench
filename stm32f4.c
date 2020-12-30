#include <string.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/timer.h>
#include <libopencm3/stm32/exti.h>
#include <libopencm3/cm3/nvic.h>
#include "libopencm3/stm32/f4/rcc.h"

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

  /* Use exti to trigger prepare_fake_adc_tx on rising CS edge */
  nvic_enable_irq(NVIC_EXTI15_10_IRQ);
  exti_select_source(EXTI12, GPIOB);
  exti_set_trigger(EXTI12, EXTI_TRIGGER_RISING);
  exti_enable_request(EXTI12);

}

void exti15_10_isr() {
  uint32_t ints = exti_get_flag_status(0xFFFF);
  prepare_for_spi_tx_dma();
  exti_reset_request(ints);
  __asm__("dsb");
  __asm__("isb");
}

uint32_t current_time() {
  return timer_get_counter(TIM5);
}

static void wait_until(uint32_t time) {
  while ((int32_t)(time - current_time()) > 0);
}


static void setup_timer() {

  rcc_periph_clock_enable(RCC_TIM8);
  timer_set_mode(TIM8, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
  timer_set_period(TIM8, 41); /* 0.25 uS */
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


static struct test_case *recording_test;

void start_recording_outputs(struct test_case *tc) {
  recording_test = tc;
  rcc_periph_clock_enable(RCC_GPIOE);
  /* Enable GPIO E0-7 as input */
  gpio_mode_setup(GPIOE, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, 0x00FF);

  nvic_enable_irq(NVIC_EXTI0_IRQ);
  nvic_enable_irq(NVIC_EXTI1_IRQ);
  nvic_enable_irq(NVIC_EXTI2_IRQ);
  nvic_enable_irq(NVIC_EXTI3_IRQ);
  nvic_enable_irq(NVIC_EXTI4_IRQ);
  nvic_enable_irq(NVIC_EXTI9_5_IRQ);

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


static void record_gpio_change() {
  uint32_t flag_changes = exti_get_flag_status(0xFF);
  uint32_t gpio = gpio_port_read(GPIOE);
  exti_reset_request(flag_changes);
  uint32_t at_time = current_time();
  recording_test->outputs[recording_test->n_outputs].time = at_time;
  recording_test->outputs[recording_test->n_outputs].outputs = gpio;
  recording_test->n_outputs += 1;
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

void execute_inputs(struct test_case *tc) {

  uint32_t evnum = 0;

  while (evnum < tc->n_inputs) {
    struct ems_input_event *ev = &tc->inputs[evnum];

    wait_until(tc->start_time + ev->time);
    
    if (ev->trigger == 0) {
      gpio_set(GPIOA, GPIO0);
      gpio_clear(GPIOA, GPIO0);
    } else if (ev->trigger == 1) {
      gpio_set(GPIOA, GPIO1);
      gpio_clear(GPIOA, GPIO1);
    }
    ev->time += tc->start_time;

    memcpy(&spi_tx_buf[1], ev->adc, sizeof(uint16_t) * 8);
    evnum += 1;
  }
}

void platform_init() {
  rcc_clock_setup_pll(&rcc_hse_8mhz_3v3[RCC_CLOCK_3V3_168MHZ]);
  rcc_periph_clock_enable(RCC_SYSCFG);
  setup_spi();
  setup_timer();
  setup_triggers();
}
