#include "Arduino.h"
#include "SGTL5000_controller.hpp"
#include "led_controller.hpp"
#include "ble_controller.hpp"
#include "a2dp_controller.hpp"
#include "aux_detect.hpp"
#include "audio_analysis_controller.hpp"
#include "eeprom_controller.hpp"
#include "status_led_controller.hpp"

#ifdef __cplusplus
extern "C"
{
#endif
  void app_main()
  {
    initArduino();

    // init_status_led_controller();
    init_eeprom_controller();
    init_audio_analysis_controller(); // should be called before init_led_controller
    // init_aux_detect();
    init_sgtl5000_controller();
    init_a2dp_controller();
    init_ble_controller();
    init_led_controller();

    // Serial.println("CPU0 reset reason:");
    // print_reset_reason(rtc_get_reset_reason(0));
    // verbose_print_reset_reason(rtc_get_reset_reason(0));

    // Serial.println("CPU1 reset reason:");
    // print_reset_reason(rtc_get_reset_reason(1));
    // verbose_print_reset_reason(rtc_get_reset_reason(1));
  }

#ifdef __cplusplus
}
#endif