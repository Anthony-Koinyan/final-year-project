#ifndef JS_STD_LIB_H
#define JS_STD_LIB_H

#include "jerryscript.h"

typedef struct
{
  const char *name;
  jerry_external_handler_t handler;
} js_native_function_def_t;

/**
 * @brief Init JS standard library functions (e.g console, gpio etc)
 *
 * @param global_this JS value corresponsding to the `JS global this`
 */
void init_js_std_lib(jerry_value_t global_this);

#endif