#ifndef JS_MODULE_RESOLVER_H
#define JS_MODULE_RESOLVER_H

#include "jerryscript-ext/module.h"

/**
 * @brief Runs the application's main entry point (`main.js`).
 *
 * This function orchestrates the entire process of loading, linking, and
 * evaluating the main JavaScript module and all of its dependencies
 * (both native and file-based).
 */
void js_run_main_module(void);

#endif /* JS_MODULE_RESOLVER_H */
