# LED Stage Control - Firmware Development Guide

Technical documentation for AI coding agents and developers working on the ESP32 firmware.

## Project Structure

```
ta25/
├── platformio.ini          # Build configurations (5 environments)
├── include/
│   └── common.h            # Shared structs, constants, MAC addresses
├── src/
│   ├── master/
│   │   └── main.cpp        # Master: MQTT subscriber + ESP-NOW sender
│   └── panel/
│       └── main.cpp        # Panel: ESP-NOW receiver + PWM controller
├── justfile                # Build automation shortcuts
└── docs/                   # Hardware specs and protocols
```

## Build System

### PlatformIO Environments

| Environment | Description                | Build Flag      |
|-------------|----------------------------|-----------------|
| `master`    | Master controller          | None            |
| `panel1`    | Panel 1                    | `PANEL_ID=1`    |
| `panel2`    | Panel 2                    | `PANEL_ID=2`    |
| `panel3`    | Panel 3                    | `PANEL_ID=3`    |
| `panel4`    | Panel 4                    | `PANEL_ID=4`    |

### Build Flags

- **Panels**: `-D PANEL_ID=X` differentiates panel identity at compile time
- **MQTT**: `MQTT_MAX_PACKET_SIZE=1024` for JSON payloads
- **PWM**: 5kHz frequency (flicker-free), 8-bit resolution

### Dependencies (from platformio.ini)

- `knolleary/PubSubClient` - MQTT client
- `bblanchon/ArduinoJson` - JSON parsing
- `espressif/arduino-esp32` - ESP32 Arduino core

## Firmware Architecture

### Master Controller (`src/master/main.cpp`)

**Responsibilities:**
1. Connect to WiFi network
2. Subscribe to MQTT topics (`stage/command`, `stage/panel[1-4]/command`, `stage/audio`)
3. Parse JSON commands
4. Broadcast to panels via ESP-NOW

**Key Functions:**

```cpp
void setup_wifi()          // WiFi STA mode connection with status tracking
void checkWiFiStatus()     // Periodic WiFi health check and reconnection
void publishHeartbeat()    // Publish status to ta25stage/status (30s interval)
void mqttCallback()        // MQTT message handler
void onDataSent()          // ESP-NOW send status callback
void reconnect()           // Progressive retry logic with backoff
void enterLowPowerMode()   // Deep sleep after max failures (5 min)
void loop()                // MQTT loop + heartbeat + WiFi monitoring
```

**Connection Management:**
- Progressive retry timeouts: 5s → 15s → 25s → ... → 120s max
- WiFi reconnection after 5 MQTT failures
- Deep sleep after 10 consecutive MQTT failures
- Heartbeat publishing every 30 seconds to `ta25stage/status`
- WiFi health check every 60 seconds

**ESP-NOW Setup:**
- Uses factory MAC address
- Registers 4 panel peers (custom MAC addresses)
- Broadcasts to all panels or specific panel based on `panelId`

**WiFi Channel Validation:**
- Checks if router's WiFi channel matches `ESPNOW_WIFI_CHANNEL`
- Warns on mismatch (ESP-NOW won't work across channels)

### Panel Controller (`src/panel/main.cpp`)

**Responsibilities:**
1. Set custom MAC address (`AA:AA:AA:AA:AA:0X`)
2. Initialize WiFi in STA mode (no AP connection)
3. Set WiFi channel to match master
4. Receive ESP-NOW commands
5. Execute LED patterns via PWM

**Key Functions:**

```cpp
void onDataRecv()                 // ESP-NOW receive callback with timestamp
void setRegionBrightness()        // PWM control for one region
void executePattern()             // Pattern dispatcher with watchdog tracking
void checkCommandTimeout()        // Warn if no commands for 5 minutes
void printHeartbeat()             // Status logging every 60 seconds
void pattern_allOn()              // Pattern 0: Static
void pattern_breathing()          // Pattern 1: Breathing
void pattern_wave()               // Pattern 2: Wave/Chase
void pattern_pulse()              // Pattern 3: Pulse
void loop()                       // Pattern execution + monitoring
```

**Status Monitoring:**
- Tracks last command received timestamp
- Logs heartbeat every 60 seconds (uptime, pattern, loop count, heap)
- Warns if no commands received for 5 minutes
- Pattern execution watchdog (5s timeout detection ready)

**Custom MAC Address:**
- Set via `esp_wifi_set_mac(WIFI_IF_STA, panelX_mac)`
- Allows master to address specific panels
- Must be set before WiFi init

**GPIO Pin Mapping:**

```cpp
const uint8_t regionPins[NUM_REGIONS] = {25, 26, 27, 14, 12, 13, 15};
// Region 1-7 → GPIO 25, 26, 27, 14, 12, 13, 15
```

**PWM Setup:**
- 5000 Hz frequency
- 8-bit resolution (0-255)
- Each region gets dedicated PWM channel (0-6)

## Communication Flow

### MQTT → ESP-NOW Flow

1. **App/Web** publishes JSON to `stage/command` (MQTT)
2. **Master** receives via `mqtt_callback()`
3. **Master** parses JSON into `LEDCommand` struct
4. **Master** broadcasts struct via ESP-NOW to panels
5. **Panels** receive in `OnDataRecv()`
6. **Panels** validate `panelId` (0 = all, or matches own ID)
7. **Panels** update `currentState` and execute pattern

### LEDCommand Struct

```cpp
typedef struct {
  uint8_t panelId;              // 0 = broadcast, 1-4 = specific
  uint8_t patternId;            // 0-4 (pattern function index)
  uint8_t brightness;           // 0-255 (used in Pattern 0)
  bool regions[NUM_REGIONS];    // 7 booleans (region enable flags)
  uint8_t speed;                // 0-100 (mapped to timing intervals)
  bool audioReactive;           // Future: audio modulation flag
  uint8_t audioIntensity;       // Future: audio level 0-255
} LEDCommand;
```

**Size**: ~18 bytes (fits well in ESP-NOW 250-byte limit)

### ESP-NOW Configuration

**Custom MAC Addresses** (defined in `include/common.h`):

```cpp
uint8_t panel1_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x01};
uint8_t panel2_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x02};
uint8_t panel3_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x03};
uint8_t panel4_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x04};
uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
```

**Critical**: All devices must be on the same WiFi channel:

```cpp
#define ESPNOW_WIFI_CHANNEL 6  // Must match router's channel
```

**Master**: WiFi channel determined by router connection  
**Panels**: Explicitly set via `esp_wifi_set_channel()`

## Pattern System

### Pattern Execution Loop

```cpp
void loop() {
  executePattern();  // Called continuously (no delay)
}

void executePattern() {
  switch(currentState.patternId) {
    case 0: pattern_allOn(); break;
    case 1: pattern_breathing(); break;
    case 2: pattern_wave(); break;
    case 3: pattern_pulse(); break;
    case 4: pattern_flicker(); break;
  }
}
```

### Pattern Implementation Guidelines

All patterns follow this structure:

```cpp
void pattern_example() {
  static unsigned long lastUpdate = 0;
  unsigned long currentMillis = millis();
  
  // Map speed (0-100) to update interval (fast to slow)
  uint16_t interval = map(currentState.speed, 0, 100, MIN_DELAY, MAX_DELAY);
  
  if (currentMillis - lastUpdate >= interval) {
    lastUpdate = currentMillis;
    
    // Update logic here
    for (int r = 0; r < NUM_REGIONS; r++) {
      if (currentState.regions[r]) {
        uint8_t brightness = /* calculate brightness */;
        setRegionBrightness(r, brightness);
      }
    }
  }
}
```

**Key Points:**
- Use `static` variables to maintain state between calls
- Non-blocking (no `delay()`)
- Check enabled regions before updating
- Use `map()` to scale `speed` parameter to timing

### Existing Patterns

#### Pattern 0: All On (Static)

- Uses `currentState.brightness` directly
- No animation, just PWM write
- All enabled regions at same brightness

#### Pattern 1: Breathing

- Sine wave brightness modulation
- Speed controls frequency
- Synchronized across all regions

#### Pattern 2: Wave/Chase

- One region lit at a time
- Cycles through enabled regions sequentially
- Speed controls transition rate

#### Pattern 3: Pulse

- Quick on/off toggle
- Can be audio-reactive (future)
- All regions synchronized

#### Pattern 4: Flicker

- Random brightness per region
- Simulates candle/fire effect
- Speed controls update frequency

### Adding New Patterns

1. **Define pattern function** in `src/panel/main.cpp`:

```cpp
void pattern_myNewEffect() {
  static unsigned long lastUpdate = 0;
  static uint8_t step = 0;
  
  uint16_t interval = map(currentState.speed, 0, 100, 500, 50);
  
  if (millis() - lastUpdate >= interval) {
    lastUpdate = millis();
    
    for (int r = 0; r < NUM_REGIONS; r++) {
      if (currentState.regions[r]) {
        uint8_t brightness = /* your logic */;
        setRegionBrightness(r, brightness);
      }
    }
    
    step++;
  }
}
```

2. **Add to switch** in `executePattern()`:

```cpp
case 5:
  pattern_myNewEffect();
  break;
```

3. **Upload to all panels**

## Development Workflow

### Testing Changes

1. **Upload to one panel first**: `just up s1`
2. **Monitor serial output**: `just mon s1`
3. **Send test MQTT command** via MQTT client
4. **Verify pattern behavior**
5. **Upload to remaining panels**: `just up s2 && just up s3 && just up s4`

### Debugging Techniques

#### Master Debugging

Enable verbose ESP-NOW status:

```cpp
void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.printf("Send to %02X:%02X:%02X:%02X:%02X:%02X - %s\n",
    mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5],
    status == ESP_NOW_SEND_SUCCESS ? "Success" : "Fail");
}
```

#### Panel Debugging

Print received commands:

```cpp
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  Serial.printf("Received from %02X:%02X:..., Panel ID: %d, Pattern: %d\n",
    mac[0], mac[1], cmd.panelId, cmd.patternId);
}
```

#### Monitor Multiple Panels

Use `tmux` or multiple terminals:

```bash
# Terminal 1
just mon m

# Terminal 2
just mon s1

# Terminal 3
just mon s2
```

### Performance Monitoring

**Pattern Update Rate**: Measure in each pattern function:

```cpp
static unsigned long frameCount = 0;
static unsigned long lastReport = 0;

if (millis() - lastReport >= 1000) {
  Serial.printf("FPS: %lu\n", frameCount);
  frameCount = 0;
  lastReport = millis();
}
frameCount++;
```

**ESP-NOW Latency**: Timestamp in master send callback and panel receive callback.

## WiFi Channel Management

### Why It Matters

ESP-NOW uses WiFi radio at the data link layer. Both sender and receiver must be on the same channel to communicate.

**Master**: Channel determined by router (can't be controlled)  
**Panels**: Must explicitly match master's channel

### Channel Configuration Flow

1. **Master connects to WiFi** → Router assigns channel (e.g., 11)
2. **Master reads channel**: `wifi_second_chan_t second; esp_wifi_get_channel(&primaryChan, &second);`
3. **Master prints**: `Master WiFi Channel: 11`
4. **Developer updates** `include/common.h`: `#define ESPNOW_WIFI_CHANNEL 11`
5. **Panels set channel**: `esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);`

### Validation Logic

**Master** (warns on mismatch):

```cpp
if (primaryChan != ESPNOW_WIFI_CHANNEL) {
  Serial.println("WARNING: WiFi CHANNEL MISMATCH!");
  Serial.printf("Expected: %d, Actual: %d\n", ESPNOW_WIFI_CHANNEL, primaryChan);
}
```

**Panel** (confirms setting):

```cpp
esp_err_t err = esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
if (err == ESP_OK) {
  Serial.printf("✓ WiFi channel set to: %d\n", ESPNOW_WIFI_CHANNEL);
}
```

## Code Conventions

### Naming

- **Global variables**: `camelCase` (e.g., `currentState`, `regionPins`)
- **Functions**: `camelCase` (e.g., `setRegionBrightness()`)
- **Constants**: `UPPER_SNAKE_CASE` (e.g., `NUM_REGIONS`, `ESPNOW_WIFI_CHANNEL`)
- **Structs**: `PascalCase` (e.g., `LEDCommand`)

### Serial Output

- **Info**: `Serial.println("info message")`
- **Success**: `Serial.println("✓ Success message")`
- **Warning**: `Serial.println("⚠ Warning message")`
- **Error**: `Serial.println("✗ Error message")`

### Memory Management

- **Static buffers**: Use fixed-size arrays (no dynamic allocation)
- **String handling**: Prefer `const char*` over `String` objects
- **JSON parsing**: Use `StaticJsonDocument` with appropriate size

## Common Modifications

### Change GPIO Pins

Edit `src/panel/main.cpp`:

```cpp
const uint8_t regionPins[NUM_REGIONS] = {25, 26, 27, 14, 12, 13, 15};
```

### Add More Regions

1. Update `include/common.h`: `#define NUM_REGIONS 10`
2. Add pins in `src/panel/main.cpp`: `{25, 26, 27, 14, 12, 13, 15, 32, 33, 34}`
3. Update `LEDCommand` struct handling

### Change PWM Frequency

Edit `src/panel/main.cpp` in `setup()`:

```cpp
ledcSetup(i, 5000, 8);  // 5000 Hz → change to desired frequency
```

### Add More Panels

1. Define MAC in `include/common.h`: `uint8_t panel5_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x05};`
2. Add environment in `platformio.ini`:
   ```ini
   [env:panel5]
   build_flags = ${common.build_flags} -D PANEL_ID=5
   ```
3. Register peer in master `setup()`: `esp_now_add_peer(&panel5);`
4. Update `justfile` aliases

### Modify MQTT Topics

Edit `src/master/main.cpp`:

```cpp
client.subscribe("stage/command");
client.subscribe("stage/panel1/command");
// Add more topics
```

## Testing Without Hardware

### Master (MQTT Only)

Comment out ESP-NOW send in `mqtt_callback()`:

```cpp
// esp_now_send(broadcastAddress, (uint8_t *)&ledCmd, sizeof(ledCmd));
Serial.println("Would send ESP-NOW command");
```

### Panel (ESP-NOW Simulator)

Create fake commands in `setup()`:

```cpp
LEDCommand testCmd = {0, 1, 255, {true, true, true, true, true, true, true}, 50, false, 0};
memcpy(&currentState, &testCmd, sizeof(LEDCommand));
```

### PWM Verification (Without LEDs)

Measure GPIO with multimeter:
- 0% duty → 0V
- 50% duty → ~1.65V average
- 100% duty → 3.3V

## Performance Characteristics

- **MQTT Latency**: 50-200ms (network dependent)
- **ESP-NOW Latency**: <10ms (master to panel)
- **Pattern Update Rate**: 10-100 Hz (pattern dependent)
- **PWM Frequency**: 5 kHz (flicker-free)
- **WiFi Range**: ~50m indoor
- **ESP-NOW Range**: ~200m line-of-sight

## Security Notes

**Current Implementation** (development):
- Public MQTT broker (test.mosquitto.org)
- No ESP-NOW encryption
- No authentication

**Production Recommendations**:
- Local Mosquitto broker with TLS
- ESP-NOW encryption (shared key)
- MQTT authentication (username/password)
- VPN for remote access

## Future Development

### Planned Features
- [ ] Audio reactivity via I2S microphone
- [ ] OTA firmware updates
- [ ] Pattern presets stored in SPIFFS
- [ ] Web-based pattern designer
- [ ] DMX512 protocol support

### Code Improvements
- [ ] Refactor patterns into class hierarchy
- [ ] Implement pattern interpolation
- [ ] Add state machine for transitions
- [ ] Optimize ESP-NOW packet structure
- [ ] Implement graceful degradation

## Related Documentation

- **[README.md](README.md)** - Quick start and setup
- **[docs/hardware.md](docs/hardware.md)** - Wiring and components
- **[docs/protocols.md](docs/protocols.md)** - MQTT and ESP-NOW specs
- **[docs/troubleshooting.md](docs/troubleshooting.md)** - Debug guide
