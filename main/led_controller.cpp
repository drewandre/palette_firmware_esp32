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

CRGB leds[NUM_LEDS];
// CRGB tempLeds[NUM_LEDS];

const uint8_t kMatrixWidth = 5;
const uint8_t kMatrixHeight = 5;
const bool kMatrixSerpentineLayout = true;
#define MAX_DIMENSION ((kMatrixWidth > kMatrixHeight) ? kMatrixWidth : kMatrixHeight)
uint8_t animationVal1;
uint8_t animationVal2;
uint8_t animationVal3;
uint8_t animationVal4;
uint8_t noise[MAX_DIMENSION][MAX_DIMENSION];

float iHue = 255 / (float)NUM_LEDS;

uint16_t x;
uint16_t y;
uint16_t z;

/** show() for ESP32
    Call this function instead of FastLED.show(). It signals core 0 to issue a show,
    then waits for a notification that it is done.
*/
void fastled_show_esp32()
{
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
    // FastLED.setBrightness(animationVal4);
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
        if ((*cVal >= 255) || (*cVal >= *tVal)) {
          *cVal += maxChanges;
        }
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
        if ((*cVal <= 0) || (*cVal <= *tVal)) {
          *cVal -= maxChanges;
        }
      }
    }
  }
}

void add_palette(uint8_t *payload, int length)
{
  targetPalette = payload;
  set_palette(length, payload);
}

long map(long x, long in_min, long in_max, long out_min, long out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void blackout() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  fastled_show_esp32();
  FastLED.show();
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  fastled_show_esp32();
  FastLED.show();
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  fastled_show_esp32();
  FastLED.show();
  fill_solid(leds, NUM_LEDS, CRGB::Black);
  fastled_show_esp32();
  FastLED.show();
}

void run_animation_task(void *pvParameters)
{
  // blackout();
  int number_of_audio_analysis_bands = get_averages_size();
  float segment_length = (float)NUM_LEDS / number_of_audio_analysis_bands;
  float color_multiplier = 255.0 / (float)number_of_audio_analysis_bands;
  float filter_values[number_of_audio_analysis_bands];
  int myVar = 0;
  int i = 0;

  for (;;)
  {
    blend_animation_value_toward_value(&animationVal1, &systemState.animation.animation_settings.val1, MAX_ANIMATION_VALUE_CHANGES);
    blend_animation_value_toward_value(&animationVal2, &systemState.animation.animation_settings.val2, MAX_ANIMATION_VALUE_CHANGES);
    blend_animation_value_toward_value(&animationVal3, &systemState.animation.animation_settings.val3, MAX_ANIMATION_VALUE_CHANGES);
    blend_animation_value_toward_value(&animationVal4, &systemState.animation.animation_settings.val4, MAX_ANIMATION_VALUE_CHANGES);
    // printf("%3i %3i %3i %3i\n", animationVal1, animationVal2, animationVal3, animationVal4);
    for (int i = 0; i < NUM_CALLS_TO_N_BLEND_PALETTE_TOWARDS_PALETTE; i++)
    {
      nblendPaletteTowardPalette(currentPalette, targetPalette, 24);
    }

    // switch (systemState.animation.animation_number)
    switch (myVar)
    {
    // FOR IIR FILTER
    case 0:
      // fill_solid(leds, NUM_LEDS, CHSV(i, 255, 255));
      // i++;
      // fastled_show_esp32();
      // break;
      memcpy(filter_values, get_audio_analysis_results(), number_of_audio_analysis_bands * sizeof(float));
      #ifdef DEBUG_AUDIO_ANALYSIS
        printf("\033[2J\033[1;1H");
      #endif
      for (int i = 0; i < number_of_audio_analysis_bands; i++) {
      #ifdef DEBUG_AUDIO_ANALYSIS
        printf("%f\t", filter_values[i] /* * 5100.0 */);
      #endif
        int current_brightness = filter_values[i] * 300.0f;
        int next_brightness = i == number_of_audio_analysis_bands - 1 ? current_brightness : filter_values[i + 1] * 300.0f;
        int current_hue = i * color_multiplier;
        if (current_hue > 255) {
          current_hue = 255;
        }
        int next_hue = i == number_of_audio_analysis_bands - 1 ? current_hue : (i + 1) * color_multiplier;
        if (next_hue > 255) {
          next_hue = 255;
        }
        for (int l = i * segment_length; l < (i + 1) * segment_length; l++) {
          leds[l] = CHSV(current_hue, 255, current_brightness);
          // fill_gradient(
          //   tempLeds,
          //   i * segment_length,
          //   CHSV(current_hue, 255, current_brightness),
          //   ((i + 1) * segment_length),
          //   CHSV(next_hue, 255, next_brightness),
          //   SHORTEST_HUES
          // );
        }
      }
      #ifdef DEBUG_AUDIO_ANALYSIS
        printf("\n");
      #endif
      // map_temp_leds_to_leds();
      fastled_show_esp32();
      fadeToBlackBy(leds, NUM_LEDS, 50);
      break;

    // case 0:
    //   // fill_solid(leds, NUM_LEDS, CHSV(i, 255, 255));
    //   // i++;
    //   // fastled_show_esp32();
    //   // break;
    //   memcpy(filter_values, get_audio_analysis_results(), number_of_audio_analysis_bands * sizeof(float));
    //   #ifdef DEBUG_AUDIO_ANALYSIS
    //     printf("\033[2J\033[1;1H");
    //   #endif
    //   for (int i = 0; i < number_of_audio_analysis_bands; i++) {
    //     int current_brightness = 255 * filter_values[i];

    //   #ifdef DEBUG_AUDIO_ANALYSIS
    //     printf("%i\t", current_brightness);
    //   #endif
    //     int current_hue = i * color_multiplier;
    //     if (current_hue > 255) {
    //       current_hue = 255;
    //     }
    //     // int next_brightness = i == number_of_audio_analysis_bands - 1 ? current_brightness : filter_values[i + 1] * 255;
    //     // int next_hue = i == number_of_audio_analysis_bands - 1 ? current_hue : (i + 1) * color_multiplier;
    //     // if (next_hue > 255) {
    //     //   next_hue = 255;
    //     // }
    //     for (int l = i * segment_length; l < (i + 1) * segment_length; l++) {
    //       leds[l] = CHSV(current_hue, 255, current_brightness);
    //       // fill_gradient(
    //       //   tempLeds,
    //       //   i * segment_length,
    //       //   CHSV(current_hue, 255, current_brightness),
    //       //   ((i + 1) * segment_length),
    //       //   CHSV(next_hue, 255, next_brightness),
    //       //   SHORTEST_HUES
    //       // );
    //     }
    //   }
    //   #ifdef DEBUG_AUDIO_ANALYSIS
    //     printf("\n");
    //   #endif
    //   // map_temp_leds_to_leds();
    //   fastled_show_esp32();
    //   fadeToBlackBy(leds, NUM_LEDS, 50);
    //   break;
    case 1:
      map_palette();
      fastled_show_esp32();
      break;
    case 2:
      rainbow();
      fastled_show_esp32();
      // fadeToBlackBy(leds, NUM_LEDS, 50);
      // mapAudioAnalysisMono();
      break;
    case 3:
      // horizontalFlex();
      // noise_2d();
      fill_solid(leds, NUM_LEDS, CRGB::Red);
      fastled_show_esp32();
      break;
    case 4:
      // solidColor(true);
      // fadeToBlackBy(leds, NUM_LEDS, 45);
      // FillLEDsFromPaletteColors();
      // blur1d(leds, NUM_LEDS, 5);
      fill_solid(leds, NUM_LEDS, CRGB::Blue);
      fastled_show_esp32();
      break;
    case 5:
      // mapAudioAnalysisMonoSmoothHorizontal();
      // fadeToBlackBy(leds, NUM_LEDS, 50);
      fill_solid(leds, NUM_LEDS, CRGB::Purple);
      fastled_show_esp32();
      break;
    case 6:
      // mapPalette();
      // FillLEDsFromPaletteColors();
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

void map_palette()
{
  static uint8_t startIndex = 0;
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, i * iHue, 255, LINEARBLEND);
  }
}

void addGlitter(fract8 chanceOfGlitter) 
{
  if (random8() < chanceOfGlitter) {
    leds[random16(NUM_LEDS)] += CRGB::White;
  }
}

void rainbow()
{
  // static int initial_hue = 0;
  // initial_hue-=(animationVal2*0.1);
  // fill_rainbow(leds, NUM_LEDS, initial_hue, float(animationVal1)*0.05);
  // // map_temp_leds_to_leds();
  static int initial_hue = 0;
  initial_hue++;
  // initial_hue -= (animationVal2 * 0.1);
  // fill_rainbow(leds, NUM_LEDS, initial_hue, float(animationVal1) * 0.05);
  fill_rainbow(leds, NUM_LEDS, initial_hue, 1);
  // map_temp_leds_to_leds();

}

void noise_2d()
{
  fill_noise_8();
  map_noise_to_leds_using_palette();
}

void noise_1d()
{
  fill_noise_8();
  map_noise_to_leds_using_palette();
}

// Fill the x/y array of 8-bit noise values using the inoise8 function.
void fill_noise_8()
{
  // If we're runing at a low "speed", some 8-bit artifacts become visible
  // from frame-to-frame.  In order to reduce this, we can do some fast data-smoothing.
  // The amount of data smoothing we're doing depends on "speed" (animationVal2).
  uint8_t dataSmoothing = 0;
  if (animationVal2 < 50)
  {
    dataSmoothing = 200 - (animationVal2 * 4);
  }

  for (int i = 0; i < MAX_DIMENSION; i++)
  {
    int ioffset = animationVal1 * i;
    for (int j = 0; j < MAX_DIMENSION; j++)
    {
      int joffset = animationVal1 * j;

      uint8_t data = inoise8(x + ioffset, y + joffset, z);

      // The range of the inoise8 function is roughly 16-238.
      // These two operations expand those values out to roughly 0..255
      // You can comment them out if you want the raw noise data.
      data = qsub8(data, 16);
      data = qadd8(data, scale8(data, 39));

      if (dataSmoothing)
      {
        uint8_t olddata = noise[i][j];
        uint8_t newdata = scale8(olddata, dataSmoothing) + scale8(data, 256 - dataSmoothing);
        data = newdata;
      }

      noise[i][j] = data;
    }
  }

  z += animationVal2;
  // apply slow drift to X and Y, just for visual variation.
  x += animationVal2 / 8;
  y -= animationVal2 / 16;
}

//
// Mark's xy coordinate mapping code.  See the XYMatrix for more information on it.
//
uint16_t XY(uint8_t x, uint8_t y)
{
  uint16_t i;
  if (kMatrixSerpentineLayout == false)
  {
    i = (y * kMatrixWidth) + x;
  }
  if (kMatrixSerpentineLayout == true)
  {
    if (y & 0x01)
    {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    }
    else
    {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
  }
  return i;
}

void map_noise_to_leds_using_palette()
{
  // static uint8_t ihue = 0;

  for (int i = 0; i < kMatrixWidth; i++)
  {
    for (int j = 0; j < kMatrixHeight; j++)
    {
      // We use the value at the (i,j) coordinate in the noise
      // array for our brightness, and the flipped value from (j,i)
      // for our pixel's index into the color palette.

      uint8_t index = noise[j][i];
      uint8_t bri = noise[i][j];

      // // if this palette is a 'loop', add a slowly-changing base value
      // if (colorLoop)
      // {
      //   index += ihue;

      // }

      // brighten up, as the color palette itself often contains the
      // light/dark dynamic range desired
      if (bri > 127)
      {
        bri = 255;
      }
      else
      {
        bri = dim8_raw(bri * 2);
      }

      CRGB color = ColorFromPalette(currentPalette, index, bri);
      leds[XY(i, j)] = color;
    }
  }
  // ihue += 1;
}

void test_cylon_speed()
{
  fadeToBlackBy(leds, NUM_LEDS, 7);
  static int i = 0;
  if (i > NUM_LEDS - 1)
  {
    i = 0;
  }
  i++;
  leds[i] += ColorFromPalette(currentPalette, i * iHue, 255, currentBlending);
};

void show_led_ota_percentage(float percentage) {
  int amount = NUM_LEDS * percentage;
  printf("Received progress %i\n", amount);
  fill_solid(leds, amount, CRGB::White);
  fastled_show_esp32();
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