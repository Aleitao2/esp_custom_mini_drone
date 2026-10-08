# Phase 0 — power bring-up (checklist)

Send me the numbers in **bold** as you go. Steps are ordered so you can start before the purchases arrive.

| Needs | Steps |
|---|---|
| Nothing new | 1, 2*, 3, 4, 5 |
| A 3.3 kΩ resistor | *2 |
| Props | 6, 7 |
| SS34 diodes (+ MOSFETs if yours are unsuitable) | 8, 9, 10 |

## 1. Cells — inspect and measure (no charging yet)
- [ ] Measure the resting voltage of each cell: **V_16350 = ?, V_13400 = ?, V_pouch = ?**
- [ ] Re-wrap both vape cells (see [safety.md](safety.md)).

## 2. Charger — fix the charge current
- [ ] Read the marking on the 8-pin chip next to USB-C: **marking = ?**
- [ ] Continuity check: the `1201` resistor connects chip pin 2 (PROG) to GND.
- [ ] Replace it with 3.3 kΩ.
- [ ] Charge a partly empty cell from USB-C. Measure PROG (pin 2) to GND: **V_PROG = ?** (expect ≈ 1.0 V → 364 mA).
- [ ] Let it finish: the charge LED changes and the cell reads ≈ 4.20 V.

## 3. Boost — set and characterise
- [ ] Bench PSU at 3.70 V, current limit 1 A, into **B+/B−** (not the 5 V output!).
- [ ] Measure OUT+ to OUT− with no load and **turn the pot until it reads 5.00 V**: **V_out = ?** Then put a drop of nail polish on the pot so vibration can't move it.
- [ ] Continuity B− ↔ OUT−: **same net? yes/no**
- [ ] Weigh the module: **? g**

## 4. Flash the bench firmware (Stamp-S3 on USB only, nothing else connected)
- [ ] Plug the Stamp-S3 into the PC. If it isn't detected, hold the **G0 button while plugging in** (download mode).
- [ ] I flash it from here (`pio run -e bench -t upload`), or you click PlatformIO → Upload in VS Code.
- [ ] Serial monitor (115200): the LED goes **blue**, and `status` prints a reset reason and counters.

## 5. Weigh everything
- [ ] Fill in the "measured" column of [budget.md](budget.md), or just send me the list. Remove pin headers first where you can.

## 6. ⚠️ Single-motor thrust and current test (PROPS ON — one motor, clamped)
- **Rig:** hot-glue/tape one motor **upside down** (shaft pointing down) to the top of a ~10 cm stand (e.g. a piece of wood or a printed post) on the scale. The prop then blows air **up**, away from the scale pan, and the reading goes **up** by the thrust.
- **Check first:** the scale's max capacity is above stand + motor + ~40 g.
- Bench PSU, current limit **3 A**, motor wires straight to the PSU (no MCU). Safety glasses, fingers clear.
- [ ] Record at 3.4 V, 3.7 V and 4.0 V: **current (A) and thrust (g)**.
- [ ] Repeat with 65 mm props if you have them.

## 7. Cell internal-resistance (IR) test
- [ ] Charge the cell full, rest 30 min, measure **V_open**.
- [ ] Connect the step-6 motor + prop directly to the cell for ~3 s and read the cell voltage *during* the run: **V_load**.
- [ ] IR = (V_open − V_load) / I, using the current from step 6 at the nearest voltage. Do both cells: **IR_16350 = ?, IR_13400 = ?**
- [ ] Pass: ≤ 60 mΩ (see [budget.md](budget.md)).

## 8. Build the motor driver board + wiring
- [ ] Build the driver per [motor-driver.md](motor-driver.md) and the power wiring per [power.md](power.md):
  - divider on G1,
  - optional 5 V-sense divider on G2,
  - SS34 + SW1 on the 5 V line,
  - star ground.
- [ ] Battery connected, USB connected, **props off**. `status` shows VBAT. Then `vcal <your multimeter reading>`. Pass: `status` agrees with the meter within ±0.05 V.
- [ ] **Twitch test:** motors connected, props off. Press the reset (EN) ~10 times and unplug/replug the battery ~10 times. **No motor may move.**
- [ ] `arm PROPS-OFF`, then `m 1 30`, `m 2 30`, `m 3 30`, `m 4 30`. Check each motor matches its position in [pin-table.md](pin-table.md) and spins the right way: CW for M1/M4, CCW for M2/M3, seen from above. Swap the motor's two wires to reverse it.

## 9. Load test — logic on the boost (props off)
1. **Tethered first:** `arm PROPS-OFF`, then `load 5 tx` (all motors 100 % + ESP-NOW TX at max power). Expect `completed` and a sag value.
2. **Untethered:** this is the real one, because with USB plugged in the logic runs from USB.
   - `btntest on`, unplug USB, switch SW1 on (logic now on the boost).
   - Hold **BOOT (G0) for 2 s**: the LED blinks red for 3 s, then the motors run at 100 % for 5 s.
   - Plug USB back in, then type `log` and `status`.
   - **Pass:** test `completed`, brownouts = 0.
3. **Boost minimum input:** power B+/B− from the bench PSU instead of the cell. `tx on`, unplug USB, then lower the PSU from 3.7 V in 0.1 V steps until the LED stops/resets: **V_min = ?** (want ≤ 3.0 V). Replug USB: `status` shows the reset reason.
4. `btntest off` when done.

## 10. ⚠️ Optional: all-motors load test with PROPS ON, clamped
This is the only test that applies the real 6–8 A flight current to the cell, the driver and the boost together.
- **Setup:** all four motors hot-glued to a heavy wooden block (not a frame), props on, block clamped to the bench. Glasses on, stand back.
- **Ramp up:** `all 30 2`, `all 60 2`, then `load 3`.
- **Record:** sag and completed/brownout from `log`. Do it only after steps 8–9 pass.

## Phase 0 is done when
- [ ] Charge current 364 mA verified (V_PROG ≈ 1.0 V)
- [ ] Boost set to 5.00 V; minimum input voltage known
- [ ] No twitch on reset/power-cycle
- [ ] VBAT reading within ±0.05 V of the multimeter
- [ ] Load test completed tethered and untethered with brownouts = 0
- [ ] Thrust/current per motor and both cells' IR measured; cell chosen; budget updated
