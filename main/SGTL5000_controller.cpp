#include "SGTL5000_controller.hpp"

const char *SGTL5000_TAG = "SGTL5000";
static bool initialized = false;

void init_sgtl5000_controller()
{
  if (initialized)
  {
    ESP_LOGW(SGTL5000_TAG, "SGTL5000 controller has already been initialized. Skipping %s", __func__);
    return;
  }
  ESP_LOGI(SGTL5000_TAG, "Initializing SGTL5000 controller");
  vTaskDelay(250 / portTICK_PERIOD_MS); // Wait for complete boot-up
  audiobit_i2s_init();
  vTaskDelay(250 / portTICK_PERIOD_MS); // Wait for complete boot-up
  audiobit_i2c_init();                  // Init interface to AudioBit

  if (audiobit_playback_init() == ESP_OK)
  {
    initialized = true;
    ESP_LOGI(SGTL5000_TAG, "SGTL5000 I2S and I2C initialization complete");
  }
  else
  {
    ESP_LOGE(SGTL5000_TAG, "Error initializing SGTL5000 I2S and I2C");
    return;
  }
}

void deinit_sgtl5000_controller()
{
  if (!initialized) {
    ESP_LOGW(SGTL5000_TAG, "SGTL5000 controller not yet initialized - skipping %s", __func__);
    return;
  }
  
  if (i2s_driver_uninstall(I2S_NUM) != ESP_OK)
  {
    ESP_LOGE(SGTL5000_TAG, "Error uninstalling i2s driver");
  }
  if (i2c_driver_delete(AUDIOBIT_I2C_NUM) != ESP_OK)
  {
    ESP_LOGE(SGTL5000_TAG, "Error uninstalling i2s driver");
  }
  initialized = false;
  ESP_LOGW(SGTL5000_TAG, "SGTL5000 controller deinitialized");
}