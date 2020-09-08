#ifndef __EEPROM_CONTROLLER_H__
#define __EEPROM_CONTROLLER_H__

#include <EEPROM.h>
#include "FastLED.h"

#define MAX_PALETTE_LENGTH 200
#define EEPROM_START_ADDRESS 0
#define NUMBER_OF_SETTINGS_PER_ANIMATION 4

struct AnimationSettings
{
  uint8_t val1;
  uint8_t val2;
  uint8_t val3;
  uint8_t val4;
};

struct Animation
{
  uint8_t animation_number;
  AnimationSettings animation_settings;
};

struct Palette
{
  uint16_t palette_length;
  uint8_t palette_data[MAX_PALETTE_LENGTH];
};

struct SystemSettings
{
  int initialized;
  int eeprom_version;
  bool enabled;
  uint8_t brightness;
  Animation animation;
  Palette palette;
};

extern Palette defaultPalette;
extern Animation defaultAnimation;
extern SystemSettings systemState;

#define SYSTEM_SETTINGS_EEPROM_SIZE sizeof(SystemSettings)

void init_eeprom_controller();

bool is_eeprom_empty();

void persist_state_to_eeprom();

void rehydrate_state_from_eeprom();

void set_animation_number(int animation_number);

void set_palette(int length, uint8_t *data);

void set_animation_settings(int *payload);

void deinit_eeprom_controller();

#endif /* __EEPROM_CONTROLLER_H__*/
