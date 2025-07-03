idf_component_register(SRCS "src/js_std_lib.c" "src/module_console.c" "src/module_gpio.c"
                    INCLUDE_DIRS "include"
                    PRIV_INCLUDE_DIRS "src"
                    REQUIRES "jerryscript" "js_module_resolver" "driver")
