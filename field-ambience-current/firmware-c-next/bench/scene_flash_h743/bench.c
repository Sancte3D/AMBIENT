#include "bench.h"
#include <string.h>

static bool busy(const scene_bench_t *b) {
    return b->writer.status == SCENE_ASYNC_BUSY || b->recovery.status == SCENE_RECOVERY_BUSY;
}
static void disarm(scene_bench_t *b) {
    b->armed_request = 0;
    b->mailbox->arm_token = 0;
}
static void publish_record(scene_bench_t *b) {
    volatile scene_bench_mailbox_t *m = b->mailbox;
    scene_journal_t *j = &b->journal;
    m->record_valid = j->initialized && j->has_record;
    m->record_sector = j->sector; m->record_slot = j->slot;
    m->record_sequence = j->sequence; m->record_length = j->payload_length;
}
static void request(scene_bench_t *b, uint32_t now) {
    volatile scene_bench_mailbox_t *m = b->mailbox;
    uint32_t id = m->request_id;
    if (!id || id == m->ack_id) return;
    uint32_t command = m->command, argument = m->argument, key = m->key;
    if (m->request_id != id) return;
    m->ack_id = id; m->reply = BENCH_REJECTED;
    if (command == BENCH_CANCEL) {
        disarm(b);
        bool accepted = b->writer.status == SCENE_ASYNC_BUSY
                      ? scene_journal_async_cancel(&b->writer)
                      : scene_journal_recovery_cancel(&b->recovery);
        if (accepted) m->reply = BENCH_ACCEPTED;
        return;
    }
    if (command == BENCH_DISARM) { disarm(b); m->reply = BENCH_ACCEPTED; return; }
    if (busy(b)) { disarm(b); return; }
    if (command == BENCH_ARM_WRITE) {
        disarm(b);
        if (key != BENCH_ARM_KEY || !b->journal.initialized) return;
        uint32_t token = id ^ BENCH_TOKEN_XOR;
        if (!token) return;
        b->armed_at = now; b->armed_request = id;
        m->arm_token = token; m->reply = BENCH_ARMED;
        return;
    }
    if (command == BENCH_SAVE_PATTERN) {
        bool allowed = b->armed_request && (uint32_t)(now - b->armed_at) < BENCH_ARM_MS &&
                       key == m->arm_token && key != 0 && b->journal.initialized;
        disarm(b); /* Each attempted SAVE consumes the one-shot arm. */
        if (!allowed) return;
        for (unsigned i = 0; i < sizeof b->payload; ++i)
            b->payload[i] = (uint8_t)(argument + i * 29u + (i >> 8));
        memcpy(b->payload, "AMBENCH1", 8); /* Bench pattern, NOT an SCN7 Scene. */
        if (!scene_journal_async_begin(&b->writer, b->payload, sizeof b->payload)) return;
        m->operation_id = id; m->state = BENCH_WRITING; m->error = BENCH_ERROR_NONE;
        m->reply = BENCH_ACCEPTED; m->steps = 0;
        return;
    }
    disarm(b);
    if (command == BENCH_RECOVER) {
        if (!scene_journal_recovery_begin(&b->recovery)) return;
        m->operation_id = id; m->state = BENCH_RECOVERING; m->error = BENCH_ERROR_NONE;
        m->reply = BENCH_ACCEPTED; m->steps = 0;
        publish_record(b);
    } else if (command == BENCH_READ_WORD) {
        m->data_length = 0;
        if ((argument & 31u) || argument > 2u * b->journal.io.sector_bytes - 32u) return;
        unsigned sector = argument / b->journal.io.sector_bytes;
        unsigned offset = argument % b->journal.io.sector_bytes;
        uint8_t word[32];
        scene_journal_read_result_t r = b->journal.io.read(b->journal.io.context,
                                                         sector, offset, word, 32);
        if (r == SCENE_JOURNAL_READ_OK) {
            for (unsigned i = 0; i < 32; ++i) m->data[i] = word[i];
            m->data_length = 32; m->reply = BENCH_READ_OK;
        } else m->reply = r == SCENE_JOURNAL_READ_CORRUPT
                       ? BENCH_READ_CORRUPT : BENCH_READ_ERROR;
    }
}
bool scene_bench_init(scene_bench_t *b, volatile scene_bench_mailbox_t *m,
                       const scene_journal_io_t *journal_io,
                       const scene_journal_async_io_t *async_io) {
    if (!b || !m || !journal_io || !async_io || !journal_io->read ||
        !journal_io->erase || !journal_io->program || !async_io->start_erase ||
        !async_io->start_program || !async_io->poll ||
        journal_io->sector_bytes < SCENE_JOURNAL_RECORD_BYTES ||
        journal_io->sector_bytes > 128u * 1024u) return false;
    memset(b, 0, sizeof *b);
    /* Called once at boot with a fresh object, never an abort/reinit API. */
    b->mailbox = m;
    volatile uint8_t *bytes = (volatile uint8_t *)m;
    for (unsigned i = 0; i < sizeof *m; ++i) bytes[i] = 0;
    m->magic = BENCH_MAGIC; m->version = BENCH_VERSION;
    scene_journal_configure(&b->journal, journal_io);
    scene_journal_async_configure(&b->writer, &b->journal, async_io);
    scene_journal_recovery_configure(&b->recovery, &b->journal);
    if (!scene_journal_recovery_begin(&b->recovery)) return false;
    m->state = BENCH_RECOVERING;
    return true;
}
void scene_bench_tick(scene_bench_t *b, uint32_t now) {
    if (!b || !b->mailbox) return;
    volatile scene_bench_mailbox_t *m = b->mailbox;
    ++m->heartbeat; m->tick_ms = now;
    if (b->armed_request && (uint32_t)(now - b->armed_at) >= BENCH_ARM_MS) disarm(b);
    request(b, now);
    if (b->recovery.status == SCENE_RECOVERY_BUSY) {
        ++m->steps;
        scene_recovery_status_t r = scene_journal_recovery_step(&b->recovery);
        if (r == SCENE_RECOVERY_DONE) m->state = BENCH_READY;
        else if (r == SCENE_RECOVERY_FAILED) {
            m->state = BENCH_FAILED;
            m->error = b->recovery.error == SCENE_RECOVERY_ERROR_CANCELLED
                     ? BENCH_ERROR_CANCELLED : BENCH_ERROR_RECOVERY;
        }
    } else if (b->writer.status == SCENE_ASYNC_BUSY) {
        ++m->steps;
        scene_async_status_t r = scene_journal_async_step(&b->writer);
        if (r == SCENE_ASYNC_DONE) m->state = BENCH_DONE;
        else if (r == SCENE_ASYNC_FAILED) {
            m->state = BENCH_FAILED;
            m->error = b->writer.error == SCENE_ASYNC_ERROR_CANCELLED
                     ? BENCH_ERROR_CANCELLED : BENCH_ERROR_WRITE;
        }
    }
    publish_record(b);
}
