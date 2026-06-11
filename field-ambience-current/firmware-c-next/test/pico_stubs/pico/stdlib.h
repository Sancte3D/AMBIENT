/* Fake Pico-SDK header for HOST builds of device-only tools (see
 * test_display_bench.c). Declarations only — the test provides the
 * implementations (fake time, recorded PWM, scripted GPIO). */
#ifndef _PICO_STDLIB_H
#define _PICO_STDLIB_H
#include <stdint.h>
#include <stdbool.h>
typedef unsigned int uint;
void stdio_init_all(void);
void sleep_ms(uint32_t ms);
#include "hardware/gpio.h"
#include "pico/time.h"
#endif
