#ifndef FAM_SCENES_FLASH_H743_ASYNC_H
#define FAM_SCENES_FLASH_H743_ASYNC_H
#include "scenes_journal_async.h"

#define SCENES_FLASH_H743_SECTOR_BYTES (128u * 1024u)
#define SCENES_FLASH_H743_BASE 0x081c0000u

/* Standalone component; also integrated by explicit SOUND_SESSION candidate.
 * No default activation.
 * Main only, one global FLASH owner, no other HAL flash users. The integrator
 * must reserve Bank-2 sectors 6/7, keep executable code/constants in Bank 1,
 * branch FLASH_IRQHandler to flash_irq and BusFault_Handler to busfault_entry.
 * Bank swap and ECC interrupts must be off. Device acceptance is still due. */
typedef struct {
    volatile uint32_t operation, result;
    uint32_t expected_return;
    _Alignas(32) uint8_t program_word[SCENE_JOURNAL_WORD_BYTES];
} scenes_flash_h743_async_t;

/* Init/rebind rejects an unconsumed operation, including a terminal IRQ whose
 * poll has not yet drained it. Both objects and IO must outlive every call. */
bool scenes_flash_h743_async_init(scenes_flash_h743_async_t *adapter);
bool scenes_flash_h743_async_make_io(scenes_flash_h743_async_t *adapter,
                                     scene_journal_io_t *journal_io,
                                     scene_journal_async_io_t *async_io);
void scenes_flash_h743_async_flash_irq(void);
void scenes_flash_h743_async_busfault_entry(void);
bool scenes_flash_h743_async_busfault(uint32_t *stacked, uint32_t exc_return);

#if defined(SCENES_FLASH_H743_HOST_MODEL)
/* Low-level hardware seams only; adapter and fault-frame logic stay real. */
bool scenes_flash_h743_model_setup(void);
bool scenes_flash_h743_model_context_ready(void);
bool scenes_flash_h743_model_quiescent(void);
bool scenes_flash_h743_model_lock(void);
bool scenes_flash_h743_model_start_erase(unsigned sector);
bool scenes_flash_h743_model_start_program(unsigned sector, unsigned offset,
                                           const uint8_t word[32]);
scene_journal_read_result_t scenes_flash_h743_model_read_word(uint32_t, uint32_t *);
void scenes_flash_h743_model_dispatch_irq(void);
void scenes_flash_h743_async_complete(uint32_t value);
void scenes_flash_h743_async_error(uint32_t value);
#endif
#endif
