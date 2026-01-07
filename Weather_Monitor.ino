// ==========================================
// WEATHER MONITOR STATION - FINAL CODE
// Sesuai Laporan & Logika Full
// ==========================================

// Include Libraries
#include <Wire.h>
#include <BH1750.h>
#include <AHT20.h>
#include <Adafruit_BMP280.h>
#include <RTClib.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

// Define Pins
#define rainDigital 4
#define rainAnalog A0
#define buzzer 5
#define TFT_CS -1
#define TFT_DC 9
#define TFT_RST 8

// Create Sensor Objects
BH1750 lightMeter;
AHT20 aht;
Adafruit_BMP280 bmp;
RTC_DS3231 rtc;
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Global Variables
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}; 
float lux; 
float temperatureAHT, humidity; 
float temperatureBMP, pressure, seaLevelPressure, altitude; 
int digitalRainValue, analogRainValue; 
String formattedTime; 
String weatherStatus;
String conditionStatus = "";

// Variabel Timer & Update
unsigned long lastSensorUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastBuzzerMillis = 0;

// Variabel rata-rata suhu (Dideklarasikan di sini, dihitung nanti di loop)
float avgTemp = 0.0; 

void setup() {
  Serial.begin(9600);
  SPI.begin();
  Wire.begin();

  pinMode(rainDigital, INPUT);
  pinMode(rainAnalog, INPUT);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW); // Pastikan buzzer mati saat awal

  // Inisialisasi Layar TFT
  tft.init(240, 240, SPI_MODE3);
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);

  // Cek Koneksi Sensor
  check();

  // Tampilan Awal
  display();
}

void loop() {
  unsigned long now = millis();

  // Update Sensor & Logika setiap 1 detik (1000 ms)
  if (now - lastSensorUpdate >= 1000) {
    lastSensorUpdate = now;

    header();
    DS3231();   // Baca Waktu
    BH1750();   // Baca Cahaya
    AHT20();    // Baca Suhu & Lembab
    BMP280();   // Baca Tekanan
    FC37();     // Baca Hujan
    
    // --- PEMANGGILAN LOGIKA UTAMA ---
    weather();   // Tentukan Status Cuaca (Hujan/Cerah)
    condition(); // Tentukan Kenyamanan (Gerah/Nyaman)
    alert();     // Kontrol Buzzer
    // --------------------------------
  }

  // Update Layar TFT setiap 10 detik (Sesuai Laporan)
  if (now - lastDisplayUpdate >= 10000) {
    lastDisplayUpdate = now;
    display();
  }
  
  Serial.println(); // Baris baru di Serial Monitor
}

// ==========================================
// FUNGSI - FUNGSI PENDUKUNG
// ==========================================

void header(){
  Serial.println("===============================");
  Serial.println("=        WEATHER MONITOR        =");
  Serial.println("===============================");
}

void check(){
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);

  bool error = false;

  if (!lightMeter.begin()) {
    tft.println("BH1750 Error!"); error = true;
  }
  if (!aht.begin()){
    tft.println("AHT20 Error!"); error = true;
  }
  if (!bmp.begin()) {
    tft.println("BMP280 Error!"); error = true;
  }
  if (!rtc.begin()) { 
    tft.println("RTC Error!"); error = true;
  }

  if (!error) {
    tft.println("All Sensors OK");
    tft.println("Starting...");
  }
  
  delay(3000);
}

void BH1750() {
  lux = lightMeter.readLightLevel();
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
}

void AHT20(){
  temperatureAHT = aht.getTemperature();
  humidity  = aht.getHumidity();

  Serial.print("AHT20 Temp: ");
  Serial.print(temperatureAHT);
  Serial.println(" C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
}

void BMP280(){
  float locationAltitude = 86; // Sesuaikan lokasi (Purwakarta +/- 86m)
  temperatureBMP = bmp.readTemperature();
  pressure = bmp.readPressure() / 100.0; // hPa
  seaLevelPressure = bmp.seaLevelForAltitude(locationAltitude, pressure);
  altitude = bmp.readAltitude(seaLevelPressure); 

  Serial.print("BMP280 Temp: ");
  Serial.print(temperatureBMP);
  Serial.println(" C");
  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.println(" hPa");
}

void FC37() {
  digitalRainValue = digitalRead(rainDigital);
  analogRainValue = analogRead(rainAnalog);
  Serial.print("Rain Digital: ");
  Serial.println(digitalRainValue);
  Serial.print("Rain Analog: ");
  Serial.println(analogRainValue);
}

void DS3231() {
  DateTime now = rtc.now();
  String yearStr = String(now.year(), DEC);
  String monthStr = (now.month() < 10 ? "0" : "") + String(now.month(), DEC);
  String dayStr = (now.day() < 10 ? "0" : "") + String(now.day(), DEC);
  String hourStr = (now.hour() < 10 ? "0" : "") + String(now.hour(), DEC); 
  String minuteStr = (now.minute() < 10 ? "0" : "") + String(now.minute(), DEC);
  String secondStr = (now.second() < 10 ? "0" : "") + String(now.second(), DEC);
  String dayOfWeek = daysOfTheWeek[now.dayOfTheWeek()];

  formattedTime = dayOfWeek + ", " + yearStr + "-" + monthStr + "-" + dayStr + " " + hourStr + ":" + minuteStr + ":" + secondStr;
  Serial.println(formattedTime);
}

// ==========================================
// LOGIKA AI (RULE BASED & FUZZY SEDERHANA)
// ==========================================

void weather() {
  // Ambil jam saat ini dari RTC
  DateTime now = rtc.now();
  int currentHour = now.hour();

  // Prioritas 1: Hujan (Selalu deteksi hujan 24 jam)
  if (analogRainValue < 500) {
    weatherStatus = "Rain / Hujan";
  } 
  else {
    // Prioritas 2: Cahaya
    if (lux >= 7000) {
      weatherStatus = "Sweltering/Terik";
    }
    else if (lux >= 5000 && lux < 7000) {
      weatherStatus = "Clear / Cerah";
    }
    else if (lux >= 1000 && lux < 5000) {
      weatherStatus = "Cloudy / Berawan";
    }
    else {
      if (currentHour >= 6 && currentHour < 18) {
         weatherStatus = "Overcast / Mendung";
      } else {
         weatherStatus = "Night / Malam";
      }
    }
  }
  Serial.print("Weather Status: ");
  Serial.println(weatherStatus);
}

void condition() {
  // DIPERBAIKI: Hitung rata-rata suhu di sini agar selalu update
  avgTemp = (temperatureAHT + temperatureBMP) / 2.0;

  if (analogRainValue < 500) {
    conditionStatus = " - "; // Kalau hujan, status kenyamanan diabaikan
  } 
  else {
    // Tabel 4.2 Laporan (Logika Kenyamanan)
    if (avgTemp > 33) {
      conditionStatus = "Danger / Bahaya";
    }
    else if (avgTemp > 29 && humidity > 70) {
      conditionStatus = "Hot / Panas";
    }
    else if (avgTemp < 22) {
      conditionStatus = "Cold / Dingin";
    }
    else {
      conditionStatus = "Pleasant/Nyaman";
    }
  }
  
  // Debugging info
  Serial.print("Avg Temp: ");
  Serial.println(avgTemp);
  Serial.print("Condition: ");
  Serial.println(conditionStatus);
}

void alert(){
  unsigned long now = millis();

  // Kondisi 1: HUJAN -> Buzzer Nyala Terus (Kontinu)
  if (weatherStatus == "Rain / Hujan") {
    digitalWrite(buzzer, HIGH); 
  }
  // Kondisi 2: MENDUNG -> Buzzer Putus-putus (Per 1 detik)
  else if (weatherStatus == "Overcast / Mendung") {
    // DIPERBAIKI: Timer reset ditambahkan agar kedip berfungsi benar
    if (now - lastBuzzerMillis >= 1000) {
      lastBuzzerMillis = now; // <--- PENTING: Reset waktu
      
      int buzzerState = digitalRead(buzzer);
      digitalWrite(buzzer, !buzzerState); // Balik status (High->Low / Low->High)
    }
  }
  // Kondisi 3: Aman -> Matikan Buzzer
  else {
    digitalWrite(buzzer, LOW);
  }
}

// ==========================================
// TAMPILAN LAYAR (GUI)
// ==========================================

void display() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("== WEATHER MONITOR ==");
  tft.println();

  tft.setTextSize(1);
  
  // Waktu
  tft.setTextColor(ST77XX_CYAN);
  tft.println("Last Updated:");
  tft.setTextColor(ST77XX_WHITE);
  tft.println(formattedTime);
  tft.println();

  // Cahaya
  tft.setTextColor(ST77XX_YELLOW);
  tft.println("Light Intensity:");
  tft.setTextColor(ST77XX_WHITE);
  tft.print(lux);
  tft.println(" lx");
  tft.println();

  // AHT20
  tft.setTextColor(ST77XX_GREEN);
  tft.println("AHT20 (Temp/Hum):");
  tft.setTextColor(ST77XX_WHITE);
  tft.print(temperatureAHT, 1);
  tft.print(" C  |  ");
  tft.print(humidity, 1);
  tft.println(" %");
  tft.println();

  // BMP280
  tft.setTextColor(ST77XX_MAGENTA);
  tft.println("BMP280 (Press/Alt):");
  tft.setTextColor(ST77XX_WHITE);
  tft.print(pressure, 1);
  tft.print(" hPa | ");
  tft.print(altitude, 0);
  tft.println(" m");
  tft.println();

  // Hujan
  tft.setTextColor(ST77XX_BLUE);
  tft.println("Rain Sensor (A0):");
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Value: ");
  tft.println(analogRainValue);
  tft.println();

  // STATUS FINAL
  tft.setTextColor(ST77XX_ORANGE);
  tft.println("STATUS:");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  
  tft.print("Weather:   ");
  tft.println(weatherStatus);
  
  tft.print("Condition: ");
  tft.println(conditionStatus);
}