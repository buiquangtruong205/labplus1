#pragma once
#include <WiFi.h>
#include <WiFiManager.h>

// ====================== WIFI MANAGER (NODE) ======================
void connectWiFiDynamic() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  WiFiManager wm;
  wm.setDebugOutput(true);
  wm.setConfigPortalTimeout(180);
  wm.setConnectTimeout(20);

  Serial.println("[WIFI] Node autoConnect (fail -> portal)");
  bool ok = wm.autoConnect("NODE_Config_AP");

  if (ok) {
    Serial.print("[WIFI] Connected SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("[WIFI] IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println(
        "[WIFI] Not connected (portal timeout). Continue ESPNOW only.");
  }
}

// ====================== PREPARE FOR ESPNOW ======================
void prepareForEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(false);

  // Giảm TX power để giảm nhiễu trong môi trường đông sóng
  WiFi.setTxPower(WIFI_POWER_15dBm);

  delay(200);

  Serial.print("[NODE] STA MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.println("[NODE] TX Power set to 15dBm for noise reduction");
}
