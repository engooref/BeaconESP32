set (srcs)

if (CONFIG_SOCKET)
    list (APPEND srcs "Socket.cpp")
endif()

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "include"
                    REQUIRES WiFi Kernel)
