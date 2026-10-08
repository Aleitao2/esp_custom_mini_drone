# Motor driver — 4 × low-side MOSFET

## One channel (×4)

```
 VBAT+ (cell, direct) ──────────┬───────────────┐
                                │               │
                             ( MOTOR )        ─┴─  SS34 Schottky (cathode band → VBAT+)
                                │              ─┬─
                                ├───────────────┘
                                │ drain
 Stamp-S3 GPIO ──[100 Ω]──┬── gate   AO3400A (SOT-23, N-ch, logic level)
                          │     source
                       [10 kΩ]  │
                          │     │
 PGND (star point) ───────┴─────┴──────────────────────────
```

| Part | Value | Why |
|---|---|---|
| Q | **09N03LA** (yours, DPAK) for now; **AO3400A** (marking `A09T`) later to save weight | 30 V, 5.7 A continuous, 30 A pulsed. R_DS(on) < 48 mΩ at V_GS = 2.5 V and < 32 mΩ at 4.5 V, so ~35–40 mΩ at our 3.3 V gate. At 2 A: ~0.16 W, about +20 °C in SOT-23. SI2302 (`A2SHB`) also works but has ~2× the R_DS(on) and runs warmer. |
| Gate R | 100 Ω | Limits GPIO current spikes into the gate charge (~7 nC) and damps ringing. Edges stay ≈0.2–0.4 µs, about 1 % of a 20 kHz period. |
| Gate pull-down | 10 kΩ gate→source | Keeps the motor **off** while the MCU is resetting, booting or unpowered. |
| Flyback | **SS34** (3 A, 40 V Schottky, SMA) | Carries motor current during PWM off-time. A regular 1N400x is **not acceptable**: its slow reverse recovery at 20 kHz causes current spikes, heating and EMI. SS14 / 1N5819 (1 A) are acceptable fallbacks. |
| Bulk | 470–1000 µF, ≥6.3 V (10 V preferred), low-ESR, plus 10 µF ceramic | Supplies the PWM current edges locally so the battery leads' inductance doesn't make voltage spikes. Mount it **at the driver board**, across VBAT+/PGND, ≤2 cm from the FETs. |

### Your MOSFETs: 09N03LA (Infineon IPD09N03LA, DPAK/TO-252) — OK to use
Data from the 09N03LA family datasheet (IPB09N03LA G):

| Parameter | Value | Verdict |
|---|---|---|
| "LA" | Logic level | ✅ |
| V_GS(th) | 1.2 V min, 1.6 V typ, 2.0 V max | ✅ |
| R_DS(on) at 4.5 V | ≤ 15 mΩ | ✅ |
| R_DS(on) at 3.3 V | Not specified. Expected ~20–40 mΩ at 2 A: 3.3 V is 1.3–2 V above threshold, and these are 50 A parts. | ✅ probably; verify on the bench |
| Gate charge | ≈ 2× the AO3400A | Edges ~0.5–1 µs with 100 Ω. Switching loss ≈ 0.1 W per FET at 20 kHz, fine. If they get warm, drop the gate resistor to 47 Ω. |
| Weight | 0.375 g each (1.5 g for 4) | ⚠️ AO3400A is 0.01 g each, so switching later saves ~1.4 g |

They are often salvaged from PC motherboards (CPU power stage). Before soldering, check each one with the multimeter in diode mode:
- Red probe on source, black on drain: ~0.4–0.7 V (body diode).
- Reversed: open.
- Gate to anything: open.

**Bench verification (phase 4, single motor + prop on the bench PSU, through the driver at 100 %):** measure drain→source voltage. V_DS < 0.1 V at ~2 A means R_DS(on) < 50 mΩ, which is good.

DPAK on perfboard: the large tab is the **drain**. Solder the tab flat onto a pad cluster (it is also the heatsink). Gate is the left leg, source the right leg (the middle leg is drain, usually cut short).

### Other MOSFET types, for reference
- **Suitable:** small SOT-23 parts marked `A09T` (AO3400A) or `A2SHB` (SI2302).
- **Too small:** 2N7002 / BSS138 (≤0.3 A).
- **Not suitable:** TO-220 parts such as IRFZ44N, IRF540N, IRF3205. They are not fully on at 3.3 V gate drive, and they weigh ~2 g each.
- **P-channel MOSFETs are not needed.**

### Caps you have
Read each capacitor's label: capacitance (µF) and voltage (V). Look for:
- one **470 µF to 1000 µF, 6.3 V or 10 V** (low-ESR / "105 °C" / polymer). Bigger and heavier is not better: aim for ≤1 g.
- one **22 µF** (5 V input of the Stamp-S3) and a couple of **10 µF** (SX1278, driver board).
- several **100 nF** ceramics.

## PWM frequency

| Firmware | Brushed PWM |
|---|---|
| madflight StampFly example | 5 kHz |
| ESP-Drone (720 motors) | 15 kHz, 8-bit |
| Crazyflie | 328 kHz, 8-bit |
| **This build (default)** | **20 kHz, 10-bit** (LEDC clock = 40 MHz crystal on the ESP32-S3 in Arduino-ESP32 3.x; 40 MHz / 20 kHz = 2000 → 10 bits = 1024 steps) |

- **Why 20 kHz:** it's above hearing, and 1024 steps (0.1 %) is plenty for smooth PID output (Crazyflie and ESP-Drone use 256). Switching loss stays small.
- **Why not lower:** coreless motors have very low inductance, so lower frequencies make the current ripple large (worse efficiency) and audible.
- **Why not higher:** higher frequencies lose resolution and increase switching loss.
- **Phase 4 will measure it:** the bench CLI `freq` command switches 8 / 16 / 20 / 32 kHz. Compare current at equal thrust and FET temperature, then set the flight value in madflight (`out.setup_brushed(4, idx, freq_hz)`).

## Boot safety (no twitch)
- **Pins G39–G42** have no pull-up and no boot-time activity on the ESP32-S3. M5Stack's own StampFly drone uses G41/G42 for motors.
- **Hardware:** the 10 kΩ pull-downs keep the gates low until firmware runs.
- **Firmware:** the bench firmware drives the pins LOW as the very first thing in `setup()`. The flight firmware must do the same.
- **Test (phase 0, props off):** press reset ~10 times and unplug/replug power ~10 times. No motor may twitch.

## Perfboard layout (≈ 22 × 26 mm, ~2.5 g populated)

```
   VBAT+ bus (bare 0.8 mm tinned wire along the top edge) ──────────────
   [C1 470-1000µF]   D1   D2   D3   D4      (SS34, cathode to VBAT+ bus)
                      │    │    │    │
   M1+ M1-  M2+ M2-  M3+ M3-  M4+ M4-       (motor pads; M+ on VBAT+ bus)
                      │    │    │    │
                     Q1   Q2   Q3   Q4      (AO3400A: drain up, source down)
                    Rg/Rpd ×4               (100 Ω to header, 10 k to PGND)
   PGND bus (bare wire along the bottom edge) ─── STAR POINT at C1(−)
   ─ 5-pin header: G39 G40 G41 G42 GND(logic, single wire to star) ─
```
- **SOT-23 on 2.54 mm perfboard:** tack each FET onto three pads with short tinned-wire bridges, or use SOT-23→DIP adapter boards (~0.1 g each).
- **High-current paths** (VBAT+ bus, PGND bus, battery leads): 22–24 AWG silicone wire or solid bare wire, as short as possible. Motor leads are the factory ones.
- **Mounting:** keep the board at the frame centre, with the battery leads entering next to C1.
