/*
 * ESP32 Node - Vaccine Cold Chain Monitoring System
 *
 * Chức năng:
 * - Đo nhiệt độ với bộ lọc Median Filter
 * - Gửi dữ liệu đến Gateway qua ESP-NOW
 * - Edge Logic: Bật còi tại chỗ khi temp > 8°C và mất gateway
 * - Nhận lệnh điều khiển từ Gateway (TURN_ON_BACKUP_COOLER, RESET_ALARM)
 */

#include "config.h"
#include "espnow_node.h"
#include "hardware.h"
#include "sensor.h"
#include "wifi_node.h"

// ====================== STATE ======================
uint32_t lastSendMs = 0;
float lastTemp = 0;

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== NODE BOOT ===");

  // 1. Khởi tạo phần cứng
  initHardware();

  // 2. Cấu hình WiFi động (đúng đề: mang đến kho mới có thể config)
  connectWiFiDynamic();

  // 3. Chuẩn bị cho ESP-NOW
  prepareForEspNow();

  // 4. Khởi tạo ESP-NOW
  if (!initEspNow()) {
    Serial.println(
        "[NODE] ESP-NOW init failed. Please reset after Gateway is ready.");
    return;
  }

  // 5. Áp dụng trạng thái ban đầu
  applyOutputs();
  sendStat();
}

// ====================== LOOP ======================
void loop() {
  // Cho ESP xử lý ESP-NOW callbacks trước
  yield();

  // Đo nhiệt độ + lọc nhiễu
  lastTemp = median5();
  uint32_t now = millis();

  // Kiểm tra mất kết nối gateway
  bool gatewayLost = isGatewayLost();

  // Edge logic theo đề: nếu temp > 8°C và mất kết nối gateway -> bật còi tại
  // chỗ
  if (!isAlarmOn() && (lastTemp > TEMP_ON_ALARM) && gatewayLost) {
    triggerLocalAlarm();
    sendStat();
  }

  // Gửi telemetry định kỳ (5s)
  if (now - lastSendMs >= SEND_PERIOD_MS) {
    lastSendMs = now;

    esp_err_t r = sendTelemetry(lastTemp);

    // Log trạng thái
    Serial.print("temp=");
    Serial.print(lastTemp, 2);
    Serial.print("  gatewayLost=");
    Serial.print(gatewayLost ? "YES" : "NO");
    Serial.print("  alarm=");
    Serial.print(isAlarmOn() ? "ON" : "OFF");
    Serial.print("  cooler=");
    Serial.print(isCoolerOn() ? "ON" : "OFF");
    Serial.print("  send=");
    Serial.println((r == ESP_OK) ? "OK" : "ERR");
  }
}
