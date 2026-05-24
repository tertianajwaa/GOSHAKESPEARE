// Tentukan pin GPIO yang digunakan
const int pwmPin = 18; 

// Konfigurasi PWM
const int channel = 0;      
const int frekuensi = 5000; 
const int resolusi = 8;    

void setup() {
  // 1. Mengatur konfigurasi PWM (channel, frekuensi, resolusi)
  ledcSetup(channel, frekuensi, resolusi);
  
  // 2. Menyambungkan channel PWM ke pin GPIO fisik
  ledcAttachPin(pwmPin, channel);
}

void loop() {
  // Efek Lampu Dari Redup ke Terang
  for (int dutyCycle = 0; dutyCycle <= 255; dutyCycle++) {
    ledcWrite(channel, dutyCycle);
    delay(10); 
  }

  // Efek Lampu Dari Terang ke Redup
  for (int dutyCycle = 255; dutyCycle >= 0; dutyCycle--) {
    ledcWrite(channel, dutyCycle);
    delay(10); 
  }
  
  delay(1000); 
}