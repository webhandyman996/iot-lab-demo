#include <WiFi.h>
#include <WiFiClientSecure.h>

// *** 預設 WiFi 設定（可由 UART 覆蓋） ***
const char* DEFAULT_SSID = "";      // 留空，由 Mega2560 提供
const char* DEFAULT_PASSWORD = "";  // 留空，由 Mega2560 提供

// UART: use Serial2 (GPIO16 RX, GPIO17 TX)
// 支援以 UUID 代表 SSID，由 UART 指令提供
String ssid, uuid, password, url, body;
bool readyToSend = false;
bool wifiConnected = false;

WiFiClientSecure client;

void setup() {
  // UART 使用 9600 與 Mega2560 一致，確保穩定傳輸
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  delay(500);
  
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP32 WiFi Handler (Debug Mode) ===");
  Serial.println("Waiting for WiFi configuration from Mega2560...");
  Serial2.println("ESP32 ready");
  Serial.println("Waiting for commands...\n");
}

void loop() {
  if (Serial2.available()) {
    String line = Serial2.readStringUntil('\n');
    line.trim();
    
    Serial.println("<<< Received: [" + line + "]");

    // *** 測試命令：簡單 echo ***
    if (line == "TEST") {
      Serial.println(">>> Received TEST command");
      // 延遲讓 Mega2560 準備好接收
      delay(500);
      Serial2.println("ESP32 Echo: TEST received");
      Serial2.flush();
      Serial.println(">>> Sent echo response (after 500ms delay)");
    }
    else if (line.startsWith("SSID:")) {
      ssid = line.substring(5);
      Serial2.println("SSID received");
      Serial.println("    SSID = [" + ssid + "] (length=" + String(ssid.length()) + ")");
    }
    // 新增：支援 UUID 當作 SSID 的別名
    else if (line.startsWith("UUID:")) {
      uuid = line.substring(5);
      ssid = uuid; // 直接以 UUID 當作 SSID 使用
      Serial2.println("UUID received");
      Serial.println("    UUID = [" + uuid + "] (length=" + String(uuid.length()) + ")");
      Serial.println("    SSID set from UUID");
    }
    else if (line.startsWith("PASS:")) {
      password = line.substring(5);
      Serial2.println("Password received");
      Serial.println("    PASS = [" + password + "] (length=" + String(password.length()) + ")");
    } 
    else if (line.startsWith("URL:")) {
      url = line.substring(4);
      Serial2.println("URL received");
      Serial.println("    URL = " + url);
    } 
    else if (line.startsWith("BODY:")) {
      body = line.substring(5);
      Serial2.println("Body received");
      Serial.println("    BODY = " + body);
    } 
    // 兼容：部分主控可能使用 ESP8266 AT 指令風格（如 AT+CWJAP_CUR, AT+CIPSTATUS）
    // 簡易解析，將 AT 指令映射到本程式的行為
    else if (line.startsWith("AT+CWJAP_CUR=")) {
      // 格式：AT+CWJAP_CUR="ssid","password"
      // 解析雙引號中的 SSID 與 PASSWORD
      int firstQuote = line.indexOf('"');
      int secondQuote = line.indexOf('"', firstQuote + 1);
      int thirdQuote = line.indexOf('"', secondQuote + 1);
      int fourthQuote = line.indexOf('"', thirdQuote + 1);
      if (firstQuote > 0 && secondQuote > firstQuote && thirdQuote > secondQuote && fourthQuote > thirdQuote) {
        ssid = line.substring(firstQuote + 1, secondQuote);
        password = line.substring(thirdQuote + 1, fourthQuote);
        Serial.println("[AT] CWJAP_CUR parsed -> SSID:" + ssid + " PASS:" + password);
        Serial2.println("OK");
        // 立即嘗試連線
        testWiFi();
      } else {
        Serial2.println("ERROR");
      }
    }
    else if (line.startsWith("AT+CIPSTATUS")) {
      int st = WiFi.status();
      String ip = WiFi.localIP().toString();
      
      // 立即回傳所有狀態資訊，不加延遲
      Serial2.println("[AT] CIPSTATUS -> status=" + String(st));
      Serial2.println("STATUS:" + String(st));
      Serial2.println("IP:" + ip);
      Serial2.println("OK");
      Serial2.flush();  // 確保資料發送完成
      
      // 調試輸出到 USB Serial
      Serial.println("[AT] CIPSTATUS -> status=" + String(st));
      Serial.println("    Sent to Mega2560: status=" + String(st) + ", IP=" + ip);
    }
    else if (line == "WIFI_TEST") {
      Serial2.println("Testing WiFi");
      Serial.println(">>> WiFi Test Started");
      Serial.println("    SSID: " + ssid);
      Serial.println("    PASS: " + password);
      testWiFi();
    }
    else if (line == "SEND") {
      Serial2.println("Starting upload");
      Serial.println(">>> Upload Started");
      readyToSend = true;
    }
  }

  if (readyToSend) {
    readyToSend = false;
    
    if (!wifiConnected) {
      Serial.println(">>> WiFi not connected, connecting...");
      if (!connectWiFi()) {
        Serial.println(">>> WiFi connection failed, aborting upload");
        return;
      }
    }
    
    uploadData();
  }
}

void testWiFi() {
  // 同時支援以 UUID 提供 SSID（UUID 會被複製到 ssid）
  if ((ssid == "" && uuid == "") || password == "") {
    Serial2.println("Error: No credentials");
    Serial.println(">>> ERROR: Missing SSID or Password");
    return;
  }
  connectWiFi();
}

bool connectWiFi() {
  Serial2.println("Connecting WiFi...");
  Serial.println(">>> Starting WiFi connection");
  Serial.println("    Target SSID: " + ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  
  WiFi.begin(ssid.c_str(), password.c_str());
  Serial.println(">>> WiFi.begin() called");
  
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 20) {
    delay(500);
    Serial2.print(".");
    
    int status = WiFi.status();
    Serial.print(".");
    Serial.print(" [" + String(timeout) + "] Status=" + String(status));
    
    switch(status) {
      case WL_IDLE_STATUS:
        Serial.println(" (Idle)");
        break;
      case WL_NO_SSID_AVAIL:
        Serial.println(" (SSID not found!)");
        break;
      case WL_CONNECT_FAILED:
        Serial.println(" (Connection failed!)");
        break;
      case WL_DISCONNECTED:
        Serial.println(" (Disconnected)");
        break;
      default:
        Serial.println("");
    }
    
    timeout++;
  }
  
  Serial.println("\n>>> Final WiFi Status: " + String(WiFi.status()));
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial2.println("\nWiFi failed!");
    Serial.println(">>> WiFi Connection FAILED");
    Serial.println("    Status Code: " + String(WiFi.status()));
    wifiConnected = false;
    return false;
  }
  
  wifiConnected = true;
  String ip = WiFi.localIP().toString();
  Serial2.println("\nWiFi connected");
  Serial2.println("IP: " + ip);
  Serial.println(">>> WiFi Connection SUCCESS");
  Serial.println("    IP Address: " + ip);
  Serial.println("    RSSI: " + String(WiFi.RSSI()) + " dBm");
  
  return true;
}

void uploadData() {
  Serial.println(">>> Starting HTTPS Upload");
  
  client.setInsecure();
  
  String host = "";
  String path = "/";
  
  String tempUrl = url;
  if (tempUrl.startsWith("https://")) {
    tempUrl.remove(0, 8);
  }
  
  int slash = tempUrl.indexOf('/');
  if (slash > 0) {
    host = tempUrl.substring(0, slash);
    path = tempUrl.substring(slash);
  } else {
    host = tempUrl;
  }

  Serial.println("    Host: " + host);
  Serial.println("    Path: " + path);
  Serial2.println("Connecting to: " + host);
  
  if (!client.connect(host.c_str(), 443)) {
    Serial2.println("Connection failed");
    Serial.println(">>> HTTPS Connection FAILED");
    return;
  }

  Serial.println(">>> HTTPS Connected, sending POST request");

  client.println("POST " + path + " HTTP/1.1");
  client.println("Host: " + host);
  client.println("User-Agent: ESP32-Mega2560");
  client.println("Content-Type: application/json");
  client.println("Content-Length: " + String(body.length()));
  client.println("Connection: close");
  client.println();
  client.print(body);  // 使用 print 而非 println 避免多餘的換行

  Serial2.println("Request sent");
  Serial.println(">>> Request Sent");
  Serial.println(">>> POST Body: " + body);

  unsigned long start = millis();
  while (client.connected() && millis() - start < 10000) {
    String line = client.readStringUntil('\n');
    if (line == "\r") break;
  }
  
  String response = "";
  while (client.available()) {
    response += (char)client.read();
  }
  
  if (response.length() > 0) {
    Serial2.println("Response: " + response);
    Serial.println(">>> Server Response: " + response);
  } else {
    Serial2.println("No response");
    Serial.println(">>> No Response from Server");
  }

  client.stop();
  Serial2.println("Upload complete");
  Serial.println(">>> Upload Complete\n");
}
