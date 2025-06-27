/**
 * TODO:
 * - Create RTOS task for JS
 * - Create JS thread state opaque
 * - Create event loop
 */

#ifndef JS_MAIN_THREAD_H
#define JS_MAIN_THREAD_H

#include <stdio.h>

typedef struct
{
  const char *name;      // e.g. "gpio.js" or "main.js"
  unsigned char *buffer; // contents of the file
  size_t size;           // size of the buffer
} js_file_t;

void js_task(void *);
#endif