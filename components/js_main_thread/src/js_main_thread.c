#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "jerryscript.h"
#include "esp_log.h"

#include "js_main_thread.h"
#include "js_std_lib.h"
#include "js_module_resolver.h"
#include "js_event.h"
#include "js_timers.h"
#include "js_gpio.h"

#define TAG "JS_THREAD"
#define MAX_LOG_LENGTH 64

QueueHandle_t js_event_queue = NULL;

void print_js_error(jerry_value_t error_val)
{
  if (!jerry_value_is_exception(error_val))
  {
    return;
  }

  jerry_value_t err_str_val = jerry_value_to_string(jerry_exception_value(error_val, false));
  jerry_size_t err_str_size = jerry_string_size(err_str_val, JERRY_ENCODING_UTF8);

  // Allocate a buffer on the stack, but handle potentially long error messages
  char err_str_buf[MAX_LOG_LENGTH];
  jerry_size_t copy_size = (err_str_size < MAX_LOG_LENGTH - 1) ? err_str_size : MAX_LOG_LENGTH - 1;

  jerry_string_to_buffer(err_str_val, JERRY_ENCODING_UTF8, (jerry_char_t *)err_str_buf, copy_size);
  err_str_buf[copy_size] = '\0';

  ESP_LOGE("Unhandled Exception", "%s", err_str_buf);

  jerry_value_free(err_str_val);
}

static void js_dispatch_event(const js_event_t *event)
{
  switch (event->type)
  {
  case JS_EVENT_TIMER:

    if (!js_timers_dispatch(event->handle_id))
    {
      ESP_LOGW(TAG, "Unknown timer handle %lu", event->handle_id);
    }
    break;

  case JS_EVENT_GPIO:
    ESP_LOGD(TAG, "[EVENT] GPIO event for pin %lu", event->handle_id);
    js_gpio_dispatch_event((js_event_t *)event); // Forward to the GPIO module's dispatcher
    break;
    break;

  default:
    ESP_LOGW(TAG, "[EVENT] Unknown type=%d", event->type);
    break;
  }
}

void js_task(void *params)
{
  // 1. Initialise JerryScript engine
  jerry_init(JERRY_INIT_EMPTY);

  // 2. Initialise and bind standard libraries (like global 'console').
  js_init_std_libs();

  // 3. Initialise timers
  js_timers_init();

  // 4. Create a queue that can hold up to 8 events
  js_event_queue = xQueueCreate(8, sizeof(js_event_t));
  if (js_event_queue == NULL)
  {
    ESP_LOGE(TAG, "Failed to create JS event queue");
    vTaskDelete(NULL);
  }

  // 5. Start the application by resolving and running the main.js module.
  ESP_LOGI(TAG, "Starting execution of main.js module...");
  js_run_main_module();

  // 6. Event-driven loop
  ESP_LOGI(TAG, "Main module finished. Entering event loop.");
  js_event_t event;
  while (1)
  {
    // Block indefinitely until an event arrives
    if (xQueueReceive(js_event_queue, &event, portMAX_DELAY) == pdTRUE)
    {
      js_dispatch_event(&event);
    }

    // Run promises:
    jerry_run_jobs();
  }

  // Final cleanup (will not be reached in the current loop)
  jerry_cleanup();
  vTaskDelete(NULL);
}
