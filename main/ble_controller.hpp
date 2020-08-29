#ifndef __BLE_CONTROLLER_H__
#define __BLE_CONTROLLER_H__

#include "led_controller.hpp"
#include "application_task_controller.hpp"

#include <BLEDevice.h>
#include <BLEAdvertising.h>
#include <BLEAdvertisedDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "esp_ota_ops.h"

#define SERVICE_UUID "0b6cc699-edba-4d0f-b907-8f3e07c1bf94"

#define COLOR_PALETTE_CHARACTERISTIC "8192ff33-3cf1-4ff0-be12-3e84d0be5877"
#define ANIMATION_NUMBER_CHARACTERISTIC "dc71b719-d5bb-4f4f-a750-210c3664333e"
#define MASTER_BRIGHTNESS_CHARACTERISTIC "18f8c1a3-7197-4a7f-a497-58fc3d88e904"
#define PROJECT_VERSION_CHARACTERISTIC "9c479ab0-ae12-4c5e-ac37-62c53a7f07e4"
#define APPLICATION_MODE_CHARACTERISTIC "4ca942b4-0273-4dc1-84ea-e8aa807a29a5"
#define OTA_FILE_WRITE_CHARACTERISTIC "73271ca9-8fd9-49f2-9dcd-66f17f96ae9c"

#define APPLICATION_MODE_CHAR_NOTIFY_PAYLOAD_LENGTH 10
#define FULL_PACKET 288

void set_application_mode_characteristic(int current_task);

void init_ble_controller(void);

void start_advertising(void);

void deinit_ble_controller(void);

bool ble_is_initialized(void);

bool ota_update_is_ap_mode(void);

#endif /* __BLE_CONTROLLER_H__*/
