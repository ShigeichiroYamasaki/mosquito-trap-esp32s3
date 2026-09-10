#pragma once

struct DetectionResult {
  bool candidate;
  int activeCount;
  float totalShadow;
  float cx;
  float cy;
  float motion;
  float maxLocalContrast;
};
