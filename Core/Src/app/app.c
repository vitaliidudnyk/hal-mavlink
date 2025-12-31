#include "app/app.h"
#include "button.h"
#include "led_fsm.h"
#include "logger.h"
#include "stm32f4xx_hal.h"
#include "mavlink_rx.h"
#include "stm32f4xx_hal.h"
#include "app_tests.h"
#include "mavlink/common/mavlink.h"
#include "app/telemetry/telemetry.h"
#include "health_rules.h"


extern UART_HandleTypeDef huart1;

static MavlinkRx s_mav_rx;

static LedMode s_mode = LED_MODE_BLINK;

static void OnMavlinkMessage(void* ctx, const mavlink_message_t* msg);

static LedMode App_NextMode(LedMode mode)
{
    switch (mode)
    {
        case LED_MODE_OFF:   return LED_MODE_BLINK;
        case LED_MODE_BLINK: return LED_MODE_SOS;
        case LED_MODE_SOS:   return LED_MODE_OFF;
        default:             return LED_MODE_OFF;
    }
}

void App_Init(void)
{
	AppTests_Init();
	Led_Init(GPIOC, GPIO_PIN_13);

	Telemetry_Init();
	HealthRules_Init();

	Logger_Write(LOG_LEVEL_INFO, "App_Init", "start");
    Led_SetMode(LED_MODE_BLINK);
    s_mode = LED_MODE_BLINK;

    // Button on PA0, active low (pressed = RESET) is common for many boards.
    Button_Init(GPIOA, GPIO_PIN_0, GPIO_PIN_RESET);

    Logger_Init();

    MavlinkRx_Init(&s_mav_rx, &huart1);
    MavlinkRx_SetOnMessage(&s_mav_rx, OnMavlinkMessage, NULL);
    HAL_StatusTypeDef status = MavlinkRx_Start(&s_mav_rx);
    Logger_Write(LOG_LEVEL_INFO, "App_Init", "MavlinkRx_Start status=%d", (int)status);


    Logger_Write(LOG_LEVEL_INFO, "BOOT", "app init ok");
}

void App_Update(uint32_t now_ms)
{
	MavlinkRx_Update(&s_mav_rx);

	Telemetry_Update(now_ms);
	HealthRules_Update(now_ms);

	//AppTest_MavlinkRx_LogRxStatsOncePerSecond(&s_mav_rx);

    Led_Update(now_ms);

    ButtonEvent ev = Button_Update(now_ms);
    if (ev == BUTTON_EVENT_NONE)
    {
        return;
    }

    if (ev == BUTTON_EVENT_SHORT)
    {
        s_mode = App_NextMode(s_mode);
        Led_SetMode(s_mode);
        Logger_Write(LOG_LEVEL_INFO, "BTN", "short press | mode=%d", (int)s_mode);
    }
    else if (ev == BUTTON_EVENT_LONG)
    {
        s_mode = LED_MODE_OFF;
        Led_SetMode(s_mode);
        Logger_Write(LOG_LEVEL_INFO, "BTN", "long press | mode=%d", (int)s_mode);
    }
}

static void OnMavlinkMessage(void* ctx, const mavlink_message_t* msg)
{
    (void)ctx;
    Telemetry_OnMavlink(msg, HAL_GetTick());
}
