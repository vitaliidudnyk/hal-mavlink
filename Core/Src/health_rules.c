#include "health_rules.h"
#include "app/telemetry/telemetry.h"
#include "logger.h"

// --- Tuning knobs (MVP) ---
#ifndef HEALTH_RULES_LOG_PERIOD_MS
#define HEALTH_RULES_LOG_PERIOD_MS 1000u
#endif

#ifndef HEALTH_HEARTBEAT_WARN_MS
#define HEALTH_HEARTBEAT_WARN_MS 2000u
#endif

#ifndef HEALTH_HEARTBEAT_CRIT_MS
#define HEALTH_HEARTBEAT_CRIT_MS 5000u
#endif

#ifndef HEALTH_LINK_WARN_MS
#define HEALTH_LINK_WARN_MS 2000u
#endif

#ifndef HEALTH_LINK_CRIT_MS
#define HEALTH_LINK_CRIT_MS 5000u
#endif

#ifndef HEALTH_BATT_WARN_V
#define HEALTH_BATT_WARN_V 10.8f
#endif

#ifndef HEALTH_BATT_CRIT_V
#define HEALTH_BATT_CRIT_V 10.5f
#endif

static uint32_t s_last_log_ms = 0u;

static HealthLevel HealthRules_Evaluate(const TelemetryState* t, uint32_t now_ms)
{
    if (t == NULL)
    {
        return HEALTH_CRIT;
    }

    HealthLevel level = HEALTH_OK;

    // Heartbeat timeout rule
    uint32_t dt_hb = (t->last_hb_ms == 0u) ? 0xFFFFFFFFu : (now_ms - t->last_hb_ms);
    uint32_t dt_link = (t->last_msg_ms == 0u) ? 0xFFFFFFFFu : (now_ms - t->last_msg_ms);

    // Link timeout rule (primary): if no MAVLink at all -> WARN/CRIT
    if (dt_link >= HEALTH_LINK_CRIT_MS)
    {
        level = HEALTH_CRIT;
    }
    else if (dt_link >= HEALTH_LINK_WARN_MS)
    {
        level = HEALTH_WARN;
    }

    // Heartbeat timeout rule (secondary): if link is alive, do NOT escalate to CRIT
    // but we may raise WARN to indicate heartbeat is stale.
    if (level != HEALTH_CRIT)
    {
        if (dt_hb >= HEALTH_HEARTBEAT_WARN_MS)
        {
            if (level < HEALTH_WARN)
            {
                level = HEALTH_WARN;
            }
        }
    }

    // Battery rule (only if available)
    if (t->has_battery)
    {
        if (t->battery_voltage_v <= HEALTH_BATT_CRIT_V)
        {
            level = HEALTH_CRIT;
        }
        else if (t->battery_voltage_v <= HEALTH_BATT_WARN_V)
        {
            if (level < HEALTH_WARN)
            {
                level = HEALTH_WARN;
            }
        }
    }

    // GPS rule (only if available)
    if (t->has_gps)
    {
        // Fix type: 0/1 = no fix, 2 = 2D, 3 = 3D...
        if (t->gps_fix_type < 2u)
        {
            if (level < HEALTH_WARN)
            {
                level = HEALTH_WARN;
            }
        }
    }

    return level;
}

static const char* HealthRules_LevelToStr(HealthLevel lvl)
{
    switch (lvl)
    {
        case HEALTH_OK:   return "OK";
        case HEALTH_WARN: return "WARN";
        case HEALTH_CRIT: return "CRIT";
        default:          return "UNK";
    }
}

void HealthRules_Init(void)
{
    s_last_log_ms = 0u;
}

void HealthRules_Update(uint32_t now_ms)
{
    const TelemetryState* t = Telemetry_Get();
    HealthLevel lvl = HealthRules_Evaluate(t, now_ms);

    // Log summary once per second (MVP)
    if ((now_ms - s_last_log_ms) < HEALTH_RULES_LOG_PERIOD_MS)
    {
        return;
    }
    s_last_log_ms = now_ms;

    uint32_t dt_hb = (t->last_hb_ms == 0u) ? 0xFFFFFFFFu : (now_ms - t->last_hb_ms);
    uint32_t dt_link = (t->last_msg_ms == 0u) ? 0xFFFFFFFFu : (now_ms - t->last_msg_ms);


    Logger_Write(LOG_LEVEL_INFO, "HEALTH",
    	"lvl=%s sys=%u comp=%u armed=%u link_dt=%lu hb_dt=%lu hb_count=%lu batt=%.2fV gps_fix=%u sats=%u",
        HealthRules_LevelToStr(lvl),
        (unsigned)t->sysid,
        (unsigned)t->compid,
        (unsigned)(t->armed ? 1u : 0u),
		(unsigned long)dt_link,
        (unsigned long)dt_hb,
        (unsigned long)t->hb_count,
        (double)(t->has_battery ? t->battery_voltage_v : 0.0f),
        (unsigned)(t->has_gps ? t->gps_fix_type : 0u),
        (unsigned)(t->has_gps ? t->gps_sats_visible : 0u));
}
