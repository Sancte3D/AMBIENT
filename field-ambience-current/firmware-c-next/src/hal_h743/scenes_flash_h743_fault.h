#ifndef FAM_SCENES_FLASH_H743_FAULT_H
#define FAM_SCENES_FLASH_H743_FAULT_H
#include <stdbool.h>
#include <stdint.h>

/* Numeric ARMv7-M/H743 masks keep this exact fault matcher host-testable. */
#define SCENE_FAULT_PRECISE (1u << 9)
#define SCENE_FAULT_BFAR_VALID (1u << 15)
#define SCENE_FAULT_DBECC (1u << 26)
#define SCENE_FAULT_THUMB (1u << 24)
typedef struct {
    volatile uint32_t active, faulted;
    uint32_t address, load_pc, resume_pc;
} scenes_flash_h743_read_guard_t;
typedef struct {
    uint32_t cfsr, bfar, sr1, sr2, ecc_word;
} scenes_flash_h743_fault_snapshot_t;

/* Only Thread-mode precise data-read faults with a trustworthy stack frame,
 * exact load PC/address, and matching Bank-2 double-ECC word may resume.
 * Does not acknowledge registers; rejected faults leave frame/guard intact. */
bool scenes_flash_h743_fault_fixup(scenes_flash_h743_read_guard_t *guard,
    uint32_t *stacked, uint32_t exc_return,
    const scenes_flash_h743_fault_snapshot_t *snapshot);
#endif
