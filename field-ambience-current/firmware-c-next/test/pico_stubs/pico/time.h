#ifndef _PICO_TIME_H
#define _PICO_TIME_H
#include <stdint.h>
#include <stdbool.h>
typedef uint64_t absolute_time_t;
absolute_time_t get_absolute_time(void);
uint32_t to_ms_since_boot(absolute_time_t t);
struct repeating_timer { void *user_data; };
typedef bool (*repeating_timer_callback_t)(struct repeating_timer *rt);
bool add_repeating_timer_us(int64_t delay_us, repeating_timer_callback_t cb,
                            void *user_data, struct repeating_timer *out);
#endif
