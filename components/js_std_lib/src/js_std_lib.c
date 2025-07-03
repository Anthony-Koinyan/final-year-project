#include <string.h>
#include "esp_log.h"
#include "jerryscript.h"

#include "js_std_lib.h"
#include "module_console.h"
#include "module_gpio.h"

#define TAG "JS_STD_LIBRARY"

// --- Native Module Registry ---

/**
 * @brief Defines the structure for a native C module, including its exports.
 */
typedef struct
{
  const char *name;                              /**< The module specifier (e.g., "gpio"). */
  jerry_native_module_evaluate_cb_t evaluate_cb; /**< The callback to populate the module's exports. */
  const char **exports;                          /**< A NULL-terminated list of exported names. */
  size_t export_count;                           /**< The number of exports. */
} native_module_def_t;

// Define the lists of exported names for our native modules
const char *console_exports[] = {"log", "warn", "error"};
const char *gpio_exports[] = {"config", "reset_pin", "get_level", "set_level"};

/**
 * @brief A registry of all available native modules.
 */
static const native_module_def_t native_module_registry[] = {
    {.name = "console", .evaluate_cb = console_module_evaluate, .exports = console_exports, .export_count = 3},
    {.name = "gpio", .evaluate_cb = gpio_module_evaluate, .exports = gpio_exports, .export_count = 4}
    // Add new native modules here
};

// (bind_object_to_global and js_init_std_libs remain the same)
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
  jerry_value_t console_obj = create_console_object();
  if (bind_object_to_global(global_obj, console_obj, "console"))
  {
    ESP_LOGI(TAG, "Successfully bound 'console' object to global scope.");
  }
  jerry_value_free(console_obj);
  jerry_value_free(global_obj);
}

// --- THE FIX IS HERE ---
jerry_value_t
js_get_native_module(const jerry_value_t specifier)
{
  jerry_size_t specifier_size = jerry_string_size(specifier, JERRY_ENCODING_UTF8);
  jerry_char_t specifier_buf[specifier_size + 1];
  jerry_string_to_buffer(specifier, JERRY_ENCODING_UTF8, specifier_buf, specifier_size);
  specifier_buf[specifier_size] = '\0';

  for (size_t i = 0; i < sizeof(native_module_registry) / sizeof(native_module_def_t); i++)
  {
    if (strcmp((const char *)specifier_buf, native_module_registry[i].name) == 0)
    {
      ESP_LOGI(TAG, "Found native module in registry: %s", native_module_registry[i].name);

      const native_module_def_t *def = &native_module_registry[i];

      // Create an array of jerry_value_t strings for the export names
      jerry_value_t exports[def->export_count];
      for (size_t j = 0; j < def->export_count; j++)
      {
        exports[j] = jerry_string_sz(def->exports[j]);
      }

      // Create the native module, DECLARING its exports to the linker
      jerry_value_t native_module = jerry_native_module(def->evaluate_cb, exports, def->export_count);

      // Clean up the jerry_value_t strings
      for (size_t j = 0; j < def->export_count; j++)
      {
        jerry_value_free(exports[j]);
      }

      return native_module;
    }
  }

  return jerry_throw_sz(JERRY_ERROR_COMMON, "Module not found in native registry.");
}
