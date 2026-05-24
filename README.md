# SHAKESPEARE
Proyek SHAKESPEARE ini merupakan sistem manajemen energi ruang kelas berbasis IoT yang dikembangkan sebagai bagian dari inisiatif green campus. Sistem ini dirancang untuk mengurangi pemborosan energi listrik yang selama ini terjadi akibat peralatan seperti AC dan lampu yang dibiarkan menyala meski ruangan sudah kosong. Perangkat keras utama yang digunakan adalah mikrokontroler ESP32 yang terhubung ke lima sensor berbeda secara bersamaan. Seluruh data dari sensor dikirim melalui jaringan WiFi ke server kampus dan dapat diakses lewat endpoint HTTP dalam format JSON.

Sensor dan Fungsinya
1. BH1750 Sensor Intensitas Cahaya
Sensor ini membaca tingkat kecerahan ruangan dalam satuan lux. Nilainya digunakan untuk mengatur duty cycle lampu secara otomatis melalui sinyal PWM. Jika cahaya alami dari luar sudah cukup terang (di atas 150 lux), sistem tidak akan menyalakan lampu. Sebaliknya, jika ruangan gelap atau mendung, lampu dinyalakan secara proporsional. Proses transisi lampu dilakukan secara halus menggunakan teknik fading agar tidak terjadi kedipan mendadak.
2. SCD41 Sensor CO2, Suhu, dan Kelembapan
Sensor ini bekerja berdasarkan prinsip Demand-Controlled Ventilation. Pada kondisi normal ketika kelas masih kosong dan kadar CO2 rendah (di bawah 600 ppm), sistem akan mempertahankan sirkulasi udara internal saja tanpa membuka katup udara luar pada AC central. Ini secara langsung mengurangi beban kerja kompresor AC. Saat kelas sudah penuh dan kadar CO2 melonjak di atas 1000 ppm, sistem mengirim sinyal untuk membuka katup udara luar secara bertahap. Data suhu dan kelembapan dari sensor yang sama digunakan sebagai termostat pintar untuk mengatur level pendinginan AC.
3. HLK-LD2410 Sensor Radar Kehadiran
Sensor radar ini mendeteksi kehadiran manusia di dalam ruangan, baik yang sedang diam maupun bergerak. Berbeda dengan sensor PIR biasa yang hanya mendeteksi gerakan, LD2410 tetap dapat mendeteksi seseorang yang sedang duduk diam membaca. Logika utamanya sederhana: jika ruangan terdeteksi kosong selama durasi yang sudah ditentukan (misalnya 20 menit berturut-turut), sistem akan mematikan AC dan lampu secara otomatis. Ini adalah solusi langsung untuk masalah peralatan yang dibiarkan menyala semalaman.
4. PZEM-004T Sensor Energi Listrik
Sensor ini membaca tegangan, arus, daya, frekuensi, power factor, dan akumulasi energi dalam kWh secara real-time. Data kWh harian juga digunakan untuk keperluan pelaporan jejak karbon dan audit efisiensi energi antar ruangan.

Alur Kerja Sistem
Sebagai gambaran singkat, berikut skenario kerja harian sistem ini dari pagi hingga sore.
Pada pukul 07.00 ketika mahasiswa mulai masuk, sensor radar mendeteksi kehadiran dan mempersiapkan AC untuk menyala. Sensor cahaya kemudian mengecek kondisi ruangan — jika pagi itu cerah dan sinar matahari sudah cukup, lampu tidak dinyalakan. Memasuki siang hari saat kelas penuh, sensor CO2 mendeteksi udara mulai pengap dan sistem membuka katup ventilasi secara otomatis. Pada pukul 15.00 setelah jadwal kuliah selesai, sensor radar tidak lagi mendeteksi kehadiran. Setelah 20 menit, seluruh perangkat dimatikan secara otomatis. Sepanjang hari, sensor energi mencatat seluruh konsumsi listrik dan mengirimkan datanya ke server kampus.

Arsitektur Perangkat Keras
ESP32 berkomunikasi dengan BH1750 dan SCD41 melalui protokol I2C pada pin SDA (GPIO 21) dan SCL (GPIO 22). Sensor radar LD2410 terhubung via UART melalui Serial2 pada pin RX (GPIO 32) dan TX (GPIO 33). Sensor energi PZEM-004T terhubung via UART melalui Serial1 pada pin RX (GPIO 26) dan TX (GPIO 27). Output PWM untuk kontrol lampu menggunakan GPIO 18 dengan frekuensi 5000 Hz dan resolusi 8-bit.

Endpoint API
Setelah ESP32 terhubung ke jaringan WiFi, terdapat tiga endpoint yang bisa diakses melalui browser atau HTTP client.
GET / menampilkan halaman sambutan dan daftar endpoint yang tersedia.
GET /data mengembalikan seluruh data sensor dalam format JSON, termasuk nilai lux, CO2, suhu, kelembapan, status kehadiran, data energi listrik, duty cycle lampu saat ini, uptime, dan alamat IP perangkat.
GET /status mengembalikan ringkasan koneksi dan status perangkat dalam format teks biasa.

