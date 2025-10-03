# Troubleshooting Guide

Comprehensive debugging guide for the LED Stage Control System.

## Quick Diagnostic Checklist

Run through this list before diving into detailed troubleshooting:

- [ ] All ESP32s powered on
- [ ] Master shows "✓ WiFi connected"
- [ ] Master shows "✓ connected" (MQTT)
- [ ] Master shows all panels added
- [ ] Master publishes heartbeat to `ta25stage/status` every 30s
- [ ] Panels show "✓ WiFi channel configured correctly"
- [ ] Panels show "Ready to receive commands"
- [ ] Panels print heartbeat every 60s (uptime, pattern, loops)
- [ ] WiFi channel numbers match across all devices
- [ ] Common ground connected (ESP32 GND to PSU GND)
- [ ] LED strips have power supply voltage

## Master Issues

### WiFi Connection Failed

**Symptoms**:
```
Connecting to SSID...
...
...
WiFi connection failed
```

**Solutions**:

1. **Check Credentials**:
   - Verify SSID and password in `src/master/main.cpp`
   - Check for typos (case-sensitive)
   - Ensure no hidden characters

2. **Signal Strength**:
   - Move master closer to router
   - Check router is powered on and broadcasting
   - Use 2.4 GHz network (ESP32 doesn't support 5 GHz)

3. **Router Settings**:
   - Ensure DHCP enabled
   - Check MAC filtering (allow master MAC)
   - Verify router not in "AP isolation" mode

4. **ESP32 Hardware**:
   - Try different ESP32 board
   - Check antenna connection (if external)
   - Re-seat USB cable

### MQTT Connection Failed

**Symptoms**:
```
✓ WiFi connected
IP address: 192.168.1.100
Attempting MQTT connection...✗ failed, rc=-2
Retrying in 5 seconds...
```

**New Features**:
- Progressive retry timeout: 5s → 15s → 25s → ... → 120s max
- After 5 failures: WiFi reconnection check
- After 10 failures: Deep sleep for 5 minutes with troubleshooting tips

**Error Codes**:

| Code | Meaning              | Solution                          |
|------|----------------------|-----------------------------------|
| -4   | Connection timeout   | Check broker address and port     |
| -3   | Connection lost      | Broker went offline               |
| -2   | Connect failed       | Network issue, check firewall     |
| -1   | Disconnected         | Broker refused connection         |
| 0    | Connected            | Success                           |
| 1    | Bad protocol version | Update PubSubClient library       |
| 2    | Client ID rejected   | Change client ID in code          |
| 5    | Not authorized       | Check username/password           |

**Solutions**:

1. **Test Broker Reachability**:
   ```bash
   ping test.mosquitto.org
   ```

2. **Verify Broker Port**:
   - Default: 1883 (unencrypted)
   - TLS: 8883
   - WebSocket: 9001

3. **Check Firewall**:
   - Allow outbound TCP port 1883
   - Check router/network firewall rules

4. **Test with MQTT Client**:
   ```bash
   mosquitto_sub -h test.mosquitto.org -t "test/topic"
   ```

5. **Try Alternative Broker**:
   - `broker.hivemq.com`
   - `mqtt.eclipseprojects.io`

### WiFi Channel Mismatch Warning

**Symptoms**:
```
Master WiFi Channel: 11
⚠ WARNING: WiFi CHANNEL MISMATCH!
Expected channel: 6
Actual channel: 11
```

**Solution**:

1. Note the actual channel number (11 in this example)
2. Edit `include/common.h`:
   ```cpp
   #define ESPNOW_WIFI_CHANNEL 11  // Change from 6 to 11
   ```
3. Re-upload to all 4 panels:
   ```bash
   just up s1 && just up s2 && just up s3 && just up s4
   ```

**Why This Happens**:
- Router determines WiFi channel
- Can change if router reboots or switches channels automatically
- ESP-NOW requires all devices on same channel

### ESP-NOW Send Failures

**Symptoms**:
```
Sending to Panel 1... Fail
Sending to Panel 2... Fail
```

**Solutions**:

1. **Channel Mismatch** (most common):
   - See "WiFi Channel Mismatch" above

2. **Panel Not Powered**:
   - Check panel power supply
   - Verify panel ESP32 has power LED on

3. **MAC Address Error**:
   - Verify MAC addresses in `include/common.h`
   - Check panel serial: "Custom MAC: AA:AA:AA:AA:AA:0X"
   - Ensure no duplicate MACs

4. **Range Issue**:
   - Move panels closer to master (<50m)
   - Remove obstacles (metal, concrete)

5. **ESP-NOW Not Initialized**:
   - Check master serial for "ESP-NOW initialized"
   - Verify `esp_now_init()` returns ESP_OK

## Panel Issues

### Custom MAC Address Not Set

**Symptoms**:
```
=== PANEL 1 ===
Factory MAC: XX:XX:XX:XX:XX:XX
Failed to set custom MAC address
```

**Solutions**:

1. **Call Before WiFi Init**:
   - Ensure `esp_wifi_set_mac()` called **before** `WiFi.mode()`
   - Check firmware order in `setup()`

2. **Use Correct Interface**:
   ```cpp
   esp_wifi_set_mac(WIFI_IF_STA, panel1_mac);  // STA, not AP
   ```

3. **ESP32 Variant**:
   - Some ESP32 clones don't support custom MAC
   - Try different board or use factory MAC

### WiFi Channel Not Set

**Symptoms**:
```
WiFi Channel set to: 6
✗ Failed to set WiFi channel
```

**Solutions**:

1. **Initialize WiFi First**:
   ```cpp
   WiFi.mode(WIFI_STA);  // Must be called before esp_wifi_set_channel()
   esp_wifi_set_channel(ESPNOW_WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
   ```

2. **Valid Channel Number**:
   - Channels 1-13 supported (1-11 in USA)
   - Verify `ESPNOW_WIFI_CHANNEL` is valid

3. **WiFi Not Disconnected**:
   - Ensure `WiFi.disconnect()` called
   - Panel should NOT be connected to AP

### Not Receiving Commands

**Symptoms**:
```
=== PANEL 2 ===
✓ WiFi channel configured correctly
Ready to receive commands
✓ Heartbeat | Uptime: 60s | Pattern: 0 | Loops: 6000 | Free heap: 240000
⚠ No commands received for 5 minutes
Master may be offline or communication lost
```

**New Features**:
- Heartbeat logged every 60 seconds with diagnostics
- Warning after 5 minutes without commands
- Loop counter to detect frozen states

**Solutions**:

1. **WiFi Channel Mismatch**:
   - Check master channel: "Master WiFi Channel: X"
   - Update `ESPNOW_WIFI_CHANNEL` in `common.h` to match
   - Re-upload panel firmware

2. **Panel ID Mismatch**:
   - Send command with `"panelId": 0` (broadcast)
   - Verify panel firmware compiled with correct `PANEL_ID`
   - Check serial: "=== PANEL X ===" matches expected

3. **ESP-NOW Not Initialized**:
   - Check serial for "Ready to receive commands"
   - Verify `esp_now_init()` returns ESP_OK
   - Try re-uploading firmware

4. **Master Not Sending**:
   - Check master serial for send attempts
   - Verify MQTT command received by master
   - Test with MQTT client

### Receiving But Ignoring Commands

**Symptoms**:
```
Received from: XX:XX:XX:XX:XX:XX
Command ignored (wrong panel ID)
```

**Solution**:

Command's `panelId` field doesn't match this panel.

**Fix Options**:

1. Send broadcast: `"panelId": 0`
2. Send to specific panel: `"panelId": 2` (for panel 2)
3. Verify panel ID in firmware: `#define PANEL_ID 2`

## LED Issues

### LEDs Not Lighting At All

**Symptoms**:
- Panel receives commands correctly
- No LED output on any region

**Solutions**:

1. **Check Common Ground** (most common):
   ```
   ESP32 GND ──┬─── MOSFET Source
               │
               └─── PSU GND
   ```
   - Measure resistance: should be <1Ω
   - Use thick ground wire (18 AWG minimum)

2. **Verify LED Power Supply**:
   - Measure PSU output: 12V ±1V (or 5V ±0.25V)
   - Check PSU has load capacity for LEDs
   - Ensure PSU is turned on

3. **Test GPIO Output**:
   - Measure GPIO voltage with multimeter
   - At full brightness: 3.3V DC
   - At 50% brightness: ~1.65V average (5 kHz PWM)

4. **Check MOSFET Wiring**:
   - Gate: Connected to ESP32 GPIO
   - Drain: Connected to LED strip (-)
   - Source: Connected to PSU GND
   - Pull-down: 10kΩ from gate to source

5. **Verify Region Enabled**:
   - Check command: `"regions": [1,2,3,4,5,6,7]`
   - Not: `"regions": []` (empty)

### Some Regions Not Working

**Symptoms**:
- Regions 1, 2, 3 work
- Regions 4, 5, 6, 7 don't work

**Solutions**:

1. **Check Pin Mapping**:
   - Verify `regionPins[]` in `src/panel/main.cpp`
   - Ensure GPIO numbers match your wiring

2. **Test Individual GPIO**:
   - Upload test sketch with single GPIO high
   - Measure voltage on each GPIO pin

3. **MOSFET Failure**:
   - Swap working MOSFET to non-working region
   - If problem moves, MOSFET is faulty

4. **LED Strip Issue**:
   - Test LED strip with direct power (bypass MOSFET)
   - Check for cut traces or damaged LEDs

### LEDs Flickering

**Symptoms**:
- LEDs flicker at low brightness levels
- Visible strobing effect

**Solutions**:

1. **Increase PWM Frequency**:
   ```cpp
   ledcSetup(channel, 10000, 8);  // 10 kHz instead of 5 kHz
   ```

2. **Check Power Supply**:
   - Measure ripple voltage with oscilloscope
   - Use larger filter capacitors (1000µF near PSU)

3. **Ground Loop**:
   - Ensure single-point ground connection
   - Avoid ground loops through multiple paths

4. **Insufficient Current**:
   - PSU too small for total LED load
   - Upgrade to higher-current PSU

### LEDs Dim or Uneven Brightness

**Symptoms**:
- LEDs dimmer than expected
- Some regions brighter than others

**Solutions**:

1. **Voltage Drop on Long Cables**:
   - Measure voltage at LED strip: should be 12V ±0.5V
   - Use thicker wire gauge (16 AWG or 14 AWG)
   - Add power injection at far end of strip

2. **MOSFET Not Fully On**:
   - Gate voltage should be 3.3V when on
   - If lower, check pull-down resistor (should be 10kΩ, not 1kΩ)

3. **LED Strip Quality**:
   - Cheap strips have inconsistent brightness
   - Calibrate brightness per region in firmware

4. **PWM Resolution**:
   - Current: 8-bit (0-255)
   - Increase to 10-bit for finer control:
     ```cpp
     ledcSetup(channel, 5000, 10);  // 0-1023
     ```

### MOSFETs Overheating

**Symptoms**:
- MOSFET too hot to touch (>80°C)
- LED brightness drops over time

**Solutions**:

1. **Add Heat Sink**:
   - Use TO-220 heat sinks
   - Apply thermal paste

2. **Reduce LED Current**:
   - Limit maximum brightness in firmware
   - Use shorter LED strips per region

3. **Use Lower R_ds(on) MOSFET**:
   - IRLZ44N: 0.022Ω
   - IRLB8721: 0.0065Ω (3× better)

4. **Parallel MOSFETs**:
   - Use two MOSFETs per region
   - Connect gates together
   - Halves current per MOSFET

## Communication Issues

### Master and Panels on Different Channels

**Symptoms**:
```
Master WiFi Channel: 11
Panel WiFi Channel: 6
No communication
```

**Solution**:

1. Always upload master **first**
2. Note master's WiFi channel from serial output
3. Update `ESPNOW_WIFI_CHANNEL` in `include/common.h`
4. Upload all panels with corrected channel

**Prevention**:
- Document router's WiFi channel
- Set router to fixed channel (disable auto-select)

### Intermittent Communication

**Symptoms**:
- Commands work sometimes
- Random delivery failures

**Solutions**:

1. **RF Interference**:
   - Move away from microwave ovens, Bluetooth devices
   - Change WiFi channel to less congested one
   - Use WiFi analyzer app to find clear channel

2. **Power Supply Noise**:
   - Add 100µF capacitor near ESP32 VIN
   - Use separate PSU for ESP32 vs LEDs

3. **Distance Too Far**:
   - Keep panels within 30m of master (indoor)
   - Add WiFi repeater to extend range

4. **Packet Collisions**:
   - Don't send commands faster than 100 Hz
   - Add small delay between commands

### High Latency

**Symptoms**:
- Commands take several seconds to execute
- Pattern changes lag

**Solutions**:

1. **MQTT Broker Overloaded**:
   - Use local broker instead of public
   - Check broker CPU usage

2. **Network Congestion**:
   - Use QoS 0 (fire-and-forget)
   - Avoid large JSON payloads

3. **Master Processing Slow**:
   - Check for `delay()` in loop
   - Optimize JSON parsing

4. **ESP-NOW Queue Full**:
   - Don't send faster than 100 Hz
   - Current implementation should be fine

## Build and Upload Issues

### PlatformIO Environment Not Found

**Symptoms**:
```
Error: Unknown environment names 'master'
```

**Solution**:

Verify `platformio.ini` has:
```ini
[env:master]
platform = espressif32
board = esp32dev
framework = arduino
```

### Upload Failed / Timeout

**Symptoms**:
```
A fatal error occurred: Failed to connect to ESP32
```

**Solutions**:

1. **Hold BOOT Button**:
   - Press and hold BOOT during "Connecting..."
   - Release after upload starts

2. **Check USB Cable**:
   - Must support data (not just power)
   - Try different cable

3. **USB Port**:
   - Try different USB port
   - Avoid USB hubs

4. **Drivers**:
   - Install CH340 drivers (cheap ESP32 clones)
   - Or CP210x drivers (official boards)

5. **Serial Port**:
   - Check correct port selected in PlatformIO
   - Linux: `/dev/ttyUSB0` or `/dev/ttyACM0`
   - macOS: `/dev/cu.usbserial-*`
   - Windows: `COM3` or similar

### Compilation Errors

**Missing Library**:
```
fatal error: PubSubClient.h: No such file or directory
```

**Solution**:
```bash
pio lib install "knolleary/PubSubClient"
```

**PANEL_ID Not Defined**:
```
error: 'PANEL_ID' was not declared
```

**Solution**:
- Only happens if using `panel` environment without ID
- Use `panel1`, `panel2`, etc. (not `panel`)

## Testing and Validation

### Test Master Without Panels

1. Upload master firmware
2. Open serial monitor
3. Verify:
   - WiFi connected
   - MQTT connected
   - Panels added (even if offline)
4. Send MQTT command via MQTT client
5. Check master serial: "Parsed command: ..."

### Test Panel Without Master

1. Upload panel firmware
2. Open serial monitor
3. Verify:
   - Custom MAC set
   - WiFi channel configured
   - Ready to receive
4. Manually send test command:
   ```cpp
   void loop() {
     // Add once for testing
     LEDCommand testCmd = {0, 1, 255, {1,1,1,1,1,1,1}, 50, 0, 0};
     memcpy(&currentState, &testCmd, sizeof(LEDCommand));
   }
   ```
5. LEDs should show pattern 1 (breathing)

### Test LEDs Without ESP32

1. Disconnect MOSFET gate from ESP32
2. Connect gate to 3.3V directly (through 10kΩ resistor)
3. LEDs should turn on at full brightness
4. If not, check LED power supply and wiring

### Test MOSFET Without LEDs

1. Disconnect LED strip
2. Set pattern 0, full brightness
3. Measure MOSFET gate voltage: should be 3.3V
4. Measure drain-to-source voltage: should be ~0V (MOSFET fully on)

## Serial Monitor Tips

### Enable Verbose Output

Add to firmware:
```cpp
Serial.setDebugOutput(true);  // ESP32 WiFi/ESP-NOW debug
```

### Filter Serial Output

Using `grep`:
```bash
pio device monitor | grep "Error"
```

### Monitor Multiple Panels

Using `tmux`:
```bash
tmux new-session -s panels \; \
  split-window -h \; \
  split-window -v \; \
  select-pane -t 0 \; \
  split-window -v \; \
  select-pane -t 0 \; send-keys "just mon m" C-m \; \
  select-pane -t 1 \; send-keys "just mon s1" C-m \; \
  select-pane -t 2 \; send-keys "just mon s2" C-m \; \
  select-pane -t 3 \; send-keys "just mon s3" C-m
```

## Advanced Debugging

### ESP-NOW Packet Sniffer

Use Wireshark to capture ESP-NOW packets:

1. Set WiFi adapter to monitor mode
2. Filter: `wlan.fc.type == 2 && wlan.fc.subtype == 13`
3. Look for source MAC = master, dest MAC = panel MACs

### Oscilloscope Analysis

Measure PWM signal:
- **GPIO Pin**: Clean 5 kHz square wave
- **MOSFET Gate**: Same as GPIO (3.3V high, 0V low)
- **LED Strip (-)**: Choppy waveform (due to LED load)

### Logic Analyzer

Capture ESP-NOW timing:
- Probe: ESP32 TX/RX pins
- Trigger: Start of packet
- Measure: Time from master send to panel receive

## Getting Help

### Information to Provide

When asking for help, include:

1. **Hardware**:
   - ESP32 board model
   - LED strip voltage and current
   - Power supply specifications

2. **Firmware**:
   - Which environment (master, panel1, etc.)
   - Any code modifications
   - Library versions (from `platformio.ini`)

3. **Serial Output**:
   - Master serial output (full)
   - Panel serial output (full)
   - Copy-paste, don't screenshot

4. **Configuration**:
   - `ESPNOW_WIFI_CHANNEL` value
   - WiFi SSID (router model)
   - MQTT broker address

5. **Symptoms**:
   - Exact error messages
   - When problem occurs (always, sometimes, after X minutes)
   - What troubleshooting steps already tried

### Common Mistakes (Learn From Others)

1. **Forgetting common ground** (ESP32 GND to PSU GND)
2. **Mismatched WiFi channels** (master vs panels)
3. **Using wrong MOSFET** (standard instead of logic-level)
4. **Insufficient power supply** (LEDs dim, ESP32 resets)
5. **Wrong GPIO pins** (code doesn't match wiring)
6. **Empty regions array** (`"regions": []` → no LEDs)
7. **Upload to wrong environment** (panel1 code to panel2 board)

## Related Documentation

- **[AGENTS.md](../AGENTS.md)** - Firmware architecture
- **[hardware.md](hardware.md)** - Wiring diagrams
- **[protocols.md](protocols.md)** - Communication specs
