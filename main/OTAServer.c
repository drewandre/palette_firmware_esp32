#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <esp_log.h>
#include <sys/param.h>
#include <esp_http_server.h>
#include "esp_ota_ops.h"
#include "esp_log.h"
#include "freertos/event_groups.h"

  // ESP_LOGI("HIGH", "%u", uxTaskGetStackHighWaterMark(NULL));

#define BUFFSIZE 1024
  // char ota_buff[2000000];
  static char ota_buff[BUFFSIZE + 1] = {0};

  // Embedded Files. To add or remove make changes is component.mk file as well.
  extern const uint8_t index_html_start[] asm("_binary_index_html_start");
  extern const uint8_t index_html_end[] asm("_binary_index_html_end");

  extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
  extern const uint8_t favicon_ico_end[] asm("_binary_favicon_ico_end");

  httpd_handle_t OTA_server = NULL;
  int8_t flash_status = 0;

  EventGroupHandle_t reboot_event_group;
  const int REBOOT_BIT = BIT0;

  /*****************************************************
 
	systemRebootTask()
 
	NOTES: This had to be a task because the web page needed
			an ack back. So i could not call this in the handler
 
 *****************************************************/
  void systemRebootTask(void *parameter)
  {

    // Init the event group
    reboot_event_group = xEventGroupCreate();

    // Clear the bit
    xEventGroupClearBits(reboot_event_group, REBOOT_BIT);

    for (;;)
    {
      // Wait here until the bit gets set for reboot
      EventBits_t staBits = xEventGroupWaitBits(reboot_event_group, REBOOT_BIT, pdTRUE, pdFALSE, portMAX_DELAY);

      // Did portMAX_DELAY ever timeout, not sure so lets just check to be sure
      if ((staBits & REBOOT_BIT) != 0)
      {
        ESP_LOGI("OTA", "Reboot Command, Restarting");
        vTaskDelay(2000 / portTICK_PERIOD_MS);

        esp_restart();
      }
    }
  }

  /* Send index.html Page */
  esp_err_t OTA_index_html_handler(httpd_req_t *req)
  {
    ESP_LOGI("OTA", "index.html Requested");

    // Clear this every time page is requested
    flash_status = 0;

    httpd_resp_set_type(req, "text/html");

    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);

    return ESP_OK;
  }
  /* Send .ICO (icon) file  */
  esp_err_t OTA_favicon_ico_handler(httpd_req_t *req)
  {
    ESP_LOGI("OTA", "favicon_ico Requested");

    httpd_resp_set_type(req, "image/x-icon");

    httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_end - favicon_ico_start);

    return ESP_OK;
  }

  /* Status */
  esp_err_t OTA_update_status_handler(httpd_req_t *req)
  {
    char ledJSON[100];

    ESP_LOGI("OTA", "Status Requested");

    sprintf(ledJSON, "{\"status\":%d,\"compile_time\":\"%s\",\"compile_date\":\"%s\"}", flash_status, __TIME__, __DATE__);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, ledJSON, strlen(ledJSON));

    if (flash_status == 1)
    {
      // We cannot directly call reboot here because we need the
      // browser to get the ack back.
      xEventGroupSetBits(reboot_event_group, REBOOT_BIT);
    }

    return ESP_OK;
  }
  /* Receive .Bin file */
  esp_err_t OTA_update_post_handler(httpd_req_t *req)
  {
    esp_ota_handle_t ota_handle;

    int content_length = req->content_len;
    int content_received = 0;
    int recv_len;
    bool is_req_body_started = false;
    const esp_partition_t *update_partition = esp_ota_get_next_update_partition(NULL);

    // Unsuccessful Flashing
    flash_status = -1;

    ESP_LOGI("OTA", "%u", content_length);
    do
    {
      /* Read the data for the request */
      if ((recv_len = httpd_req_recv(req, ota_buff, MIN(content_length, sizeof(ota_buff)))) < 0)
      {
        if (recv_len == HTTPD_SOCK_ERR_TIMEOUT)
        {
          ESP_LOGI("OTA", "Socket Timeout");
          /* Retry receiving if timeout occurred */
          continue;
        }
        ESP_LOGI("OTA", "OTA Other Error %d", recv_len);
        return ESP_FAIL;
      }

      ESP_LOGI("OTA RX", "%d of %d ", content_received, content_length);

      // Is this the first data we are receiving
      // If so, it will have the information in the header we need.
      if (!is_req_body_started)
      {
        is_req_body_started = true;

        // Lets find out where the actual data starts after the header info
        char *body_start_p = strstr(ota_buff, "\r\n\r\n") + 4;
        int body_part_len = recv_len - (body_start_p - ota_buff);

        //int body_part_sta = recv_len - body_part_len;
        //printf("OTA File Size: %d : Start Location:%d - End Location:%d\r\n", content_length, body_part_sta, body_part_len);
        ESP_LOGI("OTA File Size", "%d", content_length);
        ESP_LOGI("Attempting to write to partition subtype", "%d at offset 0x%x\r\n", update_partition->subtype, update_partition->address);

        esp_err_t err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &ota_handle);
        // if (err != ESP_OK)
        // {
        //   printf("Error With OTA Begin, Cancelling OTA\r\n");
        //   return ESP_FAIL;
        // }
        // else
        // {
        //   ESP_LOGI("Writing to partition subtype", "%d at offset 0x%x\r\n", update_partition->subtype, update_partition->address);
        // }

        // Lets write this first part of data out
        // esp_ota_write(ota_handle, body_start_p, body_part_len);
      }
      else
      {
        // Write OTA data
        // esp_ota_write(ota_handle, ota_buff, recv_len);

        content_received += recv_len;
      }
    } while (recv_len > 0 && content_received < content_length);

    // // End response
    // //httpd_resp_send_chunk(req, NULL, 0);

    // if (esp_ota_end(ota_handle) == ESP_OK)
    // {
    //   // Lets update the partition
    //   if (esp_ota_set_boot_partition(update_partition) == ESP_OK)
    //   {
    //     const esp_partition_t *boot_partition = esp_ota_get_boot_partition();

    //     // Webpage will request status when complete
    //     // This is to let it know it was successful
    //     flash_status = 1;

    //     ESP_LOGI("OTA", "Next boot partition subtype %d at offset 0x%x", boot_partition->subtype, boot_partition->address);
    //     ESP_LOGI("OTA", "Please Restart System...");
    //   }
    //   else
    //   {
    //     ESP_LOGI("OTA", "\r\n\r\n !!! Flashed Error !!!");
    //   }
    // }
    // else
    // {
    //   ESP_LOGI("OTA", "\r\n\r\n !!! OTA End Error !!!");
    // }

    return ESP_OK;
  }

  httpd_uri_t index_html = {
      .uri = "/",
      .method = HTTP_GET,
      .handler = OTA_index_html_handler,
      /* Let's pass response string in user
	 * context to demonstrate it's usage */
      .user_ctx = NULL};

  httpd_uri_t favicon_ico = {
      .uri = "/favicon.ico",
      .method = HTTP_GET,
      .handler = OTA_favicon_ico_handler,
      /* Let's pass response string in user
	 * context to demonstrate it's usage */
      .user_ctx = NULL};

  httpd_uri_t update = {
      .uri = "/update",
      .method = HTTP_POST,
      .handler = OTA_update_post_handler,
      .user_ctx = NULL};
  httpd_uri_t status = {
      .uri = "/status",
      .method = HTTP_POST,
      .handler = OTA_update_status_handler,
      .user_ctx = NULL};

  httpd_handle_t start_OTA_webserver(void)
  {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Need this task to spin up, see why in task
    // xTaskCreate(&systemRebootTask, "rebootTask", 2048, NULL, 5, NULL);

    // Lets change this from port 80 (default) to 8080
    config.server_port = 8080;

    // Lets bump up the stack size (default was 4096)
    config.stack_size = 40960;

    // Start the httpd server
    ESP_LOGI("OTA", "Starting server on port: '%d'", config.server_port);

    if (httpd_start(&OTA_server, &config) == ESP_OK)
    {
      // Set URI handlers
      ESP_LOGI("OTA", "Registering URI handlers");
      httpd_register_uri_handler(OTA_server, &index_html);
      httpd_register_uri_handler(OTA_server, &favicon_ico);
      httpd_register_uri_handler(OTA_server, &update);
      httpd_register_uri_handler(OTA_server, &status);
      ESP_LOGI("OTA", "Server successfully started");
      return OTA_server;
    }

    ESP_LOGI("OTA", "Error starting server!");
    return NULL;
  }

  void stop_OTA_webserver(httpd_handle_t server)
  {
    // Stop the httpd server
    httpd_stop(server);
  }

#ifdef __cplusplus
}
#endif