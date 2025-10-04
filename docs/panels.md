# Panel Configurations

Each panel has distinct regions with named identifiers and GPIO pin assignments. All regions are listed from top to bottom within each panel.

## Panel 1 (5 Regions)

| Region | GPIO Pin | Name             | Local Group | Cross-Panel Group |
|--------|----------|------------------|-------------|-------------------|
| 0      | 25       | BULL_SYMBOL      | BULL        | SYMBOL            |
| 1      | 26       | BULL_HEAD        | BULL        | -                 |
| 2      | 27       | BHARATHI_EYES    | BHARATHI    | -                 |
| 3      | 14       | BHARATHI_SYMBOL  | BHARATHI    | SYMBOL            |
| 4      | 12       | BHARATHI_CLOTH   | BHARATHI    | -                 |

**Local Groups**: BULL, BHARATHI

## Panel 2 (6 Regions)

| Region | GPIO Pin | Name             | Local Group | Cross-Panel Group |
|--------|----------|------------------|-------------|-------------------|
| 0      | 25       | CONTINENT_F      | -           | CONTINENT         |
| 1      | 26       | VEENA_SYMBOL     | VEENA       | SYMBOL            |
| 2      | 27       | VEENA_REST       | VEENA       | -                 |
| 3      | 14       | DANCER_SYMBOL    | DANCER      | SYMBOL            |
| 4      | 12       | DANCER_TOP       | DANCER      | -                 |
| 5      | 13       | DANCER_BOTTOM    | DANCER      | -                 |

**Local Groups**: VEENA, DANCER

## Panel 3 (5 Regions)

| Region | GPIO Pin | Name             | Local Group | Cross-Panel Group |
|--------|----------|------------------|-------------|-------------------|
| 0      | 25       | CONTINENT_U      | -           | CONTINENT         |
| 1      | 26       | RAAVANA_HEAD_P3  | RAAVANA     | RAAVANA_HEAD      |
| 2      | 27       | VALLUVAR_SYMBOL  | VALLUVAR    | SYMBOL            |
| 3      | 14       | VALLUVAR_REST    | VALLUVAR    | -                 |
| 4      | 12       | CONTINENT_C      | -           | CONTINENT         |

**Local Groups**: VALLUVAR, RAAVANA (local)

## Panel 4 (4 Regions)

| Region | GPIO Pin | Name                | Local Group | Cross-Panel Group |
|--------|----------|---------------------|-------------|-------------------|
| 0      | 25       | RAAVANA_HEAD_SYMBOL | RAAVANA     | RAAVANA_HEAD      |
| 1      | 26       | RAAVANA_HEAD_REST   | RAAVANA     | RAAVANA_HEAD      |
| 2      | 27       | RAAVANA_CORE        | RAAVANA     | RAAVANA           |
| 3      | 14       | RAAVANA_TORSO       | RAAVANA     | RAAVANA           |

**Local Groups**: RAAVANA (local)

## Cross-Panel Groups

Groups that span multiple panels for coordinated effects:

- **SYMBOL**: Symbolic regions across all 4 panels (5 regions total)
- **RAAVANA**: Raavana body parts (Panels 3, 4)
- **RAAVANA_HEAD**: Raavana head specifically (Panels 3, 4)
- **CONTINENT**: Continent regions (Panel 2, 3)

## GPIO Pin Notes

**Current Configuration:**
- All panels use GPIO pins: 25, 26, 27, 14, 12
- Panel 2 additionally uses GPIO 13
- These are placeholder assignments for development

**Production Deployment:**
- Verify GPIO pins match actual hardware connections
- Update `include/panel_config.h` with correct pin assignments
- Avoid pins with special boot behavior (GPIO 0, 2, 15)
- PWM channels auto-assigned based on region count

## Sequences (Future Implementation)

Sequences are master-side choreographed narratives. The master calculates which regions to light at each step and broadcasts commands to panels.

### Sequence 0: Test Sequence (Auto-Boot)

**Purpose**: Hardware validation and brightness calibration

**Steps**:
1. Light all 20 regions at full brightness for 2 seconds
2. Run wave effect across all panels
3. Run pulse effect with varying speeds
4. Cycle through fade in/out effects

**Trigger**: Automatically runs on master boot after initialization

**Status**: Not yet implemented

### Sequence 1: Vertical Sweep

**Purpose**: Top-to-bottom cascading illumination

**Flow**:
1. Start with topmost region of Panel 1 (region 0)
2. Progress downward through Panel 1 regions (0→4)
3. Continue to Panel 2 top (region 0) through bottom (5)
4. Continue through Panels 3 and 4
5. When all regions lit, hold for 2 seconds
6. Reverse: fade out bottom-to-top
7. Optional: Repeat with different effects

**Status**: Not yet implemented

### Sequence 2: Group Narrative

**Purpose**: Symbolic storytelling through grouped illumination

**Choreography**:
1. Light SYMBOL group (5 regions across all panels)
2. Light RAAVANA_HEAD group (Panels 3, 4)
3. Expand to full RAAVANA group (adds core and torso)
4. Light CONTINENT group last (symbolizing world/context)

**Timing**: Each group holds for 3-5 seconds before next

**Status**: Not yet implemented

### Sequence 3: Symbol Emergence

**Purpose**: Evoke emergence of meaning from symbols

**Flow**:
1. Begin with all regions dark
2. Fade in SYMBOL regions simultaneously (0.5s fade)
3. Hold symbols at full brightness for 3 seconds
4. Gradually fade in all remaining regions (2s fade)
5. Hold full illumination for 5 seconds
6. Optional: Reverse for closing effect

**Symbolism**: Represents concepts manifesting before physical forms

**Status**: Not yet implemented

## Implementation Notes

- Sequences use `mode=1` in `LightCommand` struct
- Master calculates region arrays and broadcasts to panels
- Panels execute effects (breathing, fade, etc.) on specified regions
- Sequence logic resides in master firmware (`src/master/main.cpp`)
- See `AGENTS.md` for implementation guidelines
