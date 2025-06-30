#include <stdio.h>
#include <string.h>

#include "esp_log.h"

#include "jerryscript.h"
#include "jerryscript-ext/module.h"

#include "js_main_thread.h"
#include "js_module_resolver.h"

#define SPIFFS_DIR "/storage"
#define MAX_PATH_LENGTH 64
#define JERRYX_NATIVE_MODULES_SUPPORTED 1

// --- JS File Module Resolver ---

/**
 * @brief Reads a JS file from the SPIFFS filesystem.
 * This is the core of the JS file resolver.
 */
static unsigned char *read_spiffs_file_into_buffer(const char *path, size_t *out_size)
{
  char full_path[MAX_PATH_LENGTH];
  snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", SPIFFS_DIR, path);

  ESP_LOGI("MODULE_FS", "Loading JS module from path: %s", full_path);
  FILE *file = fopen(full_path, "rb");

  if (!file)
  {
    ESP_LOGW("MODULE_FS", "Failed to open file: %s", full_path);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  unsigned char *buffer = malloc(size);
  if (!buffer)
  {
    ESP_LOGE("MODULE_FS", "Failed to allocate buffer for module");
    fclose(file);
    return NULL;
  }

  size_t read_bytes = fread(buffer, 1, size, file);
  if (read_bytes != size)
  {
    ESP_LOGE("MODULE_FS", "Read mismatch: expected %zu, got %zu", size, read_bytes);
    free(buffer);
    fclose(file);
    return NULL;
  }

  fclose(file);
  if (out_size)
  {
    *out_size = size;
  }
  ESP_LOGI("MODULE_FS", "Successfully loaded JS module: %s", full_path);
  return buffer;
}

/**
 * @brief The resolver function for loading JavaScript files from SPIFFS.
 * JerryScript calls this when it needs to resolve a module.
 */
static bool spiffs_js_module_resolve(const jerry_value_t canonical_name, jerry_value_t *result)
{
  jerry_size_t name_size = jerry_string_size(canonical_name, JERRY_ENCODING_UTF8);
  jerry_char_t name_buffer[name_size + 1];
  jerry_string_to_buffer(canonical_name, JERRY_ENCODING_UTF8, name_buffer, name_size);
  name_buffer[name_size] = '\0';

  size_t script_size = 0;
  unsigned char *script_buffer = read_spiffs_file_into_buffer((const char *)name_buffer, &script_size);

  if (script_buffer == NULL)
  {
    // Could not find or read the file.
    // Returning false tells JerryScript to try the next resolver.
    return false;
  }

  jerry_parse_options_t parse_options;
  parse_options.options = JERRY_PARSE_MODULE;

  jerry_value_t parsed_code = jerry_parse((jerry_char_t *)script_buffer, script_size, &parse_options);
  free(script_buffer);

  if (jerry_value_is_exception(parsed_code))
  {
    ESP_LOGE("MODULE_FS", "Failed to parse JS module: %s", name_buffer);
    *result = jerry_exception_value(parsed_code, true); // Propagate the error
    jerry_value_free(parsed_code);
    return true; // Return true because we handled it (by creating an error)
  }

  // The result of a module is the result of its evaluation.
  *result = parsed_code;

  jerry_value_free(parsed_code);

  return true; // We successfully resolved this module.
}

// The resolver structure for filesystem modules.
// We don't need a canonical name resolver for now, so it's NULL.
const jerryx_module_resolver_t js_fs_resolver = {
    .get_canonical_name_p = NULL, // Use the provided name as is
    .resolve_p = spiffs_js_module_resolve,
};

// --- Native Module Resolver ---

// For now, this is just a placeholder.
// We will register native modules (like 'console', 'gpio') with this resolver.
static const jerryx_native_module_t *native_modules = NULL;

/**
 * @brief Initialize the module system and run the main entry point.
 */
void js_run_main(void)
{
  // The order is important. We first check for native modules, then for files.
  const jerryx_module_resolver_t *resolvers[] = {
      &jerryx_module_native_resolver,
      &js_fs_resolver};

  jerry_value_t name = jerry_string_sz("main.js");
  jerry_value_t module_val = jerryx_module_resolve(name, resolvers, 2);

  if (jerry_value_is_exception(module_val))
  {
    ESP_LOGE("MAIN", "Failed to resolve or run main.js module.");
    // TODO: Add more detailed error printing here.
  }

  jerry_value_free(module_val);
  jerry_value_free(name);
}
