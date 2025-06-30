
set(JERRY_SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/jerry)
set(JERRY_INSTALL_DIR ${CMAKE_CURRENT_BINARY_DIR}/jerry_install)

idf_component_register(
    SRCS "${JERRY_SOURCE_DIR}/targets/baremetal-sdk/espressif/main/jerry-port.c"
    INCLUDE_DIRS "${JERRY_SOURCE_DIR}/jerry-ext/include" "${JERRY_SOURCE_DIR}/jerry-core/include"
)

include(ExternalProject)

ExternalProject_Add(
  jerryscript_proj
  SOURCE_DIR ${JERRY_SOURCE_DIR}
  CMAKE_ARGS
    -DCMAKE_TOOLCHAIN_FILE=${JERRY_SOURCE_DIR}/cmake/toolchain-esp32.cmake
    -DJERRY_CMDLINE=OFF
    -DENABLE_AMALGAM=ON
    -DJERRY_PROFILE=es.next
    -DJERRY_GLOBAL_HEAP_SIZE=64
    -DFEATURE_INIT_FINI=ON
    # -DJERRYX_NATIVE_MODULES_SUPPORTED=ON
    -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
    # -DJERRY_CPOINTER_32_BIT=ON
  USES_TERMINAL_DOWNLOAD TRUE
  USES_TERMINAL_CONFIGURE TRUE
  USES_TERMINAL_BUILD TRUE
  INSTALL_DIR ${JERRY_INSTALL_DIR}
  BUILD_BYPRODUCTS
    ${JERRY_INSTALL_DIR}/lib/libjerry-core.a
    ${JERRY_INSTALL_DIR}/lib/libjerry-ext.a
    ${JERRY_INSTALL_DIR}/lib/libjerry-port.a
)

# IMPORT THE BUILT LIBRARIES
add_library(jerry-core STATIC IMPORTED GLOBAL)
set_target_properties(jerry-core PROPERTIES
  IMPORTED_LOCATION "${JERRY_INSTALL_DIR}/lib/libjerry-core.a"
)
add_dependencies(jerry-core jerryscript_proj)

add_library(jerry-ext STATIC IMPORTED GLOBAL)
set_target_properties(jerry-ext PROPERTIES
  IMPORTED_LOCATION "${JERRY_INSTALL_DIR}/lib/libjerry-ext.a"
)
add_dependencies(jerry-ext jerryscript_proj)

add_library(jerry-port STATIC IMPORTED GLOBAL)
set_target_properties(jerry-port PROPERTIES
  IMPORTED_LOCATION "${JERRY_INSTALL_DIR}/lib/libjerry-port.a"
)
add_dependencies(jerry-port jerryscript_proj)

# jerry-core has espressif/main/jerry-port.c as dependency
target_link_libraries(jerry-core INTERFACE ${COMPONENT_LIB})

# LINK THE IMPORTED LIBRARIES TO YOUR COMPONENT
target_link_libraries(${COMPONENT_LIB} INTERFACE jerry-core jerry-ext jerry-port)
