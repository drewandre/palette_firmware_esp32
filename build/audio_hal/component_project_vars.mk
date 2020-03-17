# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += /Users/drewandre/esp/esp-adf/components/audio_hal/include /Users/drewandre/esp/esp-adf/components/audio_hal/driver/include /Users/drewandre/esp/esp-adf/components/audio_hal/driver/es8388 /Users/drewandre/esp/esp-adf/components/audio_hal/driver/es8374 /Users/drewandre/esp/esp-adf/components/audio_hal/driver/es8311 /Users/drewandre/esp/esp-adf/components/audio_hal/driver/es7243 /Users/drewandre/esp/esp-adf/components/audio_hal/driver/zl38063 /Users/drewandre/esp/esp-adf/components/audio_hal/driver/zl38063/api_lib /Users/drewandre/esp/esp-adf/components/audio_hal/driver/zl38063/example_apps /Users/drewandre/esp/esp-adf/components/audio_hal/driver/zl38063/firmware
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/audio_hal -laudio_hal -L/Users/drewandre/esp/esp-adf/components/audio_hal/driver/zl38063/firmware -lfirmware
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += audio_hal
COMPONENT_LDFRAGMENTS += 
component-audio_hal-build: 
