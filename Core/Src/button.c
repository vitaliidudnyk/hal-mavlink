#include "button.h"
#include <stdbool.h>

#define BUTTON_DEBOUNCE_MS_DEFAULT    50u
#define BUTTON_LONG_PRESS_MS_DEFAULT  1000u

static GPIO_TypeDef* s_port = NULL;
static uint16_t s_pin = 0u;
static GPIO_PinState s_pressed_level = GPIO_PIN_RESET;

static volatile bool s_exti_flag = false;

static GPIO_PinState s_raw_state = GPIO_PIN_SET;
static GPIO_PinState s_stable_state = GPIO_PIN_SET;
static uint32_t s_raw_changed_ms = 0u;

static bool s_is_pressed = false;
static bool s_long_fired = false;
static uint32_t s_pressed_start_ms = 0u;

static uint32_t s_debounce_ms = BUTTON_DEBOUNCE_MS_DEFAULT;
static uint32_t s_long_press_ms = BUTTON_LONG_PRESS_MS_DEFAULT;

void Button_Init(GPIO_TypeDef* port, uint16_t pin, GPIO_PinState pressed_level)
{
    s_port = port;
    s_pin = pin;
    s_pressed_level = pressed_level;

    s_exti_flag = false;

    s_raw_state = HAL_GPIO_ReadPin(s_port, s_pin);
    s_stable_state = s_raw_state;
    s_raw_changed_ms = 0u;

    s_is_pressed = (s_stable_state == s_pressed_level);
    s_long_fired = false;
    s_pressed_start_ms = 0u;
}

void Button_OnExti(uint16_t gpio_pin)
{
    if (gpio_pin == s_pin)
    {
        s_exti_flag = true;
    }
}

static bool Button_IsPressed(GPIO_PinState state)
{
    return (state == s_pressed_level);
}

ButtonEvent Button_Update(uint32_t now_ms)
{
    if (s_port == NULL)
    {
        return BUTTON_EVENT_NONE;
    }

    // Optional fast path: if no EXTI signal and we're idle, do nothing.
    // We still need polling while pressed to detect LONG_PRESS without relying on EXTI.
    if ((s_exti_flag == false) && (s_is_pressed == false))
    {
        return BUTTON_EVENT_NONE;
    }

    s_exti_flag = false;

    GPIO_PinState current = HAL_GPIO_ReadPin(s_port, s_pin);

    if (current != s_raw_state)
    {
        s_raw_state = current;
        s_raw_changed_ms = now_ms;
    }

    // Debounce: accept new stable state only after it stays unchanged for debounce window
    if ((now_ms - s_raw_changed_ms) < s_debounce_ms)
    {
        // Still bouncing
        // But we still can check long press on stable pressed state.
    }
    else
    {
        if (s_stable_state != s_raw_state)
        {
            s_stable_state = s_raw_state;

            if (Button_IsPressed(s_stable_state))
            {
                s_is_pressed = true;
                s_long_fired = false;
                s_pressed_start_ms = now_ms;
                return BUTTON_EVENT_NONE;
            }
            else
            {
                // Released
                bool was_pressed = s_is_pressed;
                bool was_long = s_long_fired;

                s_is_pressed = false;
                s_long_fired = false;

                if (was_pressed && !was_long)
                {
                    return BUTTON_EVENT_SHORT;
                }

                return BUTTON_EVENT_NONE;
            }
        }
    }

    // Long press detection (while button is stably pressed)
    if (s_is_pressed && !s_long_fired)
    {
        if ((now_ms - s_pressed_start_ms) >= s_long_press_ms)
        {
            s_long_fired = true;
            return BUTTON_EVENT_LONG;
        }
    }

    return BUTTON_EVENT_NONE;
}
