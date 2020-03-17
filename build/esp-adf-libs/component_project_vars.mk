# Automatically generated build file. Do not edit.
COMPONENT_INCLUDES += /Users/drewandre/esp/esp-adf/components/esp-adf-libs/esp_audio/include /Users/drewandre/esp/esp-adf/components/esp-adf-libs/esp_codec/include/codec /Users/drewandre/esp/esp-adf/components/esp-adf-libs/esp_codec/include/processing /Users/drewandre/esp/esp-adf/components/esp-adf-libs/recorder_engine/include /Users/drewandre/esp/esp-adf/components/esp-adf-libs/esp_ssdp/include /Users/drewandre/esp/esp-adf/components/esp-adf-libs/esp_upnp/include /Users/drewandre/esp/esp-adf/components/esp-adf-libs/esp_sip/include /Users/drewandre/esp/esp-adf/components/esp-adf-libs/audio_misc/include
COMPONENT_LDFLAGS += -L$(BUILD_DIR_BASE)/esp-adf-libs -lesp-adf-libs -L/Users/drewandre/esp/esp-adf/components/esp-adf-libs/esp_audio/lib -L/Users/drewandre/esp/esp-adf/components/esp-adf-libs/esp_codec/lib -L/Users/drewandre/esp/esp-adf/components/esp-adf-libs/recorder_engine/lib -L/Users/drewandre/esp/esp-adf/components/esp-adf-libs/esp_ssdp/lib -L/Users/drewandre/esp/esp-adf/components/esp-adf-libs/esp_upnp/lib -L/Users/drewandre/esp/esp-adf/components/esp-adf-libs/esp_sip/lib -lesp_processing -lesp_audio -lesp-amr -lesp-amrwbenc -lesp-aac -lesp-ogg-container -lesp-opus -lesp-tremor -lesp-flac -lesp_ssdp -lesp_upnp -lesp_sip -lesp-mp3 -lcodec-utils -lrecorder_engine 
COMPONENT_LINKER_DEPS += 
COMPONENT_SUBMODULES += 
COMPONENT_LIBRARIES += esp-adf-libs
COMPONENT_LDFRAGMENTS += 
component-esp-adf-libs-build: 
