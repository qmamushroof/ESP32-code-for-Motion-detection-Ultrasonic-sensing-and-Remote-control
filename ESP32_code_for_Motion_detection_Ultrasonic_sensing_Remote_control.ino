// https://github.com/qmamushroof/ESP32-code-for-Motion-detection-Ultrasonic-sensing-and-Remote-control

#include <WiFi.h>
#include "SinricPro.h"
#include "SinricProSwitch.h"
#include <map>

// The pin numbers depends on the specific setup
#define PIR_PIN 14
#define TRIG_PIN 12
#define ECHO_PIN 13
#define RED_LED_PIN 15
#define GREEN_LED_PIN 4

// Name and password of the specific wifi network
#define WIFI_SSID "XXX XXX"
#define WIFI_PASS "XXXXXXX"

// Unique keys and IDs from Sinric Pro
#define APP_KEY "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
#define APP_SECRET "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx-xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
#define SWITCH_ID_RED "xxxxxxxxxxxxxxxxxxxxxxxx"
#define SWITCH_ID_GREEN "yyyyyyyyyyyyyyyyyyyyyyyy"

std::map<String, bool> devices = {
  {SWITCH_ID_RED, false},
  {SWITCH_ID_GREEN, false}
};

bool redLEDState = false;
bool greenLEDState = false;
bool motionDetected = false;
unsigned long lastMotionTime = 0;
const unsigned long motionTimeout = 10000; // 10 seconds timeout

void setup() {
  Serial.begin(115200);
  pinMode(PIR_PIN, INPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  setupWiFi();
  setupSinricPro();
}

void setupWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void setupSinricPro()
{
  for (auto &device : devices)
  {
    const char *deviceId = device.first.c_str();
    SinricProSwitch &mySwitch = SinricPro[deviceId];
    mySwitch.onPowerState(onPowerState);
  }

  SinricPro.onConnected([]() {
    Serial.println("Connected to SinricPro");
  });
  SinricPro.onDisconnected([]() {
    Serial.println("Disconnected from SinricPro");
  });

  SinricPro.begin(APP_KEY, APP_SECRET);
  SinricPro.restoreDeviceStates(true);
}

bool onPowerState(String deviceId, bool &state)
{
  Serial.printf("%s: %s\r\n", deviceId.c_str(), state ? "on" : "off");

  if (deviceId == SWITCH_ID_RED) {
    digitalWrite(RED_LED_PIN, state ? HIGH : LOW);
    redLEDState = state;
  } else if (deviceId == SWITCH_ID_GREEN) {
    digitalWrite(GREEN_LED_PIN, state ? HIGH : LOW);
    greenLEDState = state;
  }

  devices[deviceId] = state; // Update the device state in the map

  return true;
}

void loop() {
  SinricPro.handle();
  handleMotionAndPresence();
}

void handleMotionAndPresence() {
  bool currentMotion = digitalRead(PIR_PIN) == HIGH;

  if (currentMotion) {
    lastMotionTime = millis();
    motionDetected = true;
  } else if (millis() - lastMotionTime > motionTimeout) {
    motionDetected = false;
  }

  if (motionDetected) {
    if (checkRoomOccupancy()) {
      turnOnLights();
    } else {
      if (!checkRoomOccupancy()) {
        turnOffLights();
      }
    }
  }
}

bool checkRoomOccupancy() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  long distance = duration * 0.034 / 2;

  return distance < 3; // Adjustable threshold based on room size
}

void turnOnLights() {
  if (!redLEDState || !greenLEDState) {
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, HIGH);
    redLEDState = true;
    greenLEDState = true;

    // Update SinricPro state
    SinricProSwitch &mySwitch1 = SinricPro[SWITCH_ID_RED];
    mySwitch1.sendPowerStateEvent(true);

    SinricProSwitch &mySwitch2 = SinricPro[SWITCH_ID_GREEN];
    mySwitch2.sendPowerStateEvent(true);
  }
}

void turnOffLights() {
  if (redLEDState || greenLEDState) {
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, LOW);
    redLEDState = false;
    greenLEDState = false;

    // Update SinricPro state
    SinricProSwitch &mySwitch1 = SinricPro[SWITCH_ID_RED];
    mySwitch1.sendPowerStateEvent(false);

    SinricProSwitch &mySwitch2 = SinricPro[SWITCH_ID_GREEN];
    mySwitch2.sendPowerStateEvent(false);
  }
}
