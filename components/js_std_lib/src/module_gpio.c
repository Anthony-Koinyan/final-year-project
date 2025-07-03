#include "jerryscript.h"
#include "jerryscript-ext/properties.h"
#include "jerryscript-ext/arg.h" // <-- Include the correct header
#include "driver/gpio.h"
#include "esp_log.h"

#define TAG "GPIO_MODULE"

// --- JerryScript C Handlers for GPIO functions ---

/**
 * @brief JS-callable function for `gpio.config()`.
 * Expects one argument: a configuration object.
 * e.g., gpio.config({ pin_bit_mask: (1 << 2), mode: 2, ... })
 */
static jerry_value_t
js_gpio_config_handler(const jerry_call_info_t *call_info_p,
                       const jerry_value_t args[],
                       const jerry_length_t argc)
{
  // --- Use jerryx-arg to safely extract properties from the JS object ---

  double pin_bit_mask, mode, pull_up_en, pull_down_en, intr_type;

  // 1. Define the names of the properties we expect in the object.
  const char *prop_names[] = {"pin_bit_mask", "mode", "pull_up_en", "pull_down_en", "intr_type"};

  // 2. Define the C variables where the extracted property values will be stored.
  const jerryx_arg_t prop_mapping[] = {
      jerryx_arg_number(&pin_bit_mask, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED),
      jerryx_arg_number(&mode, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED),
      jerryx_arg_number(&pull_up_en, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED),
      jerryx_arg_number(&pull_down_en, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED),
      jerryx_arg_number(&intr_type, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED)};

  // 3. Bundle the property names and their mappings into a single definition.
  const jerryx_arg_object_props_t props_info = {
      .name_p = (const jerry_char_t **)prop_names,
      .name_cnt = 5,
      .c_arg_p = prop_mapping,
      .c_arg_cnt = 5};

  // 4. Define the overall argument mapping for the function call itself.
  // We expect one argument, and it must be an object matching the definition above.
  const jerryx_arg_t main_arg_mapping[] = {
      jerryx_arg_object_properties(&props_info, JERRYX_ARG_REQUIRED)};

  // 5. Perform the validation and transformation.
  jerry_value_t result = jerryx_arg_transform_args(args, argc, main_arg_mapping, 1);

  if (jerry_value_is_exception(result))
  {
    // The jerryx-arg helper already created an appropriate error object.
    return result;
  }
  jerry_value_free(result);
  // --- End of property extraction ---

  gpio_config_t io_conf = {
      .pin_bit_mask = (uint64_t)pin_bit_mask,
      .mode = (gpio_mode_t)mode,
      .pull_up_en = (gpio_pullup_t)pull_up_en,
      .pull_down_en = (gpio_pulldown_t)pull_down_en,
      .intr_type = (gpio_int_type_t)intr_type,
  };

  esp_err_t err = gpio_config(&io_conf);

  if (err != ESP_OK)
  {
    return jerry_throw_sz(JERRY_ERROR_COMMON, "Failed to configure GPIO.");
  }

  return jerry_undefined();
}

/**
 * @brief JS-callable function for `gpio.reset_pin()`.
 * Expects one argument: the GPIO number.
 */
static jerry_value_t
js_gpio_reset_pin_handler(const jerry_call_info_t *call_info_p,
                          const jerry_value_t args[],
                          const jerry_length_t argc)
{
  double pin_num;
  const jerryx_arg_t mapping[] = {
      jerryx_arg_number(&pin_num, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED)};

  jerry_value_t result = jerryx_arg_transform_args(args, argc, mapping, 1);
  if (jerry_value_is_exception(result))
  {
    return result;
  }
  jerry_value_free(result);

  esp_err_t err = gpio_reset_pin((gpio_num_t)pin_num);
  if (err != ESP_OK)
  {
    return jerry_throw_sz(JERRY_ERROR_COMMON, "Failed to reset GPIO pin.");
  }

  return jerry_undefined();
}

/**
 * @brief JS-callable function for `gpio.get_level()`.
 * Expects one argument: the GPIO number. Returns 0 or 1.
 */
static jerry_value_t
js_gpio_get_level_handler(const jerry_call_info_t *call_info_p,
                          const jerry_value_t args[],
                          const jerry_length_t argc)
{
  double pin_num;
  const jerryx_arg_t mapping[] = {
      jerryx_arg_number(&pin_num, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED)};

  jerry_value_t result = jerryx_arg_transform_args(args, argc, mapping, 1);
  if (jerry_value_is_exception(result))
  {
    return result;
  }
  jerry_value_free(result);

  int level = gpio_get_level((gpio_num_t)pin_num);

  return jerry_number(level);
}

/**
 * @brief JS-callable function for `gpio.set_level()`.
 * Expects two arguments: the GPIO number and the level (0 or 1).
 */
static jerry_value_t
js_gpio_set_level_handler(const jerry_call_info_t *call_info_p,
                          const jerry_value_t args[],
                          const jerry_length_t argc)
{
  double pin_num, level;
  const jerryx_arg_t mapping[] = {
      jerryx_arg_number(&pin_num, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED),
      jerryx_arg_number(&level, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED)};

  jerry_value_t result = jerryx_arg_transform_args(args, argc, mapping, 2);
  if (jerry_value_is_exception(result))
  {
    return result;
  }
  jerry_value_free(result);

  esp_err_t err = gpio_set_level((gpio_num_t)pin_num, (uint32_t)level);
  if (err != ESP_OK)
  {
    return jerry_throw_sz(JERRY_ERROR_COMMON, "Failed to set GPIO level.");
  }

  return jerry_undefined();
}

/**
 * @brief The evaluate callback for the native 'gpio' module.
 * This function's job is to provide the actual values for the exports
 * that were declared when the module was created.
 */
jerry_value_t
gpio_module_evaluate(const jerry_value_t native_module)
{
  // 1. Create jerry_value_t representations of the C handler functions
  jerry_value_t config_func = jerry_function_external(js_gpio_config_handler);
  jerry_value_t reset_pin_func = jerry_function_external(js_gpio_reset_pin_handler);
  jerry_value_t get_level_func = jerry_function_external(js_gpio_get_level_handler);
  jerry_value_t set_level_func = jerry_function_external(js_gpio_set_level_handler);

  // 2. Create jerry_value_t representations of the export names
  jerry_value_t config_name = jerry_string_sz("config");
  jerry_value_t reset_pin_name = jerry_string_sz("reset_pin");
  jerry_value_t get_level_name = jerry_string_sz("get_level");
  jerry_value_t set_level_name = jerry_string_sz("set_level");

  // 3. Set the exports on the native module object.
  // This connects the declared export name to its actual function value.
  jerry_native_module_set(native_module, config_name, config_func);
  jerry_native_module_set(native_module, reset_pin_name, reset_pin_func);
  jerry_native_module_set(native_module, get_level_name, get_level_func);
  jerry_native_module_set(native_module, set_level_name, set_level_func);

  // 4. Clean up all the jerry_value_t's we created.
  jerry_value_free(config_func);
  jerry_value_free(reset_pin_func);
  jerry_value_free(get_level_func);
  jerry_value_free(set_level_func);

  jerry_value_free(config_name);
  jerry_value_free(reset_pin_name);
  jerry_value_free(get_level_name);
  jerry_value_free(set_level_name);

  return jerry_undefined(); // Return undefined on success
}
