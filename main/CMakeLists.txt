set(COMPONENT_SRCS "MyWiFi.c" "OTAServer.c" "main.cpp" "fft_controller.c" "bt_app_av.c"
                   "bt_app_core.c"
                  )
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_EMBED_TXTFILES ${IDF_PROJECT_PATH}/server_certs/ca_cert.pem)

register_component()
