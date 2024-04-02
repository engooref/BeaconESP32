set (srcs)

if (CONFIG_UART)
    list (APPEND srcs "UART.cpp")
endif()


idf_component_register(SRCS "${srcs}"
                        INCLUDE_DIRS "include")
