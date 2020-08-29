set(COMPONENT_SRCS "audio_analysis_controller.cpp" "free_rtos_controller.cpp" "aux_detect.cpp" "aux_detect.cpp" "status_led_controller.cpp" "application_task_controller.cpp" "ota_controller.cpp" "eeprom_controller.cpp" "ble_controller.cpp" "SGTL5000_controller.cpp" "sgtl5000_controller.cpp" "main.cpp" "led_controller.cpp" "iir_filter_controller.cpp" "bt_app_av.cpp"
                   "bt_app_core.cpp" "a2dp_controller.cpp"
                  )
set(COMPONENT_ADD_INCLUDEDIRS ".")

register_component()
