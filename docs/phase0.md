# Phase 0 — power bring-up (checklist)

**Plan (your choice): everything on the bench power supply first; the battery comes last.** The bench PSU stands in for the cell: it connects where the battery connector will be.

Send me the numbers in **bold** as you go.

## Rule: USB and boost never together
The Stamp-S3 `5V` pin is wired directly to USB 5 V. If USB is plugged in while the boost module also feeds `5V`, the two 5 V sources push against each other and into your PC's USB port.
- **USB plugged in** → boost output disconnected from the Stamp-S3 (SW1 off, or the wire unplugged). The logic runs from USB; motors still run from the PSU. Grounds are shared.
- **Boost connected** → USB unplugged.
- The SS34 diode in [power.md](power.md) makes mistakes harmless. Fit it when your SS34s arrive.

Bench PSU defaults: **3.70 V**, current limit **1 A** (props off) or **3 A** (one motor with prop). **Check polarity twice**: nothing on the board survives reverse polarity.

---

## 1. Firmware on the Stamp-S3 (USB only) — ✅ flashed 2026-10-08
- [ ] Serial monitor at 115200 (Arduino IDE: set "Newline" line ending). Type `status`.
  - Expect a battery reading (VBAT) of ~0 mV, since nothing is connected yet.
  - The LED should be **blue**.
  - If nothing appears, unplug and replug the USB cable (that power-cycles the board).

## 2. Charger/boost module on the PSU
- [ ] PSU 3.70 V, limit 1 A → module **B+/B−** (not the 5 V output!).
- [ ] Measure OUT+ to OUT− with no load and **turn the pot until it reads 5.00 V**: **V_out = ?** Then put a drop of nail polish on the pot.
- [ ] Continuity B− ↔ OUT−: **same net? yes/no**
- [ ] Lower the PSU slowly to find where the 5 V output collapses with no load: **V_min_noload = ?**

## 3. ⚠️ Single-motor thrust and current test (PROPS ON — one motor, clamped)
- **Rig:** hot-glue/tape one motor **upside down** (shaft pointing down) to the top of a ~10 cm stand on the scale. The prop blows air **up**, away from the pan, so the reading goes **up** by the thrust.
- **Check first:** the scale's max capacity is above stand + motor + ~40 g.
- PSU current limit **3 A**, motor wires straight to the PSU (no MCU). Safety glasses, fingers clear.
- [ ] Record at 3.4 V, 3.7 V and 4.0 V: **PSU current (A) and thrust (g)**. Check that the prop direction blows air up; if not, swap the motor wires.

## 4. Build the motor driver board + wiring
- [ ] Driver per [motor-driver.md](motor-driver.md), using your 09N03LA (diode-test them first) and SS34 flyback diodes. **Don't run motors through the driver without flyback diodes.**
- [ ] Wiring per [power.md](power.md):
  - VBAT divider on G1,
  - optional 5 V-sense divider on G2,
  - star ground,
  - boost → SW1 → Stamp-S3 `5V`.
- [ ] PSU on the battery input (3.70 V, limit 1 A). **SW1 off**, USB plugged in.
- [ ] Measure the voltage at the driver input with the multimeter, then type `vcal <reading>`. Pass: `status` agrees within ±0.05 V. Set the PSU to 3.3 V and 4.2 V and check again.

## 5. Motor tests through the driver (PROPS OFF)
- [ ] **Twitch test:** motors connected, PSU on, props off. Unplug/replug USB ~10 times. **No motor may move.**
- [ ] `arm PROPS-OFF`, then `m 1 30`, `m 2 30`, `m 3 30`, `m 4 30`.
  - Each motor must match its position in [pin-table.md](pin-table.md).
  - It must spin the right way: CW for M1/M4, CCW for M2/M3, seen from above. Swap the motor's two wires to reverse it.
- [ ] `all 100 5`: check the current on the PSU and feel the MOSFETs (should be barely warm).

## 6. Logic on the boost (USB unplugged, props off)
1. **Button load test:**
   - With USB in: `btntest on`.
   - Unplug USB, then **SW1 on** (logic now runs PSU → boost → Stamp-S3).
   - Hold **BOOT (G0) for 2 s**: the LED blinks red for 3 s, then the motors run at 100 % for 5 s.
   - **SW1 off**, plug USB in, then type `log` and `status`.
   - **Pass:** test `completed`, brownouts = 0.
2. **Boost minimum input under real load:**
   - With USB in: `tx on`.
   - Unplug USB, SW1 on.
   - Lower the PSU from 3.7 V in 0.1 V steps until the LED goes off or blinks: **V_min = ?** (want ≤ 3.0 V).
   - SW1 off, USB in: `status` shows the reset reason.
3. With USB in: `btntest off`.

## 7. Battery — last
- [ ] Measure each cell's resting voltage (**V_16350, V_13400, V_pouch**) and re-wrap both vape cells (see [safety.md](safety.md)).
- [ ] Charger: read the chip marking, replace `1201` with **3.3 kΩ**, then while charging measure **V_PROG ≈ 1.0 V**.
- [ ] **IR test:** cell full, rested 30 min, measure **V_open**. Run the step-3 motor + prop straight from the cell for 3 s and read **V_load**. IR = ΔV / I, using the current from step 3: **IR_16350 = ?, IR_13400 = ?**
- [ ] Repeat steps 5–6 on the battery instead of the PSU.
- [ ] Optional ⚠️ all-motors props-on load test (motors glued to a heavy clamped block): `all 30 2`, `all 60 2`, `load 3`, then `log`.

## Phase 0 is done when
- [ ] Boost set to 5.00 V; minimum input voltage known
- [ ] No twitch on reset/power-cycle
- [ ] VBAT reading within ±0.05 V of the multimeter
- [ ] Load test completed with logic on the boost, brownouts = 0
- [ ] Thrust/current per motor measured; budget updated
- [ ] (battery) Charge current 364 mA verified; IR measured; cell chosen
