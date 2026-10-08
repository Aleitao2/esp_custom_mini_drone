# Weight, thrust and flight-time budget

Status: **preliminary**. Replace every "est." with your jewelry-scale reading (phase 0, step 5). The thrust row gets replaced by the single-motor thrust test (phase 0, step 6).

## Weight

| Item | est. g | measured g |
|---|---|---|
| Frame (PETG/PLA, target) | 9.0 | |
| Stamp-S3 | 3.0 | 3.0 |
| MPU6050 (GY-521, pin headers removed) | 1.3 | |
| Baro (HW-291) | 1.0 | |
| VL53L1X (TOF400C) | 1.5 | |
| PMW3901 breakout | 2.0 | |
| SX1278 module + 17 cm wire antenna | 1.8 | |
| MOSFET driver board (populated) | 2.5 | |
| Charger/boost module | 3.0 | |
| Wiring, battery connector, switch, diode | 4.0 | |
| 4 × 720 motors (3.4 g each per listing) | 13.6 | |
| 4 × 55 mm props | 1.6 | |
| **Subtotal without battery** | **44.3** | |
| + IP16350 650 mAh | 13.8 | 13.8 |
| + 13400 550 mAh | 10.6 | 10.6 |
| GY-271 magnetometer (phase 8 only) | +1.2 | |
| Future AIO FPV camera reserve | +5.0 | |

| Configuration | AUW (g) |
|---|---|
| 16350, no camera | **58.1** |
| 13400, no camera | **54.9** |
| 16350 + camera | 63.1 |
| 13400 + camera | 59.9 |

Leave the magnetometer off until phase 8: it saves 1.2 g, and it's the least useful sensor indoors (motor currents distort it anyway).

## Thrust (estimate, to be measured)

- **Motor:** 720 coreless, ~48 krpm no-load at 3.7 V (listing), so Kv ≈ 13 000 rpm/V. Your no-load 120 mA fits that.
- **Prop:** 55 mm (the standard prop for 720 motors). Momentum/prop-coefficient estimate (C_T ≈ 0.09, C_P ≈ 0.05) at a loaded 28–32 krpm and ~3.6 V gives **25–30 g thrust at ~1.5–2 A per motor**. Total: **100–120 g**.
- **Your 300 mA "hand-braked" reading:** that was friction, not a prop load. Expect ~5× that with a prop at full throttle.

| Configuration | TWR @ 100 g | TWR @ 120 g |
|---|---|---|
| 16350, no camera | 1.72 | 2.07 |
| 13400, no camera | 1.82 | 2.19 |
| 16350 + camera | 1.58 | 1.90 |
| 13400 + camera | 1.67 | 2.00 |

**Verdict:** ≥2:1 is reachable without the camera **only if the motors reach the upper end of the estimate**. The camera makes it marginal. The measured thrust decides. If short, the options are:
- 65 mm props (more thrust, more current, worth testing),
- a lighter frame,
- the 13400 cell,
- dropping the baro (the ToF covers 0–1.3 m).

## Current and flight time (estimate)

- **Hover:** each motor carries AUW/4 ≈ 14.5 g, about 53 % of max thrust. Power scales ~T^1.5, giving ≈ 0.8–1.0 A per motor. **Total hover ≈ 3.5–4.5 A** including ~0.2 A for the logic.
- **Full throttle:** 4 × 1.8 A + logic ≈ **6–8.5 A peak**.

| Cell | Usable (80 %) | Hover current | Flight time | Hover C | Peak C |
|---|---|---|---|---|---|
| IP16350 650 mAh, 13.8 g | 520 mAh | 3.6–4.5 A | **~7–8.5 min** | 5.5–7C | ~12C |
| 13400 550 mAh, 10.6 g | 440 mAh | 3.4–4.3 A | **~6–8 min** | 6–8C | ~14C |

**Minimum cell rating:** ≥ 6 A continuous and ≥ 10 A for a few seconds. These are vape cells with no published rating. Disposable-vape coils draw ~3–4 A, so the cells may be marginal at our peak. The **internal-resistance test** in phase 0 (step 7) decides:

| Measured IR | Sag at 8 A | Verdict |
|---|---|---|
| ≤ 60 mΩ | ≤ 0.5 V | OK |
| 60–100 mΩ | 0.5–0.8 V | Marginal: limit max throttle, expect short flights |
| > 100 mΩ | > 0.8 V | Not usable for flight. Buy a 1S LiPo 300–450 mAh, ≥ 30C (e.g. 1S "whoop" packs, 8–12 g) |

**Provisional cell choice: IP16350.** It's bigger, so expect lower IR and less sag, and it gives ~15 % more flight time for +3.2 g. Final pick after the IR and thrust tests.
