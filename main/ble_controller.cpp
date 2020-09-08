#include "ble_controller.hpp"

BLEServer *pServer = NULL;
BLEAdvertising *pAdvertising = NULL;
BLEService *pService = NULL;
BLECharacteristic *animationNumberCharacteristic = NULL;
BLECharacteristic *colorPaletteCharacteristic = NULL;
BLECharacteristic *masterBrightnessCharacteristic = NULL;
BLECharacteristic *otaFileWriteCharacteristic = NULL;
BLECharacteristic *otaFileSizeCharacteristic = NULL;
BLECharacteristic *firmwareVersionCharacteristic = NULL;

BLEUUID uuid("0b6cc699-edba-4d0f-b907-8f3e07c1bf94");

static bool initialized = false;
bool device_is_connected = false;

bool updateFlag = false;
bool readyFlag = false;
int bytesReceived = 0;
int timesWritten = 0;
int file_size = 0;

esp_ota_handle_t otaHandler = 0;

const char *BLE_TAG = "BLE_CTRL";
const char *BLE_ADV_NAME = "Palette";

void abort_ota() {
  // esp_err_t ret = esp_ota_abort(otaHandler);
  // if (ret == ESP_OK) {
  ESP_LOGW(BLE_TAG, "OTA aborted");
  // } else {
  //   ESP_LOGE(BLE_TAG, "Error in abort_ota's esp_ota_end (error = %s)\n", esp_err_to_name(ret));
  // }
  resume_led_animations();
  updateFlag = false;
}

class OTAFileSizeCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxData = pCharacteristic->getValue();
    file_size = std::stoi(rxData);
    printf("File size: %i\n", file_size);
  }
};

class OTAFileWriteCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    esp_err_t ret;
    std::string rxData = pCharacteristic->getValue();

    if (!updateFlag) {
      // If it's the first packet of OTA since bootup, begin OTA
      ESP_LOGI(BLE_TAG, "Begin OTA");
      suspend_led_animations();
      const esp_partition_t* next_partition = esp_ota_get_next_update_partition(NULL);
      ret = esp_ota_begin(next_partition, OTA_SIZE_UNKNOWN, &otaHandler);
      if (ret != ESP_OK) {
        ESP_LOGE(BLE_TAG, "Error beginning OTA (error = %s)\n", esp_err_to_name(ret));
      }
      updateFlag = true;
    }

    bytesReceived += rxData.length();
    float progress = (float)bytesReceived/(float)file_size;
    
    if (rxData.length() > 0)
    {
      ret = esp_ota_write(otaHandler, rxData.c_str(), rxData.length());
      if (ret != ESP_OK) {
        ESP_LOGE(BLE_TAG, "Error in esp_ota_write (len = %i, error = %s)\n", rxData.length(), esp_err_to_name(ret));
        abort_ota();
      }

      printf("OTA progress: %-3.3f%%\n", progress * 100);
      // show_led_ota_percentage(progress);

      if (rxData.length() != FULL_PACKET)
      {
        ret = esp_ota_end(otaHandler);
        if (ret == ESP_OK) {
          ESP_LOGI(BLE_TAG, "EndOTA\n");
        } else {
          ESP_LOGE(BLE_TAG, "Error in esp_ota_end (error = %s)\n", esp_err_to_name(ret));
          updateFlag = false;
          abort_ota();
        }
        ret = esp_ota_set_boot_partition(esp_ota_get_next_update_partition(NULL));
        if (ret == ESP_OK) {
          set_application_task(OTA_FIRMWARE_UPDATE_SUCCESS);
          esp_restart();
        } else {
          ESP_LOGE(BLE_TAG, "Upload Error (error = %s)\n", esp_err_to_name(ret));
          abort_ota();
        }
      }
    }

    uint8_t txData[2] = {'o', 'k'};
    pCharacteristic->setValue((uint8_t*)txData, 2);
    pCharacteristic->notify();
  }
};

class ServerCallbacks : public BLEServerCallbacks
{
  void onConnect(BLEServer *pServer)
  {
    device_is_connected = true;
    set_application_mode_characteristic(get_current_application_task());
    ESP_LOGI(BLE_TAG, "Client connected!");
  };

  void onDisconnect(BLEServer *pServer)
  {
    ESP_LOGW(BLE_TAG, "Client disconnected");
    device_is_connected = false;
    abort_ota();
    start_advertising();
  }
};

class AnimationCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();
    int animation_number = std::stoi(rxValue);
    set_animation_number(animation_number);
  }
};

class FirmwareVersionCallbacks : public BLECharacteristicCallbacks
{
  void onRead(BLECharacteristic *pCharacteristic)
  {
    ESP_LOGI(BLE_TAG, "Project version requested");
  }
};

class AnimationSettingsCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();
    int length = rxValue.length();
    int halfLength = length * 0.5;
    int x[halfLength];
    int c = 0;
    for (int i = 0; i < length; i += 2)
    {
      char z[2] = {rxValue[i], rxValue[i + 1]};
      x[c] = (int)strtol(z, 0, 16);
      c++;
    }
    set_animation_settings(x);
  }
};

class ColorPaletteCallbacks : public BLECharacteristicCallbacks
{
  void onWrite(BLECharacteristic *pCharacteristic)
  {
    std::string rxValue = pCharacteristic->getValue();
    int length = rxValue.length();
    int halfLength = length * 0.5;
    uint8_t x[halfLength];
    int c = 0;
    for (int i = 0; i < length; i += 2)
    {
      char z[2] = {rxValue[i], rxValue[i + 1]};
      x[c] = strtol(z, 0, 16);
      c++;
    }
    add_palette(x, halfLength);
  }
};

void set_application_mode_characteristic(int task) {
  // char mode[APPLICATION_MODE_CHAR_NOTIFY_PAYLOAD_LENGTH];
  // if (initialized && device_is_connected) {
  //   applicationModeCharacteristic->setValue(itoa(task, mode, APPLICATION_MODE_CHAR_NOTIFY_PAYLOAD_LENGTH));
  //   applicationModeCharacteristic->notify();
  //   // ESP_LOGI(BLE_TAG, "Notifying application mode: %s", itoa(task, mode, APPLICATION_MODE_CHAR_NOTIFY_PAYLOAD_LENGTH));
  // } else {
  //   ESP_LOGW(BLE_TAG, "Unable to notify application mode: %s (initialized = %i, device_is_connected = %i)", itoa(task, mode, APPLICATION_MODE_CHAR_NOTIFY_PAYLOAD_LENGTH), initialized, device_is_connected);
  // }
}

bool ble_is_initialized() {
  return initialized;
}

void start_advertising() {
  const esp_app_desc_t *app_description = esp_ota_get_app_description();

  BLEAdvertisementData advertisementData;

  char version[6] = {
    app_description->version[0],
    app_description->version[1],
    app_description->version[2],
    app_description->version[3],
    app_description->version[4],
  };

  std::string strServiceData = "";
  strServiceData += version;

  pAdvertising = BLEDevice::getAdvertising();
  advertisementData.setPartialServices(uuid);
  advertisementData.setManufacturerData(strServiceData);
  pAdvertising->setAdvertisementData(advertisementData);
  pAdvertising->setScanResponse(true);
  pAdvertising->start();
}

void init_ble_controller(void)
{
if (initialized)
  {
    ESP_LOGW(BLE_TAG, "BLE controller has already been initialized. Skipping %s", __func__);
    return;
  }

  ESP_LOGI(BLE_TAG, "Initializing BLE controller");

  const esp_app_desc_t *app_description = esp_ota_get_app_description();
  
  BLEDevice::init(BLE_ADV_NAME);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());

  pService = pServer->createService(uuid);

  otaFileWriteCharacteristic = pService->createCharacteristic(
      OTA_FILE_WRITE_CHARACTERISTIC,
      BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE
  );

  otaFileSizeCharacteristic = pService->createCharacteristic(
      OTA_FILE_SIZE_CHARACTERISTIC,
      BLECharacteristic::PROPERTY_WRITE_NR
  );

  animationNumberCharacteristic = pService->createCharacteristic(
      ANIMATION_NUMBER_CHARACTERISTIC,
      BLECharacteristic::PROPERTY_WRITE_NR);

  colorPaletteCharacteristic = pService->createCharacteristic(
      COLOR_PALETTE_CHARACTERISTIC,
      BLECharacteristic::PROPERTY_WRITE_NR);

  masterBrightnessCharacteristic = pService->createCharacteristic(
      MASTER_BRIGHTNESS_CHARACTERISTIC,
      BLECharacteristic::PROPERTY_WRITE_NR);

  firmwareVersionCharacteristic = pService->createCharacteristic(
      FIRMWARE_VERSION_CHARACTERISTIC,
      BLECharacteristic::PROPERTY_READ);

  animationNumberCharacteristic->setCallbacks(new AnimationCallbacks());
  colorPaletteCharacteristic->setCallbacks(new ColorPaletteCallbacks());
  masterBrightnessCharacteristic->setCallbacks(new AnimationSettingsCallbacks());
  firmwareVersionCharacteristic->setCallbacks(new FirmwareVersionCallbacks());
  otaFileWriteCharacteristic->setCallbacks(new OTAFileWriteCallbacks());
  otaFileSizeCharacteristic->setCallbacks(new OTAFileSizeCallbacks());

  otaFileWriteCharacteristic->addDescriptor(new BLE2902());

  firmwareVersionCharacteristic->setValue(app_description->version);

  pService->start();

  start_advertising();

  initialized = true;
  ESP_LOGI(BLE_TAG, "BLE controller initialized");
}

void deinit_ble_controller()
{
  if (!initialized) {
    ESP_LOGW(BLE_TAG, "BLE controller not yet initialized - skipping %s", __func__);
    return;
  }
  BLEDevice::deinit();
  initialized = false;
}