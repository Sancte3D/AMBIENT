#ifndef FAM_SCENES_JOURNAL_H
#define FAM_SCENES_JOURNAL_H

#include <stdbool.h>
#include <stdint.h>

/* A whole Scene-store transaction; SCN5/6/7 payloads remain unchanged.
 * Exactly two independently erasable sectors, 32-byte one-shot flashwords.
 * Main-context only. read must safely classify unreadable/ECC-corrupt words;
 * CPU fault recovery is the platform adapter's responsibility. */
#define SCENE_JOURNAL_WORD_BYTES 32u
#define SCENE_JOURNAL_PAYLOAD_BYTES 512u
#define SCENE_JOURNAL_RECORD_BYTES (SCENE_JOURNAL_WORD_BYTES + SCENE_JOURNAL_PAYLOAD_BYTES)

typedef enum {
    SCENE_JOURNAL_READ_OK,
    SCENE_JOURNAL_READ_CORRUPT, /* safely detected corrupt flash, skip record */
    SCENE_JOURNAL_READ_ERROR    /* unavailable I/O: fail without changing flash */
} scene_journal_read_result_t;

typedef struct {
    void *context;
    unsigned sector_bytes;
    scene_journal_read_result_t (*read)(void *, unsigned, unsigned, void *, unsigned);
    bool (*erase)(void *, unsigned);
    bool (*program)(void *, unsigned, unsigned, const uint8_t *);
} scene_journal_io_t;

typedef struct {
    scene_journal_io_t io;
    bool initialized, has_record, fresh_sector_required;
    unsigned sector, slot, next_slot, payload_length;
    uint32_t sequence;
    _Alignas(32) uint8_t staging[SCENE_JOURNAL_RECORD_BYTES];
} scene_journal_t;

void scene_journal_configure(scene_journal_t *journal, const scene_journal_io_t *io);
bool scene_journal_recover(scene_journal_t *journal);
bool scene_journal_read(scene_journal_t *journal, void *blob, unsigned length);
bool scene_journal_write(scene_journal_t *journal, const void *blob, unsigned length);

#endif
