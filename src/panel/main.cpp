#include "common.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// This will be defined by build flag in platformio.ini
#ifndef PANEL_ID
#define PANEL_ID 1
#endif

// LED Region Setup
const uint8_t pwmPins[NUM_REGIONS] = {25, 26, 27, 14, 12, 13, 15};
const uint8_t pwmChannels[NUM_REGIONS] = {0, 1, 2, 3, 4, 5, 6};

// PWM Configuration
#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

// Current state
LEDCommand currentState;

void setRegionBrightness(uint8_t region, uint8_t brightness) {
  if (region < NUM_REGIONS) {
    ledcWrite(pwmChannels[region], brightness);
  }
}

// Pattern 0: All On
void pattern_allOn() {
  for (int r = 0; r < NUM_REGIONS; r++) {
    if (currentState.regions[r]) {
      setRegionBrightness(r, currentState.brightness);
    } else {
      setRegionBrightness(r, 0);
    }
  }
}

// Pattern 1: Breathing
void pattern_breathing() {
  static unsigned long lastUpdate = 0;
  static uint8_t breatheBrightness = 0;
  static int8_t direction = 1;

  unsigned long currentMillis = millis();
  uint16_t updateInterval = map(currentState.speed, 0, 100, 50, 5);

  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;

    breatheBrightness += direction * 5;

    if (breatheBrightness >= 255) {
      breatheBrightness = 255;
      direction = -1;
    } else if (breatheBrightness <= 0) {
      breatheBrightness = 0;
      direction = 1;
    }

    for (int r = 0; r < NUM_REGIONS; r++) {
      if (currentState.regions[r]) {
        setRegionBrightness(r, breatheBrightness);
      } else {
        setRegionBrightness(r, 0);
      }
    }
  }
}

// Pattern 2: Wave
void pattern_wave() {
  static unsigned long lastUpdate = 0;
  static uint8_t activeRegion = 0;

  unsigned long currentMillis = millis();
  uint16_t updateInterval = map(currentState.speed, 0, 100, 1000, 100);

  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;

    for (int r = 0; r < NUM_REGIONS; r++) {
      setRegionBrightness(r, 0);
    }

    if (currentState.regions[activeRegion]) {
      setRegionBrightness(activeRegion, 255);
    }

    do {
      activeRegion = (activeRegion + 1) % NUM_REGIONS;
    } while (!currentState.regions[activeRegion] && activeRegion != 0);
  }
}

// Pattern 3: Pulse
void pattern_pulse() {
  static unsigned long lastUpdate = 0;
  static uint8_t pulseBrightness = 0;
  static bool increasing = true;

  unsigned long currentMillis = millis();
  uint16_t updateInterval = map(currentState.speed, 0, 100, 30, 5);

  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;

    if (increasing) {
      pulseBrightness += 10;
      if (pulseBrightness >= 255) {
        pulseBrightness = 255;
        increasing = false;
      }
    } else {
      if (pulseBrightness >= 10) {
        pulseBrightness -= 10;
      } else {
        pulseBrightness = 0;
        increasing = true;
      }
    }

    uint8_t finalBrightness = pulseBrightness;

    // Modulate with audio if enabled
    if (currentState.audioReactive && currentState.audioIntensity > 0) {
      finalBrightness = map(currentState.audioIntensity, 0, 255,
                            pulseBrightness / 2, pulseBrightness);
    }

    for (int r = 0; r < NUM_REGIONS; r++) {
      if (currentState.regions[r]) {
        setRegionBrightness(r, finalBrightness);
      } else {
        setRegionBrightness(r, 0);
      }
    }
  }
}

void executePattern() {
  switch (currentState.patternId) {
  case 0:
    pattern_allOn();
    break;
  case 1:
    pattern_breathing();
    break;
  case 2:
    pattern_wave();
    break;
  case 3:
    pattern_pulse();
    break;
  default:
    pattern_allOn();
  }
}

// ESP-NOW receive callback
void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0],
           mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

  Serial.print("Received from: ");
  Serial.println(macStr);

  if (data_len == sizeof(LEDCommand)) {
    LEDCommand receivedCmd;
    memcpy(&receivedCmd, data, sizeof(LEDCommand));

    // Check if command is for this panel or broadcast
    if (receivedCmd.panelId == 0 || receivedCmd.panelId == PANEL_ID) {
      Serial.print("Command accepted - Pattern: ");
      Serial.print(receivedCmd.patternId);
      Serial.print(" Brightness: ");
      Serial.println(receivedCmd.brightness);

      // Update current state
      currentState = receivedCmd;
    } else {
      Serial.print("Command ignored (for panel ");
      Serial.print(receivedCmd.panelId);
      Serial.println(")");
    }
  }
}

void setup() {
  Serial.begin(115200);

  Serial.print("\n=== PANEL ");
  Serial.print(PANEL_ID);
  Serial.println(" ===");

  // Initialize WiFi in station mode
  WiFi.mode(WIFI_STA);

  // Print factory MAC for reference
  Serial.print("Factory MAC: ");
  Serial.println(WiFi.macAddress());

  // Set custom MAC address based on PANEL_ID
  uint8_t customMAC[6] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x00};
  customMAC[5] = PANEL_ID;

  esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, customMAC);
  if (err == ESP_OK) {
    Serial.println("Custom MAC address set successfully");
  } else {
    Serial.println("Failed to set custom MAC address");
  }

  Serial.print("Custom MAC: ");
  Serial.println(WiFi.macAddress());

  // Disconnect from any WiFi
  WiFi.disconnect();

  // Set WiFi channel to match master
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  // Verify channel was set correctly
  int8_t actual_channel = WiFi.channel();
  Serial.print("WiFi Channel set to: ");
  Serial.println(actual_channel);

  if (actual_channel != ESPNOW_WIFI_CHANNEL) {
    Serial.println("ERROR: Failed to set WiFi channel!");
    Serial.print("Expected: ");
    Serial.print(ESPNOW_WIFI_CHANNEL);
    Serial.print(", Got: ");
    Serial.println(actual_channel);
  } else {
    Serial.println("✓ WiFi channel configured correctly");
  }

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register receive callback
  esp_now_register_recv_cb(onDataRecv);

  // Initialize PWM channels
  for (int i = 0; i < NUM_REGIONS; i++) {
    ledcSetup(pwmChannels[i], PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(pwmPins[i], pwmChannels[i]);
    setRegionBrightness(i, 0);
  }

  // Initialize default state
  currentState.patternId = 0;
  currentState.brightness = 128;
  currentState.speed = 50;
  currentState.audioReactive = false;
  currentState.audioIntensity = 0;

  for (int i = 0; i < NUM_REGIONS; i++) {
    currentState.regions[i] = true;
  }

  Serial.println("Ready to receive commands");
}

void loop() {
  executePattern();
  delay(10);
}
