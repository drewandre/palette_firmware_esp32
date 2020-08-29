#include "application_task_controller.hpp"

#ifdef __cplusplus
extern "C"
{
#endif
  void app_main()
  {
    initArduino();
    init_application_task_controller();

    // Serial.println("CPU0 reset reason:");
    // print_reset_reason(rtc_get_reset_reason(0));
    // verbose_print_reset_reason(rtc_get_reset_reason(0));

    // Serial.println("CPU1 reset reason:");
    // print_reset_reason(rtc_get_reset_reason(1));
    // verbose_print_reset_reason(rtc_get_reset_reason(1));

    // Serial.print("ESP32 SDK: ");
    // Serial.println(ESP.getSdkVersion());
    
    for (;;)
    {
      if (run_app_mode_switcher_task() == SHUT_DOWN)
      {
        break;
      }
    }
    vTaskDelete(NULL);
  }

#ifdef __cplusplus
}
#endif