#ifndef _HW_PWM_H
#define _HW_PWM_H
#include <stdint.h>
#include <stdbool.h>
typedef unsigned int uint;
uint pwm_gpio_to_slice_num(uint gpio);
void pwm_set_wrap(uint slice, uint16_t wrap);
void pwm_set_gpio_level(uint gpio, uint16_t level);
void pwm_set_enabled(uint slice, bool enabled);
#endif
