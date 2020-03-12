deps_config := \
	/Users/drewandre/esp/esp-adf/esp-idf/components/app_trace/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/aws_iot/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/bt/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/driver/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/efuse/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/esp32/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/esp_adc_cal/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/esp_event/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/esp_http_client/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/esp_http_server/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/esp_https_ota/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/espcoredump/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/ethernet/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/fatfs/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/freemodbus/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/freertos/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/heap/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/libsodium/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/log/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/lwip/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/mbedtls/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/mdns/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/mqtt/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/nvs_flash/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/openssl/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/pthread/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/spi_flash/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/spiffs/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/tcpip_adapter/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/unity/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/vfs/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/wear_levelling/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/wifi_provisioning/Kconfig \
	/Users/drewandre/esp/esp-adf/esp-idf/components/app_update/Kconfig.projbuild \
	/Users/drewandre/esp/esp-adf/esp-idf/components/bootloader/Kconfig.projbuild \
	/Users/drewandre/esp/esp-adf/esp-idf/components/esptool_py/Kconfig.projbuild \
	/Users/drewandre/esp/esp-adf/esp-idf/examples/bluetooth/a2dp_gatts_coex/main/Kconfig.projbuild \
	/Users/drewandre/esp/esp-adf/esp-idf/components/partition_table/Kconfig.projbuild \
	/Users/drewandre/esp/esp-adf/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)

ifneq "$(IDF_TARGET)" "esp32"
include/config/auto.conf: FORCE
endif
ifneq "$(IDF_CMAKE)" "n"
include/config/auto.conf: FORCE
endif

$(deps_config): ;
