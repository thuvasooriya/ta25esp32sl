# Communication Protocols

Detailed specification of MQTT and ESP-NOW communication protocols.

## System Architecture

```

┌─────────────────────────────────────────────────────────────────┐
│                     Control Layer                               │
│  ┌──────────────────┐              ┌──────────────────┐         │
│  │   Android App    │              │  Web Interface   │         │
│  └────────┬─────────┘              └────────┬─────────┘         │
│           │                                 │                   │
│           └──────────────┬──────────────────┘                   │
│                          │ MQTT                                 │
└──────────────────────────┼──────────────────────────────────────┘
                           │
                  ┌────────▼────────┐
                  │  MQTT Broker    │
                  └────────┬────────┘
                           │ MQTT Subscribe
┌──────────────────────────┼───────────────────────────────────────┐
│                          │      Master Layer                     │
│                  ┌───────▼───────┐                               │
│                  │ Master ESP32  │                               │
│                  │ (MQTT + WiFi) │                               │
│                  └───────┬───────┘                               │
│                          │ ESP-NOW Broadcast                     │
└──────────────────────────┼───────────────────────────────────────┘
                           │
        ┌──────────────────┼──────────────────┐
        │                  │                  │
┌───────▼───────┐   ┌──────▼──────┐   ┌───────▼───────┐
│  Panel 1 ESP  │   │ Panel 2 ESP │   │  Panel 3 ESP  │ ...
│   (ESP-NOW)   │   │  (ESP-NOW)  │   │   (ESP-NOW)   │
└───────┬───────┘   └──────┬──────┘   └───────┬───────┘
        │                  │                  │
┌───────▼───────┐   ┌──────▼──────┐   ┌───────▼───────┐
│  7 LED        │   │ 7 LED       │   │  7 LED        │
│  Regions      │   │ Regions     │   │  Regions      │
│  (PWM Control)│   │(PWM Control)│   │ (PWM Control) │
└───────────────┘   └─────────────┘   └───────────────┘
```

## MQTT Protocol

### Broker Configuration

**Development** (current):

- Broker: `test.mosquitto.org`
- Port: `1883` (unencrypted)
- Authentication: None
- QoS: 0 (fire and forget)

**Production** (recommended):

- Local Mosquitto broker
- Port: `8883` (TLS encrypted)
- Authentication: Username/password
- QoS: 1 (at least once delivery)

### Topic Structure

| Topic                      | Direction    | Purpose                         | Retained | QoS |
| -------------------------- | ------------ | ------------------------------- | -------- | --- |
| `ta25stage/command`        | App → Master | All panel control (panelId in JSON) | No   | 0   |
| `ta25stage/master/status`  | Master → App | Master status heartbeat         | Yes      | 0   |

**Notes:**
- Panel routing handled by `panelId` field in JSON (0=all, 1-4=specific)
- Audio data included in command JSON via `audioReactive` and `audioIntensity` fields
- No separate panel or audio topics needed

### Command Payload Format

**JSON Schema**:

```json
{
  "panelId": 0,
  "mode": 0,
  "sequenceId": 0,
  "groupId": 0,
  "regions": [0, 1, 2, 3, 4],
  "effectType": 1,
  "brightness": 200,
  "speed": 50,
  "step": 0,
  "audioReactive": false,
  "audioIntensity": 0
}
```

**Field Specifications**:

| Field             | Type    | Range   | Required | Description                                        |
| ----------------- | ------- | ------- | -------- | -------------------------------------------------- |
| `panelId`         | integer | 0-4     | Yes      | 0 = all panels, 1-4 = specific panel               |
| `mode`            | integer | 0-2     | Yes      | 0=direct regions, 1=sequence, 2=group              |
| `sequenceId`      | integer | 0-10    | No       | Sequence ID (used when mode=1)                     |
| `groupId`         | integer | 0-10    | No       | Group ID (used when mode=2)                        |
| `regions`         | array   | [0-19]  | Yes      | Region indices (panel-specific, 0-indexed)         |
| `effectType`      | integer | 0-5     | Yes      | 0=static, 1=breathing, 2=wave, 3=pulse, 4=fade_in, 5=fade_out |
| `brightness`      | integer | 0-255   | No       | Target brightness level (default: 128)             |
| `speed`           | integer | 0-100   | No       | Animation speed (0=slowest, 100=fastest, default: 50) |
| `step`            | integer | 0-255   | No       | Step number for multi-step sequences               |
| `audioReactive`   | boolean | true/false | No    | Enable audio modulation (default: false)           |
| `audioIntensity`  | integer | 0-255   | No       | Audio intensity level (default: 0)                 |

**Modes**:
- **0 (MODE_DIRECT_REGIONS)**: Direct region control via `regions` array
- **1 (MODE_SEQUENCE)**: Orchestrated sequence (master calculates regions)
- **2 (MODE_GROUP)**: Group-based selection (master calculates regions)

**Effect Types**:
- **0 (EFFECT_STATIC)**: Solid brightness
- **1 (EFFECT_BREATHING)**: Sine wave fade in/out
- **2 (EFFECT_WAVE)**: Chase effect across regions
- **3 (EFFECT_PULSE)**: Quick on/off pulses
- **4 (EFFECT_FADE_IN)**: Gradual fade in
- **5 (EFFECT_FADE_OUT)**: Gradual fade out

### Example Commands

#### Master Status Payload

Published to `ta25stage/master/status` every 30 seconds:

```json
{
  "device": "master",
  "uptime": 3600,
  "wifi_rssi": -45,
  "wifi_connected": true,
  "mqtt_connected": true,
  "mqtt_fails": 0,
  "free_heap": 245000,
  "last_command": {
    "panelId": 0,
    "mode": 0,
    "effectType": 1,
    "brightness": 200,
    "speed": 50
  }
}
```

**Field Descriptions:**
- `uptime` - Seconds since boot
- `wifi_rssi` - WiFi signal strength (dBm)
- `mqtt_fails` - Consecutive MQTT connection failures
- `last_command` - Most recent command sent to panels

#### All Panels, Static Effect on All Regions

```json
{
  "panelId": 0,
  "mode": 0,
  "regions": [0, 1, 2, 3, 4, 5],
  "effectType": 0,
  "brightness": 255,
  "speed": 50
}
```

#### Panel 2, Breathing Effect on Specific Regions

```json
{
  "panelId": 2,
  "mode": 0,
  "regions": [0, 2, 4],
  "effectType": 1,
  "brightness": 200,
  "speed": 50
}
```

#### Panel 1, Wave Effect Fast on Selected Regions

```json
{
  "panelId": 1,
  "mode": 0,
  "regions": [1, 2, 3],
  "effectType": 2,
  "brightness": 180,
  "speed": 80
}
```

#### Panel 3, Fade In Effect

```json
{
  "panelId": 3,
  "mode": 0,
  "regions": [0, 1, 2, 3, 4],
  "effectType": 4,
  "brightness": 255,
  "speed": 30
}
```

#### All Panels, Sequence Mode (Future)

```json
{
  "panelId": 0,
  "mode": 1,
  "sequenceId": 1,
  "effectType": 1,
  "brightness": 200,
  "speed": 60,
  "step": 0
}
```

#### Group-Based Control (Future)

```json
{
  "panelId": 0,
  "mode": 2,
  "groupId": 1,
  "effectType": 3,
  "brightness": 255,
  "speed": 70
}
```

### Error Handling

**Invalid JSON**:

- Master ignores malformed messages
- No error feedback to sender

**Out-of-Range Values**:

- Clamped to valid range (e.g., brightness 300 → 255)
- Command still executed

**Missing Fields**:

- Uses default values:
  - `brightness`: 128
  - `speed`: 50
  - `audioReactive`: false

### MQTT Client Implementation (Master)

**Libraries Used**:

- `PubSubClient` (MQTT client)
- `ArduinoJson` (JSON parsing)

**Callback Function**:

```cpp
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  StaticJsonDocument<1024> doc;
  deserializeJson(doc, payload, length);

  LightCommand lightCmd;
  lightCmd.panelId = doc["panelId"] | 0;
  lightCmd.mode = doc["mode"] | 0;
  lightCmd.sequenceId = doc["sequenceId"] | 0;
  lightCmd.groupId = doc["groupId"] | 0;
  lightCmd.effectType = doc["effectType"] | 0;
  lightCmd.brightness = doc["brightness"] | 128;
  lightCmd.speed = doc["speed"] | 50;
  lightCmd.step = doc["step"] | 0;
  lightCmd.audioReactive = doc["audioReactive"] | false;
  lightCmd.audioIntensity = doc["audioIntensity"] | 0;
  
  // Parse regions array (0-indexed)
  JsonArray regions = doc["regions"];
  for (uint8_t i = 0; i < MAX_REGIONS; i++) {
    lightCmd.regions[i] = false;
  }
  for (JsonVariant v : regions) {
    uint8_t region = v.as<uint8_t>();
    if (region < MAX_REGIONS) {
      lightCmd.regions[region] = true;
    }
  }

  // Send via ESP-NOW
  sendESPNowCommand(&lightCmd);
}
```

**Topic Subscription**:

```cpp
client.subscribe("ta25stage/command");
```

Single topic - panel routing via `panelId` in JSON payload.

## ESP-NOW Protocol

### Overview

ESP-NOW is a connectionless communication protocol developed by Espressif:

- Operates at the WiFi data link layer
- Does not require WiFi connection or handshake
- Low latency (<10ms typical)
- 250-byte payload limit
- Range: ~200m line-of-sight

### MAC Address Configuration

**Master**:

- Uses factory MAC address (varies per device)
- Discovered via `WiFi.macAddress()`

**Panels** (custom MAC addresses):

```cpp
uint8_t panel1_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x01};
uint8_t panel2_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x02};
uint8_t panel3_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x03};
uint8_t panel4_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x04};
```

**Why Custom MACs?**:

- Easier to remember and configure
- Avoids factory MAC conflicts
- Simplifies peer registration on master

**Setting Custom MAC** (panel firmware):

```cpp
esp_wifi_set_mac(WIFI_IF_STA, panel1_mac);
```

Must be called **before** `WiFi.mode()`.

### Message Structure

**C Struct** (`LightCommand`):

```cpp
typedef struct {
  uint8_t panelId;              // 1 byte - 0=all, 1-4=specific panel
  uint8_t mode;                 // 1 byte - 0=direct, 1=sequence, 2=group
  uint8_t sequenceId;           // 1 byte - sequence identifier
  uint8_t groupId;              // 1 byte - group identifier
  uint8_t effectType;           // 1 byte - 0-5 (static to fade_out)
  uint8_t brightness;           // 1 byte - 0-255
  bool regions[MAX_REGIONS];    // 20 bytes (MAX_REGIONS = 20)
  uint8_t speed;                // 1 byte - 0-100
  uint8_t step;                 // 1 byte - multi-step sequence number
  bool audioReactive;           // 1 byte
  uint8_t audioIntensity;       // 1 byte - 0-255
} LightCommand;                 // Total: 30 bytes
```

**Memory Layout**:

```
Offset  | Field          | Size
--------|----------------|-------
0       | panelId        | 1 byte
1       | mode           | 1 byte
2       | sequenceId     | 1 byte
3       | groupId        | 1 byte
4       | effectType     | 1 byte
5       | brightness     | 1 byte
6-25    | regions[0-19]  | 20 bytes
26      | speed          | 1 byte
27      | step           | 1 byte
28      | audioReactive  | 1 byte
29      | audioIntensity | 1 byte
```

**Advantages**:

- Fixed size (no dynamic allocation)
- Binary encoding (efficient)
- Direct memory copy (fast)
- Well below ESP-NOW 250-byte limit
- Supports up to 20 regions across 4 panels

### WiFi Channel Requirements

**Critical**: All ESP-NOW devices must be on the same WiFi channel.

**Master**:

- Channel determined by router connection
- Cannot be manually set (connected mode)

**Panels**:

- Must explicitly set channel to match master
- Use `esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE)`

**Configuration Constant** (in `include/common.h`):

```cpp
#define ESPNOW_WIFI_CHANNEL 6  // Update to match router
```

**Channel Discovery Process**:

1. Upload master firmware
2. Check serial output: `Master WiFi Channel: X`
3. Update `ESPNOW_WIFI_CHANNEL` to match `X`
4. Re-upload all panel firmware

### Peer Registration (Master)

**Add Panel Peers**:

```cpp
esp_now_peer_info_t panel1;
memset(&panel1, 0, sizeof(esp_now_peer_info_t));
memcpy(panel1.peer_addr, panel1_mac, 6);
panel1.channel = 0;  // Use current channel
panel1.encrypt = false;

esp_err_t result = esp_now_add_peer(&panel1);
if (result == ESP_OK) {
  Serial.println("Panel 1 added");
}
```

Repeat for panels 2, 3, 4.

**Broadcast Address** (alternative):

```cpp
uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
esp_now_add_peer(&broadcast_peer);
```

Sends to all ESP-NOW devices in range (not selective).

### Sending Data (Master)

**Unicast to Specific Panel**:

```cpp
esp_now_send(panel2_mac, (uint8_t*)&ledCmd, sizeof(ledCmd));
```

**Broadcast to All Panels**:

```cpp
esp_now_send(broadcast_mac, (uint8_t*)&ledCmd, sizeof(ledCmd));
```

**Send Callback**:

```cpp
void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    Serial.println("Delivery success");
  } else {
    Serial.println("Delivery fail");
  }
}
```

Register: `esp_now_register_send_cb(espnow_send_cb);`

### Receiving Data (Panel)

**Receive Callback**:

```cpp
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  if (len == sizeof(LightCommand)) {
    LightCommand cmd;
    memcpy(&cmd, incomingData, sizeof(LightCommand));

    // Check if command is for this panel
    if (cmd.panelId == 0 || cmd.panelId == PANEL_ID) {
      memcpy(&currentState, &cmd, sizeof(LightCommand));
      lastCommandTime = millis();
      Serial.printf("✓ Command accepted - Mode: %d, Effect: %d, Brightness: %d\n", 
                    cmd.mode, cmd.effectType, cmd.brightness);
    } else {
      Serial.printf("Command ignored (panelId %d != %d)\n", cmd.panelId, PANEL_ID);
    }
  } else {
    Serial.printf("⚠ Invalid packet size: %d (expected %d)\n", len, sizeof(LightCommand));
  }
}
```

Register: `esp_now_register_recv_cb(OnDataRecv);`

### Error Handling

**Send Failures**:

- `ESP_NOW_SEND_FAIL` → Panel offline, out of range, or channel mismatch
- No retry mechanism in current implementation
- App/master assumes best-effort delivery

**Receive Validation**:

- Check payload size matches struct
- Validate `panelId` matches this panel
- Ignore malformed packets

### Performance Characteristics

**Latency**:

- Master to panel: <10ms (typical)
- Includes serialization, transmission, deserialization

**Throughput**:

- Max packet rate: ~100 Hz (10ms intervals)
- Payload size: 13 bytes (well below 250-byte limit)
- No congestion expected for this application

**Range**:

- Line-of-sight: ~200m
- Indoor (typical): ~50m
- Same as WiFi (2.4 GHz ISM band)

**Reliability**:

- No ACK at ESP-NOW layer (send callback indicates radio TX only)
- Packet loss possible in noisy RF environments
- Mitigation: Send duplicate commands for critical changes

## Security Considerations

### Current Implementation (Development)

**Vulnerabilities**:

- MQTT: No encryption, anyone can publish commands
- ESP-NOW: No encryption, packets visible to sniffers
- No authentication or authorization

**Risk Level**: Medium (local network only, public MQTT broker)

### Production Hardening

#### MQTT Security

1. **TLS Encryption**:

   ```cpp
   WiFiClientSecure espClient;
   espClient.setCACert(ca_cert);  // Validate broker certificate
   ```

2. **Authentication**:

   ```cpp
   client.connect("master", "username", "password");
   ```

3. **Access Control**:
   - Use Mosquitto ACL file
   - Restrict topic publish/subscribe by user

#### ESP-NOW Security

1. **Encryption** (shared key):

   ```cpp
   panel1.encrypt = true;
   memcpy(panel1.lmk, encryption_key, 16);  // 128-bit key
   esp_now_set_pmk((uint8_t*)"primary_master_key");
   ```

2. **MAC Filtering**:
   - Panel validates sender MAC address
   - Only accept commands from known master

3. **Command Validation**:
   - Add checksum or HMAC to `LEDCommand` struct
   - Reject tampered packets

### Network Isolation

- Run on isolated VLAN
- Firewall rules to block external MQTT access
- VPN for remote control

## Debugging Communication

### MQTT Debugging

**Test with CLI**:

```bash
# Subscribe to all topics
mosquitto_sub -h test.mosquitto.org -t "ta25stage/#" -v

# Publish test command (all panels, breathing effect)
mosquitto_pub -h test.mosquitto.org -t "ta25stage/command" \
  -m '{"panelId":0,"mode":0,"regions":[0,1,2,3,4],"effectType":1,"brightness":200,"speed":50}'

# Publish to Panel 2 (wave effect on specific regions)
mosquitto_pub -h test.mosquitto.org -t "ta25stage/command" \
  -m '{"panelId":2,"mode":0,"regions":[0,2,4],"effectType":2,"brightness":180,"speed":70}'

# Panel 1 fade in
mosquitto_pub -h test.mosquitto.org -t "ta25stage/command" \
  -m '{"panelId":1,"mode":0,"regions":[0,1,2,3,4],"effectType":4,"brightness":255,"speed":30}'

# Monitor master status
mosquitto_sub -h test.mosquitto.org -t "ta25stage/master/status" -v
```

**Monitor Master Serial**:

```
✓ MQTT connected
📨 Received MQTT message on ta25stage/command
Parsed: Panel=0, Mode=0, Effect=1, Brightness=200
Active regions: 0 1 2 3 4 
ESP-NOW command sent: Mode=0, Effect=1, Panel=0
```

### ESP-NOW Debugging

**Master Serial Output**:

```
Sending to Panel 1 (AA:AA:AA:AA:AA:01)... Success
Sending to Panel 2 (AA:AA:AA:AA:AA:02)... Success
Sending to Panel 3 (AA:AA:AA:AA:AA:03)... Fail
ESP-NOW command sent: Mode=0, Effect=1, Panel=0
```

**Panel Serial Output**:

```
Received command from: DE:AD:BE:EF:00:01 (14 bytes)
✓ Command accepted - Mode: 0, Effect: 1, Brightness: 200
Executing effect_breathing (5 active regions)
```

**Common Issues**:

| Symptom                    | Cause                 | Solution                      |
| -------------------------- | --------------------- | ----------------------------- |
| "Delivery fail" for all    | WiFi channel mismatch | Update `ESPNOW_WIFI_CHANNEL`  |
| "Delivery fail" for one    | Panel powered off     | Check panel power and serial  |
| Panel receives but ignores | `panelId` mismatch    | Check command `panelId` field |
| No "Received from" message | Panel not listening   | Verify ESP-NOW initialized    |

### Packet Sniffing (Advanced)

**Tools**:

- Wireshark with ESP-NOW dissector
- tcpdump on access point

**Filter for ESP-NOW**:

```
wlan.fc.type == 2 && wlan.fc.subtype == 13
```

Shows raw ESP-NOW data frames.

## Protocol Extensions (Future)

### Bidirectional Communication

**Panel → Master Status**:

- Battery level (if wireless)
- Temperature sensor readings
- Error codes

**Master → Panel OTA**:

- Firmware updates over ESP-NOW
- Configuration changes

### Multi-Master Coordination

**Synchronization**:

- Time-sync via NTP
- Coordinated pattern changes
- Load balancing across masters

### Audio Reactivity

**`stage/audio` Topic**:

```json
{
  "intensity": 200,
  "frequency": 2000,
  "beat": true
}
```

Passed through to panels via `audioIntensity` field.

## Related Documentation

- **[AGENTS.md](../AGENTS.md)** - Firmware architecture
- **[hardware.md](hardware.md)** - Wiring and components
- **[troubleshooting.md](troubleshooting.md)** - Debug guide
