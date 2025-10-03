# LED Stage Control System

A wireless LED control system for stage decoration, featuring centralized control via Android app and web interface, with synchronized LED patterns across multiple panels.

## Features

- 🎨 **Multiple Pattern Support**: Static, breathing, wave, pulse, and flicker effects
- 📱 **Dual Control Interface**: Android app and web browser control
- 🔄 **Synchronized Operation**: All panels update simultaneously via ESP-NOW
- 🎛️ **Independent Region Control**: 7 LED regions per panel, individually controllable
- 📶 **Remote Access**: Control from anywhere via MQTT
- ⚡ **Low Latency**: <10ms command propagation to panels

## System Components

- **1 Master ESP32**: Connects to WiFi and MQTT, broadcasts commands via ESP-NOW
- **4 Panel ESP32s**: Receive commands and control LED strips via PWM
- **~28 LED Regions Total**: 7 regions × 4 panels
- **MQTT Broker**: Message routing (currently using test.mosquitto.org)

## Hardware Requirements

### Per Panel

- 1× ESP32 DevKit board
- 7× N-channel MOSFETs (IRLZ44N or similar)
- 7× 10kΩ resistors (MOSFET pull-down)
- Warm white LED strips (analog, non-addressable)
- 5V or 12V power supply (rated for total LED current)
- Jumper wires and breadboard/PCB

### Master

- 1× ESP32 DevKit board
- USB power supply or 5V adapter

### Tools

- Computer with VSCode and PlatformIO
- USB cables for programming
- Multimeter (for debugging)

## Requirements

- A Good Editor
- [PlatformIO IDE extension](https://platformio.org/)
- [Just command runner](https://github.com/casey/just) (optional, for build automation)

## Installation

### 1. Clone Repository

```

git clone <repository-url>
cd led_stage_panel

```

### 2. Configure WiFi

Edit `src/master/main.cpp`:

```

const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

```

### 3. Check WiFi Channel

The system requires all ESP32s to be on the same WiFi channel. Your router determines this channel.

1. Upload master firmware first (step 4)
2. Check serial monitor for: `Master WiFi Channel: X`
3. Note the channel number

### 4. Configure ESP-NOW Channel

Edit `include/common.h`:

```

\#define ESPNOW_WIFI_CHANNEL 6  // Change to match your router's channel

```

### 5. Build and Upload

#### Using Just (Recommended)

```


# Upload master

just up m

# Upload panels

just up s1
just up s2
just up s3
just up s4

```

#### Using PlatformIO CLI

```


# Upload master

pio run -e master -t upload

# Upload panel 1

pio run -e panel1 -t upload

# Upload panel 2

pio run -e panel2 -t upload

# ... and so on

```

#### Using VSCode GUI

1. Open PlatformIO sidebar
2. Select environment (e.g., `env:master`)
3. Click "Upload" button
4. Wait for upload to complete
5. Repeat for each panel

### 6. Verify Operation

#### Master Serial Output

```

=== MASTER ESP32 ===
WiFi connected.
IP address: 192.168.X.X
Master WiFi Channel: 6
✓ WiFi channel matches ESPNOW_WIFI_CHANNEL
Panel 1 added
Panel 2 added
Panel 3 added
Panel 4 added
ESP-NOW initialized
Attempting MQTT connection...connected

```

#### Panel Serial Output

```

=== PANEL 1 ===
Custom MAC: AA:AA:AA:AA:AA:01
WiFi Channel set to: 6
✓ WiFi channel configured correctly
Ready to receive commands

```

## Usage

### Testing with MQTT Client

Install an MQTT client app (e.g., MQTT Explorer, MQTT Dash).

**Connect to Broker:**

- Host: `test.mosquitto.org`
- Port: `1883`
- No authentication required

**Send Test Command:**

Topic: `stage/command`

Payload:

```

{
"panelId": 0,
"patternId": 0,
"brightness": 200,
"regions":
}

```

**Expected Result:** All panels light up at ~80% brightness.

### Command Reference

#### Pattern 0: All On (with brightness control)

```

{
"panelId": 0,
"patternId": 0,
"brightness": 128,
"regions":
}

```

#### Pattern 1: Breathing Effect

```

{
"panelId": 1,
"patternId": 1,
"regions": ,
"speed": 50
}

```

#### Pattern 2: Wave Effect

```

{
"panelId": 0,
"patternId": 2,
"regions": ,
"speed": 70
}

```

#### Pattern 3: Pulse

```

{
"panelId": 2,
"patternId": 3,
"regions": ,
"speed": 60
}

```

#### Control Specific Panel

```

{
"panelId": 3,
"patternId": 1,
"brightness": 255,
"regions":
}

```

### Command Parameters

| Parameter       | Type  | Range      | Description                         |
| --------------- | ----- | ---------- | ----------------------------------- |
| `panelId`       | int   | 0-4        | 0=all panels, 1-4=specific panel    |
| `patternId`     | int   | 0-4        | Pattern number to display           |
| `brightness`    | int   | 0-255      | Brightness level (Pattern 0 only)   |
| `regions`       | array | [1-7]      | Which regions to activate           |
| `speed`         | int   | 0-100      | Pattern animation speed (0=slowest) |
| `audioReactive` | bool  | true/false | Enable audio modulation             |

## Build Commands (Just)

```


# Upload firmware

just up m         \# Master
just up s1        \# Panel 1
just up s2        \# Panel 2
just up s3        \# Panel 3
just up s4        \# Panel 4

# Monitor serial output

just mon m        \# Master
just mon s1       \# Panel 1

# Upload and monitor

just flash m      \# Upload master and open monitor
just flash s2     \# Upload panel 2 and open monitor

# Build without uploading

just build m      \# Master
just build s1     \# Panel 1
just build all    \# All environments

# Clean build files

just clean        \# Clean current environment
just clean-all    \# Clean all environments

# List available commands

just list

```

## Wiring Diagrams

### MOSFET Connection (Per Region)

```

ESP32                           LED Strip            Power Supply

GPIO Pin ──────┬─── Gate       ┌──────────┐
│                │  LEDs    │
[10kΩ]            (+)────────────────── +5V/+12V
│                │          │
GND               (-)────┐   │
│   │
Drain ─────────┘   │
│
MOSFET                  Source ────────────┴─── GND
(IRLZ44N)
│
ESP32 GND ─────────────────────────────────┘
(Common Ground)

```

### Panel Overview

````

     ESP32 DevKit
    ┌──────────┐
    │  GPIO 25 ├──→ MOSFET 1 → Region 1 LEDs
    │  GPIO 26 ├──→ MOSFET 2 → Region 2 LEDs
    │  GPIO 27 ├──→ MOSFET 3 → Region 3 LEDs
    │  GPIO 14 ├──→ MOSFET 4 → Region 4 LEDs
    │  GPIO 12 ├──→ MOSFET 5 → Region 5 LEDs
    │  GPIO 13 ├──→ MOSFET 6 → Region 6 LEDs
    │  GPIO 15 ├──→ MOSFET 7 → Region 7 LEDs
    │          │
    │   GND    ├──→ Common Ground (to PSU GND)
    │   USB    ├──→ 5V Power (programming)
    └──────────┘
    ```

## Troubleshooting

### Master not connecting to MQTT
- Check WiFi credentials
- Verify internet connectivity
- Try pinging test.mosquitto.org
- Check serial monitor for error messages

### Panels not receiving commands
1. Check WiFi channel matches between master and panels
2. Verify custom MAC addresses set correctly
3. Ensure ESP-NOW initialized successfully
4. Check physical distance (<50m recommended)

### LEDs not lighting up
1. Verify common ground connection
2. Test MOSFET with multimeter (gate voltage should be 3.3V when on)
3. Check LED strip power supply voltage
4. Confirm regions are enabled in command (`"regions": [1,2,3...]`)
5. Try maximum brightness: `"brightness": 255`

### WiFi Channel Mismatch Error
````

WARNING: WiFi CHANNEL MISMATCH!
Expected channel: 6
Actual channel: 11

```

**Solution:**
1. Note the "Actual channel" number from master serial output
2. Edit `include/common.h`:
```

\#define ESPNOW_WIFI_CHANNEL 11 // Use actual channel

```
3. Rebuild and re-upload to all panels:
```

just up s1 \&\& just up s2 \&\& just up s3 \&\& just up s4

```

### Flashing/Uploading Issues
- Press and hold BOOT button on ESP32 while uploading
- Check USB cable (must support data, not just power)
- Try different USB port
- Install/update USB-to-UART drivers (CP210x or CH340)

## Development

### Adding New Patterns

1. Open `src/panel/main.cpp`
2. Add your pattern function:
```

void pattern_myEffect() {
static unsigned long lastUpdate = 0;

     if (millis() - lastUpdate >= 50) {
       lastUpdate = millis();

       for (int r = 0; r < NUM_REGIONS; r++) {
         if (currentState.regions[r]) {
           uint8_t brightness = /* your logic */;
           setRegionBrightness(r, brightness);
         }
       }
     }
    }

```

3. Add to pattern switch:
```

void executePattern() {
switch(currentState.patternId) {
// ... existing patterns
case 5:
pattern_myEffect();
break;
}
}

```

4. Rebuild and upload to all panels

### Monitoring Multiple Panels

Open multiple terminal windows:

```

# Terminal 1

just mon m

# Terminal 2

just mon s1

# Terminal 3

just mon s2

# ... and so on

```

Or use screen/tmux for split-pane monitoring.

## Production Deployment

For live events, consider these improvements:

1. **Local MQTT Broker**: Deploy Mosquitto on local server instead of test.mosquitto.org
2. **Static IPs**: Assign static IPs to master ESP32
3. **Backup Master**: Keep spare programmed ESP32 ready
4. **Power Management**: Use UPS for master and router
5. **Cable Management**: Secure all connections with strain relief
6. **Test Run**: Full system test at venue before event
7. **Emergency Stop**: Physical switch to cut all LED power

## Support

For technical issues or questions:
1. Check serial monitor output for error messages
2. Review AGENTS.md for detailed technical information
3. Verify hardware connections match wiring diagrams
4. Ensure all firmware versions are up-to-date

## License

[Your License Here]

## Credits

Built with:
- [ESP32 Arduino Core](https://github.com/espressif/arduino-esp32)
- [PlatformIO](https://platformio.org/)
- [PubSubClient](https://github.com/knolleary/pubsubclient)
- [ArduinoJson](https://arduinojson.org/)
```
