#pragma once
#include "messages.h"
#include "mqtt_handler.h"
#include <ESP8266WiFi.h>
#include <espnow.h>

// Node MAC (ESP32) để forward lệnh (điền đúng MAC của Node)
uint8_t nodeMac[] = {0x9C, 0x13, 0x9E, 0xF2, 0x9A, 0xF8};

// ====================== ESPNOW RECEIVE ======================
void onEspNowRecv(uint8_t *mac, uint8_t *data, uint8_t len) {
  // Log chi tiết để debug
  Serial.printf("[ESP-NOW] Recv %d bytes from %02X:%02X:%02X:%02X:%02X:%02X\n",
                len, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  if (len < 1)
    return;
  uint8_t type = data[0];

  if (type == MSG_TELEMETRY) {
    if (len != sizeof(TelemetryMsg)) {
      Serial.print("[ESP-NOW] len mismatch: ");
      Serial.print(len);
      Serial.print(" vs ");
      Serial.println(sizeof(TelemetryMsg));
      return;
    }
    TelemetryMsg t{};
    memcpy(&t, data, sizeof(t));
    handleTelemetry(t);
  } else if (type == MSG_STAT) {
    if (len != sizeof(StatMsg))
      return;
    StatMsg s{};
    memcpy(&s, data, sizeof(s));
    publishStat(s);
  }
}

// ====================== FORWARD CMD TO NODE ======================
void forwardCmdToNode(const char *cmdStr) {
  CmdMsg c{};
  c.type = MSG_CMD;
  memset(c.cmd, 0, sizeof(c.cmd));
  strncpy(c.cmd, cmdStr, sizeof(c.cmd) - 1);

  uint8_t r = esp_now_send(nodeMac, (uint8_t *)&c, sizeof(c));
  Serial.print("[ESP-NOW] forward cmd -> ");
  Serial.println(r == 0 ? "OK" : "FAIL");
}

// ====================== INIT ESP-NOW ======================
bool initEspNow() {
  Serial.println("[ESP-NOW] init...");
  if (esp_now_init() != 0) {
    Serial.println("[ESP-NOW] init FAIL");
    return false;
  }
  Serial.println("[ESP-NOW] init OK");

  esp_now_set_self_role(ESP_NOW_ROLE_COMBO);
  esp_now_register_recv_cb(onEspNowRecv);

  // add peer Node (channel = 0 để theo current channel của radio)
  uint8_t res = esp_now_add_peer(nodeMac, ESP_NOW_ROLE_COMBO, 0, NULL, 0);
  Serial.print("[ESP-NOW] add peer -> ");
  Serial.println(res == 0 ? "OK" : "FAIL");

  return true;
}
