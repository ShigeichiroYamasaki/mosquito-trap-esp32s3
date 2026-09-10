#pragma once
#include <stdint.h>

constexpr int PIXELS = 18;
constexpr int MAX_FRAMES = 120;
struct AdcStats { uint16_t mean, low, high; };
struct Frame {
  uint32_t startMs, durationUs;
  uint32_t phaseUs[4]; // elapsed us at each phase's scan start
  AdcStats values[4][PIXELS]; // dark-left, left, dark-right, right
};
inline int muxIndex(int pixel) { return pixel < 16 ? 0 : 1; }
inline int muxChannel(int pixel) { return pixel < 16 ? pixel : pixel - 16; }
inline int difference(const Frame& frame, int led, int pixel) {
  return int(frame.values[led * 2][pixel].mean) - int(frame.values[led * 2 + 1][pixel].mean);
}
inline AdcStats summarize(const uint16_t* values, int count) {
  uint32_t sum = 0; uint16_t low = 4095, high = 0;
  for (int i=0; i<count; ++i) {
    sum += values[i]; if(values[i]<low) low=values[i]; if(values[i]>high) high=values[i];
  }
  return {uint16_t(sum / count), low, high};
}
