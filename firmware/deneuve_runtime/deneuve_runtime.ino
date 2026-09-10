#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESP32Servo.h>
#include "DetectionResult.h"

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
const int LED_SETTLE_US = 250;
const int OFF_SETTLE_US = 150;

const float SHADOW_THRESHOLD = 0.15;
const float LOCAL_CONTRAST_THRESHOLD = 0.08;
const float MIN_TOTAL_SHADOW = 0.25;
const int MIN_ACTIVE_PIXELS = 1;
const int MAX_ACTIVE_PIXELS = 8;
const int REQUIRED_CANDIDATE_FRAMES = 2;
const float BASELINE_ALPHA = 0.002;

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
  delayMicroseconds(8);
}

int readAdcAverage(int pin) {
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
  int background[NUM_PIXELS];

  allIrOff();
  delayMicroseconds(OFF_SETTLE_US);

  for (int p = 0; p < NUM_PIXELS; p++) {
    background[p] = readPixelRaw(p);
  }

  for (int l = 0; l < NUM_LEDS; l++) {
    setIrLed(l);
    delayMicroseconds(LED_SETTLE_US);

    for (int p = 0; p < NUM_PIXELS; p++) {
      int onValue = readPixelRaw(p);
      float sig = background[p] - onValue;
      if (sig < 0) sig = 0;
      signalNow[l][p] = sig;
    }

    allIrOff();
  }
}

void calibrateBaseline() {
  Serial.println("Calibrating baseline...");

  for (int l = 0; l < NUM_LEDS; l++) {
    for (int p = 0; p < NUM_PIXELS; p++) {
      baseline[l][p] = 0;
    }
  }

  const int rounds = 80;

  for (int r = 0; r < rounds; r++) {
    scanOpticalFrame();

    for (int l = 0; l < NUM_LEDS; l++) {
      for (int p = 0; p < NUM_PIXELS; p++) {
        baseline[l][p] += signalNow[l][p];
      }
    }

    delay(15);
  }

  for (int l = 0; l < NUM_LEDS; l++) {
    for (int p = 0; p < NUM_PIXELS; p++) {
      baseline[l][p] /= rounds;
      if (baseline[l][p] < 1) baseline[l][p] = 1;
    }
  }

  Serial.println("Baseline ready");
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
      float s = 1.0 - signalNow[l][p] / baseline[l][p];
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

void updateBaselineIfSafe(bool objectDetected) {
  if (objectDetected) return;

  for (int l = 0; l < NUM_LEDS; l++) {
    for (int p = 0; p < NUM_PIXELS; p++) {
      baseline[l][p] =
        baseline[l][p] * (1.0 - BASELINE_ALPHA) +
        signalNow[l][p] * BASELINE_ALPHA;

      if (baseline[l][p] < 1) baseline[l][p] = 1;
    }
  }
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

int candidateFrames = 0;

bool updateMosquitoDecision(DetectionResult r) {
  if (r.candidate) {
    candidateFrames++;
  } else {
    candidateFrames = 0;
  }

  if (candidateFrames >= REQUIRED_CANDIDATE_FRAMES) {
    candidateFrames = 0;
    return true;
  }

  return false;
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
  html += "<h2>Mosquito Trap Logs</h2>";
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
  String json = "{\"logs\":[";
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

  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }

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

  Serial.println("Mosquito trap ready");
}

void loop() {
  server.handleClient();
  updateVacuumSequence();

  scanOpticalFrame();
  makeShadowImage();

  DetectionResult r = analyzeShadow();
  bool detected = updateMosquitoDecision(r);

  if (detected) {
    Serial.println("MOSQUITO DETECTED");
    addLog(r.activeCount, r.totalShadow, r.motion, r.maxLocalContrast, r.cx, r.cy);
    startVacuumStrongSequence();
  }

  updateBaselineIfSafe(r.candidate);
  savePrevShadow();

  delay(5);
}
