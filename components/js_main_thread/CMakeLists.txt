idf_component_register(SRCS "src/js_main_thread.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "src"
                    REQUIRES "freertos"
                    PRIV_REQUIRES "jerryscript" "js_std_lib")