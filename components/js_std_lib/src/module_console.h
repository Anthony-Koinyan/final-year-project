#ifndef MODULE_CONSOLE_H
#define MODULE_CONSOLE_H

#include "jerryscript.h"

/**
 * @brief Creates a new JavaScript 'console' object.
 *
 * This function acts as a factory for the console object, creating the object
 * and attaching all its methods (log, warn, error).
 *
 * @return A jerry_value_t representing the new console object. The caller is
 * responsible for releasing this value with jerry_value_free(). If an error
 * occurs during creation, the returned object may be empty.
 */
jerry_value_t create_console_object(void);

#endif
