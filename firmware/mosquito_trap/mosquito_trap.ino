// Initial bring-up: no GPIO outputs or trap hardware are enabled.
#include <Arduino.h>

constexpr uint32_t kHeartbeatMs = 1000;
uint32_t lastHeartbeatMs = 0;

void setup() {
  Serial.begin(115200);
  // A disconnected serial monitor must not block startup indefinitely.
  const uint32_t startMs = millis();
  while (!Serial && static_cast<uint32_t>(millis() - startMs) < 2000) {
    delay(10);
  }
  Serial.println("mosquito-trap-esp32s3: serial bring-up ready");
}

void loop() {
  const uint32_t now = millis();
  if (static_cast<uint32_t>(now - lastHeartbeatMs) >= kHeartbeatMs) {
    lastHeartbeatMs = now;
    Serial.printf("uptime_ms=%lu state=BRINGUP outputs=DISABLED\n",
                  static_cast<unsigned long>(now));
  }
  delay(1);
}
