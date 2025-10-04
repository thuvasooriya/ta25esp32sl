#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>

#define MAX_REGIONS 20
#define ESPNOW_WIFI_CHANNEL 6

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

uint8_t panel1_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x01};
uint8_t panel2_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x02};
uint8_t panel3_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x03};
uint8_t panel4_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x04};

uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#endif
