#ifndef MODULE_CONSOLE_H
#define MODULE_CONSOLE_H

#include "jerryscript.h"
#include "js_std_lib.h"

/**
 * @brief Gets the list of native console functions.
 *
 * @param count Pointer to a size_t variable that will be filled with the
 * number of functions in the returned array.
 * @return A pointer to a constant array of native function definitions.
 */
const js_native_function_def_t *console_module_get_functions(size_t *count);

#endif