#include <Arduino.h>
#include <ESP32Servo.h>

// =====================
// Pin settings
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
// Servo
// =====================
Servo vacuumServo;

const int SERVO_REST_ANGLE  = 20;
const int SERVO_PRESS_ANGLE = 65;

const int ADC_SAMPLES = 4;

// =====================
// MUX / ADC
// =====================
void setMuxChannel(int ch) {
  digitalWrite(MUX_S0_PIN, ch & 0x01);
  digitalWrite(MUX_S1_PIN, ch & 0x02);
  digitalWrite(MUX_S2_PIN, ch & 0x04);
  digitalWrite(MUX_S3_PIN, ch & 0x08);
  delayMicroseconds(10);
}

int readAdcAverage(int pin) {
  long sum = 0;

  for (int i = 0; i < ADC_SAMPLES; i++) {
    sum += analogRead(pin);
    delayMicroseconds(20);
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
// IR LED
// =====================
void allIrOff() {
  digitalWrite(IR_LEFT_PIN, LOW);
  digitalWrite(IR_RIGHT_PIN, LOW);
}

void leftIrOn() {
  allIrOff();
  digitalWrite(IR_LEFT_PIN, HIGH);
}

void rightIrOn() {
  allIrOff();
  digitalWrite(IR_RIGHT_PIN, HIGH);
}

// =====================
// Test 1: LED blink
// =====================
void testLedBlink() {
  Serial.println("TEST 1: IR LED blink");

  leftIrOn();
  Serial.println("LEFT IR ON");
  delay(1000);

  rightIrOn();
  Serial.println("RIGHT IR ON");
  delay(1000);

  allIrOff();
  Serial.println("IR OFF");
  delay(1000);
}

// =====================
// Test 2: raw ADC all pixels
// =====================
void testRawAdc() {
  Serial.println("TEST 2: Raw ADC PT1-PT18");

  allIrOff();

  for (int i = 0; i < 18; i++) {
    int v = readPixelRaw(i);

    Serial.print("PT");
    Serial.print(i + 1);
    Serial.print("=");
    Serial.print(v);
    Serial.print(" ");
  }

  Serial.println();
}

// =====================
// Test 3: left/right signal
// signal = background - LED_ON
// =====================
void testLedDifference() {
  int bg[18];
  int left[18];
  int right[18];

  allIrOff();
  delayMicroseconds(300);

  for (int i = 0; i < 18; i++) {
    bg[i] = readPixelRaw(i);
  }

  leftIrOn();
  delayMicroseconds(300);

  for (int i = 0; i < 18; i++) {
    left[i] = readPixelRaw(i);
  }

  rightIrOn();
  delayMicroseconds(300);

  for (int i = 0; i < 18; i++) {
    right[i] = readPixelRaw(i);
  }

  allIrOff();

  Serial.println("TEST 3: signal = background - LED_ON");

  Serial.print("LEFT : ");
  for (int i = 0; i < 18; i++) {
    int sig = bg[i] - left[i];
    if (sig < 0) sig = 0;

    Serial.print(sig);
    Serial.print(" ");
  }
  Serial.println();

  Serial.print("RIGHT: ");
  for (int i = 0; i < 18; i++) {
    int sig = bg[i] - right[i];
    if (sig < 0) sig = 0;

    Serial.print(sig);
    Serial.print(" ");
  }
  Serial.println();
}

// =====================
// Test 4: simple shadow detection
// =====================
void testShadowDetectionLoop() {
  static bool calibrated = false;
  static int baseLeft[18];
  static int baseRight[18];

  int bg[18];
  int left[18];
  int right[18];

  allIrOff();
  delayMicroseconds(300);
  for (int i = 0; i < 18; i++) bg[i] = readPixelRaw(i);

  leftIrOn();
  delayMicroseconds(300);
  for (int i = 0; i < 18; i++) left[i] = readPixelRaw(i);

  rightIrOn();
  delayMicroseconds(300);
  for (int i = 0; i < 18; i++) right[i] = readPixelRaw(i);

  allIrOff();

  int sigLeft[18];
  int sigRight[18];

  for (int i = 0; i < 18; i++) {
    sigLeft[i] = bg[i] - left[i];
    sigRight[i] = bg[i] - right[i];

    if (sigLeft[i] < 0) sigLeft[i] = 0;
    if (sigRight[i] < 0) sigRight[i] = 0;
  }

  if (!calibrated) {
    Serial.println("Calibrating simple baseline...");
    for (int i = 0; i < 18; i++) {
      baseLeft[i] = sigLeft[i];
      baseRight[i] = sigRight[i];

      if (baseLeft[i] < 1) baseLeft[i] = 1;
      if (baseRight[i] < 1) baseRight[i] = 1;
    }

    calibrated = true;
    delay(1000);
    return;
  }

  bool shadow = false;

  Serial.print("ratio: ");

  for (int i = 0; i < 18; i++) {
    float rL = (float)sigLeft[i] / baseLeft[i];
    float rR = (float)sigRight[i] / baseRight[i];

    float r = min(rL, rR);

    Serial.print(r, 2);
    Serial.print(" ");

    if (r < 0.75) {
      shadow = true;
    }
  }

  Serial.print(" shadow=");
  Serial.println(shadow ? "YES" : "NO");

  delay(100);
}

// =====================
// Test 5: servo single press
// =====================
void testServoSinglePress() {
  Serial.println("TEST 5: servo single press");

  vacuumServo.write(SERVO_PRESS_ANGLE);
  delay(250);

  vacuumServo.write(SERVO_REST_ANGLE);
  delay(500);
}

// =====================
// Test 6: vacuum full sequence
// press-release, press-release, wait 3 sec, press-release
// =====================
void pressSwitchOnce() {
  vacuumServo.write(SERVO_PRESS_ANGLE);
  delay(250);

  vacuumServo.write(SERVO_REST_ANGLE);
  delay(300);
}

void testVacuumSequence() {
  Serial.println("TEST 6: vacuum strong sequence");

  Serial.println("Press 1");
  pressSwitchOnce();

  Serial.println("Press 2 -> strong");
  pressSwitchOnce();

  Serial.println("Strong suction 3 sec");
  delay(3000);

  Serial.println("Press 3 -> stop");
  pressSwitchOnce();

  Serial.println("Done");
}

// =====================
// Setup / loop
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

  vacuumServo.setPeriodHertz(50);
  vacuumServo.attach(SERVO_PIN, 500, 2400);
  vacuumServo.write(SERVO_REST_ANGLE);

  Serial.println("=== Mosquito Trap Hardware Test ===");
  Serial.println("Send command:");
  Serial.println("1 = IR LED blink");
  Serial.println("2 = Raw ADC PT1-PT18");
  Serial.println("3 = Left/Right IR difference");
  Serial.println("4 = Simple shadow detection loop");
  Serial.println("5 = Servo single press");
  Serial.println("6 = Vacuum full sequence");
}

void loop() {
  if (!Serial.available()) return;

  char c = Serial.read();

  if (c == '1') {
    testLedBlink();
  } else if (c == '2') {
    testRawAdc();
  } else if (c == '3') {
    testLedDifference();
  } else if (c == '4') {
    Serial.println("Running shadow detection loop. Reset board to stop/recalibrate.");
    while (true) {
      testShadowDetectionLoop();
    }
  } else if (c == '5') {
    testServoSinglePress();
  } else if (c == '6') {
    testVacuumSequence();
  }
}