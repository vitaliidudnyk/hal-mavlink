#include "mavlink_rx.h"
#include <stddef.h>
#include "mavlink/common/mavlink.h"
#include "logger.h"

#define USE_MAVLINK_C_LIB 1
#ifndef MAVLINK_RX_READ_CHUNK
#define MAVLINK_RX_READ_CHUNK 64u
#endif

void MavlinkRx_Init(MavlinkRx* self, UART_HandleTypeDef* huart)
{
    if (self == NULL)
    {
        return;
    }

    self->huart = huart;
    self->on_message = NULL;
    self->on_message_ctx = NULL;

#ifdef USE_MAVLINK_C_LIB
    // Reset MAVLink parser status
    self->mav_status.parse_state = 0;
    self->mav_status.packet_rx_success_count = 0;
    self->mav_status.packet_rx_drop_count = 0;
#endif

    UartRxRing_Init(&self->rx_ring, huart);
}

HAL_StatusTypeDef MavlinkRx_Start(MavlinkRx* self)
{
    if (self == NULL)
    {
        return HAL_ERROR;
    }

    return UartRxRing_StartDma(&self->rx_ring);
}

void MavlinkRx_SetOnMessage(MavlinkRx* self, MavlinkRx_OnMessageFn fn, void* ctx)
{
    if (self == NULL)
    {
        return;
    }

    self->on_message = fn;
    self->on_message_ctx = ctx;
}

void MavlinkRx_GetRxStats(MavlinkRx* self, UartRxRing_Stats* out_stats)
{
    if (self == NULL)
    {
        return;
    }

    UartRxRing_GetStats(&self->rx_ring, out_stats);
}

void MavlinkRx_Update(MavlinkRx* self)
{
    if (self == NULL)
    {
        return;
    }

    // Move new bytes from DMA circular buffer into SW ring buffer
    UartRxRing_PollFromDma(&self->rx_ring);

    // Drain SW ring in small chunks to keep loop responsive
    uint8_t buf[MAVLINK_RX_READ_CHUNK];

    while (UartRxRing_Available(&self->rx_ring) > 0u)
    {
        uint16_t n = UartRxRing_Read(&self->rx_ring, buf, (uint16_t)sizeof(buf));
        if (n == 0u)
        {
            break;
        }

#ifdef USE_MAVLINK_C_LIB
        // Feed bytes to MAVLink parser
        for (uint16_t i = 0u; i < n; i++)
        {
            mavlink_message_t msg;
            if (mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &self->mav_status) != 0u)
            {
                if (self->on_message != NULL)
                {
                    self->on_message(self->on_message_ctx, &msg);
                }
            }
        }

        //DIAG
//        static uint32_t s_last_log_ms = 0u;
//        uint32_t now = HAL_GetTick();
//        if ((now - s_last_log_ms) >= 1000u)
//        {
//            s_last_log_ms = now;
//            Logger_Write(LOG_LEVEL_INFO, "MAV_PARSER",
//                "ok=%lu drop=%lu",
//                (unsigned long)self->mav_status.packet_rx_success_count,
//                (unsigned long)self->mav_status.packet_rx_drop_count);
//        }
#else
        // No MAVLink library yet: just consume bytes.
        // Callback not called in this mode.
        (void)buf;
#endif
    }
}
