#include <WiFi.h>
#include <WiFiClientSecure.h>

String ssid, password, url, body;
bool readyToSend = false;
bool wifiConnected = false;

WiFiClientSecure client;

void setup() {
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  delay(500);
  Serial2.println("ESP32 ready");
  
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== ESP32 WiFi Handler (Debug Mode) ===");
  Serial.println("Waiting for commands...\n");
}

void loop() {
  if (Serial2.available()) {
    String line = Serial2.readStringUntil('\n');
    line.trim();
    
    Serial.println("<<< Received: [" + line + "]");

    if (line.startsWith("SSID:")) {
      ssid = line.substring(5);
      Serial2.println("SSID received");
      Serial.println("    SSID = [" + ssid + "] (length=" + String(ssid.length()) + ")");
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
  if (ssid == "" || password == "") {
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
  client.println(body);

  Serial2.println("Request sent");
  Serial.println(">>> Request Sent");

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