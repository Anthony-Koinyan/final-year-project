#ifndef JS_MODULE_RESOLVER_H
#define JS_MODULE_RESOLVER_H

#include "jerryscript-ext/module.h"

/**
 * @brief Resolver for loading JavaScript modules from the SPIFFS filesystem.
 */
extern const jerryx_module_resolver_t js_fs_resolver;

/**
 * @brief Resolver for loading built-in native C modules.
 */
extern const jerryx_module_resolver_t js_native_resolver;

/**
 * @brief Initializes the module resolver system and executes the main.js file as a module.
 */
void js_run_main(void);

#endif /* JS_MODULE_RESOLVER_H */
