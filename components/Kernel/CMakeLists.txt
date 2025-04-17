idf_component_register(SRCS "Sandglass.cpp" "Kernel.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES spiffs Gpio UART I2C driver Bluetooth)

If(CONFIG_SPIFFS)
    spiffs_create_partition_image(storage "../../Data" FLASH_IN_PROJECT)
endif()