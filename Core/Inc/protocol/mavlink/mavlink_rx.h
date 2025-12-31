#pragma once

#include <stdint.h>
#include "stm32f4xx_hal.h"
#include "uart_rx_ring.h"
#include "mavlink/common/mavlink.h"

#ifdef __cplusplus
extern "C" {
#endif


typedef void (*MavlinkRx_OnMessageFn)(void* ctx,
#ifdef USE_MAVLINK_C_LIB
                                     const mavlink_message_t* msg
#else
                                     uint8_t dummy
#endif
);

typedef struct
{
    UART_HandleTypeDef* huart;
    UartRxRing rx_ring;

    // Optional message callback
    MavlinkRx_OnMessageFn on_message;
    void* on_message_ctx;

#ifdef USE_MAVLINK_C_LIB
    mavlink_status_t mav_status;
#endif
} MavlinkRx;

void MavlinkRx_Init(MavlinkRx* self, UART_HandleTypeDef* huart);
HAL_StatusTypeDef MavlinkRx_Start(MavlinkRx* self);

// Call frequently from main loop.
// It will poll DMA and consume bytes from RX ring.
// If MAVLink parsing enabled, it will emit messages via callback.
void MavlinkRx_Update(MavlinkRx* self);

void MavlinkRx_SetOnMessage(MavlinkRx* self, MavlinkRx_OnMessageFn fn, void* ctx);

// Optional: diagnostics passthrough
void MavlinkRx_GetRxStats(MavlinkRx* self, UartRxRing_Stats* out_stats);

#ifdef __cplusplus
}
#endif
