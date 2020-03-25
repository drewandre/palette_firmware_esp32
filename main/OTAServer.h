#ifdef __cplusplus
extern "C"
{
#endif

#include <esp_http_server.h>

  extern httpd_handle_t OTA_server;
  httpd_handle_t start_OTA_webserver(void);
  void stop_OTA_webserver(httpd_handle_t server);

#ifdef __cplusplus
}
#endif