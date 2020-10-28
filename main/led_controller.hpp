#ifndef __LED_CONTROLLER_H__
#define __LED_CONTROLLER_H__

#include "FastLED.h"
#include <string>
#include "esp_log.h"
#include "eeprom_controller.hpp"
#include "free_rtos_controller.hpp"
#include "application_task_controller.hpp"
#include "iir_filterbank_controller.hpp"

#define NUM_STRIPS 1
#define NUM_LEDS_PER_STRIP 100
const int numLeds = NUM_STRIPS * NUM_LEDS_PER_STRIP;
#define NUM_LEDS numLeds

#define LED_MAX_VOLTAGE 5
#define LED_MAX_AMPERAGE 30000

#define LED_TYPE WS2812B
#define COLOR_ORDER GRB

// #define LOG_FASTLED_PERFORMANCE

#define NUM_CALLS_TO_N_BLEND_PALETTE_TOWARDS_PALETTE 3
#define MAX_ANIMATION_VALUE_CHANGES 3

void init_led_controller();

void run_led_palette_task(void *pvParameters);

void map_temp_leds_to_leds();

void run_animation_task(void *pvParameters);

void fill_noise_8();

void map_noise_to_leds_using_palette();

uint16_t XY(uint8_t x, uint8_t y);

void noise_1d();

void noise_2d();

void blackout();

void show_leds();

void suspend_led_animations();

void resume_led_animations();

void test_cylon_speed();

void log_led_performance();

void show_led_ota_percentage(float percentage);

void add_palette(uint8_t *payload, int length);

void blend_animation_value_toward_value(uint8_t *cVal, uint8_t *tVal, int maxChanges);

void deinit_led_controller();

void map_palette();

void rainbow();

void addGlitter( fract8 chanceOfGlitter);

void run_led_audio_animation(float *buffer, int len);

#endif /* __LED_CONTROLLER_H__*/