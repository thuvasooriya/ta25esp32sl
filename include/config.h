#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// GPIO PIN DEFINITIONS
// ============================================================================

// Panel 1 GPIO Pins
#define PIN_P1_BULL_SYMBOL 25
#define PIN_P1_BULL_HEAD 26
#define PIN_P1_BHARATHI_EYES 27
#define PIN_P1_BHARATHI_SYMBOL 14
#define PIN_P1_BHARATHI_CLOTH 12

// Panel 2 GPIO Pins
#define PIN_P2_CONTINENT_F 25
#define PIN_P2_VEENA_SYMBOL 26
#define PIN_P2_VEENA_REST 27
#define PIN_P2_DANCER_SYMBOL 14
#define PIN_P2_DANCER_TOP 12
#define PIN_P2_DANCER_BOTTOM 13

// Panel 3 GPIO Pins
#define PIN_P3_CONTINENT_U 25
#define PIN_P3_RAAVANA_HEAD 26
#define PIN_P3_VALLUVAR_SYMBOL 27
#define PIN_P3_VALLUVAR_REST 14
#define PIN_P3_CONTINENT_C 12

// Panel 4 GPIO Pins
#define PIN_P4_RAAVANA_HEAD_SYMBOL 25
#define PIN_P4_RAAVANA_HEAD_REST 26
#define PIN_P4_RAAVANA_CORE 27
#define PIN_P4_RAAVANA_TORSO 14

// ============================================================================
// REGION GROUP TAGS
// ============================================================================

#define GROUP_NONE 0
#define GROUP_SYMBOL (1 << 0)
#define GROUP_RAAVANA_HEAD (1 << 1)
#define GROUP_RAAVANA (1 << 2)
#define GROUP_CONTINENT (1 << 3)
#define GROUP_BULL (1 << 4)
#define GROUP_BHARATHI (1 << 5)
#define GROUP_VEENA (1 << 6)
#define GROUP_DANCER (1 << 7)
#define GROUP_VALLUVAR (1 << 8)

// ============================================================================
// NETWORK CONFIGURATION
// ============================================================================

// #define ESPNOW_WIFI_CHANNEL 6
#define ESPNOW_WIFI_CHANNEL 1

uint8_t panel1_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x01};
uint8_t panel2_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x02};
uint8_t panel3_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x03};
uint8_t panel4_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x04};
uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// ============================================================================
// PROTOCOL DEFINITIONS
// ============================================================================

#define MAX_REGIONS 20

#define DEFAULT_BRIGHTNESS 128
#define DEFAULT_SPEED 50

enum EffectType {
  EFFECT_STATIC = 0,
  EFFECT_BREATHING = 1,
  EFFECT_WAVE = 2,
  EFFECT_PULSE = 3,
  EFFECT_FADE_IN = 4,
  EFFECT_FADE_OUT = 5
};

typedef struct __attribute__((packed)) {
  uint8_t sequence;
  uint8_t effect;
  uint8_t brightness;
  uint8_t speed;
  bool debugMode;
  uint8_t panelId;
  bool regions[MAX_REGIONS];
} LightCommand;

// ============================================================================
// REGION INFORMATION STRUCTURE
// ============================================================================

struct RegionInfo {
  uint8_t globalIndex;
  uint8_t pin;
  const char *name;
  uint8_t verticalPos;
  uint16_t groups;
};

// ============================================================================
// COMPLETE REGION DEFINITIONS (Master reference)
// ============================================================================

const RegionInfo ALL_REGIONS[MAX_REGIONS] PROGMEM = {
    {0, PIN_P1_BULL_SYMBOL, "BULL_SYMBOL", 0, GROUP_BULL | GROUP_SYMBOL},
    {1, PIN_P1_BULL_HEAD, "BULL_HEAD", 1, GROUP_BULL},
    {2, PIN_P1_BHARATHI_EYES, "BHARATHI_EYES", 2, GROUP_BHARATHI},
    {3, PIN_P1_BHARATHI_SYMBOL, "BHARATHI_SYMBOL", 3,
     GROUP_BHARATHI | GROUP_SYMBOL},
    {4, PIN_P1_BHARATHI_CLOTH, "BHARATHI_CLOTH", 4, GROUP_BHARATHI},

    {5, PIN_P2_CONTINENT_F, "CONTINENT_F", 0, GROUP_CONTINENT},
    {6, PIN_P2_VEENA_SYMBOL, "VEENA_SYMBOL", 1, GROUP_VEENA | GROUP_SYMBOL},
    {7, PIN_P2_VEENA_REST, "VEENA_REST", 2, GROUP_VEENA},
    {8, PIN_P2_DANCER_SYMBOL, "DANCER_SYMBOL", 3, GROUP_DANCER | GROUP_SYMBOL},
    {9, PIN_P2_DANCER_TOP, "DANCER_TOP", 4, GROUP_DANCER},
    {10, PIN_P2_DANCER_BOTTOM, "DANCER_BOTTOM", 5, GROUP_DANCER},

    {11, PIN_P3_CONTINENT_U, "CONTINENT_U", 0, GROUP_CONTINENT},
    {12, PIN_P3_RAAVANA_HEAD, "RAAVANA_HEAD_P3", 1,
     GROUP_RAAVANA | GROUP_RAAVANA_HEAD},
    {13, PIN_P3_VALLUVAR_SYMBOL, "VALLUVAR_SYMBOL", 2,
     GROUP_VALLUVAR | GROUP_SYMBOL},
    {14, PIN_P3_VALLUVAR_REST, "VALLUVAR_REST", 3, GROUP_VALLUVAR},
    {15, PIN_P3_CONTINENT_C, "CONTINENT_C", 4, GROUP_CONTINENT},

    {16, PIN_P4_RAAVANA_HEAD_SYMBOL, "RAAVANA_HEAD_SYMBOL", 0,
     GROUP_RAAVANA | GROUP_RAAVANA_HEAD | GROUP_SYMBOL},
    {17, PIN_P4_RAAVANA_HEAD_REST, "RAAVANA_HEAD_REST", 1,
     GROUP_RAAVANA | GROUP_RAAVANA_HEAD},
    {18, PIN_P4_RAAVANA_CORE, "RAAVANA_CORE", 2, GROUP_RAAVANA},
    {19, PIN_P4_RAAVANA_TORSO, "RAAVANA_TORSO", 3, GROUP_RAAVANA}};

// ============================================================================
// PANEL-SPECIFIC CONFIGURATIONS (only for panel builds)
// ============================================================================

#ifdef PANEL_ID

#if PANEL_ID == 1
#define NUM_REGIONS 5
const RegionInfo regionConfig[NUM_REGIONS] PROGMEM = {
    {0, PIN_P1_BULL_SYMBOL, "BULL_SYMBOL", 0, GROUP_BULL | GROUP_SYMBOL},
    {1, PIN_P1_BULL_HEAD, "BULL_HEAD", 1, GROUP_BULL},
    {2, PIN_P1_BHARATHI_EYES, "BHARATHI_EYES", 2, GROUP_BHARATHI},
    {3, PIN_P1_BHARATHI_SYMBOL, "BHARATHI_SYMBOL", 3,
     GROUP_BHARATHI | GROUP_SYMBOL},
    {4, PIN_P1_BHARATHI_CLOTH, "BHARATHI_CLOTH", 4, GROUP_BHARATHI}};

#elif PANEL_ID == 2
#define NUM_REGIONS 6
const RegionInfo regionConfig[NUM_REGIONS] PROGMEM = {
    {5, PIN_P2_CONTINENT_F, "CONTINENT_F", 0, GROUP_CONTINENT},
    {6, PIN_P2_VEENA_SYMBOL, "VEENA_SYMBOL", 1, GROUP_VEENA | GROUP_SYMBOL},
    {7, PIN_P2_VEENA_REST, "VEENA_REST", 2, GROUP_VEENA},
    {8, PIN_P2_DANCER_SYMBOL, "DANCER_SYMBOL", 3, GROUP_DANCER | GROUP_SYMBOL},
    {9, PIN_P2_DANCER_TOP, "DANCER_TOP", 4, GROUP_DANCER},
    {10, PIN_P2_DANCER_BOTTOM, "DANCER_BOTTOM", 5, GROUP_DANCER}};

#elif PANEL_ID == 3
#define NUM_REGIONS 5
const RegionInfo regionConfig[NUM_REGIONS] PROGMEM = {
    {11, PIN_P3_CONTINENT_U, "CONTINENT_U", 0, GROUP_CONTINENT},
    {12, PIN_P3_RAAVANA_HEAD, "RAAVANA_HEAD_P3", 1,
     GROUP_RAAVANA | GROUP_RAAVANA_HEAD},
    {13, PIN_P3_VALLUVAR_SYMBOL, "VALLUVAR_SYMBOL", 2,
     GROUP_VALLUVAR | GROUP_SYMBOL},
    {14, PIN_P3_VALLUVAR_REST, "VALLUVAR_REST", 3, GROUP_VALLUVAR},
    {15, PIN_P3_CONTINENT_C, "CONTINENT_C", 4, GROUP_CONTINENT}};

#elif PANEL_ID == 4
#define NUM_REGIONS 4
const RegionInfo regionConfig[NUM_REGIONS] PROGMEM = {
    {16, PIN_P4_RAAVANA_HEAD_SYMBOL, "RAAVANA_HEAD_SYMBOL", 0,
     GROUP_RAAVANA | GROUP_RAAVANA_HEAD | GROUP_SYMBOL},
    {17, PIN_P4_RAAVANA_HEAD_REST, "RAAVANA_HEAD_REST", 1,
     GROUP_RAAVANA | GROUP_RAAVANA_HEAD},
    {18, PIN_P4_RAAVANA_CORE, "RAAVANA_CORE", 2, GROUP_RAAVANA},
    {19, PIN_P4_RAAVANA_TORSO, "RAAVANA_TORSO", 3, GROUP_RAAVANA}};

#else
#error "PANEL_ID must be defined as 1, 2, 3, or 4"
#endif

inline const char *getRegionName(uint8_t index) {
  if (index >= NUM_REGIONS)
    return "INVALID";
  return (const char *)pgm_read_ptr(&regionConfig[index].name);
}

inline uint8_t getRegionPin(uint8_t index) {
  if (index >= NUM_REGIONS)
    return 0;
  return pgm_read_byte(&regionConfig[index].pin);
}

inline uint8_t getRegionVerticalPos(uint8_t index) {
  if (index >= NUM_REGIONS)
    return 0xFF;
  return pgm_read_byte(&regionConfig[index].verticalPos);
}

inline uint8_t getRegionGlobalIndex(uint8_t index) {
  if (index >= NUM_REGIONS)
    return 0xFF;
  return pgm_read_byte(&regionConfig[index].globalIndex);
}

inline uint16_t getRegionGroups(uint8_t index) {
  if (index >= NUM_REGIONS)
    return GROUP_NONE;
  return pgm_read_word(&regionConfig[index].groups);
}

#endif

// ============================================================================
// MASTER SEQUENCE HELPERS (auto-generated from ALL_REGIONS)
// ============================================================================

#ifndef PANEL_ID

class RegionGroups {
public:
  static void getByGroup(uint16_t groupMask, uint8_t *buffer, uint8_t &count) {
    count = 0;
    for (uint8_t i = 0; i < MAX_REGIONS; i++) {
      uint16_t groups = pgm_read_word(&ALL_REGIONS[i].groups);
      if (groups & groupMask) {
        buffer[count++] = i;
      }
    }
  }

  static uint8_t getCount(uint16_t groupMask) {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_REGIONS; i++) {
      uint16_t groups = pgm_read_word(&ALL_REGIONS[i].groups);
      if (groups & groupMask) {
        count++;
      }
    }
    return count;
  }
};

const uint8_t PANEL_REGION_COUNTS[] = {5, 6, 5, 4};
const uint8_t PANEL_REGION_OFFSETS[] = {0, 5, 11, 16, 20};

#endif

#endif
