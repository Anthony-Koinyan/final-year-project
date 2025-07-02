#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "jerryscript.h"

#include "js_std_lib.h"
#include "js_module_resolver.h"
#include "js_main_thread.h"

#define TAG "JS_THREAD"

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
