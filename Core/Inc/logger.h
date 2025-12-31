#pragma once
#include <stdarg.h>

typedef enum
{
  LOG_LEVEL_ERROR,
  LOG_LEVEL_WARN,
  LOG_LEVEL_INFO,
  LOG_LEVEL_DEBUG
} LogLevel;

void Logger_Init(void); // init current sink (uart/swv)
void Logger_Write(LogLevel level, const char* tag, const char* fmt, ...);
