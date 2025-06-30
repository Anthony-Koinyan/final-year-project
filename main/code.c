#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>

#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "freertos/FreeRTOS.h"
#include "js_main_thread.h"

static void mount_file_system()
{
  ESP_LOGI("FS", "Initializing SPIFFS");

  esp_vfs_spiffs_conf_t config = {
      .base_path = "/storage",
      .partition_label = "storage", // Use the label from partitions.csv
      .max_files = 5,
      .format_if_mount_failed = true};

  esp_err_t result = esp_vfs_spiffs_register(&config);

  if (result != ESP_OK)
  {
    ESP_LOGE("FS", "Failed to initialize SPIFFS (%s)", esp_err_to_name(result));
    return;
  }

  size_t total = 0, used = 0;
  result = esp_spiffs_info(config.partition_label, &total, &used);
  if (result != ESP_OK)
  {
    ESP_LOGE("FS", "Failed to get SPIFFS partition information (%s).", esp_err_to_name(result));
  }
  else
  {
    ESP_LOGI("FS", "Partition size: total: %d, used: %d", total, used);
  }
}

void app_main(void)
{
  // 1. Mount the filesystem where our .js files live.
  mount_file_system();

  // 2. Create the FreeRTOS task that will run the JerryScript engine.
  // The task now handles all JS-related initializations and execution.
  xTaskCreatePinnedToCore(js_task, "js_main_thread", 16 * 1024, NULL, 10, NULL, 1);

  // The rest of the system can do other things here.
  // For this project, we just let the JS task run.
}
