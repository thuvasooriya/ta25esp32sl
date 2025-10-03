#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>

#define NUM_REGIONS 7

typedef struct {
  uint8_t panelId;
  uint8_t patternId;
  uint8_t brightness;
  bool regions[NUM_REGIONS];
  uint8_t speed;
  bool audioReactive;
  uint8_t audioIntensity;
} LEDCommand;

// Custom MAC addresses for panels (much cleaner!)
uint8_t panel1_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x01};
uint8_t panel2_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x02};
uint8_t panel3_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x03};
uint8_t panel4_mac[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x04};

uint8_t broadcast_mac[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

#endif
