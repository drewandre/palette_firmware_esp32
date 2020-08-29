#ifndef __A2DP_CONTROLLER_H__
#define __A2DP_CONTROLLER_H__

#include "esp_bt.h"
#include "bt_app_av.hpp"
#include "esp_a2dp_api.h"
#include "esp_avrc_api.h"

/* event for handler "bt_av_hdl_stack_up */
enum
{
  BT_APP_EVT_STACK_UP = 0,
};

void init_a2dp_controller();

void bt_av_hdl_stack_evt(uint16_t event, void *p_param);

void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

void deinit_a2dp_controller();

#endif /* __A2DP_CONTROLLER_H__*/