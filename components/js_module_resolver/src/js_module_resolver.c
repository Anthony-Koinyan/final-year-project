#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "jerryscript.h"
#include "js_main_thread.h"
#include "js_std_lib.h" // For js_get_native_module
#include "js_module_resolver.h"

#define TAG "MODULE_RESOLVER"
#define SPIFFS_DIR "/storage"
#define MAX_PATH_LENGTH 64

/**
 * @brief Reads a file from the SPIFFS filesystem into a newly allocated buffer.
 *
 * @param path The relative path to the file within the SPIFFS directory (e.g., "main.js").
 * @param out_size Pointer to a size_t where the file size will be stored.
 * @return A pointer to the allocated buffer with the file content, or NULL on
 * failure. The caller is responsible for freeing this buffer.
 */
static unsigned char *read_file_into_buffer(const char *path, size_t *out_size)
{
  char full_path[MAX_PATH_LENGTH];
  snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", SPIFFS_DIR, path);
  ESP_LOGI(TAG, "Attempting to load module from path: %s", full_path);
  FILE *file = fopen(full_path, "rb");
  if (!file)
  {
    ESP_LOGE(TAG, "File not found: %s", full_path);
    return NULL;
  }
  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);
  unsigned char *buffer = malloc(size + 1);
  if (!buffer)
  {
    ESP_LOGE(TAG, "Failed to allocate buffer for module: %s", full_path);
    fclose(file);
    return NULL;
  }
  if (fread(buffer, 1, size, file) != size)
  {
    ESP_LOGE(TAG, "Read mismatch for module: %s", full_path);
    free(buffer);
    fclose(file);
    return NULL;
  }
  buffer[size] = '\0';
  fclose(file);
  if (out_size)
  {
    *out_size = size;
  }
  return buffer;
}

/**
 * @brief Checks if a module specifier looks like a relative file path.
 *
 * This heuristic determines whether a specifier should be treated as a file
 * path or a potential native module name.
 *
 * @param specifier The module specifier string (e.g., "./utils.js" or "gpio").
 * @return `true` if the specifier starts with './', '../', or '/' OR ends with
 * '.js', otherwise `false`.
 */
static bool is_file_specifier(const jerry_char_t *specifier)
{
  if (strncmp((const char *)specifier, "./", 2) == 0 ||
      strncmp((const char *)specifier, "../", 3) == 0 ||
      specifier[0] == '/')
  {
    return true;
  }
  size_t len = strlen((const char *)specifier);
  if (len > 3 && strcmp((const char *)specifier + len - 3, ".js") == 0)
  {
    return true;
  }
  return false;
}

/**
 * @brief The callback given to `jerry_module_link` to resolve module dependencies.
 *
 * This is the heart of the module system. When JerryScript encounters an
 * `import` statement, it calls this function with the module specifier (e.g.,
 * "gpio" or "./my_module.js"). This function's job is to find the source for
 * that module and return it to the engine as a new, unlinked module object.
 *
 * The resolution strategy is:
 * 1. Check if the specifier looks like a file path. If not, try to resolve it
 * as a native module using `js_get_native_module`.
 * 2. If it is a file path, or if native resolution fails, attempt to load the
 * corresponding file from the SPIFFS filesystem.
 *
 * @param specifier The module specifier string from the import statement.
 * @param referrer The module that is doing the importing (unused).
 * @param user_p A user-provided pointer (unused).
 * @return A new, unlinked jerry_value_t module object on success, or a
 * JerryScript error on failure.
 */
static jerry_value_t module_resolve_callback(const jerry_value_t specifier,
                                             const jerry_value_t referrer,
                                             void *user_p)
{
  (void)referrer;
  (void)user_p;

  jerry_size_t specifier_size = jerry_string_size(specifier, JERRY_ENCODING_UTF8);
  jerry_char_t specifier_buf[specifier_size + 1];
  jerry_string_to_buffer(specifier, JERRY_ENCODING_UTF8, specifier_buf, specifier_size);
  specifier_buf[specifier_size] = '\0';

  // 1. Check if it's a file path specifier. If not, try the native registry.
  if (!is_file_specifier(specifier_buf))
  {
    jerry_value_t native_module = js_get_native_module(specifier);
    // If we got a valid module (not an error), return it.
    if (!jerry_value_is_exception(native_module))
    {
      return native_module;
    }
    // If it was an error, we release it and fall through to the file search.
    // This allows a file like `gpio.js` to override a native module.
    jerry_value_free(native_module);
  }

  // 2. If it's a file or native lookup failed, proceed with filesystem search.
  char path_buf[MAX_PATH_LENGTH];
  const char *filename = (char *)specifier_buf;
  if (strncmp(filename, "./", 2) == 0)
  {
    filename += 2; // Skip the './' prefix
  }

  // Add .js extension if it's missing
  if (strstr(filename, ".js") == NULL)
  {
    snprintf(path_buf, MAX_PATH_LENGTH, "%s.js", filename);
  }
  else
  {
    snprintf(path_buf, MAX_PATH_LENGTH, "%s", filename);
  }

  size_t script_size = 0;
  unsigned char *script_buffer = read_file_into_buffer(path_buf, &script_size);

  if (script_buffer == NULL)
  {
    ESP_LOGE(TAG, "Cannot resolve module: %s", path_buf);
    return jerry_throw_sz(JERRY_ERROR_COMMON, "Module not found");
  }

  jerry_parse_options_t parse_options = {
      .options = JERRY_PARSE_MODULE | JERRY_PARSE_HAS_SOURCE_NAME,
      .source_name = jerry_string_sz(path_buf),
  };
  jerry_value_t new_module = jerry_parse(script_buffer, script_size, &parse_options);

  free(script_buffer);
  return new_module;
}

void js_run_main_module(void)
{
  const char *main_file_name = "main.js";
  size_t script_size = 0;
  unsigned char *script_buffer = read_file_into_buffer("main.js", &script_size);
  if (script_buffer == NULL)
  {
    ESP_LOGE(TAG, "Could not load main.js. Aborting.");
    return;
  }

  jerry_parse_options_t parse_options = {
      .options = JERRY_PARSE_MODULE | JERRY_PARSE_HAS_SOURCE_NAME,
      .source_name = jerry_string_sz(main_file_name),
  };
  jerry_value_t main_module = jerry_parse(script_buffer, script_size, &parse_options);
  free(script_buffer);

  if (jerry_value_is_exception(main_module))
  {
    ESP_LOGE(TAG, "Failed to parse main.js.");
    print_js_error(main_module);
    jerry_value_free(main_module);
    return;
  }

  ESP_LOGI(TAG, "Linking main module...");
  jerry_value_t link_result = jerry_module_link(main_module, module_resolve_callback, NULL);
  if (jerry_value_is_exception(link_result))
  {
    ESP_LOGE(TAG, "Failed to link modules.");
    print_js_error(link_result);
    jerry_value_free(link_result);
    jerry_value_free(main_module);
    return;
  }
  jerry_value_free(link_result);

  ESP_LOGI(TAG, "Evaluating main module...");
  jerry_value_t eval_result = jerry_module_evaluate(main_module);
  if (jerry_value_is_exception(eval_result))
  {
    ESP_LOGE(TAG, "Error during module evaluation.");
    print_js_error(eval_result);
  }
  jerry_value_free(eval_result);

  jerry_value_free(main_module);
  ESP_LOGI(TAG, "Module execution finished.");
}
