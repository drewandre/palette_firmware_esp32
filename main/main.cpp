/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/****************************************************************************
* The demo shows BLE and classic Bluetooth coexistence. You can use BLE GATT server and classic bluetooth A2DP together.
* The BLE GATT server part of the demo creates a GATT service and then starts advertising, waiting to be connected by a GATT client.
* After the program is started, a GATT client can discover the device named "ESP_COEX_BLE_DEMO". Once the connection is established,
* GATT client can read or write data to the device. It can also receive notification or indication data.
* Attention: If you test the demo with iPhone, BLE GATT server adv name will change to "ESP_COEX_A2DP_DEMO" after you connect it.
* The classic bluetooth A2DP part of the demo implements Advanced Audio Distribution Profile to receive an audio stream.
* After the program is started, other bluetooth devices such as smart phones can discover the device named "ESP_COEX_A2DP_DEMO".
* Once the connection is established, audio data can be transmitted. This will be visible in the application log including a count
* of audio data packets.
****************************************************************************/

#include "Arduino.h"
#include "FastLED.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_system.h"
#include "esp_log.h"

#include "esp_bt.h"
#include "bt_app_core.h"
#include "bt_app_av.h"
#include "esp_bt_main.h"
#include "esp_bt_device.h"
#include "esp_gap_bt_api.h"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

#include "audio_element.h"
#include "audio_event_iface.h"
#include "audio_common.h"
#include "board.h"
#include "esp_dsp.h"
#include "i2s_stream.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_gatt_common_api.h"

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_ota_ops.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"

#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#endif

#include "fft_controller.h"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

extern const uint8_t server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
extern const uint8_t server_cert_pem_end[] asm("_binary_ca_cert_pem_end");

#define OTA_URL_SIZE 256

#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN
static esp_err_t example_configure_stdin_stdout(void)
{
  // Initialize VFS & UART so we can use std::cout/cin
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);
  /* Install UART driver for interrupt-driven reads and writes */
  ESP_ERROR_CHECK(uart_driver_install((uart_port_t)CONFIG_CONSOLE_UART_NUM,
                                      256, 0, 0, NULL, 0));
  /* Tell VFS to use UART driver */
  esp_vfs_dev_uart_use_driver(CONFIG_CONSOLE_UART_NUM);
  esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_CR);
  /* Move the caret to the beginning of the next line on '\n' */
  esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);
  return ESP_OK;
}
#endif

CRGBPalette16 currentPalette = HeatColors_p;
TBlendType currentBlending;

extern CRGBPalette16 myRedWhiteBluePalette;
extern const TProgmemPalette16 IRAM_ATTR myRedWhiteBluePalette_p;

/*
Final dipswitch configuration
1: OFF
2: OFF
3: ON (for GPIO_NUM_15 access)
4: ON (for GPIO_NUM_13 access)
5: ON (for GPIO_NUM_12 access)
6: ON (for GPIO_NUM_14 access)
7: ON (for aux input detection)
8: OFF

GPIO_NUM_0  - n/a - Automatic Upload, I2S MCLK (wouldn't be able to use automatic reset, second furthest pin from usb)
GPIO_NUM_2  - n/a - Automatic Upload, MicroSD D0 (wouldn't be able to use automatic reset, fourth furthest pin from usb)
RX          - n/a - UART0 RX (wouldn't be able to use any HOST -> ESP32 UART control)
TX          - n/a - UART0 TX (can't use USB debugging though...)
GPIO_NUM_12 - 5   - JTAG MTDI, MicroSD D2, Aux signal detect
GPIO_NUM_13 - 4   - JTAG MTCK, MicroSD D3, Audio Vol- (TP)
GPIO_NUM_14 - 6   - JTAG MTMS, MicroSD CLK
GPIO_NUM_15 - 3   - JTAG MTDO, MicroSD CMD

Other useful pins:
GPIO_NUM_22 - green led
GPIO_NUM_19 - headphone insert detection
GPIO_NUM_12 - aux insert detection
GPIO_NUM_21 - PA enable output
*/

#define NUM_LEDS 800
#define DATA_PIN GPIO_NUM_12
CRGB leds[NUM_LEDS];

#define OTA_TAG "OTA"
#define ESP_DSP_TAG "DSP"
#define LED_TAG "FASTLED"
#define AUDIO_CODEC_TAG "CODEC"
#define BT_BLE_COEX_TAG "BT_BLE_COEX"

#define BT_DEVICE_NAME "Palette BLE"
#define BLE_ADV_NAME "Palette BLE"

#define GATTS_SERVICE_UUID_A 0x00FF
#define GATTS_CHAR_UUID_A 0xFF01
#define GATTS_DESCR_UUID_A 0x3333
#define GATTS_NUM_HANDLE_A 4

#define GATTS_SERVICE_UUID_B 0x00EE
#define GATTS_CHAR_UUID_B 0xEE01
#define GATTS_DESCR_UUID_B 0x2222
#define GATTS_NUM_HANDLE_B 4

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 0x40
#define PREPARE_BUF_MAX_SIZE 1024
#define PROFILE_NUM 2
#define PROFILE_A_APP_ID 0
#define PROFILE_B_APP_ID 1

typedef struct
{
  uint8_t *prepare_buf;
  int prepare_len;
} prepare_type_env_t;

static prepare_type_env_t a_prepare_write_env;
static prepare_type_env_t b_prepare_write_env;

// Declare the static function
static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
static void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

/* event for handler "bt_av_hdl_stack_up */
enum
{
  BT_APP_EVT_STACK_UP = 0,
};
static uint8_t ble_char_value_str[] = {0x11, 0x22, 0x33};
esp_gatt_char_prop_t a_property = 0;
esp_gatt_char_prop_t b_property = 0;

esp_attr_value_t gatts_initial_char_val = {
    .attr_max_len = GATTS_DEMO_CHAR_VAL_LEN_MAX,
    .attr_len = sizeof(ble_char_value_str),
    .attr_value = ble_char_value_str,
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x060,
    .adv_int_max = 0x060,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_RANDOM,
    .peer_addr = {0, 1, 2, 3, 4, 5},
    .peer_addr_type = BLE_ADDR_TYPE_RANDOM,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst
{
  esp_gatts_cb_t gatts_cb;
  uint16_t gatts_if;
  uint16_t app_id;
  uint16_t conn_id;
  uint16_t service_handle;
  esp_gatt_srvc_id_t service_id;
  uint16_t char_handle;
  esp_bt_uuid_t char_uuid;
  esp_gatt_perm_t perm;
  esp_gatt_char_prop_t property;
  uint16_t descr_handle;
  esp_bt_uuid_t descr_uuid;
};

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst gl_profile_tab[PROFILE_NUM] = {
    [PROFILE_A_APP_ID] = {
        .gatts_cb = gatts_profile_a_event_handler,
        .gatts_if = ESP_GATT_IF_NONE, /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
    [PROFILE_B_APP_ID] = {
        .gatts_cb = gatts_profile_b_event_handler, /* This demo does not implement, similar as profile A */
        .gatts_if = ESP_GATT_IF_NONE,              /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};
static void ble_init_adv_data(const char *name)
{
  int len = strlen(name);
  uint8_t raw_adv_data[len + 5];
  // flag
  raw_adv_data[0] = 2;
  raw_adv_data[1] = ESP_BT_EIR_TYPE_FLAGS;
  raw_adv_data[2] = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT);
  // adv name
  raw_adv_data[3] = len + 1;
  raw_adv_data[4] = ESP_BLE_AD_TYPE_NAME_CMPL;
  for (int i = 0; i < len; i++)
  {
    raw_adv_data[i + 5] = *(name++);
  }
  // The length of adv data must be less than 31 bytes
  esp_err_t raw_adv_ret = esp_ble_gap_config_adv_data_raw(raw_adv_data, sizeof(raw_adv_data));
  if (raw_adv_ret)
  {
    ESP_LOGE(BT_BLE_COEX_TAG, "config raw adv data failed, error code = 0x%x ", raw_adv_ret);
  }
  esp_err_t raw_scan_ret = esp_ble_gap_config_scan_rsp_data_raw(raw_adv_data, sizeof(raw_adv_data));
  if (raw_scan_ret)
  {
    ESP_LOGE(BT_BLE_COEX_TAG, "config raw scan rsp data failed, error code = 0x%x", raw_scan_ret);
  }
}

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
  switch (event)
  {
  case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
    // esp_ble_gap_start_advertising(&adv_params);
    break;
  case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
    esp_ble_gap_start_advertising(&adv_params);
    break;
  case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
    // advertising start complete event to indicate advertising start successfully or failed
    if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
    {
      ESP_LOGE(BT_BLE_COEX_TAG, "Advertising start failed");
    }
    else
    {
      ESP_LOGI(BT_BLE_COEX_TAG, "Start adv successfully");
    }
    break;
  case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
    if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
    {
      ESP_LOGE(BT_BLE_COEX_TAG, "Advertising stop failed");
    }
    else
    {
      ESP_LOGI(BT_BLE_COEX_TAG, "Stop adv successfully");
    }
    break;
  case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
    ESP_LOGI(BT_BLE_COEX_TAG, "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
             param->update_conn_params.status,
             param->update_conn_params.min_int,
             param->update_conn_params.max_int,
             param->update_conn_params.conn_int,
             param->update_conn_params.latency,
             param->update_conn_params.timeout);
    break;
  default:
    break;
  }
}

void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
  esp_gatt_status_t status = ESP_GATT_OK;
  if (param->write.need_rsp)
  {
    if (param->write.is_prep)
    {
      if (prepare_write_env->prepare_buf == NULL)
      {
        prepare_write_env->prepare_buf = (uint8_t *)malloc(PREPARE_BUF_MAX_SIZE * sizeof(uint8_t));
        prepare_write_env->prepare_len = 0;
        if (prepare_write_env->prepare_buf == NULL)
        {
          ESP_LOGE(BT_BLE_COEX_TAG, "Gatt_server prep no mem");
          status = ESP_GATT_NO_RESOURCES;
        }
      }
      else
      {
        if (param->write.offset > PREPARE_BUF_MAX_SIZE)
        {
          status = ESP_GATT_INVALID_OFFSET;
        }
        else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE)
        {
          status = ESP_GATT_INVALID_ATTR_LEN;
        }
      }

      esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
      gatt_rsp->attr_value.len = param->write.len;
      gatt_rsp->attr_value.handle = param->write.handle;
      gatt_rsp->attr_value.offset = param->write.offset;
      gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
      memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
      esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, gatt_rsp);
      if (response_err != ESP_OK)
      {
        ESP_LOGE(BT_BLE_COEX_TAG, "Send response error");
      }
      free(gatt_rsp);
      if (status != ESP_GATT_OK)
      {
        return;
      }
      memcpy(prepare_write_env->prepare_buf + param->write.offset,
             param->write.value,
             param->write.len);
      prepare_write_env->prepare_len += param->write.len;
    }
    else
    {
      esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status, NULL);
    }
  }
}

void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param)
{
  if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC)
  {
    esp_log_buffer_hex(BT_BLE_COEX_TAG, prepare_write_env->prepare_buf, prepare_write_env->prepare_len);
  }
  else
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "ESP_GATT_PREP_WRITE_CANCEL");
  }
  if (prepare_write_env->prepare_buf)
  {
    free(prepare_write_env->prepare_buf);
    prepare_write_env->prepare_buf = NULL;
  }
  prepare_write_env->prepare_len = 0;
}

static void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
  switch (event)
  {
  case ESP_GATTS_REG_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "REGISTER_APP_EVT, status %d, app_id %d", param->reg.status, param->reg.app_id);
    esp_ble_gap_config_local_privacy(true);
    gl_profile_tab[PROFILE_A_APP_ID].service_id.is_primary = true;
    gl_profile_tab[PROFILE_A_APP_ID].service_id.id.inst_id = 0x00;
    gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab[PROFILE_A_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_A;
    //init BLE adv data and scan response data
    ble_init_adv_data(BLE_ADV_NAME);
    esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_A_APP_ID].service_id, GATTS_NUM_HANDLE_A);
    break;
  }
  case ESP_GATTS_READ_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
    esp_gatt_rsp_t rsp;
    memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
    rsp.attr_value.handle = param->read.handle;
    rsp.attr_value.len = 4;
    rsp.attr_value.value[0] = 0xde;
    rsp.attr_value.value[1] = 0xed;
    rsp.attr_value.value[2] = 0xbe;
    rsp.attr_value.value[3] = 0xef;
    esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                ESP_GATT_OK, &rsp);
    break;
  }
  case ESP_GATTS_WRITE_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
    if (!param->write.is_prep)
    {
      ESP_LOGI(BT_BLE_COEX_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
      esp_log_buffer_hex(BT_BLE_COEX_TAG, param->write.value, param->write.len);
      if (gl_profile_tab[PROFILE_A_APP_ID].descr_handle == param->write.handle && param->write.len == 2)
      {
        uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
        if (descr_value == 0x0001)
        {
          if (a_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
          {
            ESP_LOGI(BT_BLE_COEX_TAG, "notify enable");
            uint8_t notify_data[15];
            for (int i = 0; i < sizeof(notify_data); ++i)
            {
              notify_data[i] = i % 0xff;
            }
            // the size of notify_data[] need less than MTU size
            esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                        sizeof(notify_data), notify_data, false);
          }
        }
        else if (descr_value == 0x0002)
        {
          if (a_property & ESP_GATT_CHAR_PROP_BIT_INDICATE)
          {
            ESP_LOGI(BT_BLE_COEX_TAG, "indicate enable");
            uint8_t indicate_data[15];
            for (int i = 0; i < sizeof(indicate_data); ++i)
            {
              indicate_data[i] = i % 0xff;
            }
            // the size of indicate_data[] need less than MTU size
            esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_A_APP_ID].char_handle,
                                        sizeof(indicate_data), indicate_data, true);
          }
        }
        else if (descr_value == 0x0000)
        {
          ESP_LOGI(BT_BLE_COEX_TAG, "notify/indicate disable ");
        }
        else
        {
          ESP_LOGE(BT_BLE_COEX_TAG, "unknown descr value");
          esp_log_buffer_hex(BT_BLE_COEX_TAG, param->write.value, param->write.len);
        }
      }
    }
    example_write_event_env(gatts_if, &a_prepare_write_env, param);
    break;
  }
  case ESP_GATTS_EXEC_WRITE_EVT:
  {

    ESP_LOGI(BT_BLE_COEX_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
    example_exec_write_event_env(&a_prepare_write_env, param);
    break;
  }
  case ESP_GATTS_MTU_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
    break;
  }
  case ESP_GATTS_UNREG_EVT:
    break;
  case ESP_GATTS_CREATE_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d", param->create.status, param->create.service_handle);
    gl_profile_tab[PROFILE_A_APP_ID].service_handle = param->create.service_handle;
    gl_profile_tab[PROFILE_A_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab[PROFILE_A_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_A;

    esp_ble_gatts_start_service(gl_profile_tab[PROFILE_A_APP_ID].service_handle);
    a_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
    esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_A_APP_ID].service_handle, &gl_profile_tab[PROFILE_A_APP_ID].char_uuid,
                                                    ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                    a_property,
                                                    &gatts_initial_char_val, NULL);
    if (add_char_ret)
    {
      ESP_LOGE(BT_BLE_COEX_TAG, "add char failed, error code = 0x%x", add_char_ret);
    }
    break;
  }
  case ESP_GATTS_ADD_INCL_SRVC_EVT:
    break;
  case ESP_GATTS_ADD_CHAR_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d",
             param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
    gl_profile_tab[PROFILE_A_APP_ID].char_handle = param->add_char.attr_handle;
    gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab[PROFILE_A_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
    esp_err_t add_descr_ret = esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_A_APP_ID].service_handle, &gl_profile_tab[PROFILE_A_APP_ID].descr_uuid,
                                                           ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, NULL, NULL);
    if (add_descr_ret)
    {
      ESP_LOGE(BT_BLE_COEX_TAG, "add char descr failed, error code = 0x%x", add_descr_ret);
    }
    break;
  }
  case ESP_GATTS_ADD_CHAR_DESCR_EVT:
  {
    gl_profile_tab[PROFILE_A_APP_ID].descr_handle = param->add_char_descr.attr_handle;
    ESP_LOGI(BT_BLE_COEX_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d",
             param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
    break;
  }
  case ESP_GATTS_DELETE_EVT:
    break;
  case ESP_GATTS_START_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "SERVICE_START_EVT, status %d, service_handle %d",
             param->start.status, param->start.service_handle);
    break;
  }
  case ESP_GATTS_STOP_EVT:
    break;
  case ESP_GATTS_CONNECT_EVT:
  {
    esp_ble_conn_update_params_t conn_params = {0};
    memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
    break;
  }
  case ESP_GATTS_DISCONNECT_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "ESP_GATTS_DISCONNECT_EVT");
    esp_ble_gap_start_advertising(&adv_params);
    break;
  }
  case ESP_GATTS_CONF_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "ESP_GATTS_CONF_EVT, status %d", param->conf.status);
    if (param->conf.status != ESP_GATT_OK)
    {
      esp_log_buffer_hex(BT_BLE_COEX_TAG, param->conf.value, param->conf.len);
    }
    break;
  }
  case ESP_GATTS_OPEN_EVT:
  case ESP_GATTS_CANCEL_OPEN_EVT:
  case ESP_GATTS_CLOSE_EVT:
  case ESP_GATTS_LISTEN_EVT:
  case ESP_GATTS_CONGEST_EVT:
  default:
    break;
  }
}

static void gatts_profile_b_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
  switch (event)
  {
  case ESP_GATTS_REG_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "REGISTER_APP_EVT, status %d, app_id %d", param->reg.status, param->reg.app_id);
    gl_profile_tab[PROFILE_B_APP_ID].service_id.is_primary = true;
    gl_profile_tab[PROFILE_B_APP_ID].service_id.id.inst_id = 0x00;
    gl_profile_tab[PROFILE_B_APP_ID].service_id.id.uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab[PROFILE_B_APP_ID].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_B;

    esp_ble_gatts_create_service(gatts_if, &gl_profile_tab[PROFILE_B_APP_ID].service_id, GATTS_NUM_HANDLE_B);
    break;
  }
  case ESP_GATTS_READ_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "GATT_READ_EVT, conn_id %d, trans_id %d, handle %d", param->read.conn_id, param->read.trans_id, param->read.handle);
    esp_gatt_rsp_t rsp;
    memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
    rsp.attr_value.handle = param->read.handle;
    rsp.attr_value.len = 4;
    rsp.attr_value.value[0] = 0xde;
    rsp.attr_value.value[1] = 0xed;
    rsp.attr_value.value[2] = 0xbe;
    rsp.attr_value.value[3] = 0xef;
    esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id,
                                ESP_GATT_OK, &rsp);
    break;
  }
  case ESP_GATTS_WRITE_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "GATT_WRITE_EVT, conn_id %d, trans_id %d, handle %d", param->write.conn_id, param->write.trans_id, param->write.handle);
    if (!param->write.is_prep)
    {
      ESP_LOGI(BT_BLE_COEX_TAG, "GATT_WRITE_EVT, value len %d, value :", param->write.len);
      esp_log_buffer_hex(BT_BLE_COEX_TAG, param->write.value, param->write.len);
      if (gl_profile_tab[PROFILE_B_APP_ID].descr_handle == param->write.handle && param->write.len == 2)
      {
        uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
        if (descr_value == 0x0001)
        {
          if (b_property & ESP_GATT_CHAR_PROP_BIT_NOTIFY)
          {
            ESP_LOGI(BT_BLE_COEX_TAG, "notify enable");
            uint8_t notify_data[15];
            for (int i = 0; i < sizeof(notify_data); ++i)
            {
              notify_data[i] = i % 0xff;
            }
            // the size of notify_data[] need less than MTU size
            esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_B_APP_ID].char_handle,
                                        sizeof(notify_data), notify_data, false);
          }
        }
        else if (descr_value == 0x0002)
        {
          if (b_property & ESP_GATT_CHAR_PROP_BIT_INDICATE)
          {
            ESP_LOGI(BT_BLE_COEX_TAG, "indicate enable");
            uint8_t indicate_data[15];
            for (int i = 0; i < sizeof(indicate_data); ++i)
            {
              indicate_data[i] = i % 0xff;
            }
            // the size of indicate_data[] need less than MTU size
            esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, gl_profile_tab[PROFILE_B_APP_ID].char_handle,
                                        sizeof(indicate_data), indicate_data, true);
          }
        }
        else if (descr_value == 0x0000)
        {
          ESP_LOGI(BT_BLE_COEX_TAG, "notify/indicate disable ");
        }
        else
        {
          ESP_LOGE(BT_BLE_COEX_TAG, "unknown value");
        }
      }
    }
    example_write_event_env(gatts_if, &b_prepare_write_env, param);
    break;
  }
  case ESP_GATTS_EXEC_WRITE_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
    example_exec_write_event_env(&b_prepare_write_env, param);
    break;
  }
  case ESP_GATTS_MTU_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
    break;
  }
  case ESP_GATTS_UNREG_EVT:
    break;
  case ESP_GATTS_CREATE_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "CREATE_SERVICE_EVT, status %d,  service_handle %d", param->create.status, param->create.service_handle);
    gl_profile_tab[PROFILE_B_APP_ID].service_handle = param->create.service_handle;
    gl_profile_tab[PROFILE_B_APP_ID].char_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab[PROFILE_B_APP_ID].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_B;

    esp_ble_gatts_start_service(gl_profile_tab[PROFILE_B_APP_ID].service_handle);
    b_property = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
    esp_err_t add_char_ret = esp_ble_gatts_add_char(gl_profile_tab[PROFILE_B_APP_ID].service_handle, &gl_profile_tab[PROFILE_B_APP_ID].char_uuid,
                                                    ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                                    b_property,
                                                    NULL, NULL);
    if (add_char_ret)
    {
      ESP_LOGE(BT_BLE_COEX_TAG, "add char failed, error code = 0x%x", add_char_ret);
    }
    break;
  }
  case ESP_GATTS_ADD_INCL_SRVC_EVT:
    break;
  case ESP_GATTS_ADD_CHAR_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "ADD_CHAR_EVT, status %d,  attr_handle %d, service_handle %d",
             param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);

    gl_profile_tab[PROFILE_B_APP_ID].char_handle = param->add_char.attr_handle;
    gl_profile_tab[PROFILE_B_APP_ID].descr_uuid.len = ESP_UUID_LEN_16;
    gl_profile_tab[PROFILE_B_APP_ID].descr_uuid.uuid.uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
    esp_ble_gatts_add_char_descr(gl_profile_tab[PROFILE_B_APP_ID].service_handle, &gl_profile_tab[PROFILE_B_APP_ID].descr_uuid,
                                 ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                 NULL, NULL);
    break;
  }
  case ESP_GATTS_ADD_CHAR_DESCR_EVT:
  {
    gl_profile_tab[PROFILE_B_APP_ID].descr_handle = param->add_char_descr.attr_handle;
    ESP_LOGI(BT_BLE_COEX_TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d",
             param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
    break;
  }
  case ESP_GATTS_DELETE_EVT:
    break;
  case ESP_GATTS_START_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "SERVICE_START_EVT, status %d, service_handle %d",
             param->start.status, param->start.service_handle);
    break;
  }
  case ESP_GATTS_STOP_EVT:
    break;
  case ESP_GATTS_CONNECT_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x:",
             param->connect.conn_id,
             param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
             param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
    gl_profile_tab[PROFILE_B_APP_ID].conn_id = param->connect.conn_id;
    break;
  }
  case ESP_GATTS_CONF_EVT:
  {
    ESP_LOGI(BT_BLE_COEX_TAG, "ESP_GATTS_CONF_EVT status %d", param->conf.status);
    if (param->conf.status != ESP_GATT_OK)
    {
      esp_log_buffer_hex(BT_BLE_COEX_TAG, param->conf.value, param->conf.len);
    }
    break;
  }
  case ESP_GATTS_DISCONNECT_EVT:
  case ESP_GATTS_OPEN_EVT:
  case ESP_GATTS_CANCEL_OPEN_EVT:
  case ESP_GATTS_CLOSE_EVT:
  case ESP_GATTS_LISTEN_EVT:
  case ESP_GATTS_CONGEST_EVT:
  default:
    break;
  }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
  /* If event is register event, store the gatts_if for each profile */
  if (event == ESP_GATTS_REG_EVT)
  {
    if (param->reg.status == ESP_GATT_OK)
    {
      gl_profile_tab[param->reg.app_id].gatts_if = gatts_if;
    }
    else
    {
      ESP_LOGI(BT_BLE_COEX_TAG, "Reg app failed, app_id %04x, status %d",
               param->reg.app_id,
               param->reg.status);
      return;
    }
  }

  /* If the gatts_if equal to profile A, call profile A cb handler,
     * so here call each profile's callback */
  do
  {
    int idx;
    for (idx = 0; idx < PROFILE_NUM; idx++)
    {
      if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
          gatts_if == gl_profile_tab[idx].gatts_if)
      {
        if (gl_profile_tab[idx].gatts_cb)
        {
          gl_profile_tab[idx].gatts_cb(event, gatts_if, param);
        }
      }
    }
  } while (0);
}

static void ble_gatts_init(void)
{
  esp_err_t ret = esp_ble_gatts_register_callback(gatts_event_handler);
  if (ret)
  {
    ESP_LOGE(BT_BLE_COEX_TAG, "gatts register error, error code = 0x%x", ret);
    return;
  }
  ret = esp_ble_gap_register_callback(gap_event_handler);
  if (ret)
  {
    ESP_LOGE(BT_BLE_COEX_TAG, "gap register error, error code = 0x%x", ret);
    return;
  }
  ret = esp_ble_gatts_app_register(PROFILE_A_APP_ID);
  if (ret)
  {
    ESP_LOGE(BT_BLE_COEX_TAG, "gatts app register error, error code = 0x%x", ret);
    return;
  }
  ret = esp_ble_gatts_app_register(PROFILE_B_APP_ID);
  if (ret)
  {
    ESP_LOGE(BT_BLE_COEX_TAG, "gatts app register error, error code = 0x%x", ret);
    return;
  }
  esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
  if (local_mtu_ret)
  {
    ESP_LOGE(BT_BLE_COEX_TAG, "set local  MTU failed, error code = 0x%x", local_mtu_ret);
  }
}

/* handler for bluetooth stack enabled events */
static void bt_av_hdl_stack_evt(uint16_t event, void *p_param)
{
  ESP_LOGD(BT_BLE_COEX_TAG, "%s evt %d", __func__, event);
  switch (event)
  {
  case BT_APP_EVT_STACK_UP:
  {
    /* set up bt device name */
    esp_bt_dev_set_device_name(BT_DEVICE_NAME);

    /* initialize A2DP sink */
    esp_a2d_register_callback(&bt_app_a2d_cb);
    esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb);
    esp_a2d_sink_init();

    /* initialize AVRCP controller */
    esp_avrc_ct_init();
    esp_avrc_ct_register_callback(bt_app_rc_ct_cb);

    /* set discoverable and connectable mode, wait to be connected */
    esp_bt_gap_set_scan_mode(ESP_BT_SCAN_MODE_CONNECTABLE_DISCOVERABLE);
    break;
  }
  default:
  {
    ESP_LOGE(BT_BLE_COEX_TAG, "%s unhandled evt %d", __func__, event);
    break;
  }
  }
}

int iHue = 255 / 100;
void testCylonSpeed(void *pvParameters)
{
  while (1)
  {
    fadeToBlackBy(leds, NUM_LEDS, 7);
    static int i = 0;
    if (i > NUM_LEDS - 1)
    {
      i = 0;
    }
    i++;
    leds[i] = ColorFromPalette(currentPalette, i * iHue, 255, currentBlending);

    FastLED.show();
  }
};

esp_err_t _http_event_handler(esp_http_client_event_t *evt)
{
  switch (evt->event_id)
  {
  case HTTP_EVENT_ERROR:
    ESP_LOGD(OTA_TAG, "HTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGD(OTA_TAG, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGD(OTA_TAG, "HTTP_EVENT_ON_FINISH");
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGD(OTA_TAG, "HTTP_EVENT_DISCONNECTED");
    break;
  }
  return ESP_OK;
}

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
  switch (event->event_id)
  {
  case SYSTEM_EVENT_STA_START:
    esp_wifi_connect();
    break;
  case SYSTEM_EVENT_STA_GOT_IP:
    xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    break;
  case SYSTEM_EVENT_STA_DISCONNECTED:
    /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
    esp_wifi_connect();
    xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    break;
  default:
    break;
  }
  return ESP_OK;
}

static void initialise_wifi(void)
{
  tcpip_adapter_init();
  wifi_event_group = xEventGroupCreate();
  ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
  wifi_config_t wifi_config = {
      .sta = {
          {.ssid = CONFIG_WIFI_SSID},
          {.password = CONFIG_WIFI_PASSWORD},
      },
  };
  ESP_LOGI(OTA_TAG, "Setting WiFi configuration SSID %s", wifi_config.sta.ssid);
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
  ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
  ESP_ERROR_CHECK(esp_wifi_start());
}

void simple_ota_example_task(void *pvParameter)
{
  /* Wait for the callback to set the CONNECTED_BIT in the
       event group.
    */
  xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT,
                      false, true, portMAX_DELAY);
  ESP_LOGI(OTA_TAG, "Starting OTA example");
  ESP_LOGI(OTA_TAG, "Connected to WiFi network! Attempting to connect to server...");

  esp_http_client_config_t config = {
      .url = CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL,
      .host = "192.168.0.3",
      .port = 8070,
      .username = "",
      .password = "",
      .auth_type = HTTP_AUTH_TYPE_NONE,
      .path = "/hello-world.bin",
      .query = "",
      .cert_pem = (char *)server_cert_pem_start,
      .client_cert_pem = NULL,
      .client_key_pem = NULL,
      .method = HTTP_METHOD_GET,
      .timeout_ms = 1000000,
      .disable_auto_redirect = true,
      .max_redirection_count = 0,
      .event_handler = _http_event_handler,
      .transport_type = HTTP_TRANSPORT_UNKNOWN,
      .buffer_size = 1000,
      .user_data = NULL,
      .is_async = false,
      .use_global_ca_store = false,
      .skip_cert_common_name_check = true,
  };

  // typedef struct
  // {
  //   const char *url;                            /*!< HTTP URL, the information on the URL is most important, it overrides the other fields below, if any */
  //   const char *host;                           /*!< Domain or IP as string */
  //   int port;                                   /*!< Port to connect, default depend on esp_http_client_transport_t (80 or 443) */
  //   const char *username;                       /*!< Using for Http authentication */
  //   const char *password;                       /*!< Using for Http authentication */
  //   esp_http_client_auth_type_t auth_type;      /*!< Http authentication type, see `esp_http_client_auth_type_t` */
  //   const char *path;                           /*!< HTTP Path, if not set, default is `/` */
  //   const char *query;                          /*!< HTTP query */
  //   const char *cert_pem;                       /*!< SSL server certification, PEM format as string, if the client requires to verify server */
  //   const char *client_cert_pem;                /*!< SSL client certification, PEM format as string, if the server requires to verify client */
  //   const char *client_key_pem;                 /*!< SSL client key, PEM format as string, if the server requires to verify client */
  //   esp_http_client_method_t method;            /*!< HTTP Method */
  //   int timeout_ms;                             /*!< Network timeout in milliseconds */
  //   bool disable_auto_redirect;                 /*!< Disable HTTP automatic redirects */
  //   int max_redirection_count;                  /*!< Max redirection number, using default value if zero*/
  //   http_event_handle_cb event_handler;         /*!< HTTP Event Handle */
  //   esp_http_client_transport_t transport_type; /*!< HTTP transport type, see `esp_http_client_transport_t` */
  //   int buffer_size;                            /*!< HTTP buffer size (both send and receive) */
  //   void *user_data;                            /*!< HTTP user_data context */
  //   bool is_async;                              /*!< Set asynchronous mode, only supported with HTTPS for now */
  //   bool use_global_ca_store;                   /*!< Use a global ca_store for all the connections in which this bool is set. */
  //   bool skip_cert_common_name_check;           /*!< Skip any validation of server certificate CN field */
  // } esp_http_client_config_t;

#ifdef CONFIG_EXAMPLE_FIRMWARE_UPGRADE_URL_FROM_STDIN
  char url_buf[OTA_URL_SIZE];
  if (strcmp(config.url, "FROM_STDIN") == 0)
  {
    example_configure_stdin_stdout();
    fgets(url_buf, OTA_URL_SIZE, stdin);
    int len = strlen(url_buf);
    url_buf[len - 1] = '\0';
    config.url = url_buf;
  }
  else
  {
    ESP_LOGE(OTA_TAG, "Configuration mismatch: wrong firmware upgrade image url");
    abort();
  }
#endif

#ifdef CONFIG_EXAMPLE_SKIP_COMMON_NAME_CHECK
  config.skip_cert_common_name_check = true;
#endif

  esp_err_t err = esp_https_ota(&config);
  if (err == ESP_OK)
  {
    esp_restart();
  }
  else
  {
    ESP_LOGE(OTA_TAG, "Firmware upgrade failed");
  }
  while (1)
  {
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}

#ifdef __cplusplus
extern "C"
{
#endif
  void app_main()
  {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES)
    {
      // NVS partition was truncated and needs to be erased
      // Retry nvs_flash_init
      ESP_ERROR_CHECK(nvs_flash_erase());
      err = nvs_flash_init();
    }

    pinMode(22, OUTPUT);
    digitalWrite(22, HIGH);

    FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.setMaxPowerInVoltsAndMilliamps(5, 30000);

    init_fft();
    initialise_wifi();

#if CONFIG_SPIRAM_SUPPORT
    psramInit();
#endif

    ESP_LOGI(AUDIO_CODEC_TAG, "[ 1 ] Create Bluetooth service");

    ESP_LOGI(AUDIO_CODEC_TAG, "[ 2 ] Start codec chip");
    audio_board_handle_t board_handle = audio_board_init();
    audio_hal_ctrl_codec(board_handle->audio_hal, AUDIO_HAL_CODEC_MODE_DECODE, AUDIO_HAL_CTRL_START);
    audio_hal_set_volume(board_handle->audio_hal, 100);

    ESP_LOGI(AUDIO_CODEC_TAG, "[3.1] Create i2s stream to write data to codec chip");
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT();
    i2s_cfg.type = AUDIO_STREAM_WRITER;
    audio_element_handle_t i2s_stream_writer = i2s_stream_init(&i2s_cfg);

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    err = esp_bt_controller_init(&bt_cfg);
    if (err)
    {
      ESP_LOGE(BT_BLE_COEX_TAG, "%s initialize controller failed", __func__);
      return;
    }

    err = esp_bt_controller_enable(ESP_BT_MODE_BTDM);
    if (err)
    {
      ESP_LOGE(BT_BLE_COEX_TAG, "%s enable controller failed", __func__);
      return;
    }

    err = esp_bluedroid_init();
    if (err)
    {
      ESP_LOGE(BT_BLE_COEX_TAG, "%s init bluetooth failed", __func__);
      return;
    }
    err = esp_bluedroid_enable();
    if (err)
    {
      ESP_LOGE(BT_BLE_COEX_TAG, "%s enable bluetooth failed", __func__);
      return;
    }

    /* create application task */
    bt_app_task_start_up();

    /* Bluetooth device name, connection mode and profile set up */
    bt_app_work_dispatch(bt_av_hdl_stack_evt, BT_APP_EVT_STACK_UP, NULL, 0, NULL);

    //gatt server init
    ble_gatts_init();

    xTaskCreatePinnedToCore(&testCylonSpeed, "testCylonSpeed", 4000, NULL, 5, NULL, 1);
    xTaskCreate(&simple_ota_example_task, "ota_example_task", 8192, NULL, 5, NULL);

    while (1)
    {
      vTaskDelay(10);
      // main application loop
    }

    audio_element_deinit(i2s_stream_writer);
    deinit_fft();
  }
#ifdef __cplusplus
}
#endif