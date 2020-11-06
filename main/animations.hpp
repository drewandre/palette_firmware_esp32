#ifndef __ANIMATIONS_H__
#define __ANIMATIONS_H__

#include "FastLED.h"
#include "led_controller.hpp"
#include "iir_filterbank_controller.hpp"
#include "eeprom_controller.hpp"

void map_filterbank();

void map_palette();

void rainbow();

void noise();

#endif