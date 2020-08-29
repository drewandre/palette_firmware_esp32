#ifndef __APPLICATION_TASK_CONTROLLER_H__
#define __APPLICATION_TASK_CONTROLLER_H__

#include "led_controller.hpp"
#include "status_led_controller.hpp"
#include "SGTL5000_controller.hpp"
#include "ble_controller.hpp"
#include "audio_analysis_controller.hpp"
#include "a2dp_controller.hpp"
#include "eeprom_controller.hpp"
#include "aux_detect.hpp"
#include "free_rtos_controller.hpp"

// #define ENABLE_PERFORMANCE_MONITOR
typedef enum
{
  OTA_FIRMWARE_UPDATE_INIT,
  OTA_FIRMWARE_UPDATE_INIT_WIFI,
  OTA_FIRMWARE_UPDATE_WIFI_STA_ENABLED,
  OTA_FIRMWARE_UPDATE_WIFI_AP_ENABLED,
  OTA_FIRMWARE_UPDATE_WIFI_ERROR,
  OTA_FIRMWARE_UPDATE_WIFI_CONNECTED,
  OTA_FIRMWARE_UPDATE_ACTIVE,
  OTA_FIRMWARE_UPDATE_SUCCESS,
  OTA_FIRMWARE_UPDATE_ERROR,
  OTA_FIRMWARE_UPDATE_DEINIT,
  MAIN_APPLICATION_INIT,
  MAIN_APPLICATION_ACTIVE,
  MAIN_APPLICATION_DEINIT,
  SHUT_DOWN,
} application_task;

application_task run_app_mode_switcher_task();

application_task set_application_task(application_task newMode);

application_task enter_ota_update_mode();

application_task exit_ota_update_mode();

application_task get_current_application_task();

application_task init_application_task_controller();

void deinit_application_task_controller();

#endif /* __APPLICATION_TASK_CONTROLLER_H__*/