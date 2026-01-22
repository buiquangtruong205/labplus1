#pragma once
#include <Arduino.h>

// ====================== MESSAGE TYPES (MUST MATCH NODE) ======================
enum MsgType : uint8_t { MSG_TELEMETRY = 1, MSG_STAT = 2, MSG_CMD = 3 };

#pragma pack(push, 1)
typedef struct {
  uint8_t type;
  char dev[10];
  float temp;
  uint32_t ts;
  uint8_t status;
} TelemetryMsg;

typedef struct {
  uint8_t type;
  char dev[10];
  uint32_t ts;
  uint8_t cooler_on;
  uint8_t alarm_on;
} StatMsg;

typedef struct {
  uint8_t type;
  char cmd[32];
} CmdMsg;
#pragma pack(pop)
