idf_component_register(SRCS "code.c"
                    INCLUDE_DIRS "."
                    )

spiffs_create_partition_image(storage "../js" FLASH_IN_PROJECT)

