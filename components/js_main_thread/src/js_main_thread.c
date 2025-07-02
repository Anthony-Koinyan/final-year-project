#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "jerryscript.h"

#include "js_std_lib.h"
#include "js_module_resolver.h"
#include "js_main_thread.h"

#define TAG "JS_THREAD"
#define MAX_LOG_LENGTH 64

/**
 * @brief Prints a JerryScript error value to the log for debugging.
 */
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

void js_task(void *params)
{
  // 1. Initialise JerryScript engine
  jerry_init(JERRY_INIT_EMPTY);

  // 3. Initialise and bind standard libraries (like global 'console').
  js_init_std_libs();

  // 4. Start the application by resolving and running the main.js module.
  ESP_LOGI(TAG, "Starting execution of main.js module...");
  js_run_main_module();

  // 5. Idle loop
  ESP_LOGI(TAG, "Main module finished. Entering idle loop.");
  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Final cleanup (will not be reached in the current loop)
  jerry_cleanup();
  vTaskDelete(NULL);
}
