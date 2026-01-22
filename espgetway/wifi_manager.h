#pragma once
#include <ESP8266WiFi.h>
#include <WiFiManager.h>

// AP phụ để Node scan channel
static const char *GW_CH_SSID = "GW_CH";

// Fixed channel sau khi kết nối
static int fixedChannel = 0;

// ====================== LOG ======================
void printWiFiStatus() {
  Serial.print("[WIFI] SSID: ");
  Serial.println(WiFi.SSID());
  Serial.print("[WIFI] IP  : ");
  Serial.println(WiFi.localIP());
  Serial.print("[WIFI] CH  : ");
  Serial.println(WiFi.channel());
}

// ====================== GW_CH AP (same channel as STA) ======================
void startGwChAp() {
  int ch = WiFi.channel();
  if (ch < 1)
    ch = 1;

  // Lưu channel cố định
  fixedChannel = ch;

  WiFi.mode(WIFI_AP_STA);

  // Tạo AP phụ với channel cố định
  bool ok = WiFi.softAP(GW_CH_SSID, "", ch, 0, 4); // hidden=0, max_conn=4

  Serial.println(ok ? "[GW_CH] softAP OK" : "[GW_CH] softAP FAIL");
  Serial.print("[GW_CH] FIXED Channel: ");
  Serial.println(ch);
  Serial.print("[GW_CH] AP IP: ");
  Serial.println(WiFi.softAPIP());
  Serial.println("[GW_CH] Channel is now FIXED for ESP-NOW stability");
}

// ====================== WIFI (Gateway) ======================
void connectWifiGateway() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setOutputPower(17); // Tăng TX power cho ESP-NOW range tốt hơn

  // Tắt auto-reconnect để tránh channel hopping
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true); // Lưu credentials vào flash

  WiFiManager wm;
  wm.setDebugOutput(true);
  wm.setConnectTimeout(20);
  wm.setConfigPortalTimeout(180);

  Serial.println("[WIFI] autoConnect (portal if needed)");
  bool ok = wm.autoConnect("GW_Config_AP");

  if (ok) {
    Serial.println("[WIFI] Connected!");
    printWiFiStatus();

    // Lưu channel cố định
    fixedChannel = WiFi.channel();
    Serial.print("[WIFI] Channel FIXED at: ");
    Serial.println(fixedChannel);
  } else {
    Serial.println("[WIFI] Not connected (portal timeout).");
  }
}
