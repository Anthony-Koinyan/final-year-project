#ifndef MODULE_CONSOLE_H
#define MODULE_CONSOLE_H

#include "jerryscript.h"

/**
 * @brief Creates a new JavaScript 'console' object.
 *
 * This function acts as a factory for the console object, creating the object
 * and attaching all its methods (log, warn, error). This is used for binding
 * to the global object.
 *
 * @return A jerry_value_t representing the new console object. The caller is
 * responsible for releasing this value with jerry_value_free().
 */
jerry_value_t create_console_object(void);

/**
 * @brief The evaluate callback for the native 'console' module.
 *
 * This function is called by the JerryScript engine when the 'console' module is
 * first evaluated. It populates the module's namespace with the console functions.
 *
 * @param native_module The jerry_value_t representing the 'console' module object.
 * @return A jerry_value_t which is undefined on success, or an error.
 */
jerry_value_t console_module_evaluate(const jerry_value_t native_module);

#endif /* MODULE_CONSOLE_H */
