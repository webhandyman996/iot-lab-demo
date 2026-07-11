/**
 * Time / Device / T / H logger with API_KEY
 * - Sheet tab: "T/H"
 * - Col A: Time (台灣時區)
 * - Col B: Device ID
 * - Col C: Temperature (°C)
 * - Col D: Humidity (%)
 *
 * Script Properties:
 *   API_KEY = YOUR_API_KEY
 *
 * 設定方式：
 * 1. 專案設定 > Script Properties > 新增屬性
 *    - 屬性: API_KEY
 *    - 值: YOUR_API_KEY
 *
 * Deploy as Web App:
 *   Execute as: Me
 *   Access: Anyone (或 Anyone with Google account)
 *   After changes: New deployment -> Deploy
 */

const SHEET_NAME = "T/H";
const TIMEZONE = "Asia/Taipei";

function doGet(e) {
  const TAG = "v2025-12-28-MEGA2560"; // ← 版本標記

  // 測試用：GET也可以寫入資料
  if (e && e.parameter && (e.parameter.temperature || e.parameter.T)) {
    const expectedKey = PropertiesService.getScriptProperties().getProperty("API_KEY");
    const key = e.parameter.key || e.parameter.api_key || "";
    
    if (expectedKey && key !== expectedKey) {
      return json_({ ok: false, tag: TAG, error: "Invalid API key" });
    }

    const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName(SHEET_NAME);
    if (!sheet) {
      return json_({ ok: false, tag: TAG, error: "Sheet '" + SHEET_NAME + "' not found" });
    }

    const dt = e.parameter.time ? new Date(e.parameter.time) : new Date();
    const deviceId = e.parameter.device_id || "UNKNOWN";
    const T = Number(e.parameter.T || e.parameter.temperature);
    const H = Number(e.parameter.H || e.parameter.humidity);

    sheet.appendRow([dt, deviceId, T, H]);
    return json_({ 
      ok: true, 
      tag: TAG, 
      written: { 
        time: Utilities.formatDate(dt, TIMEZONE, "yyyy-MM-dd HH:mm:ss"),
        device_id: deviceId,
        temperature: T, 
        humidity: H 
      } 
    });
  }

  // 沒參數：返回使用說明
  return json_({ 
    ok: true, 
    tag: TAG, 
    message: "Mega2560 T/H Logger API",
    usage: "POST with params: api_key, device_id, temperature, humidity",
    note: "GET also supported for testing"
  });
}

function doPost(e) {
  const TAG = "v2025-12-28-MEGA2560";
  
  try {
    // 調試：記錄原始請求
    if (!e) {
      return json_({ ok: false, tag: TAG, error: "Empty request object" });
    }
    
    // 解析POST請求（支援 application/x-www-form-urlencoded 和 application/json）
    const { payload, contentType } = parseRequest_(e);
    
    // 調試：記錄解析後的payload
    Logger.log("ContentType: " + contentType);
    Logger.log("Payload: " + JSON.stringify(payload));
    
    // 驗證 API Key
    const expectedKey = PropertiesService.getScriptProperties().getProperty("API_KEY");
    if (!expectedKey) {
      return json_({ ok: false, tag: TAG, error: "Server missing API_KEY in Script Properties" });
    }

    const key = String(payload.key || payload.api_key || "");
    if (key !== expectedKey) {
      return json_({ 
        ok: false, 
        tag: TAG, 
        error: "Invalid API key",
        received_key: key ? "***" + key.slice(-4) : "none",
        debug_payload_keys: Object.keys(payload)
      });
    }

    // 取得資料欄位（支援多種命名方式）
    const deviceId = String(payload.device_id || payload.deviceId || payload.device || "UNKNOWN");
    const tRaw = payload.temperature || payload.T || payload.t || payload.temp || "";
    const hRaw = payload.humidity || payload.H || payload.h || payload.hum || "";

    Logger.log("Device ID: " + deviceId);
    Logger.log("Temperature raw: " + tRaw);
    Logger.log("Humidity raw: " + hRaw);

    if (tRaw === "" || hRaw === "") {
      return json_({ 
        ok: false, 
        tag: TAG, 
        error: "Missing temperature or humidity",
        received_params: Object.keys(payload),
        payload_values: payload
      });
    }

    const T = Number(tRaw);
    const H = Number(hRaw);
    
    if (!Number.isFinite(T) || !Number.isFinite(H)) {
      return json_({ 
        ok: false, 
        tag: TAG, 
        error: "Temperature and Humidity must be valid numbers",
        received: { temperature: tRaw, humidity: hRaw }
      });
    }

    // 時間處理（如果沒提供就用伺服器時間）
    const timeRaw = String(payload.time || payload.Time || payload.timestamp || "");
    const dt = timeRaw ? new Date(timeRaw) : new Date();
    
    Logger.log("Time raw: " + timeRaw);
    Logger.log("Date object: " + dt);
    
    if (isNaN(dt.getTime())) {
      return json_({ 
        ok: false, 
        tag: TAG, 
        error: "Invalid time format",
        hint: "Use ISO format like: 2025-12-28T15:30:00",
        received: timeRaw
      });
    }

    // 寫入試算表
    const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName(SHEET_NAME);
    if (!sheet) {
      return json_({ 
        ok: false, 
        tag: TAG, 
        error: "Sheet '" + SHEET_NAME + "' not found. Please create it first."
      });
    }

    // 寫入資料：時間、裝置ID、溫度、濕度
    sheet.appendRow([dt, deviceId, T, H]);

    // 回傳成功訊息
    return json_({
      ok: true,
      tag: TAG,
      contentType: contentType,
      written: {
        time_iso: dt.toISOString(),
        time_tw: Utilities.formatDate(dt, TIMEZONE, "yyyy-MM-dd HH:mm:ss"),
        device_id: deviceId,
        temperature: T,
        humidity: H
      }
    });
    
  } catch (err) {
    return json_({ 
      ok: false, 
      tag: TAG, 
      error: String(err),
      stack: err.stack 
    });
  }
}

/** ====== 請求解析 ====== */
function parseRequest_(e) {
  let contentType = "unknown";
  let payload = {};

  try {
    // 檢查 POST 資料
    if (e && e.postData) {
      contentType = String(e.postData.type || "").toLowerCase();
      
      Logger.log("postData.type: " + contentType);
      Logger.log("postData.contents: " + (e.postData.contents || "empty"));
      
      if (contentType.indexOf("application/json") !== -1) {
        // JSON 格式（ESP32 使用此格式）
        try {
          const contents = e.postData.contents || "{}";
          payload = JSON.parse(contents);
          Logger.log("Parsed JSON payload successfully");
        } catch (err) {
          Logger.log("JSON parse error: " + err);
          // 如果 JSON 解析失敗，嘗試從 parameter 讀取
          payload = e.parameter || {};
        }
      } else {
        // application/x-www-form-urlencoded 格式 (備用格式)
        payload = e.parameter || {};
        Logger.log("Using form-urlencoded payload");
      }
    } else if (e && e.parameter) {
      // GET 請求或沒有 postData
      payload = e.parameter || {};
      contentType = "query-string";
      Logger.log("Using query-string payload");
    } else {
      Logger.log("WARNING: No parameter or postData found");
    }
    
    Logger.log("Final payload keys: " + Object.keys(payload).join(", "));
  } catch (err) {
    Logger.log("parseRequest_ error: " + err);
    throw new Error("Failed to parse request: " + err);
  }

  return { payload, contentType };
}

/** ====== JSON 回應 ====== */
function json_(obj) {
  return ContentService
    .createTextOutput(JSON.stringify(obj, null, 2))
    .setMimeType(ContentService.MimeType.JSON);
}

/** ====== 本地測試函數 ====== */
function testAppendLocal() {
  const sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName(SHEET_NAME);
  if (!sheet) {
    Logger.log("Sheet '" + SHEET_NAME + "' not found!");
    return;
  }
  
  const now = new Date();
  sheet.appendRow([now, "TEST_DEVICE", 25.6, 60.2]);
  Logger.log("Test data appended: " + Utilities.formatDate(now, TIMEZONE, "yyyy-MM-dd HH:mm:ss"));
}

/**
 * 測試 POST 請求模擬（在 Apps Script 編輯器中執行）
 */
function testPostRequest() {
  const mockEvent = {
    postData: {
      type: "application/x-www-form-urlencoded",
      contents: ""
    },
    parameter: {
      api_key: "YOUR_API_KEY",
      device_id: "MEGA2560_001",
      temperature: "25.5",
      humidity: "60.0"
    }
  };
  
  const result = doPost(mockEvent);
  Logger.log(result.getContent());
}

/**
 * 初始化工作表（第一次使用時執行）
 * 建立工作表並加入標題列
 */
function initializeSheet() {
  const ss = SpreadsheetApp.getActiveSpreadsheet();
  let sheet = ss.getSheetByName(SHEET_NAME);
  
  if (!sheet) {
    sheet = ss.insertSheet(SHEET_NAME);
    Logger.log("Created new sheet: " + SHEET_NAME);
  }
  
  // 檢查是否已有標題列，或標題不正確需要更新
  const firstRow = sheet.getRange(1, 1, 1, 4).getValues()[0];
  const needsHeader = sheet.getLastRow() === 0 || 
                      firstRow[0] !== "Time" || 
                      firstRow[1] !== "ID" || 
                      firstRow[2] !== "T" || 
                      firstRow[3] !== "H";
  
  if (needsHeader) {
    // 清除第一列並重新設定標題
    sheet.getRange(1, 1, 1, 4).clearContent();
    sheet.getRange(1, 1, 1, 4).setValues([["Time", "ID", "T", "H"]]);
    sheet.getRange(1, 1, 1, 4).setFontWeight("bold");
    sheet.setFrozenRows(1);
    
    // 設定欄寬
    sheet.setColumnWidth(1, 180); // Time
    sheet.setColumnWidth(2, 120); // ID
    sheet.setColumnWidth(3, 80);  // T
    sheet.setColumnWidth(4, 80);  // H
    
    Logger.log("Updated header row to: Time | ID | T | H");
  }
  
  Logger.log("Sheet initialization complete!");
}
