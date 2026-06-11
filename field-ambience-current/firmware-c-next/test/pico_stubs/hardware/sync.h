#ifndef _HW_SYNC_H
#define _HW_SYNC_H
#include <stdint.h>
uint32_t save_and_disable_interrupts(void);
void restore_interrupts(uint32_t status);
#endif
