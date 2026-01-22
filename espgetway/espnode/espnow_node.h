#pragma once
#include "config.h"
#include "hardware.h"
#include "messages.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h> // Cho esp_wifi_set_channel

// ====================== CONFIG RETRY ======================
#define ESPNOW_RETRY_COUNT 3
#define ESPNOW_RETRY_DELAY_MS 50
#define ESPNOW_ACK_TIMEOUT_MS 100

// ====================== STATE ======================
volatile bool lastSendOk = false;
uint32_t lastOkSendMs = 0;
bool espNowInitialized = false; // Flag để track trạng thái ESP-NOW

// ====================== HELPER: WAIT FOR ACK ======================
bool waitForAck(uint32_t timeoutMs) {
  uint32_t waitStart = millis();
  while (!lastSendOk && (millis() - waitStart < timeoutMs)) {
    yield(); // Cho ESP xử lý callback
    delayMicroseconds(100);
  }
  return lastSendOk;
}

// ====================== SEND WITH RETRY ======================
esp_err_t sendWithRetry(uint8_t *data, size_t len) {
  for (int retry = 0; retry < ESPNOW_RETRY_COUNT; retry++) {
    lastSendOk = false;
    esp_err_t r = esp_now_send(gatewayMac, data, len);

    if (r != ESP_OK) {
      Serial.printf("[ESPNOW] send err=%d, retry %d/%d\n", r, retry + 1,
                    ESPNOW_RETRY_COUNT);
      delay(ESPNOW_RETRY_DELAY_MS * (retry + 1)); // Exponential backoff
      continue;
    }

    // Đợi ACK callback
    if (waitForAck(ESPNOW_ACK_TIMEOUT_MS)) {
      return ESP_OK;
    }

    Serial.printf("[ESPNOW] no ACK, retry %d/%d\n", retry + 1,
                  ESPNOW_RETRY_COUNT);
    delay(ESPNOW_RETRY_DELAY_MS * (retry + 1));
  }
  return ESP_FAIL;
}

// ====================== SEND STAT ======================
void sendStat() {
  StatMsg st{};
  st.type = MSG_STAT;
  strncpy(st.dev, DEV_ID, sizeof(st.dev) - 1);
  st.ts = millis() / 1000;
  st.cooler_on = isCoolerOn() ? 1 : 0;
  st.alarm_on = isAlarmOn() ? 1 : 0;

  sendWithRetry((uint8_t *)&st, sizeof(st));
}

// ====================== SEND TELEMETRY ======================
esp_err_t sendTelemetry(float temp) {
  TelemetryMsg t{};
  t.type = MSG_TELEMETRY;
  strncpy(t.dev, DEV_ID, sizeof(t.dev) - 1);
  t.temp = temp;
  t.ts = millis() / 1000;
  t.status = isCoolerOn() ? 1 : 0;

  return sendWithRetry((uint8_t *)&t, sizeof(t));
}

// ====================== ESPNOW CALLBACKS ======================
// Send callback (ESP32 core mới)
void onDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
  lastSendOk = (status == ESP_NOW_SEND_SUCCESS);
  if (lastSendOk)
    lastOkSendMs = millis();
}

// Receive callback (ESP32 core 3+ dạng esp_now_recv_info*)
void onDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
  if (len <= 0)
    return;

  uint8_t type = data[0];
  if (type != MSG_CMD)
    return;
  if (len < (int)sizeof(CmdMsg))
    return;

  CmdMsg cmd{};
  memcpy(&cmd, data, sizeof(cmd));

  Serial.print("[CMD] ");
  Serial.println(cmd.cmd);

  if (strcmp(cmd.cmd, "TURN_ON_BACKUP_COOLER") == 0) {
    turnOnCooler();
    sendStat(); // báo lại STAT ON
  } else if (strcmp(cmd.cmd, "RESET_ALARM") == 0) {
    resetAlarm();
    sendStat(); // báo lại alarm off
  }
}

// ====================== FIND GATEWAY CHANNEL ======================
int findGwChannel() {
  int n = WiFi.scanNetworks(false, true);
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == GW_CH_SSID)
      return WiFi.channel(i);
  }
  return -1;
}

// ====================== CHECK GATEWAY LOST ======================
bool isGatewayLost() { return (millis() - lastOkSendMs > GW_LOST_MS); }

// ====================== STORED CHANNEL ======================
static int fixedChannel = 0;

// ====================== INIT ESP-NOW ======================
bool initEspNow() {
  // Scan channel từ AP phụ GW_CH với RETRY
  Serial.println("[NODE] Scan GW_CH to get channel...");

  int ch = -1;
  for (int retry = 0; retry < 5; retry++) { // Thử 5 lần
    ch = findGwChannel();
    if (ch > 0)
      break;

    Serial.print("[NODE] GW_CH not found, retry ");
    Serial.print(retry + 1);
    Serial.println("/5...");
    delay(2000); // Đợi 2 giây rồi thử lại
  }

  Serial.print("[NODE] GW_CH channel = ");
  Serial.println(ch);

  if (ch < 1) {
    Serial.println("[NODE] GW_CH not found after 5 retries. Reset Node after "
                   "Gateway is ready.");
    espNowInitialized = false;
    return false;
  }

  // LƯU VÀ FIX CHANNEL
  fixedChannel = ch;

  // Chuyển sang channel cố định
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  Serial.print("[NODE] Channel FIXED at: ");
  Serial.println(fixedChannel);

  // Init ESPNOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW] init FAIL");
    return false;
  }
  Serial.println("[ESPNOW] init OK");

  esp_now_register_send_cb(onDataSent);
  esp_now_register_recv_cb(onDataRecv);

  // Add peer Gateway với channel cố định
  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, gatewayMac, 6);
  peer.channel = fixedChannel; // Dùng channel đã fix
  peer.encrypt = false;

  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("[ESPNOW] add peer FAIL");
    return false;
  }
  Serial.println("[ESPNOW] peer OK with FIXED channel");

  espNowInitialized = true; // Đánh dấu đã init thành công
  return true;
}
