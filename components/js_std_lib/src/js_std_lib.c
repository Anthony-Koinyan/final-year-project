#include "esp_log.h"

#include "jerryscript.h"
#include "jerryscript-ext/module.h"

#include "js_std_lib.h"
#include "module_console.h"

#define TAG "JS_STD_LIBRARY"

/**
 * @brief This function is called by JerryScript when `import 'console'` is evaluated.
 *
 * It creates the console object with its functions (log, warn, error).
 *
 * @return A jerry_value_t representing the module's exports (the console object).
 */
static jerry_value_t console_module_on_resolve(void)
{
  // Create the object that will be the "export" of this module
  jerry_value_t console_obj = jerry_object();

  // Get the function definitions from our console implementation
  size_t function_count = 0;
  const js_native_function_def_t *functions = console_module_get_functions(&function_count);

  // Attach each function (log, warn, error) to the console object
  for (size_t i = 0; i < function_count; ++i)
  {
    jerry_value_t func_name = jerry_string_sz(functions[i].name);
    jerry_value_t func_obj = jerry_function_external(functions[i].handler);
    jerry_object_set(console_obj, func_name, func_obj);
    jerry_value_free(func_name);
    jerry_value_free(func_obj);
  }

  return console_obj;
}

// This is the magic macro from JerryScript's module extension.
// It registers our 'console' module and links it to the on_resolve callback.
// NOTE: No semicolon at the end!
JERRYX_NATIVE_MODULE(console, console_module_on_resolve)

/**
 * @brief Initializes all standard native modules.
 *
 * This function must be called BEFORE the JerryScript engine starts
 * resolving any modules.
 */
void register_js_std_lib(void)
{
  // This calls the registration function created by the JERRYX_NATIVE_MODULE macro.
  // The function name is derived from the module name: `console_register`.
  console_register();
  ESP_LOGI("NATIVE_MODULE", "Registered 'console' module.");

  // When you create a 'gpio' module, you'll add 'gpio_register();' here.
}
