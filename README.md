# LED Stage Control System

Wireless LED control system for stage decoration with centralized control via MQTT and ESP-NOW communication.

## System Components

- **1 Master ESP32**: WiFi + MQTT → ESP-NOW broadcaster
- **4 Panel ESP32s**: ESP-NOW receivers → PWM LED controllers
- **28 LED Regions Total**: 7 regions per panel

## Quick Start

### Prerequisites

- VSCode with [PlatformIO](https://platformio.org/)
- [Just](https://github.com/casey/just) (optional)
- ESP32 DevKit boards
- USB cables for programming

### Configuration Required Before Upload

⚠️ **You must configure these before uploading firmware:**

#### 1. WiFi Credentials (Master Only)

Edit `src/master/main.cpp`:

```cpp
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";
```

#### 2. WiFi Channel (Critical for ESP-NOW)

**First**: Upload master firmware and check serial monitor for the WiFi channel:

```
Master WiFi Channel: 11
```

**Then**: Edit `include/common.h` to match that channel:

```cpp
#define ESPNOW_WIFI_CHANNEL 11  // Must match your router's channel
```

**Finally**: Upload to all 4 panels with the correct channel setting.

> **Why this matters**: ESP-NOW only works when all devices are on the same WiFi channel. If the master connects to WiFi on channel 11, all panels must also be on channel 11.

#### 3. Optional: GPIO Pin Mapping

Default pin mapping in `src/panel/main.cpp`:

```cpp
const uint8_t regionPins[NUM_REGIONS] = {25, 26, 27, 14, 12, 13, 15};
```

Change these if your wiring differs.

#### 4. Optional: MQTT Broker

Default uses `test.mosquitto.org`. To use your own broker, edit `src/master/main.cpp`:

```cpp
const char* mqtt_server = "your.mqtt.broker.com";
const int mqtt_port = 1883;
```

### Upload Firmware

#### Using Just (Recommended)

```bash
# Master
just up m

# Panels (after configuring channel!)
just up s1
just up s2
just up s3
just up s4
```

#### Using PlatformIO CLI

```bash
pio run -e master -t upload
pio run -e panel1 -t upload
pio run -e panel2 -t upload
pio run -e panel3 -t upload
pio run -e panel4 -t upload
```

#### Using PlatformIO GUI

1. Open PlatformIO sidebar
2. Select environment (e.g., `env:master`)
3. Click "Upload"
4. Repeat for each panel

### Verify Installation

#### Master Serial Output

```
=== MASTER ESP32 ===
WiFi connected. IP: 192.168.X.X
Master WiFi Channel: 6
✓ WiFi channel matches ESPNOW_WIFI_CHANNEL
Panel 1 added
Panel 2 added
Panel 3 added
Panel 4 added
ESP-NOW initialized
MQTT connected
```

✅ **Success**: All panels added, WiFi channel matches

❌ **Error**: "WiFi channel mismatch" → Update `ESPNOW_WIFI_CHANNEL` in `common.h`

#### Panel Serial Output

```
=== PANEL 1 ===
Custom MAC: AA:AA:AA:AA:AA:01
WiFi Channel set to: 6
✓ WiFi channel configured correctly
Ready to receive commands
```

### Test the System

Use an MQTT client (MQTT Explorer, MQTT Dash) to send a test command:

**Broker**: `test.mosquitto.org:1883`  
**Topic**: `stage/command`  
**Payload**:

```json
{
  "panelId": 0,
  "patternId": 0,
  "brightness": 200,
  "regions": [1, 2, 3, 4, 5, 6, 7]
}
```

**Expected**: All panels light up at ~80% brightness.

## Available Patterns

| Pattern ID | Name      | Description                  |
| ---------- | --------- | ---------------------------- |
| 0          | Static    | Solid brightness control     |
| 1          | Breathing | Smooth fade in/out           |
| 2          | Wave      | Sequential region activation |
| 3          | Pulse     | Quick on/off pulses          |
| 4          | Flicker   | Random brightness (candle)   |

## Command Parameters

| Parameter       | Type  | Range | Description                 |
| --------------- | ----- | ----- | --------------------------- |
| `panelId`       | int   | 0-4   | 0=all, 1-4=specific panel   |
| `patternId`     | int   | 0-4   | Pattern to display          |
| `brightness`    | int   | 0-255 | Brightness (Pattern 0 only) |
| `regions`       | array | [1-7] | Active regions              |
| `speed`         | int   | 0-100 | Animation speed             |
| `audioReactive` | bool  |       | Enable audio modulation     |

## Common Issues

### WiFi Channel Mismatch

**Symptom**: Panels don't receive commands  
**Solution**:

1. Check master serial: `Master WiFi Channel: X`
2. Update `ESPNOW_WIFI_CHANNEL` in `include/common.h`
3. Re-upload to all panels

### LEDs Not Lighting

- [ ] Check common ground (ESP32 GND ↔ LED PSU GND)
- [ ] Verify MOSFET wiring (gate, drain, source)
- [ ] Confirm regions enabled in command
- [ ] Test with max brightness: `"brightness": 255`

### MQTT Connection Failed

- [ ] Verify WiFi credentials
- [ ] Check internet connectivity
- [ ] Try pinging `test.mosquitto.org`

### Upload Failed

- Hold BOOT button during upload
- Check USB cable (must support data)
- Try different USB port
- Install CH340/CP210x drivers

## Build Commands (Just)

```bash
# Upload
just up m        # Master
just up s1       # Panel 1

# Monitor serial
just mon m       # Master
just mon s1      # Panel 1

# Upload + monitor
just flash m     # Master
just flash s2    # Panel 2

# Build all
just build all

# Clean
just clean-all
```

## Documentation

- **[AGENTS.md](AGENTS.md)** - Firmware architecture and development guide
- **[docs/hardware.md](docs/hardware.md)** - Wiring diagrams and specifications
- **[docs/protocols.md](docs/protocols.md)** - MQTT and ESP-NOW communication details
- **[docs/troubleshooting.md](docs/troubleshooting.md)** - Detailed debugging guide

## Production Checklist

For live events:

- [ ] Use local MQTT broker (not test.mosquitto.org)
- [ ] Assign static IP to master
- [ ] Test full system at venue before event
- [ ] Keep backup ESP32s programmed and ready
- [ ] Use UPS for master and router
- [ ] Implement emergency stop switch

## License

MIT
