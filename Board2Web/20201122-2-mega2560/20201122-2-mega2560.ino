/*
  MEGA2560 + ESP32(AT command mode) - 純AT指令版本
  Upload Temperature/Humidity to Google Apps Script (HTTPS)
  Google Script URL: https://script.google.com/macros/s/...
  API Key: YOUR_API_KEY
  
  不使用WiFiEsp庫，直接用AT指令控制ESP32
*/

#include <Wire.h>
#include <avr/wdt.h>

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
// Upload interval
// =====================
const unsigned long UPLOAD_INTERVAL_MS = 10000; // 10 秒（測試用）
unsigned long lastUploadMs = 0;

// =====================
// Google Apps Script endpoint (HTTPS)
// =====================
const char GOOGLE_HOST[] = "script.google.com";
const char GOOGLE_PATH[] = "/macros/s/AKfycbxVK1WvnfHeIn9fWaJI0hwWAqyheFhIv0sFtzvi5tZnFZl3xg3nni9nf1Ckkv7TWKpS2g/exec";
const char API_KEY[] = "YOUR_API_KEY";
const char DEVICE_ID[] = "MEGA2560_001";

// =====================
// IO (optional, from your original logic)
// =====================
int Liquid_level = 0;

// =====================
// ESP32 自定義協議輔助函數
// =====================
String sendESP32Command(String cmd, unsigned long timeout = 2000) {
  // 清空接收緩衝區
  while (ESP_SERIAL.available()) {
    ESP_SERIAL.read();
  }
  delay(150);  // 增加延遲確保清空完成
  
  ESP_SERIAL.println(cmd);
  Serial.println("[ESP] Sent: " + cmd);
  
  // 給 ESP32 更多時間處理命令
  delay(500);  // 從 200ms 增加到 500ms
  
  String response = "";
  unsigned long start = millis();
  unsigned long lastWdtReset = millis();  // 記錄上次重置 watchdog 的時間
  
  Serial.println("[ESP] Waiting for response...");
  
  while (millis() - start < timeout) {
    if (ESP_SERIAL.available()) {
      char c = ESP_SERIAL.read();
      response += c;
      Serial.write(c);  // Echo to monitor
    }
    
    // 定期重置 watchdog，防止長時間等待時超時
    if (millis() - lastWdtReset > 1000) {  // 每秒重置一次
      wdt_reset();
      lastWdtReset = millis();
    }
    
    delay(10);
  }
  
  Serial.println();
  Serial.print("[ESP] Received bytes: ");
  Serial.println(response.length());
  return response;
}

// 檢查ESP32是否連線WiFi
bool checkWiFiConnected() {
  Serial.println(F("[ESP] Checking WiFi status..."));
  String resp = sendESP32Command("AT+CIPSTATUS", 3000);
  
  Serial.print(F("[ESP] Response length: "));
  Serial.println(resp.length());
  
  // ESP32自定義協議回應格式: 
  // [AT] CIPSTATUS -> status=3 (WL_CONNECTED)
  // STATUS:3
  // IP:x.x.x.x
  bool connected = false;
  
  if (resp.indexOf("status=3") != -1) {
    Serial.println(F("[ESP] Found 'status=3' in response"));
    connected = true;
  } else if (resp.indexOf("STATUS:3") != -1) {
    Serial.println(F("[ESP] Found 'STATUS:3' in response"));
    connected = true;
  } else {
    Serial.println(F("[ESP] Status 3 not found in response"));
  }
  
  // 額外排除 IP 為 0.0.0.0 的情況
  if (connected && resp.indexOf("IP:0.0.0.0") != -1) {
    Serial.println(F("[ESP] IP is 0.0.0.0, treating as disconnected"));
    connected = false;
  }
  
  if (connected) {
    Serial.println(F("[ESP] ✓ WiFi is connected"));
  } else {
    Serial.println(F("[ESP] ✗ WiFi not connected"));
  }
  
  return connected;
}

/*
  使用ESP32自定義協議上傳到Google Apps Script
  
  協議步驟：
  1. SSID:xxx (可選，如果ESP32未連線)
  2. PASS:xxx (可選，如果ESP32未連線)
  3. WIFI_TEST (可選，測試連線)
  4. URL:https://script.google.com/...
  5. BODY:{"api_key":"xxx","device_id":"xxx","temperature":25.5,"humidity":65.0}
  6. SEND
*/
bool postToGoogleScript(float tempC, float hum) {
  Serial.println(F(""));
  Serial.println(F("========================================"));
  Serial.println(F("[GOOGLE] *** postToGoogleScript() CALLED ***"));
  Serial.println(F("========================================"));
  
  // 檢查資料，如果異常則使用測試值
  bool dataValid = true;
  if (isnan(tempC) || isnan(hum)) {
    Serial.println(F("[GOOGLE] DHT NaN detected!"));
    Serial.println(F("[GOOGLE] Using test values for debugging: T=25.0, H=60.0"));
    tempC = 25.0;
    hum = 60.0;
    dataValid = false;
  }
  
  Serial.print(F("[GOOGLE] Uploading: T=")); Serial.print(tempC);
  Serial.print(F(", H=")); Serial.println(hum);

  // 構建完整URL
  String fullUrl = String("https://") + GOOGLE_HOST + GOOGLE_PATH;
  Serial.println(F("[ESP] Sending URL..."));
  sendESP32Command("URL:" + fullUrl, 1000);
  delay(500);
  wdt_reset();  // 防止 watchdog 超時
  
  // 構建JSON body
  String jsonBody = "{";
  jsonBody += "\"api_key\":\"" + String(API_KEY) + "\",";
  jsonBody += "\"device_id\":\"" + String(DEVICE_ID) + "\",";
  jsonBody += "\"temperature\":" + String(tempC, 2) + ",";
  jsonBody += "\"humidity\":" + String(hum, 2);
  jsonBody += "}";
  
  Serial.println(F("[ESP] Sending BODY..."));
  Serial.println("[ESP] JSON: " + jsonBody);
  sendESP32Command("BODY:" + jsonBody, 1000);
  delay(500);
  wdt_reset();  // 防止 watchdog 超時
  
  // 發送上傳命令
  Serial.println(F("[ESP] Sending SEND command..."));
  wdt_reset();  // SEND 命令會等待較長時間，先重置 watchdog
  String resp = sendESP32Command("SEND", 15000);  // 等待15秒
  wdt_reset();  // SEND 完成後再次重置
  
  // 檢查回應
  bool success = (resp.indexOf("Upload complete") != -1 || resp.indexOf("Response:") != -1);
  
  if (success) {
    Serial.println(F("\n[GOOGLE] Upload OK"));
    return true;
  } else {
    Serial.println(F("\n[GOOGLE] Upload FAILED"));
    return false;
  }
}

// =====================
// Setup
// =====================
void setup() {
  Serial.begin(9600);
  ESP_SERIAL.begin(9600);  // 使用穩定的 9600 鮑率

  pinMode(13, OUTPUT);
  pinMode(A0, INPUT);
  pinMode(5, INPUT);
  pinMode(10, OUTPUT);

  dht.begin();

  Serial.println(F("\n\n=== MEGA2560 + ESP32 Custom Protocol ==="));
  
  // *** UART 硬體測試 ***
  Serial.println(F("[UART] Testing Serial3 hardware..."));
  Serial.print(F("[UART] Serial3 available: "));
  Serial.println(ESP_SERIAL ? "YES" : "NO");
  
  // 等待ESP32啟動
  delay(2000);
  
  // *** 簡單 Echo 測試 ***
  Serial.println(F("[UART] Sending test command: TEST"));
  ESP_SERIAL.println("TEST");
  
  // 持續檢查 3 秒，每 100ms 顯示狀態
  Serial.println(F("[UART] Waiting for response (3 seconds)..."));
  bool receivedData = false;
  for (int i = 0; i < 30; i++) {
    delay(100);
    if (ESP_SERIAL.available() > 0) {
      receivedData = true;
      break;
    }
    if (i % 5 == 0) {  // 每 500ms 顯示一個點
      Serial.print(".");
    }
  }
  Serial.println();
  
  Serial.print(F("[UART] Available bytes: "));
  Serial.println(ESP_SERIAL.available());
  
  bool uartOK = false;
  if (ESP_SERIAL.available() > 0) {
    Serial.println(F("[UART] ✓ Received data:"));
    while (ESP_SERIAL.available()) {
      Serial.write(ESP_SERIAL.read());
    }
    Serial.println();
    uartOK = true;
  } else {
    Serial.println(F("[UART] ✗✗✗ CRITICAL ERROR: No response from ESP32! ✗✗✗"));
    Serial.println(F("[UART] UART communication is broken!"));
    Serial.println(F("[UART] "));
    Serial.println(F("[UART] Please check wiring:"));
    Serial.println(F("[UART]   ESP32 TX (GPIO17) --> Mega2560 RX3 (pin 15)"));
    Serial.println(F("[UART]   ESP32 RX (GPIO16) --> Mega2560 TX3 (pin 14)"));
    Serial.println(F("[UART]   ESP32 GND         --> Mega2560 GND"));
    Serial.println(F("[UART] "));
    Serial.println(F("[UART] System will HALT. Please fix wiring and reset."));
    Serial.println(F("[UART] "));
    
    // 停止執行，不斷閃爍 LED 13 表示錯誤
    while (true) {
      digitalWrite(13, HIGH);
      delay(100);
      digitalWrite(13, LOW);
      delay(100);
    }
  }
  
  delay(1000);
  
  // 清空緩衝區
  while (ESP_SERIAL.available()) {
    ESP_SERIAL.read();
  }
  
  Serial.println(F("[UART] ✓ UART communication verified!"));
  
  // 重置 watchdog（防止 setup 過程中超時）
  wdt_reset();
  
  // 檢查ESP32回應
  Serial.println(F("[ESP] Checking ESP32..."));
  delay(500);
  
  // 等待 ESP32 ready 訊息
  unsigned long start = millis();
  bool esp32Ready = false;
  while (millis() - start < 3000) {
    while (ESP_SERIAL.available()) {
      String line = ESP_SERIAL.readStringUntil('\n');
      Serial.println("[ESP] " + line);
      if (line.indexOf("ESP32 ready") != -1 || line.indexOf("Waiting") != -1) {
        esp32Ready = true;
        break;
      }
    }
    if (esp32Ready) break;
    delay(100);
  }
  
  if (esp32Ready) {
    Serial.println(F("[ESP] ESP32 is ready"));
  } else {
    Serial.println(F("[ESP] Warning: No response from ESP32"));
  }
  
  // 從 config.h 讀取 WiFi 設定並傳送給 ESP32
  Serial.println(F("[ESP] Configuring WiFi from Mega2560..."));
  wdt_reset();  // 重置 watchdog
  
  Serial.print(F("[ESP] Sending SSID: ")); Serial.println(Wifi_SSID);
  sendESP32Command(String("SSID:") + Wifi_SSID, 1000);
  delay(500);
  
  Serial.print(F("[ESP] Sending PASSWORD: ")); Serial.println("****");
  sendESP32Command(String("PASS:") + Wifi_password, 1000);
  delay(500);
  
  // 觸發 WiFi 連線測試
  Serial.println(F("[ESP] Triggering WiFi connection..."));
  wdt_reset();  // 重置 watchdog，WiFi 連線可能需要時間
  sendESP32Command("WIFI_TEST", 10000);  // 增加等待時間到 10 秒
  delay(2000);  // 等待連線完成
  
  // 檢查WiFi狀態
  Serial.println(F("[ESP] Verifying WiFi connection..."));
  wdt_reset();  // 重置 watchdog
  delay(500);
  
  if (checkWiFiConnected()) {
    Serial.println(F("[ESP] ✓ WiFi configuration successful!"));
  } else {
    Serial.println(F("[ESP] ✗ WiFi configuration failed!"));
    Serial.println(F("[ESP] Retrying WiFi check..."));
    delay(1000);
    // 再試一次
    if (checkWiFiConnected()) {
      Serial.println(F("[ESP] ✓ WiFi configuration successful (2nd attempt)!"));
    } else {
      Serial.println(F("[ESP] ✗ WiFi configuration failed!"));
      Serial.println(F("[ESP] Please check SSID/Password in config.h"));
    }
  }
  
  // 啟用 Watchdog Timer (8秒)
  // 注意：sendESP32Command 內部會定期重置，可支援最長 15 秒的 SEND 命令
  wdt_enable(WDTO_8S);
  wdt_reset();  // 重置 watchdog
  
  // 設定為負值，讓第一次 loop 立即上傳
  lastUploadMs = millis() - UPLOAD_INTERVAL_MS - 1000;
  
  Serial.println(F("[SETUP] Complete\n"));
  Serial.println(F("[SETUP] Watchdog timer enabled (8 seconds)"));
  Serial.println(F("[SETUP] First upload will happen in next loop cycle"));
  Serial.println(F(""));
}

// =====================
// Loop
// =====================
void loop() {
  // Read optional IO (kept from your style)
  int soil = 1023 - analogRead(A0);
  Liquid_level = digitalRead(5);

  // *** 測試模式：使用固定預設值（未接傳感器）***
  // float h1 = dht.readHumidity();
  // float t1 = dht.readTemperature();
  float t1 = 25.5;  // 預設溫度
  float h1 = 65.0;  // 預設濕度

  Serial.print(F("Soil=")); Serial.println(soil);
  Serial.print(F("WaterLevel=")); Serial.println(Liquid_level);
  Serial.print(F("T=")); Serial.print(t1); Serial.print(F("°C (TEST), "));
  Serial.print(F("H=")); Serial.print(h1); Serial.println(F("% (TEST)"));
  
  // 顯示下次上傳倍數
  unsigned long now = millis();
  unsigned long timeSinceUpload = now - lastUploadMs;
  unsigned long timeUntilUpload = UPLOAD_INTERVAL_MS - timeSinceUpload;
  
  if (timeUntilUpload > UPLOAD_INTERVAL_MS) timeUntilUpload = 0; // 防止溢位
  
  Serial.print(F("[INFO] Next upload in: "));
  Serial.print(timeUntilUpload / 1000);
  Serial.print(F(" seconds (lastUploadMs="));
  Serial.print(lastUploadMs);
  Serial.print(F(", now="));
  Serial.print(now);
  Serial.println(F(")"));

  if ((now - lastUploadMs) >= UPLOAD_INTERVAL_MS) {
    lastUploadMs = now;

    Serial.println(F("\n╔════════════════════════════════════════════╗"));
    Serial.println(F("║  UPLOAD CYCLE TRIGGERED                    ║"));
    Serial.println(F("╚════════════════════════════════════════════╝"));
    Serial.println(F("[MAIN] === Starting upload to Google Apps Script ==="));
    
    // 檢查WiFi狀態
    Serial.println(F("[MAIN] Checking WiFi connection..."));
    bool wifiOK = checkWiFiConnected();
    
    if (wifiOK) {
      Serial.println(F("[MAIN] WiFi OK, attempting upload..."));
      bool uploadOK = postToGoogleScript(t1, h1);
      if (uploadOK) {
        Serial.println(F("[MAIN] ✓ Upload SUCCESS"));
      } else {
        Serial.println(F("[MAIN] ✗ Upload FAILED"));
      }
    } else {
      Serial.println(F("[MAIN] ✗ ESP32 NOT connected to WiFi!"));
      Serial.println(F("[MAIN] Please check ESP32 WiFi connection first"));
      Serial.println(F("[MAIN] You may need to configure ESP32 with AT+CWJAP"));
    }
    
    Serial.println(F("[MAIN] === Upload cycle complete ===\n"));
  }

  // heartbeat
  digitalWrite(13, HIGH); delay(120);
  digitalWrite(13, LOW);  delay(120);

  wdt_reset();
  delay(800);
}
