#pragma once
#include <stdint.h>

#ifdef DEBUG
void DebugHw_InitItm(void);
void DebugHw_SendChar(char c);
#else
static inline void DebugHw_InitItm(void) {}
static inline void DebugHw_SendChar(char c) { (void)c; }
#endif
