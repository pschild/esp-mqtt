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
char* version;

WiFiClient espClient;
PubSubClient client(espClient);

long lastMsg = 0;
char msg[50];
int value = 0;

unsigned long lastUpdateCheck = 0;
const long UPDATE_INTERVAL = 1 * 60 * 60 * 1000; // 1 hour

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
    ESPhttpUpdate.update(client, "http://192.168.178.28:9042/ota", version);
    lastUpdateCheck = now;
  }
}

void reconnectMqttClient() {
  while (!client.connected()) {
    Serial.print("Reconnecting...");
    if (!client.connect("NodeMcuClient")) {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      delay(5000);
    }
  }
  client.subscribe("/foo/bar");
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Received message [");
  Serial.print(topic);
  Serial.print("] ");
  char msg[length+1];
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
    msg[i] = (char)payload[i];
  }
  Serial.println();

  msg[length] = '\0';
  Serial.println(msg);
}

void setup() {
  if (VERSION == "") {
    version = "unknown";
  }

  Serial.begin(9600);
  connectToWifi();
  checkForUpdate(true);
  client.setServer(MQTT_BROKER, 1883);
  client.setCallback(callback);
}

void loop() {
  checkForUpdate(false);

  if (!client.connected()) {
    reconnectMqttClient();
  }
  client.loop();

  client.publish("/devices/NodeMcuClient/version", version);
  delay(5000);
}
