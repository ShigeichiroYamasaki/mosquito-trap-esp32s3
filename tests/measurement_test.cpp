#include "../firmware/optical_measure/Measurement.h"
#include <cassert>
int main() {
  assert(muxIndex(0)==0 && muxChannel(0)==0);
  assert(muxIndex(15)==0 && muxChannel(15)==15);
  assert(muxIndex(16)==1 && muxChannel(16)==0);
  assert(muxIndex(17)==1 && muxChannel(17)==1);
  uint16_t a[]={0,4095,100,101};auto s=summarize(a,4);
  assert(s.mean==1074 && s.low==0 && s.high==4095);
  uint16_t b[]={4095};s=summarize(b,1);assert(s.mean==4095&&s.low==s.high);
  Frame f={};f.values[0][0].mean=100;f.values[1][0].mean=200;
  f.values[2][0].mean=300;f.values[3][0].mean=50;
  assert(difference(f,0,0)==-100);assert(difference(f,1,0)==250);
}
