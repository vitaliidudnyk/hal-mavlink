#pragma once
#include <stddef.h>
#include <stdint.h>
#include "stm32f4xx_hal.h"

typedef struct
{
    uint32_t dropped_bytes;
    uint32_t overflow_events;
    uint32_t dma_errors;
} LoggerSinkUart_Stats;

void LoggerSink_Init(void);
void LoggerSink_Write(const char* data, size_t len);
void LoggerSinkUart_GetStats(LoggerSinkUart_Stats* out_stats);

/* Called from HAL callbacks router */
void LoggerSinkUart_OnTxComplete(UART_HandleTypeDef* huart);
void LoggerSinkUart_OnError(UART_HandleTypeDef* huart);
