#include <string.h>
#include "jerryscript.h"
#include "jerryscript-ext/properties.h"
#include "jerryscript-ext/arg.h"
#include "driver/gpio.h"
#include "esp_log.h"

#include "js_gpio.h"
#include "module_gpio.h"

#define TAG "GPIO_MODULE"

// Forward declaration for the native object's free callback
static void pin_native_free_cb(void *native_p, jerry_object_native_info_t *info_p);

/**
 * @brief JerryScript native object info. Connects a JS object to our js_pin_t struct.
 */
static const jerry_object_native_info_t pin_native_info = {
    .free_cb = pin_native_free_cb,
};

// --- Pin Object Method Implementations (Bindings) ---

static jerry_value_t
js_pin_read_handler(const jerry_call_info_t *call_info_p, const jerry_value_t args[], const jerry_length_t argc)
{
  js_pin_t *state = (js_pin_t *)jerry_object_get_native_ptr(call_info_p->this_value, &pin_native_info);
  if (!state || !state->in_use)
  {
    return jerry_throw_sz(JERRY_ERROR_TYPE, "Pin is closed or invalid.");
  }
  return jerry_boolean(gpio_get_level(state->pin_num));
}

static jerry_value_t
js_pin_write_handler(const jerry_call_info_t *call_info_p, const jerry_value_t args[], const jerry_length_t argc)
{
  js_pin_t *state = (js_pin_t *)jerry_object_get_native_ptr(call_info_p->this_value, &pin_native_info);

  if (!state || !state->in_use)
  {
    return jerry_throw_sz(JERRY_ERROR_TYPE, "Pin is closed or invalid.");
  }

  bool level;
  const jerryx_arg_t mapping[] = {
      jerryx_arg_boolean(&level, JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED)};

  jerry_value_t result = jerryx_arg_transform_args(args, argc, mapping, 1);
  if (jerry_value_is_exception(result))
  {
    return result;
  }
  jerry_value_free(result);

  gpio_set_level(state->pin_num, level);
  return jerry_undefined();
}

static jerry_value_t
js_pin_attach_isr_handler(const jerry_call_info_t *call_info_p, const jerry_value_t args[], const jerry_length_t argc)
{
  js_pin_t *state = (js_pin_t *)jerry_object_get_native_ptr(call_info_p->this_value, &pin_native_info);

  if (!state || !state->in_use)
  {
    return jerry_throw_sz(JERRY_ERROR_TYPE, "Pin is closed or invalid.");
  }

  jerry_value_t callback;
  const jerryx_arg_t mapping[] = {
      jerryx_arg_function(&callback, JERRYX_ARG_REQUIRED)};

  jerry_value_t result = jerryx_arg_transform_args(args, argc, mapping, 1);
  if (jerry_value_is_exception(result))
  {
    return result;
  }
  // The transform function does not copy the value, so we don't free the result.
  // We will copy it in js_gpio_attach_isr.
  esp_err_t err = js_gpio_attach_isr(state->pin_num, callback);
  jerry_value_free(result); // result is undefined, but good practice to free.
  if (err != ESP_OK)
  {
    return jerry_throw_sz(JERRY_ERROR_COMMON, "Failed to attach ISR.");
  }
  return jerry_undefined();
}

static jerry_value_t
js_pin_detach_isr_handler(const jerry_call_info_t *call_info_p, const jerry_value_t args[], const jerry_length_t argc)
{
  js_pin_t *state = (js_pin_t *)jerry_object_get_native_ptr(call_info_p->this_value, &pin_native_info);

  if (!state || !state->in_use)
  {
    return jerry_throw_sz(JERRY_ERROR_TYPE, "Pin is closed or invalid.");
  }
  js_gpio_detach_isr(state->pin_num);
  return jerry_undefined();
}

static jerry_value_t
js_pin_close_handler(const jerry_call_info_t *call_info_p, const jerry_value_t args[], const jerry_length_t argc)
{
  js_pin_t *state = (js_pin_t *)jerry_object_get_native_ptr(call_info_p->this_value, &pin_native_info);

  if (state)
  {
    js_gpio_close(state->pin_num);
    // Remove the native pointer from the JS object to prevent use-after-free
    jerry_object_set_native_ptr(call_info_p->this_value, NULL, NULL);
  }
  return jerry_undefined();
}

/**
 * @brief Callback when a Pin object is garbage collected.
 */
static void pin_native_free_cb(void *native_p, jerry_object_native_info_t *info_p)
{
  js_pin_t *state = (js_pin_t *)native_p;
  if (state && state->in_use)
  {
    ESP_LOGD(TAG, "GC collecting pin %d, ensuring cleanup.", state->pin_num);
    js_gpio_close(state->pin_num);
  }
}

/**
 * @brief Creates a JS Pin object and links it to its native state.
 */
static jerry_value_t create_pin_object(gpio_num_t pin_num)
{
  js_pin_t *pin_state = js_gpio_get_state(pin_num);
  if (!pin_state)
  {
    return jerry_throw_sz(JERRY_ERROR_COMMON, "Failed to get internal pin state.");
  }

  jerry_value_t pin_obj = jerry_object();
  jerry_object_set_native_ptr(pin_obj, &pin_native_info, pin_state);

  // Attach methods
  jerryx_property_entry props[] = {
      JERRYX_PROPERTY_FUNCTION("read", js_pin_read_handler),
      JERRYX_PROPERTY_FUNCTION("write", js_pin_write_handler),
      JERRYX_PROPERTY_FUNCTION("attachISR", js_pin_attach_isr_handler),
      JERRYX_PROPERTY_FUNCTION("detachISR", js_pin_detach_isr_handler),
      JERRYX_PROPERTY_FUNCTION("close", js_pin_close_handler),
      JERRYX_PROPERTY_LIST_END(),
  };

  jerryx_set_properties(pin_obj, props);

  // Attach readonly 'pin' property
  jerry_value_t pin_prop_name = jerry_string_sz("pin");
  jerry_property_descriptor_t prop_desc = jerry_property_descriptor();
  prop_desc.flags |= JERRY_PROP_IS_VALUE_DEFINED;
  prop_desc.value = jerry_number(pin_num);
  jerry_value_t ret = jerry_object_define_own_prop(pin_obj, pin_prop_name, &prop_desc);

  if (jerry_value_is_exception(ret))
  {
    return jerry_throw_sz(JERRY_ERROR_COMMON, "Failed to create pin object.");
  }

  jerry_property_descriptor_free(&prop_desc);
  jerry_value_free(pin_prop_name);
  jerry_value_free(ret);

  return pin_obj;
}

/**
 * @brief Native implementation of `gpio.setup(pins, config)`.
 */
static jerry_value_t
js_gpio_setup_handler(const jerry_call_info_t *call_info_p, const jerry_value_t args[], const jerry_length_t argc)
{
  if (argc < 2)
    return jerry_throw_sz(JERRY_ERROR_TYPE, "Expected 2 arguments: pin(s) and config object.");
  if (!jerry_value_is_number(args[0]) && !jerry_value_is_array(args[0]))
    return jerry_throw_sz(JERRY_ERROR_TYPE, "First argument must be a pin number or an array of pin numbers.");
  if (!jerry_value_is_object(args[1]))
    return jerry_throw_sz(JERRY_ERROR_TYPE, "Second argument must be a config object.");

  // --- Parse Config Object ---
  jerry_value_t config_obj = args[1];
  gpio_config_t io_conf;

  // Buffers to hold the string values from the JS object
  char mode_str[16] = "";
  char pull_mode_str[16] = "";
  char interrupt_str[16] = "";
  double debounce_ms = 0;

  const char *prop_names[] = {"mode", "pullMode", "interrupt", "debounce"};
  const jerryx_arg_t prop_mapping[] = {
      jerryx_arg_string(mode_str, sizeof(mode_str), JERRYX_ARG_COERCE, JERRYX_ARG_REQUIRED),
      jerryx_arg_string(pull_mode_str, sizeof(pull_mode_str), JERRYX_ARG_COERCE, JERRYX_ARG_OPTIONAL),
      jerryx_arg_string(interrupt_str, sizeof(interrupt_str), JERRYX_ARG_COERCE, JERRYX_ARG_OPTIONAL),
      jerryx_arg_number(&debounce_ms, JERRYX_ARG_COERCE, JERRYX_ARG_OPTIONAL),
  };

  jerry_value_t result = jerryx_arg_transform_object_properties(config_obj,
                                                                (const jerry_char_t **)prop_names,
                                                                4,
                                                                prop_mapping,
                                                                4);

  if (jerry_value_is_exception(result))
  {
    return result;
  }
  jerry_value_free(result);

  // --- Convert parsed strings to enum values ---
  if (strcmp(mode_str, "output") == 0)
    io_conf.mode = GPIO_MODE_OUTPUT;
  else if (strcmp(mode_str, "input") == 0)
    io_conf.mode = GPIO_MODE_INPUT;
  else if (strcmp(mode_str, "input_output") == 0)
    io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
  else
    io_conf.mode = GPIO_MODE_DISABLE;

  if (strlen(pull_mode_str) > 0)
  {
    if (strcmp(pull_mode_str, "pullup") == 0)
      io_conf.pull_up_en = 1;
    else if (strcmp(pull_mode_str, "pulldown") == 0)
      io_conf.pull_down_en = 1;
    else if (strcmp(pull_mode_str, "both") == 0)
    {
      io_conf.pull_up_en = 1;
      io_conf.pull_down_en = 1;
    }
    else
    {
      io_conf.pull_up_en = 0;
      io_conf.pull_down_en = 0;
    }
  }
  else
  {

    io_conf.pull_up_en = 0;
    io_conf.pull_down_en = 0;
  }

  if (strlen(interrupt_str) > 0)
  {
    if (strcmp(interrupt_str, "rising") == 0)
      io_conf.intr_type = GPIO_INTR_POSEDGE;
    else if (strcmp(interrupt_str, "falling") == 0)
      io_conf.intr_type = GPIO_INTR_NEGEDGE;
    else if (strcmp(interrupt_str, "both") == 0)
      io_conf.intr_type = GPIO_INTR_ANYEDGE;
    else if (strcmp(interrupt_str, "low") == 0)
      io_conf.intr_type = GPIO_INTR_LOW_LEVEL;
    else if (strcmp(interrupt_str, "high") == 0)
      io_conf.intr_type = GPIO_INTR_HIGH_LEVEL;
    else
      io_conf.intr_type = GPIO_INTR_DISABLE;
  }
  else
    io_conf.intr_type = GPIO_INTR_DISABLE;

  // --- Process Pin(s) and Apply Config ---
  uint64_t pin_bit_mask = 0;
  if (jerry_value_is_number(args[0]))
  {
    pin_bit_mask = (1ULL << (int)jerry_value_as_number(args[0]));
  }
  else // It's an array
  {
    uint32_t len = jerry_array_length(args[0]);
    for (uint32_t i = 0; i < len; i++)
    {
      jerry_value_t pin_val = jerry_object_get_index(args[0], i);
      if (jerry_value_is_number(pin_val))
      {
        pin_bit_mask |= (1ULL << (int)jerry_value_as_number(pin_val));
      }
      jerry_value_free(pin_val);
    }
  }

  io_conf.pin_bit_mask = pin_bit_mask;
  if (js_gpio_configure(&io_conf) != ESP_OK)
  {
    return jerry_throw_sz(JERRY_ERROR_COMMON, "Failed to configure GPIO pin(s).");
  }

  // --- Set Debounce and Create Pin Object(s) ---
  if (jerry_value_is_number(args[0]))
  {
    gpio_num_t pin_num = (gpio_num_t)jerry_value_as_number(args[0]);
    js_gpio_get_state(pin_num)->debounce_ms = (uint32_t)debounce_ms; // Set debounce value
    return create_pin_object(pin_num);
  }
  else // Handle array case
  {
    uint32_t len = jerry_array_length(args[0]);
    jerry_value_t pin_array = jerry_array(len);
    for (uint32_t i = 0; i < len; i++)
    {
      jerry_value_t pin_val = jerry_object_get_index(args[0], i);
      gpio_num_t pin_num = (gpio_num_t)jerry_value_as_number(pin_val);
      js_gpio_get_state(pin_num)->debounce_ms = (uint32_t)debounce_ms; // Set debounce for each pin
      jerry_value_t pin_obj = create_pin_object(pin_num);
      jerry_object_set_index(pin_array, i, pin_obj);
      jerry_value_free(pin_obj);
      jerry_value_free(pin_val);
    }
    return pin_array;
  }
}

/**
 * @brief The evaluation callback for the native 'gpio' module.
 */
jerry_value_t
gpio_module_evaluate(const jerry_value_t native_module)
{
  jerry_value_t setup_func = jerry_function_external(js_gpio_setup_handler);
  jerry_value_t setup_name = jerry_string_sz("setup");
  jerry_native_module_set(native_module, setup_name, setup_func);
  jerry_value_free(setup_func);
  jerry_value_free(setup_name);

  return jerry_undefined();
}
