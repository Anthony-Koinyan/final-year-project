idf_component_register(SRCS "src/js_main_thread.c" "src/js_timers.c" "src/js_gpio.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "src"
                    REQUIRES "freertos" "jerryscript" "esp_timer"
                    PRIV_REQUIRES "js_std_lib")