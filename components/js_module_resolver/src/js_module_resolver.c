#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "esp_log.h"
#include "jerryscript.h" // Core API

#include "js_module_resolver.h"

#define TAG "MODULE_RESOLVER"
#define SPIFFS_DIR "/storage"
#define MAX_PATH_LENGTH 64

/**
 * @brief Prints a JerryScript error value to the log for debugging.
 */
static void print_js_error(jerry_value_t error_val)
{
  if (!jerry_value_is_exception(error_val))
  {
    return;
  }

  jerry_value_t err_val_cp = jerry_value_copy(error_val);
  jerry_value_t err_str_val = jerry_value_to_string(err_val_cp);
  jerry_size_t err_str_size = jerry_string_size(err_str_val, JERRY_ENCODING_UTF8);

  // Allocate a buffer on the stack, but handle potentially long error messages
  char err_str_buf[MAX_PATH_LENGTH];
  jerry_size_t copy_size = (err_str_size < MAX_PATH_LENGTH - 1) ? err_str_size : MAX_PATH_LENGTH - 1;

  jerry_string_to_buffer(err_str_val, JERRY_ENCODING_UTF8, (jerry_char_t *)err_str_buf, copy_size);
  err_str_buf[copy_size] = '\0';

  ESP_LOGE(TAG, "JavaScript Error: %s", err_str_buf);

  jerry_value_free(err_str_val);
  jerry_value_free(err_val_cp);
}

/**
 * @brief Reads a file from the SPIFFS filesystem into a newly allocated buffer.
 * @param path The relative path to the file within the SPIFFS directory.
 * @param out_size Pointer to a size_t where the file size will be stored.
 * @return A pointer to the allocated buffer with the file content, or NULL on failure.
 * The caller is responsible for freeing this buffer.
 */
static unsigned char *read_file_into_buffer(const char *path, size_t *out_size)
{
  char full_path[MAX_PATH_LENGTH];
  snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", SPIFFS_DIR, path);

  ESP_LOGI(TAG, "Attempting to load module from path: %s", full_path);
  FILE *file = fopen(full_path, "rb");

  if (!file)
  {
    ESP_LOGD(TAG, "File not found: %s", full_path);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  unsigned char *buffer = malloc(size + 1); // +1 for potential null terminator
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

  buffer[size] = '\0'; // Null-terminate the buffer
  fclose(file);
  if (out_size)
  {
    *out_size = size;
  }
  return buffer;
}

/**
 * @brief The callback given to jerry_module_link to resolve module dependencies.
 *
 * This function is the heart of the module system. When JerryScript encounters an
 * 'import' statement, it calls this function with the module specifier (e.g., "./my_module").
 * This function's job is to find the source code for that module and return it to the
 * engine as a new, unlinked module object.
 *
 * @param specifier The module specifier string from the import statement.
 * @param referrer The module that is doing the importing. (Useful for resolving complex relative paths).
 * @param user_p A user-provided pointer (not used in this implementation).
 * @return A new, unlinked jerry_value_t module object on success, or a JerryScript error on failure.
 */
static jerry_value_t module_resolve_callback(const jerry_value_t specifier,
                                             const jerry_value_t referrer,
                                             void *user_p)
{
  (void)referrer; // Unused in this simple implementation
  (void)user_p;   // Unused

  jerry_size_t specifier_size = jerry_string_size(specifier, JERRY_ENCODING_UTF8);
  jerry_char_t specifier_buf[specifier_size + 1];
  jerry_string_to_buffer(specifier, JERRY_ENCODING_UTF8, specifier_buf, specifier_size);
  specifier_buf[specifier_size] = '\0';

  // Normalize the path: handle './' prefix and add '.js' extension if missing.
  char path_buf[MAX_PATH_LENGTH];
  const char *filename = (char *)specifier_buf;
  if (strncmp(filename, "./", 2) == 0)
  {
    filename += 2; // Skip the './' prefix
  }

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

  // The callback must return a new, unlinked module object.
  // We create this by parsing the source code of the dependency.
  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_MODULE | JERRY_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name = jerry_string_sz(path_buf);

  jerry_value_t new_module = jerry_parse(script_buffer, script_size, &parse_options);

  free(script_buffer);
  return new_module;
}

/**
 * @brief Runs the application's main entry point using the standard JerryScript module API.
 */
void js_run_main_module(void)
{
  // 1. PARSE the main entry point file into an unlinked module object.
  const char *main_file_name = "main.js";
  size_t script_size = 0;
  unsigned char *script_buffer = read_file_into_buffer("main.js", &script_size);
  if (script_buffer == NULL)
  {
    ESP_LOGE(TAG, "Could not load main.js. Aborting.");
    return;
  }

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_MODULE | JERRY_PARSE_HAS_SOURCE_NAME;
  parse_options.source_name = jerry_string_sz(main_file_name);

  jerry_value_t main_module = jerry_parse(script_buffer, script_size, &parse_options);
  free(script_buffer);

  if (jerry_value_is_exception(main_module))
  {
    ESP_LOGE(TAG, "Failed to parse main.js.");
    print_js_error(main_module);
    jerry_value_free(main_module);
    return;
  }

  // 2. LINK the main module. JerryScript will call our callback for each dependency.
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
  jerry_value_free(link_result); // On success, this is 'true', which we must release.

  // 3. EVALUATE the fully linked module tree.
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
