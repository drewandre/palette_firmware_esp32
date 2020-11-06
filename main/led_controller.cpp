#include "led_controller.hpp"
#include "pin_def.h"

const char *LED_TAG = "FASTLED";
static bool initialized = false;

TaskHandle_t FastLEDshowTaskHandle = 0;
TaskHandle_t RunAnimationTaskHandle = 0;
TaskHandle_t RunLEDPaletteTaskHandle = 0;
TaskHandle_t userTaskHandle = 0;

CRGBPalette16 currentPalette(CRGB::Black);
CRGBPalette16 targetPalette(CRGB::Black);
TBlendType currentBlending;

float hue_multiplier = 255.0f / (float)NUM_LEDS;

CRGB leds[NUM_LEDS];
CRGB tempLeds[NUM_LEDS];

// const uint8_t kMatrixWidth = 5;
// const uint8_t kMatrixHeight = 5;
// const bool kMatrixSerpentineLayout = true;
// const uint8_t max_dimension (kMatrixWidth > kMatrixHeight) ? kMatrixWidth : kMatrixHeight;
// uint8_t noise[max_dimension][max_dimension];
// uint8_t noise[NUM_LEDS];

uint8_t animationVal1 = systemState.animation.animation_settings.val1;
uint8_t animationVal2 = systemState.animation.animation_settings.val2;
uint8_t animationVal3 = systemState.animation.animation_settings.val3;
uint8_t animationVal4 = systemState.animation.animation_settings.val4;

uint16_t x;
uint16_t y;
uint16_t z;

/** show() for ESP32
    Call this function instead of FastLED.show(). It signals core 0 to issue a show,
    then waits for a notification that it is done.
*/
void fastled_show_esp32()
{
  if (!initialized) {
    return;
  }
  if (userTaskHandle == 0)
  {
    // -- Store the handle of the current task, so that the show task can
    //    notify it when it's done
    userTaskHandle = xTaskGetCurrentTaskHandle();

    // -- Trigger the show task
    xTaskNotifyGive(FastLEDshowTaskHandle);

    // -- Wait to be notified that it's done
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(200);
    ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
    userTaskHandle = 0;
  }
}

/** show Task
    This function runs on core 0 and just waits for requests to call FastLED.show()
*/
void fastled_show_task(void *pvParameters)
{
  // -- Run forever...
  for (;;)
  {
    // -- Wait for the trigger
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // -- Do the show (synchronously)
    FastLED.setBrightness(animationVal4);
    FastLED.show();
    // -- Notify the calling task
    xTaskNotifyGive(userTaskHandle);
  }
  vTaskDelete(NULL);
}

void run_led_palette_task(void *pvParameters)
{
  for (;;)
  {
    blend_animation_value_toward_value(&animationVal1, &systemState.animation.animation_settings.val1, MAX_ANIMATION_VALUE_CHANGES);
    blend_animation_value_toward_value(&animationVal2, &systemState.animation.animation_settings.val2, MAX_ANIMATION_VALUE_CHANGES);
    blend_animation_value_toward_value(&animationVal3, &systemState.animation.animation_settings.val3, MAX_ANIMATION_VALUE_CHANGES);
    blend_animation_value_toward_value(&animationVal4, &systemState.animation.animation_settings.val4, MAX_ANIMATION_VALUE_CHANGES);
    for (int i = 0; i < NUM_CALLS_TO_N_BLEND_PALETTE_TOWARDS_PALETTE; i++)
    {
      nblendPaletteTowardPalette(currentPalette, targetPalette, 24);
    }
    vTaskDelay(RUN_LED_PALETTE_TASK_DELAY / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void blend_animation_value_toward_value(uint8_t *cVal, uint8_t *tVal, int maxChanges)
{
  uint8_t newVal;
  if (*cVal != *tVal)
  {
    if (*tVal > *cVal)
    {
      newVal = *cVal + maxChanges;
      if ((newVal >= 255) || (newVal >= *tVal))
      {
        *cVal = *tVal;
      }
      else
      {
        *cVal += maxChanges;
        // if the value is still greater, increase it again
        // if ((*cVal >= 255) || (*cVal >= *tVal)) {
        //   *cVal += maxChanges;
        // }
      }
    }
    else
    {
      newVal = *cVal + maxChanges;
      if ((newVal <= 0) || (newVal <= *tVal))
      {
        *cVal = *tVal;
      }
      else
      {
        *cVal -= maxChanges;
        // if ((*cVal <= 0) || (*cVal <= *tVal)) {
        //   *cVal -= maxChanges;
        // }
      }
    }
  }
}

void add_palette(uint8_t *payload, int length)
{
  targetPalette = payload;
  set_palette(length, payload);
}

void run_animation_task(void *pvParameters)
{
  int number_of_audio_analysis_bands = get_num_iir_filters();
  float segment_length = (float)(NUM_LEDS) / (float)(number_of_audio_analysis_bands);
  float color_multiplier = 255.0f / (float)number_of_audio_analysis_bands;
  float band_multiplier = (float)255.0f / (number_of_audio_analysis_bands);
  FilterBank *filter_bank = get_filterbank();

  for (;;)
  {
    blend_animation_value_toward_value(&animationVal1, &systemState.animation.animation_settings.val1, MAX_ANIMATION_VALUE_CHANGES);
    blend_animation_value_toward_value(&animationVal2, &systemState.animation.animation_settings.val2, MAX_ANIMATION_VALUE_CHANGES);
    blend_animation_value_toward_value(&animationVal3, &systemState.animation.animation_settings.val3, MAX_ANIMATION_VALUE_CHANGES);
    blend_animation_value_toward_value(&animationVal4, &systemState.animation.animation_settings.val4, MAX_ANIMATION_VALUE_CHANGES);
    for (int i = 0; i < NUM_CALLS_TO_N_BLEND_PALETTE_TOWARDS_PALETTE; i++)
    {
      nblendPaletteTowardPalette(currentPalette, targetPalette, 24);
    }

    switch (systemState.animation.animation_number)
    {
      case 0:
        map_filterbank();
        fastled_show_esp32();
        break;
      case 1:
        map_palette();
        fastled_show_esp32();
        break;
      case 2:
        rainbow();
        fastled_show_esp32();
        break;
      case 3:
        noise();
        fastled_show_esp32();
        break;
      case 4:
        fill_solid(leds, NUM_LEDS, CRGB::Blue);
        fastled_show_esp32();
        break;
      case 5:
        fill_solid(leds, NUM_LEDS, CRGB::Purple);
        fastled_show_esp32();
        break;
      case 6:
        fill_solid(leds, NUM_LEDS, CRGB::White);
        fastled_show_esp32();
        break;
      default:
        ESP_LOGE(LED_TAG, "Unknown animation number: %i", systemState.animation.animation_number);
        break;
    }
    vTaskDelay(RUN_ANIMATION_TASK_DELAY / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void map_temp_leds_to_leds() {
  // for (int i = 0; i < NUM_LEDS; i++) {
  //   leds[i] += tempLeds[i];
  // }
}

void init_led_controller()
{
  if (initialized)
  {
    ESP_LOGW(LED_TAG, "LED controller has already been initialized. Skipping %s", __func__);
    return;
  }
  ESP_LOGI(LED_TAG, "Initializing LED controller");

  targetPalette = systemState.palette.palette_data;

  FastLED.addLeds<LED_TYPE, GPIO_NUM_32, COLOR_ORDER>(leds, 0 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);
  // FastLED.addLeds<LED_TYPE, LED_PIN_1, COLOR_ORDER>(leds, 1 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);
  // FastLED.addLeds<LED_TYPE, LED_PIN_2, COLOR_ORDER>(leds, 2 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);
  // FastLED.addLeds<LED_TYPE, LED_PIN_3, COLOR_ORDER>(leds, 3 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);
  // FastLED.addLeds<LED_TYPE, LED_PIN_4, COLOR_ORDER>(leds, 4 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);
  // FastLED.addLeds<LED_TYPE, LED_PIN_5, COLOR_ORDER>(leds, 5 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);
  // FastLED.addLeds<LED_TYPE, LED_PIN_6, COLOR_ORDER>(leds, 6 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);
  // FastLED.addLeds<LED_TYPE, LED_PIN_7, COLOR_ORDER>(leds, 7 * NUM_LEDS_PER_STRIP, NUM_LEDS_PER_STRIP).setCorrection(TypicalLEDStrip);

  FastLED.setMaxPowerInVoltsAndMilliamps(LED_MAX_VOLTAGE, LED_MAX_AMPERAGE);
  
  xTaskCreatePinnedToCore(fastled_show_task, "FastLEDshowTask", FASTLED_SHOW_TASK_STACK_SIZE, NULL, FASTLED_SHOW_TASK_PRIORITY, &FastLEDshowTaskHandle, FASTLED_SHOW_TASK_CORE_ID);
  xTaskCreatePinnedToCore(run_animation_task, "RunAnimationTask", RUN_ANIMATION_TASK_STACK_SIZE, NULL, RUN_ANIMATION_TASK_PRIORITY, &RunAnimationTaskHandle, RUN_ANIMATION_TASK_CORE_ID);

  initialized = true;
  ESP_LOGI(LED_TAG, "LED controller initialized");
}

void resume_led_animations() {
  if (initialized) {
    vTaskResume(FastLEDshowTaskHandle);
    vTaskResume(RunAnimationTaskHandle);
    // xTaskCreatePinnedToCore(run_animation_task, "RunAnimationTask", RUN_ANIMATION_TASK_STACK_SIZE, NULL, RUN_ANIMATION_TASK_PRIORITY, &RunAnimationTaskHandle, RUN_ANIMATION_TASK_CORE_ID);
    // xTaskCreatePinnedToCore(run_led_palette_task, "RunColorPaletteTask", RUN_LED_PALETTE_TASK_STACK_SIZE, NULL, RUN_LED_PALETTE_TASK_PRIORITY, &RunLEDPaletteTaskHandle, RUN_LED_PALETTE_TASK_CORE_ID);
    ESP_LOGI(LED_TAG, "Led animations started");
  } else {
    ESP_LOGW(LED_TAG, "Attempted to begin led animations without initializing led controller first");
  }
}

void suspend_led_animations() {
  if (initialized) {
    // blackout();
    vTaskSuspend(RunAnimationTaskHandle);
    vTaskSuspend(FastLEDshowTaskHandle);
    ESP_LOGI(LED_TAG, "Led animations suspended");
  } else {
    ESP_LOGW(LED_TAG, "Attempted to suspend led animations without initializing led controller first");
  }
}

void deinit_led_controller()
{
  if (!initialized) {
    ESP_LOGW(LED_TAG, "LED controller not yet initialized - skipping %s", __func__);
    return;
  }
  vTaskDelete(FastLEDshowTaskHandle);
  vTaskDelete(RunAnimationTaskHandle);
  // vTaskDelete(RunLEDPaletteTaskHandle);
  initialized = false;
  ESP_LOGW(LED_TAG, "LED controller deinitialized");
}