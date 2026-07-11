// 用於操作 DHT 系列感測器的函式庫
#include <TroykaDHT.h>
// 建立 DHT 類別的物件
// 傳入感測器所連接的腳位號碼以及感測器型號
// 感測器型號: DHT11, DHT21, DHT22
DHT dht(4, DHT11);

void setup()
{
  // 開啟序列埠以監控程式的執行狀況
  Serial.begin(9600);
  dht.begin();
}

void loop()
{
  // 從感測器讀取資料
  dht.read();
  // 檢查資料的狀態
  switch(dht.getState()) {
    // 一切正常
    case DHT_OK:
      // 輸出濕度與溫度的數值
      Serial.print("Temperature = ");
      Serial.print(dht.getTemperatureC());
      Serial.println(" C \t");
      Serial.print("Temperature = ");
      Serial.print(dht.getTemperatureK());
      Serial.println(" K \t");
      Serial.print("Temperature = ");
      Serial.print(dht.getTemperatureF());
      Serial.println(" F \t");
      Serial.print("Humidity = ");
      Serial.print(dht.getHumidity());
      Serial.println(" %");
      break;
    // 校驗碼錯誤
    case DHT_ERROR_CHECKSUM:
      Serial.println("Checksum error");
      break;
    // 等待時間超過
    case DHT_ERROR_TIMEOUT:
      Serial.println("Time out error");
      break;
    // 沒有資料，感測器未回應或未連接
    case DHT_ERROR_NO_REPLY:
      Serial.println("Sensor not connected");
      break;
  }
  
  // 等待兩秒
  delay(2000);
}