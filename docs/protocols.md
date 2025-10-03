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

| Topic                  | Direction    | Purpose                       | Retained | QoS |
| ---------------------- | ------------ | ----------------------------- | -------- | --- |
| `stage/command`        | App → Master | Broadcast to all panels       | No       | 0   |
| `stage/panel1/command` | App → Master | Control panel 1 only          | No       | 0   |
| `stage/panel2/command` | App → Master | Control panel 2 only          | No       | 0   |
| `stage/panel3/command` | App → Master | Control panel 3 only          | No       | 0   |
| `stage/panel4/command` | App → Master | Control panel 4 only          | No       | 0   |
| `stage/audio`          | App → Master | Audio intensity data (future) | No       | 0   |
| `stage/status`         | Master → App | System status (future)        | Yes      | 0   |

### Command Payload Format

**JSON Schema**:

```json
{
  "panelId": 0,
  "patternId": 2,
  "brightness": 200,
  "regions": [1, 2, 3, 4, 5, 6, 7],
  "speed": 50,
  "audioReactive": false
}
```

**Field Specifications**:

| Field           | Type    | Range      | Required | Description                              |
| --------------- | ------- | ---------- | -------- | ---------------------------------------- |
| `panelId`       | integer | 0-4        | Yes      | 0 = all panels, 1-4 = specific panel     |
| `patternId`     | integer | 0-4        | Yes      | Pattern ID to execute                    |
| `brightness`    | integer | 0-255      | No\*     | Brightness (used only in pattern 0)      |
| `regions`       | array   | [1-7]      | Yes      | List of enabled regions                  |
| `speed`         | integer | 0-100      | No       | Animation speed (0=slowest, 100=fastest) |
| `audioReactive` | boolean | true/false | No       | Enable audio modulation (future)         |

\*Required for pattern 0, optional for others

### Example Commands

#### All Panels, Static Pattern

```json
{
  "panelId": 0,
  "patternId": 0,
  "brightness": 255,
  "regions": [1, 2, 3, 4, 5, 6, 7]
}
```

#### Panel 2, Breathing Effect, Medium Speed

```json
{
  "panelId": 2,
  "patternId": 1,
  "regions": [1, 3, 5, 7],
  "speed": 50
}
```

#### Panel 1, Wave Effect, Fast, Selective Regions

```json
{
  "panelId": 1,
  "patternId": 2,
  "regions": [2, 4, 6],
  "speed": 80
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
  StaticJsonDocument<512> doc;
  deserializeJson(doc, payload, length);

  LEDCommand ledCmd;
  ledCmd.panelId = doc["panelId"] | 0;
  ledCmd.patternId = doc["patternId"] | 0;
  ledCmd.brightness = doc["brightness"] | 128;
  // ... parse regions array
  ledCmd.speed = doc["speed"] | 50;
  ledCmd.audioReactive = doc["audioReactive"] | false;

  // Send via ESP-NOW
  esp_now_send(broadcastAddress, (uint8_t*)&ledCmd, sizeof(ledCmd));
}
```

**Topic Subscription**:

```cpp
client.subscribe("stage/command");
client.subscribe("stage/panel1/command");
client.subscribe("stage/panel2/command");
client.subscribe("stage/panel3/command");
client.subscribe("stage/panel4/command");
```

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

**C Struct** (`LEDCommand`):

```cpp
typedef struct {
  uint8_t panelId;              // 1 byte
  uint8_t patternId;            // 1 byte
  uint8_t brightness;           // 1 byte
  bool regions[NUM_REGIONS];    // 7 bytes (NUM_REGIONS = 7)
  uint8_t speed;                // 1 byte
  bool audioReactive;           // 1 byte
  uint8_t audioIntensity;       // 1 byte
} LEDCommand;                   // Total: 13 bytes
```

**Memory Layout**:

```
Offset  | Field          | Size
--------|----------------|-----
0       | panelId        | 1 byte
1       | patternId      | 1 byte
2       | brightness     | 1 byte
3-9     | regions[0-6]   | 7 bytes
10      | speed          | 1 byte
11      | audioReactive  | 1 byte
12      | audioIntensity | 1 byte
```

**Advantages**:

- Fixed size (no dynamic allocation)
- Binary encoding (efficient)
- Direct memory copy (fast)

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
  if (len == sizeof(LEDCommand)) {
    LEDCommand cmd;
    memcpy(&cmd, incomingData, sizeof(LEDCommand));

    // Check if command is for this panel
    if (cmd.panelId == 0 || cmd.panelId == PANEL_ID) {
      memcpy(&currentState, &cmd, sizeof(LEDCommand));
      Serial.printf("Command accepted - Pattern: %d\n", cmd.patternId);
    } else {
      Serial.println("Command ignored (wrong panel ID)");
    }
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
mosquitto_sub -h test.mosquitto.org -t "stage/#" -v

# Publish test command
mosquitto_pub -h test.mosquitto.org -t "stage/command" -m '{"panelId":0,"patternId":0,"brightness":255,"regions":[1,2,3,4,5,6,7]}'
```

**Monitor Master Serial**:

```
MQTT connected
Received message on topic: stage/command
Parsed command: Panel=0, Pattern=0, Brightness=255
Sending via ESP-NOW...
```

### ESP-NOW Debugging

**Master Serial Output**:

```
Sending to Panel 1 (AA:AA:AA:AA:AA:01)... Success
Sending to Panel 2 (AA:AA:AA:AA:AA:02)... Success
Sending to Panel 3 (AA:AA:AA:AA:AA:03)... Fail
```

**Panel Serial Output**:

```
Received from: DE:AD:BE:EF:00:01 (master MAC)
Command accepted - Pattern: 2 Brightness: 200
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
