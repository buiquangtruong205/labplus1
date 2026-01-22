/*
 * Đọc địa chỉ MAC của ESP8266/ESP32
 * Upload lên board để lấy MAC address
 * Dùng MAC này điền vào config của Node/Gateway
 */

// ========== TỰ ĐỘNG DETECT BOARD ==========
#if defined(ESP32)
#include <WiFi.h>
#define BOARD_NAME "ESP32"
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#define BOARD_NAME "ESP8266"
#else
#error "Board không được hỗ trợ! Chỉ dùng với ESP8266 hoặc ESP32"
#endif

void setup() {
  Serial.begin(115200);
  delay(1000);

  // Đưa WiFi về chế độ Station để đọc MAC
  WiFi.mode(WIFI_STA);
  delay(100);

  String mac = WiFi.macAddress();

  Serial.println();
  Serial.println("╔═══════════════════════════════════════════════╗");
  Serial.println("║       ĐỌC ĐỊA CHỈ MAC - VACCINE COLD CHAIN    ║");
  Serial.println("╠═══════════════════════════════════════════════╣");
  Serial.print("║  Board: ");
  Serial.print(BOARD_NAME);
  for (int i = 0; i < 37 - strlen(BOARD_NAME); i++)
    Serial.print(" ");
  Serial.println("║");
  Serial.println("╠═══════════════════════════════════════════════╣");
  Serial.print("║  MAC Address: ");
  Serial.print(mac);
  Serial.println("         ║");
  Serial.println("╠═══════════════════════════════════════════════╣");

  // Chuyển sang format C array
  Serial.print("║  C Array: {");
  for (int i = 0; i < 6; i++) {
    String byteStr = mac.substring(i * 3, i * 3 + 2);
    Serial.print("0x");
    Serial.print(byteStr);
    if (i < 5)
      Serial.print(", ");
  }
  Serial.println("} ║");

  Serial.println("╚═══════════════════════════════════════════════╝");
  Serial.println();
  Serial.println(">> Copy dòng C Array vào file config.h của Node/Gateway");

  // Hướng dẫn cụ thể
  Serial.println();
  Serial.println("Nếu đây là ESP32 (Node):");
  Serial.println(
      "  -> Copy vào espgetway/espnow_handler.h dòng: uint8_t nodeMac[]");
  Serial.println();
  Serial.println("Nếu đây là ESP8266 (Gateway):");
  Serial.println(
      "  -> Copy vào espnode/config.h dòng: static uint8_t gatewayMac[]");
}

void loop() {
  // Không cần xử lý gì
  delay(10000);
}