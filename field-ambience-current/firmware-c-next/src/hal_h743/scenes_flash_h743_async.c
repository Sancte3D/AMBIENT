/* Prepared standalone H743 IO; see the header for integration prerequisites. */
#include "scenes_flash_h743_async.h"
#include "scenes_flash_h743_fault.h"
#include <string.h>

enum { OP_IDLE, OP_ERASE, OP_PROGRAM };
enum { RESULT_NONE, RESULT_BUSY, RESULT_DONE, RESULT_ERROR };
static scenes_flash_h743_async_t *s_owner;

#if !defined(SCENES_FLASH_H743_HOST_MODEL)
#include "stm32h7xx_hal.h"
static scenes_flash_h743_read_guard_t s_guard;
extern uint8_t scenes_flash_h743_guarded_load, scenes_flash_h743_guarded_resume;
extern uint32_t scenes_flash_h743_guarded_read(uint32_t address);

_Static_assert(SCB_CFSR_PRECISERR_Msk == SCENE_FAULT_PRECISE, "precise mask");
_Static_assert(SCB_CFSR_BFARVALID_Msk == SCENE_FAULT_BFAR_VALID, "BFAR mask");
_Static_assert(FLASH_SR_DBECCERR == SCENE_FAULT_DBECC, "ECC mask");

static bool platform_context_ready(void) {
    return (__get_CONTROL() & 1u) == 0u && __get_IPSR() == 0u && __get_PRIMASK() == 0u &&
           __get_FAULTMASK() == 0u && __get_BASEPRI() == 0u &&
           (FLASH->OPTCR & FLASH_OPTCR_SWAP_BANK) == 0u &&
           (FLASH->CR2 & (FLASH_CR_SNECCERRIE | FLASH_CR_DBECCERRIE)) == 0u;
}
static bool platform_quiescent(void) {
    return (FLASH->SR2 & (FLASH_SR_QW | FLASH_SR_BSY | FLASH_SR_WBNE)) == 0u &&
           pFlash.Lock == HAL_UNLOCKED && pFlash.ProcedureOnGoing == FLASH_PROC_NONE;
}
static bool platform_setup(void) {
    if (!platform_context_ready() || !platform_quiescent()) return false;
    SCB->SHCSR |= SCB_SHCSR_BUSFAULTENA_Msk;
    HAL_NVIC_SetPriority(FLASH_IRQn, 15u, 0u);
    HAL_NVIC_EnableIRQ(FLASH_IRQn);
    __DSB(); __ISB();
    return true;
}
static bool platform_lock(void) { return HAL_FLASHEx_Lock_Bank2() == HAL_OK; }
static bool platform_start_ready(void) {
    /* HAL *_IT still calls its wait helper before starting. Under exclusive
     * Main ownership this precheck ensures that helper has nothing to wait for. */
    return platform_quiescent() && (FLASH->CR2 & FLASH_CR_LOCK) != 0u;
}
static bool platform_start_erase(unsigned sector) {
    if (!platform_start_ready() || HAL_FLASHEx_Unlock_Bank2() != HAL_OK) return false;
    FLASH_EraseInitTypeDef erase = {0};
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Banks = FLASH_BANK_2;
    erase.Sector = FLASH_SECTOR_6 + sector;
    erase.NbSectors = 1u;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_4;
    if (HAL_FLASHEx_Erase_IT(&erase) == HAL_OK) return true;
    (void)platform_lock();
    return false;
}
static bool platform_start_program(unsigned sector, unsigned offset,
                                    const uint8_t word[32]) {
    uint32_t address = SCENES_FLASH_H743_BASE +
                       sector * SCENES_FLASH_H743_SECTOR_BYTES + offset;
    if (!platform_start_ready() || HAL_FLASHEx_Unlock_Bank2() != HAL_OK) return false;
    if (HAL_FLASH_Program_IT(FLASH_TYPEPROGRAM_FLASHWORD, address,
                            (uint32_t)(uintptr_t)word) == HAL_OK) return true;
    (void)platform_lock();
    return false;
}
static uint32_t ecc_word_address(void) {
    return (FLASH->ECC_FA2 & FLASH_ECC_FA_FAIL_ECC_ADDR) * 32u + FLASH_BANK2_BASE;
}
static scene_journal_read_result_t platform_read_word(uint32_t address, uint32_t *value) {
    /* A preexisting unowned ECC fault is unavailable IO, never silently cleared.
     * Clear only a previous correctable flag so ECC_FA2 can identify this read. */
    if ((SCB->SHCSR & SCB_SHCSR_BUSFAULTENA_Msk) == 0u || SCB->CFSR != 0u ||
        (FLASH->SR1 & FLASH_SR_DBECCERR) != 0u ||
        (FLASH->SR2 & FLASH_SR_DBECCERR) != 0u) return SCENE_JOURNAL_READ_ERROR;
    __HAL_FLASH_CLEAR_FLAG_BANK2(FLASH_FLAG_SNECCERR_BANK2);
    s_guard.address = address;
    s_guard.load_pc = (uint32_t)(uintptr_t)&scenes_flash_h743_guarded_load;
    s_guard.resume_pc = (uint32_t)(uintptr_t)&scenes_flash_h743_guarded_resume;
    s_guard.faulted = 0u;
    s_guard.active = 1u;
    __DSB();
    uint32_t loaded = scenes_flash_h743_guarded_read(address);
    __DSB();
    s_guard.active = 0u;
    if (s_guard.faulted) return SCENE_JOURNAL_READ_CORRUPT;
    if ((FLASH->SR2 & FLASH_SR_DBECCERR) != 0u) return SCENE_JOURNAL_READ_ERROR;
    __HAL_FLASH_CLEAR_FLAG_BANK2(FLASH_FLAG_SNECCERR_BANK2);
    *value = loaded;
    return SCENE_JOURNAL_READ_OK;
}
#else
static bool platform_context_ready(void) { return scenes_flash_h743_model_context_ready(); }
static bool platform_quiescent(void) { return scenes_flash_h743_model_quiescent(); }
static bool platform_setup(void) { return scenes_flash_h743_model_setup(); }
static bool platform_lock(void) { return scenes_flash_h743_model_lock(); }
static bool platform_start_erase(unsigned sector) {
    return scenes_flash_h743_model_start_erase(sector);
}
static bool platform_start_program(unsigned sector, unsigned offset, const uint8_t word[32]) {
    return scenes_flash_h743_model_start_program(sector, offset, word);
}
static scene_journal_read_result_t platform_read_word(uint32_t address, uint32_t *value) {
    return scenes_flash_h743_model_read_word(address, value);
}
#endif

/* Callbacks only record an event. HAL may report EOP and error in one dispatch;
 * ERROR dominates. No buffer release/relock before the terminal quiescent poll. */
void scenes_flash_h743_async_complete(uint32_t value) {
    if (!s_owner || s_owner->operation == OP_IDLE) return;
    if (value != s_owner->expected_return) s_owner->result = RESULT_ERROR;
    else if (s_owner->result == RESULT_BUSY) s_owner->result = RESULT_DONE;
}
void scenes_flash_h743_async_error(uint32_t value) {
    (void)value;
    if (s_owner && s_owner->operation != OP_IDLE) s_owner->result = RESULT_ERROR;
}

static bool idle_owner(scenes_flash_h743_async_t *adapter) {
    return adapter && adapter == s_owner && adapter->operation == OP_IDLE;
}
static scene_journal_read_result_t adapter_read(void *context, unsigned sector,
                                                unsigned offset, void *out, unsigned length) {
    if (!idle_owner(context) || !out || sector >= 2u || length == 0u || length > 32u ||
        offset > SCENES_FLASH_H743_SECTOR_BYTES ||
        length > SCENES_FLASH_H743_SECTOR_BYTES - offset ||
        !platform_context_ready() || !platform_quiescent()) return SCENE_JOURNAL_READ_ERROR;
    uint32_t address = SCENES_FLASH_H743_BASE +
                       sector * SCENES_FLASH_H743_SECTOR_BYTES + offset;
#if !defined(SCENES_FLASH_H743_HOST_MODEL)
    uint32_t first = address & ~31u;
    unsigned bytes = ((address - first) + length + 31u) & ~31u;
    SCB_InvalidateDCache_by_Addr((void *)(uintptr_t)first, (int32_t)bytes);
#endif
    uint8_t scratch[32];
    unsigned copied = 0u;
    while (copied < length) {
        uint32_t current = address + copied, value = 0u;
        scene_journal_read_result_t result = platform_read_word(current & ~3u, &value);
        if (result != SCENE_JOURNAL_READ_OK) return result;
        unsigned start = current & 3u, count = 4u - start;
        if (count > length - copied) count = length - copied;
        memcpy(scratch + copied, (const uint8_t *)&value + start, count);
        copied += count;
    }
    memcpy(out, scratch, length); /* No partially returned buffer on error. */
    return SCENE_JOURNAL_READ_OK;
}
static bool unsupported_sync_erase(void *context, unsigned sector) {
    (void)context; (void)sector; return false;
}
static bool unsupported_sync_program(void *context, unsigned sector, unsigned offset,
                                      const uint8_t *word) {
    (void)context; (void)sector; (void)offset; (void)word; return false;
}
static bool adapter_start_erase(void *context, unsigned sector) {
    scenes_flash_h743_async_t *adapter = context;
    if (!idle_owner(adapter) || sector >= 2u || !platform_context_ready()) return false;
    adapter->expected_return = 0xffffffffu;
    adapter->operation = OP_ERASE; adapter->result = RESULT_BUSY;
    if (platform_start_erase(sector)) return true;
    adapter->operation = OP_IDLE; adapter->result = RESULT_NONE;
    return false;
}
static bool adapter_start_program(void *context, unsigned sector, unsigned offset,
                                   const uint8_t *word) {
    scenes_flash_h743_async_t *adapter = context;
    if (!idle_owner(adapter) || !word || sector >= 2u || (offset & 31u) != 0u ||
        offset > SCENES_FLASH_H743_SECTOR_BYTES - 32u || !platform_context_ready()) return false;
    memmove(adapter->program_word, word, 32u);
    adapter->expected_return = SCENES_FLASH_H743_BASE +
                              sector * SCENES_FLASH_H743_SECTOR_BYTES + offset;
    adapter->operation = OP_PROGRAM; adapter->result = RESULT_BUSY;
    if (platform_start_program(sector, offset, adapter->program_word)) return true;
    adapter->operation = OP_IDLE; adapter->result = RESULT_NONE;
    return false;
}
static scene_async_io_result_t adapter_poll(void *context) {
    scenes_flash_h743_async_t *adapter = context;
    if (!adapter || adapter != s_owner || adapter->operation == OP_IDLE)
        return SCENE_ASYNC_IO_ERROR;
    /* An invalid caller must not tell the writer an in-flight word is free. */
    if (!platform_context_ready() || adapter->result == RESULT_BUSY ||
        !platform_quiescent()) return SCENE_ASYNC_IO_BUSY;
    bool ok = platform_lock();
    scene_async_io_result_t result = ok && adapter->result == RESULT_DONE
                                  ? SCENE_ASYNC_IO_DONE : SCENE_ASYNC_IO_ERROR;
    adapter->operation = OP_IDLE; adapter->result = RESULT_NONE;
    return result;
}

bool scenes_flash_h743_async_init(scenes_flash_h743_async_t *adapter) {
    if (!adapter || (s_owner && s_owner->operation != OP_IDLE) || !platform_setup()) return false;
    memset(adapter, 0, sizeof *adapter); s_owner = adapter;
    return true;
}
bool scenes_flash_h743_async_make_io(scenes_flash_h743_async_t *adapter,
                                     scene_journal_io_t *journal_io,
                                     scene_journal_async_io_t *async_io) {
    if (!idle_owner(adapter) || !journal_io || !async_io) return false;
    /* Recovery requires nonnull sync hooks; accidental sync writes fail closed. */
    *journal_io = (scene_journal_io_t){adapter, SCENES_FLASH_H743_SECTOR_BYTES,
        adapter_read, unsupported_sync_erase, unsupported_sync_program};
    *async_io = (scene_journal_async_io_t){adapter, adapter_start_erase,
        adapter_start_program, adapter_poll};
    return true;
}
void scenes_flash_h743_async_flash_irq(void) {
#if defined(SCENES_FLASH_H743_HOST_MODEL)
    scenes_flash_h743_model_dispatch_irq();
#else
    HAL_FLASH_IRQHandler();
#endif
}
bool scenes_flash_h743_async_busfault(uint32_t *stacked, uint32_t exc_return) {
#if defined(SCENES_FLASH_H743_HOST_MODEL)
    (void)stacked; (void)exc_return; return false;
#else
    scenes_flash_h743_fault_snapshot_t snapshot = {SCB->CFSR, SCB->BFAR,
        FLASH->SR1, FLASH->SR2, ecc_word_address()};
    if (!scenes_flash_h743_fault_fixup(&s_guard, stacked, exc_return, &snapshot)) return false;
    __HAL_FLASH_CLEAR_FLAG_BANK2(FLASH_FLAG_DBECCERR_BANK2 | FLASH_FLAG_SNECCERR_BANK2);
    SCB->CFSR = SCENE_FAULT_PRECISE | SCENE_FAULT_BFAR_VALID;
    __DSB(); __ISB();
    return true;
#endif
}

#if !defined(SCENES_FLASH_H743_HOST_MODEL)
/* Assembly entry branches here with the original EXC_RETURN in LR. */
void scenes_flash_h743_busfault_dispatch(uint32_t *stacked, uint32_t exc_return) {
    if (scenes_flash_h743_async_busfault(stacked, exc_return)) return;
    for (;;) { }
}
void HAL_FLASH_EndOfOperationCallback(uint32_t value) { scenes_flash_h743_async_complete(value); }
void HAL_FLASH_OperationErrorCallback(uint32_t value) { scenes_flash_h743_async_error(value); }
#else
void scenes_flash_h743_async_busfault_entry(void) { }
#endif
