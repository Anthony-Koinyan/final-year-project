#ifndef MODULE_CONSOLE_H
#define MODULE_CONSOLE_H

#include "jerryscript.h"

/**
 * @brief Creates a complete JavaScript `console` object and binds it to the
 * global scope, making `console.log` available everywhere without an import.
 */
void console_bind_global(jerry_value_t);

/**
 * @brief The evaluate callback for the native 'console' module.
 *
 * This function is called by the JerryScript engine when the 'console' module is
 * first evaluated. It populates the module's namespace with the console functions.
 *
 * @param native_module The jerry_value_t representing the 'console' module object.
 * @return A jerry_value_t which is undefined on success, or an error.
 */
jerry_value_t console_module_evaluate(const jerry_value_t);

#endif /* MODULE_CONSOLE_H */
