#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <BH1750.h>
#include <SensirionI2cScd4x.h>
#include <ld2410.h>
#include <PZEM004Tv30.h>   

//wifi
const char* WIFI_SSID     = "Hanafi";
const char* WIFI_PASSWORD = "02041992";

const int pwmPin    = 18;
const int channel   = 0;
const int frekuensi = 5000;
const int resolusi  = 8;

int targetDuty   = 0;
int currentDuty  = 0;
uint32_t lastFadeTime = 0;
const int fadeDelay   = 15;

const int LUX_GELAP  = 20;
const int LUX_TERANG = 150;

//PIN definisi SDA SCL BH1740 dan SCD41
#define I2C_SDA       21
#define I2C_SCL       22

// LD2410 di serial 2
#define RADAR_RX_PIN  32
#define RADAR_TX_PIN  33

// PZEM-004T di serial 1
#define PZEM_RX_PIN   26
#define PZEM_TX_PIN   27

#ifdef NO_ERROR
  #undef NO_ERROR
#endif
#define NO_ERROR 0

BH1750            lightMeter;
SensirionI2cScd4x scd4x;
ld2410            radar;
PZEM004Tv30       pzem(Serial1, PZEM_RX_PIN, PZEM_TX_PIN);  // ← Serial1!
WebServer         server(80);

struct SensorData {
  // BH1750
  float    lux          = 0.0f;
  bool     bh1750Ok     = false;

  // SCD41
  uint16_t co2          = 0;
  float    temperature  = 0.0f;
  float    humidity     = 0.0f;
  bool     scd4xOk      = false;

  // LD2410
  bool     presence       = false;
  bool     stationary     = false;
  bool     moving         = false;
  uint16_t stationaryDist   = 0;
  uint8_t  stationaryEnergy = 0;
  uint16_t movingDist     = 0;
  uint8_t  movingEnergy   = 0;
  bool     radarOk        = false;

  //PZEM004
  float    voltage    = NAN;
  float    current    = NAN;
  float    power      = NAN;
  float    energy     = NAN;
  float    frequency  = NAN;
  float    pf         = NAN;
  bool     pzemOk     = false;

  unsigned long lastUpdate = 0;
} data;

// Timing millis
uint32_t lastBH1750   = 0;
uint32_t lastSCD4x    = 0;
uint32_t lastRadarLog = 0;
uint32_t lastPZEM     = 0;    

// Error buffer SCD4x
static char  scdErrMsg[64];
static int16_t scdErr;

//Bagian inisialisasi sensor
void initBH1750() {
  if (lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE, 0x23, &Wire)) {
    data.bh1750Ok = true;
    Serial.println("[BH1750] ✓ Terhubung — mode CONTINUOUS_HIGH_RES");
  } else {
    data.bh1750Ok = false;
    Serial.println("[BH1750] ✗ Tidak terdeteksi, cek kabel SDA/SCL!");
  }
}

void initSCD4x() {
  scd4x.begin(Wire, SCD41_I2C_ADDR_62);
  delay(30);
  scd4x.wakeUp();
  scd4x.stopPeriodicMeasurement();
  delay(500);
  scd4x.reinit();
  delay(30);

  uint64_t serial = 0;
  scdErr = scd4x.getSerialNumber(serial);
  if (scdErr != NO_ERROR) {
    Serial.println("[SCD4x] ✗ Gagal membaca serial number!");
    data.scd4xOk = false;
    return;
  }
  scdErr = scd4x.startPeriodicMeasurement();
  if (scdErr != NO_ERROR) {
    Serial.println("[SCD4x] ✗ Gagal memulai pengukuran periodik!");
    data.scd4xOk = false;
    return;
  }
  data.scd4xOk = true;
  Serial.printf("[SCD4x] ✓ Terhubung — Serial: 0x%08X%08X\n",
    (uint32_t)(serial >> 32), (uint32_t)(serial & 0xFFFFFFFF));
}

void initLD2410() {
  Serial2.begin(256000, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  delay(500);
  if (radar.begin(Serial2)) {
    data.radarOk = true;
    Serial.printf("[LD2410] ✓ Terhubung — Firmware v%d.%d.%X\n",
      radar.firmware_major_version,
      radar.firmware_minor_version,
      radar.firmware_bugfix_version);
  } else {
    data.radarOk = false;
    Serial.println("[LD2410] ✗ Gagal! Cek silang kabel TX/RX.");
  }
}

void initPZEM() {
  float v = pzem.voltage();
  if (!isnan(v)) {
    data.pzemOk = true;
    Serial.printf("[PZEM]  Terhubung Tegangan awal: %.1f V\n", v);
  } else {
    data.pzemOk = false;
    Serial.println("[PZEM]  Belum ada respons (cek wiring / pastikan ada tegangan AC)");
  }
}

//http handler
void handleRoot() {
  server.send(200, "text/plain", "SHAKESPEARE ESP32 Sensor Hub\n/data → JSON semua sensor\n/status → ringkasan");
}

// Helper dengan format float NAN → "null" agar JSON valid
String fmtFloat(float v, int dec = 2) {
  if (isnan(v)) return "null";
  return String(v, dec);
}

void handleData() {
  String presenceStr = "Tidak Ada Target";
  if (data.presence) {
    if (data.stationary && data.moving) presenceStr = "Diam & Bergerak";
    else if (data.stationary)           presenceStr = "Diam";
    else if (data.moving)               presenceStr = "Bergerak";
    else                                presenceStr = "Terdeteksi";
  }

  String jT = "true", jF = "false";
  String json = "{";

  // BH1750
  json += "\"lux\":"         + String(data.lux, 1)           + ",";
  json += "\"bh1750Ok\":"    + (data.bh1750Ok ? jT : jF)     + ",";

  // SCD41
  json += "\"co2\":"         + String(data.co2)               + ",";
  json += "\"temperature\":" + String(data.temperature, 1)    + ",";
  json += "\"humidity\":"    + String(data.humidity, 1)        + ",";
  json += "\"scd4xOk\":"     + (data.scd4xOk ? jT : jF)      + ",";

  // LD2410
  json += "\"presence\":"       + (data.presence   ? jT : jF) + ",";
  json += "\"presenceLabel\":\"" + presenceStr + "\",";
  json += "\"stationary\":"     + (data.stationary ? jT : jF) + ",";
  json += "\"moving\":"         + (data.moving     ? jT : jF) + ",";
  json += "\"stationaryDist\":" + String(data.stationaryDist)   + ",";
  json += "\"stationaryEnergy\":" + String(data.stationaryEnergy) + ",";
  json += "\"movingDist\":"     + String(data.movingDist)       + ",";
  json += "\"movingEnergy\":"   + String(data.movingEnergy)     + ",";
  json += "\"radarOk\":"        + (data.radarOk ? jT : jF)    + ",";

  // PZEM 004
  json += "\"voltage\":"    + fmtFloat(data.voltage, 1)   + ",";
  json += "\"current\":"    + fmtFloat(data.current, 3)   + ",";
  json += "\"power\":"      + fmtFloat(data.power, 1)     + ",";
  json += "\"energy\":"     + fmtFloat(data.energy, 3)    + ",";
  json += "\"frequency\":"  + fmtFloat(data.frequency, 1) + ",";
  json += "\"pf\":"         + fmtFloat(data.pf, 2)        + ",";
  json += "\"pzemOk\":"     + (data.pzemOk ? jT : jF)    + ",";

  // Lampu & Meta
  json += "\"lampDuty\":"   + String(currentDuty)               + ",";
  json += "\"uptime\":"     + String(millis() / 1000)            + ",";
  json += "\"ip\":\""       + WiFi.localIP().toString()          + "\"";
  json += "}";

  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Cache-Control", "no-cache");
  server.send(200, "application/json", json);
}

void handleStatus() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  String out = "SHAKESPEARE IoT Hub — ESP32\n";
  out += "IP     : " + WiFi.localIP().toString() + "\n";
  out += "Uptime : " + String(millis() / 1000) + " detik\n";
  out += "PZEM   : " + String(data.pzemOk ? "OK" : "Tidak Terhubung") + "\n";
  server.send(200, "text/plain", out);
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Wire.begin(I2C_SDA, I2C_SCL);
  delay(100);

  initBH1750();
  initSCD4x();
  initLD2410();
  initPZEM(); 

  // PWM Lampu
  ledcSetup(channel, frekuensi, resolusi);
  ledcAttachPin(pwmPin, channel);
  ledcWrite(channel, 0);

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(400);
  }
  Serial.println("[WiFi]  Terhubung — IP: " + WiFi.localIP().toString());

  server.on("/",       handleRoot);
  server.on("/data",   handleData);
  server.on("/status", handleStatus);
  server.begin();
}

void loop() {
  server.handleClient();

  // LD2410
  if (data.radarOk) {
    radar.read();
    if (radar.isConnected() && (millis() - lastRadarLog > 1000)) {
      lastRadarLog = millis();
      data.presence   = radar.presenceDetected();
      data.stationary = radar.stationaryTargetDetected();
      data.moving     = radar.movingTargetDetected();

      data.stationaryDist   = data.stationary ? radar.stationaryTargetDistance() : 0;
      data.stationaryEnergy = data.stationary ? radar.stationaryTargetEnergy()   : 0;
      data.movingDist       = data.moving     ? radar.movingTargetDistance()      : 0;
      data.movingEnergy     = data.moving     ? radar.movingTargetEnergy()        : 0;
    }
  }

  // BH1750
  if (data.bh1750Ok && (millis() - lastBH1750 > 1000)) {
    lastBH1750 = millis();
    float lv = lightMeter.readLightLevel();
    if (lv >= 0) {
      data.lux = lv;
      if      (data.lux <= LUX_GELAP)  targetDuty = 255;
      else if (data.lux >= LUX_TERANG) targetDuty = 0;
      else    targetDuty = map((int)data.lux, LUX_GELAP, LUX_TERANG, 255, 0);
    }
  }

  // SCD41
  if (data.scd4xOk && (millis() - lastSCD4x > 5000)) {
    lastSCD4x = millis();
    bool ready = false;
    scdErr = scd4x.getDataReadyStatus(ready);
    if (scdErr == NO_ERROR && ready) {
      uint16_t co2Raw = 0; float tempRaw = 0, humRaw = 0;
      scdErr = scd4x.readMeasurement(co2Raw, tempRaw, humRaw);
      if (scdErr == NO_ERROR) {
        data.co2         = co2Raw;
        data.temperature = tempRaw;
        data.humidity    = humRaw;
        data.lastUpdate  = millis();
      }
    }
  }

  // PZEM004T
  if (millis() - lastPZEM > 2000) {
    lastPZEM = millis();

    float v = pzem.voltage();
    if (!isnan(v)) {
      data.voltage   = v;
      data.current   = pzem.current();
      data.power     = pzem.power();
      data.energy    = pzem.energy();
      data.frequency = pzem.frequency();
      data.pf        = pzem.pf();
      data.pzemOk    = true;
    } else {
      data.pzemOk = false;
      Serial.println("[PZEM] Gagal baca — sensor terputus atau tidak ada tegangan AC");
    }
  }

  // fader lampu smooth
  if (millis() - lastFadeTime > fadeDelay) {
    lastFadeTime = millis();
    if      (currentDuty < targetDuty) { currentDuty++; ledcWrite(channel, currentDuty); }
    else if (currentDuty > targetDuty) { currentDuty--; ledcWrite(channel, currentDuty); }
  }
}