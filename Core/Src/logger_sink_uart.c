#include "logger_sink.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stddef.h>

extern UART_HandleTypeDef huart2;

#define LOGGER_SINK_UART_TX_BUF_SIZE       1024u
#define LOGGER_SINK_UART_WRITE_CHUNK_MAX   64u

static uint8_t s_tx_buf[LOGGER_SINK_UART_TX_BUF_SIZE];
static volatile uint16_t s_head = 0u;
static volatile uint16_t s_tail = 0u;

static volatile uint16_t s_dma_len_in_flight = 0u;
static volatile uint8_t  s_dma_in_progress = 0u;

/* Diagnostics */
static volatile uint32_t s_dropped_bytes   = 0u;
static volatile uint32_t s_overflow_events = 0u;
static volatile uint32_t s_dma_errors      = 0u;

static uint32_t EnterCritical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void ExitCritical(uint32_t primask)
{
    __set_PRIMASK(primask);
}

static uint16_t FreeSpace_NoLock(void)
{
    if (s_head >= s_tail)
    {
        return (uint16_t)(LOGGER_SINK_UART_TX_BUF_SIZE - (s_head - s_tail) - 1u);
    }
    else
    {
        return (uint16_t)((s_tail - s_head) - 1u);
    }
}

static uint16_t ContiguousBytes_NoLock(void)
{
    if (s_head >= s_tail)
    {
        return (uint16_t)(s_head - s_tail);
    }
    else
    {
        return (uint16_t)(LOGGER_SINK_UART_TX_BUF_SIZE - s_tail);
    }
}

static void TryStartDma_NoLock(void)
{
    if (s_dma_in_progress != 0u)
    {
        return;
    }

    uint16_t chunk = ContiguousBytes_NoLock();
    if (chunk == 0u)
    {
        return;
    }

    s_dma_in_progress = 1u;
    s_dma_len_in_flight = chunk;

    if (HAL_UART_Transmit_DMA(&huart2, &s_tx_buf[s_tail], chunk) != HAL_OK)
    {
        s_dma_in_progress = 0u;
        s_dma_len_in_flight = 0u;
        s_dma_errors++;
    }
}

void LoggerSink_Init(void)
{
    // DMA + NVIC configured in CubeMX
}

void LoggerSink_Write(const char* data, size_t len)
{
    if (data == NULL || len == 0u)
    {
        return;
    }

    size_t offset = 0u;

    while (offset < len)
    {
        uint32_t primask = EnterCritical();

        uint16_t free_space = FreeSpace_NoLock();
        if (free_space == 0u)
        {
            s_overflow_events++;
            s_dropped_bytes += (uint32_t)(len - offset);
            ExitCritical(primask);
            return;
        }

        uint16_t to_write = (uint16_t)(len - offset);
        if (to_write > free_space)
        {
            to_write = free_space;
        }
        if (to_write > LOGGER_SINK_UART_WRITE_CHUNK_MAX)
        {
            to_write = LOGGER_SINK_UART_WRITE_CHUNK_MAX;
        }

        for (uint16_t i = 0u; i < to_write; i++)
        {
            s_tx_buf[s_head] = (uint8_t)data[offset + i];
            s_head = (uint16_t)((s_head + 1u) % LOGGER_SINK_UART_TX_BUF_SIZE);
        }

        TryStartDma_NoLock();

        ExitCritical(primask);
        offset += to_write;
    }
}

void LoggerSinkUart_GetStats(LoggerSinkUart_Stats* out_stats)
{
    if (out_stats == NULL)
    {
        return;
    }

    uint32_t primask = EnterCritical();

    out_stats->dropped_bytes   = s_dropped_bytes;
    out_stats->overflow_events = s_overflow_events;
    out_stats->dma_errors      = s_dma_errors;

    ExitCritical(primask);
}

/* ===== Routed from stm32_uart_callbacks.c ===== */

void LoggerSinkUart_OnTxComplete(UART_HandleTypeDef* huart)
{
    if (huart != &huart2)
    {
        return;
    }

    uint32_t primask = EnterCritical();

    s_tail = (uint16_t)((s_tail + s_dma_len_in_flight) % LOGGER_SINK_UART_TX_BUF_SIZE);
    s_dma_len_in_flight = 0u;
    s_dma_in_progress = 0u;

    TryStartDma_NoLock();

    ExitCritical(primask);
}

void LoggerSinkUart_OnError(UART_HandleTypeDef* huart)
{
    if (huart != &huart2)
    {
        return;
    }

    uint32_t primask = EnterCritical();

    s_dma_errors++;
    s_dma_in_progress = 0u;
    s_dma_len_in_flight = 0u;

    TryStartDma_NoLock();

    ExitCritical(primask);
}
