# 物聯網實習教學範例（ESP8266 / ESP32 / Arduino）

物聯網實習課程的教學範例程式，涵蓋 ESP8266、ESP32、Arduino Mega2560 等常見物聯網開發板的基礎操作。

## `ESP2880Code/`：ESP8266 基礎測試

| 資料夾 | 說明 |
| --- | --- |
| ConnectWPA | 連接 WPA 加密的 WiFi 網路 |
| FixBoadRatTest | 固定鮑率（Baud Rate）測試 |
| MutiBoadrateTest1 | 多組鮑率切換測試 |

## `ESP8266Test/`

ESP8266 基礎測試程式。

## `Board2Web/`：開發板連網範例

| 檔案 | 說明 |
| --- | --- |
| 20201122-2-mega2560/ | Arduino Mega2560 版本，含 config.h 設定檔 |
| esp32/sketch_dec9f.ino | ESP32 版本 |
| google_apps_script.gs | 搭配的 Google Apps Script 後端程式 |

## `OLED_DEMO/`：OLED 顯示與感測器整合

使用 U8g2 函式庫驅動 OLED 顯示螢幕，搭配 DHT 溫溼度感測器，示範感測資料的顯示與遠端讀取；`Weather/` 為串接氣象資料的延伸範例。

## `ESP32_20251021/`：ESP32 / Mega2560 練習片段

課堂練習當下的程式片段存檔。

## `Dashboard/`：儀表板版面設定

`factory_monitor_dark.json` 為 Looker Studio（原 Google Data Studio）儀表板的版面設定範例，示範感測器資料串到雲端儀表板的最後一步呈現方式；完整的「感測器→MCU→Google表單→試算表→儀表板」資料串接邏輯，詳見教學網頁底部說明。

## 環境需求

Arduino IDE，需安裝對應開發板的板卡管理員（ESP8266/ESP32）與函式庫（U8g2、TroykaDHT 等，依各程式內 `#include` 為準）。
