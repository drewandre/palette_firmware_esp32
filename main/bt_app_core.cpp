#include "bt_app_core.hpp"

const char *BT_APP_CORE_TAG = "BT_APP_CORE";

static void bt_app_task_handler(void *arg);
static bool bt_app_send_msg(bt_app_msg_t *msg);
static void bt_app_work_dispatched(bt_app_msg_t *msg);

static xQueueHandle s_bt_app_task_queue = NULL;
// static xTaskHandle BTAppTaskHandler = NULL;
// static xTaskHandle BTi2sTaskHandler = NULL;
TaskHandle_t BTAppTaskHandler = NULL;
TaskHandle_t BTi2sTaskHandler = NULL;
static RingbufHandle_t s_ringbuf_i2s = NULL;

bool bt_app_work_dispatch(bt_app_cb_t p_cback, uint16_t event, void *p_params, int param_len, bt_app_copy_cb_t p_copy_cback)
{
  ESP_LOGD(BT_APP_CORE_TAG, "%s event 0x%x, param len %d", __func__, event, param_len);

  bt_app_msg_t msg;
  memset(&msg, 0, sizeof(bt_app_msg_t));

  msg.sig = BT_APP_SIG_WORK_DISPATCH;
  msg.event = event;
  msg.cb = p_cback;

  if (param_len == 0)
  {
    return bt_app_send_msg(&msg);
  }
  else if (p_params && param_len > 0)
  {
    if ((msg.param = malloc(param_len)) != NULL)
    {
      memcpy(msg.param, p_params, param_len);
      /* check if caller has provided a copy callback to do the deep copy */
      if (p_copy_cback)
      {
        p_copy_cback(&msg, msg.param, p_params);
      }
      return bt_app_send_msg(&msg);
    }
  }

  return false;
}

static bool bt_app_send_msg(bt_app_msg_t *msg)
{
  if (msg == NULL)
  {
    return false;
  }

  if (xQueueSend(s_bt_app_task_queue, msg, 10 / portTICK_RATE_MS) != pdTRUE)
  {
    ESP_LOGE(BT_APP_CORE_TAG, "%s xQueue send failed", __func__);
    return false;
  }
  return true;
}

static void bt_app_work_dispatched(bt_app_msg_t *msg)
{
  if (msg->cb)
  {
    msg->cb(msg->event, msg->param);
  }
}

static void bt_app_task_handler(void *arg)
{
  bt_app_msg_t msg;
  for (;;)
  {
    if (pdTRUE == xQueueReceive(s_bt_app_task_queue, &msg, (portTickType)portMAX_DELAY))
    {
      ESP_LOGD(BT_APP_CORE_TAG, "%s, sig 0x%x, 0x%x", __func__, msg.sig, msg.event);
      switch (msg.sig)
      {
      case BT_APP_SIG_WORK_DISPATCH:
        bt_app_work_dispatched(&msg);
        break;
      default:
        ESP_LOGW(BT_APP_CORE_TAG, "%s, unhandled sig: %d", __func__, msg.sig);
        break;
      } // switch (msg.sig)

      if (msg.param)
      {
        free(msg.param);
      }
    }
  }
  vTaskDelete(NULL);
}

void bt_app_task_start_up(void)
{
  s_bt_app_task_queue = xQueueCreate(10, sizeof(bt_app_msg_t));
  xTaskCreatePinnedToCore(bt_app_task_handler, "BtAppT", BT_APP_TASK_STACK_SIZE, NULL, BT_APP_TASK_PRIORITY, &BTAppTaskHandler, BT_APP_TASK_CORE_ID);
  return;
}

void bt_app_task_shut_down(void)
{
  if (BTAppTaskHandler)
  {
    vTaskDelete(BTAppTaskHandler);
    BTAppTaskHandler = NULL;
  }
  if (s_bt_app_task_queue)
  {
    vQueueDelete(s_bt_app_task_queue);
    s_bt_app_task_queue = NULL;
  }
}

static void bt_i2s_task_handler(void *arg)
{
  uint8_t *data = NULL;
  size_t item_size = 0;
  size_t bytes_written = 0;

  for (;;)
  {
    data = (uint8_t *)xRingbufferReceive(s_ringbuf_i2s, &item_size, (portTickType)portMAX_DELAY);
    if (item_size != 0)
    {
      i2s_write(I2S_NUM_0, data, item_size, &bytes_written, portMAX_DELAY);
      vRingbufferReturnItem(s_ringbuf_i2s, (void *)data);
    }
  }
  vTaskDelete(NULL);
}

void bt_i2s_task_start_up(void)
{
  s_ringbuf_i2s = xRingbufferCreate(8 * 1024, RINGBUF_TYPE_BYTEBUF);
  if (s_ringbuf_i2s == NULL)
  {
    return;
  }

  xTaskCreatePinnedToCore(bt_i2s_task_handler, "BtI2ST", BT_I2S_TASK_STACK_SIZE, NULL, BT_I2S_TASK_PRIORITY, &BTi2sTaskHandler, BT_I2S_TASK_CORE_ID);
  return;
}

void bt_i2s_task_shut_down(void)
{
  if (BTi2sTaskHandler)
  {
    vTaskDelete(BTi2sTaskHandler);
    BTi2sTaskHandler = NULL;
  }

  if (s_ringbuf_i2s)
  {
    vRingbufferDelete(s_ringbuf_i2s);
    s_ringbuf_i2s = NULL;
  }
}

size_t write_ringbuf(const uint8_t *data, size_t size)
{
  BaseType_t done = xRingbufferSend(s_ringbuf_i2s, (void *)data, size, (portTickType)portMAX_DELAY);
  if (done)
  {
    return size;
  }
  else
  {
    return 0;
  }
}