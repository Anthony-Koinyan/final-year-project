#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "js_timers.h"
#include "js_main_thread.h" // for js_event_queue and print_js_error

static const char *TAG = "JS_TIMERS";

/// @brief The head of the linked list of active timers.
static js_timer_t *timers_head = NULL;

/// @brief The next handle to be assigned to a new timer. Starts at 1.
static uint32_t next_handle = 1;

/**
 * @brief The internal callback function executed by the esp_timer service.
 */
static void IRAM_ATTR timer_cb(void *arg)
{
  uint32_t handle_id = (uint32_t)(uintptr_t)arg;
  js_event_t ev = {
      .type = JS_EVENT_TIMER,
      .handle_id = handle_id,
      .data = NULL,
  };
  BaseType_t woke = pdFALSE;
  xQueueSendFromISR(js_event_queue, &ev, &woke);
  if (woke)
  {
    portYIELD_FROM_ISR();
  }
}

/**
 * @brief Initializes the timer management system.
 */
void js_timers_init(void)
{
  ESP_LOGI(TAG, "Initializing timer system.");
  timers_head = NULL;
}

/**
 * @brief Creates and starts a new timer.
 */
uint32_t js_timers_set(bool is_interval, jerry_value_t callback, uint64_t delay_ms)
{
  js_timer_t *new_timer = (js_timer_t *)malloc(sizeof(js_timer_t));
  if (new_timer == NULL)
  {
    ESP_LOGE(TAG, "Failed to allocate memory for a new timer.");
    return 0;
  }

  uint32_t handle = next_handle++;
  new_timer->handle_id = handle;
  new_timer->is_interval = is_interval;
  new_timer->js_callback = jerry_value_copy(callback);

  esp_timer_create_args_t args = {
      .callback = timer_cb,
      .arg = (void *)(uintptr_t)handle,
      .dispatch_method = ESP_TIMER_ISR,
      .name = "js_timer",
  };
  esp_err_t err = esp_timer_create(&args, &new_timer->timer);
  if (err != ESP_OK)
  {
    ESP_LOGE(TAG, "Failed to create esp_timer: %s", esp_err_to_name(err));
    jerry_value_free(new_timer->js_callback);
    free(new_timer);
    return 0;
  }

  // Add the new timer to the front of the list
  new_timer->next = timers_head;
  timers_head = new_timer;

  if (is_interval)
  {
    esp_timer_start_periodic(new_timer->timer, delay_ms * 1000);
  }
  else
  {
    esp_timer_start_once(new_timer->timer, delay_ms * 1000);
  }

  // NOTE: Removed high-frequency logging from here to improve performance.
  // ESP_LOGI(TAG, "Timer SET with handle %lu. is_interval=%d", handle, is_interval);

  return handle;
}

/**
 * @brief Stops and releases a timer.
 */
bool js_timers_clear(uint32_t handle_id)
{
  js_timer_t *current = timers_head;
  js_timer_t *prev = NULL;

  while (current != NULL && current->handle_id != handle_id)
  {
    prev = current;
    current = current->next;
  }

  if (current == NULL)
  {
    // This can happen if a timer is cleared that was already cleared. Not an error.
    ESP_LOGD(TAG, "Timer CLEAR failed: handle %lu not found.", handle_id);
    return false;
  }

  if (prev == NULL)
  {
    timers_head = current->next;
  }
  else
  {
    prev->next = current->next;
  }

  esp_timer_stop(current->timer);
  esp_timer_delete(current->timer);
  jerry_value_free(current->js_callback);
  free(current);

  // NOTE: Removed high-frequency logging from here to improve performance.
  // ESP_LOGI(TAG, "Timer CLEARED with handle %lu.", handle_id);
  return true;
}

/**
 * @brief Executes the JavaScript callback for a given timer handle.
 */
bool js_timers_dispatch(uint32_t handle_id)
{
  js_timer_t *timer = timers_head;
  while (timer != NULL && timer->handle_id != handle_id)
  {
    timer = timer->next;
  }

  if (timer != NULL)
  {
    bool is_interval = timer->is_interval;
    jerry_value_t callback = timer->js_callback;

    jerry_value_t global = jerry_current_realm();
    jerry_value_t res = jerry_call(callback, global, NULL, 0);
    jerry_value_free(global);

    if (jerry_value_is_exception(res))
    {
      print_js_error(res);
    }
    jerry_value_free(res);

    if (!is_interval)
    {
      js_timers_clear(handle_id);
    }
    return true;
  }

  // This is not an error. It can happen if a timer is cleared
  // after its event has been queued but before it has been dispatched.
  ESP_LOGD(TAG, "Timer DISPATCH ignored: handle %lu already cleared.", handle_id);
  return false;
}
