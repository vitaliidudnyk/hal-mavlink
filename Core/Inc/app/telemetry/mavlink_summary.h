#pragma once

#include <stdint.h>
#include "mavlink/common/mavlink.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAV_SUM_MSGID_MAX 256u

struct TelemetryState;

typedef struct MavlinkSummary
{
    // Accumulated totals (since boot)
    uint32_t total_msgs;
    uint32_t total_hb;

    // Window counters (since last log)
    uint32_t win_msgs;
    uint32_t win_hb;

    // Last seen message (for quick insight)
    uint8_t last_msgid;
    uint8_t last_sysid;
    uint8_t last_compid;

    // Timing
    uint32_t last_log_ms;
    uint32_t period_ms;

    uint16_t win_msgid_counts[MAV_SUM_MSGID_MAX];

	// Important message counters (window)
	uint16_t win_sys_status;
	uint16_t win_gps_raw_int;
	uint16_t win_attitude;

} MavlinkSummary;

void MavlinkSummary_Init(MavlinkSummary* self, uint32_t period_ms);
void MavlinkSummary_OnMessage(MavlinkSummary* self, const mavlink_message_t* msg);
void MavlinkSummary_UpdateAndLog(MavlinkSummary* self, uint32_t now_ms, const struct TelemetryState* tlm);

#ifdef __cplusplus
}
#endif
