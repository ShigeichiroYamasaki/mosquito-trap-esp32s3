#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Measurement.h"
#include "WebUi.h"

// Measurement only: GPIO16 (servo) is never configured or driven.
const int SIG_PINS[2] = {1, 2};
const int SELECT_PINS[4] = {6, 7, 8, 9};
const int LED_PINS[2] = {10, 11};
WebServer server(80);
Frame frames[MAX_FRAMES];
int frameCount = 0, targetFrames = 30, samples = 8;
uint32_t ledSettleUs = 2000, muxSettleUs = 50, intervalMs = 250;
uint32_t sessionId = 0, sessionStartMs = 0, lastStartMs = 0;
int distanceMm = 100, resistorKohm = 100;
String condition = "open";
bool running = false;
String serialCommand;

void ledsOff() { digitalWrite(LED_PINS[0], LOW); digitalWrite(LED_PINS[1], LOW); }

void scanFrame() {
  Frame& f = frames[frameCount];
  f.startMs = millis();
  const uint32_t startUs = micros();
  for(int phase=0; phase<4; ++phase) {
    ledsOff();
    if(phase==1) digitalWrite(LED_PINS[0], HIGH);
    if(phase==3) digitalWrite(LED_PINS[1], HIGH);
    delayMicroseconds(ledSettleUs);
    f.phaseUs[phase] = uint32_t(micros()-startUs);
    for(int p=0; p<PIXELS; ++p) {
      int channel = muxChannel(p);
      for(int bit=0; bit<4; ++bit) digitalWrite(SELECT_PINS[bit], (channel>>bit)&1);
      delayMicroseconds(muxSettleUs);
      analogRead(SIG_PINS[muxIndex(p)]); // discard first ADC conversion after switching
      uint16_t raw[32];
      for(int n=0; n<samples; ++n) raw[n] = analogRead(SIG_PINS[muxIndex(p)]);
      f.values[phase][p] = summarize(raw, samples);
    }
  }
  ledsOff();
  f.durationUs = uint32_t(micros()-startUs);
  ++frameCount;
  if(frameCount>=targetFrames) running=false;
}

void beginSession() {
  ledsOff(); ++sessionId; frameCount=0; running=true;
  sessionStartMs=millis(); lastStartMs=sessionStartMs-intervalMs;
  Serial.printf("# session=%lu condition=%s frames=%d led_us=%lu mux_us=%lu samples=%d\n",
    (unsigned long)sessionId, condition.c_str(), targetFrames,
    (unsigned long)ledSettleUs, (unsigned long)muxSettleUs, samples);
}

String csvHeader() {
  return "session,condition,distance_mm,resistor_kohm,led_settle_us,mux_settle_us,samples,interval_ms,frame,start_ms,duration_us,dark_l_scan_us,left_scan_us,dark_r_scan_us,right_scan_us,pt,mux,channel,dark_l_mean,dark_l_min,dark_l_max,left_mean,left_min,left_max,dark_r_mean,dark_r_min,dark_r_max,right_mean,right_min,right_max,left_diff,right_diff\n";
}
String csvRow(int i, int p) {
  const Frame& f=frames[i];
  String s=String(sessionId)+","+condition+","+String(distanceMm)+","+String(resistorKohm)+",";
  s+=String(ledSettleUs)+","+String(muxSettleUs)+","+String(samples)+","+String(intervalMs)+",";
  s+=String(i+1)+","+String(f.startMs)+","+String(f.durationUs)+",";
  for(int k=0;k<4;++k) s+=String(f.phaseUs[k])+",";
  s+=String(p+1)+","+String(muxIndex(p)+1)+","+String(muxChannel(p))+",";
  for(int k=0;k<4;++k) {
    const AdcStats& v=f.values[k][p];
    s+=String(v.mean)+","+String(v.low)+","+String(v.high)+",";
  }
  s+=String(difference(f,0,p))+","+String(difference(f,1,p))+"\n";
  return s;
}

void handleData() {
  String s; s.reserve(8500);
  s="{\"running\":"+String(running?"true":"false")+",\"session\":"+String(sessionId);
  s+=",\"count\":"+String(frameCount)+",\"target\":"+String(targetFrames)+",\"condition\":\""+condition+"\"";
  s+=",\"ledUs\":"+String(ledSettleUs)+",\"muxUs\":"+String(muxSettleUs)+",\"samples\":"+String(samples);
  s+=",\"intervalMs\":"+String(intervalMs)+",\"distanceMm\":"+String(distanceMm)+",\"resistorKohm\":"+String(resistorKohm);
  s+=",\"pixels\":[";
  if(frameCount) {
    const Frame& f=frames[frameCount-1];
    for(int p=0;p<PIXELS;++p) {
      if(p) s+=",";
      s+="{\"pt\":"+String(p+1)+",\"raw\":[";
      for(int k=0;k<4;++k) { if(k) s+=","; s+=String(f.values[k][p].mean); }
      s+="],\"noise\":[";
      for(int k=0;k<4;++k) { if(k) s+=","; s+=String(f.values[k][p].high-f.values[k][p].low); }
      long sumL=0,sumR=0;
      for(int i=0;i<frameCount;++i) { sumL+=difference(frames[i],0,p);sumR+=difference(frames[i],1,p); }
      s+="],\"left\":"+String(difference(f,0,p))+",\"right\":"+String(difference(f,1,p));
      s+=",\"avgLeft\":"+String(float(sumL)/frameCount,1)+",\"avgRight\":"+String(float(sumR)/frameCount,1)+"}";
    }
  }
  s+="]}";
  server.sendHeader("Cache-Control","no-store");server.send(200,"application/json",s);
}

bool validNumber(const String& name, long low, long high, long& value) {
  if(!server.hasArg(name)) return false;
  String text=server.arg(name);if(!text.length() || text.length()>6) return false;
  for(unsigned int i=0;i<text.length();++i) if(text[i]<'0'||text[i]>'9') return false;
  value=text.toInt();return value>=low && value<=high;
}
void handleStart() {
  if(running) {server.send(409,"text/plain","Stop the current session first.");return;}
  long n,l,m,a,t,d,r;
  String tag=server.arg("condition");
  if(!validNumber("frames",1,MAX_FRAMES,n)||!validNumber("ledUs",50,20000,l)||
     !validNumber("muxUs",5,1000,m)||!validNumber("samples",1,32,a)||
     !validNumber("intervalMs",100,5000,t)||!validNumber("distanceMm",1,2000,d)||
     !validNumber("resistorKohm",1,1000,r)||
     !(tag=="open"||tag=="shadow"||tag=="ambient")) {
    server.send(400,"text/plain","Invalid measurement settings.");return;
  }
  targetFrames=n;ledSettleUs=l;muxSettleUs=m;samples=a;intervalMs=t;distanceMm=d;resistorKohm=r;condition=tag;
  beginSession();server.send(200,"text/plain","Started");
}
void handleCsv() {
  if(running) {server.send(409,"text/plain","Stop measurement before downloading.");return;}
  if(!frameCount) {server.send(409,"text/plain","No measurements yet.");return;}
  server.sendHeader("Content-Disposition","attachment; filename=deneuve2-"+String(sessionId)+"-"+condition+".csv");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);server.send(200,"text/csv; charset=utf-8","");
  server.sendContent(csvHeader());
  for(int i=0;i<frameCount;++i) {
    String chunk; chunk.reserve(5000);
    for(int p=0;p<PIXELS;++p) chunk+=csvRow(i,p);
    server.sendContent(chunk);delay(0);
  }
  server.sendContent("");
}
void serialInput() {
  while(Serial.available()) {
    char c=Serial.read();
    if(c=='\r'||c=='\n') {
      serialCommand.trim();
      if(serialCommand=="stop") {running=false;ledsOff();Serial.println("# stopped");}
      else if(serialCommand=="open"||serialCommand=="shadow"||serialCommand=="ambient") {
        if(!running) {condition=serialCommand;beginSession();} else Serial.println("# stop first");
      } else if(serialCommand=="csv") {
        if(running) Serial.println("# stop first");
        else {Serial.print(csvHeader());for(int i=0;i<frameCount;++i) for(int p=0;p<PIXELS;++p) Serial.print(csvRow(i,p));}
      } else if(serialCommand.length()) Serial.println("# commands: open / shadow / ambient / stop / csv (newline required)");
      serialCommand="";
    } else if(serialCommand.length()<24) serialCommand+=c;
  }
}
void setup() {
  Serial.begin(115200);
  for(int pin:SELECT_PINS) {pinMode(pin,OUTPUT);digitalWrite(pin,LOW);}
  for(int pin:LED_PINS) pinMode(pin,OUTPUT);
  ledsOff();analogReadResolution(12);
  analogSetPinAttenuation(SIG_PINS[0],ADC_11db);analogSetPinAttenuation(SIG_PINS[1],ADC_11db);
  WiFi.mode(WIFI_AP);
  bool ok=WiFi.softAP("Deneuve2-Measure","deneuve2measure");
  server.on("/",HTTP_GET,[](){server.send_P(200,"text/html; charset=utf-8",WEB_UI);});
  server.on("/api",HTTP_GET,handleData);
  server.on("/start",HTTP_POST,handleStart);
  server.on("/stop",HTTP_POST,[](){running=false;ledsOff();server.send(200,"text/plain","Stopped");});
  server.on("/data.csv",HTTP_GET,handleCsv);
  server.begin();
  Serial.println("# Deneuve 2 Optical Measurement v1 (servo unused)");
  Serial.printf("# AP %s: Deneuve2-Measure / deneuve2measure\n",ok?"ready":"failed");
  Serial.print("# Open http://");Serial.println(WiFi.softAPIP());
  Serial.println("# open / shadow / ambient / stop / csv (send newline)");
}
void loop() {
  server.handleClient();serialInput();
  if(running && uint32_t(millis()-lastStartMs)>=intervalMs) {
    lastStartMs=millis();scanFrame();
    Serial.printf("# session=%lu frame=%d/%d scan_us=%lu%s\n",(unsigned long)sessionId,frameCount,targetFrames,
      (unsigned long)frames[frameCount-1].durationUs,running?"":" complete");
  }
  delay(1);
}
