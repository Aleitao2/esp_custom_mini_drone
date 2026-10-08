# DroneClaude — DIY ESP32-S3 brushed micro-quad

Line-of-sight first, FPV later.

- **Flight controller:** M5Stack Stamp-S3, firmware based on madflight v2.4.2
- **Motors:** 4 × 720 coreless on discrete MOSFETs
- **Battery:** 1S cell
- **Control:** DualShock 4 → classic ESP32 ground station → ESP-NOW / LoRa

## Layout
| Folder | What |
|---|---|
| `drone/` | ESP32-S3 firmware (PlatformIO). `env:bench` = phase test firmware; `env:flight` arrives in phase 5. |
| `ground/` | Classic ESP32 ground station (Bluepad32), from phase 2 |
| `common/` | Shared link protocol, from phase 3 |
| `docs/` | Decisions, power, motor driver, pins, BOM, budget, radio, safety, phase checklists |

## Phase status
| Phase | Status |
|---|---|
| 0 Power | **in progress** → [docs/phase0.md](docs/phase0.md) |
| 1 Sensors | — |
| 2 Ground station / DS4 | — |
| 3 Radio link + failsafe | — |
| 4 Motors (props off) | — |
| 5 Flight firmware, stabilize | — |
| 6 Altitude hold (baro + ToF) | — |
| 7 Position hold (flow) | — |
| 8 Mag, FPV, range | — |

## Build and flash the drone bench firmware
From PowerShell or the VS Code PlatformIO terminal (not Git Bash):

```powershell
cd "D:\Arduino Proj\DroneClaude\drone"
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e bench -t upload
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" device monitor -b 115200
```

If the Stamp-S3 isn't detected, hold its button (G0) while plugging in USB.

**Safety first:** [docs/safety.md](docs/safety.md). Props stay off until phase 5.
