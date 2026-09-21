idf_component_register(SRCS "app_main.c"
                            "cloud_provision.c"
                       PRIV_REQUIRES
                            spi_flash
                            esp_psram
                            esp_pm
                            esp_app_format
                            esp_hw_support
                            heap
                            nvs_flash
                            BSP
                            platform
                            app_state
                            services
                            contract_checks
                       INCLUDE_DIRS "")

idf_component_optional_requires(PRIVATE unity)

spiffs_create_partition_image(spiffs ${CMAKE_CURRENT_LIST_DIR}/../spiffs FLASH_IN_PROJECT)
