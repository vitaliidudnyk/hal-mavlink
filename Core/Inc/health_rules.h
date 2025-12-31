#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum HealthLevel
{
    HEALTH_OK = 0,
    HEALTH_WARN = 1,
    HEALTH_CRIT = 2
} HealthLevel;

void HealthRules_Init(void);
void HealthRules_Update(uint32_t now_ms);

#ifdef __cplusplus
}
#endif
