#ifndef FAM_SCENES_JOURNAL_RECOVERY_H
#define FAM_SCENES_JOURNAL_RECOVERY_H
#include "scenes_journal.h"

/* Prepared Main-only recovery; no Scene/H743/default activation.
 * The journal/IO must be configured and exclusively owned while BUSY: no
 * writer, synchronous read/recover or reconfigure. read must return promptly
 * and safely classify ECC-corrupt/unavailable memory. No flash mutation.
 * Begin invalidates availability; confirmed metadata/staging are preserved
 * until the whole scan succeeds. There is no partially published winner. */
typedef enum { SCENE_RECOVERY_IDLE, SCENE_RECOVERY_BUSY,
               SCENE_RECOVERY_DONE, SCENE_RECOVERY_FAILED } scene_recovery_status_t;
typedef enum { SCENE_RECOVERY_ERROR_NONE, SCENE_RECOVERY_ERROR_CANCELLED,
               SCENE_RECOVERY_ERROR_READ } scene_recovery_error_t;
typedef struct {
    scene_journal_t *journal;
    scene_recovery_status_t status;
    scene_recovery_error_t error;
    unsigned phase,slots,sector,slot,length,offset;
    uint32_t sequence,expected,crc;
    unsigned chosen_sector,chosen_slot,chosen_length;
    uint32_t chosen_sequence;
    bool found,cancel_requested;
    _Alignas(32) uint8_t word[SCENE_JOURNAL_WORD_BYTES];
} scene_journal_recovery_t;

/* Quiescent configure only; not a reentrant abort operation. */
void scene_journal_recovery_configure(scene_journal_recovery_t *,scene_journal_t *);
/* No read/scan/erase/program. Rejected calls leave both objects unchanged. */
bool scene_journal_recovery_begin(scene_journal_recovery_t *);
/* At most one <=32-byte read and CRC of <=32 bytes. No record/sector loop.
 * CORRUPT skips the record; ERROR fails the scan with the cache unavailable.
 * DONE publishes a winner or a valid empty journal and requires fresh erase
 * in the other sector before writing, as in the existing sync recovery. */
scene_recovery_status_t scene_journal_recovery_step(scene_journal_recovery_t *);
/* No in-flight operation survives a read return. Cancel is finalized by the
 * next step without reading; the cache remains unavailable until recovery. */
bool scene_journal_recovery_cancel(scene_journal_recovery_t *);
#endif
