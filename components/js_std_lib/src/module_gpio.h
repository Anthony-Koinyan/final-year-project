#ifndef MODULE_GPIO_H
#define MODULE_GPIO_H

#include "jerryscript.h"

/**
 * @brief The evaluate callback for the native 'gpio' module.
 *
 * This function is called by the JerryScript engine when the 'gpio' module is
 * first evaluated. It is responsible for populating the module's namespace
 * with the exported native functions (e.g., gpio_config, gpio_set_level).
 *
 * @param native_module The jerry_value_t representing the 'gpio' module object.
 * @return A jerry_value_t which is undefined on success, or an error.
 */
jerry_value_t gpio_module_evaluate(const jerry_value_t native_module);

#endif /* MODULE_GPIO_H */
