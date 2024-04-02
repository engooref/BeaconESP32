set (srcs)

if (CONFIG_SOCKET)
    list (APPEND srcs "WiFi.cpp")
endif()

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "." "include"
                    REQUIRES nvs_flash log ClaJSON)
