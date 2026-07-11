#include <Arduino.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <TroykaDHT.h>

// 建立 DHT 類別的物件 (腳位 4, 型號 DHT11)
DHT dht(4, DHT11);

// 使用 U8g2 函式庫控制 SSD1306 OLED (128x64, I2C)
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(
  U8G2_R0,
  U8X8_PIN_NONE
);

void setup() {
  Serial.begin(9600);
  dht.begin();
  u8g2.begin();   // 初始化 OLED
}

void loop() {
  dht.read();

  u8g2.clearBuffer();  // 清除畫面緩衝區

  switch (dht.getState()) {
    case DHT_OK: {
      float tempC = dht.getTemperatureC();
      float hum = dht.getHumidity();

      // 在序列埠輸出
      Serial.print("Temperature = ");
      Serial.print(tempC);
      Serial.println(" C");
      Serial.print("Humidity = ");
      Serial.print(hum);
      Serial.println(" %");

      // 在 OLED 顯示
      u8g2.setFont(u8g2_font_ncenB08_tr); // 設定字型
      u8g2.drawStr(0, 15, "DHT11 Sensor");
      
      char buf[20];
      sprintf(buf, "Temp: %.1f C", tempC);
      u8g2.drawStr(0, 35, buf);

      sprintf(buf, "Hum : %.1f %%", hum);
      u8g2.drawStr(0, 55, buf);

      break;
    }
    case DHT_ERROR_CHECKSUM:
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(0, 30, "Checksum error");
      Serial.println("Checksum error");
      break;

    case DHT_ERROR_TIMEOUT:
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(0, 30, "Timeout error");
      Serial.println("Time out error");
      break;

    case DHT_ERROR_NO_REPLY:
      u8g2.setFont(u8g2_font_ncenB08_tr);
      u8g2.drawStr(0, 30, "Sensor not connected");
      Serial.println("Sensor not connected");
      break;
  }

  u8g2.sendBuffer();   // 把緩衝區內容送到 OLED 顯示
  delay(2000);         // 每 2 秒更新一次
}
