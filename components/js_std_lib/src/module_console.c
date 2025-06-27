#include "jerryscript.h"
#include "js_std_lib.h"
#include "esp_log.h"

#define LOG_BUFFER_SIZE 256

typedef enum
{
  JS_LOG_LEVEL_INFO,
  JS_LOG_LEVEL_WARN,
  JS_LOG_LEVEL_ERROR
} js_log_level_t;

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

const js_native_function_def_t *console_module_get_functions(size_t *count)
{
  static const js_native_function_def_t console_fns[] = {
      {.name = "log", .handler = js_console_log_handler},
      {.name = "warn", .handler = js_console_warn_handler},
      {.name = "error", .handler = js_console_error_handler}};

  if (count)
  {
    *count = sizeof(console_fns) / sizeof(console_fns[0]);
  }

  return console_fns;
}
