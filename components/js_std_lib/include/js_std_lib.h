#ifndef JS_STD_LIB_H
#define JS_STD_LIB_H

#include "jerryscript.h"

// This struct is still useful for defining the functions within a module
typedef struct
{
  const char *name;
  jerry_external_handler_t handler;
} js_native_function_def_t;

/**
 * @brief Registers all standard library native modules (e.g., console, gpio)
 * with the JerryScript engine. This must be called before running any JS code.
 */
void register_js_std_lib(void);

#endif
