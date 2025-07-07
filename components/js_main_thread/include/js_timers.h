
#ifndef JS_TIMERS_H
#define JS_TIMERS_H

#include <stdbool.h>

#include "esp_timer.h"
#include "jerryscript.h"

#include "js_event.h"

#define MAX_TIMERS 16 // adjust as needed

typedef struct
{
  bool in_use;
  uint32_t handle_id;
  bool is_interval;
  esp_timer_handle_t timer;
  jerry_value_t js_callback;
} js_timer_t;

void js_timers_init(void);
uint32_t js_timers_set(bool is_interval,
                       jerry_value_t callback,
                       uint64_t delay_ms);
bool js_timers_clear(uint32_t handle_id);

// Called from js_main_thread dispatch loop
bool js_timers_dispatch(uint32_t handle_id);
#endif /* JS_TIMERS_H */