// master main.cpp
#include "common.h"
#include "region_groups.h"
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

void runSequence0();
void runSequence1();
void runSequence2();
void runSequence3();

bool sequenceRunning = false;

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
  
  if (cmd.mode == MODE_SEQUENCE) {
    Serial.print("Sequence mode requested, ID: ");
    Serial.println(cmd.sequenceId);
    
    switch (cmd.sequenceId) {
      case 0:
        runSequence0();
        break;
      case 1:
        runSequence1();
        break;
      case 2:
        runSequence2();
        break;
      case 3:
        runSequence3();
        break;
      default:
        Serial.print("Unknown sequence ID: ");
        Serial.println(cmd.sequenceId);
    }
  } else {
    sendESPNowCommand(cmd);
  }
}

void setAllRegions(bool state, LightCommand &cmd) {
  for (int i = 0; i < MAX_REGIONS; i++) {
    cmd.regions[i] = state;
  }
}

void setRegionsByList(const uint8_t* regionList, uint8_t count, bool state, LightCommand &cmd) {
  for (uint8_t i = 0; i < count; i++) {
    cmd.regions[regionList[i]] = state;
  }
}

void runSequence0() {
  Serial.println("\n=== Running Sequence 0: Test Sequence ===");
  sequenceRunning = true;
  
  LightCommand cmd;
  cmd.panelId = 0;
  cmd.mode = MODE_SEQUENCE;
  cmd.sequenceId = 0;
  cmd.groupId = 0;
  cmd.step = 0;
  cmd.audioReactive = false;
  cmd.audioIntensity = 0;
  
  Serial.println("Step 1: All regions full brightness (2s)");
  setAllRegions(true, cmd);
  cmd.effectType = EFFECT_STATIC;
  cmd.brightness = 255;
  cmd.speed = 50;
  sendESPNowCommand(cmd);
  delay(2000);
  
  Serial.println("Step 2: Wave effect (5s)");
  cmd.effectType = EFFECT_WAVE;
  cmd.speed = 60;
  sendESPNowCommand(cmd);
  delay(5000);
  
  Serial.println("Step 3: Pulse fast (3s)");
  cmd.effectType = EFFECT_PULSE;
  cmd.speed = 80;
  sendESPNowCommand(cmd);
  delay(3000);
  
  Serial.println("Step 4: Pulse slow (3s)");
  cmd.speed = 30;
  sendESPNowCommand(cmd);
  delay(3000);
  
  Serial.println("Step 5: Fade in (3s)");
  cmd.effectType = EFFECT_FADE_IN;
  cmd.speed = 50;
  sendESPNowCommand(cmd);
  delay(3000);
  
  Serial.println("Step 6: Fade out (3s)");
  cmd.effectType = EFFECT_FADE_OUT;
  sendESPNowCommand(cmd);
  delay(3000);
  
  Serial.println("Step 7: All off");
  cmd.effectType = EFFECT_STATIC;
  cmd.brightness = 0;
  sendESPNowCommand(cmd);
  delay(500);
  
  Serial.println("✓ Sequence 0 complete\n");
  sequenceRunning = false;
}

void runSequence1() {
  Serial.println("\n=== Running Sequence 1: Vertical Sweep ===");
  sequenceRunning = true;
  
  LightCommand cmd;
  cmd.panelId = 0;
  cmd.mode = MODE_SEQUENCE;
  cmd.sequenceId = 1;
  cmd.groupId = 0;
  cmd.effectType = EFFECT_FADE_IN;
  cmd.brightness = 200;
  cmd.speed = 60;
  cmd.audioReactive = false;
  cmd.audioIntensity = 0;
  
  setAllRegions(false, cmd);
  
  Serial.println("Forward sweep: top to bottom");
  for (uint8_t region = 0; region < MAX_REGIONS; region++) {
    cmd.regions[region] = true;
    cmd.step = region;
    sendESPNowCommand(cmd);
    delay(300);
  }
  
  Serial.println("Hold all lit (2s)");
  delay(2000);
  
  Serial.println("Reverse sweep: bottom to top");
  cmd.effectType = EFFECT_FADE_OUT;
  for (int region = MAX_REGIONS - 1; region >= 0; region--) {
    cmd.regions[region] = false;
    cmd.step = MAX_REGIONS - region - 1;
    sendESPNowCommand(cmd);
    delay(300);
  }
  
  Serial.println("✓ Sequence 1 complete\n");
  sequenceRunning = false;
}

void runSequence2() {
  Serial.println("\n=== Running Sequence 2: Group Narrative ===");
  sequenceRunning = true;
  
  LightCommand cmd;
  cmd.panelId = 0;
  cmd.mode = MODE_SEQUENCE;
  cmd.sequenceId = 2;
  cmd.groupId = 0;
  cmd.effectType = EFFECT_FADE_IN;
  cmd.brightness = 220;
  cmd.speed = 50;
  cmd.audioReactive = false;
  cmd.audioIntensity = 0;
  
  setAllRegions(false, cmd);
  
  Serial.println("Step 1: Light SYMBOL group");
  cmd.step = 0;
  setRegionsByList(SYMBOL_REGIONS, SYMBOL_COUNT, true, cmd);
  sendESPNowCommand(cmd);
  delay(4000);
  
  Serial.println("Step 2: Add RAAVANA_HEAD group");
  cmd.step = 1;
  setRegionsByList(RAAVANA_HEAD_REGIONS, RAAVANA_HEAD_COUNT, true, cmd);
  sendESPNowCommand(cmd);
  delay(4000);
  
  Serial.println("Step 3: Expand to full RAAVANA group");
  cmd.step = 2;
  setRegionsByList(RAAVANA_REGIONS, RAAVANA_COUNT, true, cmd);
  sendESPNowCommand(cmd);
  delay(5000);
  
  Serial.println("Step 4: Light CONTINENT group");
  cmd.step = 3;
  setRegionsByList(CONTINENT_REGIONS, CONTINENT_COUNT, true, cmd);
  sendESPNowCommand(cmd);
  delay(5000);
  
  Serial.println("Step 5: Fade all out");
  cmd.effectType = EFFECT_FADE_OUT;
  sendESPNowCommand(cmd);
  delay(3000);
  
  Serial.println("✓ Sequence 2 complete\n");
  sequenceRunning = false;
}

void runSequence3() {
  Serial.println("\n=== Running Sequence 3: Symbol Emergence ===");
  sequenceRunning = true;
  
  LightCommand cmd;
  cmd.panelId = 0;
  cmd.mode = MODE_SEQUENCE;
  cmd.sequenceId = 3;
  cmd.groupId = 0;
  cmd.brightness = 200;
  cmd.audioReactive = false;
  cmd.audioIntensity = 0;
  
  setAllRegions(false, cmd);
  
  Serial.println("Step 1: Fade in SYMBOL regions (0.5s fade)");
  cmd.step = 0;
  cmd.effectType = EFFECT_FADE_IN;
  cmd.speed = 80;
  setRegionsByList(SYMBOL_REGIONS, SYMBOL_COUNT, true, cmd);
  sendESPNowCommand(cmd);
  delay(500);
  
  Serial.println("Step 2: Hold symbols (3s)");
  cmd.effectType = EFFECT_STATIC;
  cmd.brightness = 200;
  sendESPNowCommand(cmd);
  delay(3000);
  
  Serial.println("Step 3: Fade in all other regions (2s fade)");
  cmd.step = 1;
  cmd.effectType = EFFECT_FADE_IN;
  cmd.speed = 40;
  setAllRegions(true, cmd);
  sendESPNowCommand(cmd);
  delay(2000);
  
  Serial.println("Step 4: Hold full illumination (5s)");
  cmd.effectType = EFFECT_STATIC;
  cmd.brightness = 200;
  sendESPNowCommand(cmd);
  delay(5000);
  
  Serial.println("Step 5: Fade all out");
  cmd.effectType = EFFECT_FADE_OUT;
  sendESPNowCommand(cmd);
  delay(3000);
  
  Serial.println("✓ Sequence 3 complete\n");
  sequenceRunning = false;
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
  
  Serial.println("\nStarting test sequence in 3 seconds...");
  delay(3000);
  runSequence0();
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
