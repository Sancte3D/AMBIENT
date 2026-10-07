#include "scenes_journal.h"
#include <string.h>

#define JOURNAL_MAGIC 0x3143534au /* JSC1, little-endian wire */
#define JOURNAL_VERSION 1u

static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static void put32(uint8_t *p, uint32_t value) {
    for (unsigned i=0; i<4; ++i) p[i]=(uint8_t)(value>>(8u*i));
}
static uint32_t crc32(const uint8_t *p, unsigned length) {
    uint32_t crc=0xffffffffu;
    for (unsigned i=0; i<length; ++i) {
        crc^=p[i];
        for (unsigned bit=0; bit<8; ++bit)
            crc=(crc>>1)^(0xedb88320u & (0u-(crc&1u)));
    }
    return ~crc;
}
static bool header_valid(const uint8_t *p) {
    unsigned length=get32(p+12);
    return get32(p)==JOURNAL_MAGIC && get32(p+4)==JOURNAL_VERSION &&
           length>0 && length<=SCENE_JOURNAL_PAYLOAD_BYTES &&
           get32(p+20)==0xffffffffu && get32(p+24)==0xffffffffu &&
           get32(p+28)==crc32(p,28);
}
static bool newer(uint32_t a, uint32_t b) {
    return a!=b && (uint32_t)(a-b)<0x80000000u;
}
static bool configured(const scene_journal_t *j) {
    return j && j->io.read && j->io.erase && j->io.program &&
           j->io.sector_bytes>=SCENE_JOURNAL_RECORD_BYTES;
}
static unsigned record_offset(unsigned slot) { return slot*SCENE_JOURNAL_RECORD_BYTES; }

void scene_journal_configure(scene_journal_t *j, const scene_journal_io_t *io) {
    if (!j) return;
    memset(j,0,sizeof *j);
    if (io) j->io=*io;
}

bool scene_journal_recover(scene_journal_t *j) {
    if (!configured(j)) return false;
    j->initialized=false;
    j->has_record=false;
    unsigned slots=j->io.sector_bytes/SCENE_JOURNAL_RECORD_BYTES;
    for (unsigned sector=0; sector<2; ++sector) {
        for (unsigned slot=0; slot<slots; ++slot) {
            unsigned offset=record_offset(slot);
            scene_journal_read_result_t result=j->io.read(j->io.context,sector,offset,
                                                        j->staging,SCENE_JOURNAL_WORD_BYTES);
            if (result==SCENE_JOURNAL_READ_ERROR) return false;
            if (result!=SCENE_JOURNAL_READ_OK || !header_valid(j->staging)) continue;
            unsigned length=get32(j->staging+12);
            uint32_t sequence=get32(j->staging+8), expected=get32(j->staging+16);
            result=j->io.read(j->io.context,sector,offset+SCENE_JOURNAL_WORD_BYTES,
                             j->staging+SCENE_JOURNAL_WORD_BYTES,length);
            if (result==SCENE_JOURNAL_READ_ERROR) return false;
            if (result!=SCENE_JOURNAL_READ_OK ||
                crc32(j->staging+SCENE_JOURNAL_WORD_BYTES,length)!=expected) continue;
            if (!j->has_record || newer(sequence,j->sequence)) {
                j->has_record=true;
                j->sequence=sequence;
                j->sector=sector;
                j->slot=slot;
                j->payload_length=length;
            }
        }
    }
    /* An interrupted operation may leave apparently erased bytes with unknown
     * ECC/program state. After every recovery, erase the other sector before
     * writing. Never append into an uncertain post-reset flashword. The sector
     * containing the last validated record is untouched until a new commit. */
    j->fresh_sector_required=true;
    j->next_slot=0;
    j->initialized=true;
    return true;
}

static bool read_current(scene_journal_t *j, void *blob, unsigned length) {
    if (!j->has_record || length!=j->payload_length) return false;
    unsigned offset=record_offset(j->slot);
    unsigned read_bytes=SCENE_JOURNAL_WORD_BYTES+((length+31u)&~31u);
    if (j->io.read(j->io.context,j->sector,offset,j->staging,read_bytes)
        !=SCENE_JOURNAL_READ_OK || !header_valid(j->staging) ||
        get32(j->staging+8)!=j->sequence || get32(j->staging+12)!=length ||
        crc32(j->staging+SCENE_JOURNAL_WORD_BYTES,length)!=get32(j->staging+16)) return false;
    memcpy(blob,j->staging+SCENE_JOURNAL_WORD_BYTES,length);
    return true;
}

bool scene_journal_read(scene_journal_t *j, void *blob, unsigned length) {
    if (!blob || length==0 || length>SCENE_JOURNAL_PAYLOAD_BYTES || !configured(j)) return false;
    if (!j->initialized && !scene_journal_recover(j)) return false;
    if (!j->has_record || length!=j->payload_length) return false;
    if (read_current(j,blob,length)) return true;
    /* A cached record can become corrupt. One bounded rescan can select the
     * preceding valid transaction; failed reads never publish partial bytes. */
    if (!scene_journal_recover(j)) return false;
    return read_current(j,blob,length);
}

bool scene_journal_write(scene_journal_t *j, const void *blob, unsigned length) {
    if (!blob || length==0 || length>SCENE_JOURNAL_PAYLOAD_BYTES || !configured(j)) return false;
    if (!j->initialized && !scene_journal_recover(j)) return false;
    unsigned slots=j->io.sector_bytes/SCENE_JOURNAL_RECORD_BYTES;
    unsigned sector=j->sector, slot=j->next_slot;
    if (j->fresh_sector_required || slot>=slots) {
        /* With no journal yet, sector 1 may contain the old raw SCN store. */
        sector=j->has_record ? 1u-j->sector : 0u;
        slot=0;
        if (!j->io.erase(j->io.context,sector)) { j->initialized=false; return false; }
    }
    uint32_t sequence=j->has_record ? j->sequence+1u : 0u;
    memset(j->staging,0xff,sizeof j->staging);
    memcpy(j->staging+SCENE_JOURNAL_WORD_BYTES,blob,length);
    put32(j->staging,JOURNAL_MAGIC);
    put32(j->staging+4,JOURNAL_VERSION);
    put32(j->staging+8,sequence);
    put32(j->staging+12,length);
    put32(j->staging+16,crc32(j->staging+SCENE_JOURNAL_WORD_BYTES,length));
    put32(j->staging+28,crc32(j->staging,28));
    unsigned offset=record_offset(slot);
    unsigned words=(length+SCENE_JOURNAL_WORD_BYTES-1u)/SCENE_JOURNAL_WORD_BYTES;
    /* Payload first. The CRC-protected header is a separately programmed,
     * final 32-byte commit. Each word is programmed once since its erase. */
    for (unsigned word=1; word<=words; ++word) {
        if (!j->io.program(j->io.context,sector,offset+word*SCENE_JOURNAL_WORD_BYTES,
                           j->staging+word*SCENE_JOURNAL_WORD_BYTES)) {
            j->initialized=false;
            return false;
        }
    }
    if (!j->io.program(j->io.context,sector,offset,j->staging)) {
        j->initialized=false;
        return false;
    }
    /* Verify committed bytes before reporting success. One small read buffer;
     * the fixed staging state is not a 512-byte automatic stack allocation. */
    uint8_t readback[SCENE_JOURNAL_WORD_BYTES];
    for (unsigned word=0; word<=words; ++word) {
        if (j->io.read(j->io.context,sector,offset+word*SCENE_JOURNAL_WORD_BYTES,
                      readback,sizeof readback)!=SCENE_JOURNAL_READ_OK ||
            memcmp(readback,j->staging+word*SCENE_JOURNAL_WORD_BYTES,sizeof readback)!=0) {
            j->initialized=false;
            return false;
        }
    }
    j->sector=sector;
    j->slot=slot;
    j->next_slot=slot+1u;
    j->sequence=sequence;
    j->payload_length=length;
    j->has_record=true;
    j->fresh_sector_required=false;
    return true;
}
