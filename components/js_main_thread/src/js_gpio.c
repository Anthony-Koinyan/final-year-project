#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "js_gpio.h"
#include "js_main_thread.h" // For js_event_queue and print_js_error

static const char *TAG = "JS_GPIO_ENGINE";
#define GLOBAL_DEBOUNCE_US 50000 // 50,000 microseconds = 50ms

/// @brief Pool of pin states.
static js_pin_t pins[MAX_GPIO_PINS];

/// @brief Flag to track if the ISR service has been installed.
static bool isr_service_installed = false;

/**
 * @brief The low-level ISR handler that runs in an interrupt context.
 * It simply sends an event to the main JS task for processing.
 */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
  uint32_t pin_num = (uint32_t)arg;
  js_pin_t *pin_state = &pins[pin_num]; // Direct access is safe in ISR

  // --- Per-Pin Debounce Logic ---
  if (pin_state->debounce_ms > 0)
  {
    int64_t now_us = esp_timer_get_time();
    if (now_us - pin_state->last_isr_time_us < pin_state->debounce_ms * 1000)
    {
      // Bounce detected for this specific pin, ignore this interrupt.
      portYIELD_FROM_ISR();
    }
    // This is a valid interrupt, update the pin's timestamp.
    pin_state->last_isr_time_us = now_us;
  }

  // If we passed the debounce check, send the event.
  js_event_t ev = {
      .type = JS_EVENT_GPIO,
      .handle_id = pin_num,
      .data = NULL,
  };
  BaseType_t woke = pdFALSE;
  xQueueSendFromISR(js_event_queue, &ev, &woke);
  if (woke)
  {
    portYIELD_FROM_ISR();
  }
}

/**
 * @brief Initializes the GPIO management system.
 */
void js_gpio_init(void)
{
  for (int i = 0; i < MAX_GPIO_PINS; i++)
  {
    pins[i].in_use = false;
    pins[i].js_isr_callback = jerry_undefined();
    pins[i].debounce_ms = 0;
    pins[i].last_isr_time_us = 0;
  }
}

/**
 * @brief Gets the internal state object for a given pin number.
 */
js_pin_t *js_gpio_get_state(gpio_num_t pin_num)
{
  if (pin_num < MAX_GPIO_PINS)
  {
    return &pins[pin_num];
  }
  return NULL;
}

/**
 * @brief Configures a set of pins using a bitmask.
 */
esp_err_t js_gpio_configure(const gpio_config_t *pGPIOConfig)
{
  esp_err_t err = gpio_config(pGPIOConfig);
  if (err != ESP_OK)
  {
    return err;
  }

  // If configuring for an interrupt, ensure the service is installed.
  if (pGPIOConfig->intr_type != GPIO_INTR_DISABLE && !isr_service_installed)
  {
    err = gpio_install_isr_service(0); // Use default flags
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
      // ESP_ERR_INVALID_STATE means it was already installed, which is fine.
      return err;
    }
    isr_service_installed = true;
  }

  // Mark all configured pins as 'in_use'
  for (int i = 0; i < MAX_GPIO_PINS; i++)
  {
    if ((pGPIOConfig->pin_bit_mask >> i) & 1)
    {
      pins[i].in_use = true;
      pins[i].pin_num = i;
    }
  }

  return ESP_OK;
}

/**
 * @brief Attaches a JS callback function to a pin's ISR.
 */
esp_err_t js_gpio_attach_isr(gpio_num_t pin_num, jerry_value_t callback)
{
  js_pin_t *pin_state = js_gpio_get_state(pin_num);
  if (!pin_state || !pin_state->in_use)
  {
    return ESP_ERR_NOT_FOUND;
  }

  // Release the old callback if it exists
  if (jerry_value_is_function(pin_state->js_isr_callback))
  {
    jerry_value_free(pin_state->js_isr_callback);
  }
  pin_state->js_isr_callback = jerry_value_copy(callback);

  return gpio_isr_handler_add(pin_num, &gpio_isr_handler, (void *)pin_num);
}

/**
 * @brief Detaches the ISR from a pin.
 */
esp_err_t js_gpio_detach_isr(gpio_num_t pin_num)
{
  js_pin_t *pin_state = js_gpio_get_state(pin_num);
  if (!pin_state || !pin_state->in_use)
  {
    return ESP_ERR_NOT_FOUND;
  }

  if (jerry_value_is_function(pin_state->js_isr_callback))
  {
    jerry_value_free(pin_state->js_isr_callback);
    pin_state->js_isr_callback = jerry_undefined();
  }

  return gpio_isr_handler_remove(pin_num);
}

/**
 * @brief Resets a pin and releases its resources.
 */
void js_gpio_close(gpio_num_t pin_num)
{
  js_pin_t *pin_state = js_gpio_get_state(pin_num);
  if (pin_state && pin_state->in_use)
  {
    js_gpio_detach_isr(pin_num); // Ensure ISR is detached
    gpio_reset_pin(pin_num);
    pin_state->in_use = false;
  }
}

/**
 * @brief Executes the JavaScript callback for a given GPIO event.
 */
void js_gpio_dispatch_event(js_event_t *event)
{
  js_pin_t *pin_state = js_gpio_get_state(event->handle_id);
  if (pin_state && pin_state->in_use && jerry_value_is_function(pin_state->js_isr_callback))
  {
    jerry_value_t global = jerry_current_realm();
    jerry_value_t res = jerry_call(pin_state->js_isr_callback, global, NULL, 0);
    jerry_value_free(global);
    if (jerry_value_is_exception(res))
    {
      print_js_error(res);
    }
    jerry_value_free(res);
  }
}
