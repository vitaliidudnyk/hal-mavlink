#ifndef LED_FSM_H
#define LED_FSM_H

#include "stm32f4xx_hal.h"

typedef enum
{
  LED_MODE_OFF = 0,
  LED_MODE_BLINK,
  LED_MODE_SOS
} LedMode;

void Led_Init(GPIO_TypeDef* port, uint16_t pin);
void Led_SetMode(LedMode mode);
void Led_Update(uint32_t now_ms);

#endif // LED_FSM_H
