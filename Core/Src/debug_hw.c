#include "debug_hw.h"

#ifdef DEBUG

#include "stm32f4xx_hal.h"   // ← ВАЖЛИВО: тільки цей include

void DebugHw_InitItm(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    ITM->LAR = 0xC5ACCE55;
    ITM->TER = 1U;
}

void DebugHw_SendChar(char c)
{
    ITM_SendChar((uint32_t)c);
}

#endif
