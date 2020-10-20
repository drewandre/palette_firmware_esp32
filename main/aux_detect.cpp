/*
 * ESPRESSIF MIT License
 *
 * Copyright (c) 2018 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on all ESPRESSIF SYSTEMS products, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/timers.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "pin_def.h"

#define HP_DELAY_TIME_MS 500

const char *AUX_DETECT_TAG = "AUX_DETECT";
static bool initialized = false;

static xTimerHandle timer_aux;

static void aux_timer_cb(TimerHandle_t xTimer)
{
  int res = gpio_get_level(AUX_INPUT_DETECT);
  ESP_LOGW(AUX_DETECT_TAG, "Aux jack %s", res ? "removed" : "inserted");
}

static int aux_timer_init(int num)
{
  timer_aux = xTimerCreate("aux_timer0", HP_DELAY_TIME_MS / portTICK_RATE_MS, pdFALSE, (void *) num, aux_timer_cb);
  if (timer_aux == NULL) {
    ESP_LOGE(AUX_DETECT_TAG, "aux_timer create err");
    return ESP_FAIL;
  }
  return ESP_OK;
}

static void IRAM_ATTR aux_gpio_intr_handler(void *arg)
{
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xTimerResetFromISR(timer_aux, &xHigherPriorityTaskWoken);
  if (xHigherPriorityTaskWoken != pdFALSE) {
    portYIELD_FROM_ISR();
  }
}

int aux_status_get()
{
  return gpio_get_level(AUX_INPUT_DETECT);
}

void init_aux_detect()
{
  if (initialized)
  {
    ESP_LOGW(AUX_DETECT_TAG, "Aux input detection controller has already been initialized. Skipping %s", __func__);
    return;
  }
  ESP_LOGI(AUX_DETECT_TAG, "Initializing aux input detection controller");

  aux_timer_init(AUX_INPUT_DETECT);
  gpio_config_t io_conf;
  memset(&io_conf, 0, sizeof(io_conf));
  io_conf.intr_type = GPIO_INTR_ANYEDGE;
  io_conf.mode = GPIO_MODE_INPUT;
  io_conf.pin_bit_mask = BIT64(AUX_INPUT_DETECT);
  io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&io_conf);
  gpio_install_isr_service(0);
  gpio_isr_handler_add(AUX_INPUT_DETECT, aux_gpio_intr_handler, (void *)AUX_INPUT_DETECT);
  initialized = true;
  ESP_LOGI(AUX_DETECT_TAG, "Initialized aux input detection");
}

void deinit_aux_detect()
{
  if (!initialized) {
    ESP_LOGW(AUX_DETECT_TAG, "Aux detect controller not yet initialized - skipping %s", __func__);
    return;
  }
  xTimerDelete(timer_aux, HP_DELAY_TIME_MS / portTICK_RATE_MS);
  gpio_uninstall_isr_service();
  timer_aux = NULL;
  initialized = false;
  ESP_LOGI(AUX_DETECT_TAG, "Deinitialized aux detect controller");
}
