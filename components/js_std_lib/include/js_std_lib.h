#ifndef JS_STD_LIB_H
#define JS_STD_LIB_H

/**
 * @brief Initializes standard JavaScript libraries and binds them to the global scope.
 *
 * This function is called once during runtime initialization to make common
 * functions like `console.log` and `setTimeout` globally available.
 */
void js_init_std_libs(void);

/**
 * @brief Resolves a module specifier against the native module registry.
 *
 * This function is the core of the native module resolver. It is called by the
 * JerryScript engine when it needs to resolve an `import` declaration. It searches
 * the `native_module_registry` for a matching module name.
 *
 * @param specifier The module name (e.g., "gpio") as a JerryScript string.
 * @return A `jerry_native_module_t` if found, or a JerryScript error otherwise.
 */
jerry_value_t js_get_native_module(const jerry_value_t specifier);

#endif /* JS_STD_LIB_H */
