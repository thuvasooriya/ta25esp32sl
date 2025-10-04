#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>

#define MAX_REGIONS 20
#define ESPNOW_WIFI_CHANNEL 6

enum CommandMode {
  MODE_DIRECT_REGIONS = 0,
  MODE_SEQUENCE = 1,
  MODE_GROUP = 2
};

enum EffectType {
  EFFECT_STATIC = 0,
  EFFECT_BREATHING = 1,
  EFFECT_WAVE = 2,
  EFFECT_PULSE = 3,
  EFFECT_FADE_IN = 4,
  EFFECT_FADE_OUT = 5
};

typedef struct {
  uint8_t panelId;
  uint8_t mode;
  uint8_t sequenceId;
  uint8_t groupId;
  bool regions[MAX_REGIONS];
  uint8_t effectType;
  uint8_t brightness;
  uint8_t speed;
  uint8_t step;
  bool audioReactive;
  uint8_t audioIntensity;
} LightCommand;

// Custom MAC addresses for panels (much cleaner!)
uint8_t panel1_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x01};
uint8_t panel2_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x02};
uint8_t panel3_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x03};
uint8_t panel4_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x04};

uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#endif
