# 📘 STM32 MAVLink Health Monitor

## Overview

This project implements a **DMA-based MAVLink health monitor** on STM32.

It receives a continuous MAVLink stream, parses messages, tracks telemetry state,
evaluates system health, and outputs **compressed diagnostic logs once per second**.

The design focuses on:

- non-blocking UART reception
- predictable real-time behavior
- debuggability under high data rates

---

## System Diagram (ASCII)

+-------------------+
| ArduPilot SITL |
| (PC) |
+-------------------+
|
| MAVLink (UDP)
v
+-------------------+
| MAVProxy |
| |
| --out USB-UART |
+-------------------+
|
| USB-UART
v
+-------------------+
| ESP32 |
| |
| UART RX <- PC |
| UART TX -> STM32 |
+-------------------+
|
| UART (DMA RX)
v
+---------------------------------+
| STM32 |
| |
| [UART RX DMA - Circular] |
| | |
| v |
| Software Ring Buffer |
| | |
| v |
| MAVLink Parser |
| | |
| v |
| Telemetry State |
| (HB, Battery, GPS) |
| | |
| v |
| Health Rules (OK/WARN/CRIT) |
| | |
| v |
| 1 Hz Compressed Logs |
+---------------------------------+
|
| UART TX
v
+-------------------+
| ESP32 |
| UDP Logger |
+-------------------+
|
| UDP
v
+-------------------+
| PC (nc / logs) |
+-------------------+

yaml
Copy code

---

## Core Modules

drivers/
uart_rx_ring.c
- DMA circular RX
- software ring buffer
- overflow and drop statistics

protocol/
mavlink_rx.c
- byte-wise MAVLink parsing
- callback on complete message

app/telemetry/
telemetry.c
- last known vehicle state
- heartbeat, battery, GPS tracking

app/mavlink_summary/
mavlink_summary.c
- 1-second message window
- top-3 msgid per window
- link_dt / hb_dt metrics

app/health_rules/
health_rules.c
- OK / WARN / CRIT evaluation

yaml
Copy code

---

## Example Output

Normal operation:

[MAV_SUM] msgs=44 hb=1 link_dt=85ms hb_dt=278ms batt_mv=12100 top=0(1) 30(4) 74(4)
[HEALTH ] lvl=OK sys=1 comp=1 armed=0

yaml
Copy code

After MAVLink loss:

[MAV_SUM] msgs=0 hb=0 link_dt=5092ms hb_dt=9128ms
[HEALTH ] lvl=CRIT

yaml
Copy code

---

## Design Rationale

- **DMA circular RX** prevents data loss under high UART load
- **Software ring buffer** decouples DMA reception from parsing
- **Polling in main loop** avoids ISR-heavy designs
- **Compressed logs** enable debugging over low-bandwidth links

---

## Status

- MVP complete
- Stable under high MAVLink traffic
- Suitable for:
  - UAV health monitoring
  - telemetry gateways
  - embedded diagnostics tools
