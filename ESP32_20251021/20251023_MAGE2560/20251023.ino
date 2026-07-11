#define ESP32_Serial Serial1

String ssid = "YOUR_WIFI_SSID";
String password = "YOUR_WIFI_PASSWORD";
String scriptUrl = "https://script.google.com/macros/s/AKfycbyaeRjLSCww7jJVwlnVsp57GIosTJaxnVWxFxMumkv5OMSGW-a78LtcJ4NCsjoij8x-/exec";

unsigned long lastUploadTime = 0;
const unsigned long uploadInterval = 10000;
String esp32IPAddress = "";
bool wifiConnected = false;
bool waitingForResponse = false;
unsigned long responseStartTime = 0;

void setup() {
  Serial.begin(9600);
  ESP32_Serial.begin(115200);

  Serial.println("========================================");
  Serial.println("  Mega 2560 + ESP32 系統啟動");
  Serial.println("========================================");
  Serial.println("WiFi: " + ssid);
  Serial.println("========================================");
  
  delay(3000);
  
  while (ESP32_Serial.available()) ESP32_Serial.read();
  
  randomSeed(analogRead(A0));
  
  Serial.println("✅ 初始化完成\n");
  Serial.println("⏳ WiFi 連線測試中...");
  
  testWiFi();
}

void loop() {
  receiveFromESP32();
  
  if (waitingForResponse && millis() - responseStartTime > 20000) {
    waitingForResponse = false;
    if (!wifiConnected) {
      Serial.println("\n❌ 測試超時，30秒後重試...\n");
    }
  }
  
  if (wifiConnected) {
    if (millis() - lastUploadTime >= uploadInterval) {
      lastUploadTime = millis();
      uploadData();
    }
  } else {
    static unsigned long lastRetry = 0;
    if (!waitingForResponse && millis() - lastRetry >= 30000) {
      lastRetry = millis();
      Serial.println("\n⚠️  重新連線...");
      testWiFi();
    }
  }
  
  delay(100);
}

void testWiFi() {
  Serial.println("╔════════════════════════════════════════╗");
  Serial.println("║      WiFi 連線測試                    ║");
  Serial.println("╚════════════════════════════════════════╝");
  
  while (ESP32_Serial.available()) ESP32_Serial.read();
  delay(100);
  
  ESP32_Serial.println("SSID:" + ssid);
  delay(200);
  ESP32_Serial.println("PASS:" + password);
  delay(200);
  ESP32_Serial.println("WIFI_TEST");
  
  waitingForResponse = true;
  responseStartTime = millis();
  
  Serial.println("⏳ 等待結果...");
}

void uploadData() {
  if (!wifiConnected) return;
  
  float temp = random(200, 350) / 10.0;
  float humi = random(400, 800) / 10.0;
  String json = "{\"temp\":" + String(temp, 1) + ",\"humi\":" + String(humi, 1) + "}";

  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║       上傳資料到 Google Sheet         ║");
  Serial.println("╚════════════════════════════════════════╝");
  Serial.println("📊 溫度: " + String(temp, 1) + "°C");
  Serial.println("💧 濕度: " + String(humi, 1) + "%");
  Serial.println("📦 JSON: " + json);
  Serial.println("📍 IP: " + esp32IPAddress);
  Serial.println("----------------------------------------");

  ESP32_Serial.println("SSID:" + ssid);
  delay(100);
  ESP32_Serial.println("PASS:" + password);
  delay(100);
  ESP32_Serial.println("URL:" + scriptUrl);
  delay(100);
  ESP32_Serial.println("BODY:" + json);
  delay(100);
  ESP32_Serial.println("SEND");
  
  Serial.println("📤 已送出");
}

void receiveFromESP32() {
  if (ESP32_Serial.available()) {
    String msg = ESP32_Serial.readStringUntil('\n');
    msg.trim();
    
    if (msg.length() == 0) return;
    
    if (msg.indexOf("ready") >= 0) {
      Serial.println("🟢 [ESP32] 已就緒");
    }
    else if (msg.indexOf("received") >= 0) {
      Serial.println("📥 [ESP32] " + msg);
    }
    else if (msg.indexOf("Testing WiFi") >= 0) {
      Serial.println("🔄 [ESP32] 開始測試");
    }
    else if (msg.indexOf("Connecting WiFi") >= 0) {
      Serial.println("🔄 [ESP32] 連線中...");
    }
    else if (msg.indexOf("WiFi connected") >= 0) {
      Serial.println("✅ [ESP32] WiFi 連線成功！");
      wifiConnected = true;
      waitingForResponse = false;
    }
    else if (msg.indexOf("IP:") >= 0) {
      esp32IPAddress = msg.substring(msg.indexOf("IP:") + 3);
      esp32IPAddress.trim();
      Serial.println("🌐 IP: " + esp32IPAddress);
      Serial.println("========================================");
      Serial.println("✅ 系統就緒，開始資料上傳\n");
    }
    else if (msg.indexOf("WiFi failed") >= 0) {
      Serial.println("❌ [ESP32] WiFi 失敗");
      wifiConnected = false;
      waitingForResponse = false;
    }
    else if (msg.indexOf("Request sent") >= 0) {
      Serial.println("📮 [ESP32] 請求已送出");
    }
    else if (msg.indexOf("Response:") >= 0) {
      String resp = msg.substring(msg.indexOf(":") + 1);
      resp.trim();
      Serial.println("📨 [Google] " + resp);
    }
    else if (msg.indexOf("Upload complete") >= 0) {
      Serial.println("╔════════════════════════════════════════╗");
      Serial.println("║    ✅ 上傳完成！                      ║");
      Serial.println("╚════════════════════════════════════════╝\n");
    }
    else if (msg.indexOf("Connection failed") >= 0) {
      Serial.println("❌ [ESP32] 伺服器連線失敗");
    }
    else if (msg == ".") {
      Serial.print(".");
    }
    else {
      Serial.println("[ESP32] " + msg);
    }
  }
}