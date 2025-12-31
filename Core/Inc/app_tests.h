#pragma once

#include "mavlink_rx.h"

#ifdef __cplusplus
extern "C" {
#endif

// App-level tests/diagnostics module.
// Keep product logic out of here. Add small, targeted test helpers.

void AppTests_Init(void);

// Call from App_Update() once per main loop iteration.
void AppTests_Update(void);

// Specific tests:
void AppTest_MavlinkRx_LogRxStatsOncePerSecond(const MavlinkRx* mav_rx);

#ifdef __cplusplus
}
#endif
