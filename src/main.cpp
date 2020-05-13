#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>
#include <PubSubClient.h>

#ifndef WIFI_SSID
  #error "Missing WIFI_SSID"
#endif

#ifndef WIFI_PASSWORD
  #error "Missing WIFI_PASSWORD"
#endif

#ifndef VERSION
  #error "Missing VERSION"
#endif

const char* MQTT_BROKER = "192.168.178.28";

WiFiClient espClient;
PubSubClient mqttClient(espClient);

long lastMsg = 0;
char msg[50];
int value = 0;

unsigned long lastUpdateCheck = 0;
const long UPDATE_INTERVAL = 1 * 60 * 60 * 1000; // 1 hour

unsigned long lastPing = 0;
const long PING_INTERVAL = 60 * 1000; // 1 min

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

void checkForUpdate(bool forceUpdate) {
  unsigned long now = millis();
  if (
    WiFi.status() == WL_CONNECTED
    && (now >= lastUpdateCheck + UPDATE_INTERVAL || forceUpdate == true)
  ) {
    WiFiClient client;
    ESPhttpUpdate.setLedPin(LED_BUILTIN, LOW);
    ESPhttpUpdate.update(client, "http://192.168.178.28:9042/ota", VERSION);
    lastUpdateCheck = now;
  }
}

void sendPing() {
  unsigned long now = millis();
  if (now >= lastPing + PING_INTERVAL) {
    mqttClient.publish("/devices/nodemcu/version", VERSION);
    lastPing = now;
  }
}

void reconnectMqttClient() {
  while (!mqttClient.connected()) {
    Serial.print("Reconnecting...");
    if (!mqttClient.connect("NodemcuClient")) {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
  mqttClient.subscribe("/foo/bar");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received message [");
  Serial.print(topic);
  Serial.print("] ");
  char msg[length+1];
  for (unsigned int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    msg[i] = (char)payload[i];
  }
  Serial.println();

  msg[length] = '\0';
  Serial.println(msg);
  if (strcmp(msg, "on") == 0) {
    digitalWrite(LED_BUILTIN, LOW);
  } else if (strcmp(msg, "off") == 0) {
    digitalWrite(LED_BUILTIN, HIGH);
  }
}

void setup() {
  Serial.begin(9600);
  connectToWifi();
  checkForUpdate(true);
  mqttClient.setServer(MQTT_BROKER, 1883);
  mqttClient.setCallback(callback);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
}

void loop() {
  checkForUpdate(false);
  sendPing();

  if (!mqttClient.connected()) {
    reconnectMqttClient();
  }
  mqttClient.loop();
}
