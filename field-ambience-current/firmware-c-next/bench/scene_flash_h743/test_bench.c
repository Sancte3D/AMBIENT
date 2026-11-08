#include "bench.h"
#include "scenes_flash_h743_async.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned checks, ticks, io_calls, max_io, erase_count, program_count, pending, sector, offset;
static bool reject_start, read_error;
static uint32_t corrupt_at = UINT32_MAX, now_ms = 0, request_id;
static uint8_t flash_bytes[2][SCENES_FLASH_H743_SECTOR_BYTES];
static const uint8_t *borrowed;
static uint8_t accepted[32];
#define CHECK(c) do { ++checks; if (!(c)) { fprintf(stderr,"line %d: %s\n",__LINE__,#c); exit(1); } } while (0)
bool scenes_flash_h743_model_setup(void) { return !pending; }
bool scenes_flash_h743_model_context_ready(void) { return true; }
bool scenes_flash_h743_model_quiescent(void) { return !pending; }
bool scenes_flash_h743_model_lock(void) { return true; }
bool scenes_flash_h743_model_start_erase(unsigned s) {
    if (pending || reject_start) return false;
    pending = 1; sector = s; ++erase_count; return true;
}
bool scenes_flash_h743_model_start_program(unsigned s,unsigned off,const uint8_t word[32]) {
    if (pending || reject_start) return false;
    pending = 2; sector = s; offset = off; borrowed = word;
    memcpy(accepted,word,32); ++program_count; return true;
}
scene_journal_read_result_t scenes_flash_h743_model_read_word(uint32_t address,uint32_t *value) {
    if (read_error) return SCENE_JOURNAL_READ_ERROR;
    if ((address & ~31u) == corrupt_at) return SCENE_JOURNAL_READ_CORRUPT;
    uint32_t rel = address - SCENES_FLASH_H743_BASE;
    CHECK(rel < sizeof flash_bytes && !(address & 3u));
    memcpy(value,((const uint8_t *)flash_bytes) + rel,4); return SCENE_JOURNAL_READ_OK;
}
void scenes_flash_h743_model_dispatch_irq(void) { }
static void finish(void) {
    CHECK(pending != 0);
    uint32_t value = UINT32_MAX;
    if (pending == 1) memset(flash_bytes[sector],0xff,sizeof flash_bytes[0]);
    else {
        CHECK(memcmp(borrowed,accepted,32) == 0);
        for (unsigned i=0;i<32;++i) flash_bytes[sector][offset+i] &= borrowed[i];
        value = SCENES_FLASH_H743_BASE + sector * SCENES_FLASH_H743_SECTOR_BYTES + offset;
    }
    scenes_flash_h743_async_complete(value); pending = 0;
}
static scene_journal_io_t real_journal;
static scene_journal_async_io_t real_async;
static scene_journal_read_result_t counted_read(void *c,unsigned s,unsigned o,void *p,unsigned n) {
    ++io_calls; CHECK(n <= 32); return real_journal.read(c,s,o,p,n);
}
static bool counted_erase(void *c,unsigned s) { ++io_calls; return real_async.start_erase(c,s); }
static bool counted_program(void *c,unsigned s,unsigned o,const uint8_t *p) {
    ++io_calls; return real_async.start_program(c,s,o,p);
}
static scene_async_io_result_t counted_poll(void *c) { ++io_calls; return real_async.poll(c); }
static scene_bench_t bench;
static scene_bench_mailbox_t mailbox;
static void tick(void) {
    io_calls = 0; uint32_t heartbeat = mailbox.heartbeat;
    scene_bench_tick(&bench,++now_ms);
    if (io_calls > max_io) max_io = io_calls;
    CHECK(io_calls <= 1 && mailbox.heartbeat == heartbeat + 1); ++ticks;
}
static void send(unsigned command,uint32_t argument,uint32_t key) {
    mailbox.command = command; mailbox.argument = argument; mailbox.key = key;
    mailbox.request_id = ++request_id; tick(); CHECK(mailbox.ack_id == request_id);
}
static void drain(void) {
    unsigned n = 0;
    while (mailbox.state == BENCH_RECOVERING || mailbox.state == BENCH_WRITING) {
        if (pending) finish();
        tick(); CHECK(++n < 20000);
    }
}
static uint32_t arm(void) {
    send(BENCH_ARM_WRITE,0,BENCH_ARM_KEY); CHECK(mailbox.reply == BENCH_ARMED);
    CHECK(mailbox.arm_token == (request_id ^ BENCH_TOKEN_XOR)); return mailbox.arm_token;
}
int main(void) {
    memset(flash_bytes,0xff,sizeof flash_bytes);
    memcpy(flash_bytes[1],"old raw Scene must survive first save",36);
    uint8_t legacy[36]; memcpy(legacy,flash_bytes[1],36);
    static scenes_flash_h743_async_t adapter;
    CHECK(scenes_flash_h743_async_init(&adapter));
    CHECK(scenes_flash_h743_async_make_io(&adapter,&real_journal,&real_async));
    scene_journal_io_t j = real_journal; scene_journal_async_io_t a = real_async;
    j.read = counted_read; a.start_erase = counted_erase;
    a.start_program = counted_program; a.poll = counted_poll;
    CHECK(scene_bench_init(&bench,&mailbox,&j,&a));
    CHECK(mailbox.magic == BENCH_MAGIC && mailbox.version == 1 && io_calls == 0);
    CHECK(mailbox.state == BENCH_RECOVERING && erase_count == 0 && program_count == 0);
    send(BENCH_CANCEL,0,0); CHECK(mailbox.state == BENCH_FAILED);
    CHECK(mailbox.error == BENCH_ERROR_CANCELLED && io_calls == 0);
    send(BENCH_RECOVER,0,0); drain();
    CHECK(mailbox.state == BENCH_READY && !mailbox.record_valid);
    CHECK(erase_count == 0 && program_count == 0 && memcmp(legacy,flash_bytes[1],36) == 0);
    send(BENCH_SAVE_PATTERN,7,0); CHECK(mailbox.reply == BENCH_REJECTED && !pending);
    send(BENCH_ARM_WRITE,0,1); CHECK(mailbox.reply == BENCH_REJECTED && !mailbox.arm_token);
    uint32_t token = arm();
    send(BENCH_READ_WORD,SCENES_FLASH_H743_SECTOR_BYTES,0);
    CHECK(mailbox.reply == BENCH_READ_OK && mailbox.data_length == 32 && !mailbox.arm_token);
    CHECK(memcmp(mailbox.data,legacy,32) == 0);
    send(BENCH_SAVE_PATTERN,7,token); CHECK(mailbox.reply == BENCH_REJECTED && !pending);
    token = arm();
    send(BENCH_SAVE_PATTERN,7,token); CHECK(mailbox.reply == BENCH_ACCEPTED && mailbox.state == BENCH_WRITING);
    uint32_t operation = mailbox.operation_id;
    send(BENCH_RECOVER,0,0);
    CHECK(mailbox.reply == BENCH_REJECTED && mailbox.operation_id == operation);
    drain();
    CHECK(mailbox.state == BENCH_DONE && mailbox.record_valid && mailbox.record_sequence == 0);
    CHECK(mailbox.record_length == BENCH_PAYLOAD_BYTES && mailbox.record_sector == 0);
    CHECK(memcmp(flash_bytes[0]+32,"AMBENCH1",8) == 0);
    for (unsigned i=8;i<BENCH_PAYLOAD_BYTES;++i)
        CHECK(flash_bytes[0][32+i] == (uint8_t)(7+i*29u+(i>>8)));
    CHECK(memcmp(legacy,flash_bytes[1],36) == 0);
    unsigned writes = erase_count + program_count;
    tick(); CHECK(io_calls == 0 && erase_count + program_count == writes); /* no duplicate replay */
    send(BENCH_RECOVER,0,0); drain(); CHECK(mailbox.record_valid && mailbox.record_sequence == 0);
    now_ms = UINT32_MAX-10u; token = arm(); now_ms += BENCH_ARM_MS;
    send(BENCH_SAVE_PATTERN,9,token); CHECK(mailbox.reply == BENCH_REJECTED && !pending);
    now_ms = UINT32_MAX-10u; token = arm();
    send(BENCH_SAVE_PATTERN,9,token); CHECK(mailbox.state == BENCH_WRITING && pending);
    send(BENCH_CANCEL,0,0); CHECK(mailbox.state == BENCH_WRITING && pending);
    send(BENCH_ARM_WRITE,0,BENCH_ARM_KEY);
    CHECK(mailbox.reply == BENCH_REJECTED && pending && !mailbox.arm_token);
    finish(); tick(); CHECK(mailbox.state == BENCH_FAILED && mailbox.error == BENCH_ERROR_CANCELLED);
    CHECK(!mailbox.record_valid);
    send(BENCH_ARM_WRITE,0,BENCH_ARM_KEY); CHECK(mailbox.reply == BENCH_REJECTED);
    send(BENCH_RECOVER,0,0); drain(); CHECK(mailbox.record_valid && mailbox.record_sequence == 0);
    CHECK(memcmp(flash_bytes[0]+32,"AMBENCH1",8) == 0);
    read_error = true; send(BENCH_RECOVER,0,0);
    CHECK(mailbox.state == BENCH_FAILED && mailbox.error == BENCH_ERROR_RECOVERY && !mailbox.record_valid);
    read_error = false; send(BENCH_RECOVER,0,0); drain(); CHECK(mailbox.record_valid);
    corrupt_at = SCENES_FLASH_H743_BASE; send(BENCH_READ_WORD,0,0);
    CHECK(mailbox.reply == BENCH_READ_CORRUPT && mailbox.data_length == 0);
    corrupt_at = UINT32_MAX; read_error = true; send(BENCH_READ_WORD,0,0);
    CHECK(mailbox.reply == BENCH_READ_ERROR && mailbox.data_length == 0); read_error = false;
    send(BENCH_READ_WORD,1,0); CHECK(mailbox.reply == BENCH_REJECTED && io_calls == 0);
    send(BENCH_READ_WORD,UINT32_MAX-31u,0); CHECK(mailbox.reply == BENCH_REJECTED && io_calls == 0);
    token = arm(); reject_start = true; send(BENCH_SAVE_PATTERN,42,token);
    CHECK(mailbox.state == BENCH_FAILED && mailbox.error == BENCH_ERROR_WRITE && !pending);
    reject_start = false; send(BENCH_RECOVER,0,0); drain(); CHECK(mailbox.record_valid);
    (void)arm(); send(BENCH_DISARM,0,0); CHECK(!mailbox.arm_token && mailbox.reply == BENCH_ACCEPTED);
    (void)arm(); send(UINT32_MAX,0,0); CHECK(!mailbox.arm_token && mailbox.reply == BENCH_REJECTED);
    printf("{\"status\":\"passed\",\"checks\":%u,\"ticks\":%u,\"erase_starts\":%u,\"program_starts\":%u,\"max_io_per_tick\":%u}\n",
        checks,ticks,erase_count,program_count,max_io);
    return 0;
}
