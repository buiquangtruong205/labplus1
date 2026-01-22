#pragma once
#include <Arduino.h>

// ====================== CONFIG ======================
#define DEV_ID "NODE_01"
#define TEMP_ON_ALARM 8.0f  // >8C -> nguy hiểm
#define TEMP_OFF_ALARM 7.5f // hysteresis (tắt khi <7.5C nếu có reset)
#define SEND_PERIOD_MS 5000
#define GW_LOST_MS                                                             \
  15000 // quá thời gian không ACK/không liên lạc -> coi như mất gateway

// Pins (hãy chỉnh theo board bạn)
static const int POT_PIN = 6; // ADC (nếu sai ADC, đổi sang chân ADC thật)
static const int BUZZER_PIN = 5;
static const int RELAY_PIN = 4;
static const int SERVO_PIN = 7;

// Gateway ESP8266 STA MAC (điền đúng MAC của Gateway)
static uint8_t gatewayMac[] = {0x40, 0x91, 0x51, 0x7F, 0x9B, 0xFD};

// Node scan AP phụ để lấy channel (Gateway phát)
static const char *GW_CH_SSID = "GW_CH";
