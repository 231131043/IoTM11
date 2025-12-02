#include <WiFi.h>
#include <Firebase_ESP_Client.h>
 
// Wi-Fi credentials
const char* ssid = "namawifianda";        // nama Wi-Fi anda (SSID)
const char* password = "passwordwifianda"; // password Wi-Fi anda
 
// Firebase credentials
#define API_KEY "APIfirebaseAnda"    // Found in Project Settings > General
#define DATABASE_URL "https://project-anda-id.firebasedatabase.app/"  // From Realtime Database URL
#define USER_EMAIL "usernamayangusudah didaftarkan" // email user saat membuat database
#define USER_PASSWORD "your-auth-password"   // password user yang telah didaftarkan
 
#define dht 23
#define ldr 19
#define soil 18
 
void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("\n=== SMART PLANT GREENHOUSE ===");
    Serial.println("Inisialisasi sistem...\n");
 
    // Pin modes
    pinMode(LDR_PIN, INPUT);
    pinMode(SOIL_PIN, INPUT);
    pinMode(PIR_PIN, INPUT);
    pinMode(FLAME_PIN, INPUT);
    pinMode(OBJECT_PIN, INPUT);
 
    // Connect WiFi
    connectWiFi();
 
    // Setup NTP Time
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.println("Sinkronisasi waktu dengan NTP...");
 
    // Firebase setup
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;
 
    config.token_status_callback = tokenStatusCallback;
 
    Serial.println("Menghubungkan ke Firebase...");
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
 
    unsigned long fbStart = millis();
    while (!Firebase.ready() && millis() - fbStart < 10000) {
        Serial.print(".");
        delay(500);
    }
 
    if (Firebase.ready()) {
        Serial.println("\n✓ Firebase terhubung!");
        Serial.println("\n Sistem siap monitoring!\n");
    } else {
        Serial.println("\n✗ Firebase gagal terhubung, sistem tetap berjalan...\n");
    }
}
 
void loop() {
    // Cek koneksi WiFi
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi terputus! Mencoba reconnect...");
        connectWiFi();
    }
 
    // Update sensor secara berkala
    unsigned long now = millis();
    if (now - lastSensorUpdate > sensorInterval) {
        lastSensorUpdate = now;
        bacaKirimData();
    }
}
 
void connectWiFi() {
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Menghubungkan ke WiFi");
 
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
 
        if (millis() - start > 20000) {
            Serial.println("\n✗ Gagal terhubung WiFi - restart...");
            ESP.restart();
        }
    }
 
    Serial.println();
    Serial.println("✓ WiFi Terhubung!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
}
 
// Fungsi untuk mendapatkan timestamp epoch dalam milliseconds
unsigned long getTimeStamp() {
    time_t now;
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("⚠️ Gagal mendapat waktu NTP, gunakan millis()");
        return millis();
    }
 
    time(&now);
    return (unsigned long)now * 1000; // Convert ke milliseconds untuk Javascript
}
 
 
// Fungsi untuk membaca sensor dan kirim ke Firebase
void bacaKirimData() {
    Serial.println("\n====================================");
    Serial.println("         PEMBACAAN SENSOR GREENHOUSE");
    Serial.println("====================================\n");
 
    // === BACA LDR (Cahaya) ===
    int rawLdr = analogRead(LDR_PIN);
 
    // Mapping: LDR semakin gelap → nilai ADC semakin tinggi
    // Mapping: 0 = sangat terang, 100 = sangat gelap
    int lightLevel = map(rawLdr, 0, 4095, 0, 100);
    lightLevel = constrain(lightLevel, 0, 100);
 
    Serial.printf("🌤️ Cahaya: %d (ADC=%d)\n", lightLevel, rawLdr);
 
 
    // === BACA SOIL MOISTURE ===
    int rawSoil = analogRead(SOIL_PIN);
 
    // Mapping: Sensor kering = nilai tinggi, basah = nilai rendah
    int soilPercent = map(rawSoil, 4095, 0, 0, 100);
    soilPercent = constrain(soilPercent, 0, 100);
 
    Serial.printf("🌱 Kelembaban Tanah: %d (ADC=%d)\n", soilPercent, rawSoil);
 
    if (soilPercent < 40) {
        Serial.println("⚠️ STATUS: KERING - Perlu disiram!");
    } else {
        Serial.println("✓ STATUS: Kelembaban cukup");
    }
}
 
// === BACA SENSOR DIGITAL ===
    motionDetected = digitalRead(PIR_PIN) == HIGH;
    flameDetected = digitalRead(FLAME_PIN) == HIGH;
    objectDetected = digitalRead(OBJECT_PIN) == HIGH;
 
    Serial.printf("🚶 Gerakan (PIR): %s\n", 
                  motionDetected ? "TERDETEKSI ⚠️" : "Tidak ada");
    Serial.printf("🔥 Api: %s\n", 
                  flameDetected ? "TERDETEKSI ⚠️" : "Aman");
    Serial.printf("🧍 Objek: %s\n", 
                  objectDetected ? "TERDETEKSI ⚠️" : "Tidak ada");
 
 
    // === KIRIM KE FIREBASE ===
    if (Firebase.ready()) {
        Serial.println("\n📤 Mengirim data ke Firebase...");
        
        String basePath = "/greenhouse/sensors";
        bool allSuccess = true;
 
 
        // Kirim Light Level
        if (Firebase.RTDB.setInt(&fbdo, basePath + "/lightLevel", lightLevel)) {
            Serial.println("✓ lightLevel terkirim");
        } else {
            Serial.printf("✗ lightLevel gagal: %s\n", fbdo.errorReason().c_str());
            allSuccess = false;
        }
 
 
        // Kirim Soil Moisture
        if (Firebase.RTDB.setInt(&fbdo, basePath + "/soilMoisture", soilPercent)) {
            Serial.println("✓ soilMoisture terkirim");
        } else {
            Serial.printf("✗ soilMoisture gagal: %s\n", fbdo.errorReason().c_str());
            allSuccess = false;
        }
 
 
        // Kirim Motion (PIR)
        if (Firebase.RTDB.setBool(&fbdo, basePath + "/motion", motionDetected)) {
            Serial.println("✓ motion terkirim");
        } else {
            Serial.printf("✗ motion gagal: %s\n", fbdo.errorReason().c_str());
            allSuccess = false;
        }
    }
 
// === Kirim Flame ===
if (Firebase.RTDB.setBool(&fbdo, basePath + "/flame", flameDetected)) {
    Serial.println("✓ flame terkirim");
} else {
    Serial.printf("✗ flame gagal: %s\n", fbdo.errorReason().c_str());
    allSuccess = false;
}
 
// === Kirim Object ===
if (Firebase.RTDB.setBool(&fbdo, basePath + "/object", objectDetected)) {
    Serial.println("✓ object terkirim");
} else {
    Serial.printf("✗ object gagal: %s\n", fbdo.errorReason().c_str());
    allSuccess = false;
}
 
// === Kirim Timestamp (epoch milliseconds untuk Javascript Date) ===
unsigned long timestamp = getTimeStamp();
if (Firebase.RTDB.setUint(&fbdo, basePath + "/timestamp", timestamp)) {
    Serial.printf("✓ timestamp terkirim (%lu)\n", timestamp);
} else {
    Serial.printf("✗ timestamp gagal: %s\n", fbdo.errorReason().c_str());
    allSuccess = false;
}
 
if (allSuccess) {
    Serial.println("✔️ Semua data berhasil dikirim!");
} else {
    Serial.println("⚠️ Beberapa data gagal dikirim!");
}
 
} else {
    Serial.println("⚠️ Firebase belum siap, skip pengiriman!");
}
 
Serial.println();
delay(100);
 
