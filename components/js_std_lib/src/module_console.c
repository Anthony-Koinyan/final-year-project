#include "jerryscript.h"
#include "jerryscript-ext/properties.h" // For jerryx_set_properties
#include "esp_log.h"
#include "module_console.h"

#define LOG_BUFFER_SIZE 256
#define TAG "CONSOLE_MODULE"

// --- Private Handler Functions ---

typedef enum
{
  JS_LOG_LEVEL_INFO,
  JS_LOG_LEVEL_WARN,
  JS_LOG_LEVEL_ERROR
} js_log_level_t;

/**
 * @brief Common logging logic used by console.log, .warn, and .error.
 * It concatenates all arguments into a single string and prints to ESP_LOG.
 */
static void js_console_log_common(js_log_level_t level,
                                  const jerry_value_t args[],
                                  jerry_length_t argc)
{
  char buffer[LOG_BUFFER_SIZE];
  size_t offset = 0;

  for (jerry_length_t i = 0; i < argc; i++)
  {
    jerry_value_t str_val = jerry_value_to_string(args[i]);

    jerry_size_t str_size = jerry_string_size(str_val, JERRY_ENCODING_UTF8);
    if (offset + str_size + 1 >= LOG_BUFFER_SIZE)
    {
      ESP_LOGW(TAG, "Log message truncated, exceeds buffer size.");
      jerry_value_free(str_val);
      break;
    }

    offset += jerry_string_to_buffer(str_val, JERRY_ENCODING_UTF8,
                                     (jerry_char_t *)(buffer + offset), str_size);
    buffer[offset++] = ' ';

    jerry_value_free(str_val);
  }

  if (offset > 0 && buffer[offset - 1] == ' ')
  {
    buffer[offset - 1] = '\0';
  }
  else
  {
    buffer[offset] = '\0';
  }

  switch (level)
  {
  case JS_LOG_LEVEL_INFO:
    ESP_LOGI("JS", "%s", buffer);
    break;
  case JS_LOG_LEVEL_WARN:
    ESP_LOGW("JS", "%s", buffer);
    break;
  case JS_LOG_LEVEL_ERROR:
    ESP_LOGE("JS", "%s", buffer);
    break;
  }
}

static jerry_value_t js_console_log_handler(const jerry_call_info_t *call_info_p,
                                            const jerry_value_t args[],
                                            const jerry_length_t argc)
{
  js_console_log_common(JS_LOG_LEVEL_INFO, args, argc);
  return jerry_undefined();
}

static jerry_value_t js_console_warn_handler(const jerry_call_info_t *call_info_p,
                                             const jerry_value_t args[],
                                             const jerry_length_t argc)
{
  js_console_log_common(JS_LOG_LEVEL_WARN, args, argc);
  return jerry_undefined();
}

static jerry_value_t js_console_error_handler(const jerry_call_info_t *call_info_p,
                                              const jerry_value_t args[],
                                              const jerry_length_t argc)
{
  js_console_log_common(JS_LOG_LEVEL_ERROR, args, argc);
  return jerry_undefined();
}

// --- Public Factory Function ---

jerry_value_t create_console_object(void)
{
  // Create the empty 'console' object that will hold our functions
  jerry_value_t console_obj = jerry_object();

  // Define the properties to be added to the console object
  jerryx_property_entry console_properties[] = {
      JERRYX_PROPERTY_FUNCTION("log", js_console_log_handler),
      JERRYX_PROPERTY_FUNCTION("warn", js_console_warn_handler),
      JERRYX_PROPERTY_FUNCTION("error", js_console_error_handler),
      JERRYX_PROPERTY_LIST_END()};

  // Set all the properties on the console object in one go.
  jerryx_register_result reg_result = jerryx_set_properties(console_obj, console_properties);

  // Check for errors during registration
  if (jerry_value_is_exception(reg_result.result))
  {
    ESP_LOGE(TAG, "Failed to register console properties. Only %u succeeded.", (unsigned int)reg_result.registered);
    // As per docs, we must clean up the entries and the error value on failure.
    jerryx_release_property_entry(console_properties, reg_result);
  }
  else
  {
    // On success, the result is 'undefined', which we must still release.
    jerry_value_free(reg_result.result);
  }

  return console_obj;
}
