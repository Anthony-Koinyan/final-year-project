#include <string.h>

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

  // 2. Register all native C modules (like 'console', 'gpio', etc.)
  // This must be done before any JS code is run.
  register_js_std_lib();

  // 3. Start the application by resolving and running the main.js module
  ESP_LOGI(TAG, "Starting execution of main.js module...");
  js_run_main();

  // 4. TODO: Implement an event loop here for async operations.
  // For now, the task will finish after main.js is done.
  ESP_LOGI(TAG, "Main script finished. Entering idle loop.");
  while (1)
  {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }

  // Final cleanup (will not be reached in the current loop)
  ESP_LOGI(TAG, "Task finished. Cleaning up JerryScript engine.");
  jerry_cleanup();

  ESP_LOGI(TAG, "Deleting task.");
  vTaskDelete(NULL);
}
