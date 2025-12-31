#include "app/telemetry/mavlink_summary.h"
#include "app/telemetry/telemetry.h"
#include "logger.h"
#include <string.h>

typedef struct TopMsg
{
    uint8_t id;
    uint16_t count;
} TopMsg;

typedef struct MavSumLogFields
{
    uint32_t link_dt;
    uint32_t hb_dt;

    uint8_t armed;

    uint8_t has_batt;
    float batt_v;

    uint8_t has_gps;
    uint8_t gps_fix;
    uint8_t sats;
    uint16_t batt_mv;

} MavSumLogFields;


static void MavSummary_FillLogFields(MavSumLogFields* out, uint32_t now_ms, const struct TelemetryState* tlm)
{
    if (out == NULL)
    {
        return;
    }

    // Defaults mean "unknown / not available"
    out->link_dt = 0xFFFFFFFFu;
    out->hb_dt   = 0xFFFFFFFFu;

    out->armed = 0u;

    out->has_batt = 0u;
    out->batt_v = 0.0f;
    out->batt_mv = 0u;

    out->has_gps = 0u;
    out->gps_fix = 0u;
    out->sats = 0u;

    if (tlm == NULL)
    {
        return;
    }

    out->link_dt = (tlm->last_msg_ms == 0u) ? 0xFFFFFFFFu : (now_ms - tlm->last_msg_ms);
    out->hb_dt   = (tlm->last_hb_ms  == 0u) ? 0xFFFFFFFFu : (now_ms - tlm->last_hb_ms);

    out->armed = (uint8_t)(tlm->armed ? 1u : 0u);

    out->has_batt = (uint8_t)(tlm->has_battery ? 1u : 0u);
    out->batt_v = tlm->battery_voltage_v;

    // batt_mv is logged as an integer to avoid ambiguity if float formatting is truncated.
    if (out->has_batt != 0u)
    {
        float mv_f = out->batt_v * 1000.0f;
        if (mv_f < 0.0f)
        {
            mv_f = 0.0f;
        }
        if (mv_f > 65535.0f)
        {
            mv_f = 65535.0f;
        }
        out->batt_mv = (uint16_t)(mv_f + 0.5f);
    }
    else
    {
        out->batt_mv = 0u;
    }

    out->has_gps = (uint8_t)(tlm->has_gps ? 1u : 0u);
    out->gps_fix = tlm->gps_fix_type;
    out->sats = tlm->gps_sats_visible;
}


static void MavSummary_FindTop3(const MavlinkSummary* self, TopMsg out_top[3])
{
    if (out_top == NULL)
    {
        return;
    }

    out_top[0] = (TopMsg){0u, 0u};
    out_top[1] = (TopMsg){0u, 0u};
    out_top[2] = (TopMsg){0u, 0u};

    if (self == NULL)
    {
        return;
    }

    // If no messages in the window, keep top as zeros.
    if (self->win_msgs == 0u)
    {
        return;
    }

    for (uint16_t i = 0u; i < MAV_SUM_MSGID_MAX; i++)
    {
        uint16_t c = self->win_msgid_counts[i];
        if (c == 0u)
        {
            continue;
        }

        // For equal counts, prefer smaller msgid to keep output stable.
        uint8_t id = (uint8_t)i;

        if ((c > out_top[0].count) || ((c == out_top[0].count) && (id < out_top[0].id)))
        {
            out_top[2] = out_top[1];
            out_top[1] = out_top[0];
            out_top[0] = (TopMsg){ id, c };
        }
        else if ((c > out_top[1].count) || ((c == out_top[1].count) && (id < out_top[1].id)))
        {
            out_top[2] = out_top[1];
            out_top[1] = (TopMsg){ id, c };
        }
        else if ((c > out_top[2].count) || ((c == out_top[2].count) && (id < out_top[2].id)))
        {
            out_top[2] = (TopMsg){ id, c };
        }
    }
}


void MavlinkSummary_Init(MavlinkSummary* self, uint32_t period_ms)
{
    if (self == NULL)
    {
        return;
    }

    (void)memset(self, 0, sizeof(*self));

    self->period_ms = (period_ms == 0u) ? 1000u : period_ms;
    self->last_log_ms = 0u;
}

void MavlinkSummary_OnMessage(MavlinkSummary* self, const mavlink_message_t* msg)
{
    if (self == NULL || msg == NULL)
    {
        return;
    }

    self->total_msgs++;
    self->win_msgs++;

    uint8_t id = (uint8_t)msg->msgid;

    self->last_msgid = id;
    self->last_sysid = msg->sysid;
    self->last_compid = msg->compid;

    // Count per-msgid frequency in the current window.
    if (id < MAV_SUM_MSGID_MAX)
    {
        self->win_msgid_counts[id]++;
    }

    // Heartbeat counters must match msgid==0
    if (msg->msgid == MAVLINK_MSG_ID_HEARTBEAT)
    {
        self->total_hb++;
        self->win_hb++;
    }

    // Important messages (window counters)
    switch (msg->msgid)
    {
        case MAVLINK_MSG_ID_SYS_STATUS:
        {
            self->win_sys_status++;
            break;
        }
        case MAVLINK_MSG_ID_GPS_RAW_INT:
        {
            self->win_gps_raw_int++;
            break;
        }
        case MAVLINK_MSG_ID_ATTITUDE:
        {
            self->win_attitude++;
            break;
        }
        default:
        {
            break;
        }
    }
}

void MavlinkSummary_UpdateAndLog(MavlinkSummary* self, uint32_t now_ms, const struct TelemetryState* tlm)
{
    if (self == NULL)
    {
        return;
    }

    // Log only once per period.
    if ((now_ms - self->last_log_ms) < self->period_ms)
    {
        return;
    }
    self->last_log_ms = now_ms;

    // Collect fields from telemetry state (read-only)
    MavSumLogFields f;
    MavSummary_FillLogFields(&f, now_ms, tlm);

    // Compute top-3 message IDs for this window (optional but very useful)
    TopMsg top[3];
    MavSummary_FindTop3(self, top);

    // One unified log line (no branching)
    // NOTE: has_batt/has_gps indicate whether batt_v/gps_fix/sats are valid.
    Logger_Write(LOG_LEVEL_INFO, "MAV_SUM",
        "msgs=%lu hb=%lu link_dt=%lums hb_dt=%lums armed=%u has_batt=%u batt=%.2fV has_gps=%u gps_fix=%u sats=%u "
        "last=%u sys=%u comp=%u top=%u(%u) %u(%u) %u(%u)",
        (unsigned long)self->win_msgs,
        (unsigned long)self->win_hb,
        (unsigned long)f.link_dt,
        (unsigned long)f.hb_dt,
        (unsigned)f.armed,
        (unsigned)f.has_batt,
        (double)f.batt_v,
        (unsigned)f.has_gps,
        (unsigned)f.gps_fix,
        (unsigned)f.sats,
        (unsigned)self->last_msgid,
        (unsigned)self->last_sysid,
        (unsigned)self->last_compid,
        (unsigned)top[0].id, (unsigned)top[0].count,
        (unsigned)top[1].id, (unsigned)top[1].count,
        (unsigned)top[2].id, (unsigned)top[2].count);

    // Reset window after logging
    self->win_msgs = 0u;
    self->win_hb = 0u;

    (void)memset(self->win_msgid_counts, 0, sizeof(self->win_msgid_counts));
    self->win_sys_status = 0u;
    self->win_gps_raw_int = 0u;
    self->win_attitude = 0u;
}
