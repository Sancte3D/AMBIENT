#include "scenes_flash_h743_async.h"
#include "scenes_flash_h743_fault.h"
#include "scenes_journal_recovery.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, read_cases, fault_cases, setup_calls, lock_calls, irq_calls;
#define CHECK(test) do { ++checks; if (!(test)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #test); exit(1); } } while (0)
static uint8_t flash_bytes[2][SCENES_FLASH_H743_SECTOR_BYTES];
static bool reject_start, context_ok = true, quiescent = true, lock_ok = true;
static uint32_t corrupt_word = UINT32_MAX, error_word = UINT32_MAX;
static unsigned pending, last_sector, last_offset, erase_calls, program_calls;
static const uint8_t *borrowed;
static uint8_t accepted[32];

bool scenes_flash_h743_model_setup(void) { ++setup_calls; return context_ok && quiescent; }
bool scenes_flash_h743_model_context_ready(void) { return context_ok; }
bool scenes_flash_h743_model_quiescent(void) { return quiescent; }
bool scenes_flash_h743_model_lock(void) { ++lock_calls; return lock_ok; }
bool scenes_flash_h743_model_start_erase(unsigned sector) {
    if (reject_start || !quiescent) return false;
    ++erase_calls; last_sector = sector; pending = 1; quiescent = false;
    return true;
}
bool scenes_flash_h743_model_start_program(unsigned sector, unsigned offset,
                                           const uint8_t word[32]) {
    if (reject_start || !quiescent) return false;
    ++program_calls; last_sector = sector; last_offset = offset;
    CHECK(((uintptr_t)word & 31u) == 0u);
    memcpy(accepted, word, 32); borrowed = word; pending = 2; quiescent = false;
    return true;
}
scene_journal_read_result_t scenes_flash_h743_model_read_word(uint32_t address,
                                                              uint32_t *value) {
    uint32_t word = address & ~31u;
    if (word == corrupt_word) return SCENE_JOURNAL_READ_CORRUPT;
    if (word == error_word) return SCENE_JOURNAL_READ_ERROR;
    CHECK(address >= SCENES_FLASH_H743_BASE);
    uint32_t relative = address - SCENES_FLASH_H743_BASE;
    unsigned sector = relative / SCENES_FLASH_H743_SECTOR_BYTES;
    unsigned offset = relative % SCENES_FLASH_H743_SECTOR_BYTES;
    CHECK(sector < 2 && offset <= SCENES_FLASH_H743_SECTOR_BYTES - 4);
    memcpy(value, flash_bytes[sector] + offset, 4);
    return SCENE_JOURNAL_READ_OK;
}
void scenes_flash_h743_model_dispatch_irq(void) { ++irq_calls; }
static void finish(bool ok) {
    CHECK(pending != 0);
    if (pending == 1 && ok) memset(flash_bytes[last_sector], 0xff, sizeof flash_bytes[0]);
    if (pending == 2) {
        CHECK(memcmp(borrowed, accepted, 32) == 0);
        if (ok) for (unsigned i = 0; i < 32; ++i)
            flash_bytes[last_sector][last_offset + i] &= borrowed[i];
    }
    uint32_t result = pending == 1 ? UINT32_MAX :
        SCENES_FLASH_H743_BASE + last_sector * SCENES_FLASH_H743_SECTOR_BYTES + last_offset;
    if (ok) scenes_flash_h743_async_complete(result);
    else scenes_flash_h743_async_error(result);
    pending = 0; quiescent = true;
}

static void fault_frames(void) {
    const uint32_t returns[] = {0xfffffff9u, 0xfffffffdu, 0xffffffe9u, 0xffffffedu};
    for (unsigned f = 0; f < 4; ++f) for (unsigned defect = 0; defect < 82; ++defect) {
        uint32_t frame[26] = {0}, old[26];
        /* PM0253 Figure11 fixture: core[0..7], then optional FP[8..25].
         * Distinct FP words catch an erroneous +18-word core-frame offset. */
        const unsigned start = 0;
        if (f >= 2) for (unsigned i = 8; i < 26; ++i) frame[i] = 0xf0000000u + i;
        scenes_flash_h743_read_guard_t guard = {1, 0, SCENES_FLASH_H743_BASE + 36u,
            0x08001000u, 0x08001002u};
        scenes_flash_h743_fault_snapshot_t snapshot = {
            SCENE_FAULT_PRECISE | SCENE_FAULT_BFAR_VALID, guard.address,
            0, SCENE_FAULT_DBECC, guard.address & ~31u};
        uint32_t ret = returns[f];
        frame[start + 6] = guard.load_pc;
        frame[start + 7] = SCENE_FAULT_THUMB;
        if (defect >= 1 && defect <= 32) snapshot.cfsr ^= 1u << (defect - 1);
        else if (defect >= 33 && defect <= 64) frame[start + 6] ^= 1u << (defect - 33);
        else if (defect == 65) guard.active = 0;
        else if (defect == 66) snapshot.bfar += 4;
        else if (defect == 67) snapshot.sr1 = SCENE_FAULT_DBECC;
        else if (defect == 68) snapshot.sr2 = 0;
        else if (defect == 69) snapshot.ecc_word += 32;
        else if (defect == 70) ret = 0xfffffff1u;
        else if (defect == 71) ret = 0xffffffe1u;
        else if (defect == 72) ret = 0;
        else if (defect == 73) frame[start + 7] = 0;
        else if (defect == 74) frame[start + 7] |= 3;
        else if (defect == 75) guard.address = snapshot.bfar = 0x081bfffcu;
        else if (defect == 76) guard.address = snapshot.bfar = 0x08200000u;
        else if (defect == 77) guard.address = snapshot.bfar = SCENES_FLASH_H743_BASE + 1u;
        else if (defect == 78) frame[start + 7] |= 0x200u; /* valid stack alignment bit */
        else if (defect == 79) snapshot.sr2 |= 1u << 25; /* corrected ECC in same word */
        memcpy(old, frame, sizeof frame);
        bool expected = defect == 0 || defect == 78 || defect == 79;
        bool fixed = scenes_flash_h743_fault_fixup(defect == 80 ? NULL : &guard,
            defect == 81 ? NULL : frame, ret, &snapshot);
        CHECK(fixed == expected);
        CHECK(guard.faulted == (expected ? 1u : 0u));
        if (expected) old[start + 6] = guard.resume_pc;
        CHECK(memcmp(frame, old, sizeof frame) == 0);
        ++fault_cases;
    }
}

int main(void) {
    fault_frames();
    for (unsigned s = 0; s < 2; ++s) for (unsigned i = 0; i < sizeof flash_bytes[s]; ++i)
        flash_bytes[s][i] = (uint8_t)(i * 29u + s * 71u);
    static scenes_flash_h743_async_t adapter, other;
    scene_journal_io_t journal = {0}; scene_journal_async_io_t async = {0};
    CHECK(!scenes_flash_h743_async_init(NULL));
    CHECK(scenes_flash_h743_async_init(&adapter));
    CHECK(scenes_flash_h743_async_make_io(&adapter, &journal, &async));
    CHECK(setup_calls == 1 && journal.context == &adapter && async.context == &adapter);
    CHECK(journal.sector_bytes == SCENES_FLASH_H743_SECTOR_BYTES);
    CHECK(!scenes_flash_h743_async_make_io(&other, &journal, &async));
    uint8_t out[32];
    for (unsigned sector = 0; sector < 2; ++sector) for (unsigned offset = 0; offset < 70; ++offset)
        for (unsigned length = 1; length <= 32; ++length) {
            CHECK(journal.read(journal.context, sector, offset, out, length) == SCENE_JOURNAL_READ_OK);
            CHECK(memcmp(out, flash_bytes[sector] + offset, length) == 0); ++read_cases;
        }
    for (unsigned s = 0; s < 2; ++s) {
        CHECK(journal.read(journal.context, s, SCENES_FLASH_H743_SECTOR_BYTES - 32, out, 32) == SCENE_JOURNAL_READ_OK);
        CHECK(memcmp(out, flash_bytes[s] + SCENES_FLASH_H743_SECTOR_BYTES - 32, 32) == 0);
    }
    CHECK(journal.read(journal.context, 2, 0, out, 1) == SCENE_JOURNAL_READ_ERROR);
    CHECK(journal.read(journal.context, 0, 0, out, 0) == SCENE_JOURNAL_READ_ERROR);
    CHECK(journal.read(journal.context, 0, 0, out, 33) == SCENE_JOURNAL_READ_ERROR);
    CHECK(journal.read(journal.context, 0, UINT32_MAX, out, 1) == SCENE_JOURNAL_READ_ERROR);
    CHECK(journal.read(journal.context, 0, SCENES_FLASH_H743_SECTOR_BYTES, out, 1) == SCENE_JOURNAL_READ_ERROR);
    memset(out, 0xa5, 32);
    corrupt_word = SCENES_FLASH_H743_BASE + 32;
    CHECK(journal.read(journal.context, 0, 31, out, 2) == SCENE_JOURNAL_READ_CORRUPT);
    for (unsigned i = 0; i < 32; ++i) CHECK(out[i] == 0xa5);
    corrupt_word = UINT32_MAX; error_word = SCENES_FLASH_H743_BASE + 32;
    CHECK(journal.read(journal.context, 0, 31, out, 2) == SCENE_JOURNAL_READ_ERROR);
    for (unsigned i = 0; i < 32; ++i) CHECK(out[i] == 0xa5);
    error_word = UINT32_MAX;
    CHECK(!journal.erase(journal.context, 0));
    CHECK(!journal.program(journal.context, 0, 0, out));
    CHECK(async.start_erase(async.context, 1));
    CHECK(async.poll(async.context) == SCENE_ASYNC_IO_BUSY);
    CHECK(!async.start_erase(async.context, 0));
    CHECK(!scenes_flash_h743_async_init(&other));
    CHECK(!scenes_flash_h743_async_make_io(&adapter, &journal, &async));
    CHECK(journal.read(journal.context, 0, 0, out, 1) == SCENE_JOURNAL_READ_ERROR);
    scenes_flash_h743_async_complete(UINT32_MAX);
    CHECK(async.poll(async.context) == SCENE_ASYNC_IO_BUSY); /* IRQ/HAL not drained */
    CHECK(lock_calls == 0);
    finish(true);
    CHECK(!scenes_flash_h743_async_init(&other));
    CHECK(async.poll(async.context) == SCENE_ASYNC_IO_DONE);
    CHECK(lock_calls == 1);
    CHECK(async.poll(async.context) == SCENE_ASYNC_IO_ERROR);

    uint8_t word[32]; memset(word, 0x69, 32);
    CHECK(!async.start_program(async.context, 0, 1, word));
    CHECK(!async.start_program(async.context, 2, 0, word));
    CHECK(!async.start_program(async.context, 0, SCENES_FLASH_H743_SECTOR_BYTES, word));
    for (unsigned order = 0; order < 2; ++order) {
        CHECK(async.start_program(async.context, 0, 96, word));
        memset(word, 0x96, 32); /* accepted program word stays immutable */
        if (order == 0) {
            scenes_flash_h743_async_complete(adapter.expected_return);
            scenes_flash_h743_async_error(adapter.expected_return);
        } else {
            scenes_flash_h743_async_error(adapter.expected_return);
            scenes_flash_h743_async_complete(adapter.expected_return);
        }
        CHECK(async.poll(async.context) == SCENE_ASYNC_IO_BUSY);
        finish(true);
        context_ok = false;
        CHECK(async.poll(async.context) == SCENE_ASYNC_IO_BUSY);
        context_ok = true;
        CHECK(async.poll(async.context) == SCENE_ASYNC_IO_ERROR);
    }
    CHECK(async.start_erase(async.context, 0));
    scenes_flash_h743_async_complete(0); /* unrelated completion cannot succeed */
    finish(true);
    CHECK(async.poll(async.context) == SCENE_ASYNC_IO_ERROR);
    CHECK(async.start_erase(async.context, 0)); finish(true); lock_ok = false;
    CHECK(async.poll(async.context) == SCENE_ASYNC_IO_ERROR); lock_ok = true;
    reject_start = true; CHECK(!async.start_erase(async.context, 0)); reject_start = false;
    context_ok = false;
    CHECK(!async.start_erase(async.context, 0));
    CHECK(journal.read(journal.context, 0, 0, out, 1) == SCENE_JOURNAL_READ_ERROR);
    context_ok = true;
    scenes_flash_h743_async_flash_irq(); CHECK(irq_calls == 1);

    /* Real prepared recovery + writer use this exact adapter, not direct IO. */
    memset(flash_bytes, 0xff, sizeof flash_bytes);
    scene_journal_t store;
    scene_journal_configure(&store, &journal);
    scene_journal_recovery_t recovery;
    scene_journal_recovery_configure(&recovery, &store);
    CHECK(scene_journal_recovery_begin(&recovery));
    unsigned steps = 0;
    while (scene_journal_recovery_step(&recovery) == SCENE_RECOVERY_BUSY) CHECK(++steps < 20000);
    CHECK(recovery.status == SCENE_RECOVERY_DONE && !store.has_record);
    scene_journal_async_t writer;
    scene_journal_async_configure(&writer, &store, &async);
    uint8_t payload[368]; for (unsigned i = 0; i < sizeof payload; ++i) payload[i] = (uint8_t)i;
    CHECK(scene_journal_async_begin(&writer, payload, sizeof payload));
    steps = 0;
    while (scene_journal_async_step(&writer) == SCENE_ASYNC_BUSY) {
        if (pending) finish(true);
        CHECK(++steps < 20000);
    }
    CHECK(writer.status == SCENE_ASYNC_DONE && store.has_record);
    CHECK(memcmp(flash_bytes[store.sector] + 32, payload, sizeof payload) == 0);
    CHECK(scene_journal_recovery_begin(&recovery)); steps = 0;
    while (scene_journal_recovery_step(&recovery) == SCENE_RECOVERY_BUSY) CHECK(++steps < 20000);
    CHECK(recovery.status == SCENE_RECOVERY_DONE && store.payload_length == sizeof payload);
    CHECK(scene_journal_async_begin(&writer, payload, sizeof payload));
    CHECK(scene_journal_async_step(&writer) == SCENE_ASYNC_BUSY && pending);
    CHECK(scene_journal_async_cancel(&writer));
    CHECK(scene_journal_async_step(&writer) == SCENE_ASYNC_BUSY);
    finish(true);
    CHECK(scene_journal_async_step(&writer) == SCENE_ASYNC_FAILED);
    CHECK(writer.error == SCENE_ASYNC_ERROR_CANCELLED && !store.initialized);
    CHECK(scenes_flash_h743_async_init(&adapter));
    printf("{\"checks\":%u,\"read_cases\":%u,\"fault_frame_cases\":%u,\"erase_starts\":%u,\"program_starts\":%u,\"status\":\"passed\"}\n",
        checks, read_cases, fault_cases, erase_calls, program_calls);
    return 0;
}
