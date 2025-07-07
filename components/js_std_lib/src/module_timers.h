#ifndef MODULE_TIMERS_H
#define MODULE_TIMERS_H

/**
 * @brief Binds timer functions to the JavaScript global object.
 *
 * This uses the `jerryx_set_properties` helper to efficiently add `setTimeout`,
 * `clearTimeout`, `setInterval`, and `clearInterval` to a given object,
 * typically the global object.
 *
 * @param global The JavaScript global object.
 */
void timers_bind_global(jerry_value_t);

/**
 * @brief Populates the exports for the 'timers' native module.
 *
 * This function is called by the JerryScript engine when `import ... from 'timers'`
 * is evaluated. It attaches the timer functions to the module's export object.
 *
 * @param native_module The `jerry_value_t` for the module being built.
 * @return `jerry_undefined()` on success.
 */
jerry_value_t timers_module_evaluate(const jerry_value_t);

#endif /* MODULE_TIMERS_H */