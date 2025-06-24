idf_component_register(SRCS "code.c"
                    INCLUDE_DIRS ".")

set(spiffs_image_dir "${CMAKE_CURRENT_SOURCE_DIR}/js")

if(EXISTS "${spiffs_image_dir}")
    spiffs_create_partition_image(js_spiffs_partition ${spiffs_image_dir} FLASH_IN_PROJECT)
endif()
