#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"

#include "jerryscript.h"
#include "jerryscript-ext/handlers.h"
#include "jerryscript-ext/properties.h"

#include "js_std_lib.h"
#include "js_main_thread.h"

/**
 * TODO:
 * - INITIALISE FILESYSTEM (done)
 * - READ MAIN.JS FILE FROM FILESYSTEM (done)
 * - INITIALISE JERRYSCRIPT (done)
 * - RUN MAIN.JS IN JERRYSCRIPT (done)
 * - CREATE LOGGING MODULE (done)
 * - CREATE GPIO MODULE
 * - SETUP ASYNC IO SYSTEM
 * - SETUP TIMERS
 * - SETUP GPIO ISR
 * - CONFIGURE NETWORKING
 * - SETUP HTTP CLIENT MODULE
 * */

#define SPIFFS_DIR "/storage"
#define MAX_PATH_LENGTH 64

static void mount_file_system()
{
  ESP_LOGI("FS", "Initializing SPIFFS");

  esp_vfs_spiffs_conf_t config = {
      .base_path = SPIFFS_DIR,
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true};

  esp_err_t result = esp_vfs_spiffs_register(&config);

  if (result != ESP_OK)
  {
    if (result == ESP_FAIL)
    {
      ESP_LOGE("FS", "Failed to mount or format filesystem");
    }
    else if (result == ESP_ERR_NOT_FOUND)
    {
      ESP_LOGE("FS", "Failed to find SPIFFS partition");
    }
    else
    {
      ESP_LOGE("FS", "Failed to initialize SPIFFS (%s)", esp_err_to_name(result));
    }

    return;
  }

  size_t total = 0, used = 0;
  result = esp_spiffs_info(config.partition_label, &total, &used);
  if (result != ESP_OK)
  {
    ESP_LOGE("FS", "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(result));
    esp_spiffs_format(config.partition_label);
    return;
  }
  else
  {
    ESP_LOGI("FS", "Partition size: total: %d, used: %d", total, used);
  }

  // Check consistency of reported partition size info.
  if (used > total)
  {
    ESP_LOGW("FS", "Number of used bytes cannot be larger than total. Performing SPIFFS_check().");
    result = esp_spiffs_check(config.partition_label);

    if (result != ESP_OK)
    {
      ESP_LOGE("FS", "SPIFFS_check() failed (%s)", esp_err_to_name(result));
      return;
    }
    else
    {
      ESP_LOGI("FS", "SPIFFS_check() successful");
    }
  }
}

// NOTE: since SPIFFS has a flat file structure only the file name is required and SPIFFS_DIR is appended internally
static unsigned char *read_spiffs_file_into_buffer(const char *path, size_t *out_size)
{
  char full_path[MAX_PATH_LENGTH];
  snprintf(full_path, MAX_PATH_LENGTH, "%s/%s", SPIFFS_DIR, path);

  ESP_LOGI("FS", "Loading file %s", full_path);
  FILE *file = fopen(full_path, "rb");

  if (!file)
  {
    ESP_LOGE("FS", "Failed to open file: %s", full_path);
    return NULL;
  }

  fseek(file, 0, SEEK_END);
  size_t size = ftell(file);
  rewind(file);

  unsigned char *buffer = malloc(size);
  if (!buffer)
  {
    ESP_LOGE("FS", "Failed to allocate buffer");
    fclose(file);
    return NULL;
  }

  size_t read = fread(buffer, 1, size, file);
  if (read != size)
  {
    ESP_LOGE("FS", "Read mismatch: %zu vs %zu", read, size);
    free(buffer);
    fclose(file);
    return NULL;
  }

  fclose(file);

  if (out_size)
    *out_size = size;

  ESP_LOGI("FS", "Loaded file %s", full_path);

  return buffer;
}

void app_main(void)
{
  // mount file system
  mount_file_system();

  // load main.js file from file system
  size_t script_size = 0;
  unsigned char *script = read_spiffs_file_into_buffer("main.js", &script_size);

  if (script == NULL)
  {
    ESP_LOGE("APP", "Failed to main.js file into buffer");
    return;
  }

  js_file_t js_files[] = {
      {.name = "main.js", .buffer = script, .size = script_size},
      {.name = NULL, .buffer = NULL, .size = 0}};

  TaskHandle_t js_main_thread_handle = NULL;

  xTaskCreatePinnedToCore(js_task, "js_main_thread", 16 * 1024, (void *)js_files, 10, &js_main_thread_handle, 1);

  vTaskDelay(portMAX_DELAY);
}