#ifndef AMBIENT_SCENE_FLASH_BENCH_H
#define AMBIENT_SCENE_FLASH_BENCH_H
#include "scenes_journal_async.h"
#include "scenes_journal_recovery.h"

#define BENCH_MAGIC 0x424d4146u
#define BENCH_VERSION 1u
#define BENCH_ARM_KEY 0x41524d21u
#define BENCH_TOKEN_XOR 0xc0dec0deu
#define BENCH_ARM_MS 10000u
#define BENCH_PAYLOAD_BYTES 368u

enum bench_command { BENCH_RECOVER=1, BENCH_READ_WORD, BENCH_ARM_WRITE,
                     BENCH_SAVE_PATTERN, BENCH_CANCEL, BENCH_DISARM };
enum bench_state { BENCH_RECOVERING=1, BENCH_READY, BENCH_WRITING,
                   BENCH_DONE, BENCH_FAILED };
enum bench_reply { BENCH_ACCEPTED=1, BENCH_REJECTED, BENCH_ARMED,
                   BENCH_READ_OK, BENCH_READ_CORRUPT, BENCH_READ_ERROR };
enum bench_error { BENCH_ERROR_NONE, BENCH_ERROR_RECOVERY, BENCH_ERROR_WRITE,
                   BENCH_ERROR_CANCELLED };

/* SWD protocol: halt, set request fields, publish a NEW nonzero request_id
 * LAST, resume. No queue. ack_id/reply acknowledge requests independently of
 * operation_id/state/error, so a rejected command cannot disguise live work.
 * All words are 32-bit; DTCM placement keeps the mailbox outside D-cache. */
typedef struct {
    uint32_t magic, version;
    uint32_t request_id, command, argument, key;
    uint32_t ack_id, reply, operation_id, state, error;
    uint32_t heartbeat, tick_ms, steps, arm_token;
    uint32_t record_valid, record_sector, record_slot, record_sequence, record_length;
    uint32_t data_length;
    uint8_t data[32];
    uint32_t cpu_hz, device_id, flash_kib, flash_irqs, max_step_cycles, boot_error;
} scene_bench_mailbox_t;

typedef struct {
    scene_journal_t journal;
    scene_journal_async_t writer;
    scene_journal_recovery_t recovery;
    volatile scene_bench_mailbox_t *mailbox;
    uint32_t armed_at, armed_request;
    uint8_t payload[BENCH_PAYLOAD_BYTES];
} scene_bench_t;

bool scene_bench_init(scene_bench_t *, volatile scene_bench_mailbox_t *,
                       const scene_journal_io_t *, const scene_journal_async_io_t *);
/* At most one IO callback/read<=32B per tick. Bounded <=368B pattern work
 * may occur when accepting SAVE; no flash mutation on boot/RECOVER/READ. */
void scene_bench_tick(scene_bench_t *, uint32_t now_ms);
#endif
