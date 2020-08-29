#include "eeprom_controller.hpp"

const char *EEPROM_TAG = "EEPROM";
static bool initialized = false;

// default rainbow palette
Palette defaultPalette = {
    .palette_length = 28,
    .palette_data = {0, 255, 0, 0, 36, 255, 165, 0, 72, 255, 218, 33, 108, 51, 221, 0, 144, 17, 51, 204, 180, 34, 0, 102, 255, 51, 0, 68},
};

AnimationSettings defaultAnimationSettings = {
    .val1 = 5,
    .val2 = 5,
    .val3 = 5,
    .val4 = 255,
};

Animation defaultAnimation = {
    .animation_number = 0,
    .animation_settings = defaultAnimationSettings,
};

SystemSettings systemState = {
    .initialized = 1,
    .eeprom_version = 1,
    .enabled = true,
    .brightness = 255,
    .animation = defaultAnimation,
    .palette = defaultPalette,
};

void init_eeprom_controller()
{
  if (initialized)
  {
    ESP_LOGW(EEPROM_TAG, "EEPROM controller has already been initialized. Skipping %s", __func__);
    return;
  }
  ESP_LOGI(EEPROM_TAG, "Initializing EEPROM controller");

  EEPROM.begin(SYSTEM_SETTINGS_EEPROM_SIZE);

  bool eepromIsUninitialized = is_eeprom_empty();
  if (eepromIsUninitialized)
  {
    persist_state_to_eeprom();
    ESP_LOGW(EEPROM_TAG, "EEPROM initialized with default system settings");
  }
  else
  {
    // TODO: Check incoming state version vs. current state storage and migrate if necessary
    rehydrate_state_from_eeprom();
  }
  initialized = true;
}

void persist_state_to_eeprom()
{
  // EEPROM.put(EEPROM_START_ADDRESS, systemState);
  // EEPROM.commit();
}

void rehydrate_state_from_eeprom()
{
  EEPROM.get(EEPROM_START_ADDRESS, systemState);
  ESP_LOGI(EEPROM_TAG, "System state rehydrated from EEPROM");
}

bool is_eeprom_empty()
{
  return EEPROM.read(EEPROM_START_ADDRESS) == 255; // start address == initialized boolean
}

void set_animation_number(int animation_number)
{
  systemState.animation.animation_number = animation_number;
  // persist_state_to_eeprom();
  ESP_LOGI(EEPROM_TAG, "Animation changed (%i)", animation_number);
}

void set_palette(int length, byte *data)
{
  systemState.palette.palette_length = length;
  memcpy(systemState.palette.palette_data, data, length);
  // persist_state_to_eeprom();
  ESP_LOGI(EEPROM_TAG, "Palette changed (length: %i)", length);
}

void set_animation_settings(int *payload)
{
  systemState.animation.animation_settings.val1 = payload[0];
  systemState.animation.animation_settings.val2 = payload[1];
  systemState.animation.animation_settings.val3 = payload[2];
  systemState.animation.animation_settings.val4 = payload[3];
  // persist_state_to_eeprom();
  ESP_LOGI(EEPROM_TAG, "Animation settings changed (%i, %i, %i, %i)", payload[0], payload[1], payload[2], payload[3]);
}

void deinit_eeprom_controller()
{
  if (!initialized) {
    ESP_LOGW(EEPROM_TAG, "EEPROM controller not yet initialized - skipping %s", __func__);
    return;
  }
  EEPROM.end();
  initialized = false;
  ESP_LOGW(EEPROM_TAG, "EEPROM controller deinitialized");
}