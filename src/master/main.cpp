#include "common.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

// WiFi Credentials
// const char *ssid = "arcane";
// const char *password = "hisashiburi";
const char *ssid = "Navam";
const char *password = "kskm2626";

// MQTT Broker
const char *mqtt_server = "broker.emqx.io";
const int mqtt_port = 1883;

const char *command_topic = "ta25stage/+/command"; // subscribe to all panels
const char *global_topic = "ta25stage/command";
const char *audio_topic = "ta25stage/audio";

WiFiClient espClient;
PubSubClient client(espClient);

LEDCommand currentCommand;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_AP_STA); // Both AP and Station mode for ESP-NOW + WiFi
  WiFi.begin(ssid, password);

  uint8_t retry_count = 0;
  while (WiFi.status() != WL_CONNECTED && retry_count < 60) {
    delay(500);
    Serial.print(".");
    retry_count++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected.");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
  } else {
    Serial.println("\nFailed to connect to WiFi.");
  }
}

// ESP-NOW send callback
void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  char macStr[18];
  snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0],
           mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

  Serial.print("Packet to ");
  Serial.print(macStr);
  Serial.print(" - Status: ");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}

void setup_espnow() {
  // Get the WiFi channel the master is connected on
  int8_t wifi_channel = WiFi.channel();
  Serial.print("Master WiFi Channel: ");
  Serial.println(wifi_channel);

  // VALIDATION: Check if it matches the expected channel
  if (wifi_channel != ESPNOW_WIFI_CHANNEL) {
    Serial.println("===============================================");
    Serial.println("WARNING: WiFi CHANNEL MISMATCH!");
    Serial.print("Expected channel: ");
    Serial.println(ESPNOW_WIFI_CHANNEL);
    Serial.print("Actual channel: ");
    Serial.println(wifi_channel);
    Serial.println("ESP-NOW communication will FAIL!");
    Serial.println("===============================================");
    Serial.println("ACTION REQUIRED:");
    Serial.println("1. Update ESPNOW_WIFI_CHANNEL in common.h to: " +
                   String(wifi_channel));
    Serial.println("2. Re-upload to all panel ESP32s");
    Serial.println("OR");
    Serial.println("3. Change your router to use channel " +
                   String(ESPNOW_WIFI_CHANNEL));
    Serial.println("===============================================");

    // Optional: halt execution until fixed
    // while(1) { delay(1000); }
  } else {
    Serial.println("✓ WiFi channel matches ESPNOW_WIFI_CHANNEL");
  }

  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Register send callback
  esp_now_register_send_cb(onDataSent);

  // Register all panel peers
  esp_now_peer_info_t peerInfo = {};
  peerInfo.channel = wifi_channel; // Use actual channel
  peerInfo.encrypt = false;
  peerInfo.ifidx = WIFI_IF_STA;

  // Add Panel 1
  memcpy(peerInfo.peer_addr, panel1_mac, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Panel 1");
  } else {
    Serial.println("Panel 1 added");
  }

  // Add Panel 2
  memcpy(peerInfo.peer_addr, panel2_mac, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Panel 2");
  } else {
    Serial.println("Panel 2 added");
  }

  // Add Panel 3
  memcpy(peerInfo.peer_addr, panel3_mac, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Panel 3");
  } else {
    Serial.println("Panel 3 added");
  }

  // Add Panel 4
  memcpy(peerInfo.peer_addr, panel4_mac, 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add Panel 4");
  } else {
    Serial.println("Panel 4 added");
  }

  Serial.println("ESP-NOW initialized");
}

void sendESPNowCommand(LEDCommand &cmd) {
  // If panelId is 0, broadcast to all panels
  if (cmd.panelId == 0) {
    esp_err_t result = esp_now_send(0, (uint8_t *)&cmd, sizeof(LEDCommand));
    if (result == ESP_OK) {
      Serial.println("Broadcast sent successfully");
    } else {
      Serial.println("Error broadcasting");
    }
  } else {
    // Send to specific panel
    uint8_t *target_mac = nullptr;
    switch (cmd.panelId) {
    case 1:
      target_mac = panel1_mac;
      break;
    case 2:
      target_mac = panel2_mac;
      break;
    case 3:
      target_mac = panel3_mac;
      break;
    case 4:
      target_mac = panel4_mac;
      break;
    default:
      Serial.println("Invalid panel ID");
      return;
    }

    esp_err_t result =
        esp_now_send(target_mac, (uint8_t *)&cmd, sizeof(LEDCommand));
    if (result == ESP_OK) {
      Serial.print("Sent to Panel ");
      Serial.println(cmd.panelId);
    } else {
      Serial.println("Error sending");
    }
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.print("MQTT message on topic: ");
  Serial.println(topic);

  StaticJsonDocument<512> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.f_str());
    return;
  }

  // Initialize command
  LEDCommand cmd;

  // Parse panelId (0 = all panels)
  cmd.panelId = doc["panelId"] | 0;

  // Parse patternId
  if (doc.containsKey("patternId")) {
    cmd.patternId = doc["patternId"];
  }

  // Parse brightness
  if (doc.containsKey("brightness")) {
    cmd.brightness = doc["brightness"];
  }

  // Parse regions
  if (doc.containsKey("regions")) {
    // Reset all regions
    for (int i = 0; i < NUM_REGIONS; i++) {
      cmd.regions[i] = false;
    }
    JsonArray regions = doc["regions"].as<JsonArray>();
    for (int region : regions) {
      if (region > 0 && region <= NUM_REGIONS) {
        cmd.regions[region - 1] = true;
      }
    }
  } else {
    // Default: all regions enabled
    for (int i = 0; i < NUM_REGIONS; i++) {
      cmd.regions[i] = true;
    }
  }

  // Parse speed
  cmd.speed = doc["speed"] | 50;

  // Parse audioReactive
  cmd.audioReactive = doc["audioReactive"] | false;

  // Audio intensity (from separate audio topic)
  cmd.audioIntensity = doc["intensity"] | 0;

  // Send command via ESP-NOW
  sendESPNowCommand(cmd);
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP32Master-" + String(random(0xffff), HEX);

    if (client.connect(clientId.c_str())) {
      Serial.println("connected");

      // Subscribe to topics
      client.subscribe(command_topic);
      client.subscribe(global_topic);
      client.subscribe(audio_topic);

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);

  Serial.println("\n=== MASTER ESP32 ===");

  setup_wifi();
  setup_espnow();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(mqttCallback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
