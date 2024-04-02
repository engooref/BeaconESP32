idf_component_register(SRCS "Sandglass.cpp" "Kernel.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES spiffs Gpio Socket UART I2C)

If(CONFIG_SPIFFS)
    spiffs_create_partition_image(storage "../../Data" FLASH_IN_PROJECT)
endif()