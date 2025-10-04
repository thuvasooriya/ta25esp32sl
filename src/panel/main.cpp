#include "common.h"
#include "panel_config.h"
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#ifndef PANEL_ID
#define PANEL_ID 1
#endif

uint8_t pwmChannels[MAX_REGIONS];

void initPWMChannels() {
  for (int i = 0; i < MAX_REGIONS; i++) {
    pwmChannels[i] = i;
  }
}

#define PWM_FREQ 5000
#define PWM_RESOLUTION 8

LightCommand currentState;
uint8_t lastEffect = EFFECT_STATIC;

unsigned long lastCommandReceived = 0;
unsigned long lastEffectUpdate = 0;
unsigned long loopCounter = 0;
unsigned long lastHeartbeat = 0;
const unsigned long COMMAND_TIMEOUT = 300000;
const unsigned long HEARTBEAT_INTERVAL = 60000;

void setRegionBrightness(uint8_t region, uint8_t brightness) {
  if (region < NUM_REGIONS) {
    ledcWrite(pwmChannels[region], brightness);
  }
}

void resetEffectStates() {
  for (int r = 0; r < NUM_REGIONS; r++) {
    setRegionBrightness(r, 0);
  }
}

void effect_static() {
  for (int r = 0; r < NUM_REGIONS; r++) {
    if (currentState.regions[r]) {
      setRegionBrightness(r, currentState.brightness);
    } else {
      setRegionBrightness(r, 0);
    }
  }
}

void effect_breathing() {
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

    uint8_t scaledBrightness = map(breatheBrightness, 0, 255, 0, currentState.brightness);

    for (int r = 0; r < NUM_REGIONS; r++) {
      if (currentState.regions[r]) {
        setRegionBrightness(r, scaledBrightness);
      } else {
        setRegionBrightness(r, 0);
      }
    }
  }
}

void effect_wave() {
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
      setRegionBrightness(activeRegion, currentState.brightness);
    }

    do {
      activeRegion = (activeRegion + 1) % NUM_REGIONS;
    } while (!currentState.regions[activeRegion] && activeRegion != 0);
  }
}

void effect_pulse() {
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

    uint8_t scaledBrightness = map(pulseBrightness, 0, 255, 0, currentState.brightness);

    for (int r = 0; r < NUM_REGIONS; r++) {
      if (currentState.regions[r]) {
        setRegionBrightness(r, scaledBrightness);
      } else {
        setRegionBrightness(r, 0);
      }
    }
  }
}

void effect_fade_in() {
  static unsigned long lastUpdate = 0;
  static uint8_t fadeBrightness = 0;

  unsigned long currentMillis = millis();
  uint16_t updateInterval = map(currentState.speed, 0, 100, 50, 5);

  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;

    if (fadeBrightness < currentState.brightness) {
      fadeBrightness += 5;
      if (fadeBrightness > currentState.brightness) {
        fadeBrightness = currentState.brightness;
      }
    }

    for (int r = 0; r < NUM_REGIONS; r++) {
      if (currentState.regions[r]) {
        setRegionBrightness(r, fadeBrightness);
      } else {
        setRegionBrightness(r, 0);
      }
    }
  }
}

void effect_fade_out() {
  static unsigned long lastUpdate = 0;
  static uint8_t fadeBrightness = 255;

  unsigned long currentMillis = millis();
  uint16_t updateInterval = map(currentState.speed, 0, 100, 50, 5);

  if (currentMillis - lastUpdate >= updateInterval) {
    lastUpdate = currentMillis;

    if (fadeBrightness > 0) {
      if (fadeBrightness >= 5) {
        fadeBrightness -= 5;
      } else {
        fadeBrightness = 0;
      }
    }

    for (int r = 0; r < NUM_REGIONS; r++) {
      if (currentState.regions[r]) {
        setRegionBrightness(r, fadeBrightness);
      } else {
        setRegionBrightness(r, 0);
      }
    }
  }
}

void executeEffect() {
  lastEffectUpdate = millis();
  
  switch (currentState.effect) {
  case EFFECT_STATIC:
    effect_static();
    break;
  case EFFECT_BREATHING:
    effect_breathing();
    break;
  case EFFECT_WAVE:
    effect_wave();
    break;
  case EFFECT_PULSE:
    effect_pulse();
    break;
  case EFFECT_FADE_IN:
    effect_fade_in();
    break;
  case EFFECT_FADE_OUT:
    effect_fade_out();
    break;
  default:
    effect_static();
  }
}

void checkCommandTimeout() {
  if (lastCommandReceived > 0) {
    unsigned long timeSinceLastCommand = millis() - lastCommandReceived;
    if (timeSinceLastCommand > COMMAND_TIMEOUT) {
      Serial.println("⚠ No commands received for 5 minutes");
      Serial.println("Master may be offline or communication lost");
      lastCommandReceived = millis();
    }
  }
}

void printHeartbeat() {
  Serial.print("✓ Panel ");
  Serial.print(PANEL_ID);
  Serial.print(" (");
  Serial.print(NUM_REGIONS);
  Serial.print(" regions) | Uptime: ");
  Serial.print(millis() / 1000);
  Serial.print("s | Effect: ");
  Serial.print(currentState.effect);
  Serial.print(" | Brightness: ");
  Serial.print(currentState.brightness);
  Serial.print(" | Speed: ");
  Serial.print(currentState.speed);
  Serial.print(" | Loops: ");
  Serial.print(loopCounter);
  Serial.print(" | Heap: ");
  Serial.print(ESP.getFreeHeap());
  
  if (lastCommandReceived > 0) {
    Serial.print(" | Last cmd: ");
    Serial.print((millis() - lastCommandReceived) / 1000);
    Serial.print("s ago");
  }
  
  Serial.print(" | Active: ");
  uint8_t activeCount = 0;
  for (int i = 0; i < NUM_REGIONS; i++) {
    if (currentState.regions[i]) activeCount++;
  }
  Serial.print(activeCount);
  Serial.print("/");
  Serial.println(NUM_REGIONS);
}

void printRegionConfig() {
  Serial.println("\n=== Region Configuration ===");
  for (int i = 0; i < NUM_REGIONS; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.print(getRegionName(i));
    Serial.print(" (GPIO ");
    Serial.print(getRegionPin(i));
    Serial.print(", Pos ");
    Serial.print(getRegionVerticalPos(i));
    Serial.println(")");
  }
  Serial.println("===========================\n");
}

void onDataRecv(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0],
           mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

  Serial.print("Received from: ");
  Serial.println(macStr);

  if (data_len == sizeof(LightCommand)) {
    LightCommand receivedCmd;
    memcpy(&receivedCmd, data, sizeof(LightCommand));

    if (receivedCmd.panelId == 0 || receivedCmd.panelId == PANEL_ID) {
      Serial.print("✓ Command accepted - Effect: ");
      Serial.print(receivedCmd.effect);
      Serial.print(" Brightness: ");
      Serial.println(receivedCmd.brightness);

      if (receivedCmd.effect != lastEffect) {
        Serial.println("Effect type changed, resetting effect states");
        resetEffectStates();
        lastEffect = receivedCmd.effect;
      }

      currentState = receivedCmd;
      lastCommandReceived = millis();
    } else {
      Serial.print("Command ignored (for panel ");
      Serial.print(receivedCmd.panelId);
      Serial.println(")");
    }
  } else {
    Serial.print("⚠ Size mismatch: expected ");
    Serial.print(sizeof(LightCommand));
    Serial.print(", got ");
    Serial.println(data_len);
  }
}

void setup() {
  Serial.begin(115200);

  Serial.print("\n=== PANEL ");
  Serial.print(PANEL_ID);
  Serial.println(" ===");

  WiFi.mode(WIFI_STA);

  Serial.print("Factory MAC: ");
  Serial.println(WiFi.macAddress());

  uint8_t customMAC[6] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x00};
  customMAC[5] = PANEL_ID;

  esp_err_t err = esp_wifi_set_mac(WIFI_IF_STA, customMAC);
  if (err == ESP_OK) {
    Serial.println("✓ Custom MAC address set");
  } else {
    Serial.println("✗ Failed to set custom MAC address");
  }

  Serial.print("Custom MAC: ");
  Serial.println(WiFi.macAddress());

  WiFi.disconnect();

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);

  int8_t actual_channel = WiFi.channel();
  Serial.print("WiFi Channel: ");
  Serial.println(actual_channel);

  if (actual_channel != ESPNOW_WIFI_CHANNEL) {
    Serial.println("✗ WiFi channel mismatch!");
    Serial.print("Expected: ");
    Serial.print(ESPNOW_WIFI_CHANNEL);
    Serial.print(", Got: ");
    Serial.println(actual_channel);
  } else {
    Serial.println("✓ WiFi channel configured");
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("✗ ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);

  initPWMChannels();

  for (int i = 0; i < NUM_REGIONS; i++) {
    uint8_t pin = getRegionPin(i);
    ledcSetup(pwmChannels[i], PWM_FREQ, PWM_RESOLUTION);
    ledcAttachPin(pin, pwmChannels[i]);
    setRegionBrightness(i, 0);
  }

  currentState.effect = EFFECT_STATIC;
  currentState.brightness = 128;
  currentState.speed = 50;
  lastEffect = EFFECT_STATIC;

  for (int i = 0; i < MAX_REGIONS; i++) {
    currentState.regions[i] = (i < NUM_REGIONS);
  }

  printRegionConfig();
  Serial.println("✓ Ready to receive commands");
}

void loop() {
  executeEffect();
  loopCounter++;
  
  unsigned long currentMillis = millis();
  
  if (currentMillis - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    lastHeartbeat = currentMillis;
    printHeartbeat();
  }
  
  checkCommandTimeout();
  
  delay(10);
}
