#ifndef JS_STD_LIB_H
#define JS_STD_LIB_H

#include "jerryscript.h"

/**
 * @brief Initialises and binds standard libraries to the global object.
 * Currently, this only binds the 'console' object for convenience.
 */
void js_init_std_libs(void);

/**
 * @brief Attempts to find and create a native module by its specifier name.
 *
 * This function searches a predefined registry of native modules. If a module
 * with the given specifier is found, it constructs a new jerry_value_t native
 * module object and returns it.
 *
 * @param specifier The name of the module to find (e.g., "gpio").
 * @return A new jerry_value_t native module on success.
 * An exception value if the module is not found in the native registry.
 */
jerry_value_t js_get_native_module(const jerry_value_t specifier);

#endif /* JS_STD_LIB_H */
