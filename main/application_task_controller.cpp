#include "application_task_controller.hpp"

const char *APPLICATION_TASK_TAG = "APP_TASK_SWITCHER";
static bool initialized = false;

application_task mode;

const char *kModeStrings[] = {
    [OTA_FIRMWARE_UPDATE_INIT] = "OTA_FIRMWARE_UPDATE_INIT",
    [OTA_FIRMWARE_UPDATE_INIT_WIFI] = "OTA_FIRMWARE_UPDATE_INIT_WIFI",
    [OTA_FIRMWARE_UPDATE_WIFI_STA_ENABLED] = "OTA_FIRMWARE_UPDATE_WIFI_STA_ENABLED",
    [OTA_FIRMWARE_UPDATE_WIFI_AP_ENABLED] = "OTA_FIRMWARE_UPDATE_WIFI_AP_ENABLED",
    [OTA_FIRMWARE_UPDATE_WIFI_ERROR] = "OTA_FIRMWARE_UPDATE_WIFI_ERROR",
    [OTA_FIRMWARE_UPDATE_WIFI_CONNECTED] = "OTA_FIRMWARE_UPDATE_WIFI_CONNECTED",
    [OTA_FIRMWARE_UPDATE_ACTIVE] = "OTA_FIRMWARE_UPDATE_ACTIVE",
    [OTA_FIRMWARE_UPDATE_SUCCESS] = "OTA_FIRMWARE_UPDATE_SUCCESS",
    [OTA_FIRMWARE_UPDATE_ERROR] = "OTA_FIRMWARE_UPDATE_ERROR",
    [OTA_FIRMWARE_UPDATE_DEINIT] = "OTA_FIRMWARE_UPDATE_DEINIT",
    [MAIN_APPLICATION_INIT] = "MAIN_APPLICATION_INIT",
    [MAIN_APPLICATION_ACTIVE] = "MAIN_APPLICATION_ACTIVE",
    [MAIN_APPLICATION_DEINIT] = "MAIN_APPLICATION_DEINIT",
    [SHUT_DOWN] = "SHUT_DOWN",
};

application_task set_application_task(application_task new_mode)
{
  ESP_LOGI(APPLICATION_TASK_TAG, "Switched from %s to %s", kModeStrings[mode], kModeStrings[new_mode]);
  mode = new_mode;
  set_application_mode_characteristic(new_mode);
  return mode;
}

application_task enter_ota_update_mode()
{
  if (mode == MAIN_APPLICATION_ACTIVE) {
    return set_application_task(MAIN_APPLICATION_DEINIT);
  } else {
    set_application_mode_characteristic(mode);
    return mode;
  }
}

application_task exit_ota_update_mode()
{
  return set_application_task(OTA_FIRMWARE_UPDATE_DEINIT);
}

application_task run_app_mode_switcher_task()
{
  switch (mode)
  {
  case MAIN_APPLICATION_INIT:
  {
  #ifdef ENABLE_PERFORMANCE_MONITOR
    init_free_rtos_controller();
  #endif
    // init_status_led_controller();
    init_eeprom_controller();
    init_iir_controller(); // should be called before init_led_controller
    init_aux_detect();
    init_sgtl5000_controller();
    init_a2dp_controller();
    init_ble_controller();
    init_led_controller();
    return set_application_task(MAIN_APPLICATION_ACTIVE);
  }
  case MAIN_APPLICATION_ACTIVE:
  {
    // run_status_led_task();
    // return set_application_task(MAIN_APPLICATION_DEINIT);
    return mode;
  }
  case MAIN_APPLICATION_DEINIT:
  {
    deinit_sgtl5000_controller();
    deinit_eeprom_controller();
    deinit_led_controller();
    deinit_iir_controller(); // should be called after deinit_led_controller
    deinit_a2dp_controller();
    deinit_aux_detect();
    return set_application_task(OTA_FIRMWARE_UPDATE_INIT);
  }
  case OTA_FIRMWARE_UPDATE_INIT:
  {
    return mode;
  }
  case OTA_FIRMWARE_UPDATE_INIT_WIFI:
  {
    // Here we are just waiting for a successful WiFi init message from WiFiEvent
    return mode;
  }
  case OTA_FIRMWARE_UPDATE_WIFI_STA_ENABLED:
  {
    return mode;
  }
  case OTA_FIRMWARE_UPDATE_WIFI_AP_ENABLED:
  {
    return mode;
  }
  case OTA_FIRMWARE_UPDATE_WIFI_ERROR:
  {
    return set_application_task(OTA_FIRMWARE_UPDATE_DEINIT);
  }
  case OTA_FIRMWARE_UPDATE_WIFI_CONNECTED:
  {
    return set_application_task(OTA_FIRMWARE_UPDATE_ACTIVE);
  }
  case OTA_FIRMWARE_UPDATE_ACTIVE:
  {
    return mode;
  }
  case OTA_FIRMWARE_UPDATE_SUCCESS:
  {
    return set_application_task(OTA_FIRMWARE_UPDATE_DEINIT);
  }
  case OTA_FIRMWARE_UPDATE_ERROR:
  {
    return set_application_task(OTA_FIRMWARE_UPDATE_DEINIT);
  }
  case OTA_FIRMWARE_UPDATE_DEINIT:
  {
    return set_application_task(MAIN_APPLICATION_INIT);
  }
  case SHUT_DOWN:
  {
  #ifdef ENABLE_PERFORMANCE_MONITOR
    deinit_free_rtos_controller();
  #endif
    deinit_sgtl5000_controller();
    deinit_eeprom_controller();
    deinit_led_controller();
    deinit_iir_controller(); // should be called after deinit_led_controller
    deinit_a2dp_controller();
    deinit_aux_detect();
    ESP_LOGW(APPLICATION_TASK_TAG, "Shutting down...");
    return mode;
  }
  default:
  {
    ESP_LOGE(APPLICATION_TASK_TAG, "Unknown mode %s", kModeStrings[mode]);
    return mode;
  }
  }
}


application_task init_application_task_controller() {
  if (initialized)
  {
    ESP_LOGW(APPLICATION_TASK_TAG, "Application task controller has already been initialized. Skipping %s", __func__);
    return mode;
  }
  ESP_LOGI(APPLICATION_TASK_TAG, "Initializing application task controller");

  // here we could use EEPROM to retrieve
  // a previously set application startup mode
  // mode = OTA_FIRMWARE_UPDATE_INIT;
  mode = MAIN_APPLICATION_INIT;

  ESP_LOGI(APPLICATION_TASK_TAG, "Startup mode set as %s", kModeStrings[mode]);
  initialized = true;
  return mode;
}

application_task get_current_application_task() {
  return mode;
}

void deinit_application_task_controller() {
  if (!initialized) {
    ESP_LOGW(APPLICATION_TASK_TAG, "Application task controller not yet initialized - skipping %s", __func__);
    return;
  }
  initialized = false;
  ESP_LOGI(APPLICATION_TASK_TAG, "Application task controller deinitialized");
}