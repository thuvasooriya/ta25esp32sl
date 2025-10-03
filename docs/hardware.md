# Hardware Specifications

Complete wiring diagrams and component specifications for the LED Stage Control System.

## Bill of Materials

### Per Panel (×4)

| Component               | Quantity | Specifications                       | Example Part      |
| ----------------------- | -------- | ------------------------------------ | ----------------- |
| ESP32 DevKit            | 1        | 30-pin, dual-core, WiFi+BT           | ESP32-WROOM-32    |
| N-Channel MOSFET        | 7        | Logic-level, ≥30V, ≥10A              | IRLZ44N           |
| Resistor (pull-down)    | 7        | 10kΩ, 1/4W                           | Generic           |
| LED Strip               | 7        | Warm white, 12V or 5V, analog        | Generic           |
| Power Supply            | 1        | 5V/12V, ≥8A (calculate for LED load) | Mean Well LRS-100 |
| Breadboard/PCB          | 1        | Prototyping or permanent mount       | Generic           |
| USB Cable (programming) | 1        | Micro-USB, data+power                | Generic           |
| Jumper Wires            | ~20      | Male-to-male, various lengths        | Generic           |

### Master (×1)

| Component        | Quantity | Specifications             | Example Part        |
| ---------------- | -------- | -------------------------- | ------------------- |
| ESP32 DevKit     | 1        | 30-pin, dual-core, WiFi+BT | ESP32-WROOM-32      |
| USB Power Supply | 1        | 5V, 1A (no LEDs connected) | Generic USB charger |

### Optional

- Multimeter (debugging)
- Heat sinks for MOSFETs (if driving >5A per region)
- Fuses (one per power supply output)
- Emergency stop switch (for all LED power)

## ESP32 Pin Configuration

### Panel GPIO Mapping

| Region | GPIO Pin | PWM Channel | Function             |
| ------ | -------- | ----------- | -------------------- |
| 1      | GPIO 25  | Channel 0   | Region 1 MOSFET Gate |
| 2      | GPIO 26  | Channel 1   | Region 2 MOSFET Gate |
| 3      | GPIO 27  | Channel 2   | Region 3 MOSFET Gate |
| 4      | GPIO 14  | Channel 3   | Region 4 MOSFET Gate |
| 5      | GPIO 12  | Channel 4   | Region 5 MOSFET Gate |
| 6      | GPIO 13  | Channel 5   | Region 6 MOSFET Gate |
| 7      | GPIO 15  | Channel 6   | Region 7 MOSFET Gate |

**Notes:**

- All GPIOs are 3.3V logic
- PWM frequency: 5 kHz (flicker-free)
- PWM resolution: 8-bit (0-255)
- Do NOT use GPIO 0, 2, 5 (boot mode pins)

### Master GPIO Usage

- **No GPIO pins used** (only WiFi radio)
- Can be powered via USB or VIN pin

## MOSFET Wiring (Per Region)

### Circuit Diagram

```
ESP32 GPIO Pin ──────┬──── MOSFET Gate
                     │
                   [10kΩ]  (Pull-down resistor)
                     │
ESP32 GND ───────────┴──── MOSFET Source ──── PSU GND
                                │
                           MOSFET Drain ──── LED Strip (-)

LED Strip (+) ────────────────────────────── PSU (+12V or +5V)
```

### Detailed Connections

1. **Gate Connection**:
   - ESP32 GPIO → MOSFET Gate pin
   - 10kΩ resistor from Gate to Ground (prevents floating gate)

2. **Drain Connection**:
   - MOSFET Drain → LED Strip negative (-)

3. **Source Connection**:
   - MOSFET Source → Power Supply GND

4. **LED Strip Power**:
   - LED Strip positive (+) → Power Supply +12V or +5V

5. **Common Ground** (Critical):
   - ESP32 GND → Power Supply GND
   - **Without common ground, PWM control won't work**

### MOSFET Selection Guide

**Logic-Level MOSFETs** (gate fully on at 3.3V):

- IRLZ44N (55V, 47A, $0.50)
- IRL540N (100V, 33A, $0.60)
- IRLB8721 (30V, 62A, $1.00)

**Standard MOSFETs** (require gate driver, NOT recommended):

- IRF540 (requires 10V gate voltage, won't work with ESP32)

### Heat Management

**When to Use Heat Sinks**:

- LED current per region > 3A
- Continuous operation > 1 hour
- High ambient temperature (>30°C)

**Power Dissipation Calculation**:

- P = I² × R_ds(on)
- Example: 5A through IRLZ44N (0.022Ω) = 5² × 0.022 = 0.55W

## LED Strip Specifications

### Supported Types

✅ **Analog (Non-addressable)**:

- Warm white LED strips
- Single color channel
- Powered by constant voltage (5V or 12V)
- Common models: SMD3528, SMD5050

❌ **NOT Supported** (without firmware changes):

- WS2812B (addressable RGB)
- APA102 (addressable RGB)
- SK6812 (addressable RGBW)

### Current Calculation

**Example**: 12V warm white strip, 60 LEDs/meter, 4.8W/meter

- Power per meter: 4.8W
- Current per meter: 4.8W ÷ 12V = 0.4A
- 5 meters per region: 5 × 0.4A = 2A per region
- 7 regions per panel: 7 × 2A = 14A per panel
- **Power supply needed**: 14A + 20% margin = ~17A @ 12V

**Always measure actual current with multimeter before final deployment.**

## Power Supply Sizing

### Formula

```
Total Current = (LED_current_per_region × 7 regions) + ESP32_current
ESP32_current ≈ 0.25A @ 5V
```

### Safety Margins

- **20% overhead**: Account for startup surge and voltage drop
- **Fuse rating**: 1.5× normal operating current
- **Wire gauge**: Use 18 AWG for 5-10A, 16 AWG for 10-15A

### Example Calculations

**Low Power Setup** (5V LEDs, 1A per region):

```
(1A × 7) + 0.25A = 7.25A
With 20% margin: 7.25A × 1.2 = 8.7A
PSU Rating: 10A @ 5V (50W)
```

**High Power Setup** (12V LEDs, 3A per region):

```
(3A × 7) = 21A for LEDs
PSU Rating: 25A @ 12V (300W)
ESP32 powered separately via USB
```

### Recommended Power Supplies

- **Mean Well LRS-100-5** (5V, 20A, 100W)
- **Mean Well LRS-100-12** (12V, 8.5A, 102W)
- **Mean Well LRS-350-12** (12V, 29A, 348W)

## Complete Panel Wiring Diagram

```
         ESP32 DevKit (Panel)
        ┌─────────────────┐
        │                 │
        │ GPIO 25 ────────┼──→ MOSFET 1 ──→ LED Region 1 (-)
        │ GPIO 26 ────────┼──→ MOSFET 2 ──→ LED Region 2 (-)
        │ GPIO 27 ────────┼──→ MOSFET 3 ──→ LED Region 3 (-)
        │ GPIO 14 ────────┼──→ MOSFET 4 ──→ LED Region 4 (-)
        │ GPIO 12 ────────┼──→ MOSFET 5 ──→ LED Region 5 (-)
        │ GPIO 13 ────────┼──→ MOSFET 6 ──→ LED Region 6 (-)
        │ GPIO 15 ────────┼──→ MOSFET 7 ──→ LED Region 7 (-)
        │                 │
        │ GND ────────────┼──→ Common Ground Bus
        │ USB/VIN ────────┼──→ 5V Power (programming/operation)
        │                 │
        └─────────────────┘

                          Common Ground Bus
                          ─────────────────
                                 │
                   ┌─────────────┼────────────────┐
                   │             │                │
            ESP32 GND    MOSFET Sources    PSU GND (12V/5V)
                                                   │
                                             ┌─────┴─────┐
                                             │   PSU     │
                                             │  (12V)    │
                                             └─────┬─────┘
                                                   │
                                              +12V Output
                                                   │
                        ┌──────────────────────────┼────────────┐
                        │                          │            │
               LED Region 1 (+)          LED Region 2 (+)   ... (all regions)
               LED Region 1 (-)          LED Region 2 (-)   ... (to MOSFETs)
```

## Physical Installation

### Panel Mounting

1. **Enclosure**: IP65 rated for outdoor use (optional)
2. **Ventilation**: Ensure airflow for MOSFET cooling
3. **Cable Management**: Strain relief for all connections
4. **Vibration Dampening**: Rubber standoffs for ESP32 mount

### Cable Lengths

- **ESP32 to MOSFET**: <20cm (keep PWM lines short)
- **MOSFET to LED Strip**: <5m (voltage drop consideration)
- **Power Supply to LEDs**: Use thicker gauge for longer runs

### Grounding

- **Safety Ground**: Connect PSU ground to earth ground
- **Common Ground**: All ESP32s and PSUs to same ground reference
- **Ground Loops**: Avoid if possible (use star grounding topology)

## Testing Procedure

### 1. Continuity Check (Power Off)

- [ ] ESP32 GND to PSU GND (should be <1Ω)
- [ ] MOSFET Gate to ESP32 GPIO (should be <1Ω)
- [ ] LED Strip (+) to PSU (+) (should be <1Ω)
- [ ] LED Strip (-) to MOSFET Drain (should be <1Ω)

### 2. Voltage Check (Power On, No LEDs)

- [ ] PSU Output: 12V ±5% (or 5V ±5%)
- [ ] ESP32 3.3V Rail: 3.3V ±0.1V
- [ ] MOSFET Gate (off state): <0.5V
- [ ] MOSFET Gate (on state): 3.3V ±0.2V

### 3. Current Check (LEDs Connected)

- [ ] Measure current per region at full brightness
- [ ] Verify total current < PSU rating
- [ ] Check for excessive heat on MOSFETs

### 4. PWM Verification

- [ ] Send pattern 0, brightness 128
- [ ] Measure gate voltage with oscilloscope: 5 kHz square wave
- [ ] Duty cycle should be ~50%

## Safety Considerations

### Electrical Safety

- **Fuse All Power Lines**: 1.5× rated current
- **Polarity Protection**: Use diodes on PSU outputs
- **Emergency Stop**: Physical switch to cut LED power
- **Double Insulation**: Ensure no exposed live conductors

### Fire Safety

- **Wire Ratings**: Use wires rated for current + 20%
- **Heat Dissipation**: MOSFETs should not exceed 80°C
- **Ventilation**: Prevent heat buildup in enclosures
- **Fire Retardant**: Use flame-resistant enclosures

### ESD Protection

- **Static Wristband**: When handling ESP32s
- **Anti-Static Mat**: For assembly workspace
- **Avoid Carpets**: Work on non-static surfaces

## Maintenance

### Regular Checks (Before Each Event)

- [ ] Visual inspection of all connections
- [ ] Check for loose screws/terminals
- [ ] Verify no burnt components
- [ ] Test emergency stop switch
- [ ] Measure PSU output voltage

### Troubleshooting Hardware

| Symptom                 | Possible Cause              | Solution                         |
| ----------------------- | --------------------------- | -------------------------------- |
| LEDs flicker at low PWM | PWM frequency too low       | Increase to 5 kHz in firmware    |
| MOSFET overheating      | Insufficient heat sinking   | Add heat sink or reduce current  |
| LEDs dim over time      | Voltage drop on long cables | Use thicker wire or boost PSU    |
| ESP32 resets randomly   | Power supply noise          | Add capacitor (100µF) near ESP32 |
| No PWM output           | Missing common ground       | Connect ESP32 GND to PSU GND     |

## Upgrading Hardware

### Adding More Regions

1. Use remaining ESP32 GPIOs: 32, 33, 34 (input-only, use with external driver)
2. Add MOSFETs and follow same wiring pattern
3. Update firmware: `#define NUM_REGIONS 10`

### Increasing LED Current

1. Parallel multiple MOSFETs (same gate signal)
2. Use MOSFET modules (e.g., 30A rated boards)
3. Upgrade PSU to higher current rating

### Adding Panels

1. Program new ESP32 as panel 5 (PANEL_ID=5)
2. Register in master firmware
3. Use custom MAC: `0xAA:AA:AA:AA:AA:05`

## Component Sources

### Online Retailers

- **Digi-Key**: MOSFETs, resistors, ESP32s
- **Mouser**: Power supplies, connectors
- **AliExpress**: LED strips, ESP32 clones (longer shipping)
- **Amazon**: Quick prototyping components

### Local Electronics Stores

- Check for MOSFETs, resistors, breadboards
- Usually more expensive but immediate availability
- Good for last-minute replacements

## Schematic Files

(For future addition: KiCad schematic files, Gerber files for PCB manufacturing)
