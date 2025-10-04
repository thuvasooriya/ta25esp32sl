# LED Stage Control - Firmware Development Guide

Technical documentation for AI coding agents and developers working on the ESP32 firmware.

## Project Structure

```
ta25/
├── platformio.ini          # Build configurations (5 environments)
├── include/
│   ├── common.h            # Shared structs, constants, MAC addresses
│   └── panel_config.h      # Panel region configurations and metadata
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
2. Subscribe to MQTT topic (`ta25stage/command`)
3. Parse JSON commands into `LightCommand` struct
4. Broadcast commands to panels via ESP-NOW
5. Publish heartbeat status to `ta25stage/master/status`

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
4. Receive ESP-NOW commands (`LightCommand` struct)
5. Execute LED effects via PWM on configured regions

**Key Functions:**

```cpp
void onDataRecv()                 // ESP-NOW receive callback with timestamp
void setRegionBrightness()        // PWM control for one region
void initPWMChannels()            // Dynamic PWM channel initialization
void executeEffect()              // Effect dispatcher
void checkCommandTimeout()        // Warn if no commands for 5 minutes
void printHeartbeat()             // Status logging every 60 seconds
void printRegionConfig()          // Display panel region layout on boot
void effect_static()              // Effect 0: Static brightness
void effect_breathing()           // Effect 1: Breathing
void effect_wave()                // Effect 2: Wave/Chase
void effect_pulse()               // Effect 3: Pulse
void effect_fade_in()             // Effect 4: Fade in
void effect_fade_out()            // Effect 5: Fade out
void loop()                       // Effect execution + monitoring
```

**Status Monitoring:**
- Tracks last command received timestamp
- Logs heartbeat every 60 seconds (uptime, effect, loop count, heap, region count)
- Warns if no commands received for 5 minutes
- Effect execution watchdog (5s timeout detection ready)

**Custom MAC Address:**
- Set via `esp_wifi_set_mac(WIFI_IF_STA, panelX_mac)`
- Allows master to address specific panels
- Must be set before WiFi init

**Panel Configuration System:**
- Region metadata defined in `include/panel_config.h`
- Each panel has unique region count and GPIO pins
- Panel 1: 5 regions, Panel 2: 6 regions, Panel 3: 5 regions, Panel 4: 4 regions
- Total system: 20 regions across 4 panels
- Each region has: GPIO pin, name, vertical position, local group, cross-panel group
- Configuration selected at compile time via `PANEL_ID` macro

**Region Metadata Structure:**

```cpp
typedef struct {
  uint8_t pin;              // GPIO pin number
  const char* name;         // Region name (e.g., "BULL_SYMBOL")
  uint8_t verticalPos;      // Vertical position (0=top)
  uint8_t localGroup;       // Panel-local group ID
  uint8_t crossPanelGroup;  // Cross-panel group ID
} RegionConfig;
```

**PWM Setup:**
- 5000 Hz frequency
- 8-bit resolution (0-255)
- Each region gets dedicated PWM channel
- Dynamic channel allocation based on panel's region count

## Communication Flow

### MQTT → ESP-NOW Flow

1. **App/Web** publishes JSON to `ta25stage/command` (MQTT)
2. **Master** receives via `mqttCallback()`
3. **Master** parses JSON into `LightCommand` struct
4. **Master** broadcasts struct via ESP-NOW to panels (or specific panel)
5. **Panels** receive in `onDataRecv()`
6. **Panels** validate `panelId` (0 = all, or matches own ID)
7. **Panels** update `currentState` and execute effect

### LightCommand Struct

```cpp
typedef struct {
  uint8_t panelId;              // 0 = broadcast, 1-4 = specific panel
  uint8_t mode;                 // 0=direct regions, 1=sequence, 2=group
  uint8_t sequenceId;           // Sequence identifier (mode=1)
  uint8_t groupId;              // Group identifier (mode=2)
  uint8_t effectType;           // 0-5 (effect function index)
  uint8_t brightness;           // 0-255 target brightness
  bool regions[MAX_REGIONS];    // 20 booleans (region enable flags)
  uint8_t speed;                // 0-100 (mapped to timing intervals)
  uint8_t step;                 // Multi-step sequence number
  bool audioReactive;           // Audio modulation flag
  uint8_t audioIntensity;       // Audio level 0-255
} LightCommand;
```

**Size**: 30 bytes (fits well in ESP-NOW 250-byte limit)

**Modes:**
- **0 (MODE_DIRECT_REGIONS)**: Direct region control - app specifies exact regions via `regions[]` array
- **1 (MODE_SEQUENCE)**: Orchestrated sequence - master calculates regions based on `sequenceId` (future)
- **2 (MODE_GROUP)**: Group-based - master calculates regions by `groupId` (future)

**Effect Types:**
- **0 (EFFECT_STATIC)**: Solid brightness
- **1 (EFFECT_BREATHING)**: Sine wave fade
- **2 (EFFECT_WAVE)**: Chase effect
- **3 (EFFECT_PULSE)**: Quick pulses
- **4 (EFFECT_FADE_IN)**: Gradual fade in
- **5 (EFFECT_FADE_OUT)**: Gradual fade out

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

## Effect vs. Sequence Terminology

**Effect**: Low-level visual animation executed on panels (breathing, wave, fade, etc.)
- Runs locally on each panel
- Operates on regions specified by command
- Examples: breathing, wave, pulse, fade_in, fade_out

**Sequence**: High-level choreographed narrative orchestrated by master (future feature)
- Multi-step storytelling with coordinated timing
- Master calculates which regions to light at each step
- Examples: vertical sweep, symbolic narrative, audio-reactive sequences

Current implementation focuses on **effects**. Sequences are planned for future development.

## Effect System

### Effect Execution Loop

```cpp
void loop() {
  executeEffect();  // Called continuously (no delay)
}

void executeEffect() {
  switch(currentState.effectType) {
    case EFFECT_STATIC: effect_static(); break;
    case EFFECT_BREATHING: effect_breathing(); break;
    case EFFECT_WAVE: effect_wave(); break;
    case EFFECT_PULSE: effect_pulse(); break;
    case EFFECT_FADE_IN: effect_fade_in(); break;
    case EFFECT_FADE_OUT: effect_fade_out(); break;
  }
}
```

### Effect Implementation Guidelines

All effects follow this structure:

```cpp
void effect_example() {
  static unsigned long lastUpdate = 0;
  unsigned long currentMillis = millis();
  
  // Map speed (0-100) to update interval (fast to slow)
  uint16_t interval = map(currentState.speed, 0, 100, MIN_DELAY, MAX_DELAY);
  
  if (currentMillis - lastUpdate >= interval) {
    lastUpdate = currentMillis;
    
    // Update logic here
    uint8_t regionCount = getRegionCount();
    for (uint8_t r = 0; r < regionCount; r++) {
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
- Use `getRegionCount()` to get panel-specific region count

### Existing Effects

#### Effect 0: Static

- Uses `currentState.brightness` directly
- No animation, just PWM write
- All enabled regions at same brightness

#### Effect 1: Breathing

- Sine wave brightness modulation
- Speed controls frequency
- Synchronized across all regions

#### Effect 2: Wave/Chase

- One region lit at a time
- Cycles through enabled regions sequentially
- Speed controls transition rate

#### Effect 3: Pulse

- Quick on/off toggle
- Can be audio-reactive (future)
- All regions synchronized

#### Effect 4: Fade In

- Gradual brightness increase from 0 to target
- Speed controls fade duration
- All enabled regions synchronized

#### Effect 5: Fade Out

- Gradual brightness decrease to 0
- Speed controls fade duration
- All enabled regions synchronized

### Adding New Effects

1. **Define effect function** in `src/panel/main.cpp`:

```cpp
void effect_myNewEffect() {
  static unsigned long lastUpdate = 0;
  static uint8_t step = 0;
  
  uint16_t interval = map(currentState.speed, 0, 100, 500, 50);
  
  if (millis() - lastUpdate >= interval) {
    lastUpdate = millis();
    
    uint8_t regionCount = getRegionCount();
    for (uint8_t r = 0; r < regionCount; r++) {
      if (currentState.regions[r]) {
        uint8_t brightness = /* your logic */;
        setRegionBrightness(r, brightness);
      }
    }
    
    step++;
  }
}
```

2. **Add enum value** in `include/common.h`:

```cpp
enum EffectType {
  EFFECT_STATIC = 0,
  EFFECT_BREATHING = 1,
  // ...
  EFFECT_MY_NEW_EFFECT = 6
};
```

3. **Add to switch** in `executeEffect()`:

```cpp
case EFFECT_MY_NEW_EFFECT:
  effect_myNewEffect();
  break;
```

4. **Upload to all panels**

## Development Workflow

### Testing Changes

1. **Upload to one panel first**: `just up s1`
2. **Monitor serial output**: `just mon s1`
3. **Verify region configuration** is printed on boot
4. **Send test MQTT command** via MQTT client
5. **Verify effect behavior**
6. **Upload to remaining panels**: `just up s2 && just up s3 && just up s4`

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
void onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  Serial.printf("Received from %02X:%02X:..., Panel ID: %d, Mode: %d, Effect: %d\n",
    mac[0], mac[1], cmd.panelId, cmd.mode, cmd.effectType);
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

**Effect Update Rate**: Measure in each effect function:

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
- **Constants**: `UPPER_SNAKE_CASE` (e.g., `MAX_REGIONS`, `ESPNOW_WIFI_CHANNEL`)
- **Structs**: `PascalCase` (e.g., `LightCommand`, `RegionConfig`)
- **Enums**: `PascalCase` for type, `UPPER_SNAKE_CASE` for values (e.g., `EffectType`, `EFFECT_BREATHING`)

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

Edit `include/panel_config.h` for the specific panel:

```cpp
#if PANEL_ID == 1
const RegionConfig PROGMEM panel1Regions[] = {
  {25, "BULL_SYMBOL", 0, LGROUP_BULL, XGROUP_SYMBOL},
  {26, "BULL_HEAD", 1, LGROUP_BULL, XGROUP_NONE},
  // ... change pin numbers as needed
};
#endif
```

### Add Region to Existing Panel

1. Edit `include/panel_config.h` for the panel
2. Add new `RegionConfig` entry with GPIO pin, name, position, groups
3. Update region count: `constexpr uint8_t getRegionCount() { return X; }`
4. Ensure total regions across all panels ≤ `MAX_REGIONS` (20)

### Add More Panels

1. Define MAC in `include/common.h`: `uint8_t panel5_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x05};`
2. Add configuration block in `include/panel_config.h`:
   ```cpp
   #elif PANEL_ID == 5
   const RegionConfig PROGMEM panel5Regions[] = { /* ... */ };
   constexpr uint8_t getRegionCount() { return X; }
   #endif
   ```
3. Update `MAX_REGIONS` in `include/common.h` if needed
4. Add environment in `platformio.ini`:
   ```ini
   [env:panel5]
   build_flags = ${common.build_flags} -D PANEL_ID=5
   ```
5. Register peer in master `setup()`: `esp_now_add_peer(&panel5);`
6. Update `justfile` aliases

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
LightCommand testCmd;
testCmd.panelId = 0;
testCmd.mode = 0;
testCmd.effectType = EFFECT_BREATHING;
testCmd.brightness = 200;
testCmd.speed = 50;
for (uint8_t i = 0; i < MAX_REGIONS; i++) {
  testCmd.regions[i] = (i < getRegionCount());  // Enable all regions
}
memcpy(&currentState, &testCmd, sizeof(LightCommand));
```

### PWM Verification (Without LEDs)

Measure GPIO with multimeter:
- 0% duty → 0V
- 50% duty → ~1.65V average
- 100% duty → 3.3V

## Performance Characteristics

- **MQTT Latency**: 50-200ms (network dependent)
- **ESP-NOW Latency**: <10ms (master to panel)
- **Effect Update Rate**: 10-100 Hz (effect dependent)
- **PWM Frequency**: 5 kHz (flicker-free)
- **WiFi Range**: ~50m indoor
- **ESP-NOW Range**: ~200m line-of-sight
- **Region Count**: 20 regions total (5+6+5+4 across 4 panels)
- **Command Size**: 30 bytes (well below 250-byte ESP-NOW limit)

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

## Sequence Implementation

### Overview

Sequences are master-orchestrated choreographed narratives that coordinate multiple regions across panels. The master calculates which regions to light at each step and broadcasts commands to panels via ESP-NOW.

**Implementation location**: `src/master/main.cpp`

### Sequence Functions

All four sequences are implemented:

- **`runSequence0()`** - Test sequence (auto-runs on boot)
- **`runSequence1()`** - Vertical sweep (top-to-bottom cascade)
- **`runSequence2()`** - Group narrative (symbolic storytelling)
- **`runSequence3()`** - Symbol emergence (meaning from symbols)

### Helper Functions

```cpp
void setAllRegions(bool state, LightCommand &cmd);
void setRegionsByList(const uint8_t* regionList, uint8_t count, bool state, LightCommand &cmd);
```

### Region Group Mappings

Defined at top of `src/master/main.cpp`:

```cpp
const uint8_t SYMBOL_REGIONS[] = {0, 3, 6, 11, 15};
const uint8_t RAAVANA_HEAD_REGIONS[] = {11, 15, 16};
const uint8_t RAAVANA_REGIONS[] = {11, 15, 16, 17, 18};
const uint8_t CONTINENT_REGIONS[] = {5, 10, 14};
```

### Triggering Sequences

**Via MQTT**: Publish to `ta25stage/command` with `mode=1`:

```json
{"mode": 1, "sequenceId": 0}
```

**In Code**: Call sequence function directly (blocking):

```cpp
runSequence1();
```

### Sequence Behavior

- **Blocking**: Sequences run to completion (cannot be interrupted)
- **Auto-boot**: Sequence 0 runs automatically 3 seconds after master setup
- **Serial output**: Each sequence logs step-by-step progress
- **Status tracking**: `sequenceRunning` flag prevents overlaps

### Adding New Sequences

1. Define sequence function:
```cpp
void runSequence4() {
  Serial.println("\n=== Running Sequence 4: My New Sequence ===");
  sequenceRunning = true;
  
  LightCommand cmd;
  cmd.mode = MODE_SEQUENCE;
  cmd.sequenceId = 4;
  
  // Your sequence logic here
  
  sequenceRunning = false;
}
```

2. Add forward declaration at top of file
3. Add case to `mqttCallback()` switch statement
4. Document in `docs/panels.md`

## Future Development

### Planned Features
- [ ] Non-blocking sequence execution with MQTT interrupts
- [ ] Group-based mode (mode=2) implementation
- [ ] Audio reactivity via I2S microphone
- [ ] OTA firmware updates
- [ ] Effect presets stored in SPIFFS
- [ ] Web-based effect designer
- [ ] DMX512 protocol support

### Code Improvements
- [ ] Implement effect interpolation
- [ ] Add state machine for transitions
- [ ] Implement graceful degradation
- [ ] Add effect chaining/composition
- [ ] Add sequence pause/resume functionality

## Related Documentation

- **[README.md](README.md)** - Quick start and setup
- **[docs/hardware.md](docs/hardware.md)** - Wiring and components
- **[docs/protocols.md](docs/protocols.md)** - MQTT and ESP-NOW specs
- **[docs/troubleshooting.md](docs/troubleshooting.md)** - Debug guide
