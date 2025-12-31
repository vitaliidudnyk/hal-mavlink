#include "led_fsm.h"
#include "logger.h"

static GPIO_TypeDef* led_port;
static uint16_t led_pin;

static LedMode current_mode = LED_MODE_OFF;
static uint32_t next_ms = 0;
static uint8_t step = 0;

static const uint16_t sos_pattern[] = {
  200, 200, 200,   // S: ...
  600,
  600, 600, 600,   // O: ---
  600,
  200, 200, 200,   // S: ...
  1200              // pause
};

#define SOS_STEPS (sizeof(sos_pattern) / sizeof(sos_pattern[0]))

void Led_Init(GPIO_TypeDef* port, uint16_t pin)
{
  led_port = port;
  led_pin = pin;
}

void Led_SetMode(LedMode mode)
{
  current_mode = mode;
  step = 0;
  next_ms = 0;
}

void Led_Update(uint32_t now_ms)
{
  if (now_ms < next_ms)
  {
    return;
  }

  switch (current_mode)
  {
    case LED_MODE_OFF:
      HAL_GPIO_WritePin(led_port, led_pin, GPIO_PIN_RESET);
      break;

    case LED_MODE_BLINK:
      HAL_GPIO_TogglePin(led_port, led_pin);
      next_ms = now_ms + 500;
      break;

    case LED_MODE_SOS:
      HAL_GPIO_TogglePin(led_port, led_pin);
      next_ms = now_ms + sos_pattern[step];
      step = (step + 1) % SOS_STEPS;
      break;
  }
}
