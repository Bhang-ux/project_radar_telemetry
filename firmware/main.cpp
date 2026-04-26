#include <Arduino.h>
#include "DFRobot_HumanDetection.h"
#include <WiFi.h>
#include <PubSubClient.h>

DFRobot_HumanDetection hu(&Serial2);

// -------- CHANGE THESE --------
const char* WIFI_SSID   = "Galaxy A25 5G BD6A";
const char* WIFI_PASS   = "12345678";
const char* MQTT_BROKER = "broker.hivemq.com";
const char* MQTT_TOPIC  = "nivaasiq/bathroom/status";
const int   MQTT_PORT   = 1883;
// ------------------------------

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long stillSince = 0;
String lastMessage = "";

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
}

void connectMQTT() {
  while (!mqtt.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqtt.connect("NivaasIQ_Device")) {
      Serial.println("connected");
    } else {
      Serial.print("failed, retrying in 3 seconds");
      delay(3000);
    }
  }
}

void sendMessage(String msg) {
  // Reconnect if dropped
  if (!mqtt.connected()) connectMQTT();

  if (msg != lastMessage) {
    // State changed — print loudly on serial
    Serial.println("----------------------------------------");
    Serial.println(msg);
    Serial.println("----------------------------------------");

    // Send to phone via MQTT
    mqtt.publish(MQTT_TOPIC, msg.c_str());

    lastMessage = msg;
  } else {
    // Same state — just a heartbeat dot
    Serial.println(".");
  }
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  connectWiFi();
  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  connectMQTT();

  Serial.println("Initializing sensor...");
  while (hu.begin() != 0) {
    Serial.println("Sensor not ready, retrying...");
    delay(1000);
  }
  while (hu.configWorkMode(hu.eFallingMode) != 0) {
    Serial.println("Mode switch failed, retrying...");
    delay(1000);
  }

  hu.dmInstallHeight(270);
  hu.dmFallTime(5);
  hu.dmUnmannedTime(1);
  hu.dmFallConfig(hu.eResidenceTime, 200);
  hu.dmFallConfig(hu.eFallSensitivityC, 3);
  hu.sensorRet();

  Serial.println("System ready. Monitoring bathroom...");
}

void loop() {

  // Keep MQTT connection alive
  mqtt.loop();

  int presence    = hu.smHumanData(hu.eHumanPresence);
  int movement    = hu.smHumanData(hu.eHumanMovement);
  int movingRange = hu.smHumanData(hu.eHumanMovingRange);
  int fallState   = hu.getFallData(hu.eFallState);
  int staticDwell = hu.getFallData(hu.estaticResidencyState);

  String currentMessage = "";

  if (fallState == 1) {
    currentMessage = "EMERGENCY: FALL DETECTED — SOS triggered";
    stillSince = 0;
  }
  else if (staticDwell == 1) {
    currentMessage = "EMERGENCY: PERSON MOTIONLESS TOO LONG — possible faint";
    stillSince = 0;
  }
  else if (presence == 1 && movement <= 1 && movingRange < 5) {
    if (stillSince == 0) stillSince = millis();
    if (millis() - stillSince > 10000) {
      currentMessage = "WARNING: Person unusually still — watching closely";
    } else {
      currentMessage = "Person entered — currently still";
    }
  }
  else if (presence == 1 && movement == 2) {
    currentMessage = "Bathroom in use — person active, all normal";
    stillSince = 0;
  }
  else if (presence == 1 && movement == 1 && movingRange >= 5) {
    currentMessage = "Person in bathroom — standing or sitting, normal";
    stillSince = 0;
  }
  else if (presence == 0) {
    currentMessage = "Bathroom empty — all clear";
    stillSince = 0;
  }
  else {
    currentMessage = "Reading unclear — continuing to monitor";
  }

  sendMessage(currentMessage);
  delay(1000);
}