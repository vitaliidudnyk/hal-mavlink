// uart_rx_ring.c
#include "uart_rx_ring.h"
#include <string.h>
#include "logger.h"

// Critical section: protects SW ring indices and stats vs ISR.
// For this stage, we use IRQ disable (simple, reliable).
static uint32_t UartRxRing_EnterCritical(void)
{
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void UartRxRing_ExitCritical(uint32_t primask)
{
    __set_PRIMASK(primask);
}

static uint16_t UartRxRing_SwFreeSpace_NoLock(const UartRxRing* ring)
{
    uint16_t head = ring->sw_head;
    uint16_t tail = ring->sw_tail;

    // Keep one byte empty to distinguish full vs empty.
    if (head >= tail)
    {
        return (uint16_t)(ring->sw_size - (head - tail) - 1u);
    }
    else
    {
        return (uint16_t)((tail - head) - 1u);
    }
}

static void UartRxRing_SwPushByte_NoLock(UartRxRing* ring, uint8_t b)
{
    uint16_t free_space = UartRxRing_SwFreeSpace_NoLock(ring);
    if (free_space == 0u)
    {
        ring->overflow_events++;
        ring->dropped_bytes++;
        return;
    }

    ring->sw_buf[ring->sw_head] = b;
    ring->sw_head = (uint16_t)((ring->sw_head + 1u) % ring->sw_size);
    ring->pushed_bytes++;
}

void UartRxRing_Init(UartRxRing* ring, UART_HandleTypeDef* huart)
{
    if (ring == NULL)
    {
        return;
    }

    ring->huart = huart;

    ring->dma_size = (uint16_t)UART_RX_RING_DMA_BUF_SIZE;
    ring->dma_last_pos = 0u;

    ring->sw_size = (uint16_t)UART_RX_RING_SW_BUF_SIZE;
    ring->sw_head = 0u;
    ring->sw_tail = 0u;

    ring->pushed_bytes = 0u;
    ring->dropped_bytes = 0u;
    ring->overflow_events = 0u;

    // Clear DMA buffer for debug readability (not required)
    (void)memset(ring->dma_buf, 0, ring->dma_size);
}

HAL_StatusTypeDef UartRxRing_StartDma(UartRxRing* ring)
{
    if (ring == NULL || ring->huart == NULL)
    {
        return HAL_ERROR;
    }

    ring->dma_last_pos = 0u;

    // Receive continuously into DMA circular buffer.
    // IMPORTANT: DMA must be configured in CubeMX as Circular.
    return HAL_UART_Receive_DMA(ring->huart, ring->dma_buf, ring->dma_size);
}

static uint16_t UartRxRing_GetDmaWritePos(const UartRxRing* ring)
{
    // For circular DMA reception, NDTR decrements from dma_size to 0 and reloads.
    // Current write position = dma_size - NDTR
    // NOTE: Accessing DMA handle fields is common in embedded, but depends on HAL internals.
    // For STM32F4 HAL, this is typically safe.
    if (ring->huart == NULL || ring->huart->hdmarx == NULL)
    {
        return 0u;
    }

    uint16_t ndtr = (uint16_t)__HAL_DMA_GET_COUNTER(ring->huart->hdmarx);
    uint16_t pos = (uint16_t)(ring->dma_size - ndtr);

    if (pos >= ring->dma_size)
    {
        pos = 0u;
    }

    return pos;
}

void UartRxRing_PollFromDma(UartRxRing* ring)
{
    if (ring == NULL || ring->huart == NULL)
    {
        return;
    }

    uint16_t pos = UartRxRing_GetDmaWritePos(ring);
    uint16_t last = ring->dma_last_pos;

    if (pos == last)
    {
        return; // no new data
    }

    // Calculate how many bytes are new in DMA since last poll (for diagnostics).
    uint16_t moved = 0u;
    if (pos > last)
    {
        moved = (uint16_t)(pos - last);
    }
    else
    {
        moved = (uint16_t)((ring->dma_size - last) + pos);
    }

    // Move bytes from DMA buffer [last..pos) into SW ring.
    uint32_t primask = UartRxRing_EnterCritical();

    if (pos > last)
    {
        for (uint16_t i = last; i < pos; i++)
        {
            UartRxRing_SwPushByte_NoLock(ring, ring->dma_buf[i]);
        }
    }
    else
    {
        for (uint16_t i = last; i < ring->dma_size; i++)
        {
            UartRxRing_SwPushByte_NoLock(ring, ring->dma_buf[i]);
        }
        for (uint16_t i = 0u; i < pos; i++)
        {
            UartRxRing_SwPushByte_NoLock(ring, ring->dma_buf[i]);
        }
    }

    ring->dma_last_pos = pos;

    UartRxRing_ExitCritical(primask);

    // ---- DIAGNOSTICS (log once per second) ----
    // NOTE: Keep this lightweight. If needed, you can guard by a compile-time flag.
//    static uint32_t s_last_log_ms = 0u;
//    uint32_t now = HAL_GetTick();
//    if ((now - s_last_log_ms) >= 1000u)
//    {
//        s_last_log_ms = now;
//
//        // Optional: show SW ring occupancy too.
//        uint16_t avail = UartRxRing_Available(ring);
//
//        Logger_Write(LOG_LEVEL_INFO, "UART1_RX_DMA",
//            "pos=%u last=%u moved=%u avail=%u pushed=%lu drop=%lu ovf=%lu",
//            (unsigned)pos,
//            (unsigned)last,
//            (unsigned)moved,
//            (unsigned)avail,
//            (unsigned long)ring->pushed_bytes,
//            (unsigned long)ring->dropped_bytes,
//            (unsigned long)ring->overflow_events);
//    }
}


uint16_t UartRxRing_Available(const UartRxRing* ring)
{
    if (ring == NULL)
    {
        return 0u;
    }

    uint32_t primask = UartRxRing_EnterCritical();

    uint16_t head = ring->sw_head;
    uint16_t tail = ring->sw_tail;
    uint16_t avail = 0u;

    if (head >= tail)
    {
        avail = (uint16_t)(head - tail);
    }
    else
    {
        avail = (uint16_t)(ring->sw_size - tail + head);
    }

    UartRxRing_ExitCritical(primask);

    return avail;
}

uint16_t UartRxRing_Read(UartRxRing* ring, uint8_t* out, uint16_t max_len)
{
    if (ring == NULL || out == NULL || max_len == 0u)
    {
        return 0u;
    }

    uint16_t read_count = 0u;

    uint32_t primask = UartRxRing_EnterCritical();

    while (read_count < max_len)
    {
        if (ring->sw_tail == ring->sw_head)
        {
            break; // empty
        }

        out[read_count] = ring->sw_buf[ring->sw_tail];
        ring->sw_tail = (uint16_t)((ring->sw_tail + 1u) % ring->sw_size);
        read_count++;
    }

    UartRxRing_ExitCritical(primask);

    return read_count;
}

void UartRxRing_GetStats(const UartRxRing* ring, UartRxRing_Stats* out_stats)
{
    if (ring == NULL || out_stats == NULL)
    {
        return;
    }

    uint32_t primask = UartRxRing_EnterCritical();

    out_stats->pushed_bytes = ring->pushed_bytes;
    out_stats->dropped_bytes = ring->dropped_bytes;
    out_stats->overflow_events = ring->overflow_events;

    UartRxRing_ExitCritical(primask);
}
