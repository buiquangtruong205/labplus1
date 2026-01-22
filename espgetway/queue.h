#pragma once
#include <Arduino.h>

// ====================== STORE & FORWARD QUEUE (Circular RAM)
// ======================
typedef struct {
  char dev[10];
  float temp;
  uint32_t ts;
  uint8_t st;
} Sample;

const int BUF_SIZE = 240; // > 60 mẫu/5 phút (5s/lần) là dư
Sample q[BUF_SIZE];
int qHead = 0, qTail = 0, qCount = 0;

bool qPush(const Sample &s) {
  if (qCount >= BUF_SIZE)
    return false;
  q[qTail] = s;
  qTail = (qTail + 1) % BUF_SIZE;
  qCount++;
  return true;
}

bool qPop(Sample &out) {
  if (qCount <= 0)
    return false;
  out = q[qHead];
  qHead = (qHead + 1) % BUF_SIZE;
  qCount--;
  return true;
}

int qGetCount() { return qCount; }
