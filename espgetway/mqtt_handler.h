#pragma once
#include "messages.h"
#include "queue.h"
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>


// ====================== CONFIG ======================
static const char *MQTT_HOST = "broker.emqx.io";
static const int MQTT_PORT = 1883;

static const char *TOPIC_TELE = "vacin/telemetry";
static const char *TOPIC_BATCH = "vacin/telemetry/batch";
static const char *TOPIC_CMD = "vacin/cmd";
static const char *TOPIC_STAT = "vacin/stat";

static const char *GW_ID = "GATEWAY_01";

// ====================== MQTT ======================
WiFiClient espClient;
PubSubClient mqtt(espClient);

// Forward declaration
void onMqttMsg(char *topic, byte *payload, unsigned int length);

// ====================== MQTT CONNECT ======================
void ensureMqtt() {
  if (mqtt.connected())
    return;

  Serial.print("[MQTT] Connecting... ");
  if (mqtt.connect(GW_ID)) {
    Serial.println("OK");
    mqtt.subscribe(TOPIC_CMD);
    Serial.println("[MQTT] Subscribed vacin/cmd");
  } else {
    Serial.print("FAIL rc=");
    Serial.println(mqtt.state());
  }
}

// ====================== FLUSH QUEUE as BATCH 50 ======================
void flushQueue() {
  if (qGetCount() == 0)
    return;
  if (WiFi.status() != WL_CONNECTED)
    return;
  if (!mqtt.connected())
    return;

  while (qGetCount() > 0) {
    int n = (qGetCount() >= 50) ? 50 : qGetCount();

    StaticJsonDocument<8192> doc;
    doc["gw"] = GW_ID;
    JsonArray arr = doc.createNestedArray("data");

    Sample tmp[50];
    int taken = 0;

    for (int i = 0; i < n; i++) {
      if (!qPop(tmp[i]))
        break;
      JsonObject o = arr.createNestedObject();
      o["dev"] = tmp[i].dev;
      o["t"] = tmp[i].temp;
      o["ts"] = tmp[i].ts;
      o["st"] = tmp[i].st;
      taken++;
    }

    char out[8192];
    size_t len = serializeJson(doc, out, sizeof(out));
    if (len == 0)
      return;

    if (mqtt.publish(TOPIC_BATCH, out)) {
      Serial.print("[BATCH] sent ");
      Serial.print(taken);
      Serial.print(" remain=");
      Serial.println(qGetCount());
    } else {
      // publish fail -> restore
      for (int i = 0; i < taken; i++)
        qPush(tmp[i]);
      Serial.println("[BATCH] publish FAIL -> restore queue");
      return;
    }
  }
}

// ====================== HANDLE INCOMING TELEMETRY ======================
void handleTelemetry(const TelemetryMsg &t) {
  // publish realtime nếu có MQTT
  if (WiFi.status() == WL_CONNECTED && mqtt.connected()) {
    char msg[200];
    snprintf(msg, sizeof(msg),
             "{\"dev\":\"%s\",\"t\":%.2f,\"ts\":%lu,\"st\":%d}", t.dev, t.temp,
             (unsigned long)t.ts, t.status);

    if (mqtt.publish(TOPIC_TELE, msg)) {
      Serial.println("[MQTT] realtime OK");
    } else {
      Serial.println("[MQTT] realtime FAIL -> queue");
      Sample s{};
      strncpy(s.dev, t.dev, sizeof(s.dev) - 1);
      s.temp = t.temp;
      s.ts = t.ts;
      s.st = t.status;
      qPush(s);
    }
  } else {
    // mất mạng -> queue
    Sample s{};
    strncpy(s.dev, t.dev, sizeof(s.dev) - 1);
    s.temp = t.temp;
    s.ts = t.ts;
    s.st = t.status;

    if (!qPush(s))
      Serial.println("[QUEUE] FULL -> drop");
    else {
      Serial.print("[QUEUE] saved count=");
      Serial.println(qGetCount());
    }
  }
}

// ====================== PUBLISH STAT ======================
void publishStat(const StatMsg &s) {
  StaticJsonDocument<256> doc;
  doc["dev"] = s.dev;
  doc["ts"] = s.ts;
  doc["cooler"] = s.cooler_on ? "ON" : "OFF";
  doc["alarm"] = s.alarm_on ? "ON" : "OFF";

  char out[256];
  serializeJson(doc, out, sizeof(out));
  mqtt.publish(TOPIC_STAT, out);

  Serial.print("[STAT] cooler=");
  Serial.print(s.cooler_on ? "ON" : "OFF");
  Serial.print(" alarm=");
  Serial.println(s.alarm_on ? "ON" : "OFF");
}

// ====================== INIT MQTT ======================
void initMqtt() {
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(onMqttMsg);
}
