idf_component_register(SRCS "main.c"
                       PRIV_REQUIRES spi_flash esp_driver_ledc esp_timer esp_driver_ppa esp_driver_i2c esp_driver_usb_serial_jtag lvgl
                       INCLUDE_DIRS "")
