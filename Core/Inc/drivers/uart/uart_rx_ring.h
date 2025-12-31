// uart_rx_ring.h
#pragma once
#include <stdint.h>
#include <stddef.h>
#include "stm32f4xx_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

// Simple UART RX ring built around a DMA circular buffer.
//
// Data flow:
//   DMA writes into dma_buf[] (circular)
//   We periodically call UartRxRing_PollFromDma() to move new bytes into sw ring
//   Consumer reads from sw ring via UartRxRing_Read()

#ifndef UART_RX_RING_DMA_BUF_SIZE
#define UART_RX_RING_DMA_BUF_SIZE 256u
#endif

#ifndef UART_RX_RING_SW_BUF_SIZE
#define UART_RX_RING_SW_BUF_SIZE 512u
#endif

typedef struct
{
    uint32_t pushed_bytes;     // total bytes moved into SW ring
    uint32_t dropped_bytes;    // bytes dropped due to SW ring full
    uint32_t overflow_events;  // number of times SW ring overflow happened
} UartRxRing_Stats;

typedef struct
{
    UART_HandleTypeDef* huart;

    // DMA circular buffer (DMA writes here)
    uint8_t  dma_buf[UART_RX_RING_DMA_BUF_SIZE];
    uint16_t dma_size;
    volatile uint16_t dma_last_pos;

    // Software ring buffer (consumer reads from here)
    uint8_t  sw_buf[UART_RX_RING_SW_BUF_SIZE];
    uint16_t sw_size;
    volatile uint16_t sw_head; // write index
    volatile uint16_t sw_tail; // read index

    // Diagnostics
    volatile uint32_t pushed_bytes;
    volatile uint32_t dropped_bytes;
    volatile uint32_t overflow_events;
} UartRxRing;

void UartRxRing_Init(UartRxRing* ring, UART_HandleTypeDef* huart);

// Starts DMA reception into internal circular DMA buffer.
// Must be called once after init (and after UART is configured).
HAL_StatusTypeDef UartRxRing_StartDma(UartRxRing* ring);

// Polls DMA write pointer and copies newly received bytes from DMA buffer
// into SW ring buffer.
// Call from main loop (e.g., every 1-10 ms) OR from an IDLE callback.
void UartRxRing_PollFromDma(UartRxRing* ring);

// Reads up to max_len bytes from SW ring into out.
// Returns number of bytes read.
uint16_t UartRxRing_Read(UartRxRing* ring, uint8_t* out, uint16_t max_len);

// Returns how many bytes currently available in SW ring.
uint16_t UartRxRing_Available(const UartRxRing* ring);

// Read-only stats snapshot.
void UartRxRing_GetStats(const UartRxRing* ring, UartRxRing_Stats* out_stats);

#ifdef __cplusplus
}
#endif
