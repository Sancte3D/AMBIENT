#ifndef FAM_SCENES_JOURNAL_ASYNC_H
#define FAM_SCENES_JOURNAL_ASYNC_H
#include "scenes_journal.h"

/* Prepared Main-only writer; not connected to the H743/Scene frontend yet.
 * All callbacks must return promptly. poll BUSY means still in flight;
 * DONE/ERROR mean quiescent. A successful start owns its word pointer until
 * that terminal poll. No timeout may release/reuse an in-flight word.
 * Recover the base journal at boot first; exclusive base-cache ownership is
 * required while busy (no synchronous read/write/recover/configure calls). */
typedef enum { SCENE_ASYNC_IO_BUSY, SCENE_ASYNC_IO_DONE, SCENE_ASYNC_IO_ERROR } scene_async_io_result_t;
typedef struct {
    void *context;
    bool (*start_erase)(void *,unsigned);
    bool (*start_program)(void *,unsigned,unsigned,const uint8_t *);
    scene_async_io_result_t (*poll)(void *);
} scene_journal_async_io_t;
typedef enum { SCENE_ASYNC_IDLE, SCENE_ASYNC_BUSY, SCENE_ASYNC_DONE, SCENE_ASYNC_FAILED } scene_async_status_t;
typedef enum {
    SCENE_ASYNC_ERROR_NONE, SCENE_ASYNC_ERROR_CANCELLED,
    SCENE_ASYNC_ERROR_START, SCENE_ASYNC_ERROR_POLL, SCENE_ASYNC_ERROR_VERIFY
} scene_async_error_t;
typedef struct {
    scene_journal_t *journal;
    scene_journal_async_io_t io;
    scene_async_status_t status;
    scene_async_error_t error;
    unsigned phase,sector,slot,length,word,words;
    uint32_t sequence;
    bool cancel_requested,touched;
    _Alignas(32) uint8_t staging[SCENE_JOURNAL_RECORD_BYTES];
    uint8_t readback[SCENE_JOURNAL_WORD_BYTES];
} scene_journal_async_t;

/* Configure only when quiescent; init/configure are not abort operations. */
void scene_journal_async_configure(scene_journal_async_t *,scene_journal_t *,
                                   const scene_journal_async_io_t *);
/* Copies <=512 bytes, computes bounded CRCs, performs no I/O or recovery.
 * Rejected calls leave the current transaction and base cache untouched. */
bool scene_journal_async_begin(scene_journal_async_t *,const void *,unsigned);
/* One start, poll or 32-byte read per call; no hidden wait/scan/write loop.
 * Last validated base cache is published only after final header readback. */
scene_async_status_t scene_journal_async_step(scene_journal_async_t *);
/* Cancellation drains an in-flight operation before returning FAILED.
 * A physically complete final commit may still be found by later recovery. */
bool scene_journal_async_cancel(scene_journal_async_t *);
#endif
