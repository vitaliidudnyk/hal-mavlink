#include "stm32_uart_callbacks.h"
#include "logger_sink.h"

/*
 * NOTE:
 * HAL callbacks are GLOBAL for the whole project.
 * Do NOT put these functions into driver modules.
 */

void HAL_UART_TxCpltCallback(UART_HandleTypeDef* huart)
{
    // Route to logger UART sink
    LoggerSinkUart_OnTxComplete(huart);

    // Future:
    // MavlinkUart_OnTxComplete(huart);
    // GpsUart_OnTxComplete(huart);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef* huart)
{
    // Route to logger UART sink
    LoggerSinkUart_OnError(huart);

    // Future:
    // MavlinkUart_OnError(huart);
}
