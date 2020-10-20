#include "free_rtos_controller.hpp"

const char *FREE_RTOS_TAG = "FREE_RTOS";
static bool initialized = false;

TaskHandle_t PrintRTOSStatusHandler = 0;

UBaseType_t GetTaskHighWaterMark(TaskHandle_t task_handle)
{
  UBaseType_t uxHighWaterMark;
  uxHighWaterMark = uxTaskGetStackHighWaterMark(task_handle);
  return uxHighWaterMark;
}

/* -----------------------------------------------------------------------------
  GetTaskHighWaterMarkPercent(TaskHandle_t task_handle, uint32_t stack_allotment)
  https://esp32.com/viewtopic.php?t=9735

 	Input Params:
    - task_handle: The task name you want to examine
   	- stack_allotment:  How much stack space did you allocate to it when you created it

  Returns: float with the % of stack used
  Example:   printf("Stack Used %04.1f%%\r\n", GetTaskHighWaterMarkPercent(xTask1, 2048) );
  Notes:
 -----------------------------------------------------------------------------*/
float GetTaskHighWaterMarkPercent(TaskHandle_t task_handle, uint32_t stack_allotment)
{
  UBaseType_t uxHighWaterMark;
  uint32_t diff;
  float result;
  uxHighWaterMark = uxTaskGetStackHighWaterMark(task_handle);
  diff = stack_allotment - uxHighWaterMark;
  result = ((float)diff / (float)stack_allotment) * 100.0;
  return result;
}

void print_free_rtos_status(void *pvParameters) {
  for (;;) {
    vTaskDelay(PRINT_RTOS_STATUS_TASK_DELAY / portTICK_PERIOD_MS);
    printf("---------------------------------------------------------------------\n");
    ESP_LOGI(FREE_RTOS_TAG, "PrintRTOSStatusHandler:\t\t%04.1f%%", GetTaskHighWaterMarkPercent(PrintRTOSStatusHandler, PRINT_RTOS_TASK_STACK_SIZE));
    ESP_LOGI(FREE_RTOS_TAG, "FastLEDshowTaskHandle:\t\t%04.1f%%", GetTaskHighWaterMarkPercent(FastLEDshowTaskHandle, FASTLED_SHOW_TASK_STACK_SIZE));
    ESP_LOGI(FREE_RTOS_TAG, "RunAnimationTaskHandle:\t\t%04.1f%%", GetTaskHighWaterMarkPercent(RunAnimationTaskHandle, RUN_ANIMATION_TASK_STACK_SIZE));
    ESP_LOGI(FREE_RTOS_TAG, "RunLEDPaletteTaskHandle:\t\t%04.1f%%", GetTaskHighWaterMarkPercent(RunLEDPaletteTaskHandle, RUN_LED_PALETTE_TASK_STACK_SIZE));
    ESP_LOGI(FREE_RTOS_TAG, "RunAudioAnalysisTaskHandle:\t\t%04.1f%%", GetTaskHighWaterMarkPercent(RunAudioAnalysisTaskHandle, RUN_AUDIO_ANALYSIS_TASK_STACK_SIZE));
    ESP_LOGI(FREE_RTOS_TAG, "BTi2sTaskHandler:\t\t\t%04.1f%%", GetTaskHighWaterMarkPercent(BTi2sTaskHandler, BT_I2S_TASK_STACK_SIZE));
    ESP_LOGI(FREE_RTOS_TAG, "BTAppTaskHandler:\t\t\t%04.1f%%", GetTaskHighWaterMarkPercent(BTAppTaskHandler, BT_APP_TASK_STACK_SIZE));
    ESP_LOGI(FREE_RTOS_TAG, "FastLED: %u leds running at %ufps on %u controller(s)", NUM_LEDS, FastLED.getFPS(), FastLED.count());
    printf("---------------------------------------------------------------------\n");
  }
  vTaskDelete(NULL);
}

void init_free_rtos_controller() {
  if (initialized)
  {
    ESP_LOGW(FREE_RTOS_TAG, "Free RTOS controller has already been initialized. Skipping %s", __func__);
    return;
  }
  ESP_LOGI(FREE_RTOS_TAG, "Initializing Free RTOS controller");

  xTaskCreatePinnedToCore(print_free_rtos_status, "print_free_rtos_status", PRINT_RTOS_TASK_STACK_SIZE, NULL, PRINT_RTOS_TASK_PRIORITY, &PrintRTOSStatusHandler, PRINT_RTOS_STATUS_CORE_ID);
  initialized = true;
  ESP_LOGI(FREE_RTOS_TAG, "Free RTOS controller initialized");
}

void deinit_free_rtos_controller() {
  if (!initialized) {
    ESP_LOGW(FREE_RTOS_TAG, "Free RTOS controller not yet initialized - skipping %s", __func__);
    return;
  }
  vTaskDelete(PrintRTOSStatusHandler);
  initialized = false;
  ESP_LOGW(FREE_RTOS_TAG, "Free RTOS controller deinitialized");
}