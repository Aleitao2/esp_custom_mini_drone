// Phase 0 bench firmware — power, brownout and motor-driver bring-up.
//
// !!! PROPS OFF for every motor command in this firmware !!!
//
// Serial CLI over native USB (115200). Type `help`.
// Any byte received while motors run stops them immediately.
// Reset reasons and the last load-test result are kept in NVS, so a test can run
// untethered (logic powered by the boost module, USB unplugged) and be read back later.

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_system.h>
#include <sdkconfig.h>
#include "pins.h"

// ---------------------------------------------------------------- config
static const uint8_t MOTOR_PINS[4] = {PIN_M1, PIN_M2, PIN_M3, PIN_M4};
static const char *MOTOR_NAMES[4] = {"M1 rear-right CW", "M2 front-right CCW", "M3 rear-left CCW", "M4 front-left CW"};

static const float    VBAT_DIVIDER     = 2.0f;   // 100k / 100k
static const float    V5_DIVIDER       = 2.0f;   // 100k / 100k (optional diag)
static const uint32_t PWM_DEFAULT_HZ   = 20000;
static const uint32_t RAMP_MS          = 300;    // soft start
static const uint32_t MAX_RUN_MS       = 15000;  // hard cap per command
static const uint32_t ARM_TIMEOUT_MS   = 60000;
static const uint32_t BTN_HOLD_MS      = 2000;
static const uint32_t BTN_COUNTDOWN_MS = 3000;

// ---------------------------------------------------------------- state
Preferences prefs;

struct TestLog {
  uint32_t seq;
  uint8_t  kind;        // 1 = single motor, 2 = all motors, 3 = load (100 %), 4 = button load
  uint8_t  pct;
  uint8_t  motor_mask;
  uint8_t  completed;   // 0 = interrupted by reset, 1 = finished, 2 = aborted by user
  uint16_t dur_ms;
  uint16_t vbat_rest_mv;
  uint16_t vbat_min_mv;
  uint16_t vbat_avg_mv;
  uint16_t v5_min_mv;
  uint8_t  tx_load;
  uint8_t  reset_reason_after; // filled in on the next boot if completed == 0
};

static TestLog lastLog = {};
static float   vcal = 1.0f;
static uint32_t pwmHz = PWM_DEFAULT_HZ;
static uint8_t  pwmBits = 0;
static bool     pwmOk = false;
static bool     btnTestEnabled = false;

static bool     armed = false;
static uint32_t armedAt = 0;

static bool     running = false;
static uint8_t  runMask = 0;
static uint8_t  runPct = 0;
static uint32_t runStart = 0, runDur = 0;
static uint32_t sampleCount = 0;
static uint64_t vbatSum = 0;
static uint16_t vbatMin = 0xFFFF, v5Min = 0xFFFF;
static uint16_t filt[4]; static uint8_t filtIdx = 0;

static bool     txOn = false;
static uint32_t txLast = 0;
static const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static String   line;

// ---------------------------------------------------------------- helpers
static const char *resetReasonStr(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_POWERON:   return "POWERON";
    case ESP_RST_EXT:       return "EXT (EN pin)";
    case ESP_RST_SW:        return "SW (esp_restart)";
    case ESP_RST_PANIC:     return "PANIC (crash)";
    case ESP_RST_INT_WDT:   return "INT_WDT";
    case ESP_RST_TASK_WDT:  return "TASK_WDT";
    case ESP_RST_WDT:       return "WDT";
    case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
    case ESP_RST_BROWNOUT:  return "BROWNOUT  <-- supply dipped!";
    case ESP_RST_SDIO:      return "SDIO";
    case ESP_RST_USB:       return "USB";
    case ESP_RST_JTAG:      return "JTAG";
    default:                return "UNKNOWN";
  }
}

static void led(uint8_t r, uint8_t g, uint8_t b) { rgbLedWrite(PIN_RGB_LED, r, g, b); }

static uint16_t readMv(uint8_t pin, float divider, int samples) {
  uint32_t acc = 0;
  for (int i = 0; i < samples; i++) acc += analogReadMilliVolts(pin);
  return (uint16_t)((float)acc / samples * divider);
}
static uint16_t vbatMv(int samples = 32) { return (uint16_t)(readMv(PIN_VBAT, VBAT_DIVIDER, samples) * vcal); }
static uint16_t v5Mv(int samples = 8)    { return readMv(PIN_V5SENSE, V5_DIVIDER, samples); }

static uint32_t ledcClockHz() {
  // Arduino-ESP32 3.x clocks LEDC from the 40 MHz XTAL on the ESP32-S3 (not the 80 MHz APB)
  return ledcGetClockSource() == LEDC_USE_XTAL_CLK ? 40000000UL : 80000000UL;
}

static uint8_t pwmBitsFor(uint32_t hz) {
  // duty steps = clock / f must fit in 2^bits, max 14 bits
  uint32_t clk = ledcClockHz();
  uint8_t bits = 1;
  while (bits < 14 && (clk >> (bits + 1)) >= hz) bits++;
  return bits;
}

static void motorsAllOff() {
  for (int i = 0; i < 4; i++) {
    if (pwmOk) ledcWrite(MOTOR_PINS[i], 0);
    else digitalWrite(MOTOR_PINS[i], LOW);
  }
}

static bool pwmSetup(uint32_t hz) {
  // Step down from the computed resolution in case the clock assumption is wrong.
  for (int bits = pwmBitsFor(hz); bits >= 4; bits--) {
    bool ok = true;
    for (int i = 0; i < 4 && ok; i++) {
      ledcDetach(MOTOR_PINS[i]);
      ok = ledcAttach(MOTOR_PINS[i], hz, bits);
    }
    if (ok) {
      for (int i = 0; i < 4; i++) ledcWrite(MOTOR_PINS[i], 0);
      pwmHz = hz; pwmBits = bits; pwmOk = true;
      return true;
    }
  }
  for (int i = 0; i < 4; i++) { ledcDetach(MOTOR_PINS[i]); pinMode(MOTOR_PINS[i], OUTPUT); digitalWrite(MOTOR_PINS[i], LOW); }
  pwmOk = false;
  return false;
}

static void setDuty(uint8_t mask, float frac) {
  uint32_t maxDuty = (1UL << pwmBits) - 1;
  uint32_t d = (uint32_t)(constrain(frac, 0.0f, 1.0f) * maxDuty + 0.5f);
  for (int i = 0; i < 4; i++) ledcWrite(MOTOR_PINS[i], (mask & (1 << i)) ? d : 0);
}

static void saveLog() { prefs.putBytes("log", &lastLog, sizeof(lastLog)); }

static void printLog(const TestLog &l) {
  if (l.seq == 0) { Serial.println("No test logged yet."); return; }
  static const char *kinds[] = {"?", "single motor", "all motors", "load 100%", "button load"};
  static const char *res[] = {"INTERRUPTED BY RESET", "completed", "aborted by user"};
  Serial.printf("Test #%lu: %s, %u%%, motors 0x%X, %u ms, tx_load=%u -> %s\n",
                (unsigned long)l.seq, kinds[l.kind < 5 ? l.kind : 0], l.pct, l.motor_mask, l.dur_ms, l.tx_load,
                res[l.completed < 3 ? l.completed : 0]);
  if (l.completed == 0) Serial.printf("  reset reason after test: %s\n", resetReasonStr((esp_reset_reason_t)l.reset_reason_after));
  Serial.printf("  VBAT rest %u mV, min %u mV, avg %u mV, sag %d mV\n",
                l.vbat_rest_mv, l.vbat_min_mv, l.vbat_avg_mv, (int)l.vbat_rest_mv - (int)l.vbat_min_mv);
  if (l.v5_min_mv > 1000 && l.v5_min_mv != 0xFFFF) Serial.printf("  5V rail min %u mV\n", l.v5_min_mv);
}

// ---------------------------------------------------------------- ESP-NOW TX load (simulates radio peak current)
static bool txSetup() {
  WiFi.mode(WIFI_STA);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  if (esp_now_init() != ESP_OK) return false;
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, BCAST, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_err_t e = esp_now_add_peer(&peer);
  return e == ESP_OK || e == ESP_ERR_ESPNOW_EXIST;
}

static void txTick() {
  if (!txOn || millis() - txLast < 10) return; // 100 packets/s
  txLast = millis();
  static uint8_t payload[200];
  esp_now_send(BCAST, payload, sizeof(payload));
}

// ---------------------------------------------------------------- motor run engine
static void startRun(uint8_t kind, uint8_t mask, uint8_t pct, uint32_t ms) {
  lastLog = {};
  lastLog.seq = prefs.getUInt("seq", 0) + 1;
  prefs.putUInt("seq", lastLog.seq);
  lastLog.kind = kind;
  lastLog.pct = pct;
  lastLog.motor_mask = mask;
  lastLog.dur_ms = (uint16_t)min<uint32_t>(ms, 65535);
  lastLog.tx_load = txOn;
  lastLog.vbat_rest_mv = vbatMv(64);
  lastLog.completed = 0; // stays 0 if a reset happens mid-test
  saveLog();             // NVS write happens before the motors start

  sampleCount = 0; vbatSum = 0; vbatMin = 0xFFFF; v5Min = 0xFFFF;
  for (auto &f : filt) f = lastLog.vbat_rest_mv;
  runMask = mask; runPct = pct; runDur = ms; runStart = millis(); running = true;
  led(40, 0, 0);
  while (Serial.available()) Serial.read();
  Serial.printf(">>> RUN motors 0x%X at %u%% for %lu ms (send any key to stop)\n", mask, pct, (unsigned long)ms);
}

static void endRun(uint8_t completed) {
  motorsAllOff();
  running = false;
  lastLog.completed = completed;
  lastLog.vbat_min_mv = vbatMin;
  lastLog.vbat_avg_mv = sampleCount ? (uint16_t)(vbatSum / sampleCount) : 0;
  lastLog.v5_min_mv = v5Min;
  saveLog();
  led(0, completed == 1 ? 30 : 0, completed == 1 ? 0 : 30);
  Serial.println(completed == 1 ? "<<< done" : "<<< STOPPED");
  printLog(lastLog);
}

static void runTick() {
  if (!running) return;
  if (Serial.available()) { while (Serial.available()) Serial.read(); endRun(2); return; }
  uint32_t t = millis() - runStart;
  if (t >= runDur) { endRun(1); return; }
  float frac = runPct / 100.0f;
  if (t < RAMP_MS) frac *= (float)t / RAMP_MS;
  setDuty(runMask, frac);

  // sample VBAT at 2 kHz (4-sample moving average for min, so a single noisy conversion does not count as sag)
  static uint32_t lastSampleUs = 0;
  if (micros() - lastSampleUs < 500) return;
  lastSampleUs = micros();
  uint16_t v = (uint16_t)(analogReadMilliVolts(PIN_VBAT) * VBAT_DIVIDER * vcal);
  filt[filtIdx++ & 3] = v;
  uint16_t avg4 = (filt[0] + filt[1] + filt[2] + filt[3]) / 4;
  if (t > 50 && avg4 < vbatMin) vbatMin = avg4;
  vbatSum += v; sampleCount++;
  if ((sampleCount & 7) == 0) {
    uint16_t v5 = (uint16_t)(analogReadMilliVolts(PIN_V5SENSE) * V5_DIVIDER);
    if (v5 > 1000 && v5 < v5Min) v5Min = v5;
  }
}

// ---------------------------------------------------------------- CLI
static void printHelp() {
  Serial.println(F(
    "\n=== Phase 0 bench CLI ===   !!! PROPS OFF !!!\n"
    "status                 battery, 5V rail, reset reason/counters, PWM\n"
    "watch [s]              stream VBAT / 5V at 10 Hz (default 10 s)\n"
    "vcal <volts>           calibrate VBAT with your multimeter reading\n"
    "log | clear            last test result | reset counters and log\n"
    "freq <hz>              motor PWM frequency (4000-40000, default 20000)\n"
    "arm PROPS-OFF          enable motor commands for 60 s\n"
    "disarm                 disable motor commands\n"
    "m <1-4> <pct> [s]      run one motor (default 2 s)\n"
    "all <pct> [s]          run all four motors (default 2 s)\n"
    "load [s] [tx]          all four at 100 % (default 5 s); 'tx' adds ESP-NOW TX load\n"
    "tx on|off              ESP-NOW broadcast at 19.5 dBm, 100 pkt/s (radio current)\n"
    "btntest on|off         allow untethered load test via BOOT button (hold 2 s)\n"
    "reboot                 restart MCU\n"
    "Any key while motors run = STOP."));
}

static void printStatus() {
  uint16_t vb = vbatMv(64), v5 = v5Mv(16);
  Serial.printf("VBAT     %u mV (cal x%.4f)\n", vb, vcal);
  if (v5 > 1000) Serial.printf("5V rail  %u mV\n", v5); else Serial.println("5V rail  not wired (optional, G2)");
  Serial.printf("Reset    %s\n", resetReasonStr(esp_reset_reason()));
  Serial.printf("Counters boots=%lu brownouts=%lu panics=%lu wdt=%lu\n",
                (unsigned long)prefs.getUInt("boots", 0), (unsigned long)prefs.getUInt("brownouts", 0),
                (unsigned long)prefs.getUInt("panics", 0), (unsigned long)prefs.getUInt("wdt", 0));
#ifdef CONFIG_ESP_BROWNOUT_DET_LVL
  Serial.printf("Brownout detector enabled, level %d\n", CONFIG_ESP_BROWNOUT_DET_LVL);
#else
  Serial.println("Brownout detector: level macro not found in sdkconfig");
#endif
  if (pwmOk)
    Serial.printf("PWM      %lu Hz, %u bits (%lu steps), LEDC clock %lu MHz\n", (unsigned long)pwmHz, pwmBits,
                  (unsigned long)(1UL << pwmBits), (unsigned long)(ledcClockHz() / 1000000UL));
  else
    Serial.println("PWM      SETUP FAILED - motor commands disabled");
  Serial.printf("Armed    %s   btntest %s   tx %s\n", armed ? "YES" : "no", btnTestEnabled ? "ON" : "off", txOn ? "ON" : "off");
  Serial.printf("Uptime   %lu s, CPU %lu MHz, heap %lu\n", (unsigned long)(millis() / 1000),
                (unsigned long)getCpuFrequencyMhz(), (unsigned long)ESP.getFreeHeap());
}

static bool requireArmed() {
  if (!pwmOk) { Serial.println("Refused: motor PWM setup FAILED (see 'status')"); return false; }
  if (armed) { armedAt = millis(); return true; }
  Serial.println("Refused: not armed. Remove props, then type: arm PROPS-OFF");
  return false;
}

static void handleLine(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;
  char buf[96];
  cmd.toCharArray(buf, sizeof(buf));
  char *tok = strtok(buf, " ");
  char *a1 = strtok(nullptr, " ");
  char *a2 = strtok(nullptr, " ");
  char *a3 = strtok(nullptr, " ");
  String c = String(tok); c.toLowerCase();

  if (c == "help" || c == "?") printHelp();
  else if (c == "status") printStatus();
  else if (c == "log") printLog(lastLog);
  else if (c == "clear") {
    prefs.putUInt("boots", 0); prefs.putUInt("brownouts", 0); prefs.putUInt("panics", 0); prefs.putUInt("wdt", 0);
    lastLog = {}; saveLog(); Serial.println("Counters and log cleared.");
  }
  else if (c == "watch") {
    uint32_t s = a1 ? atoi(a1) : 10, t0 = millis();
    while (millis() - t0 < s * 1000 && !Serial.available()) {
      uint16_t v5 = v5Mv(8);
      Serial.printf("VBAT %u mV   5V %s%u mV\n", vbatMv(32), v5 > 1000 ? "" : "n/c ", v5);
      delay(100);
    }
    while (Serial.available()) Serial.read();
  }
  else if (c == "vcal") {
    if (!a1) { Serial.println("usage: vcal <volts measured at the cell>"); return; }
    float meas = atof(a1);
    float raw = readMv(PIN_VBAT, VBAT_DIVIDER, 128) / 1000.0f;
    if (meas < 2.5f || meas > 4.5f || raw < 1.0f) { Serial.println("Refused: value out of range or divider not connected."); return; }
    vcal = meas / raw;
    prefs.putFloat("vcal", vcal);
    Serial.printf("vcal = %.4f (raw %.3f V -> %.3f V)\n", vcal, raw, meas);
  }
  else if (c == "freq") {
    uint32_t hz = a1 ? atoi(a1) : 0;
    if (running || hz < 4000 || hz > 40000) { Serial.println("usage: freq <4000-40000> (motors must be stopped)"); return; }
    if (pwmSetup(hz)) Serial.printf("OK: PWM %lu Hz, %u bits\n", (unsigned long)pwmHz, pwmBits);
    else Serial.println("LEDC setup FAILED - motor commands disabled");
  }
  else if (c == "arm") {
    if (!a1 || String(a1) != "PROPS-OFF") { Serial.println("Type exactly: arm PROPS-OFF  (confirms props are removed)"); return; }
    armed = true; armedAt = millis(); led(30, 10, 0);
    Serial.println("ARMED for 60 s of inactivity. Props must be OFF.");
  }
  else if (c == "disarm") { armed = false; led(0, 0, 20); Serial.println("Disarmed."); }
  else if (c == "m") {
    if (!requireArmed()) return;
    int n = a1 ? atoi(a1) : 0, pct = a2 ? atoi(a2) : -1;
    float s = a3 ? atof(a3) : 2.0f;
    if (n < 1 || n > 4 || pct < 0 || pct > 100) { Serial.println("usage: m <1-4> <0-100> [seconds]"); return; }
    Serial.printf("%s\n", MOTOR_NAMES[n - 1]);
    startRun(1, 1 << (n - 1), pct, min<uint32_t>(s * 1000, MAX_RUN_MS));
  }
  else if (c == "all") {
    if (!requireArmed()) return;
    int pct = a1 ? atoi(a1) : -1;
    float s = a2 ? atof(a2) : 2.0f;
    if (pct < 0 || pct > 100) { Serial.println("usage: all <0-100> [seconds]"); return; }
    startRun(2, 0x0F, pct, min<uint32_t>(s * 1000, MAX_RUN_MS));
  }
  else if (c == "load") {
    if (!requireArmed()) return;
    float s = 5.0f; bool tx = false;
    for (char *a : {a1, a2}) {
      if (!a) continue;
      if (String(a) == "tx") tx = true; else s = atof(a);
    }
    if (tx && !txOn) { txOn = txSetup(); Serial.println(txOn ? "ESP-NOW TX load ON" : "ESP-NOW init FAILED"); }
    startRun(3, 0x0F, 100, min<uint32_t>(s * 1000, MAX_RUN_MS));
  }
  else if (c == "tx") {
    if (a1 && String(a1) == "on") { txOn = txSetup(); Serial.println(txOn ? "TX load ON" : "ESP-NOW init FAILED"); }
    else { txOn = false; esp_now_deinit(); WiFi.mode(WIFI_OFF); Serial.println("TX load off"); }
  }
  else if (c == "btntest") {
    btnTestEnabled = a1 && String(a1) == "on";
    prefs.putBool("btntest", btnTestEnabled);
    Serial.printf("Button load test %s (persists across reboots)\n", btnTestEnabled ? "ENABLED - props must stay off" : "disabled");
  }
  else if (c == "reboot") { Serial.println("rebooting"); delay(100); esp_restart(); }
  else Serial.printf("Unknown command '%s' - type help\n", tok);
}

// ---------------------------------------------------------------- button-triggered untethered load test
static void buttonTick() {
  static uint32_t downSince = 0;
  if (!btnTestEnabled || running) { downSince = 0; return; }
  bool down = digitalRead(PIN_BUTTON) == LOW;
  if (!down) { downSince = 0; return; }
  if (downSince == 0) { downSince = millis(); return; }
  if (millis() - downSince < BTN_HOLD_MS) return;
  downSince = 0;
  // countdown: fast red blink, release the button and step back
  uint32_t t0 = millis();
  while (millis() - t0 < BTN_COUNTDOWN_MS) {
    led(((millis() / 125) & 1) ? 60 : 0, 0, 0);
    delay(10);
  }
  startRun(4, 0x0F, 100, 5000);
}

// ---------------------------------------------------------------- setup / loop
void setup() {
  // Motors first: drive gates low before anything else (board also has 10k pull-downs).
  for (int i = 0; i < 4; i++) { pinMode(MOTOR_PINS[i], OUTPUT); digitalWrite(MOTOR_PINS[i], LOW); }
  pinMode(PIN_BUTTON, INPUT_PULLUP);
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_VBAT, ADC_11db);
  analogSetPinAttenuation(PIN_V5SENSE, ADC_11db);

  prefs.begin("bench", false);
  esp_reset_reason_t rr = esp_reset_reason();
  prefs.putUInt("boots", prefs.getUInt("boots", 0) + 1);
  if (rr == ESP_RST_BROWNOUT) prefs.putUInt("brownouts", prefs.getUInt("brownouts", 0) + 1);
  if (rr == ESP_RST_PANIC) prefs.putUInt("panics", prefs.getUInt("panics", 0) + 1);
  if (rr == ESP_RST_INT_WDT || rr == ESP_RST_TASK_WDT || rr == ESP_RST_WDT) prefs.putUInt("wdt", prefs.getUInt("wdt", 0) + 1);
  vcal = prefs.getFloat("vcal", 1.0f);
  btnTestEnabled = prefs.getBool("btntest", false);
  if (prefs.getBytesLength("log") == sizeof(lastLog)) {
    prefs.getBytes("log", &lastLog, sizeof(lastLog));
    if (lastLog.seq && lastLog.completed == 0 && lastLog.reset_reason_after == 0) {
      lastLog.reset_reason_after = (uint8_t)rr; // a test was running when the MCU reset
      saveLog();
    }
  }

  bool pwmBoot = pwmSetup(PWM_DEFAULT_HZ);
  led(pwmBoot ? 0 : 40, 0, pwmBoot ? 20 : 0); // blue = ready, disarmed; red = PWM failed

  Serial.begin(115200);
  uint32_t t0 = millis();
  while (!Serial && millis() - t0 < 1500) delay(10);
  Serial.printf("\n\nDroneClaude phase 0 bench firmware - reset reason: %s\n", resetReasonStr(rr));
  if (pwmBoot) Serial.printf("Motor PWM ready: %lu Hz, %u bits\n", (unsigned long)pwmHz, pwmBits);
  else Serial.println("!!! Motor PWM setup FAILED - motor commands disabled");
  if (lastLog.seq && lastLog.completed == 0) Serial.println("!!! Previous test was INTERRUPTED by a reset - see 'log'");
  printHelp();
}

void loop() {
  runTick();
  txTick();
  buttonTick();
  if (armed && !running && millis() - armedAt > ARM_TIMEOUT_MS) { armed = false; led(0, 0, 20); Serial.println("Auto-disarmed (timeout)."); }
  if (!running) {
    while (Serial.available()) {
      char ch = Serial.read();
      if (ch == '\n' || ch == '\r') { handleLine(line); line = ""; }
      else if (line.length() < 90) line += ch;
    }
    delay(1);
  }
}
