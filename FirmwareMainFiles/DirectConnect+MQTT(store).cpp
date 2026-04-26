#include <Arduino.h>
#include "DFRobot_HumanDetection.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <time.h>

DFRobot_HumanDetection hu(&Serial2);

// -------- CHANGE THESE --------
const char* WIFI_SSID   = "YOUR_WIFI_NAME";
const char* WIFI_PASS   = "YOUR_WIFI_PASSWORD";
const char* MQTT_BROKER = "broker.hivemq.com";
const char* MQTT_TOPIC  = "nivaasiq/bathroom/status";
const int   MQTT_PORT   = 1883;
// ------------------------------

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

unsigned long stillSince = 0;
String lastMessage = "";

void connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  Serial.print("Connecting to WiFi");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected");
  } else {
    Serial.println("\nWiFi failed — running offline");
  }
}

void connectMQTT() {
  if (mqtt.connected()) return;
  int attempts = 0;
  while (!mqtt.connected() && attempts < 5) {
    if (mqtt.connect("NivaasIQ_Device")) {
      Serial.println("MQTT connected");
    } else {
      delay(1000);
      attempts++;
    }
  }
}

String getTimestamp() {
  if (WiFi.status() == WL_CONNECTED) {
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
      char buf[30];
      strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
      return String(buf);
    }
  }
  unsigned long ms = millis();
  unsigned long secs = ms / 1000;
  unsigned long mins = secs / 60;
  unsigned long hrs  = mins / 60;
  char buf[30];
  sprintf(buf, "T+%02lu:%02lu:%02lu", hrs, mins % 60, secs % 60);
  return String(buf);
}

void logToFile(String timestamp, String status) {
  File file = SPIFFS.open("/events.csv", FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open log file");
    return;
  }
  String line = timestamp + "," + status + "\n";
  file.print(line);
  file.close();
  Serial.println("Logged: " + line);
}

void rotateLog() {
  if (SPIFFS.exists("/events_old.csv")) {
    SPIFFS.remove("/events_old.csv");
  }
  SPIFFS.rename("/events.csv", "/events_old.csv");
  Serial.println("Log rotated — old log archived");
}

void checkStorage() {
  static int loopCount = 0;
  loopCount++;
  if (loopCount % 100 != 0) return;
  size_t total = SPIFFS.totalBytes();
  size_t used  = SPIFFS.usedBytes();
  float percentUsed = (float)used / total * 100;
  Serial.printf("Storage: %.1f%% used (%d / %d bytes)\n", percentUsed, used, total);
  if (percentUsed > 80) {
    Serial.println("Storage 80% full — rotating log");
    rotateLog();
  }
}

void sendMessage(String msg) {
  if (msg != lastMessage) {
    Serial.println("----------------------------------------");
    Serial.println(msg);
    Serial.println("----------------------------------------");
    String ts = getTimestamp();
    logToFile(ts, msg);
    if (mqtt.connected()) {
      mqtt.publish(MQTT_TOPIC, msg.c_str());
    }
    lastMessage = msg;
  }
}

void dumpLogFile(String filename) {
  File file = SPIFFS.open(filename, FILE_READ);
  if (!file) {
    Serial.println("FILE_NOT_FOUND");
    return;
  }
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS failed");
  } else {
    Serial.println("SPIFFS ready");
  }

  // Wait 5 seconds at boot for read command
  Serial.println("Send 'R' to read current log");
  Serial.println("Send 'O' to read old log");
  Serial.println("Send 'C' to clear all logs");
  unsigned long wait = millis();
  while (millis() - wait < 5000) {
    if (Serial.available()) {
      char cmd = Serial.read();
      if (cmd == 'R') {
        Serial.println("--- START LOG ---");
        dumpLogFile("/events.csv");
        Serial.println("--- END LOG ---");
      }
      else if (cmd == 'O') {
        Serial.println("--- START OLD LOG ---");
        dumpLogFile("/events_old.csv");
        Serial.println("--- END OLD LOG ---");
      }
      else if (cmd == 'C') {
        SPIFFS.remove("/events.csv");
        SPIFFS.remove("/events_old.csv");
        Serial.println("All logs cleared");
      }
    }
  }

  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    configTime(19800, 0, "pool.ntp.org");
    Serial.println("Time synced");
  }

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
  connectWiFi();
  if (WiFi.status() == WL_CONNECTED) connectMQTT();
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
  checkStorage();
  delay(0);
}