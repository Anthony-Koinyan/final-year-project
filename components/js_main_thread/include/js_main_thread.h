/**
 * TODO:
 * - Create RTOS task for JS
 * - Create JS thread state opaque
 * - Create event loop
 */

#ifndef JS_MAIN_THREAD_H
#define JS_MAIN_THREAD_H

#include <stdio.h>
#include "jerryscript.h"

/**
 * @brief Prints a JerryScript error value to the log for debugging.
 */
void print_js_error(jerry_value_t error_val);

void js_task(void *);
#endif