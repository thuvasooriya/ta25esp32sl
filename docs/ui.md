# UI Design Specification - TA25 LED Stage Control

## Overview

Control interface for 4-panel LED stage system with 20 regions. The UI features a **two-mode design**:

- **Normal Mode** (default) - Sequence control with configurable effects for live performance
- **Debug Mode** (toggle) - Granular region/panel control for testing and troubleshooting

## System Architecture

### Data Flow

```
User Interface (Web/App)
    ↓ MQTT
Master Controller (ESP32)
    ↓ ESP-NOW
Panel Controllers (4x ESP32)
    ↓ PWM
LED Regions (20 total)
```

### Communication

- **Protocol**: MQTT over WiFi
- **Topic**: `ta25stage/command`
- **Status Topic**: `ta25stage/master/status` (heartbeat every 30s)
- **Format**: JSON
- **Latency**: 50-200ms (network dependent)

## Control Modes

The UI has **two modes** accessed via a single toggle switch:

### Normal Mode (Default)

**Purpose**: Live performance control with pre-choreographed sequences.

**Features**:

- One-tap sequence triggers (Sweep, Story, Symbol, Test)
- Configurable effect parameters (effect type, brightness, speed)
- Effect parameters apply to sequences when triggered
- Simple, performance-focused interface

### Debug Mode (Toggle Enabled)

**Purpose**: Hardware testing, troubleshooting, and granular control.

**Features**:

- Individual panel selection
- Cross-panel group selection
- Direct region control (all 20 regions individually addressable)
- Effect parameters apply to selected targets
- Technical controls for setup/rehearsal

## UI Layout

### Normal Mode (Default View)

```
┌─────────────────────────────────────────────────────┐
│  TA25 Stage Control    [Debug Mode: ○]  [Status: ●] │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ┌──────────────────────────────────────────────┐  │
│  │  SEQUENCES                                   │  │
│  │  ┌──────┐ ┌──────┐ ┌──────┐ ┌──────┐        │  │
│  │  │Sweep │ │Story │ │Symbol│ │ Test │        │  │
│  │  │Top ↓ │ │  🎭  │ │  ✦   │ │  ✓   │        │  │
│  │  │  1   │ │  2   │ │  3   │ │  0   │        │  │
│  │  └──────┘ └──────┘ └──────┘ └──────┘        │  │
│  └──────────────────────────────────────────────┘  │
│                                                     │
│  ┌──────────────────────────────────────────────┐  │
│  │  EFFECT CONTROL                              │  │
│  │                                               │  │
│  │  Effect:      [Breathing ▼]                  │  │
│  │  Brightness:  ●────────────80%               │  │
│  │  Speed:       ●────────────50%               │  │
│  └──────────────────────────────────────────────┘  │
│                                                     │
│  Tap a sequence to run with current effect settings │
│                                                     │
└─────────────────────────────────────────────────────┘
```

### Debug Mode (Toggle Enabled)

```
┌─────────────────────────────────────────────────────┐
│  TA25 Stage Control    [Debug Mode: ●]  [Status: ●] │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ┌──────────────────────────────────────────────┐  │
│  │  PANEL SELECTION                             │  │
│  │  ┌─────┐ ┌────┐ ┌────┐ ┌────┐ ┌────┐        │  │
│  │  │ All │ │ P1 │ │ P2 │ │ P3 │ │ P4 │        │  │
│  │  └─────┘ └────┘ └────┘ └────┘ └────┘        │  │
│  └──────────────────────────────────────────────┘  │
│                                                     │
│  ┌──────────────────────────────────────────────┐  │
│  │  GROUP SELECTION                             │  │
│  │  □ Symbols (5)      □ Raavana (5)            │  │
│  │  □ Raavana Head (3) □ Continent (3)          │  │
│  └──────────────────────────────────────────────┘  │
│                                                     │
│  ┌──────────────────────────────────────────────┐  │
│  │  DIRECT REGION CONTROL     [Expand/Collapse] │  │
│  │                                               │  │
│  │  Panel 1: □ BULL_SYMBOL  □ BULL_HEAD  ...   │  │
│  │  Panel 2: □ CONTINENT_F  □ VEENA_SYMBOL ... │  │
│  │  Panel 3: □ CONTINENT_U  □ RAAVANA_HEAD ... │  │
│  │  Panel 4: □ RAAVANA_HEAD_SYMBOL  ...        │  │
│  └──────────────────────────────────────────────┘  │
│                                                     │
│  ┌──────────────────────────────────────────────┐  │
│  │  EFFECT CONTROL                              │  │
│  │                                               │  │
│  │  Effect:      [Static ▼]                     │  │
│  │  Brightness:  ●────────────100%              │  │
│  │  Speed:       ●────────────50%               │  │
│  │                                               │  │
│  │  [Apply Effect to Selected]                  │  │
│  └──────────────────────────────────────────────┘  │
│                                                     │
└─────────────────────────────────────────────────────┘
```

## Detailed Component Specifications

### 1. Debug Mode Toggle

**Purpose**: Switch between normal performance mode and debug/testing mode.

**Elements**:

- Toggle switch in header (top-right)
- Visual states: ○ (off/normal) and ● (on/debug)
- Label: "Debug Mode"

**Behavior**:

- **Off (Normal Mode)**: Shows sequence buttons and effect controls
- **On (Debug Mode)**: Shows panel selection, group selection, region controls
- State persists during session (can use localStorage/sessionStorage)
- Clear visual indication of current mode

### 2. Sequences Section (Normal Mode Only)

**Purpose**: One-tap access to pre-programmed narratives for live performance.

**Elements**:

- 4 large touch-friendly buttons (min 80x80px)
- Visual icons/symbols for each sequence
- Sequence number prominently displayed
- Active sequence highlighted during execution

**Sequences**:

- **Sequence 0**: Test Sequence (hardware validation - runs on master boot)
- **Sequence 1**: Vertical Sweep (top-to-bottom cascade)
- **Sequence 2**: Group Narrative (symbolic storytelling)
- **Sequence 3**: Symbol Emergence (meaning from symbols)

**Behavior**:

- Single tap triggers sequence with current effect settings
- Sequences use effect/brightness/speed from Effect Control section
- Sequences run to completion (blocking - cannot be interrupted)
- Visual feedback when sequence is running (e.g., pulsing border)
- Helper text: "Tap a sequence to run with current effect settings"

**MQTT Payload**:

```json
{
  "sequence": 1,
  "effect": 1,
  "brightness": 204,
  "speed": 50
}
```

### 3. Effect Control Section (Both Modes)

**Purpose**: Configure effect parameters that apply to sequences (normal mode) or selected targets (debug mode).

**Visibility**: Always visible in both modes

**Elements**:

**Effect Dropdown/Picker**:

- Static (solid brightness)
- Breathing (sine wave)
- Wave/Chase (sequential)
- Pulse (rhythmic flash)
- Fade In (gradual appear)
- Fade Out (gradual disappear)

**Brightness Slider**:

- Range: 0-255 (display as 0-100%)
- Default: 128 (50%)
- Real-time preview on drag
- Snap to 25%, 50%, 75%, 100%

**Speed Slider**:

- Range: 0-100 (effect-dependent timing)
- Default: 50
- Low = slow, High = fast
- Label: "Speed" or "Rate"

**Behavior**:

- **Normal Mode**: Settings apply to sequences when triggered
- **Debug Mode**: Settings apply when "Apply Effect to Selected" is clicked
- Can be adjusted during effect execution (panels respond in real-time)
- Changes persist until user modifies them

### 4. Panel Selection Section (Debug Mode Only)

**Purpose**: Target specific panels or broadcast to all.

**Visibility**: Only visible in debug mode

**Elements**:

- "All Panels" button (default, broadcasts to panelId=0)
- Individual panel buttons (Panel 1-4)
- Multi-select support (for future group broadcast)

**Visual States**:

- **Selected**: Highlighted border/background
- **Unselected**: Neutral/grayed
- **Offline**: Red indicator (future: status monitoring)

**Behavior**:

- "All Panels" is mutually exclusive with individual panels
- Selecting individual panel sets `panelId` in command

**MQTT Payload**:

```json
{"panelId": 0}  // All panels
{"panelId": 2}  // Panel 2 only
```

### 5. Group Selection Section (Debug Mode Only)

**Purpose**: Apply effects to predefined cross-panel groups.

**Visibility**: Only visible in debug mode

**Elements**:

- Checkboxes for each group
- Region count displayed (e.g., "Symbols (5)")
- Visual grouping with icons/colors

**Groups**:

- **Symbols** (5 regions): BULL_SYMBOL, BHARATHI_SYMBOL, VEENA_SYMBOL, DANCER_SYMBOL, VALLUVAR_SYMBOL
- **Raavana** (5 regions): All Raavana body parts
- **Raavana Head** (3 regions): Head-specific regions
- **Continent** (3 regions): Continent regions

**Behavior**:

- Multi-select allowed
- Regions array calculated from selected groups
- "Apply Effect to Selected" button sends command

**MQTT Payload**:

```json
{
  "effect": 1,
  "brightness": 200,
  "speed": 50,
  "regions": [0, 3, 6, 11, 15] // Calculated from group selection
}
```

### 6. Direct Region Control (Debug Mode Only)

**Purpose**: Individual region testing and troubleshooting.

**Visibility**: Only visible in debug mode

**Elements**:

- Collapsible/expandable section (can start collapsed)
- 20 individual region controls organized by panel
- "Expand/Collapse" toggle for space management

**Layout** (when expanded):

```
┌──────────────────────────────────────────────┐
│  PANEL 1                                     │
│  ☐ BULL_SYMBOL       ☐ BULL_HEAD            │
│  ☐ BHARATHI_EYES     ☐ BHARATHI_SYMBOL       │
│  ☐ BHARATHI_CLOTH                            │
├──────────────────────────────────────────────┤
│  PANEL 2                                     │
│  ☐ CONTINENT_F       ☐ VEENA_SYMBOL         │
│  ☐ VEENA_REST        ☐ DANCER_SYMBOL         │
│  ☐ DANCER_TOP        ☐ DANCER_BOTTOM         │
├──────────────────────────────────────────────┤
│  PANEL 3                                     │
│  ☐ CONTINENT_U       ☐ RAAVANA_HEAD_P3      │
│  ☐ VALLUVAR_SYMBOL   ☐ VALLUVAR_REST         │
│  ☐ CONTINENT_C                               │
├──────────────────────────────────────────────┤
│  PANEL 4                                     │
│  ☐ RAAVANA_HEAD_SYMBOL  ☐ RAAVANA_HEAD_REST │
│  ☐ RAAVANA_CORE         ☐ RAAVANA_TORSO     │
└──────────────────────────────────────────────┘

[Test Selected Regions]  [All On]  [All Off]
```

**Behavior**:

- Checkboxes map directly to `regions[]` array
- Effect control section settings apply when button is clicked
- Helper buttons: "All On", "All Off" for quick selection
- "Apply Effect to Selected" sends command with `debug: true`

**MQTT Payload**:

```json
{
  "debug": true,
  "panelId": 1,
  "effect": 0,
  "brightness": 255,
  "regions": [0, 2, 4] // Array of region indices
}
```

### 7. Status Indicator

**Purpose**: Show connection and system health.

**Elements**:

- Color-coded status dot (●)
  - **Green**: Connected to MQTT
  - **Yellow**: Connecting/Reconnecting
  - **Red**: Disconnected
- Optional: Master heartbeat timestamp (last seen)
- Optional: Panel online status (4 dots for 4 panels)

**Future Enhancement**:

- Subscribe to `ta25stage/master/status` for heartbeat
- Display uptime, WiFi strength, error counts

## Interaction Workflows

### Workflow 1: Quick Performance Control (Primary Use Case)

**Mode**: Normal Mode (default)

**Goal**: Trigger pre-programmed sequence during live performance with custom effect.

**Steps**:

1. Ensure debug mode is OFF (default state)
2. Select effect: "Breathing"
3. Adjust brightness: 80%
4. Adjust speed: 50
5. Tap "Sweep" sequence button

**MQTT Message**:

```json
{
  "sequence": 1,
  "effect": 1,
  "brightness": 204,
  "speed": 50
}
```

**Result**: Vertical sweep sequence runs with breathing effect at 80% brightness.

### Workflow 2: Quick Sequence with Default Settings

**Mode**: Normal Mode (default)

**Goal**: Trigger sequence immediately with current settings.

**Steps**:

1. Tap "Story" sequence button (no parameter adjustment needed)

**MQTT Message**:

```json
{
  "sequence": 2,
  "effect": 1,
  "brightness": 204,
  "speed": 50
}
```

**Result**: Group narrative sequence runs with whatever effect settings are currently displayed.

### Workflow 3: Test Specific Region Group

**Mode**: Debug Mode

**Goal**: Apply breathing effect to all symbolic regions.

**Steps**:

1. Enable debug mode toggle
2. Check "Symbols" in Group Selection
3. Verify effect settings (Breathing, 80%, speed 75)
4. Tap "Apply Effect to Selected"

**MQTT Message**:

```json
{
  "debug": true,
  "effect": 1,
  "brightness": 204,
  "speed": 75,
  "regions": [0, 3, 6, 11, 15]
}
```

**Result**: Only symbol regions light up with breathing effect.

### Workflow 4: Test Single Region

**Mode**: Debug Mode

**Goal**: Verify BULL_SYMBOL region is working.

**Steps**:

1. Enable debug mode toggle
2. Expand Direct Region Control
3. Check "BULL_SYMBOL" (Panel 1, region 0)
4. Set effect to "Static", brightness to 100%
5. Tap "Apply Effect to Selected"

**MQTT Message**:

```json
{
  "debug": true,
  "effect": 0,
  "brightness": 255,
  "regions": [0]
}
```

**Result**: Only BULL_SYMBOL region lights at full brightness.

### Workflow 5: Test Entire Panel

**Mode**: Debug Mode

**Goal**: Test all regions on Panel 2.

**Steps**:

1. Enable debug mode toggle
2. Select "P2" in Panel Selection
3. Set effect to "Wave", speed to 60
4. Tap "Apply Effect to Selected"

**MQTT Message**:

```json
{
  "debug": true,
  "panelId": 2,
  "effect": 2,
  "speed": 60
}
```

**Result**: All Panel 2 regions light up with wave effect.

## JSON Command Reference

### Command Structure

```json
{
  "sequence": 0, // 0-3 (optional, triggers sequence mode)
  "effect": 0, // 0-5 (EFFECT_STATIC=0, BREATHING=1, WAVE=2, PULSE=3, FADE_IN=4, FADE_OUT=5)
  "brightness": 128, // 0-255 (optional, default 128)
  "speed": 50, // 0-100 (optional, default 50)
  "debug": false, // true=direct region control (optional)
  "panelId": 0, // 0=all, 1-4=specific (optional, only in debug mode)
  "regions": [0, 3, 6] // Array of region indices 0-19 (optional, only in debug mode)
}
```

### Mode Selection Logic

**Normal Mode - Sequence Trigger** (when `sequence` field is present):

```json
{
  "sequence": 1,
  "effect": 1,
  "brightness": 204,
  "speed": 50
}
```

- Master runs pre-programmed sequence with specified effect settings
- Sequence calculates which regions to light at each step
- Effect/brightness/speed apply to all regions in sequence

**Debug Mode - Direct Control** (when `debug: true`):

```json
{
  "debug": true,
  "effect": 0,
  "brightness": 255,
  "regions": [0, 2, 4, 6]
}
```

- Bypasses sequence logic
- Directly controls specified regions
- Supports panel targeting with `panelId`

### Example Payloads

**Normal Mode - Trigger Sequence 2 with default settings**:

```json
{
  "sequence": 2,
  "effect": 1,
  "brightness": 128,
  "speed": 50
}
```

**Normal Mode - Trigger Sequence 1 with pulse effect**:

```json
{
  "sequence": 1,
  "effect": 3,
  "brightness": 255,
  "speed": 80
}
```

**Debug Mode - Test Symbols group**:

```json
{
  "debug": true,
  "effect": 0,
  "brightness": 255,
  "regions": [0, 3, 6, 11, 15]
}
```

**Debug Mode - Test RAAVANA_HEAD regions with breathing**:

```json
{
  "debug": true,
  "effect": 1,
  "brightness": 200,
  "speed": 60,
  "regions": [11, 15, 16]
}
```

**Debug Mode - Test Panel 2 with wave effect**:

```json
{
  "debug": true,
  "panelId": 2,
  "effect": 2,
  "speed": 70
}
```

## Design Considerations

### Performance Priority

**Requirements**:

- Large touch targets (min 44x44pt on mobile, 80x80px on web)
- High contrast buttons for stage lighting conditions
- Quick access to sequences (no scrolling/menus)
- Minimal confirmation dialogs (live performance context)

**Recommendations**:

- Dark theme by default (less screen glare on stage)
- Colorful icons for quick visual identification
- Haptic feedback on mobile for button presses
- Landscape orientation support for tablets

### Accessibility

- Sufficient color contrast (WCAG AA minimum)
- Label all controls with clear text (not icons only)
- Support keyboard navigation (tab order)
- Screen reader compatible (ARIA labels)

### Error Handling

**Connection Loss**:

- Visual indicator (red status dot)
- Retry logic with exponential backoff
- Queue commands and replay on reconnect (optional)

**Invalid Commands**:

- Client-side validation before sending
- Toast/notification for user feedback
- No silent failures

**Timeout Detection**:

- Warn if no response from master after 5 seconds
- Show last successful command timestamp

## Future Enhancements

## Development Checklist

### MVP (Minimum Viable Product)

**Normal Mode (Default View)**:

- [ ] MQTT connection setup (broker.emqx.io:1883)
- [ ] Debug mode toggle switch in header
- [ ] Sequences section (4 large buttons)
- [ ] Effect control section (dropdown + 2 sliders)
- [ ] Connection status indicator (colored dot)
- [ ] Basic error handling (connection status)
- [ ] Mobile-responsive layout

**Debug Mode (Toggle Enabled)**:

- [ ] Panel selection (5 buttons: All + 4 panels)
- [ ] Group selection (4 checkboxes)
- [ ] Direct region control (collapsible, 20 checkboxes by panel)
- [ ] "Apply Effect to Selected" button
- [ ] Helper buttons: "All On", "All Off"

## Design Mockup Assets Needed

1. **Icons**:
   - Sequence icons (test, sweep, story, symbol)
   - Panel icons (generic panel representation)
   - Status indicators (online/offline/connecting)
   - Effect icons (breathing wave, pulse, fade)

2. **Colors**:
   - Primary action color (e.g., #4CAF50 green for "apply")
   - Danger color (e.g., #F44336 red for errors)
   - Neutral colors (grays for unselected states)
   - Status colors (green/yellow/red for connection)

3. **Typography**:
   - Readable sans-serif font (e.g., Inter, Roboto)
   - Large text for quick sequences (18-24px)
   - Clear labels for controls (14-16px)

4. **Spacing**:
   - Generous padding for touch targets
   - Clear visual separation between sections
   - Consistent margins (e.g., 16px grid)

## Nice-to-Have

### Phase 2 Features

- [ ] Real-time status monitoring (subscribe to `ta25stage/master/status`)
- [ ] Panel online/offline indicators
- [ ] Sequence interruption (stop/pause)
- [ ] Custom sequence builder (drag-drop timeline)
- [ ] Effect presets (save/load configurations)
- [ ] Audio-reactive mode toggle

### Phase 3 Features

- [ ] Multi-user access control
- [ ] Sequence scheduling (timed triggers)
- [ ] Video preview of stage (camera feed + overlay)
- [ ] Analytics (usage tracking, popular sequences)
- [ ] OTA firmware update from UI
