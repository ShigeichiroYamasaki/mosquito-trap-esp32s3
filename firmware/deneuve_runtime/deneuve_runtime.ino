#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "DetectionResult.h"
#include "OpticalRules.h"

// =====================
// Wi-Fi
// =====================
#if __has_include("secrets.h")
#include "secrets.h"
#else
#include "secrets.example.h"
#endif

WebServer server(80);

// =====================
// Pins
// =====================
const int MUX1_SIG_PIN = 1;
const int MUX2_SIG_PIN = 2;

const int MUX_S0_PIN = 6;
const int MUX_S1_PIN = 7;
const int MUX_S2_PIN = 8;
const int MUX_S3_PIN = 9;

const int IR_LEFT_PIN  = 10;
const int IR_RIGHT_PIN = 11;

const int SERVO_PIN = 16;

// =====================
// Sensor constants
// =====================
const int NUM_PIXELS = 18;
const int NUM_LEDS = 2;

const int LED_LEFT = 0;
const int LED_RIGHT = 1;

const int ADC_SAMPLES = 3;
const int LED_SETTLE_US = 2000; // provisional: tune using optical_measure
const int OFF_SETTLE_US = 2000;

const float SHADOW_THRESHOLD = 0.15;
const float LOCAL_CONTRAST_THRESHOLD = 0.08;
const float MIN_TOTAL_SHADOW = 0.25;
const int MIN_ACTIVE_PIXELS = 1;
const int MAX_ACTIVE_PIXELS = 8;

// =====================
// Pixel coordinates: staggered 6 x 3
// =====================
float px[NUM_PIXELS] = {
  0, 10, 20, 30, 40, 50,
  5, 15, 25, 35, 45, 55,
  0, 10, 20, 30, 40, 50
};

float py[NUM_PIXELS] = {
  0, 0, 0, 0, 0, 0,
  10, 10, 10, 10, 10, 10,
  20, 20, 20, 20, 20, 20
};

// =====================
// Buffers
// =====================
float signalNow[NUM_LEDS][NUM_PIXELS];
float baseline[NUM_LEDS][NUM_PIXELS];
bool validPixel[NUM_LEDS][NUM_PIXELS];
int rawOff[NUM_PIXELS], rawOn[NUM_LEDS][NUM_PIXELS];
uint32_t frameUs = 0, framePeriodUs = 0, previousFrameStart = 0;
uint32_t phaseUs[3];
OpticalTrack tracks[2];
bool autoCapture = false; // arm after confirming vacuum is physically OFF
bool frameHealthy = true;
float shadowImg[NUM_LEDS][NUM_PIXELS];
float mergedShadow[NUM_PIXELS];
float prevMergedShadow[NUM_PIXELS];

// =====================
// Detection log
// =====================
struct DetectionLog {
  unsigned long timeMs;
  int activeCount;
  float totalShadow;
  float motion;
  float localContrast;
  float cx;
  float cy;
};

const int LOG_SIZE = 50;
DetectionLog logs[LOG_SIZE];
int logIndex = 0;
int logCount = 0;

void addLog(int active, float total, float motion, float local, float cx, float cy) {
  logs[logIndex] = { millis(), active, total, motion, local, cx, cy };
  logIndex = (logIndex + 1) % LOG_SIZE;
  if (logCount < LOG_SIZE) logCount++;
}

// =====================
// Servo state machine
// =====================
Servo vacuumServo;

const int SERVO_REST_ANGLE  = 20;
const int SERVO_PRESS_ANGLE = 65;

const unsigned long PRESS_MS = 250;
const unsigned long RELEASE_MS = 300;
const unsigned long STRONG_WIND_MS = 3000;
const unsigned long RETRIGGER_BLOCK_MS = 8000;

enum VacuumSeqState {
  VAC_IDLE,
  VAC_PRESS1,
  VAC_RELEASE1,
  VAC_PRESS2,
  VAC_RELEASE2,
  VAC_SUCTION,
  VAC_PRESS3,
  VAC_RELEASE3
};

VacuumSeqState vacState = VAC_IDLE;
unsigned long vacStateStartedAt = 0;
unsigned long lastVacuumTriggerAt = 0;

void setupVacuumServo() {
  vacuumServo.setPeriodHertz(50);
  vacuumServo.attach(SERVO_PIN, 500, 2400);
  vacuumServo.write(SERVO_REST_ANGLE);
}

bool vacuumBusy() {
  return vacState != VAC_IDLE;
}

void startVacuumStrongSequence() {
  if (vacuumBusy()) return;

  unsigned long now = millis();
  if (now - lastVacuumTriggerAt < RETRIGGER_BLOCK_MS) return;

  lastVacuumTriggerAt = now;
  vacState = VAC_PRESS1;
  vacStateStartedAt = now;
  vacuumServo.write(SERVO_PRESS_ANGLE);
  Serial.println("VAC PRESS1");
}

void updateVacuumSequence() {
  unsigned long now = millis();

  switch (vacState) {
    case VAC_IDLE:
      break;

    case VAC_PRESS1:
      if (now - vacStateStartedAt >= PRESS_MS) {
        vacuumServo.write(SERVO_REST_ANGLE);
        vacState = VAC_RELEASE1;
        vacStateStartedAt = now;
        Serial.println("VAC RELEASE1");
      }
      break;

    case VAC_RELEASE1:
      if (now - vacStateStartedAt >= RELEASE_MS) {
        vacuumServo.write(SERVO_PRESS_ANGLE);
        vacState = VAC_PRESS2;
        vacStateStartedAt = now;
        Serial.println("VAC PRESS2 -> STRONG");
      }
      break;

    case VAC_PRESS2:
      if (now - vacStateStartedAt >= PRESS_MS) {
        vacuumServo.write(SERVO_REST_ANGLE);
        vacState = VAC_RELEASE2;
        vacStateStartedAt = now;
        Serial.println("VAC RELEASE2");
      }
      break;

    case VAC_RELEASE2:
      if (now - vacStateStartedAt >= RELEASE_MS) {
        vacState = VAC_SUCTION;
        vacStateStartedAt = now;
        Serial.println("VAC STRONG SUCTION");
      }
      break;

    case VAC_SUCTION:
      if (now - vacStateStartedAt >= STRONG_WIND_MS) {
        vacuumServo.write(SERVO_PRESS_ANGLE);
        vacState = VAC_PRESS3;
        vacStateStartedAt = now;
        Serial.println("VAC PRESS3 -> STOP");
      }
      break;

    case VAC_PRESS3:
      if (now - vacStateStartedAt >= PRESS_MS) {
        vacuumServo.write(SERVO_REST_ANGLE);
        vacState = VAC_RELEASE3;
        vacStateStartedAt = now;
        Serial.println("VAC RELEASE3");
      }
      break;

    case VAC_RELEASE3:
      if (now - vacStateStartedAt >= RELEASE_MS) {
        vacState = VAC_IDLE;
        Serial.println("VAC DONE");
      }
      break;
  }
}

// =====================
// MUX / ADC
// =====================
void setMuxChannel(int ch) {
  digitalWrite(MUX_S0_PIN, ch & 0x01);
  digitalWrite(MUX_S1_PIN, ch & 0x02);
  digitalWrite(MUX_S2_PIN, ch & 0x04);
  digitalWrite(MUX_S3_PIN, ch & 0x08);
  delayMicroseconds(50);
}

int readAdcAverage(int pin) {
  analogRead(pin); // discard first sample after MUX change
  long sum = 0;
  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(pin);
    delayMicroseconds(15);
  }
  return sum / ADC_SAMPLES;
}

int readPixelRaw(int pixel) {
  if (pixel < 16) {
    setMuxChannel(pixel);
    return readAdcAverage(MUX1_SIG_PIN);
  } else {
    setMuxChannel(pixel - 16);
    return readAdcAverage(MUX2_SIG_PIN);
  }
}

// =====================
// IR LED control
// =====================
void allIrOff() {
  digitalWrite(IR_LEFT_PIN, LOW);
  digitalWrite(IR_RIGHT_PIN, LOW);
}

void setIrLed(int led) {
  allIrOff();
  if (led == LED_LEFT) digitalWrite(IR_LEFT_PIN, HIGH);
  if (led == LED_RIGHT) digitalWrite(IR_RIGHT_PIN, HIGH);
}

// =====================
// Optical scan
// =====================
void scanOpticalFrame() {
  uint32_t started = micros();
  framePeriodUs = previousFrameStart ? started - previousFrameStart : 0;
  previousFrameStart = started;
  allIrOff();
  delayMicroseconds(OFF_SETTLE_US);
  phaseUs[0] = micros() - started;
  for (int p = 0; p < NUM_PIXELS; p++) rawOff[p] = readPixelRaw(p);
  for (int l = 0; l < NUM_LEDS; l++) {
    setIrLed(l);
    delayMicroseconds(LED_SETTLE_US);
    phaseUs[l + 1] = micros() - started;
    for (int p = 0; p < NUM_PIXELS; p++) {
      rawOn[l][p] = readPixelRaw(p);
      signalNow[l][p] = rawOff[p] - rawOn[l][p]; // retain signed values
    }
  }
  allIrOff();
  frameUs = micros() - started;
}

void calibrateBaseline() {
  Serial.println("Calibration: keep optical path clear");
  OpticalCalibration stats[2][18] = {};
  for (int r = 0; r < 80; r++) {
    scanOpticalFrame();
    for (int l = 0; l < 2; l++) for (int p = 0; p < 18; p++)
      stats[l][p].add(rawOff[p], rawOn[l][p]);
    delay(1);
  }
  Serial.println("led,pixel,baseline,sd,valid");
  for (int l = 0; l < 2; l++) for (int p = 0; p < 18; p++) {
    baseline[l][p] = stats[l][p].mean;
    validPixel[l][p] = stats[l][p].valid();
    Serial.printf("%d,%d,%.2f,%.2f,%d\n", l, p+1, baseline[l][p], stats[l][p].sd(), validPixel[l][p]);
  }
  tracks[0].reset(); tracks[1].reset();
  Serial.println("Baseline fixed; use c to recalibrate while disarmed");
}

// =====================
// Image processing
// =====================
float d2(float x1, float y1, float x2, float y2) {
  float dx = x1 - x2;
  float dy = y1 - y2;
  return dx * dx + dy * dy;
}

void makeShadowImage() {
  for (int p = 0; p < NUM_PIXELS; p++) {
    float maxS = 0;

    for (int l = 0; l < NUM_LEDS; l++) {
      float s = validPixel[l][p] ? 1.0f - signalNow[l][p] / baseline[l][p] : 0;
      if (s < 0) s = 0;
      if (s > 1) s = 1;

      shadowImg[l][p] = s;
      if (s > maxS) maxS = s;
    }

    mergedShadow[p] = maxS;
  }
}

float localContrastAt(int i) {
  float sum = 0;
  int count = 0;

  for (int j = 0; j < NUM_PIXELS; j++) {
    if (i == j) continue;
    if (d2(px[i], py[i], px[j], py[j]) <= 125.0) {
      sum += mergedShadow[j];
      count++;
    }
  }

  if (count == 0) return 0;
  return mergedShadow[i] - sum / count;
}

DetectionResult analyzeShadow() {
  DetectionResult r = { false, 0, 0, 0, 0, 0, 0 };

  float wx = 0;
  float wy = 0;

  for (int i = 0; i < NUM_PIXELS; i++) {
    float s = mergedShadow[i];
    float lc = localContrastAt(i);
    float m = fabs(s - prevMergedShadow[i]);

    r.motion += m;
    if (lc > r.maxLocalContrast) r.maxLocalContrast = lc;

    if (s > SHADOW_THRESHOLD || lc > LOCAL_CONTRAST_THRESHOLD) {
      r.activeCount++;
      r.totalShadow += s;
      wx += px[i] * s;
      wy += py[i] * s;
    }
  }

  if (r.totalShadow > 0.001) {
    r.cx = wx / r.totalShadow;
    r.cy = wy / r.totalShadow;
  }

  bool smallEnough =
    r.activeCount >= MIN_ACTIVE_PIXELS &&
    r.activeCount <= MAX_ACTIVE_PIXELS;

  bool strongEnough = r.totalShadow >= MIN_TOTAL_SHADOW;
  bool localEnough = r.maxLocalContrast >= LOCAL_CONTRAST_THRESHOLD;
  bool movingEnough = r.motion >= 0.08;

  r.candidate = smallEnough && strongEnough && localEnough && movingEnough;
  return r;
}

bool updateMosquitoDecision(DetectionResult unused) {
  bool detected = false;
  frameHealthy = true;
  for (int l = 0; l < 2; l++) {
    float shadow[18];
    int usable = 0;
    bool healthy = true;
    for (int i = 0; i < 18; i++) {
      shadow[i] = shadowImg[l][i];
      if (validPixel[l][i]) {
        usable++;
        if (rawOn[l][i] <= 20 || rawOn[l][i] >= 4075 ||
            rawOff[i] <= 20 || rawOff[i] >= 4075 || signalNow[l][i] < -20)
          healthy = false;
      }
    }
    healthy = healthy && usable >= 12;
    frameHealthy = frameHealthy && healthy;
    detected = tracks[l].update(shadow, validPixel[l], millis(), healthy) || detected;
  }
  return detected && frameHealthy;
}

void savePrevShadow() {
  for (int i = 0; i < NUM_PIXELS; i++) {
    prevMergedShadow[i] = mergedShadow[i];
  }
}

// =====================
// Web UI
// =====================
void handleRoot() {
  String html;
  html += "<html><head><meta charset='utf-8'>";
  html += "<meta http-equiv='refresh' content='3'>";
  html += "<title>Mosquito Trap Logs</title>";
  html += "<style>body{font-family:sans-serif;margin:24px;}table{border-collapse:collapse;}td,th{border:1px solid #ccc;padding:6px 10px;}</style>";
  html += "</head><body>";
  html += "<h2>Deneuve 2 — behavior candidates</h2>";
  html += "<p>Auto capture: " + String(autoCapture ? "ARMED" : "DISARMED") + "</p>";
  html += "<p>Frame scan / period (us): " + String(frameUs) + " / " + String(framePeriodUs) + "</p>";
  html += "<p>Optics: " + String(frameHealthy ? "usable" : "check calibration / ADC") + "</p>";
  html += "<table><tr><th>PT</th><th>Left baseline / valid</th><th>Right baseline / valid</th></tr>";
  for (int i=0; i<18; i++) html += "<tr><td>" + String(i+1) + "</td><td>" + String(baseline[0][i]) + " / " + String(validPixel[0][i]) + "</td><td>" + String(baseline[1][i]) + " / " + String(validPixel[1][i]) + "</td></tr>";
  html += "</table>";
  html += "<p>Uptime: " + String(millis() / 1000) + " sec</p>";
  html += "<p>Vacuum: ";
  html += vacuumBusy() ? "BUSY" : "IDLE";
  html += "</p>";

  html += "<table><tr><th>#</th><th>time ms</th><th>active</th><th>total</th><th>motion</th><th>local</th><th>cx</th><th>cy</th></tr>";

  for (int i = 0; i < logCount; i++) {
    int idx = (logIndex - 1 - i + LOG_SIZE) % LOG_SIZE;
    html += "<tr>";
    html += "<td>" + String(i + 1) + "</td>";
    html += "<td>" + String(logs[idx].timeMs) + "</td>";
    html += "<td>" + String(logs[idx].activeCount) + "</td>";
    html += "<td>" + String(logs[idx].totalShadow, 3) + "</td>";
    html += "<td>" + String(logs[idx].motion, 3) + "</td>";
    html += "<td>" + String(logs[idx].localContrast, 3) + "</td>";
    html += "<td>" + String(logs[idx].cx, 1) + "</td>";
    html += "<td>" + String(logs[idx].cy, 1) + "</td>";
    html += "</tr>";
  }

  html += "</table>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void handleJson() {
  String json = "{\"frameUs\":" + String(frameUs) + ",\"periodUs\":" + String(framePeriodUs);
  json += ",\"phaseUs\":[" + String(phaseUs[0]) + "," + String(phaseUs[1]) + "," + String(phaseUs[2]) + "]";
  json += ",\"armed\":" + String(autoCapture ? "true" : "false") + ",\"healthy\":" + String(frameHealthy ? "true" : "false");
  json += ",\"logs\":[";
  for (int i = 0; i < logCount; i++) {
    int idx = (logIndex - 1 - i + LOG_SIZE) % LOG_SIZE;
    if (i > 0) json += ",";
    json += "{";
    json += "\"timeMs\":" + String(logs[idx].timeMs) + ",";
    json += "\"active\":" + String(logs[idx].activeCount) + ",";
    json += "\"total\":" + String(logs[idx].totalShadow, 3) + ",";
    json += "\"motion\":" + String(logs[idx].motion, 3) + ",";
    json += "\"local\":" + String(logs[idx].localContrast, 3) + ",";
    json += "\"cx\":" + String(logs[idx].cx, 1) + ",";
    json += "\"cy\":" + String(logs[idx].cy, 1);
    json += "}";
  }
  json += "]}";
  server.send(200, "application/json", json);
}

void setupWiFiAndWeb() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting WiFi");

  uint32_t wifiStarted = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStarted < 10000) {
    delay(300);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) { Serial.println("Offline: serial available"); return; }
  Serial.println();
  Serial.print("Open: http://");
  Serial.println(WiFi.localIP());

  server.on("/", handleRoot);
  server.on("/json", handleJson);
  server.begin();
}

// =====================
// Setup / Loop
// =====================
void setup() {
  Serial.begin(115200);
  delay(1500);

  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  pinMode(MUX_S0_PIN, OUTPUT);
  pinMode(MUX_S1_PIN, OUTPUT);
  pinMode(MUX_S2_PIN, OUTPUT);
  pinMode(MUX_S3_PIN, OUTPUT);

  pinMode(IR_LEFT_PIN, OUTPUT);
  pinMode(IR_RIGHT_PIN, OUTPUT);
  allIrOff();

  setupVacuumServo();
  setupWiFiAndWeb();

  for (int i = 0; i < NUM_PIXELS; i++) {
    prevMergedShadow[i] = 0;
  }

  calibrateBaseline();

  Serial.println("Ready, DISARMED. a=arm (vacuum must be OFF), d=disarm, c=recalibrate");
}

void loop() {
  if (Serial.available()) {
    char command = Serial.read();
    if (command == 'd') { autoCapture = false; Serial.println("DISARMED; active sequence completes stop press"); }
    if (command == 'a' && !vacuumBusy()) { autoCapture = true; Serial.println("ARMED"); }
    if (command == 'c' && !autoCapture && !vacuumBusy()) calibrateBaseline();
  }
  server.handleClient();
  updateVacuumSequence();

  scanOpticalFrame();
  makeShadowImage();

  DetectionResult r = analyzeShadow();
  bool detected = updateMosquitoDecision(r);
  if (vacuumBusy()) { tracks[0].reset(); tracks[1].reset(); detected = false; }

  static DetectionResult lastShadow = {};
  if (detected) {
    r = lastShadow; // event occurs on clear frame; retain preceding shadow diagnostics
    Serial.println("BEHAVIOR CANDIDATE");
    addLog(r.activeCount, r.totalShadow, r.motion, r.maxLocalContrast, r.cx, r.cy);
    if (autoCapture) startVacuumStrongSequence();
  }

  if (!detected && r.activeCount > 0) lastShadow = r;
  savePrevShadow();

  delay(1);
}
