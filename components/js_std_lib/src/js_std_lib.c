#include "esp_log.h"
#include "jerryscript.h"
#include "js_std_lib.h"
#include "module_console.h" // For create_console_object()

#define TAG "JS_STD_LIBRARY"

/**
 * @brief Binds a given native object to the global scope with a given name.
 *
 * @param global_obj The JerryScript global object.
 * @param object_to_bind The native JavaScript object to bind.
 * @param name The name to bind the object to.
 * @return true on success, false on failure.
 */
static bool bind_object_to_global(jerry_value_t global_obj, jerry_value_t object_to_bind, const char *name)
{
  jerry_value_t object_name = jerry_string_sz(name);
  jerry_value_t set_result = jerry_object_set(global_obj, object_name, object_to_bind);
  bool success = !jerry_value_is_exception(set_result);

  if (!success)
  {
    ESP_LOGE(TAG, "Failed to bind '%s' object to global scope.", name);
  }

  jerry_value_free(set_result);
  jerry_value_free(object_name);
  return success;
}

void js_init_std_libs(void)
{
  jerry_value_t global_obj = jerry_current_realm();

  // --- Bind Console Object ---
  jerry_value_t console_obj = create_console_object();
  if (bind_object_to_global(global_obj, console_obj, "console"))
  {
    ESP_LOGI(TAG, "Successfully bound 'console' object to global scope.");
  }
  jerry_value_free(console_obj);

  // --- Add other native objects here in the future ---
  // e.g., jerry_value_t gpio_obj = create_gpio_object();
  //      bind_object_to_global(global_obj, gpio_obj, "gpio");
  //      jerry_value_free(gpio_obj);

  jerry_value_free(global_obj);
}
