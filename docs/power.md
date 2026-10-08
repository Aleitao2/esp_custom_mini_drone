# Power architecture (phase 0)

Status: **design verified on paper, bench tests pending** (see [phase0.md](phase0.md)).

## Decision (kept from the brief, verified for the Stamp-S3)

| Rail | Source | Why |
|---|---|---|
| Motor rail | Cell → battery connector → MOSFET driver board, **direct** | 4–8 A peaks. A boost can't supply that and can't regulate below its input anyway. |
| Logic 5 V | Cell → charger/boost module (set to **5.0 V**) → SS34 diode → switch → Stamp-S3 `5V` pin | Keeps the MCU alive while motor current sags the cell. |
| Logic 3.3 V | Stamp-S3 onboard **Cyntec MUN3CAD01-SC** buck (2.7–5.5 V in, 1 A out) → Stamp-S3 `3V3` pin → all sensors + SX1278 | An efficient buck, not an LDO. The boost→buck chain is about 75–80 % efficient overall. |

The original worry ("5 V boost → dev-board LDO wastes ~40 %") does not apply. The Stamp-S3 has a 1 A buck, not an AMS1117. So the boost module stays.

Weight-saving option for later (not now): skip the boost and feed the cell straight into the Stamp-S3 `5V` pin. The buck accepts 2.7 V, but 3.3 V output is only guaranteed while the cell stays above about 3.4–3.5 V under load. With vape cells that sag hard, that is too risky until the cell has been measured. Saving: ~3 g.

## Schematic

```
                     battery connector (BT2.0 or high-current PH2.0)
 CELL+ ──┬──────────────────────┬───────────────────────────────────────────────┐
         │                      │                                               │
     [100k]                 B+ ┌┴──────────────────────────┐                    │
         ├──► G1 (VBAT)        │  CHARGER + BOOST MODULE   │                    │
     [100k] ═ 100nF            │  USB-C = charge input     │                    │
         │                     │  OUT+ (trim pot → 5.00 V) ├──►|──o/ o──┬───────┼──► Stamp-S3 5V
         │                     │                           │  SS34  SW1 │       │
         │                  B− │  OUT−                     │       22µF ═       │
         │                     └┬────────────┬─────────────┘            │       │
         │                      │            │                          │       │
         │                      │            └──────────────────────────┤       │
         │                      │                                       │       │
         │        ┌─────────────┴──── MOTOR DRIVER BOARD ───────────────┼───────┤
         │        │  C1 470-1000 µF low-ESR ═ + 10 µF ceramic           │       │
         │        │  4 × (motor, SS34 flyback, AO3400A)  see motor-driver.md    │
         │        └─────────────┬──────────────────────────────────────┘        │
 CELL− ──┴──────────────────────●  STAR POINT (on the driver board, at C1's − leg)
                                │
                                └──── single wire ──── Stamp-S3 GND ── sensors / SX1278 GND
```

### Notes
- **Bench workflow (decided):** everything is tested on the bench PSU first; the battery comes last. **USB and boost are never connected at the same time** (SW1 off whenever USB is plugged in). See [phase0.md](phase0.md).
- **SS34 on the 5 V output (diode-OR), recommended insurance.** The Stamp-S3 `5V` pin connects straight to USB VBUS; its schematic shows only a polyfuse between them. Without the diode, plugging USB in while the battery is connected ties the PC's 5 V to the boost output. With the diode, the higher source wins and nothing back-feeds. The Stamp-S3 sees ~4.6 V from the boost, which is fine for the buck.
  - Consequence: **while USB is plugged in, the logic runs from USB, not the boost.** The real "logic on boost under motor load" test therefore runs untethered. The bench firmware logs it to flash, see [phase0.md](phase0.md).
- **SW1 (logic switch)** lets you charge the cell in-circuit with the electronics off. The battery connector is the master power.
- **Always unplug the battery for storage.** The module's boost and charger chips draw a few mA even when idle. That would over-discharge an unprotected cell within days.
- **Ground / star point.** All motor return current goes MOSFET sources → star point → cell−. The logic ground joins the star point with one wire, so motor current never flows through sensor grounds.
  - Check with the multimeter whether module **B−** and **OUT−** are the same net (continuity). If they are *not* (protection MOSFETs in the negative path), still tie OUT− to the star point. That bypasses the module's protection FETs, which is acceptable because the motors bypass them anyway. Low-voltage protection is done in firmware (3.5 V warning / 3.3 V land under load).

## Logic current budget (3.3 V rail, worst case)

| Load | Peak mA | Typical mA |
|---|---|---|
| ESP32-S3, Wi-Fi/ESP-NOW TX at 19.5 dBm | ~340 | ~100 (CPU + periodic TX) |
| SX1278 TX (+17 dBm PA_BOOST / +20 dBm) | 90 / 120 | ~10 (low-duty telemetry) |
| VL53L1X (datasheet: avg 16–18 mA, peak 40 mA) | 40 | 18 |
| PMW3901 | ~10 | ~10 |
| MPU6050 + baro + mag | ~6 | ~6 |
| WS2812 LED (dim) | ~10 | ~5 |
| **Total** | **~520** (both radios never TX together) | **~150** |

- **Buck (1 A):** OK.
- **5 V side:** 3.3 V × 0.52 A / 0.9 ≈ 1.9 W → 0.38 A at 5 V. Module rated 2 A: OK.
- **Cell side at 3.3 V:** ~0.7 A peak, ~0.2 A typical (boost ~85 %).
- **Still to verify on the bench:** the module's minimum input voltage, measured in phase 0 step 3.

## Battery voltage measurement
- **Divider:** 100 kΩ / 100 kΩ, plus 100 nF from G1 to GND. 4.2 V reads as 2.1 V at the pin, well inside the ADC's 11 dB range. It draws 21 µA. Note that this always-on drain is another reason to unplug the battery.
- **Firmware:** reads with `analogReadMilliVolts()` (eFuse calibrated), then applies `vcal`, a single scale factor set from your multimeter (`vcal <volts>` in the bench CLI).
- **Thresholds (flight firmware, phase 4/5):**

| Condition | Threshold |
|---|---|
| Low-battery warning (under load) | 3.5 V |
| Auto-land (under load) | 3.3 V |
| Disarm (under load), or resting voltage below 3.0 V | 3.2 V |

## Brownout and reset logging
- ESP32-S3 brownout detection is enabled by default in Arduino-ESP32 3.3 (`CONFIG_ESP_BROWNOUT_DET_LVL 7`). It watches the **3.3 V rail**, not the battery. With boost + buck in front, a brownout reset means the supply chain failed (boost dropout or overcurrent), and that is exactly what phase 0 tests.
- The bench firmware stores boots, brownouts, panics, watchdog resets and the last load-test result in NVS (flash). The flight firmware will do the same and send the last reset reason in telemetry.

## Charging (TP4056-class charger on the module)

**The module ships set to 1 A.** The resistor marked `1201` (= 1.2 kΩ) next to the 8-pin charger chip is almost certainly R_PROG. TP4056: I_charge = 1200 / R_PROG(kΩ) mA → 1.2 kΩ = 1000 mA. That is **1.5C for the 650 mAh cell and 1.8C for the 550 mAh cell, too high.**

**Fit R_PROG = 3.3 kΩ → 364 mA:**

| Cell | Capacity | Charge rate |
|---|---|---|
| IP16350 | 650 mAh | 0.56C ✅ |
| 13400 | 550 mAh | 0.66C ✅ |
| EL-574950PU pouch (ground station) | 2200 mAh | 0.17C (slow but fine) |

Before changing it:
1. Read the marking on the 8-pin chip. If it is TP4056 or a clone (TC4056, 4056H…), the formula applies. Anything else: send me the marking.
2. Confirm with continuity that the `1201` resistor goes from chip **pin 2 (PROG)** to GND. TP4056 pinout: 1 TEMP, 2 PROG, 3 GND, 4 VCC, 5 BAT, 6 STDBY, 7 CHRG, 8 CE.
3. Replace it with 3.3 kΩ. Parallel resistors only *lower* resistance and raise the current, so the old one must come off.

**Verify without an ammeter:** while charging a partly empty cell, measure the voltage on PROG (pin 2) to GND.
- During constant-current charging it reads ≈ **1.0 V**, and current = 1.0 V / 3.3 kΩ × 1200 ≈ 364 mA.
- As the cell fills (constant-voltage phase), PROG voltage falls proportionally. Charging terminates at about 36 mA (PROG ≈ 0.1 V).

**In-circuit charging:** OK with **SW1 off** (logic off, motor gates held low by pull-downs), on a non-flammable surface, supervised. Don't charge with the logic on: the load current keeps the charger from ever terminating.

The 2200 mAh pouch (EL-574950PU, 3.8 V nominal, so probably a 4.35 V "HV" cell) is too heavy for the drone. Use it for the **ground station**. A 4.2 V TP4056 charge is safe for it, just ~5–8 % short of full.
