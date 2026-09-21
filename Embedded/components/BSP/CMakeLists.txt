set(src_dirs
            BOARD
            BSP_INCLUDE
            CO5300
            CST9217
            ES8311
            KEY
            L76KB_A58
            CW2015
            ML307R
            NS4150
            QMI8658C
            VIBRATION)

set(include_dirs
            BSP_INCLUDE
            CO5300
            CST9217
            ES8311
            KEY
            L76KB_A58
            CW2015
            ML307R
            NS4150
            QMI8658C
            VIBRATION)

set(requires
            driver
            esp_driver_gpio
            esp_driver_i2c
            esp_driver_i2s
            esp_driver_ledc
            esp_pm
            espressif__esp_codec_dev
            esp_lcd
            esp_lcd_co5300
            esp_lcd_touch
            esp_lcd_touch_cst9217)

idf_component_register(
        SRC_DIRS ${src_dirs}
        INCLUDE_DIRS ${include_dirs}
        REQUIRES ${requires}
        PRIV_REQUIRES esp_timer)
