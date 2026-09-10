#include "../firmware/deneuve_runtime/OpticalRules.h"
#include <cassert>
#include <cstdio>
int main() {
  OpticalCalibration good, weak, rail, noisy;
  for(int i=0;i<80;i++) { good.add(3000,2000); weak.add(3000,2990); rail.add(4095,2000); noisy.add(3000,i%2?1000:2900); }
  assert(good.valid() && !weak.valid() && !rail.valid() && !noisy.valid());
  bool valid[18]; for(auto &v:valid) v=true;
  float s[18]={}; OpticalTrack t;
  auto frame=[&](int pixel, unsigned ms, bool healthy=true) {
    for(auto &v:s) v=0; if(pixel>=0) s[pixel]=0.6f;
    return t.update(s,valid,ms,healthy);
  };
  assert(!frame(0,10)); assert(!frame(1,20)); assert(!frame(2,30)); assert(frame(-1,40));
  assert(!frame(-1,50)); // no duplicate
  frame(0,100); frame(0,110); frame(0,120); assert(!frame(-1,130)); // flicker at same point
  frame(0,200); frame(5,210); frame(4,220); assert(!frame(-1,230)); // disconnected jump
  frame(0,300); frame(1,310); frame(2,620); assert(!frame(-1,630)); // gap / dwell
  frame(0,700); frame(1,710); frame(2,720,false); assert(!frame(-1,730));
  for(auto &v:s) v=0.8f; assert(!t.update(s,valid,800,true)); assert(!frame(-1,810));
  frame(0,0xfffffff0u); frame(1,0xfffffffau); frame(2,4); assert(frame(-1,14)); // millis wrap
  std::puts("Optical calibration and tracking tests passed");
}
