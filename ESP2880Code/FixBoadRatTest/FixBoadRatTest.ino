/*
  WiFiEsp example: BasicTest

  This sketch tests the basic communication with ESP8266 WiFi module.
  You should see the ESP8266 firmware version and a successful AT test.

  created in 2016 by bportaluri
*/

#include "WiFiEsp.h"

// Emulate Serial1 on pins 6/7 if not present
#ifndef HAVE_HWSERIAL1
#include "SoftwareSerial.h"
SoftwareSerial Serial1(6, 7); // RX, TX
#endif

void setup()
{
  Serial.begin(115200);    // Serial monitor
  Serial1.begin(115200);   // ESP8266 module
  WiFi.init(&Serial1);     // Initialize WiFi library

  Serial.println("WiFiEsp Basic Test");
  Serial.println("Attempting to start communication with ESP8266...");

  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present or not responding.");
    while (true);
  }

  Serial.println("ESP8266 WiFi shield initialized.");
  Serial.print("Firmware version: ");
  Serial.println(WiFi.firmwareVersion());
}

void loop() {
  // 僅發送 AT 指令
  Serial1.println("AT"); 
  // 檢查回覆
  while (Serial1.available()) {
    Serial.write(Serial1.read());
  }
  delay(1000); // 每秒發送一次
}
