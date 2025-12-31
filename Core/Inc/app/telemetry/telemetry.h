#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "mavlink/common/mavlink.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct TelemetryState
{
    uint8_t sysid;
    uint8_t compid;

    uint32_t last_hb_ms;
    uint32_t hb_count;

    bool armed;

    // Battery (from SYS_STATUS), optional but useful
    bool has_battery;
    float battery_voltage_v; // V

    // GPS (from GPS_RAW_INT), optional
    bool has_gps;
    uint8_t gps_fix_type;      // 0..6
    uint8_t gps_sats_visible;  // count
    uint32_t last_msg_ms;
    uint32_t msg_count;

} TelemetryState;

void Telemetry_Init(void);
void Telemetry_OnMavlink(const mavlink_message_t* msg, uint32_t now_ms);
void Telemetry_Update(uint32_t now_ms);

// Read-only access to the current state (no ownership transfer).
const TelemetryState* Telemetry_Get(void);

#ifdef __cplusplus
}
#endif
