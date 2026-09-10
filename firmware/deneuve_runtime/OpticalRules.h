#pragma once
#include <cmath>
#include <cstdint>

// Provisional thresholds, not measured mosquito classification accuracy.
struct OpticalCalibration {
  int n = 0;
  float mean = 0, m2 = 0;
  bool clipped = false;
  void add(int off, int on) {
    clipped |= off <= 20 || off >= 4075 || on <= 20 || on >= 4075;
    float value = off - on;
    float delta = value - mean;
    mean += delta / ++n;
    m2 += delta * (value - mean);
  }
  float sd() const { return n > 1 ? std::sqrt(m2 / (n - 1)) : 0; }
  bool valid() const { return n >= 2 && !clipped && mean >= 40 && sd() <= mean * 0.1f; }
};

// Coordinates in sensor-pitch units, NOT mm; PT1..6 / PT7..12 / PT13..18.
inline float sensorX(int i) { return i % 6 + (i / 6 == 1 ? 0.5f : 0); }
inline float sensorY(int i) { return i / 6; }
inline float sensorDistance2(int i, int j) {
  float x = sensorX(i)-sensorX(j), y = sensorY(i)-sensorY(j);
  return x*x+y*y;
}
struct OpticalTrack {
  bool tracking = false, blocked = false;
  uint32_t start = 0, last = 0;
  int frames = 0;
  float x = 0, y = 0, originX = 0, originY = 0, displacement = 0;
  void reset() { tracking = blocked = false; frames = 0; displacement = 0; }
  bool update(const float* shadow, const bool* valid, uint32_t now, bool healthy) {
    if (!healthy) { reset(); return false; }
    bool active[18] = {}, visited[18] = {};
    int count=0, seed=-1;
    float weight=0, wx=0, wy=0, contrast=0;
    for(int i=0;i<18;i++) if(valid[i]) {
      float sum=0; int neighbors=0;
      for(int j=0;j<18;j++) if(i!=j && valid[j] && sensorDistance2(i,j)<=1.26f) {
        sum+=shadow[j]; neighbors++;
      }
      float local=neighbors ? shadow[i]-sum/neighbors : 0;
      if (local>contrast) contrast=local;
      if(shadow[i]>0.15f) {
        active[i]=true; count++; seed=i;
        weight+=shadow[i]; wx+=sensorX(i)*shadow[i]; wy+=sensorY(i)*shadow[i];
      }
    }
    // Only a genuinely clear frame ends a track. Broad/static shadows do not.
    if(count==0) {
      bool result = tracking && !blocked && frames>=3 && now-start>=3 &&
                    now-start<=300 && now-last<=50 && displacement>=0.25f;
      reset(); return result;
    }
    int queue[18], head=0, tail=0;
    queue[tail++]=seed; visited[seed]=true;
    while(head<tail) {
      int i=queue[head++];
      for(int j=0;j<18;j++) if(active[j]&&!visited[j]&&sensorDistance2(i,j)<=1.26f) {
        visited[j]=true; queue[tail++]=j;
      }
    }
    bool candidate=count<=4 && tail==count && weight>=0.25f && contrast>=0.08f;
    if(!candidate) { tracking=false; blocked=true; return false; }
    if(blocked) return false; // must clear before another candidate
    float cx=wx/weight, cy=wy/weight;
    if(!tracking) {
      tracking=true; start=last=now; frames=1;
      x=originX=cx; y=originY=cy; displacement=0; return false;
    }
    float dx=cx-x, dy=cy-y;
    if(now-last>50 || now-start>300 || dx*dx+dy*dy>2.25f) {
      tracking=false; blocked=true; return false;
    }
    dx=cx-originX; dy=cy-originY;
    float distance=std::sqrt(dx*dx+dy*dy);
    if(distance>displacement) displacement=distance;
    x=cx; y=cy; last=now; frames++;
    return false;
  }
};
