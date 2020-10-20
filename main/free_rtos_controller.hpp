#ifndef __FREE_RTOS_CONTROLLER_H__
#define __FREE_RTOS_CONTROLLER_H__

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_controller.hpp"

/**
 * All delays are in ms
*/

// RTOS controller tasks
extern TaskHandle_t PrintRTOSStatusHandler;
#define PRINT_RTOS_TASK_STACK_SIZE (2048)
#define PRINT_RTOS_TASK_PRIORITY (1)
#define PRINT_RTOS_STATUS_TASK_DELAY (5000)
#define PRINT_RTOS_STATUS_CORE_ID tskNO_AFFINITY

// FastLED controller tasks
#define FASTLED_DEFAULT_CORE_ID tskNO_AFFINITY

extern TaskHandle_t FastLEDshowTaskHandle;
#define FASTLED_SHOW_TASK_STACK_SIZE (3072)
#define FASTLED_SHOW_TASK_PRIORITY (configMAX_PRIORITIES - 7)
#define FASTLED_SHOW_TASK_CORE_ID FASTLED_DEFAULT_CORE_ID

extern TaskHandle_t RunAnimationTaskHandle;
#define RUN_ANIMATION_TASK_STACK_SIZE (3072)
#define RUN_ANIMATION_TASK_PRIORITY (10)
#define RUN_ANIMATION_TASK_DELAY (5)
#define RUN_ANIMATION_TASK_CORE_ID FASTLED_DEFAULT_CORE_ID

extern TaskHandle_t RunLEDPaletteTaskHandle;
#define RUN_LED_PALETTE_TASK_STACK_SIZE (1024)
#define RUN_LED_PALETTE_TASK_PRIORITY (5)
#define RUN_LED_PALETTE_TASK_DELAY (0)
#define RUN_LED_PALETTE_TASK_CORE_ID FASTLED_DEFAULT_CORE_ID

// Audio analysis controller tasks
extern TaskHandle_t RunAudioAnalysisTaskHandle;
#define RUN_AUDIO_ANALYSIS_TASK_CORE_ID tskNO_AFFINITY
#define RUN_AUDIO_ANALYSIS_TASK_PRIORITY (configMAX_PRIORITIES - 3)
#ifdef DEBUG_AUDIO_ANALYSIS
#define RUN_AUDIO_ANALYSIS_TASK_STACK_SIZE (4096)
#define RUN_AUDIO_ANALYSIS_TASK_DELAY (100)
#else
#define RUN_AUDIO_ANALYSIS_TASK_DELAY (20)
#define RUN_AUDIO_ANALYSIS_TASK_STACK_SIZE (1024)
#endif

// BT audio controller tasks
extern TaskHandle_t BTi2sTaskHandler;
#define BT_I2S_TASK_STACK_SIZE (1024)
#define BT_I2S_TASK_PRIORITY (configMAX_PRIORITIES - 3)
#define BT_I2S_TASK_CORE_ID 0

// BLE controller tasks
extern TaskHandle_t BTAppTaskHandler;
#define BT_APP_TASK_STACK_SIZE (5120)
#define BT_APP_TASK_PRIORITY (configMAX_PRIORITIES - 3)
#define BT_APP_TASK_CORE_ID 0

UBaseType_t GetTaskHighWaterMark(TaskHandle_t task_handle);

float GetTaskHighWaterMarkPercent(TaskHandle_t task_handle, uint32_t stack_allotment);

void print_free_rtos_status(void *pvParameters);

void init_free_rtos_controller();

void deinit_free_rtos_controller();

#endif /* __FREE_RTOS_CONTROLLER_H__*/