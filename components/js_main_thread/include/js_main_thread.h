/**
 * TODO:
 * - Create RTOS task for JS
 * - Create JS thread state opaque
 * - Create event loop
 */

#ifndef JS_MAIN_THREAD_H
#define JS_MAIN_THREAD_H

#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "jerryscript.h"

/// @brief A handle to the FreeRTOS queue used for the JS event loop.
extern QueueHandle_t js_event_queue;

/**
 * @brief Prints a JerryScript error value to the log for debugging.
 *
 * This function checks if the provided jerry_value_t is an exception. If it is,
 * it converts the error object to a string and prints it to the ESP-IDF log
 * with an error level. This is useful for catching and displaying unhandled
 * exceptions from JavaScript execution.
 *
 * @param error_val The jerry_value_t that might be an error object.
 */
void print_js_error(jerry_value_t error_val);

/**
 * @brief The main task for the JavaScript runtime.
 *
 * This function initializes the JerryScript engine, sets up the event loop,
 * runs the initial `main.js` script, and then enters an infinite loop to
 * process events from the `js_event_queue`. This function should be spawned
 * as a FreeRTOS task.
 *
 * @param params Task parameters (unused).
 */
void js_task(void *);
#endif