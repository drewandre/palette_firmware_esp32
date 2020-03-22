# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += $(PROJECT_PATH)/components/arduino/cores/esp32 $(PROJECT_PATH)/components/arduino/variants/esp32 $(PROJECT_PATH)/components/arduino/libraries/ArduinoOTA/src $(PROJECT_PATH)/components/arduino/libraries/DNSServer/src $(PROJECT_PATH)/components/arduino/libraries/ESPmDNS/src $(PROJECT_PATH)/components/arduino/libraries/FS/src $(PROJECT_PATH)/components/arduino/libraries/HTTPClient/src $(PROJECT_PATH)/components/arduino/libraries/Update/src $(PROJECT_PATH)/components/arduino/libraries/WebServer/src $(PROJECT_PATH)/components/arduino/libraries/WebServer/src/detail $(PROJECT_PATH)/components/arduino/libraries/WebServer/src/uri $(PROJECT_PATH)/components/arduino/libraries/WiFi/src $(PROJECT_PATH)/components/arduino/libraries/WiFiClientSecure/src
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/arduino -larduino
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += arduino
COMPONENT_LDFRAGMENTS += 
component-arduino-build: 
