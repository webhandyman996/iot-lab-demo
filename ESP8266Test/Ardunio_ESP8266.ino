/*
 * ESP8266 Blink
 * 這支程式會讓 ESP8266 模組上內建的藍色 LED 閃爍。
 * 內建 LED 通常連接到 GPIO2。
 */

// LED_BUILTIN 在 NodeMCU 板子定義中通常對應到 GPIO2
#define LED_PIN LED_BUILTIN

void setup() {
  // 初始化序列埠，用於除錯訊息，鮑率設為 115200
  Serial.begin(115200);
  Serial.println("\nESP8266 Blink sketch started!");

  // 將 LED_PIN 設定為輸出模式
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_PIN, LOW);   // 點亮 LED (ESP8266 的內建 LED 通常是低電位觸發)
  Serial.println("LED ON");
  delay(1000);                  // 等待 1 秒

  digitalWrite(LED_PIN, HIGH);  // 熄滅 LED
  Serial.println("LED OFF");
  delay(1000);                  // 等待 1 秒
}