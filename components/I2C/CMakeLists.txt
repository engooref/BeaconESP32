set (srcs)

if (CONFIG_I2C)
    list (APPEND srcs "I2C.cpp")
endif()


idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "include")
