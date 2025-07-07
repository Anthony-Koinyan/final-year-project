#include "jerryscript.h"
#include "jerryscript-ext/properties.h"

#include "js_timers.h"
#include "module_timers.h"

/**
 * @brief A simple helper to convert a jerry_value_t to a uint64_t.
 * @param v The JerryScript value, expected to be a number.
 * @return The number as a uint64_t.
 */
static uint64_t to_uint64(const jerry_value_t v)
{
  return (uint64_t)jerry_value_as_number(v);
}

/**
 * @brief Native C implementation of the JavaScript `setTimeout(callback, delay)` function.
 */
static jerry_value_t js_set_timeout(const jerry_call_info_t *call_info_p,
                                    const jerry_value_t args[],
                                    const jerry_length_t argc)
{
  if (argc < 2 || !jerry_value_is_function(args[0]) ||
      !jerry_value_is_number(args[1]))
  {
    return jerry_error_sz(JERRY_ERROR_TYPE, "setTimeout: invalid args");
  }

  uint64_t ms = to_uint64(args[1]);
  uint32_t handle = js_timers_set(false, args[0], ms);
  return jerry_number(handle);
}

/**
 * @brief Native C implementation of the JavaScript `clearTimeout(id)` function.
 */
static jerry_value_t js_clear_timeout(const jerry_call_info_t *call_info_p,
                                      const jerry_value_t args[],
                                      const jerry_length_t argc)
{
  if (argc < 1 || !jerry_value_is_number(args[0]))
  {
    return jerry_error_sz(JERRY_ERROR_TYPE, "clearTimeout: invalid args");
  }

  js_timers_clear((uint32_t)jerry_value_as_number(args[0]));
  return jerry_undefined();
}

/**
 * @brief Native C implementation of the JavaScript `setInterval(callback, delay)` function.
 */
static jerry_value_t js_set_interval(const jerry_call_info_t *call_info_p,
                                     const jerry_value_t args[],
                                     const jerry_length_t argc)
{
  if (argc < 2 || !jerry_value_is_function(args[0]) ||
      !jerry_value_is_number(args[1]))
  {
    return jerry_error_sz(JERRY_ERROR_TYPE, "setInterval: invalid args");
  }

  uint64_t ms = to_uint64(args[1]);
  uint32_t handle = js_timers_set(true, args[0], ms);
  return jerry_number(handle);
}

/**
 * @brief Native C implementation of the JavaScript `clearInterval(id)` function.
 *
 * @note This reuses the same logic as `clearTimeout`.
 */
static jerry_value_t js_clear_interval(const jerry_call_info_t *call_info_p,
                                       const jerry_value_t args[],
                                       const jerry_length_t argc)
{
  return js_clear_timeout(call_info_p, args, argc);
}

/**
 * @brief Binds timer functions to the JavaScript global object.
 *
 * This uses the `jerryx_set_properties` helper to efficiently add `setTimeout`,
 * `clearTimeout`, `setInterval`, and `clearInterval` to a given object,
 * typically the global object.
 *
 * @param global The JavaScript global object.
 */
void timers_bind_global(jerry_value_t global)
{
  jerryx_property_entry props[] = {
      JERRYX_PROPERTY_FUNCTION("setTimeout", js_set_timeout),
      JERRYX_PROPERTY_FUNCTION("clearTimeout", js_clear_timeout),
      JERRYX_PROPERTY_FUNCTION("setInterval", js_set_interval),
      JERRYX_PROPERTY_FUNCTION("clearInterval", js_clear_interval),
      JERRYX_PROPERTY_LIST_END()};

  jerryx_set_properties(global, props);
}

/**
 * @brief Populates the exports for the 'timers' native module.
 *
 * This function is called by the JerryScript engine when `import ... from 'timers'`
 * is evaluated. It attaches the timer functions to the module's export object.
 *
 * @param native_module The `jerry_value_t` for the module being built.
 * @return `jerry_undefined()` on success.
 */
jerry_value_t timers_module_evaluate(const jerry_value_t native_module)
{
  // Create function and name values
  jerry_value_t set_timeout_func = jerry_function_external(js_set_timeout);
  jerry_value_t clear_timeout_func = jerry_function_external(js_clear_timeout);
  jerry_value_t set_interval_func = jerry_function_external(js_set_interval);
  jerry_value_t clear_interval_func = jerry_function_external(js_clear_interval);
  jerry_value_t set_timeout_name = jerry_string_sz("setTimeout");
  jerry_value_t clear_timeout_name = jerry_string_sz("clearTimeout");
  jerry_value_t set_interval_name = jerry_string_sz("setInterval");
  jerry_value_t clear_interval_name = jerry_string_sz("clearInterval");

  // Set the exports on the module
  jerry_native_module_set(native_module, set_timeout_name, set_timeout_func);
  jerry_native_module_set(native_module, clear_timeout_name, clear_timeout_func);
  jerry_native_module_set(native_module, set_interval_name, set_interval_func);
  jerry_native_module_set(native_module, clear_interval_name, clear_interval_func);

  // Clean up
  jerry_value_free(set_timeout_func);
  jerry_value_free(clear_timeout_func);
  jerry_value_free(set_interval_func);
  jerry_value_free(clear_interval_func);
  jerry_value_free(set_timeout_name);
  jerry_value_free(clear_timeout_name);
  jerry_value_free(set_interval_name);
  jerry_value_free(clear_interval_name);

  return jerry_undefined();
}
