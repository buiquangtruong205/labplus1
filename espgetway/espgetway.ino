/*
 * ESP8266 Gateway - Vaccine Cold Chain Monitoring System
 *
 * Chức năng:
 * - Nhận dữ liệu từ Node qua ESP-NOW
 * - Gửi lên MQTT Server (realtime + batch)
 * - Store & Forward khi mất mạng
 * - Chuyển tiếp lệnh từ Server đến Node
 */

#include "espnow_handler.h"
#include "mqtt_handler.h"
#include "wifi_manager.h"

// ====================== MQTT CMD CALLBACK ======================
void onMqttMsg(char *topic, byte *payload, unsigned int length) {
  String msg;
  msg.reserve(length);
  for (unsigned int i = 0; i < length; i++)
    msg += (char)payload[i];

  Serial.print("[CMD MQTT] ");
  Serial.println(msg);

  if (msg.indexOf("TURN_ON_BACKUP_COOLER") >= 0) {
    forwardCmdToNode("TURN_ON_BACKUP_COOLER");
  } else if (msg.indexOf("RESET_ALARM") >= 0) {
    forwardCmdToNode("RESET_ALARM");
  } else {
    Serial.println("[CMD MQTT] Unknown -> ignore");
  }
}

// ====================== SETUP ======================
void setup() {
  Serial.begin(115200);
  delay(600);
  Serial.println();
  Serial.println("=== ESP8266 GATEWAY BOOT ===");

  // 1. Kết nối WiFi (cấu hình động qua portal)
  connectWifiGateway();

  // 2. Phát AP phụ để Node scan channel
  startGwChAp();

  // 3. Khởi tạo MQTT
  initMqtt();

  // 4. Khởi tạo ESP-NOW
  initEspNow();

  Serial.println("=== READY ===");
}

// ====================== LOOP ======================
void loop() {
  // Cho ESP8266 xử lý ESP-NOW callbacks
  yield();

  if (WiFi.status() == WL_CONNECTED) {
    ensureMqtt();
  }
  mqtt.loop();
  flushQueue(); // Pha 3: có mạng lại -> xả hàng đợi
}
