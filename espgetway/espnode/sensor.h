#pragma once
#include "config.h"
#include <Arduino.h>

// ====================== MEDIAN FILTER (Lọc nhiễu) ======================
float median5() {
  float s[5];
  for (int i = 0; i < 5; i++) {
    int raw = analogRead(POT_PIN);
    // giả lập 0..20.0C
    s[i] = map(raw, 0, 4095, 0, 200) / 10.0f;
    delayMicroseconds(500); // 0.5ms giữa các lần đọc ADC
  }
  // Bubble sort để lấy median
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 5; j++) {
      if (s[i] > s[j]) {
        float t = s[i];
        s[i] = s[j];
        s[j] = t;
      }
    }
  }
  return s[2]; // Giá trị giữa = median
}
