#pragma once

#include <stdint.h>
#include "stm32f4xx_hal.h"

typedef enum
{
    BUTTON_EVENT_NONE = 0,
    BUTTON_EVENT_SHORT,
    BUTTON_EVENT_LONG
} ButtonEvent;

void Button_Init(GPIO_TypeDef* port, uint16_t pin, GPIO_PinState pressed_level);
void Button_OnExti(uint16_t gpio_pin);
ButtonEvent Button_Update(uint32_t now_ms);
