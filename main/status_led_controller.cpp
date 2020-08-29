#include "status_led_controller.hpp"
#include <Arduino.h>

const char *STATUS_LED_TAG = "STATUS_LED";
static bool initialized = false;

TaskHandle_t ShowStatusLedEffectHandle = 0;
TaskHandle_t FastLEDshowStatusTaskHandle = 0;
TaskHandle_t statusTaskHandle = 0;

CRGB status_leds[NUM_STATUS_LEDS];

status_led_effect current_effect = STATUS_LED_BREATHING;

/** show() for ESP32
    Call this function instead of FastLED.show(). It signals core 0 to issue a show,
    then waits for a notification that it is done.
*/
void fastled_show_status_esp32()
{
  if (statusTaskHandle == 0)
  {
    // -- Store the handle of the current task, so that the show task can
    //    notify it when it's done
    statusTaskHandle = xTaskGetCurrentTaskHandle();

    // -- Trigger the show task
    xTaskNotifyGive(FastLEDshowStatusTaskHandle);

    // -- Wait to be notified that it's done
    const TickType_t xMaxBlockTime = pdMS_TO_TICKS(200);
    ulTaskNotifyTake(pdTRUE, xMaxBlockTime);
    statusTaskHandle = 0;
  }
}

/** show Task
    This function runs on core 0 and just waits for requests to call FastLED.show()
*/
void fastled_show_status_task(void *pvParameters)
{
  // -- Run forever...
  for (;;)
  {
    // -- Wait for the trigger
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

    // -- Do the show (synchronously)
    FastLED.show();
    // -- Notify the calling task
    xTaskNotifyGive(statusTaskHandle);
  }
  vTaskDelete(NULL);
}

void show_status_led_effect(void *pvParameters) {
  for (;;) {
    switch (current_effect) {
      case STATUS_LED_BREATHING:
        show_breathing_effect();
        fastled_show_status_esp32();
        break;
      default:
        fill_solid(status_leds, NUM_STATUS_LEDS, CRGB::Black);
        fastled_show_status_esp32();
        break;
    }
    vTaskDelay(20 / portTICK_PERIOD_MS);
  }
  vTaskDelete(NULL);
}

void show_breathing_effect() {
  // https://sean.voisen.org/blog/2011/10/breathing-led-with-arduino/
  float val = (exp(sin(millis()/2000.0*PI)) - 0.36787944)*108.0;
  fill_solid(status_leds, NUM_STATUS_LEDS, CHSV(0, 255, val));
  FastLED.show();
}

void init_status_led_controller()
{
  if (initialized)
  {
    ESP_LOGW(STATUS_LED_TAG, "Status LED controller has already been initialized. Skipping %s", __func__);
    return;
  }
  ESP_LOGI(STATUS_LED_TAG, "Initializing status LED controller");

  FastLED.addLeds<APA102, STATUS_LED_DI, STATUS_LED_CI, RGB>(status_leds, 1).setCorrection(TypicalLEDStrip);

  fill_solid(status_leds, 1, CHSV(0, 255, 100));

  // gpio_config_t io_conf;
  // io_conf.mode = GPIO_MODE_OUTPUT;
  // io_conf.pin_bit_mask = ((1ULL<<STATUS_LED_R) | (1ULL<<STATUS_LED_G) | (1ULL<<STATUS_LED_B));
  // io_conf.intr_type = GPIO_INTR_DISABLE;
  // io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  // io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  // gpio_config(&io_conf);

  // gpio_set_level(STATUS_LED_R, 0);
  // gpio_set_level(STATUS_LED_G, 0);
  // gpio_set_level(STATUS_LED_B, 0);

  // pinMode(STATUS_LED_R, OUTPUT);
  // pinMode(STATUS_LED_G, OUTPUT);
  // pinMode(STATUS_LED_B, OUTPUT);
  // digitalWrite(STATUS_LED_R, LOW);
  // digitalWrite(STATUS_LED_G, HIGH);
  // digitalWrite(STATUS_LED_B, HIGH);

  xTaskCreatePinnedToCore(fastled_show_status_task, "FastLEDshowStatusTask", 1024, NULL, 3, &FastLEDshowStatusTaskHandle, tskNO_AFFINITY);
  xTaskCreatePinnedToCore(show_status_led_effect, "ShowStatusLedEffect", 2048, NULL, 3, &ShowStatusLedEffectHandle, tskNO_AFFINITY);

  fastled_show_status_esp32();

  initialized = true;
  ESP_LOGI(STATUS_LED_TAG, "Initialized status LED controller");
}

void run_status_led_task()
{
  show_breathing_effect();
  // show_status_led_effect(current_effect);
}

void deinit_status_led_controller()
{
  if (!initialized) {
    ESP_LOGW(STATUS_LED_TAG, "Status LED controller not yet initialized - skipping %s", __func__);
    return;
  }
  vTaskDelete(ShowStatusLedEffectHandle);
  initialized = false;
  ESP_LOGW(STATUS_LED_TAG, "Status LED controller deinitialized");
}