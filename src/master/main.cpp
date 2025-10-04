// master main.cpp
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

const char *command_topic = "ta25stage/command";
const char *status_topic = "ta25stage/master/status";

WiFiClient espClient;
PubSubClient client(espClient);

LightCommand currentCommand;

// Connection management
uint8_t mqtt_fail_count = 0;
uint16_t retry_timeout = 5;
unsigned long last_heartbeat = 0;
unsigned long last_wifi_check = 0;
const unsigned long HEARTBEAT_INTERVAL = 30000;
const unsigned long WIFI_CHECK_INTERVAL = 60000;
const uint8_t MAX_MQTT_RETRIES = 5;
const uint16_t MAX_RETRY_TIMEOUT = 120;

bool wifi_connected = false;
bool mqtt_connected = false;
bool low_power_mode = false;

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_AP_STA);
  WiFi.begin(ssid, password);

  uint8_t retry_count = 0;
  while (WiFi.status() != WL_CONNECTED && retry_count < 60) {
    delay(500);
    Serial.print(".");
    retry_count++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
    wifi_connected = true;
  } else {
    Serial.println("\n✗ Failed to connect to WiFi");
    wifi_connected = false;
  }
}

void checkWiFiStatus() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("⚠ WiFi disconnected, reconnecting...");
    wifi_connected = false;
    setup_wifi();
  }
}

void publishHeartbeat() {
  if (!client.connected())
    return;

  StaticJsonDocument<512> doc;
  doc["device"] = "master";
  doc["uptime"] = millis() / 1000;
  doc["wifi_rssi"] = WiFi.RSSI();
  doc["wifi_connected"] = wifi_connected;
  doc["mqtt_connected"] = mqtt_connected;
  doc["mqtt_fails"] = mqtt_fail_count;
  doc["free_heap"] = ESP.getFreeHeap();
  doc["last_command"]["panelId"] = currentCommand.panelId;
  doc["last_command"]["mode"] = currentCommand.mode;
  doc["last_command"]["effectType"] = currentCommand.effectType;
  doc["last_command"]["brightness"] = currentCommand.brightness;
  doc["last_command"]["speed"] = currentCommand.speed;

  char buffer[512];
  serializeJson(doc, buffer);

  if (client.publish(status_topic, buffer)) {
    Serial.println("✓ Heartbeat published");
  }
}

void enterLowPowerMode() {
  Serial.println("\n===============================================");
  Serial.println("✗ ENTERING LOW POWER MODE");
  Serial.println("===============================================");
  Serial.println("Maximum retry attempts exceeded.");
  Serial.println("\nTROUBLESHOOTING STEPS:");
  Serial.println("1. Check MQTT broker connectivity");
  Serial.println("   - Ping: broker.emqx.io");
  Serial.println("   - Test port: 1883");
  Serial.println("2. Check WiFi signal strength");
  Serial.print("   - Current RSSI: ");
  Serial.println(WiFi.RSSI());
  Serial.println("3. Verify router/internet connection");
  Serial.println("4. Check firewall settings");
  Serial.println("\nDevice will enter deep sleep for 5 minutes...");
  Serial.println("Press reset button to restart immediately.");
  Serial.println("===============================================");
  Serial.flush();

  low_power_mode = true;
  esp_deep_sleep(5 * 60 * 1000000ULL);
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

void sendESPNowCommand(LightCommand &cmd) {
  if (cmd.panelId == 0) {
    esp_err_t result = esp_now_send(0, (uint8_t *)&cmd, sizeof(LightCommand));
    if (result == ESP_OK) {
      Serial.println("Broadcast sent successfully");
    } else {
      Serial.println("Error broadcasting");
    }
  } else {
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
        esp_now_send(target_mac, (uint8_t *)&cmd, sizeof(LightCommand));
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

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("JSON parse failed: ");
    Serial.println(error.f_str());
    return;
  }

  LightCommand cmd;
  
  for (int i = 0; i < MAX_REGIONS; i++) {
    cmd.regions[i] = false;
  }

  cmd.panelId = doc["panelId"] | 0;
  cmd.mode = doc["mode"] | MODE_DIRECT_REGIONS;
  cmd.sequenceId = doc["sequenceId"] | 0;
  cmd.groupId = doc["groupId"] | 0;
  cmd.effectType = doc["effectType"] | EFFECT_STATIC;
  cmd.brightness = doc["brightness"] | 128;
  cmd.speed = doc["speed"] | 50;
  cmd.step = doc["step"] | 0;
  cmd.audioReactive = doc["audioReactive"] | false;
  cmd.audioIntensity = doc["audioIntensity"] | 0;

  if (doc.containsKey("regions")) {
    JsonArray regions = doc["regions"].as<JsonArray>();
    for (int region : regions) {
      if (region >= 0 && region < MAX_REGIONS) {
        cmd.regions[region] = true;
      }
    }
  } else {
    for (int i = 0; i < MAX_REGIONS; i++) {
      cmd.regions[i] = true;
    }
  }

  currentCommand = cmd;
  sendESPNowCommand(cmd);
}

void reconnect() {
  if (client.connected()) {
    mqtt_connected = true;
    mqtt_fail_count = 0;
    retry_timeout = 5;
    return;
  }

  Serial.print("Attempting MQTT connection...");
  String clientId = "ESP32Master-" + String(random(0xffff), HEX);

  if (client.connect(clientId.c_str())) {
    Serial.println("✓ connected");
    mqtt_connected = true;
    mqtt_fail_count = 0;
    retry_timeout = 5;

    client.subscribe(command_topic);
  } else {
    Serial.print("✗ failed, rc=");
    Serial.println(client.state());

    mqtt_connected = false;
    mqtt_fail_count++;

    if (mqtt_fail_count >= MAX_MQTT_RETRIES) {
      Serial.println("⚠ Max MQTT retries exceeded");
      checkWiFiStatus();

      if (mqtt_fail_count >= MAX_MQTT_RETRIES * 2) {
        enterLowPowerMode();
      }
    }

    retry_timeout = min((uint16_t)(retry_timeout + 10), MAX_RETRY_TIMEOUT);
    Serial.print("Retrying in ");
    Serial.print(retry_timeout);
    Serial.println(" seconds...");
    delay(retry_timeout * 1000);
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
  if (low_power_mode)
    return;

  if (!client.connected()) {
    reconnect();
  } else {
    client.loop();
  }

  unsigned long currentMillis = millis();

  if (currentMillis - last_heartbeat >= HEARTBEAT_INTERVAL) {
    last_heartbeat = currentMillis;
    publishHeartbeat();
  }

  if (currentMillis - last_wifi_check >= WIFI_CHECK_INTERVAL) {
    last_wifi_check = currentMillis;
    checkWiFiStatus();
  }
}
