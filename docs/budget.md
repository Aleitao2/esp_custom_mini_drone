# Weight, thrust and flight-time budget

Status: **weights mostly measured (2026-10-08)**; frame and wiring still estimated. Thrust is still estimated until the single-motor thrust test ([phase0.md](phase0.md) step 6).

## Weight

| Item | g | Source |
|---|---|---|
| Frame (PETG/PLA, target) | 9.0 | estimate (not printed yet) |
| Stamp-S3 | 3.0 | measured |
| MPU6050 (no pins, no cable) | 1.1 | measured |
| Baro HW-291 (with cable + pin header) | 2.1 | measured |
| VL53L1X TOF400C (with cable + pin header) | 2.2 | measured |
| PMW3901 (with pin header) | 2.8 | measured |
| SX1278 + antenna + cables | 3.6 | measured |
| Motor driver: 4 × 09N03LA (DPAK) | 1.5 | measured |
| Motor driver: perfboard, diodes, resistors, bulk cap | 1.5 | estimate |
| Charger/boost module | 2.6 | measured |
| Wiring, battery connector, switch | 4.0 | estimate |
| 4 × 720 motors with leads | 15.2 | measured |
| 4 × 55 mm props | 1.34 | measured |
| **Subtotal without battery** | **49.9** | |
| + IP16350 650 mAh | 13.8 | measured |
| + 13400 550 mAh | 10.6 | measured |
| GY-271 magnetometer (phase 8 only) | +1.2 | estimate |
| Future AIO FPV camera reserve | +5.0 | |

| Configuration | AUW (g) |
|---|---|
| 16350, no camera | **63.7** |
| 13400, no camera | **60.5** |
| 16350 + camera | 68.7 |
| 13400 + camera | 65.5 |

That's 5.6 g more than the first estimate. Where it came from: headers and cables on the breakouts, the DPAK MOSFETs, and heavier motors (3.8 g each, not 3.4 g).

### Weight that can come off later (≈ 4–5 g)
| Change | Saves |
|---|---|
| Desolder pin headers from baro / ToF / PMW3901; wire them with thin 30 AWG | ~1.5 g |
| AO3400A (SOT-23) instead of 09N03LA (DPAK) | ~1.4 g |
| SX1278: thin 30 AWG leads and a plain 17.3 cm wire antenna | ~0.5–1 g (depends on what's fitted now) |
| Frame ≤ 8 g | ~1 g |

## Thrust — measured, test 1 (2026-10-08, **suspect setup**)
Motor glued on the scale pan, prop blowing up. Bench PSU, one motor, 55 mm prop:

| V | A | W | thrust g | g/W |
|---|---|---|---|---|
| 3.4 | 1.5 | 5.1 | 9.5–10 | 1.9 |
| 3.7 | 1.6 | 5.9 | 10.8–11 | 1.8 |
| 4.0 | 1.8 | 7.2 | 12–12.5 | 1.7 |

The current matches the estimate, but **the thrust is about half** (1.8 g/W; ~3–4 g/W expected).
- **If real:** 4 × 11 g = 44 g < 63.7 g AUW, so it **cannot hover**.
- **Why it's suspect:**
  1. The prop was ~2–3 cm above the pan. It starves its own inflow, and the suction lifts the pan, so the reading comes out low.
  2. Making air go up with an upright motor means a reversed wire or swapped/flipped prop. A prop spinning backwards gives roughly half thrust.
- **Re-test with the gallows rig** in [phase0.md](phase0.md) step 3 before changing any design.

## Thrust (estimate, before measuring)

- **Motor:** 720 coreless, ~48 krpm no-load at 3.7 V (listing), so Kv ≈ 13 000 rpm/V. Your no-load 120 mA fits that.
- **Prop:** 55 mm. Prop-coefficient estimate (C_T ≈ 0.09, C_P ≈ 0.05) at a loaded 28–32 krpm and ~3.6 V gives **25–30 g thrust at ~1.5–2 A per motor**. Total: **100–120 g**.
- **Your 300 mA "hand-braked" reading:** that was friction, not a prop load. Expect ~5× that with a prop at full throttle.

| Configuration | TWR @ 100 g | TWR @ 120 g |
|---|---|---|
| 16350, no camera | 1.57 | 1.88 |
| 13400, no camera | 1.65 | 1.98 |
| 16350 + camera | 1.46 | 1.75 |
| 13400 + camera | 1.53 | 1.83 |

**Verdict:** below 2:1 in every configuration unless the measured thrust beats the estimate.
- About 1.6–1.9 still flies (most toy quads are ~1.5–2), but with less punch and less control margin.
- To get back to 2:1:
  - the weight savings above (−4–5 g),
  - 65 mm props, if the motors can swing them,
  - the 13400 cell.
- **The single-motor thrust test decides.** Your bench PSU and 55 mm props are all it needs.

## Current and flight time (estimate, scaled to the measured weight)

- **Hover:** each motor carries AUW/4 ≈ 16 g, about 55–65 % of max thrust. That's ≈ 0.95–1.2 A per motor, so **total hover ≈ 4–5 A** including ~0.2 A for the logic.
- **Full throttle:** 4 × 1.8 A + logic ≈ **6–8.5 A peak**.

| Cell | Usable (80 %) | Hover current | Flight time | Hover C | Peak C |
|---|---|---|---|---|---|
| IP16350 650 mAh, 13.8 g | 520 mAh | 4.1–5.2 A | **~6–7.5 min** | 6–8C | ~12C |
| 13400 550 mAh, 10.6 g | 440 mAh | 3.9–5.0 A | **~5.5–7 min** | 7–9C | ~14C |

**Minimum cell rating:** ≥ 6 A continuous and ≥ 10 A for a few seconds. These are vape cells with no published rating, and disposable-vape coils only draw ~3–4 A. The **internal-resistance test** decides; it's done last, as you asked, after everything works on the bench PSU.

| Measured IR | Sag at 8 A | Verdict |
|---|---|---|
| ≤ 60 mΩ | ≤ 0.5 V | OK |
| 60–100 mΩ | 0.5–0.8 V | Marginal: limit max throttle, expect short flights |
| > 100 mΩ | > 0.8 V | Not usable for flight. Buy a 1S LiPo 300–450 mAh, ≥ 30C (e.g. 1S "whoop" packs, 8–12 g) |

**Provisional cell choice: IP16350.** It's bigger, so expect lower IR and less sag. Final pick after the IR and thrust tests.
