#ifndef __STATUS_LED_CONTROLLER_H__
#define __STATUS_LED_CONTROLLER_H__

#include "pin_def.h"
#include "esp_log.h"
#include "FastLED.h"

#define NUM_STATUS_LEDS 1

typedef enum
{
  STATUS_LED_BREATHING,
} status_led_effect;

void init_status_led_controller();

void run_status_led_task();

void deinit_status_led_controller();

void show_status_led_effect(void *pvParameters);

void show_breathing_effect();

#endif /* __STATUS_LED_CONTROLLER_H__*/
