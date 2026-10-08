# Pin table

Source of truth for the drone: [`drone/include/pins.h`](../drone/include/pins.h).

## Drone — M5Stack Stamp-S3 (ESP32-S3FN8, 8 MB flash, no PSRAM)

| GPIO | Header | Function | Notes |
|---|---|---|---|
| G1 | left 1 | **VBAT sense** (ADC1_CH0) | 100k/100k divider + 100 nF. ADC1 because ADC2 can't be read while Wi-Fi/ESP-NOW runs. |
| G2 | left 2 | 5 V rail sense (ADC1_CH1) | Optional, phase 0 diagnostics only. 100k/100k divider. |
| G3 | left 3 | — | Strapping pin, 10k pull-up on the board. Leave unconnected. |
| G4 | left 4 | FLOW SCK | SPI host A (FSPI) |
| G5 | left 5 | FLOW MOSI | |
| G6 | left 6 | FLOW MISO | |
| G7 | left 7 | FLOW CS (PMW3901) | |
| G8 | left 8 | LORA SCK | SPI host B (HSPI) |
| G9 | left 9 | LORA MOSI | |
| G10 | left 10 | LORA MISO | |
| G11 | left 12 | LORA DIO0 (IRQ) | |
| G12 | left 14 | LORA RST | |
| G13 | left 15 | **I2C SDA** | Board's labelled SDA |
| G14 | left 16 | **IMU INT** (MPU6050) | Required: the madflight IMU loop is interrupt driven |
| G15 | left 17 | **I2C SCL** | Board's labelled SCL |
| G39 | right 10 | **M1** rear-right, CW | No boot activity, 10k gate pull-down |
| G40 | right 8 | **M2** front-right, CCW | |
| G41 | right 6 | **M3** rear-left, CCW | |
| G42 | right 4 | **M4** front-left, CW | |
| G43 | right 3 | spare (UART0 TX) | Toggles with the ROM boot log. Never use it for a motor. |
| G44 | right 5 | spare (UART0 RX) | |
| G46 | right 2 | spare (strapping) | Input only |
| G0 | right 9 | BOOT button | Hold at power-up = download mode |
| G21 | internal | WS2812 RGB LED | Status LED |
| G19/G20 | USB-C | Native USB | Serial console and flashing |
| 5V | left 13 | Logic input from the boost (via SS34 + SW1) | Also connected to USB VBUS through a polyfuse |
| 3V3 | right 1 | 3.3 V out (1 A buck) | Feeds all sensors and the SX1278 |

### Bus decisions
- **I2C:** one bus at **400 kHz** for MPU6050 (0x68), baro (0x76/0x77), mag (0x0D QMC5883L or 0x1E HMC5883L) and VL53L1X (0x29). There are no address conflicts.
  - 400 kHz is the maximum for the MPU6050, QMC5883L and VL53L1X (VL53L1X datasheet: "up to 400 kHz"). madflight v2.4.2 forces 1 MHz, so we patch that in phase 5.
  - Each breakout has its own pull-ups. If the combined value drops below ~1.5 kΩ, remove some (checked in phase 1).
  - The VL53L1X is polled at 50 Hz with a 20 ms timing budget. It does not block the IMU because madflight reads the IMU from its interrupt.
  - A second I2C bus is only needed if phase 1 shows IMU sample jitter.
- **SPI:** two separate hosts instead of one shared bus with two CS lines.
  - The PMW3901 needs SPI mode 3 at ≤2 MHz. The SX1278 uses mode 0 at up to 10 MHz.
  - The flow sensor is read by madflight's sensor task, and the radio is driven by its own IRQ/task. Separate hosts mean no cross-task bus locking and no mode switching.
  - The cost is three more pins, which the Stamp-S3 has.

## Ground station — classic ESP32 (WROOM-32 dev board)
Defined in phase 2/3. Constraints: avoid GPIO 6–11 (flash), 34–39 are input-only, and watch strapping pins 0, 2, 5, 12 and 15.
