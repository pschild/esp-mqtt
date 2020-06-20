#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <MqttHandler.h>
#include <OTAUpdateHandler.h>

#ifndef WIFI_SSID
  #error "Missing WIFI_SSID"
#endif

#ifndef WIFI_PASSWORD
  #error "Missing WIFI_PASSWORD"
#endif

#ifndef VERSION
  #error "Missing VERSION"
#endif

void ping();
void onFooBar(char* payload);
void onMqttConnected();

MqttHandler mqttHandler("192.168.178.28", String("ESP_") + String(ESP.getChipId()));
OTAUpdateHandler updateHandler("192.168.178.28:9042", VERSION);
Ticker pingTimer(ping, 60 * 1000);

void connectToWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long wifiConnectStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (millis() - wifiConnectStart > 5000) {
      return;
    }
  }
}

void setup() {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);

  connectToWifi();

  mqttHandler.setOnConnectedCallback(onMqttConnected);
  mqttHandler.setup();

  pingTimer.start();
}

void loop() {
  mqttHandler.loop();
  updateHandler.loop();

  pingTimer.update();
}

void ping() {
  mqttHandler.publish("/devices/nodemcu/version", VERSION);
}

void onFooBar(char* payload) {
  if (strcmp(payload, "on") == 0) {
    digitalWrite(LED_BUILTIN, LOW);
  } else if (strcmp(payload, "off") == 0) {
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void onMqttConnected() {
  mqttHandler.subscribe("/foo/bar", onFooBar);
}
