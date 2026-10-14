#ifndef FAM_SCENES_SAVE_ASYNC_H
#define FAM_SCENES_SAVE_ASYNC_H
#include "scenes_journal_async.h"
#include "scenes_journal_recovery.h"

/* Prepared Product Scene API, enabled only with FAM_SCENE_SAVE_ASYNC_PREPARED.
 * Main-only; not wired to UI/H743/CMake/default builds. Configure a quiescent
 * writer/recovery pair for the same journal; keep them and their IO alive until
 * detached. Exclusively own all three during BUSY, including media/backend IO.
 * A true begin means accepted, NEVER saved. Poll info until DONE or FAILED.
 * Snapshot at begin; later control edits cannot change it. Confirmed slots and
 * active Scene publish only after complete writer readback. Existing synchronous
 * save/recall reject and init is a no-op during BUSY; never call from Audio/IRQ.
 * IO callbacks must yield promptly and drain borrowed words before terminal poll.
 */
typedef enum { SCENES_SAVE_IDLE, SCENES_SAVE_BUSY, SCENES_SAVE_DONE,
               SCENES_SAVE_FAILED } scenes_save_status_t;
typedef enum { SCENES_SAVE_ERROR_NONE, SCENES_SAVE_ERROR_CANCELLED,
               SCENES_SAVE_ERROR_RECOVERY, SCENES_SAVE_ERROR_WRITE } scenes_save_error_t;
typedef struct {
    scenes_save_status_t status;
    scenes_save_error_t error;
    int slot;
    uint32_t requested_ms,finished_ms;
    bool cancel_requested,storage_uncertain;
} scenes_save_info_t;

/* NULL,NULL detaches. Rejected calls preserve the last result and current work.
 * Configure/Begin perform no IO; no implicit synchronous recovery/read/write. */
bool scenes_save_async_configure(scene_journal_async_t *,scene_journal_recovery_t *);
bool scenes_save_begin(int slot,uint32_t now_ms);
/* At most one start/poll/read (<=32B) per step. Recovery and write start use
 * separate steps. Each accepted retry takes a NEW snapshot, not the failed one. */
scenes_save_status_t scenes_save_step(uint32_t now_ms);
scenes_save_info_t scenes_save_info(void);
/* Remains BUSY until in-flight IO is quiescent. DONE/FAILED are sticky until an
 * accepted new begin or configure. storage_uncertain means journal cache needs
 * recovery; a complete commit may be recovered despite FAILED/cancellation.
 * The RAM slots stay at their last confirmed state on every failed attempt. */
bool scenes_save_cancel(void);
#endif
