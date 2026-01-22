#pragma once
#include "config.h"
#include <Arduino.h>
#include <ESP32Servo.h>


// ====================== HARDWARE STATE ======================
Servo servoMotor;
bool coolerOn = false;
bool alarmLatched = false; // latch alarm theo đề (tắt bằng RESET_ALARM)

// ====================== APPLY OUTPUTS ======================
void applyOutputs() {
  // Còi theo alarmLatched
  digitalWrite(BUZZER_PIN, alarmLatched ? HIGH : LOW);

  // Relay + Servo theo coolerOn (bật máy lạnh dự phòng)
  digitalWrite(RELAY_PIN, coolerOn ? HIGH : LOW);
  servoMotor.write(coolerOn ? 90 : 0);
}

// ====================== INIT HARDWARE ======================
void initHardware() {
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RELAY_PIN, LOW);

  servoMotor.attach(SERVO_PIN);
  servoMotor.write(0);
}

// ====================== CONTROL FUNCTIONS ======================
void turnOnCooler() {
  coolerOn = true;
  applyOutputs();
}

void resetAlarm() {
  alarmLatched = false;
  applyOutputs();
}

void triggerLocalAlarm() {
  alarmLatched = true;
  Serial.println("[ALARM] Local alarm ON (temp high + gateway lost)");
  applyOutputs();
}

bool isAlarmOn() { return alarmLatched; }

bool isCoolerOn() { return coolerOn; }
