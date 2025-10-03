# LED Stage Control System - Technical Documentation

## System Overview

This is a distributed LED stage decoration control system for live events, consisting of 5 ESP32 microcontrollers:

- **1 Master Controller**: Receives MQTT commands from app/web interfaces and broadcasts to panels via ESP-NOW
- **4 Panel Controllers**: Receive ESP-NOW commands and control warm white LED strips through PWM-driven MOSFETs

## Architecture

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

## Communication Protocols

### MQTT Topics

| Topic                  | Direction    | Purpose                      | Payload         |
| ---------------------- | ------------ | ---------------------------- | --------------- |
| `stage/command`        | App → Master | Global commands (all panels) | JSON command    |
| `stage/panel1/command` | App → Master | Panel 1 specific             | JSON command    |
| `stage/panel2/command` | App → Master | Panel 2 specific             | JSON command    |
| `stage/panel3/command` | App → Master | Panel 3 specific             | JSON command    |
| `stage/panel4/command` | App → Master | Panel 4 specific             | JSON command    |
| `stage/audio`          | App → Master | Audio intensity data         | JSON audio data |
| `stage/status`         | Master → App | System status updates        | JSON status     |

### MQTT Command Format

```

{
"panelId": 0,              // 0 = all panels, 1-4 = specific panel
"patternId": 2,            // Pattern ID (0-N)
"brightness": 200,         // 0-255
"regions": , // Active regions (1-7)
"speed": 50,               // Pattern speed (0-100)
"audioReactive": false     // Enable audio reactivity
}

```

### ESP-NOW Communication

**Message Structure** (`LEDCommand` struct):

```

typedef struct {
uint8_t panelId;              // 0 = broadcast, 1-4 = specific
uint8_t patternId;            // Pattern ID
uint8_t brightness;           // 0-255
bool regions[NUM_REGIONS];    // Region enable flags
uint8_t speed;                // 0-100
bool audioReactive;           // Audio reactive flag
uint8_t audioIntensity;       // Audio level (0-255)
} LEDCommand;

```

**Custom MAC Addresses:**

- Master: Factory MAC (varies per device)
- Panel 1: `AA:AA:AA:AA:AA:01`
- Panel 2: `AA:AA:AA:AA:AA:02`
- Panel 3: `AA:AA:AA:AA:AA:03`
- Panel 4: `AA:AA:AA:AA:AA:04`

## Hardware Configuration

### Master ESP32

- **WiFi**: Connected to network in STA+AP mode
- **MQTT**: Subscribes to command topics
- **ESP-NOW**: Broadcasts to panel MAC addresses
- **Power**: USB or 5V supply
- **No LEDs connected**

### Panel ESP32 (each)

- **WiFi**: Station mode, not connected (ESP-NOW only)
- **WiFi Channel**: Set to match master (default: 6)
- **ESP-NOW**: Receives commands from master
- **GPIO Pins**: 7 PWM outputs for LED regions
- **Power**: 5V supply (shared ground with LED power)

**Default Pin Mapping:**

```

Region 1 → GPIO 25
Region 2 → GPIO 26
Region 3 → GPIO 27
Region 4 → GPIO 14
Region 5 → GPIO 12
Region 6 → GPIO 13
Region 7 → GPIO 15

```

### LED Hardware

**Per Region:**

- Warm white LED strip (analog, non-addressable)
- N-channel MOSFET (IRLZ44N or similar)
- 10kΩ pull-down resistor (gate to ground)
- Separate 5V/12V power supply

**Wiring:**

```

ESP32 GPIO → MOSFET Gate (+ 10kΩ to GND)
MOSFET Drain → LED Strip (-)
MOSFET Source → Power Supply GND
LED Strip (+) → Power Supply (+5V or +12V)
ESP32 GND → Power Supply GND (common ground required)

```

## Software Architecture

### Project Structure

```

led_stage_panel/
├── platformio.ini          \# Build configurations
├── include/
│   └── common.h            \# Shared definitions and structs
├── src/
│   ├── master/
│   │   └── main.cpp        \# Master controller code
│   └── panel/
│       └── main.cpp        \# Panel controller code
├── justfile                \# Build commands
├── AGENTS.md               \# This file
└── README.md               \# User guide

```

### Build System

**PlatformIO Environments:**

- `master` - Master controller
- `panel1` - Panel 1 (PANEL_ID=1)
- `panel2` - Panel 2 (PANEL_ID=2)
- `panel3` - Panel 3 (PANEL_ID=3)
- `panel4` - Panel 4 (PANEL_ID=4)

**Build Flags:**

- Panels use `-D PANEL_ID=X` to differentiate
- `MQTT_MAX_PACKET_SIZE=1024` for larger JSON payloads

### Pattern System

**Pattern 0: All On**

- Static brightness control
- All enabled regions at same brightness
- User-adjustable via brightness parameter

**Pattern 1: Breathing**

- Smooth fade in/out effect
- Speed controlled by speed parameter
- All enabled regions synchronized

**Pattern 2: Wave/Chase**

- Sequential region activation
- One region lit at a time
- Speed controlled by speed parameter

**Pattern 3: Pulse**

- Quick on/off pulse effect
- Can be audio-reactive
- All enabled regions synchronized

**Pattern 4: Flicker**

- Random brightness variations
- Simulates candle/fire effect
- Speed controls update rate

### Adding New Patterns

1. Add pattern function to `src/panel/main.cpp`:

```

void pattern_newEffect() {
static unsigned long lastUpdate = 0;
unsigned long currentMillis = millis();
uint16_t updateInterval = map(currentState.speed, 0, 100, 500, 50);

if (currentMillis - lastUpdate >= updateInterval) {
lastUpdate = currentMillis;

    // Your pattern logic here
    for (int r = 0; r < NUM_REGIONS; r++) {
      if (currentState.regions[r]) {
        // Set brightness for this region
        setRegionBrightness(r, yourBrightnessValue);
      }
    }
    }
}

```

2. Add to switch statement in `executePattern()`:

```

case 5:
pattern_newEffect();
break;

```

## WiFi Channel Management

**Critical Requirement**: Master and all panels must be on the same WiFi channel for ESP-NOW to work.

**Configuration**: Set `ESPNOW_WIFI_CHANNEL` in `include/common.h` to match your router's channel.

**Validation**:

- Master checks on startup and warns if mismatch detected
- Panels set their channel explicitly using `esp_wifi_set_channel()`

**If Channel Changes:**

1. Check master serial output for actual WiFi channel
2. Update `ESPNOW_WIFI_CHANNEL` in `include/common.h`
3. Rebuild and re-upload to all panels

## Debugging

### Master Debugging

**Expected Serial Output:**

```

=== MASTER ESP32 ===
Connecting to YOUR_WIFI_SSID
WiFi connected.
IP address: 192.168.X.X
MAC address: XX:XX:XX:XX:XX:XX
Master WiFi Channel: 6
✓ WiFi channel matches ESPNOW_WIFI_CHANNEL
Panel 1 added
Panel 2 added
Panel 3 added
Panel 4 added
ESP-NOW initialized
Attempting MQTT connection...connected

```

**Common Issues:**

- "WiFi channel mismatch" → Update `ESPNOW_WIFI_CHANNEL` in `common.h`
- "Failed to add Panel X" → Check MAC address definitions in `common.h`
- "MQTT connection failed" → Check broker address and network connectivity

### Panel Debugging

**Expected Serial Output:**

```

=== PANEL 1 ===
Factory MAC: XX:XX:XX:XX:XX:XX
Custom MAC address set successfully
Custom MAC: AA:AA:AA:AA:AA:01
WiFi Channel set to: 6
✓ WiFi channel configured correctly
Ready to receive commands

```

**When Command Received:**

```

Received from: XX:XX:XX:XX:XX:XX
Command accepted - Pattern: 2 Brightness: 200

```

**Common Issues:**

- "Failed to set WiFi channel" → Check ESP-NOW channel configuration
- No "Received from" messages → WiFi channel mismatch with master
- "Command ignored" → PanelId in command doesn't match this panel

### ESP-NOW Status Codes

**Master Send Callback:**

- `Success` - Panel received the packet
- `Fail` - Panel did not receive (channel mismatch, out of range, or panel offline)

## Performance Characteristics

- **MQTT Latency**: 50-200ms (depends on network)
- **ESP-NOW Latency**: <10ms (master to panel)
- **Pattern Update Rate**: 10-100 Hz (depends on pattern)
- **PWM Frequency**: 5kHz (flicker-free for human vision)
- **WiFi Range**: ~50m indoor (typical for 2.4GHz)
- **ESP-NOW Range**: ~200m line-of-sight (same as WiFi)

## Power Requirements

**Per Panel (worst case):**

- ESP32: ~250mA @ 5V
- 7 Regions at full brightness: depends on LED strip current
- Total: Calculate based on actual LED strip specifications

**Example Calculation:**

- If each region has 50 warm white LEDs @ 20mA each = 1A per region
- 7 regions × 1A = 7A per panel
- ESP32 = 0.25A
- **Total per panel: ~7.5A @ 5V or 12V** (depending on LED voltage)

**Safety Margins:**

- Use power supplies rated 20% higher than calculated
- Ensure common ground between all power supplies and ESP32s

## Security Considerations

**Current Setup (Development):**

- Using public MQTT broker (test.mosquitto.org)
- No encryption on ESP-NOW
- No authentication

**Production Recommendations:**

1. Deploy local Mosquitto broker with authentication
2. Use MQTT over TLS (port 8883)
3. Enable ESP-NOW encryption with shared key
4. Implement access control for MQTT topics
5. Consider VPN for remote access

## Troubleshooting Checklist

**No communication between master and panels:**

- [ ] Check WiFi channel matches (`ESPNOW_WIFI_CHANNEL`)
- [ ] Verify custom MAC addresses set correctly
- [ ] Confirm panels are powered on
- [ ] Check ESP-NOW initialization succeeded on both sides

**LEDs not responding:**

- [ ] Verify MOSFET wiring (gate, drain, source)
- [ ] Check common ground between ESP32 and LED power supply
- [ ] Test PWM output with multimeter or LED
- [ ] Verify region enabled in command (`regions` array)

**MQTT connection issues:**

- [ ] Check WiFi credentials
- [ ] Verify MQTT broker address reachable
- [ ] Confirm MQTT port (1883 for plain, 8883 for TLS)
- [ ] Check firewall settings

## Future Enhancements

- [ ] Audio input via I2S microphone for real-time audio reactivity
- [ ] Web-based pattern designer
- [ ] Save/recall pattern presets
- [ ] Synchronized patterns across multiple masters
- [ ] OTA (Over-The-Air) firmware updates
- [ ] Local MQTT broker setup automation
- [ ] Mobile app with native UI
- [ ] DMX512 protocol support for professional lighting integration
