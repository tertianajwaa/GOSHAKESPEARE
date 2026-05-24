#include <ld2410.h>

#define RADAR_RX_PIN 32
#define RADAR_TX_PIN 33

ld2410 radar;

uint32_t lastReading = 0;

void setup(void) {
  Serial.begin(115200); 

  Serial2.begin(256000, SERIAL_8N1, RADAR_RX_PIN, RADAR_TX_PIN);
  
  delay(500);
  Serial.print(F("\nMenghubungkan LD2410 (TX ke pin "));
  Serial.print(RADAR_RX_PIN);
  Serial.print(F(", RX ke pin "));
  Serial.print(RADAR_TX_PIN);
  Serial.println(F(")..."));
  
  if(radar.begin(Serial2)) {
    Serial.println(F("Status: TERHUBUNG OK!"));
    Serial.print(F("Versi Firmware: "));
    Serial.print(radar.firmware_major_version);
    Serial.print('.');
    Serial.print(radar.firmware_minor_version);
    Serial.print('.');
    Serial.println(radar.firmware_bugfix_version, HEX);
  } else {
    Serial.println(F("Status: GAGAL TERHUBUNG. Silakan cek silang kabel RX/TX."));
  }
}

void loop() {
  radar.read();

  if(radar.isConnected() && millis() - lastReading > 1000) {
    lastReading = millis();
    
    if(radar.presenceDetected()) {
      if(radar.stationaryTargetDetected()) {
        Serial.print(F("Diam: "));
        Serial.print(radar.stationaryTargetDistance());
        Serial.print(F("cm (Energi: "));
        Serial.print(radar.stationaryTargetEnergy());
        Serial.print(F(") | "));
      }
      if(radar.movingTargetDetected()) {
        Serial.print(F("Gerak: "));
        Serial.print(radar.movingTargetDistance());
        Serial.print(F("cm (Energi: "));
        Serial.print(radar.movingTargetEnergy());
        Serial.print(F(")"));
      }
      Serial.println();
    } else {
      Serial.println(F("Tidak ada target"));
    }
  }
}