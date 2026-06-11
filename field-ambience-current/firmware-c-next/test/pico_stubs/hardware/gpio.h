#ifndef _HW_GPIO_H
#define _HW_GPIO_H
#include <stdbool.h>
typedef unsigned int uint;
enum gpio_dir { GPIO_IN = 0, GPIO_OUT = 1 };
enum gpio_function { GPIO_FUNC_SPI = 1, GPIO_FUNC_PWM = 4, GPIO_FUNC_SIO = 5 };
void gpio_init(uint pin);
void gpio_set_dir(uint pin, bool out);
void gpio_pull_up(uint pin);
bool gpio_get(uint pin);
void gpio_set_function(uint pin, int fn);
#endif
