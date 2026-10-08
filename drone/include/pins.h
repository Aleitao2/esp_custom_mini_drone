// Drone pin map — M5Stack Stamp-S3 (ESP32-S3FN8, 8 MB flash, no PSRAM).
// Single source of truth; docs/pin-table.md mirrors this file.
//
// Rules applied:
//  - Motors only on pins with no pull-up and no boot-time activity (G39-G42),
//    each with a 10k gate pull-down on the driver board -> no twitch on reset.
//  - Battery sense on ADC1 (G1). ADC2 (G11-G20) is unusable while Wi-Fi/ESP-NOW runs.
//  - Strapping pins G0 (onboard button, 10k pull-up), G3 (10k pull-up onboard), G46 not used for outputs.
//  - G43/G44 (UART0) left free: G43 toggles with the ROM boot log.
//  - Two SPI hosts: PMW3901 (mode 3) on FSPI and SX1278 (mode 0) on HSPI, so the
//    flow sensor task and the radio IRQ never contend for one bus.
#pragma once

// --- Analog sensing (ADC1) ---
#define PIN_VBAT        1   // battery divider 100k/100k + 100 nF  -> ratio 2.0
#define PIN_V5SENSE     2   // optional phase-0 diag: 5V rail divider 100k/100k

// --- Motors (low-side MOSFET gates), Betaflight quad-X order ---
#define PIN_M1          39  // rear-right,  CW
#define PIN_M2          40  // front-right, CCW
#define PIN_M3          41  // rear-left,   CCW
#define PIN_M4          42  // front-left,  CW

// --- I2C bus 0: MPU6050 0x68, baro 0x76/0x77, mag 0x0D/0x1E, VL53L1X 0x29 (400 kHz) ---
#define PIN_I2C_SDA     13
#define PIN_I2C_SCL     15
#define PIN_IMU_INT     14  // MPU6050 INT (madflight IMU loop is interrupt driven)

// --- SPI host A (FSPI): PMW3901 optical flow ---
#define PIN_FLOW_SCK    4
#define PIN_FLOW_MOSI   5
#define PIN_FLOW_MISO   6
#define PIN_FLOW_CS     7

// --- SPI host B (HSPI): SX1278 LoRa ---
#define PIN_LORA_SCK    8
#define PIN_LORA_MOSI   9
#define PIN_LORA_MISO   10
#define PIN_LORA_DIO0   11  // TxDone/RxDone IRQ (digital use is fine on ADC2 pins)
#define PIN_LORA_RST    12

// --- Onboard ---
#define PIN_BUTTON      0   // BOOT button (active low), usable after boot
#define PIN_RGB_LED     21  // WS2812B-2020

// Spare: G3 (strap, avoid), G43/G44 (UART0), G46 (strap, input only)
