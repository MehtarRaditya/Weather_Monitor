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
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"}; //DS3231
float lux; //BH1750 
float temperatureAHT, humidity; //AHT20
float temperatureBMP, pressure, seaLevelPressure,altitude; //BMP280
int digitalRainValue, analogRainValue; //FC-37 Rain Sensor
String formattedTime; //DS3231
String weatherStatus;
String conditionStatus = "";
unsigned long lastSensorUpdate = 0;
unsigned long lastDisplayUpdate = 0;
unsigned long lastBuzzerMillis= 0;
float avgTemp = (temperatureAHT + temperatureBMP) / 2.0;

void setup() {
  Serial.begin(9600);
  SPI.begin();
  Wire.begin();

  pinMode(rainDigital, INPUT);
  pinMode(rainAnalog, INPUT);

  tft.init(240, 240, SPI_MODE3);
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextWrap(false);

  check();

  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  display();
}

void loop() {
  unsigned long now = millis();

  if (now - lastSensorUpdate >= 1000) {
    lastSensorUpdate = now;

    header();
    DS3231();
    BH1750();
    AHT20();
    BMP280();
    FC37();
    weather();
    condition();
  }

  if (now - lastDisplayUpdate >= 10000) {
    lastDisplayUpdate = now;
    display();
  }
  
  Serial.println();
}


void header(){
  Serial.println("===============================");
  Serial.println("=       WEATHER MONITOR       =");
  Serial.println("===============================");
}

void check(){
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);

  if (!lightMeter.begin()) {
    tft.println("BH1750 not found!");
  }
  if (!aht.begin()){
    tft.println("AHT20 not found!");
  }
  if (!bmp.begin()) {
    tft.println("BMP280 not found!");
  }
  if (!rtc.begin()) { 
    tft.println("RTC not found!"); 
  }

  tft.println("BH1750 OK");
  tft.println("AHT20 OK");
  tft.println("BMP280 OK");
  tft.println("RTC OK");

  delay(5000);
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

  Serial.print("AHT20 Temperature: ");
  Serial.print(temperatureAHT);
  Serial.println(" °C");

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println(" %");
}

void BMP280(){
  float locationAltitude = 86; // meter, (adjust to your location)
  temperatureBMP = bmp.readTemperature();
  pressure = bmp.readPressure() / 100.0; // hPa
  seaLevelPressure = bmp.seaLevelForAltitude(locationAltitude, pressure);
  altitude = bmp.readAltitude(seaLevelPressure); // estimation

  Serial.print("BMP280 Temperature: ");
  Serial.print(temperatureBMP);
  Serial.println(" °C");

  Serial.print("Pressure: ");
  Serial.print(pressure);
  Serial.println(" hPa");

  // Serial.print("Sea Level Pressure: ");
  // Serial.print(seaLevelPressure);
  // Serial.println(" hPa");

  Serial.print("Altitude Est. : ");
  Serial.print(altitude);
  Serial.println(" m");
}

void FC37() {
  digitalRainValue = digitalRead(rainDigital);
  analogRainValue = analogRead(rainAnalog);
  Serial.print("Digital Rain Value: ");
  Serial.println(digitalRainValue);
  Serial.print("Analog Rain Value: ");
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

void weather() {
  if (analogRainValue < 500) {
    weatherStatus = "Rain / Hujan";
  } 
  else {
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
      weatherStatus = "Overcast / Mendung";
    }
  }
  Serial.print("Weather Status: ");
  Serial.println(weatherStatus);
}

void condition() {
  if (analogRainValue < 500) {
    conditionStatus = " - ";
  } 
  else {
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
  Serial.print("Information: ");
  Serial.println(conditionStatus);
}

void display() {
  tft.fillScreen(ST77XX_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextSize(2);
  tft.println("== WEATHER MONITOR ==");
  tft.println();


  tft.setTextSize(1);
  // DS3231
  tft.setTextColor(ST77XX_CYAN);
  tft.println("Last Updated:");
  tft.setTextColor(ST77XX_WHITE);
  tft.println(formattedTime);
  tft.println();

  // BH1750
  tft.setTextColor(ST77XX_YELLOW);
  tft.println("Light Intensity:");
  tft.setTextColor(ST77XX_WHITE);
  tft.print(lux);
  tft.println(" lx");
  tft.println();

  // AHT20
  tft.setTextColor(ST77XX_GREEN);
  tft.println("AHT20:");
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Temperature: ");
  tft.print(temperatureAHT, 1);
  tft.println(" C");
  tft.print("Humidity: ");
  tft.print(humidity, 1);
  tft.println(" %");
  tft.println();

  // BMP280
  tft.setTextColor(ST77XX_MAGENTA);
  tft.println("BMP280:");
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Temperature: ");
  tft.print(temperatureBMP, 1);
  tft.println(" C");
  tft.print("Pressure: ");
  tft.print(pressure, 1);
  tft.println(" hPa");
  tft.print("Altitude: ");
  tft.print(altitude, 1);
  tft.println(" m");
  tft.println();

  //FC-37 Rain Sensor
  tft.setTextColor(ST77XX_BLUE);
  tft.println("FC-37 Rain Sensor:");
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Digital Value: ");
  tft.println(digitalRainValue, 1);
  tft.print("Analog Value: ");
  tft.println(analogRainValue, 1);
  tft.println();

  //Condition Analysis
  tft.setTextColor(ST77XX_ORANGE);
  tft.println("Status:");
  tft.setTextSize(1);
  tft.setTextColor(ST77XX_WHITE);
  tft.print("Weather Status: ");
  tft.println(weatherStatus);
  tft.print("Condition: ");
  tft.println(conditionStatus);
}

void alert(){
  unsigned long now = millis();

  if (analogRainValue < 500) {
    digitalWrite(buzzer, HIGH);
  }
  else if (lux <= 1000) {
    if (now - lastBuzzerMillis >= 1000) {
      lastBuzzerMillis = now;
      digitalWrite(buzzer, HIGH);
    }
  }
  else {
    digitalWrite(buzzer, LOW);
  }
}


