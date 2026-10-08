# Bill of materials — have vs. buy

## Have

| Item | Use | Status |
|---|---|---|
| M5Stack Stamp-S3 (3.0 g) | Flight controller | ✅ |
| ESP32 WROOM-32 dev board | Ground station (Bluetooth Classic for DS4) | ✅ |
| MPU6050 (GY-521) | IMU | ✅ wire the **INT** pin |
| HW-291 baro | Altitude above ToF range | chip TBD in phase 1 (BMP280 = madflight driver; BMP180 = small driver needed) |
| GY-271 | Magnetometer (phase 8) | chip TBD in phase 1 (QMC5883L supported by madflight) |
| TOF400C (VL53L1X) | Height 0–1.3 m | ✅ driver to add (madflight has VL53L3CX only) |
| PMW3901 | Optical flow | ✅ madflight driver |
| 2 × XL1278-SMT (SX1278) | LoRa link | ✅ |
| DualShock 4 | Controller | ✅ |
| 4 × 720 coreless motors | Propulsion | ✅ check shaft diameter (0.8 or 1.0 mm) for props |
| IP16350 650 mAh, 13400 550 mAh (vape cells) | Flight battery | ⚠️ unknown discharge rating; IR test in phase 0; **re-wrap** (wrappers torn) |
| EL-574950PU 2200 mAh pouch | **Ground station** battery | ✅ too heavy for the drone |
| Type-C charger + 5 V boost module | Logic 5 V + charging | ⚠️ change R_PROG to 3.3 kΩ, set pot to 5.00 V |
| N/P MOSFETs | Motor driver? | ❓ send the marking; AO3400A/SI2302 OK, TO-220 parts not |
| Regular diodes | — | ❌ not for flyback (too slow at 20 kHz) |
| Low-ESR capacitors | Bulk/decoupling | ❓ check values against the list below |
| AMS1117-3.3 | Ground station (optional) | Not usable on the drone: needs ≥4.4 V input |
| Connectors, perfboard | Wiring | ✅ |
| Jewelry scale (0.001 g), bench PSU, multimeter, 3D printer | Tools | ✅ check scale max capacity (needs ≥ ~60 g for the thrust test) |

## Buy

| Qty | Part | Why | Approx. weight |
|---|---|---|---|
| 6 | **SS34** Schottky, SMA (or SS14 / 1N5819) | 4 flyback + 1 OR-diode on the 5 V output + 1 spare | 0.05 g each |
| 6 | **AO3400A** SOT-23 (only if your MOSFETs aren't suitable) | Motor switches | 0.01 g each |
| 2 sets | **55 mm props for 720 motors**, 2 CW + 2 CCW per set, hole = your shaft | Flight + spares | 0.4 g each |
| 1 set | 65 mm props for 720 motors (optional) | Thrust comparison | |
| 1 | 3.3 kΩ resistor (0603/0805 SMD or 1/8 W) | TP4056 R_PROG → 364 mA | |
| 4 + 4 | 100 Ω, 10 kΩ resistors | Gate resistor / pull-down | |
| 4 | 100 kΩ resistors | VBAT and 5 V-sense dividers | |
| 1 | 470 µF 6.3–10 V low-ESR (only if you don't have one ≤1 g) | Motor bulk cap | ~0.6 g |
| 1 pair | **BT2.0** battery connector (or high-current JST-PH 2.0) | Battery, ≥ 8 A pulses | ~0.6 g |
| 1 | Mini slide switch | SW1 logic switch | 0.3 g |
| — | 22–26 AWG silicone wire, heat-shrink, cell insulator rings | Wiring, re-wrapping cells | |
| (later) | 1S LiPo 300–450 mAh ≥ 30C | Only if the vape cells fail the IR test | 8–12 g |
