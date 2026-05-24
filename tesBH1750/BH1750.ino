#include <Wire.h>
#include <BH1750.h>

BH1750 lightMeter;

void setup() {
  Serial.begin(115200);

  // Inisialisasi pin I2C (SDA, SCL)
  Wire.begin();

  Serial.println("Memulai inisialisasi sensor BH1750...");

  // Memulai sensor BH1750
  if (lightMeter.begin()) {
    Serial.println("Sensor BH1750 berhasil terhubung dan siap digunakan!");
  } else {
    Serial.println("Error! Sensor BH1750 tidak terdeteksi. Cek kembali kabel SDA/SCL.");
  }
}

void loop() {
  // Membaca intensitas cahaya
  float lux = lightMeter.readLightLevel();
  
  // Menampilkan hasil ke Serial Monitor
  Serial.print("Intensitas Cahaya: ");
  Serial.print(lux);
  Serial.println(" lx");

  // Jeda 1 detik sebelum membaca data lagi
  delay(1000);
}
