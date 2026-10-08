# Radio link analysis

Transport is behind a link abstraction (phase 3). **ESP-NOW first, then LoRa (SX1278)**, comparing measured rate, latency and loss.

## LoRa time on air (computed, Semtech formula)

Settings: BW 500 kHz, CR 4/5, **implicit header**, CRC on, preamble 8. These will be verified on hardware with RadioLib `getTimeOnAir(len)`.

| SF | 8 B | 10 B | 12 B |
|---|---|---|---|
| SF6 | 4.5 ms | 5.2 ms | 5.2 ms |
| SF7 | 9.0 ms | 9.0 ms | 10.3 ms |
| SF8 | 15.5 ms | 18.1 ms | 18.1 ms |

- **50 Hz control** (20 ms frame) fits at SF6 or SF7, with room for a telemetry slot every Nth frame. SF6 allows ~100 Hz.
- **Latency:** time on air + ~1 ms SPI/IRQ + up to one frame of waiting. At SF6 / 50 Hz the worst case is ≈ 26 ms; at 100 Hz it is ≈ 16 ms. Both are under the 30 ms target.
- **RadioLib 7.8.1:** SX1278 supports SF6 only with `implicitHeader(len)` (both ends must use the same fixed length). It applies the SX127x 500 kHz sensitivity errata fix internally.
- **TX power:** PA_BOOST range +2…+17 dBm (+20 dBm high-power mode). The XL1278-SMT uses PA_BOOST, so the minimum is +2 dBm.

## ESP-NOW
- 2.4 GHz Wi-Fi band. ESP32 modules are Anatel-certified for it, so this is the unambiguous legal option.
- 250-byte frames, ~1 ms airtime at 1 Mbps; 50–100 Hz is trivial.
- **Risk on the ground station:** the classic ESP32 has one 2.4 GHz radio shared between Wi-Fi (ESP-NOW) and Bluetooth Classic (DS4) by time-slicing ("coexistence"). Expect jitter and some loss while the DS4 is connected. Phase 3 measures it. LoRa avoids it because the SX1278 is a separate radio.

## Packet (phase 3, `common/`)
- 12 bytes:
  - bind ID (2)
  - sequence (1)
  - flags: arm, kill, mode (1)
  - 4 × 12-bit sticks (6)
  - CRC-16 (2)
- Packets with a foreign bind ID are ignored.
- **Telemetry** (drone → ground), low rate (≈5 Hz): VBAT, RSSI/SNR, armed state, last reset reason, link loss count.
- **Failsafe** (drone side, independent of madflight's 3 s timeout):

| Condition | Action |
|---|---|
| No valid packet for 250 ms | Level, reduce throttle |
| No valid packet for 1 s | Disarm, motors off |

- **Arming** requires throttle low + the arm button. The kill button disarms at any throttle.

## Regulatory (Brazil)
- **Anatel rules:** 433 MHz short-range devices fall under Anatel's restricted-radiation rules (Resolução 680/2017, technical requirements in **Ato 14448/2017, Annex I**). They are described as closely following the US rules, which only allow low-power, intermittent control transmissions at 433.92 MHz. I could not confirm the exact Brazilian power/duty limit online. **Check Annex I of the current Ato 14448** before running LoRa at power.
- **Amateur band:** 430–440 MHz in Brazil is amateur radio (licensed operators only).
- **Personal use:** these rules apply to *operating* a transmitter, not just selling one, so "personal college project" doesn't exempt it.
- **What we'll do:**
  - ESP-NOW is the default link.
  - LoRa runs at the **minimum power (+2 dBm)** for the bench comparison and short line-of-sight tests.
  - At SF6 / 50 Hz the transmitter is on ~26 % of the time, so it is a continuous-ish emitter. Keep sessions short.
