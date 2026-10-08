# Design decisions (verified 2026-10-08)

## Base firmware: madflight v2.4.2 (2026-08-18), pinned tag
Checked against the tagged source, not just the README:

| Need | madflight v2.4.2 |
|---|---|
| ESP32-S3, brushed motors | ✅ `out.setup_brushed(n, idx, freq_hz)` (LEDC). `examples/12.StampFly` is an ESP32-S3 brushed micro-quad and is our template. |
| MPU6050 (I2C) | ✅ requires the INT pin (the IMU loop is interrupt driven) |
| BMP280 / BMP180 | ✅ / ❌ |
| QMC5883L / HMC5883L | ✅ / ❌ |
| PMW3901 | ✅ raw flow driver only |
| VL53L1X | ❌ (has VL53L3CX, a different chip) → custom gizmo/driver |
| ADC battery | ✅ |
| Altitude hold | ✅ baro+accel `COPTER_ALTHOLD` (EXPERIMENTAL PID). ToF/flow are not fused into altitude or position, so we port from ESP-Drone. |
| ESP-NOW / LoRa RC | ❌ CRSF/SBUS/DSM/PPM/IBUS/MAVLink only. `rcl.gizmo` is a public `RclGizmo*` with one `update()` method, so our link plugs in without forking. |

Known patches needed:
- `hal_ESP32.cpp` forces I2C to **1 MHz**. Our sensors are rated 400 kHz (VL53L1X datasheet: "up to 400 kHz").
- The RC timeout defaults to 3 s (`RCL_TIMEOUT_US`). Our 250 ms / 1 s failsafe lives in the flight loop.

## Rejected / reference
- **esp-fc** (last release v0.2.1, Feb 2025): no rangefinder, no flow, altitude hold still on its TODO list. Rejected.
- **ESP-Drone** (espressif/esp-drone): reference only.
  - 720-motor option at 15 kHz / 8-bit LEDC.
  - VL53L1X + PMW3901 drivers, Crazyflie `flowdeck_v1v2`, `zranger2`, and complementary/Kalman estimators. These are the sources for phases 6–7.

## Architecture
- **Flight controller = ESP32-S3 (M5Stack Stamp-S3, 3 g).** Ground station = classic ESP32, because the DS4 is Bluetooth Classic (BR/EDR), which only the original ESP32 supports.
- **Bluepad32:** via `esp-idf-arduino-bluepad32-template` 4.2.2 (Arduino core 3.2.1, ESP-IDF 5.4.2, pioarduino 54.03.21). Avoid 5.0.0-beta0 (released 2026-10-05).
  - API confirmed in its example: `BP32.setup`, `BP32.update`, `axisX/axisY/axisRX/axisRY` (−511..512), `throttle/brake` (0..1023), `setColorLED`, `playDualRumble`.
  - ESP-IDF does not support spaces in paths, and this repo lives in `D:\Arduino Proj\…`. The ground build will use a space-free junction or build dir (phase 2).
- **RadioLib 7.8.1** for the SX1278. See [radio-link.md](radio-link.md).
- **Toolchain:** drone builds use pioarduino **55.03.311** (Arduino-ESP32 3.3.11), which needs the `pioarduino` 6.2 core installed in `~/.platformio/penv`.
  - Build from **PowerShell / VS Code**, not Git Bash: ESP-IDF tooling refuses MSys.

## Power
See [power.md](power.md). The brief's power plan holds:
- Motors run direct from the cell.
- Logic runs via the 5 V boost module and the Stamp-S3's own 1 A buck.
- Additions:
  - SS34 OR-diode on the 5 V output, because the Stamp-S3 `5V` pin is tied to USB VBUS.
  - SW1 logic switch for in-circuit charging.
