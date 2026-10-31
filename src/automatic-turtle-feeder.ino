#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <ESP32Servo.h>
#include <ThreeWire.h>
#include <RtcDS1302.h>
#include <math.h>
#include <WiFi.h>          // IoT addition only
#include <WebServer.h>     // IoT addition only
#include <Preferences.h>   // IoT addition only: persistent feed logs

// =====================================================
// 1. ALL EDITABLE SETTINGS
// =====================================================

// LCD AND I2C
const int LCD_ADDRESS = 0x27;
const int I2C_SDA_PIN = 21;
const int I2C_SCL_PIN = 22;

// SERVO
const int SERVO_PIN = 13;
const int SERVO_OPEN_ANGLE = 35;
const int SERVO_CLOSED_ANGLE = 0;
const unsigned long SERVO_CLOSE_DELAY_MS = 3000;

// BUTTONS
const int BUTTON_PIN = 14;
const int BUTTON_RESET_PIN = 27;
const int BUTTON_AUTO_PIN = 34;
const unsigned long RESET_DEBOUNCE_MS = 50;

// DS1302 RTC (3-wire interface)
const int RTC_IO_PIN = 2;
const int RTC_SCLK_PIN = 5;
const int RTC_CE_PIN = 15;

// BUTTON 3 AUTOMATIC FEEDING
const unsigned long AUTO_FEED_COUNTDOWN_SECONDS = 3;
const unsigned long AUTO_FEED_GAP_SECONDS = 10;
const unsigned long AUTO_BUTTON_PULSE_MS = 100;

// SONAR
const int SONAR_TRIG_PIN = 25;
const int SONAR_ECHO_PIN = 26;

const int FILTER_WINDOW = 5;
const float MAX_JUMP_CM = 10.0;
const unsigned long SONAR_INTERVAL_MS = 120;

// FOOD LEVEL CALIBRATION
const float DISTANCE_MIN = 5.0;
const float DISTANCE_MAX = 12.62;

// MOTOR
const int MOTOR_EN_PIN = 4;
const int MOTOR_IN1_PIN = 18;
const int MOTOR_IN2_PIN = 19;

const int MOTOR_PWM_VALUE = 130;

const unsigned long MOTOR_FORWARD_MS = 120;
const unsigned long MOTOR_PAUSE_MS = 60;
const unsigned long MOTOR_BACKWARD_MS = 120;

const unsigned long MOTOR_RUN_ON_MS = 3000;

// RTC
int hoursUntilFeeding = 12;

// IR BOWL SENSOR
const int IR_SENSOR_PIN = 32;
const int BOWL_DETECTED_STATE = LOW;
const unsigned long BOWL_MISSING_TIMEOUT_MS = 5000UL;

// BUZZER
const int BUZZER_PIN = 33;
const int BUZZER_FREQUENCY = 2300;

const unsigned long BUZZER_CYCLE_MS = 1200;

const unsigned long BUZZER_BEEP_1_START = 0;
const unsigned long BUZZER_BEEP_1_END = 100;

const unsigned long BUZZER_BEEP_2_START = 180;
const unsigned long BUZZER_BEEP_2_END = 280;

const unsigned long BUZZER_BEEP_3_START = 360;
const unsigned long BUZZER_BEEP_3_END = 460;

// INDIVIDUAL LEDS
const int RED_LED_PIN = 16;
const int GREEN_LED_PIN = 17;
const int BLUE_LED_PIN = 23;

const unsigned long RED_BLINK_INTERVAL_MS = 500;
const unsigned long GREEN_BLINK_INTERVAL_MS = 40;

// LCD
const unsigned long LCD_REFRESH_INTERVAL = 200;

// IOT DASHBOARD SETTINGS
const char* IOT_WIFI_SSID = "Galaxy Note 10+";
const char* IOT_WIFI_PASSWORD = "abrar123465";
const unsigned long IOT_WIFI_RETRY_MS = 15000UL;
const int IOT_MAX_LOGS = 30;

// =====================================================
// 2. HARDWARE OBJECTS
// =====================================================

LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
Servo myServo;

ThreeWire rtcWire(RTC_IO_PIN, RTC_SCLK_PIN, RTC_CE_PIN);
RtcDS1302<ThreeWire> rtc(rtcWire);

// =====================================================
// 3. RUNTIME VARIABLES
// =====================================================

// MOTOR
int motorPhase = 0;

unsigned long lastMotorPhaseTime = 0;
unsigned long motorStopDeadline = 0;

bool buttonWasPressed = false;
bool motorRunningOn = false;

// SERVO
unsigned long servoCloseDeadline = 0;
bool servoClosePending = false;
bool servoAttached = false;

// SONAR
float readings[FILTER_WINDOW] = {0.0};

int readIndex = 0;
int validReadings = 0;

float total = 0.0;
float lastValidDistance = -1.0;

bool isFiltering = false;

unsigned long lastSonarReadingTime = 0;

// RESET BUTTON
int lastResetButtonReading = HIGH;
int stableResetButtonState = HIGH;

unsigned long lastResetDebounceTime = 0;

// BUTTON 3
int lastAutoButtonReading = HIGH;
int stableAutoButtonState = HIGH;
unsigned long lastAutoButtonDebounceTime = 0;

// DS1302 / AUTOMATIC FEED SEQUENCE
bool rtcReady = false;

enum AutoFeedState {
  AUTO_IDLE,
  AUTO_WAIT_FIRST_FEED,
  AUTO_WAIT_FIRST_CYCLE,
  AUTO_WAIT_SECOND_FEED,
  AUTO_WAIT_SECOND_CYCLE
};

AutoFeedState autoFeedState = AUTO_IDLE;
uint64_t autoFeedDeadlineSeconds = 0;

bool autoButtonPulseActive = false;
unsigned long autoButtonPulseDeadline = 0;

// LCD
unsigned long lastLcdUpdate = 0;

// IR SENSOR
bool bowlDetected = false;
bool bowlAlarmActive = false;
bool bowlMissingTimerStarted = false;

unsigned long bowlMissingSince = 0;

// BUZZER
int lastBuzzerFrequency = -1;

// LEDS
bool greenBlinkOn = false;
bool bowlRedBlinkOn = false;

unsigned long lastBowlRedBlink = 0;
unsigned long lastGreenBlink = 0;

// IOT-ONLY STATE
WebServer iotServer(80);
Preferences iotPreferences;
bool iotStorageReady = false;
bool iotHttpStarted = false;
unsigned long iotLastWiFiRetry = 0;
bool iotFoodLevelValid = false;
long iotFoodPercentage = 0;
bool iotFeedCycleTracked = false;
char iotCycleStart[20] = "";
uint8_t iotCycleMode = 1;

struct IotFeedLog {
  char startedAt[20];
  char completedAt[20];
  uint8_t mode;
};

struct IotFeedLogStore {
  uint32_t magic;
  uint16_t count;
  uint16_t next;
  IotFeedLog logs[IOT_MAX_LOGS];
};

const uint32_t IOT_LOG_MAGIC = 0x54464C31UL;
IotFeedLogStore iotLogStore = {};

// =====================================================
// 4. MOTOR FUNCTIONS
// =====================================================

void jiggleForward() {

  digitalWrite(MOTOR_IN1_PIN, HIGH);
  digitalWrite(MOTOR_IN2_PIN, LOW);

  analogWrite(MOTOR_EN_PIN, MOTOR_PWM_VALUE);
}

void jiggleBackward() {

  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, HIGH);

  analogWrite(MOTOR_EN_PIN, MOTOR_PWM_VALUE);
}

void stopMotor() {

  analogWrite(MOTOR_EN_PIN, 0);

  digitalWrite(MOTOR_IN1_PIN, LOW);
  digitalWrite(MOTOR_IN2_PIN, LOW);
}

void setMotorPhase(int phase) {

  motorPhase = phase;
  lastMotorPhaseTime = millis();

  switch (motorPhase) {

    case 0:
      jiggleForward();
      break;

    case 1:
      stopMotor();
      break;

    case 2:
      jiggleBackward();
      break;

    case 3:
      stopMotor();
      break;
  }
}

void updateMotorJitter() {

  unsigned long now = millis();
  unsigned long phaseDuration;

  switch (motorPhase) {

    case 0:
      phaseDuration = MOTOR_FORWARD_MS;
      break;

    case 1:
      phaseDuration = MOTOR_PAUSE_MS;
      break;

    case 2:
      phaseDuration = MOTOR_BACKWARD_MS;
      break;

    default:
      phaseDuration = MOTOR_PAUSE_MS;
      break;
  }

  if (now - lastMotorPhaseTime >= phaseDuration) {

    setMotorPhase((motorPhase + 1) % 4);
  }
}

// =====================================================
// 5. BUZZER FUNCTION
// =====================================================

void setBuzzerFrequency(int frequency) {

  if (frequency == lastBuzzerFrequency) {
    return;
  }

  if (frequency == 0) {

    noTone(BUZZER_PIN);

  } else {

    tone(BUZZER_PIN, frequency);
  }

  lastBuzzerFrequency = frequency;
}

// =====================================================
// 6. LCD ROW HELPER
// =====================================================

void printLcdRow(int row, const char *message) {

  lcd.setCursor(0, row);

  int count = 0;

  while (message[count] != '\0' && count < 16) {

    lcd.print(message[count]);
    count++;
  }

  while (count < 16) {

    lcd.print(' ');
    count++;
  }
}

// =====================================================
// 7. IR BOWL DETECTION
// =====================================================

void updateBowlSensor() {

  bool detected =
      (digitalRead(IR_SENSOR_PIN) == BOWL_DETECTED_STATE);

  unsigned long now = millis();

  if (detected) {

    bowlDetected = true;

    bowlMissingTimerStarted = false;
    bowlAlarmActive = false;

  } else {

    bowlDetected = false;

    if (!bowlMissingTimerStarted) {

      bowlMissingTimerStarted = true;
      bowlMissingSince = now;
    }

    if (now - bowlMissingSince >= BOWL_MISSING_TIMEOUT_MS) {

      bowlAlarmActive = true;
    }
  }
}

// =====================================================
// 8. SERVO AVAILABILITY CONTROLLED BY IR SENSOR
//
// Bowl detected: attach servo and set it to closed.
// Bowl missing: detach servo and cancel pending closure.
// No servo commands are sent while bowl is missing.
// =====================================================

void updateServoAvailability() {

  if (bowlDetected) {

    if (!servoAttached) {

      myServo.attach(SERVO_PIN);
      servoAttached = true;

      myServo.write(SERVO_CLOSED_ANGLE);

      servoClosePending = false;
    }

  } else {

    if (servoAttached) {

      myServo.detach();
      servoAttached = false;
    }

    servoClosePending = false;
  }
}

// =====================================================
// 10. INDIVIDUAL LED ANIMATIONS
//
// Priority:
// 1. Bowl alarm = RED blinking
// 2. Feeding = GREEN blinking
// 3. Normal operation = BLUE ON
// =====================================================

void updateLeds(bool gateOpen) {

  unsigned long now = millis();

  // BOWL MISSING: RED LED BLINKS

  if (bowlAlarmActive) {

    if (now - lastBowlRedBlink >= RED_BLINK_INTERVAL_MS) {

      lastBowlRedBlink = now;
      bowlRedBlinkOn = !bowlRedBlinkOn;
    }

    digitalWrite(
      RED_LED_PIN,
      bowlRedBlinkOn ? HIGH : LOW
    );

    digitalWrite(GREEN_LED_PIN, LOW);
    digitalWrite(BLUE_LED_PIN, LOW);

    return;
  }

  // FEEDING: GREEN LED BLINKS

  if (gateOpen) {

    if (now - lastGreenBlink >= GREEN_BLINK_INTERVAL_MS) {

      lastGreenBlink = now;
      greenBlinkOn = !greenBlinkOn;
    }

    digitalWrite(RED_LED_PIN, LOW);

    digitalWrite(
      GREEN_LED_PIN,
      greenBlinkOn ? HIGH : LOW
    );

    digitalWrite(BLUE_LED_PIN, LOW);

    return;
  }

  // NORMAL OPERATION: BLUE LED ON

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(BLUE_LED_PIN, HIGH);
}

// =====================================================
// 11. BUZZER PATTERN
// Bowl missing: three short high-pitched beeps.
// =====================================================

void updateBuzzer() {

  unsigned long now = millis();

  int frequency = 0;

  if (bowlAlarmActive) {

    unsigned long phase = now % BUZZER_CYCLE_MS;

    if (
      (phase >= BUZZER_BEEP_1_START &&
       phase < BUZZER_BEEP_1_END) ||

      (phase >= BUZZER_BEEP_2_START &&
       phase < BUZZER_BEEP_2_END) ||

      (phase >= BUZZER_BEEP_3_START &&
       phase < BUZZER_BEEP_3_END)
    ) {

      frequency = BUZZER_FREQUENCY;
    }
  }

  setBuzzerFrequency(frequency);
}

// =====================================================
// 12. BUTTON 3 AUTOMATIC FEED SEQUENCE
// =====================================================

void startAutoFeedCountdown(AutoFeedState countdownState) {

  RtcDateTime now = rtc.GetDateTime();

  if (!rtcReady || !now.IsValid()) {

    Serial.println("DS1302 not ready. Automatic feeding unavailable.");
    autoFeedState = AUTO_IDLE;
    autoFeedDeadlineSeconds = 0;
    return;
  }

  unsigned long countdownSeconds = AUTO_FEED_COUNTDOWN_SECONDS;

  if (countdownState == AUTO_WAIT_SECOND_FEED) {

    countdownSeconds = AUTO_FEED_GAP_SECONDS;
  }

  autoFeedDeadlineSeconds =
      now.TotalSeconds64() + countdownSeconds;

  autoFeedState = countdownState;

  Serial.print("Automatic feed countdown started: ");
  Serial.print(countdownSeconds);
  Serial.println(" seconds.");
}

void updateAutoButton() {

  int reading = digitalRead(BUTTON_AUTO_PIN);

  if (reading != lastAutoButtonReading) {

    lastAutoButtonDebounceTime = millis();
    lastAutoButtonReading = reading;
  }

  if (
    millis() - lastAutoButtonDebounceTime >= RESET_DEBOUNCE_MS &&
    reading != stableAutoButtonState
  ) {

    stableAutoButtonState = reading;

    if (stableAutoButtonState == LOW && autoFeedState == AUTO_IDLE) {

      startAutoFeedCountdown(AUTO_WAIT_FIRST_FEED);
    }
  }
}

void updateAutoFeedTimer(bool physicalButtonPressed) {

  if (
    autoButtonPulseActive &&
    (long)(millis() - autoButtonPulseDeadline) >= 0
  ) {

    autoButtonPulseActive = false;
  }

  if (
    autoFeedState != AUTO_WAIT_FIRST_FEED &&
    autoFeedState != AUTO_WAIT_SECOND_FEED
  ) {

    return;
  }

  RtcDateTime now = rtc.GetDateTime();

  if (!now.IsValid()) {

    Serial.println("DS1302 time invalid. Automatic sequence cancelled.");
    rtcReady = false;
    autoFeedState = AUTO_IDLE;
    autoFeedDeadlineSeconds = 0;
    return;
  }

  // Do not interrupt a manual feeding cycle.
  if (
    now.TotalSeconds64() >= autoFeedDeadlineSeconds &&
    !physicalButtonPressed &&
    !motorRunningOn &&
    !servoClosePending &&
    !autoButtonPulseActive
  ) {

    autoButtonPulseActive = true;
    autoButtonPulseDeadline = millis() + AUTO_BUTTON_PULSE_MS;

    if (autoFeedState == AUTO_WAIT_FIRST_FEED) {

      autoFeedState = AUTO_WAIT_FIRST_CYCLE;
      Serial.println("Automatic feed 1 started.");

    } else {

      autoFeedState = AUTO_WAIT_SECOND_CYCLE;
      Serial.println("Automatic feed 2 started.");
    }
  }
}

void updateAutoFeedCycleCompletion() {

  bool feedingCycleComplete =
      !motorRunningOn &&
      !servoClosePending &&
      !buttonWasPressed &&
      !autoButtonPulseActive;

  if (
    autoFeedState == AUTO_WAIT_FIRST_CYCLE &&
    feedingCycleComplete
  ) {

    // Start second countdown after first cycle finishes.
    startAutoFeedCountdown(AUTO_WAIT_SECOND_FEED);

  } else if (
    autoFeedState == AUTO_WAIT_SECOND_CYCLE &&
    feedingCycleComplete
  ) {

    autoFeedState = AUTO_IDLE;
    autoFeedDeadlineSeconds = 0;
    Serial.println("Automatic feeding sequence complete.");
  }
}

uint64_t getAutoFeedSecondsRemaining() {

  RtcDateTime now = rtc.GetDateTime();

  if (!now.IsValid() || now.TotalSeconds64() >= autoFeedDeadlineSeconds) {

    return 0;
  }

  return autoFeedDeadlineSeconds - now.TotalSeconds64();
}

// =====================================================
// IOT DASHBOARD (ADDED ONLY; DOES NOT CONTROL FEEDING)
// =====================================================

// Use the existing DS1302 RTC for timestamps.

void iotRtcTimestamp(char output[20]) {

  RtcDateTime now = rtc.GetDateTime();

  if (!rtcReady || !now.IsValid()) {

    snprintf(output, 20, "RTC unavailable");
    return;
  }

  snprintf(
    output,
    20,
    "%04u-%02u-%02u %02u:%02u:%02u",
    (unsigned int)now.Year(),
    (unsigned int)now.Month(),
    (unsigned int)now.Day(),
    (unsigned int)now.Hour(),
    (unsigned int)now.Minute(),
    (unsigned int)now.Second()
  );
}

const char* iotModeName(uint8_t mode) {

  if (mode == 2) return "Automatic feed 1";
  if (mode == 3) return "Automatic feed 2";

  return "Manual feed";
}

// Passive monitoring:
// Does not modify servo, button or motor state.

void iotObserveFeeding() {

  bool cycleBusy =
      buttonWasPressed ||
      motorRunningOn ||
      servoClosePending ||
      autoButtonPulseActive;

  if (cycleBusy && !iotFeedCycleTracked) {

    iotFeedCycleTracked = true;

    iotRtcTimestamp(iotCycleStart);

    iotCycleMode = 1;

    if (autoFeedState == AUTO_WAIT_FIRST_CYCLE) {
      iotCycleMode = 2;
    }

    if (autoFeedState == AUTO_WAIT_SECOND_CYCLE) {
      iotCycleMode = 3;
    }
  }

  if (!cycleBusy && iotFeedCycleTracked) {

    iotFeedCycleTracked = false;

    IotFeedLog& entry = iotLogStore.logs[iotLogStore.next];

    snprintf(
      entry.startedAt,
      sizeof(entry.startedAt),
      "%s",
      iotCycleStart
    );

    iotRtcTimestamp(entry.completedAt);
    entry.mode = iotCycleMode;

    iotLogStore.next =
        (iotLogStore.next + 1) % IOT_MAX_LOGS;

    if (iotLogStore.count < IOT_MAX_LOGS) {
      iotLogStore.count++;
    }

    // Save recent logs permanently in flash.
    if (iotStorageReady) {

      iotPreferences.putBytes(
        "feedlogs",
        &iotLogStore,
        sizeof(iotLogStore)
      );
    }
  }
}

// Matches the existing LCD timer.

String iotCountdownText() {

  if (
    autoFeedState == AUTO_WAIT_FIRST_FEED ||
    autoFeedState == AUTO_WAIT_SECOND_FEED
  ) {

    return String(
      (unsigned long)getAutoFeedSecondsRemaining()
    ) + "s";
  }

  return String(hoursUntilFeeding) + "h";
}

String iotFeedStateText() {

  if (bowlAlarmActive) {
    return "Bowl missing: please realign";
  }

  if (
    bowlDetected &&
    servoAttached &&
    (buttonWasPressed || servoClosePending)
  ) {

    return "Feeding now";
  }

  if (autoFeedState == AUTO_WAIT_FIRST_FEED) {
    return "Automatic feed 1 countdown";
  }

  if (autoFeedState == AUTO_WAIT_SECOND_FEED) {
    return "Automatic feed 2 countdown";
  }

  if (
    autoFeedState == AUTO_WAIT_FIRST_CYCLE ||
    autoFeedState == AUTO_WAIT_SECOND_CYCLE
  ) {

    return "Feeding cycle in progress";
  }

  return "Standby";
}

// =====================================================
// IOT JSON DATA
// =====================================================

String iotStatusJson() {

  String json;
  json.reserve(280);

  json += "{\"foodValid\":";
  json += (iotFoodLevelValid ? "true" : "false");

  json += ",\"foodPercent\":";
  json += String(iotFoodPercentage);

  json += ",\"timer\":\"";
  json += iotCountdownText();

  json += "\",\"state\":\"";
  json += iotFeedStateText();

  json += "\",\"feedInProgress\":";
  json += (iotFeedCycleTracked ? "true" : "false");

  json += ",\"logCount\":";
  json += String(iotLogStore.count);

  json += "}";

  return json;
}

String iotLogsJson() {

  String json;
  json.reserve(200 + IOT_MAX_LOGS * 110);

  json += "[";

  for (uint16_t i = 0; i < iotLogStore.count; ++i) {

    int index =
        (iotLogStore.next + IOT_MAX_LOGS - 1 - i)
        % IOT_MAX_LOGS;

    const IotFeedLog& entry = iotLogStore.logs[index];

    if (i) {
      json += ',';
    }

    json += "{\"type\":\"";
    json += iotModeName(entry.mode);

    json += "\",\"startedAt\":\"";
    json += entry.startedAt;

    json += "\",\"completedAt\":\"";
    json += entry.completedAt;

    json += "\"}";
  }

  json += "]";

  return json;
}

// =====================================================
// WEBPAGE HTML + CSS + JAVASCRIPT
// =====================================================

const char IOT_HTML[] PROGMEM = R"iothtml(
<!doctype html>
<html lang="en">

<head>

<meta charset="utf-8">

<meta name="viewport"
content="width=device-width, initial-scale=1">

<title>Turtle Feeder | Live Monitor</title>

<style>

:root {
  color-scheme: dark;
}

* {
  box-sizing: border-box;
}

body {
  margin: 0;
  background: #10171b;
  color: #edf5f3;
  font-family: system-ui, -apple-system, 'Segoe UI', sans-serif;
}

main {
  max-width: 790px;
  margin: 0 auto;
  padding: 24px 16px 48px;
}

header {
  display: flex;
  gap: 12px;
  flex-wrap: wrap;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 24px;
}

h1 {
  font-size: clamp(1.45rem, 4vw, 2rem);
  margin: 0;
}

h2 {
  font-size: 1.02rem;
  margin: 0 0 14px;
}

.sub {
  color: #a8bab7;
  font-size: .9rem;
  margin-top: 7px;
}

#connection {
  padding: 7px 12px;
  background: #223c32;
  color: #80e7b6;
  border-radius: 99px;
  font-weight: 650;
  font-size: .79rem;
}

.offline {
  background: #432629 !important;
  color: #ff9a98 !important;
}

.grid {
  display: grid;
  grid-template-columns: repeat(2, minmax(0,1fr));
  gap: 14px;
}

.card {
  border: 1px solid #2a3b40;
  border-radius: 15px;
  padding: 21px;
  background: #192529;
}

.label {
  font-size: .82rem;
  text-transform: uppercase;
  letter-spacing: .1em;
  color: #b7c8c4;
}

.value {
  font-size: clamp(2.1rem, 7vw, 3.2rem);
  font-weight: 760;
  margin: 10px 0 15px;
}

.meter {
  width: 100%;
  height: 13px;
  background: #304145;
  overflow: hidden;
  border-radius: 40px;
}

#fill {
  height: 100%;
  width: 0%;
  background: #5ce1a5;
  transition: width .3s;
}

.hint {
  color: #a8bab7;
  margin: 10px 0 0;
  font-size: .82rem;
}

.wide {
  margin-top: 14px;
}

#state {
  font-size: 1.25rem;
  font-weight: 650;
  color: #9aebc7;
}

.logwrap {
  overflow-x: auto;
}

table {
  width: 100%;
  border-collapse: collapse;
  text-align: left;
  min-width: 520px;
}

th, td {
  padding: 12px 10px;
  border-bottom: 1px solid #304044;
  font-size: .88rem;
}

th {
  color: #a8bab7;
  font-weight: 600;
}

#empty {
  color: #b7c8c4;
  padding: 10px 0;
}

footer {
  margin-top: 20px;
  color: #92a6a1;
  font-size: .78rem;
  line-height: 1.55;
}

@media (max-width: 520px) {
  .grid {
    grid-template-columns: 1fr;
  }
}

</style>

</head>

<body>

<main>

<header>
  <div>
    <h1>Turtle Feeder</h1>
    <div class="sub">
      ESP32 &bull; Live monitoring dashboard
    </div>
  </div>

  <span id="connection" class="offline">
    CONNECTING
  </span>
</header>

<div class="grid">

<section class="card">

  <div class="label">Food remaining</div>

  <div class="value" id="food">--%</div>

  <div class="meter">
    <div id="fill"></div>
  </div>

  <p class="hint">
    Same sonar-derived percentage as LCD
  </p>

</section>

<section class="card">

  <div class="label">Feeding in</div>

  <div class="value" id="timer">--</div>

  <p class="hint">
    Matches LCD timer display
  </p>

</section>

</div>

<section class="card wide">

  <h2>Current status</h2>

  <div id="state">
    Connecting to feeder...
  </div>

</section>

<section class="card wide">

  <h2>
    Feeding history
    <span class="sub" id="log-count"></span>
  </h2>

  <div class="logwrap">

    <table>

      <thead>
        <tr>
          <th>Feeding type</th>
          <th>Started (RTC)</th>
          <th>Completed (RTC)</th>
        </tr>
      </thead>

      <tbody id="logrows"></tbody>

    </table>

  </div>

  <div id="empty">
    No completed feeding cycles logged yet.
  </div>

</section>

<footer>
  Read-only local dashboard.
  Logs record the completion of the programmed feeding
  cycle, not independent confirmation that food was
  dispensed. Timestamps come from the DS1302 RTC.
</footer>

</main>

<script>

const el = id => document.getElementById(id);

let lastCount = -1;

async function refreshStatus() {

  try {

    const response = await fetch(
      '/api/status',
      {cache: 'no-store'}
    );

    if (!response.ok) {
      throw Error('status unavailable');
    }

    const data = await response.json();

    el('connection').textContent = 'LIVE';
    el('connection').classList.remove('offline');

    el('food').textContent =
      data.foodValid ? data.foodPercent + '%' : '--%';

    el('fill').style.width =
      data.foodValid ? data.foodPercent + '%' : '0%';

    el('timer').textContent = data.timer;

    el('state').textContent = data.state;

    if (lastCount !== data.logCount) {

      lastCount = data.logCount;

      refreshLogs();
    }

  } catch (error) {

    el('connection').textContent = 'DISCONNECTED';
    el('connection').classList.add('offline');
  }
}

async function refreshLogs() {

  try {

    const response = await fetch(
      '/api/logs',
      {cache: 'no-store'}
    );

    if (!response.ok) {
      throw Error('logs unavailable');
    }

    const logs = await response.json();

    const body = el('logrows');

    body.replaceChildren();

    for (const log of logs) {

      const tr = document.createElement('tr');

      for (const field of [
        'type',
        'startedAt',
        'completedAt'
      ]) {

        const td = document.createElement('td');

        td.textContent = log[field];

        tr.appendChild(td);
      }

      body.appendChild(tr);
    }

    el('log-count').textContent =
      '(' + logs.length + ' most recent)';

    el('empty').hidden = logs.length > 0;

  } catch (error) {

    // Status indicator handles temporary WiFi failure.
  }
}

refreshStatus();
refreshLogs();

setInterval(refreshStatus, 1000);
setInterval(refreshLogs, 2000);

</script>

</body>
</html>
)iothtml";

// =====================================================
// WEB SERVER INITIALIZATION
// =====================================================

void iotStart() {

  // Load previous feeding logs from flash.

  iotStorageReady =
      iotPreferences.begin("feeder", false);

  if (
    iotStorageReady &&
    iotPreferences.getBytesLength("feedlogs")
      == sizeof(iotLogStore)
  ) {

    iotPreferences.getBytes(
      "feedlogs",
      &iotLogStore,
      sizeof(iotLogStore)
    );
  }

  if (
    iotLogStore.magic != IOT_LOG_MAGIC ||
    iotLogStore.count > IOT_MAX_LOGS ||
    iotLogStore.next >= IOT_MAX_LOGS
  ) {

    memset(&iotLogStore, 0, sizeof(iotLogStore));

    iotLogStore.magic = IOT_LOG_MAGIC;
  }

  // Main website

  iotServer.on("/", HTTP_GET, []() {

    iotServer.sendHeader(
      "Cache-Control",
      "no-store"
    );

    iotServer.send_P(
      200,
      "text/html; charset=utf-8",
      IOT_HTML
    );
  });

  // Live status API

  iotServer.on("/api/status", HTTP_GET, []() {

    iotServer.sendHeader(
      "Cache-Control",
      "no-store"
    );

    iotServer.send(
      200,
      "application/json",
      iotStatusJson()
    );
  });

  // Feeding history API

  iotServer.on("/api/logs", HTTP_GET, []() {

    iotServer.sendHeader(
      "Cache-Control",
      "no-store"
    );

    iotServer.send(
      200,
      "application/json",
      iotLogsJson()
    );
  });

  // Start WiFi without blocking the feeder.

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);

  WiFi.begin(
    IOT_WIFI_SSID,
    IOT_WIFI_PASSWORD
  );

  iotLastWiFiRetry = millis();

  Serial.println(
    "IoT WiFi connecting in the background..."
  );
}

void iotService() {

  if (WiFi.status() == WL_CONNECTED) {

    if (!iotHttpStarted) {

      iotServer.begin();

      iotHttpStarted = true;

      Serial.print("IoT dashboard: http://");
      Serial.println(WiFi.localIP());
    }

    iotServer.handleClient();

  } else if (
    millis() - iotLastWiFiRetry >= IOT_WIFI_RETRY_MS
  ) {

    iotLastWiFiRetry = millis();

    WiFi.reconnect();
  }
}

// =====================================================
// 12. SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // SONAR
  pinMode(SONAR_TRIG_PIN, OUTPUT);
  pinMode(SONAR_ECHO_PIN, INPUT);

  // BUTTONS
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON_RESET_PIN, INPUT_PULLUP);

  // IR SENSOR
  pinMode(IR_SENSOR_PIN, INPUT);

  // BUZZER
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);

  // INDIVIDUAL LEDS
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(BLUE_LED_PIN, OUTPUT);

  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(BLUE_LED_PIN, LOW);

  // Servo is attached only after IR detects bowl.

  // L298N MOTOR DRIVER
  pinMode(MOTOR_EN_PIN, OUTPUT);
  pinMode(MOTOR_IN1_PIN, OUTPUT);
  pinMode(MOTOR_IN2_PIN, OUTPUT);

  stopMotor();

  // INITIALIZE SONAR BUFFER
  for (int i = 0; i < FILTER_WINDOW; i++) {

    readings[i] = 0.0;
  }

  // LCD
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);

  lcd.init();
  lcd.backlight();
  lcd.clear();

  // BUTTON 3: GPIO34 uses external 10k pull-up.
  pinMode(BUTTON_AUTO_PIN, INPUT);

  // DS1302 RTC
  rtc.Begin();

  if (rtc.GetIsWriteProtected()) {

    rtc.SetIsWriteProtected(false);
  }

  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  // Set clock only if invalid or stopped.
  if (!rtc.IsDateTimeValid() || !rtc.GetIsRunning()) {

    rtc.SetDateTime(compiled);
    rtc.SetIsRunning(true);
  }

  rtcReady =
      rtc.IsDateTimeValid() &&
      rtc.GetIsRunning();

  if (rtcReady) {

    Serial.println("DS1302 RTC ready.");

  } else {

    Serial.println("DS1302 RTC error.");
  }

  // STARTUP SCREEN
  lcd.setCursor(0, 0);
  lcd.print("System Booting...");

  delay(1500);

  lcd.clear();

  // NORMAL OPERATION: BLUE LED ON
  digitalWrite(BLUE_LED_PIN, HIGH);

  Serial.println("Turtle feeder ready.");

  // IoT: starts WiFi without blocking feeder.
  iotStart();
}

// =====================================================
// 13. MAIN LOOP
// =====================================================

void loop() {

  // ===================================================
  // 1. BUTTON 2: RESET SONAR AFTER REFILLING
  // ===================================================

  int resetReading = digitalRead(BUTTON_RESET_PIN);

  if (resetReading != lastResetButtonReading) {

    lastResetDebounceTime = millis();
    lastResetButtonReading = resetReading;
  }

  if (
    millis() - lastResetDebounceTime >= RESET_DEBOUNCE_MS &&
    resetReading != stableResetButtonState
  ) {

    stableResetButtonState = resetReading;

    if (stableResetButtonState == LOW) {

      // Clear moving-average buffer
      for (int i = 0; i < FILTER_WINDOW; i++) {

        readings[i] = 0.0;
      }

      total = 0.0;
      readIndex = 0;
      validReadings = 0;

      // Establish fresh sonar baseline
      lastValidDistance = -1.0;
      isFiltering = false;

      Serial.println("Sonar reset after refill.");
    }
  }

  // ===================================================
  // 2. BUTTON 3: START AUTOMATIC FEEDING SEQUENCE
  // ===================================================

  updateAutoButton();

  // ===================================================
  // 3. UPDATE IR BOWL SENSOR AND SERVO AVAILABILITY
  // ===================================================

  updateBowlSensor();
  updateServoAvailability();

  bool physicalButtonPressed =
      (digitalRead(BUTTON_PIN) == LOW);

  // Automatic pulse acts like Button 1 press.
  updateAutoFeedTimer(physicalButtonPressed);

  // ===================================================
  // 4. BUTTON 1: SERVO + MOTOR JITTER
  //
  // Original servo and motor control preserved.
  // ===================================================

  bool buttonPressed =
      physicalButtonPressed ||
      autoButtonPulseActive;

  if (buttonPressed) {

    // OPEN SERVO ONLY WHEN BOWL IS DETECTED

    if (bowlDetected && servoAttached) {

      myServo.write(SERVO_OPEN_ANGLE);
      servoClosePending = false;
    }

    // START MOTOR ON INITIAL BUTTON PRESS

    if (!buttonWasPressed) {

      motorRunningOn = true;
      motorStopDeadline = 0;

      setMotorPhase(0);

    } else {

      updateMotorJitter();
    }

  } else {

    // START SERVO CLOSING COUNTDOWN AFTER RELEASE

    if (
      buttonWasPressed &&
      bowlDetected &&
      servoAttached
    ) {

      servoCloseDeadline =
          millis() + SERVO_CLOSE_DELAY_MS;

      servoClosePending = true;
    }

    // CLOSE SERVO AFTER CONFIGURED DELAY

    if (
      bowlDetected &&
      servoAttached &&
      servoClosePending &&
      (long)(millis() - servoCloseDeadline) >= 0
    ) {

      myServo.write(SERVO_CLOSED_ANGLE);
      servoClosePending = false;
    }

    // MOTOR RUN-ON TIMER

    if (buttonWasPressed && motorRunningOn) {

      motorStopDeadline =
          millis() + MOTOR_RUN_ON_MS;
    }

    // KEEP MOTOR RUNNING UNTIL DEADLINE

    if (motorRunningOn) {

      if (
        motorStopDeadline == 0 ||
        (long)(millis() - motorStopDeadline) < 0
      ) {

        updateMotorJitter();

      } else {

        stopMotor();

        motorRunningOn = false;
        motorStopDeadline = 0;
        motorPhase = 0;
      }
    }
  }

  buttonWasPressed = buttonPressed;

  updateAutoFeedCycleCompletion();

  // ===================================================
  // 5. SONAR MEASUREMENTS
  // Pause while motor or button is active.
  // ===================================================

  if (
    !motorRunningOn &&
    !buttonPressed &&
    millis() - lastSonarReadingTime >= SONAR_INTERVAL_MS
  ) {

    lastSonarReadingTime = millis();

    digitalWrite(SONAR_TRIG_PIN, LOW);
    delayMicroseconds(2);

    digitalWrite(SONAR_TRIG_PIN, HIGH);
    delayMicroseconds(10);

    digitalWrite(SONAR_TRIG_PIN, LOW);

    unsigned long duration =
        pulseIn(SONAR_ECHO_PIN, HIGH, 30000);

    // IGNORE MEASUREMENTS WITHOUT ECHO

    if (duration > 0) {

      float newDistance =
          (float)duration * 0.0343 / 2.0;

      bool acceptReading = false;

      if (
        newDistance > 0.0 &&
        newDistance < 500.0
      ) {

        // FIRST VALID READING

        if (lastValidDistance < 0.0) {

          acceptReading = true;
        }

        // REJECT DECREASES

        else if (newDistance < lastValidDistance) {

          isFiltering = true;
        }

        else {

          float increase =
              newDistance - lastValidDistance;

          if (increase <= MAX_JUMP_CM) {

            acceptReading = true;
            isFiltering = false;

          } else {

            isFiltering = true;
          }
        }

        // UPDATE MOVING AVERAGE

        if (acceptReading) {

          lastValidDistance = newDistance;

          total -= readings[readIndex];

          readings[readIndex] = newDistance;

          total += newDistance;

          readIndex =
              (readIndex + 1) % FILTER_WINDOW;

          if (validReadings < FILTER_WINDOW) {

            validReadings++;
          }
        }
      }
    }
  }

  // ===================================================
  // 5. CALCULATE SMOOTH DISTANCE
  // ===================================================

  float smoothDistance = -1.0;

  if (validReadings > 0) {

    smoothDistance =
        total / (float)validReadings;
  }

  // ===================================================
  // 6. CONVERT DISTANCE INTO FOOD PERCENTAGE
  // ===================================================

  long distancePercentage = 0;

  if (smoothDistance >= 0.0) {

    float percentageCalc =
        ((DISTANCE_MAX - smoothDistance) /
         (DISTANCE_MAX - DISTANCE_MIN)) * 100.0;

    distancePercentage =
        constrain((long)percentageCalc, 0, 100);
  }

  // ===================================================
  // 7. SERIAL MONITOR
  // ===================================================

  Serial.print("Distance: ");

  if (smoothDistance >= 0.0) {

    Serial.print(smoothDistance, 2);
    Serial.print(" cm");

  } else {

    Serial.print("--");
  }

  Serial.print(" | Percentage: ");

  if (smoothDistance >= 0.0) {

    Serial.print(distancePercentage);
    Serial.print("%");

  } else {

    Serial.print("--");
  }

  Serial.print(" | Filtering: ");
  Serial.print(isFiltering ? "YES" : "NO");

  Serial.print(" | Motor: ");
  Serial.print(
    motorRunningOn ? "JITTERING" : "STOPPED"
  );

  Serial.print(" | Feeding in: ");
  Serial.print(hoursUntilFeeding);
  Serial.println("h");

  // ===================================================
  // 8. LCD DISPLAY
  // ===================================================

  if (
    millis() - lastLcdUpdate >= LCD_REFRESH_INTERVAL
  ) {

    lastLcdUpdate = millis();

    if (bowlAlarmActive) {

      printLcdRow(0, "PLEASE REALIGN");
      printLcdRow(1, "THE BOWL");

    } else if (
      bowlDetected &&
      servoAttached &&
      (buttonPressed || servoClosePending)
    ) {

      // LCD STATUS ONLY

      printLcdRow(0, "feeding now");
      printLcdRow(1, "");

    } else {

      // LINE 1: FOOD LEVEL

      lcd.setCursor(0, 0);
      lcd.print("Food Level: ");

      if (smoothDistance >= 0.0) {

        lcd.print(distancePercentage);
        lcd.print("%");

        if (distancePercentage == 100) {

          lcd.print("   ");

        } else if (distancePercentage >= 10) {

          lcd.print("    ");

        } else {

          lcd.print("     ");
        }

      } else {

        lcd.print("--%     ");
      }

      // LINE 2: FEEDING COUNTDOWN

      lcd.setCursor(0, 1);
      lcd.print("Feeding in: ");

      if (
        autoFeedState == AUTO_WAIT_FIRST_FEED ||
        autoFeedState == AUTO_WAIT_SECOND_FEED
      ) {

        lcd.print(
          (unsigned long)getAutoFeedSecondsRemaining()
        );

        lcd.print("s   ");

      } else {

        lcd.print(hoursUntilFeeding);
        lcd.print("h   ");
      }
    }
  }

  // ===================================================
  // 9. UPDATE LEDS AND BUZZER
  // ===================================================

  bool autoFeedServoMoving =
      (autoFeedState == AUTO_WAIT_FIRST_CYCLE ||
       autoFeedState == AUTO_WAIT_SECOND_CYCLE) &&
      (buttonPressed || servoClosePending);

  updateLeds(
    buttonPressed || autoFeedServoMoving
  );

  updateBuzzer();

  // ===================================================
  // 10. IOT LIVE MONITORING (ADDED ONLY)
  // ===================================================

  // Copy values already calculated by the original code.

  iotFoodLevelValid =
      (smoothDistance >= 0.0);

  if (iotFoodLevelValid) {

    iotFoodPercentage = distancePercentage;
  }

  // Observe feeding without controlling hardware.

  iotObserveFeeding();

  // Serve website without blocking main operation.

  iotService();
}
