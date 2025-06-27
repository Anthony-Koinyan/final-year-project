#include "jerryscript.h"
#include "js_std_lib.h"
#include "module_console.h"

static void bind_to_global_this(const char *module_name,
                                const js_native_function_def_t *functions,
                                size_t function_count,
                                jerry_value_t global_this)
{
  jerry_value_t module_obj = jerry_object();

  for (size_t i = 0; i < function_count; ++i)
  {
    jerry_value_t fn = jerry_function_external(functions[i].handler);
    jerry_value_t name = jerry_string_sz(functions[i].name);
    jerry_value_t prop_result = jerry_object_set(module_obj, name, fn);
    jerry_value_free(name);
    jerry_value_free(prop_result);
    jerry_value_free(fn);
  }

  jerry_value_t module_name_str = jerry_string_sz(module_name);
  jerry_value_t result = jerry_object_set(global_this, module_name_str, module_obj);
  jerry_value_free(module_name_str);
  jerry_value_free(result);
  jerry_value_free(module_obj);
}

void init_js_std_lib(jerry_value_t global_this)
{
  size_t *console_module_size = 0;

  bind_to_global_this("console", console_module_get_functions(console_module_size), 3, global_this);
}
