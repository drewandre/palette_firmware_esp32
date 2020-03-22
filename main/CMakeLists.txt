set(COMPONENT_SRCS "main.cpp" "fft_controller.c" "bt_app_av.c"
                   "bt_app_core.c"
                  )
set(COMPONENT_ADD_INCLUDEDIRS ".")

set(COMPONENT_EMBED_TXTFILES
        "certs/cacert.pem"
        "certs/prvtkey.pem")

register_component()
