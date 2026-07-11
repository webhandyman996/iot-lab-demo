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

// 定義要測試的鮑率列表
const long baudRates[] = {
  9600,
  19200,
  38400,
  57600,
  74880, 
  115200
};
// 計算鮑率的總數
const int numBaudRates = sizeof(baudRates) / sizeof(baudRates[0]);

// 追蹤當前鮑率在陣列中的索引
int currentBaudIndex = 0;

// 上次切換鮑率的時間
unsigned long previousMillis = 0;

// 定義切換間隔 (3 秒 = 3000 毫秒)
const long interval = 3000;

void setup()
{
  // 1. 設定主序列埠 (Serial) 鮑率。
  // 注意：序列埠監視器 (Serial Monitor) 的鮑率必須與此設定一致，
  // 否則主序列埠也會出現亂碼。我們保持 115200 讓它能正常顯示訊息。
  Serial.begin(115200);
  Serial.println("\nWiFiEsp Baud Rate Test Started.");
  
  // 2. 初始化 ESP8266 序列埠 (Serial1)
  // 首次設定為列表中的第一個鮑率
  long initialBaud = baudRates[currentBaudIndex];
  Serial1.begin(initialBaud);
  WiFi.init(&Serial1);
  
  Serial.print("Initial Baud Rate Set: ");
  Serial.println(initialBaud);
  
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present or not responding.");
    // 不停止，讓它繼續測試鮑率
  }

  Serial.println("ESP8266 WiFi module initialized.");
}

void loop()
{
  // 使用 millis() 實現非阻塞延時，取代 delay()
  unsigned long currentMillis = millis();

  // 判斷是否已經過了 3 秒 (3000 ms)
  if (currentMillis - previousMillis >= interval) {
    // 儲存當前時間，為下次切換做準備
    previousMillis = currentMillis;

    // 1. 切換到下一個鮑率
    currentBaudIndex = (currentBaudIndex + 1) % numBaudRates;
    long nextBaud = baudRates[currentBaudIndex];
    
    // 2. 停止並重新啟動 ESP8266 序列埠 (Serial1)
    Serial1.end(); // 停止當前的 Serial1
    Serial1.begin(nextBaud); // 以新的鮑率啟動 Serial1
    
    // 3. 顯示當前資訊
    Serial.println("------------------------------------");
    Serial.print("Switching to Baud Rate: ");
    Serial.println(nextBaud);
    Serial.print("Index: ");
    Serial.println(currentBaudIndex);
    Serial.println("------------------------------------");
    
    // 4. 發送一個簡單的 AT 指令進行測試
    // 如果鮑率匹配，ESP8266 會回覆 "OK" 或其他訊息。
    Serial1.println("AT"); 
  }
  
  // 檢查是否有來自 ESP8266 (Serial1) 的回覆
  while (Serial1.available()) {
    // 將 ESP8266 的回覆字節讀取並輸出到主序列埠監視器 (Serial)
    Serial.write(Serial1.read());
  }
  
  // 延遲一小段時間 (可選，但在某些情況下有助於穩定)
  delay(1); 
}