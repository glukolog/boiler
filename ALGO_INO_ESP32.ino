//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial

#include "string.h"
#include "BluetoothSerial.h"
#include <OneWire.h>

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

#define F_FLOW     2
#define F_HEAT     4
#define F_DS       15

#define STATE_IDLE      0
#define STATE_WORK      1
#define STATE_STOP      2
#define STATE_END       3

BluetoothSerial SerialBT;
OneWire  ds(F_DS);

char s[40];
uint32_t period = 10;
uint32_t t, now = 0, last = 0;
float T, Tmax = 90, Tmin = 70;
//uint32_t x=0;
//int adc, p_adc=10000;
//int sw2, sw4, p_sw2 = 10, p_sw4 = 10;
int flow = 0, heat = 0;
int mm, ss;
int state = STATE_IDLE;
char cmd_buf[64];
int cmd_idx = 0;

float getTemp();
void SendTemp();
void SetMaxTemp();
void SetMinTemp();
void SetPeriod();
void LetStart();
void LetStop();
void LetReset();

void setup() {
  int i;
  pinMode(F_FLOW, OUTPUT);
  pinMode(F_HEAT, OUTPUT);
  digitalWrite(F_FLOW, flow);
  digitalWrite(F_HEAT, heat);
  
  Serial.begin(115200);
  SerialBT.begin("Boiler000"); //Bluetooth device name
  Serial.println("The device started, now you can pair it with bluetooth!");
  Serial.println(F("Autoclav Project v0.2"));

  if (ds.reset()) {
    ds.write(0xCC); // Skip reading
    ds.write(0x4E); // Write Scratchpad
    ds.write(0x4B); // Th
    ds.write(0x46); // Tl
    ds.write(0x3F); // 7-12bit, 5-11bit, 3-10bit, 1-9bit
    delay(300);
    T = getTemp();
  }
}

void loop() {
  now = millis();
  if (Serial.available()) {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available()) {
    ProcCmd();
    //Serial.write(SerialBT.read());
  }
  delay(20);
  if (now - last > 250) {
    T = getTemp();
    //Serial.print(F("TEMP: ")); Serial.println(T);
    if (state == STATE_IDLE) {
      heat = 0; digitalWrite(F_HEAT, heat ^ 1);
      flow = 0; digitalWrite(F_FLOW, flow ^ 1);
    }
    if (state == STATE_WORK) {
      if (T < Tmin) { heat = 1; digitalWrite(F_HEAT, heat ^ 1); }
      if (T > Tmax) { heat = 0; digitalWrite(F_HEAT, heat ^ 1); }
      if (t <= now) state = STATE_STOP;
    }
    if (state == STATE_STOP) {
      heat = 0; digitalWrite(F_HEAT, heat ^ 1);
      flow = 1; digitalWrite(F_FLOW, flow ^ 1);
      state = STATE_END;
      //mqtt->publish("cr0000/cmd", "stop", 1);
    }
    last = now;
  }
}

void ProcCmd() {
  char c = SerialBT.read();
  switch(c) {
    case '*' : cmd_idx = 0; break;
    case '#' : 
      if (cmd_buf[0] != '*') break;
      if (strncmp(cmd_buf+1, "getT", 4) == 0) SendTemp();
      if (strncmp(cmd_buf+1, "getS", 4) == 0) SendSets();
      if (strncmp(cmd_buf+1, "getV", 4) == 0) SendValues();
      if (strncmp(cmd_buf+1, "smaT", 4) == 0) SetMaxTemp();
      if (strncmp(cmd_buf+1, "smiT", 4) == 0) SetMinTemp();
      if (strncmp(cmd_buf+1, "setP", 4) == 0) SetPeriod();
      if (strncmp(cmd_buf+1, "strt", 4) == 0) LetStart();
      if (strncmp(cmd_buf+1, "stop", 4) == 0) LetStop();
      if (strncmp(cmd_buf+1, "rest", 4) == 0) LetReset();
      cmd_idx = 1;
      break;
  }
  cmd_buf[cmd_idx++] = c;
  if (cmd_idx > 60) cmd_idx = 0;
}

void SendTemp() {
  String str;
  str = "*Temp:" + String(T) + '#';
  SerialBT.println(str);
}

void SendSets() {
  sprintf(s, "*Sets:%.1f,%.1f,%d#", Tmax, Tmin, period);
//  Serial.println(s);
  SerialBT.println(s);
}

void SendValues() {
  uint32_t ti;
  switch (state) {
    case STATE_IDLE : ti = period * 60 * 1000; break;
    case STATE_WORK : ti = t - millis(); break;
    case STATE_STOP : ti = 0; break;
    case STATE_END : ti = 0; break;
  }
  
  sprintf(s, "*Vals:%.1f,%d,%d,%d,%d#", T, ti, heat, flow, state);
//  Serial.println(s);
  SerialBT.println(s);
}

void SetMaxTemp() {
  strncpy(s, cmd_buf + 6, cmd_idx - 6);
  Serial.print("SetMaxTemp: "); Serial.println(s);
  Tmax = atof(s);
  Serial.print("Tmax="); Serial.println(Tmax);
}

void SetMinTemp() {
  strncpy(s, cmd_buf + 6, cmd_idx - 6);
  Serial.print("SetMinTemp: "); Serial.println(s);
  Tmin = atof(s);
  Serial.print("Tmin="); Serial.println(Tmin);
}

void SetPeriod() {
  strncpy(s, cmd_buf + 6, cmd_idx - 6);
  Serial.print("SetPeriod: "); Serial.println(s);
  period = atoi(s);
  Serial.print("t="); Serial.println(period);
}

float getTemp() {
  byte data[2];
  uint16_t raw;
//  byte present;
  int i;

  if (ds.reset()) {
    ds.write(0xCC);
    ds.write(0xBE);
    data[0] = ds.read();
    data[1] = ds.read();
    raw = (data[1] << 8) | data[0];
    ds.reset();
    ds.write(0xCC);
    ds.write(0x44); // request temp calc
    return((float)raw / 16.0);
  } else return(0.0);
}

void LetStart() {
  Serial.println("Start!");
  if (state == STATE_IDLE) {
    state = STATE_WORK;
    t = millis() + period * 60 * 1000;
  }
}

void LetStop() {
  Serial.println("Stop!");
  state = STATE_STOP;
}

void LetReset() {
  Serial.println("Reset");
  if (state == STATE_END) state = STATE_IDLE;
}
/** Load WLAN credentials from EEPROM */
/*void loadCredentials() {
  EEPROM.begin(512);
  EEPROM.get(0, ssid);
  EEPROM.get(0+sizeof(ssid), password);
  char ok[2+1];
  EEPROM.get(0+sizeof(ssid)+sizeof(password), ok);
  EEPROM.end();
  if (String(ok) != String("OK")) {
    ssid[0] = 0;
    password[0] = 0;
  }
  Serial.println("Recovered credentials:");
  Serial.println(ssid);
  Serial.println(strlen(password)>0?"********":"<no password>");
}
*/
/** Store WLAN credentials to EEPROM */
/*
void saveCredentials() {
  EEPROM.begin(512);
  EEPROM.put(0, ssid);
  EEPROM.put(0+sizeof(ssid), password);
  char ok[2+1] = "OK";
  EEPROM.put(0+sizeof(ssid)+sizeof(password), ok);
  EEPROM.commit();
  EEPROM.end();
}
*/
