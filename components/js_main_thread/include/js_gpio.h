#ifndef JS_GPIO_H
#define JS_GPIO_H

#include <stdbool.h>
#include "driver/gpio.h"
#include "jerryscript.h"
#include "js_event.h"

#define MAX_GPIO_PINS 40 // Maximum number of GPIO pins on ESP32

/**
 * @brief Represents the internal state of a single managed GPIO pin.
 */
typedef struct
{
  bool in_use;
  gpio_num_t pin_num;
  jerry_value_t js_isr_callback;
  uint32_t debounce_ms;
  int64_t last_isr_time_us;
} js_pin_t;
/**
 * @brief Initializes the GPIO management system.
 */
void js_gpio_init(void);

/**
 * @brief Gets the internal state object for a given pin number.
 */
js_pin_t *js_gpio_get_state(gpio_num_t pin_num);

/**
 * @brief Configures a set of pins using a bitmask via the ESP-IDF driver.
 */
esp_err_t js_gpio_configure(const gpio_config_t *pGPIOConfig);

/**
 * @brief Attaches a JavaScript function as an ISR callback for a pin.
 */
esp_err_t js_gpio_attach_isr(gpio_num_t pin_num, jerry_value_t callback);

/**
 * @brief Detaches the ISR callback from a pin.
 */
esp_err_t js_gpio_detach_isr(gpio_num_t pin_num);

/**
 * @brief Resets a pin and frees its resources in the management system.
 */
void js_gpio_close(gpio_num_t pin_num);

/**
 * @brief Dispatches a GPIO event from the main JS event loop.
 */
void js_gpio_dispatch_event(js_event_t *event);

#endif /* JS_GPIO_H */
