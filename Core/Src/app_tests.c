#include "app_tests.h"
#include "logger.h"
#include "stm32f4xx_hal.h"

void AppTests_Init(void)
{
    // Nothing to init for now.
}

void AppTests_Update(void)
{
    // Intentionally empty.
    // We call specific tests explicitly from app.c to keep control and clarity.
}

void AppTest_MavlinkRx_LogRxStatsOncePerSecond(const MavlinkRx* mav_rx)
{
    if (mav_rx == NULL)
    {
        return;
    }

    static uint32_t s_last_ms = 0u;
    uint32_t now = HAL_GetTick();
    if ((now - s_last_ms) < 1000u)
    {
        return;
    }
    s_last_ms = now;

    UartRxRing_Stats st;
    // mav_rx->rx_ring is an internal field, so we use the public API.
    // But our API currently expects non-const (it doesn't modify state). Cast is safe here.
    MavlinkRx_GetRxStats((MavlinkRx*)mav_rx, &st);

    Logger_Write(
        LOG_LEVEL_INFO,
        "[TEST][MAV RX]",
        "pushed=%lu dropped=%lu ovf=%lu",
        (unsigned long)st.pushed_bytes,
        (unsigned long)st.dropped_bytes,
        (unsigned long)st.overflow_events
    );
}
