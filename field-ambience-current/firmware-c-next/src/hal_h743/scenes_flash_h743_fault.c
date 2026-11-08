#include "scenes_flash_h743_fault.h"

bool scenes_flash_h743_fault_fixup(scenes_flash_h743_read_guard_t *guard,
    uint32_t *stacked, uint32_t exc_return,
    const scenes_flash_h743_fault_snapshot_t *snapshot) {
    if (!guard || !stacked || !snapshot || !guard->active ||
        snapshot->cfsr != (SCENE_FAULT_PRECISE | SCENE_FAULT_BFAR_VALID) ||
        snapshot->bfar != guard->address || (guard->address & 3u) != 0u ||
        guard->address < 0x081c0000u || guard->address >= 0x08200000u ||
        (snapshot->sr1 & SCENE_FAULT_DBECC) != 0u ||
        (snapshot->sr2 & SCENE_FAULT_DBECC) == 0u ||
        snapshot->ecc_word != (guard->address & ~31u) ||
        (exc_return != 0xfffffff9u && exc_return != 0xfffffffdu &&
         exc_return != 0xffffffe9u && exc_return != 0xffffffedu)) return false;
    /* PM0253 Rev6 Figure11: R0..xPSR start at SP in BOTH frame types.
     * The extended frame adds S0..S15/FPSCR ABOVE the eight core registers. */
    if (stacked[6] != guard->load_pc ||
        (stacked[7] & (SCENE_FAULT_THUMB | 0x1ffu)) != SCENE_FAULT_THUMB)
        return false;
    guard->faulted = 1u;
    stacked[6] = guard->resume_pc;
    return true;
}
