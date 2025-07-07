#ifndef JS_EVENT_H
#define JS_EVENT_H
#include <stdint.h>

/**
 * @brief Defines the types of events that can be posted to the main JS event queue.
 *
 * This enumeration allows the event loop to distinguish between different
 * sources of asynchronous events, such as timers, GPIO interrupts, etc.
 */
typedef enum
{
  JS_EVENT_TIMER, /**< An event originating from a timer created with setTimeout or setInterval. */
  JS_EVENT_GPIO,  /**< An event originating from a GPIO interrupt. */
  // later: JS_EVENT_HTTP, JS_EVENT_ADC, etc.
} js_event_type_t;

/**
 * @brief Represents a single event to be processed by the JavaScript event loop.
 *
 * This struct is the basic unit of communication between native asynchronous
 * operations (like timers or hardware interrupts) and the main JavaScript thread.
 */
typedef struct
{
  js_event_type_t type; /**< The category of the event. */
  uint32_t handle_id;   /**< A unique ID to identify the source of the event (e.g., which timer fired). */
  void *data;           /**< An optional payload carrying extra data for the event. NULL for timers. */
} js_event_t;
#endif