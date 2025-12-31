#include "app/telemetry/telemetry.h"
#include <string.h>

#include "app/telemetry/mavlink_summary.h"

static TelemetryState s_tlm;
static MavlinkSummary s_sum;

void Telemetry_Init(void)
{
    (void)memset(&s_tlm, 0, sizeof(s_tlm));

    // Log one summary line per second.
    MavlinkSummary_Init(&s_sum, 1000u);
}

void Telemetry_OnMavlink(const mavlink_message_t* msg, uint32_t now_ms)
{
    (void)now_ms;

    if (msg == NULL)
    {
        return;
    }

    // Update summary aggregator (log output happens in Telemetry_Update).
    MavlinkSummary_OnMessage(&s_sum, msg);

    // Remember the source of the last message (often useful for debugging)
    s_tlm.sysid = msg->sysid;
    s_tlm.compid = msg->compid;
    s_tlm.last_msg_ms = now_ms;
    s_tlm.msg_count++;

    switch (msg->msgid)
    {
        case MAVLINK_MSG_ID_HEARTBEAT:
        {
            mavlink_heartbeat_t hb;
            mavlink_msg_heartbeat_decode(msg, &hb);

            s_tlm.last_hb_ms = now_ms;
            s_tlm.hb_count++;

            // MAV_MODE_FLAG_SAFETY_ARMED indicates "armed" state
            s_tlm.armed = ((hb.base_mode & MAV_MODE_FLAG_SAFETY_ARMED) != 0u);
            break;
        }

        case MAVLINK_MSG_ID_SYS_STATUS:
        {
            mavlink_sys_status_t st;
            mavlink_msg_sys_status_decode(msg, &st);

            // battery_voltage is in millivolts. UINT16_MAX means "unknown".
            if (st.voltage_battery != UINT16_MAX)
            {
                s_tlm.has_battery = true;
                s_tlm.battery_voltage_v = ((float)st.voltage_battery) * 0.001f;
            }
            break;
        }

        case MAVLINK_MSG_ID_GPS_RAW_INT:
        {
            mavlink_gps_raw_int_t gps;
            mavlink_msg_gps_raw_int_decode(msg, &gps);

            s_tlm.has_gps = true;
            s_tlm.gps_fix_type = gps.fix_type;
            s_tlm.gps_sats_visible = gps.satellites_visible;
            break;
        }

        default:
        {
            // Ignore other messages in MVP state tracking
            break;
        }
    }
}

void Telemetry_Update(uint32_t now_ms)
{
    // Emit compressed log once per second.
    MavlinkSummary_UpdateAndLog(&s_sum, now_ms, &s_tlm);
}

const TelemetryState* Telemetry_Get(void)
{
    return &s_tlm;
}
