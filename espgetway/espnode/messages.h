#pragma once
#include <Arduino.h>

// ====================== MESSAGE TYPES (MUST MATCH GATEWAY)
// ======================
enum MsgType : uint8_t { MSG_TELEMETRY = 1, MSG_STAT = 2, MSG_CMD = 3 };

#pragma pack(push, 1)
typedef struct {
  uint8_t type; // MsgType
  char dev[10];
  float temp;
  uint32_t ts;    // seconds
  uint8_t status; // 0/1 (relay/cooler)
} TelemetryMsg;

typedef struct {
  uint8_t type; // MSG_STAT
  char dev[10];
  uint32_t ts;
  uint8_t cooler_on; // 0/1
  uint8_t alarm_on;  // 0/1
} StatMsg;

typedef struct {
  uint8_t type; // MSG_CMD
  char cmd[32]; // "TURN_ON_BACKUP_COOLER" / "RESET_ALARM"
} CmdMsg;
#pragma pack(pop)
