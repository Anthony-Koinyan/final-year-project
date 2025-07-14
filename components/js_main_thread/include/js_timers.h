#ifndef JS_TIMERS_H
#define JS_TIMERS_H

#include <stdbool.h>
#include <stdlib.h> // Required for malloc and free
#include "esp_timer.h"
#include "jerryscript.h"
#include "js_event.h"

/**
 * @brief Represents a single timer node in a linked list.
 */
typedef struct js_timer_t
{
  uint32_t handle_id;
  bool is_interval;
  esp_timer_handle_t timer;
  jerry_value_t js_callback;
  struct js_timer_t *next; // Pointer to the next timer in the list
} js_timer_t;

/**
 * @brief Initializes the timer management system.
 */
void js_timers_init(void);

/**
 * @brief Creates and starts a new timer, adding it to the linked list.
 * @return The handle ID of the new timer, or 0 on failure.
 */
uint32_t js_timers_set(bool is_interval, jerry_value_t callback, uint64_t delay_ms);

/**
 * @brief Stops, removes, and frees a timer from the linked list.
 * @return True if the timer was found and cleared, false otherwise.
 */
bool js_timers_clear(uint32_t handle_id);

/**
 * @brief Dispatches a timer event by finding it in the linked list and executing its callback.
 * @return True if the timer was found and dispatched, false otherwise.
 */
bool js_timers_dispatch(uint32_t handle_id);

#endif /* JS_TIMERS_H */