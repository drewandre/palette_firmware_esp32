#ifndef __LED_CONTROLLER_H__
#define __LED_CONTROLLER_H__

#include <Arduino.h>
#include "FastLED.h"

#define NUM_FIXTURES 1
#define NUM_LEDS_PER_FIXTURE 100
#define NUM_LEDS 100
#define DATA_PIN GPIO_NUM_12
#define LED_TAG "FASTLED"

static CRGBPalette16 currentPalette = RainbowColors_p;
static TBlendType currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 IRAM_ATTR myRedWhiteBluePalette_p;
static CRGB leds[NUM_LEDS];

static const uint8_t kMatrixWidth = NUM_LEDS_PER_FIXTURE;
static const uint8_t kMatrixHeight = NUM_FIXTURES;
static const bool kMatrixSerpentineLayout = true;
#define MAX_DIMENSION ((kMatrixWidth > kMatrixHeight) ? kMatrixWidth : kMatrixHeight)
static uint16_t speed = 10; // speed is set dynamically once we've started up
static uint16_t scale = 30; // scale is set dynamically once we've started up
static uint8_t noise[MAX_DIMENSION][MAX_DIMENSION];

static int iHue = 255 / 100;
static int currentAnimation = 0;

static uint16_t x;
static uint16_t y;
static uint16_t z;

void init_leds();
void run_animation_task(void *pvParameters);
void fill_noise_8();
void map_noise_to_leds_using_palette();
uint16_t XY(uint8_t x, uint8_t y);
void noise2d();
void show_leds();

#endif
