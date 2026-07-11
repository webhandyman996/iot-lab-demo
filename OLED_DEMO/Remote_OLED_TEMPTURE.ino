#include <Wire.h>
#include <avr/wdt.h>
#include <U8g2lib.h>

#include "motoDHT.h"
#include "config.h"

// =====================
// UART to ESP32 AT
// =====================
#define ESP_SERIAL Serial3

// =====================
// DHT
// =====================
DHT dht(DHTPIN1, DHTTYPE);

// =====================
// OLED (SSD1306 I2C)
// =====================
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

// =====================
// Upload interval
// =====================
const unsigned long UPLOAD_INTERVAL_MS = 10000;
unsigned long lastUploadMs = 0;

// =====================
// Google Apps Script
// =====================
const char GOOGLE_HOST[] = "script.google.com";
const char GOOGLE_PATH[] = "/macros/s/AKfycbxVK1WvnfHeIn9fWaJI0hwWAqyheFhIv0sFtzvi5tZnFZl3xg3nni9nf1Ckkv7TWKpS2g/exec";
const char API_KEY[] = "YOUR_API_KEY";
const char DEVICE_ID[] = "MEGA2560_001";

// =====================
// ESP32 Command Helper
// =====================
String sendESP32Command(String cmd, unsigned long timeout = 2000) {
  while (ESP_SERIAL.available()) ESP_SERIAL.read();
  delay(100);

  ESP_SERIAL.println(cmd);
  Serial.println("[ESP] Sent: " + cmd);

  String response = "";
  unsigned long start = millis();

  while (millis() - start < timeout) {
    if (ESP_SERIAL.available()) {
      char c = ESP_SERIAL.read();
      response += c;
      Serial.write(c);
    }
    wdt_reset();
    delay(10);
  }
  Serial.println();
  return response;
}

bool checkWiFiConnected() {
  String resp = sendESP32Command("AT+CIPSTATUS", 3000);
  return (resp.indexOf("STATUS:3") != -1 || resp.indexOf("status=3") != -1);
}

// =====================
// Upload Function
// =====================
bool postToGoogleScript(float t, float h) {
  String url = String("https://") + GOOGLE_HOST + GOOGLE_PATH;
  sendESP32Command("URL:" + url, 1000);

  String body = "{";
  body += "\"api_key\":\"" + String(API_KEY) + "\",";
  body += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  body += "\"temperature\":" + String(t, 2) + ",";
  body += "\"humidity\":" + String(h, 2);
  body += "}";

  sendESP32Command("BODY:" + body, 1000);
  String resp = sendESP32Command("SEND", 15000);

  return (resp.indexOf("Upload") != -1 || resp.indexOf("Response") != -1);
}

// =====================
// OLED Display
// =====================
void showOLED(float t, float h) {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);

  u8g2.drawStr(0, 15, "DHT11 Sensor");

  char buf[20];
  sprintf(buf, "Temp: %.1f C", t);
  u8g2.drawStr(0, 35, buf);

  sprintf(buf, "Hum : %.1f %%", h);
  u8g2.drawStr(0, 55, buf);

  u8g2.sendBuffer();
}

// =====================
// Setup
// =====================
void setup() {
  Serial.begin(9600);
  ESP_SERIAL.begin(9600);

  dht.begin();
  u8g2.begin();

  pinMode(13, OUTPUT);

  delay(2000);

  sendESP32Command(String("SSID:") + Wifi_SSID, 1000);
  sendESP32Command(String("PASS:") + Wifi_password, 1000);
  sendESP32Command("WIFI_TEST", 10000);

  wdt_enable(WDTO_8S);
  lastUploadMs = millis() - UPLOAD_INTERVAL_MS;
}

// =====================
// Loop
// =====================
void loop() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(t) && !isnan(h)) {
    Serial.print("T="); Serial.print(t);
    Serial.print(" C  H="); Serial.print(h); Serial.println(" %");

    showOLED(t, h);

    if (millis() - lastUploadMs >= UPLOAD_INTERVAL_MS) {
      lastUploadMs = millis();

      if (checkWiFiConnected()) {
        postToGoogleScript(t, h);
      }
    }
  } else {
    Serial.println("DHT read error");
  }

  digitalWrite(13, !digitalRead(13));
  wdt_reset();
  delay(2000);
}
