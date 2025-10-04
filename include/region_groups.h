#ifndef REGION_GROUPS_H
#define REGION_GROUPS_H

#include <Arduino.h>

const uint8_t SYMBOL_REGIONS[] = {0, 3, 6, 8, 13};
const uint8_t SYMBOL_COUNT = 5;

const uint8_t RAAVANA_HEAD_REGIONS[] = {12, 16, 17};
const uint8_t RAAVANA_HEAD_COUNT = 3;

const uint8_t RAAVANA_REGIONS[] = {12, 16, 17, 18, 19};
const uint8_t RAAVANA_COUNT = 5;

const uint8_t CONTINENT_REGIONS[] = {5, 11, 15};
const uint8_t CONTINENT_COUNT = 3;

const uint8_t PANEL_REGION_COUNTS[] = {5, 6, 5, 4};
const uint8_t PANEL_REGION_OFFSETS[] = {0, 5, 11, 16, 20};

#endif
