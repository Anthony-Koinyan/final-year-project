#include <string.h>

#include "freertos/FreeRTOS.h"
#include "esp_log.h"

#include "jerryscript.h"

#include "js_std_lib.h"
#include "js_main_thread.h"

void js_task(void *params)
{
  // initialise jerryscript engine
  jerry_init(JERRY_INIT_EMPTY);

  // load standard library
  jerry_value_t global_this = jerry_current_realm(); // get global this object
  init_js_std_lib(global_this);

  // run scripts
  js_file_t *file = (js_file_t *)params;
  js_file_t *main_file = NULL;

  ESP_LOGI("JS", "filename: %s", file->name);
  // read user js code
  for (js_file_t *current_file = file; current_file->name != NULL; current_file++)
  {
    if (strcmp(current_file->name, "main.js") == 0)
    {
      main_file = current_file;
      // We found main.js continue searching for others.
      ESP_LOGI("JS", "Found main.js");
      continue;
    }

    // TODO: load other files as es modules
    ESP_LOGI("JS", "Found other file: %s (not implemented)", current_file->name);
  }

  // parse and run main.js
  if (main_file)
  {
    ESP_LOGI("JS", "Evaluating main: %s", main_file->name);
    jerry_value_t code = jerry_parse((jerry_char_t *)main_file->buffer, main_file->size, JERRY_PARSE_NO_OPTS);

    ESP_LOGI("JS", "Freeing script buffer memory.");
    free(main_file->buffer);

    if (!jerry_value_is_exception(code))
    {
      jerry_value_t result = jerry_run(code);
      jerry_value_free(result);
    }
    else
    {
      ESP_LOGE("JS", "Error parsing main.js");
    }

    jerry_value_free(code);
  }

  // TODO: add event loop

  // final cleanup
  ESP_LOGI("JS", "Task finished. Cleaning up JerryScript engine.");
  jerry_cleanup();

  ESP_LOGI("JS", "Deleting task.");
  vTaskDelete(NULL); // This is the last call. It deletes the current task.
}
