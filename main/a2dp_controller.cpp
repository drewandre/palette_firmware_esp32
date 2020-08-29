#include "a2dp_controller.hpp"

const char *A2DP_TAG = "A2DP_CTRL";
const char *A2DP_DEVICE_NAME = "Palette A2DP";
static bool initialized = false;

void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
  switch (event)
  {
  case ESP_BT_GAP_CONFIG_EIR_DATA_EVT:
  case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
    break;
  case ESP_BT_GAP_AUTH_CMPL_EVT:
  {
    if (param->auth_cmpl.stat == ESP_BT_STATUS_SUCCESS)
    {
      ESP_LOGI(A2DP_TAG, "authentication success: %s", param->auth_cmpl.device_name);
      esp_log_buffer_hex(A2DP_TAG, param->auth_cmpl.bda, ESP_BD_ADDR_LEN);
    }
    else
    {
      ESP_LOGE(A2DP_TAG, "authentication failed, status:%d", param->auth_cmpl.stat);
    }
    break;
  }
#if (CONFIG_BT_SSP_ENABLED == true)
  case ESP_BT_GAP_CFM_REQ_EVT:
    ESP_LOGI(A2DP_TAG, "ESP_BT_GAP_CFM_REQ_EVT Please compare the numeric value: %d", param->cfm_req.num_val);
    esp_bt_gap_ssp_confirm_reply(param->cfm_req.bda, true);
    break;
  case ESP_BT_GAP_KEY_NOTIF_EVT:
    ESP_LOGI(A2DP_TAG, "ESP_BT_GAP_KEY_NOTIF_EVT passkey:%d", param->key_notif.passkey);
    break;
  case ESP_BT_GAP_KEY_REQ_EVT:
    ESP_LOGI(A2DP_TAG, "ESP_BT_GAP_KEY_REQ_EVT Please enter passkey!");
    break;
#endif
  default:
  {
    ESP_LOGI(A2DP_TAG, "event: %d", event);
    break;
  }
  }
  return;
}

/* handler for bluetooth stack enabled events */
void bt_av_hdl_stack_evt(uint16_t event, void *p_param)
{
  ESP_LOGD(A2DP_TAG, "%s evt %d", __func__, event);
  switch (event)
  {
  case BT_APP_EVT_STACK_UP:
  {
    /* set up bt device name */
    esp_bt_dev_set_device_name(A2DP_DEVICE_NAME);

    esp_bt_gap_register_callback(bt_app_gap_cb);

    /* initialize AVRCP controller */
    esp_avrc_ct_init();
    esp_avrc_ct_register_callback(bt_app_rc_ct_cb);
    /* initialize AVRCP target */
    assert(esp_avrc_tg_init() == ESP_OK);
    esp_avrc_tg_register_callback(bt_app_rc_tg_cb);

    esp_avrc_rn_evt_cap_mask_t evt_set = {0};
    esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
    assert(esp_avrc_tg_set_rn_evt_cap(&evt_set) == ESP_OK);

    /* initialize A2DP sink */
    esp_a2d_register_callback(&bt_app_a2d_cb);
    esp_a2d_sink_register_data_callback(bt_app_a2d_data_cb);
    esp_a2d_sink_init();

    /* set discoverable and connectable mode, wait to be connected */
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    break;
  }
  default:
    ESP_LOGE(A2DP_TAG, "%s unhandled evt %d", __func__, event);
    break;
  }
}

void init_a2dp_controller()
{
  if (initialized)
  {
    ESP_LOGW(A2DP_TAG, "A2DP controller has already been initialized. Skipping %s", __func__);
    return;
  }
  ESP_LOGI(A2DP_TAG, "Initializing A2DP controller");

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  esp_err_t err;
  if ((err = esp_bt_controller_init(&bt_cfg)) != ESP_OK)
  {
    ESP_LOGE(A2DP_TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(err));
    return;
  }

  if ((err = esp_bt_controller_enable(ESP_BT_MODE_BTDM)) != ESP_OK)
  {
    ESP_LOGE(A2DP_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(err));
    return;
  }

  if ((err = esp_bluedroid_init()) != ESP_OK)
  {
    ESP_LOGE(A2DP_TAG, "%s initialize bluedroid failed: %s", __func__, esp_err_to_name(err));
    return;
  }

  if ((err = esp_bluedroid_enable()) != ESP_OK)
  {
    ESP_LOGE(A2DP_TAG, "%s enable bluedroid failed: %s", __func__, esp_err_to_name(err));
    return;
  }

  /* create application task */
  bt_app_task_start_up();

  /* Bluetooth device name, connection mode and profile set up */
  bt_app_work_dispatch(bt_av_hdl_stack_evt, BT_APP_EVT_STACK_UP, NULL, 0, NULL);

#if (CONFIG_BT_SSP_ENABLED == true)
  /* Set default parameters for Secure Simple Pairing */
  esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
  esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
  esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));
#endif

  /*
  * Set default parameters for Legacy Pairing
  * Use fixed pin code
  */
  esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
  esp_bt_pin_code_t pin_code;
  pin_code[0] = '1';
  pin_code[1] = '2';
  pin_code[2] = '3';
  pin_code[3] = '4';
  esp_bt_gap_set_pin(pin_type, 4, pin_code);

  initialized = true;
  ESP_LOGI(A2DP_TAG, "A2DP controller initialized");
}

void deinit_a2dp_controller()
{
  if (!initialized) {
    ESP_LOGW(A2DP_TAG, "A2DP controller not yet initialized - skipping %s", __func__);
    return;
  }
  bt_i2s_task_shut_down();
  bt_app_task_shut_down();
  if (esp_a2d_sink_deinit() == ESP_OK)
  {
    ESP_LOGW(A2DP_TAG, "A2DP controller deinitialized");
    initialized = false;
  }
  else
  {
    ESP_LOGE(A2DP_TAG, "Unable to deinitialize A2DP controller");
  }
}