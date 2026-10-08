# Safety — batteries, bench, pre-flight

## ⚠️ Unprotected lithium cells (both vape cells)
These cells have **no protection circuit**: nothing stops a short circuit, over-discharge or over-charge except you and the firmware.

1. **Re-wrap before use.** Both photos show torn wrappers with exposed metal near the terminals. On a cylindrical cell the whole can is the negative terminal, so a nick next to the + cap is a short circuit waiting to happen. Use PVC heat-shrink plus an insulator ring at the + end, or at least two layers of Kapton tape.
2. **Check resting voltage first.**

   | Resting voltage | Action |
   |---|---|
   | ≥ 3.0 V | OK |
   | 2.5–3.0 V | Charge under supervision, then confirm it holds voltage after 24 h |
   | < 2.5 V for an unknown time | Recycle it, don't charge it |

3. **Solder only to the nickel tabs** (already spot-welded on these cells), never to the cell body. Use a hot iron and be quick.
4. **Charge at ≤ 0.7C:** fit R_PROG = 3.3 kΩ (364 mA) **before** the first charge. The module as shipped charges at 1 A.
5. **Charge supervised**, on a ceramic tile / non-flammable surface, away from anything flammable. Stop if a cell gets more than warm, smells or swells.
6. **Never discharge below 3.0 V resting** (3.3 V under load = land). Store at ~3.7–3.85 V with the **battery unplugged**: the module and divider slowly drain a connected cell.
7. **Dispose of** swollen, dented or hot cells: tape the terminals and take them to a battery collection point.
8. **The 2200 mAh pouch** must not be punctured or bent. Keep it in a rigid holder in the ground station.

## ⚠️ Bench work
- **PROPS OFF** for every motor test through phase 4. The bench firmware asks you to type `arm PROPS-OFF` to confirm.
- **The only props-on tests before phase 5:**
  - the **single-motor thrust test** (phase 0, step 6): motor clamped/glued to a weighted stand, powered from the bench PSU with a current limit, eyes protected, fingers clear;
  - the optional **all-motors clamped load test** (phase 0, step 9).
- **Set the boost module pot to 5.00 V with a multimeter BEFORE connecting the Stamp-S3.** These modules can be adjustable up to ~9–12 V, and anything above 5.5 V destroys the Stamp-S3.
- **No motor twitch at reset/power-up:** test props off before ever fitting props.
- Keep the battery unplugged while soldering or rewiring.

## Pre-flight checklist (filled in during phases 4–5)
- [ ] Cell resting voltage ≥ 4.0 V, wrapper intact, connector locked
- [ ] Props: correct CW/CCW positions, seated, undamaged
- [ ] Motor order/direction verified (phase 4 test)
- [ ] Failsafe tested this session: link off → level + descend within 250 ms, disarm at 1 s
- [ ] Kill button tested
- [ ] Arming only works with throttle low
- [ ] VBAT reading matches multimeter (±0.05 V)
- [ ] Area clear, people behind the pilot, no flying over people
