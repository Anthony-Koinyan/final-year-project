#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "js_timers.h"
#include "js_main_thread.h" // for js_event_queue

static const char *TAG = "JS_TIMERS";

/// @brief Pool of timers.
static js_timer_t timers[MAX_TIMERS];

/// @brief The next handle to be assigned to a new timer. Starts at 1.
static uint32_t next_handle = 1; // start at 1

/**
 * @brief The internal callback function executed by the esp_timer service.
 *
 * This function is designed to be safe to call from an ISR (Interrupt Service
 * Request) context. It creates a JS_EVENT_TIMER event containing the timer's
 * handle ID and sends it to the main JavaScript event queue for processing in
 * the JS thread.
 *
 * @param arg The argument provided to the timer, cast to the timer's handle ID.
 */
static void IRAM_ATTR timer_cb(void *arg)
{
  uint32_t handle_id = (uint32_t)(uintptr_t)arg;
  js_event_t ev = {
      .type = JS_EVENT_TIMER,
      .handle_id = handle_id,
      .data = NULL,
  };
  // From ISR context:
  BaseType_t woke = pdFALSE;
  xQueueSendFromISR(js_event_queue, &ev, &woke);
  if (woke)
    portYIELD_FROM_ISR();
}

/**
 * @brief Initializes the timer management system.
 *
 * This function iterates through the static pool of timers and marks each
 * one as not in use, effectively clearing any previous state. It should be
 * called once before any timers are set.
 */
void js_timers_init(void)
{
  for (int i = 0; i < MAX_TIMERS; i++)
  {
    timers[i].in_use = false;
  }
}

/**
 * @brief Creates and starts a new one-shot or periodic timer.
 *
 * This function finds an available slot in the timer pool, assigns it a unique
 * handle, and configures an underlying `esp_timer`. It stores the JavaScript
 * callback function for later execution.
 *
 * @param is_interval True for a periodic timer (setInterval), false for a one-shot (setTimeout).
 * @param callback The JavaScript function to be executed when the timer fires.
 * @param delay_ms The delay in milliseconds for a one-shot timer, or the interval for a periodic timer.
 * @return The unique handle ID for the new timer, or 0 if no free slots are available.
 */
uint32_t js_timers_set(bool is_interval,
                       jerry_value_t callback,
                       uint64_t delay_ms)
{
  // Find free slot
  int idx = -1;
  for (int i = 0; i < MAX_TIMERS; i++)
  {
    if (!timers[i].in_use)
    {
      idx = i;
      break;
    }
  }
  if (idx < 0)
  {
    ESP_LOGE(TAG, "No free timer slots");
    return 0;
  }

  // Grab a handle
  uint32_t handle = next_handle++;
  timers[idx].in_use = true;
  timers[idx].handle_id = handle;
  timers[idx].is_interval = is_interval;
  timers[idx].js_callback = jerry_value_copy(callback);

  // Create the esp_timer
  esp_timer_create_args_t args = {
      .callback = timer_cb,
      .arg = (void *)(uintptr_t)handle,
      .dispatch_method = ESP_TIMER_ISR, // safe for ISR context
      .name = "js_timer",
  };
  esp_timer_handle_t h;
  esp_timer_create(&args, &h);
  timers[idx].timer = h;

  // Start it
  if (is_interval)
  {
    esp_timer_start_periodic(h, delay_ms * 1000);
  }
  else
  {
    esp_timer_start_once(h, delay_ms * 1000);
  }

  return handle;
}

/**
 * @brief Stops and releases a timer.
 *
 * Finds the timer associated with the given handle, stops the `esp_timer`,
 * deletes it, frees the JerryScript callback value, and marks the slot as
 * available for reuse.
 *
 * @param handle_id The handle of the timer to clear.
 * @return True if the timer was found and cleared, false otherwise.
 */
bool js_timers_clear(uint32_t handle_id)
{
  for (int i = 0; i < MAX_TIMERS; i++)
  {
    if (timers[i].in_use && timers[i].handle_id == handle_id)
    {
      esp_timer_stop(timers[i].timer);
      esp_timer_delete(timers[i].timer);
      jerry_value_free(timers[i].js_callback);
      timers[i].in_use = false;
      return true;
    }
  }

  return false;
}

/**
 * @brief Executes the JavaScript callback for a given timer handle.
 *
 * This function is called from the main JS thread's event loop when a
 * JS_EVENT_TIMER is received. It finds the corresponding timer and invokes
 * the stored JavaScript function. For one-shot timers, it also clears the
 * timer resources after execution.
 *
 * @param handle_id The handle of the timer to dispatch.
 * @return True if the callback was successfully dispatched, false if the handle was not found.
 */
bool js_timers_dispatch(uint32_t handle_id)
{
  for (int i = 0; i < MAX_TIMERS; i++)
  {
    if (timers[i].in_use && timers[i].handle_id == handle_id)
    {
      jerry_value_t undef = jerry_undefined();
      jerry_value_t res = jerry_call(timers[i].js_callback, undef, NULL, 0);
      jerry_value_free(undef);
      print_js_error(res);
      jerry_value_free(res);

      if (!timers[i].is_interval)
      {
        js_timers_clear(handle_id);
      }
      return true;
    }
  }
  return false;
}
