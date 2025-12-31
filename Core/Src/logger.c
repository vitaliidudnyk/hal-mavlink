#include "logger.h"
#include "logger_sink.h"
#include <stdio.h>
#include <string.h>

void Logger_Init(void)
{
  LoggerSink_Init();
}

void Logger_Write(LogLevel level, const char* tag, const char* fmt, ...)
{
  char buf[256];

  const char* lvl = "?";
  switch (level)
  {
    case LOG_LEVEL_ERROR: lvl = "E"; break;
    case LOG_LEVEL_WARN:  lvl = "W"; break;
    case LOG_LEVEL_INFO:  lvl = "I"; break;
    case LOG_LEVEL_DEBUG: lvl = "D"; break;
  }

  int off = snprintf(buf, sizeof(buf), "[%s] %s: ", lvl, tag);
  if (off < 0 || off >= (int)sizeof(buf)) { return; }

  va_list args;
  va_start(args, fmt);
  int n = vsnprintf(buf + off, sizeof(buf) - (size_t)off, fmt, args);
  va_end(args);

  if (n < 0) { return; }

  size_t len = strnlen(buf, sizeof(buf));
  LoggerSink_Write(buf, len);
  LoggerSink_Write("\r\n", 2);
}
