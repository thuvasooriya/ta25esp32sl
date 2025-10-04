#ifndef PANEL_CONFIG_H
#define PANEL_CONFIG_H

#include "common.h"
#include <Arduino.h>

enum LocalGroup {
  LGROUP_BULL,
  LGROUP_BHARATHI,
  LGROUP_VEENA,
  LGROUP_DANCER,
  LGROUP_VALLUVAR,
  LGROUP_RAAVANA_LOCAL,
  LGROUP_NONE = 0xFF
};

enum CrossPanelGroup {
  XGROUP_SYMBOL,
  XGROUP_RAAVANA,
  XGROUP_RAAVANA_HEAD,
  XGROUP_CONTINENT,
  XGROUP_NONE = 0xFF
};

struct RegionInfo {
  uint8_t pin;
  const char *name;
  uint8_t verticalPos;
  uint8_t localGroup;
  uint8_t crossPanelGroup;
};

#if PANEL_ID == 1
#define NUM_REGIONS 5
const RegionInfo regionConfig[NUM_REGIONS] PROGMEM = {
    {25, "BULL_SYMBOL", 0, LGROUP_BULL, XGROUP_SYMBOL},
    {26, "BULL_HEAD", 1, LGROUP_BULL, XGROUP_NONE},
    {27, "BHARATHI_EYES", 2, LGROUP_BHARATHI, XGROUP_NONE},
    {14, "BHARATHI_SYMBOL", 3, LGROUP_BHARATHI, XGROUP_SYMBOL},
    {12, "BHARATHI_CLOTH", 4, LGROUP_BHARATHI, XGROUP_NONE}};

#elif PANEL_ID == 2
#define NUM_REGIONS 6
const RegionInfo regionConfig[NUM_REGIONS] PROGMEM = {
    {25, "CONTINENT_F", 0, LGROUP_NONE, XGROUP_CONTINENT},
    {26, "VEENA_SYMBOL", 1, LGROUP_VEENA, XGROUP_SYMBOL},
    {27, "VEENA_REST", 2, LGROUP_VEENA, XGROUP_NONE},
    {14, "DANCER_SYMBOL", 3, LGROUP_DANCER, XGROUP_SYMBOL},
    {12, "DANCER_TOP", 4, LGROUP_DANCER, XGROUP_NONE},
    {13, "DANCER_BOTTOM", 5, LGROUP_DANCER, XGROUP_NONE}};

#elif PANEL_ID == 3
#define NUM_REGIONS 5
const RegionInfo regionConfig[NUM_REGIONS] PROGMEM = {
    {25, "CONTINENT_U", 0, LGROUP_NONE, XGROUP_CONTINENT},
    {26, "RAAVANA_HEAD_P3", 1, LGROUP_RAAVANA_LOCAL, XGROUP_RAAVANA_HEAD},
    {27, "VALLUVAR_SYMBOL", 2, LGROUP_VALLUVAR, XGROUP_SYMBOL},
    {14, "VALLUVAR_REST", 3, LGROUP_VALLUVAR, XGROUP_NONE},
    {12, "CONTINENT_C", 4, LGROUP_NONE, XGROUP_CONTINENT}};

#elif PANEL_ID == 4
#define NUM_REGIONS 4
const RegionInfo regionConfig[NUM_REGIONS] PROGMEM = {
    {25, "RAAVANA_HEAD_SYMBOL", 0, LGROUP_RAAVANA_LOCAL, XGROUP_RAAVANA_HEAD},
    {26, "RAAVANA_HEAD_REST", 1, LGROUP_RAAVANA_LOCAL, XGROUP_RAAVANA_HEAD},
    {27, "RAAVANA_CORE", 2, LGROUP_RAAVANA_LOCAL, XGROUP_RAAVANA},
    {14, "RAAVANA_TORSO", 3, LGROUP_RAAVANA_LOCAL, XGROUP_RAAVANA}};

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

inline uint8_t getRegionLocalGroup(uint8_t index) {
  if (index >= NUM_REGIONS)
    return LGROUP_NONE;
  return pgm_read_byte(&regionConfig[index].localGroup);
}

inline uint8_t getRegionCrossPanelGroup(uint8_t index) {
  if (index >= NUM_REGIONS)
    return XGROUP_NONE;
  return pgm_read_byte(&regionConfig[index].crossPanelGroup);
}

#endif
