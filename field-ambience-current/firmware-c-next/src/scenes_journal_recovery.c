#include "scenes_journal_recovery.h"
#include <string.h>

enum { HEADER, PAYLOAD };
static uint32_t get32(const uint8_t *p) {
    return (uint32_t)p[0] | (uint32_t)p[1]<<8 | (uint32_t)p[2]<<16 | (uint32_t)p[3]<<24;
}
static uint32_t crc_update(uint32_t crc,const uint8_t *p,unsigned n) {
    for(unsigned i=0;i<n;++i) {
        crc^=p[i];
        for(unsigned b=0;b<8;++b) crc=(crc>>1)^(0xedb88320u & (0u-(crc&1u)));
    }
    return crc;
}
static bool valid_header(const uint8_t *p) {
    unsigned n=get32(p+12);
    return get32(p)==SCENE_JOURNAL_MAGIC && get32(p+4)==1u && n>0 &&
           n<=SCENE_JOURNAL_PAYLOAD_BYTES && get32(p+20)==0xffffffffu &&
           get32(p+24)==0xffffffffu && get32(p+28)==~crc_update(0xffffffffu,p,28);
}
static scene_recovery_status_t fail(scene_journal_recovery_t *r,scene_recovery_error_t e) {
    r->journal->initialized=false;
    r->error=e; r->status=SCENE_RECOVERY_FAILED;
    return r->status;
}
static void advance(scene_journal_recovery_t *r) {
    r->phase=HEADER;
    if(++r->slot==r->slots) { r->slot=0; ++r->sector; }
    if(r->sector!=2) return;
    scene_journal_t *j=r->journal;
    if(r->found) {
        j->sector=r->chosen_sector; j->slot=r->chosen_slot;
        j->sequence=r->chosen_sequence; j->payload_length=r->chosen_length;
    }
    j->has_record=r->found; j->fresh_sector_required=true; j->next_slot=0;
    j->initialized=true; r->status=SCENE_RECOVERY_DONE;
}
void scene_journal_recovery_configure(scene_journal_recovery_t *r,scene_journal_t *j) {
    if(!r) return;
    memset(r,0,sizeof *r); r->journal=j;
}
bool scene_journal_recovery_begin(scene_journal_recovery_t *r) {
    if(!r || r->status==SCENE_RECOVERY_BUSY) return false;
    scene_journal_t *j=r->journal;
    if(!j || !j->io.read || !j->io.erase || !j->io.program ||
       j->io.sector_bytes<SCENE_JOURNAL_RECORD_BYTES) return false;
    r->slots=j->io.sector_bytes/SCENE_JOURNAL_RECORD_BYTES;
    r->sector=r->slot=r->length=r->offset=0;
    r->phase=HEADER; r->found=r->cancel_requested=false;
    r->error=SCENE_RECOVERY_ERROR_NONE; r->status=SCENE_RECOVERY_BUSY;
    j->initialized=false;
    return true;
}
bool scene_journal_recovery_cancel(scene_journal_recovery_t *r) {
    if(!r || r->status!=SCENE_RECOVERY_BUSY) return false;
    r->cancel_requested=true; return true;
}
scene_recovery_status_t scene_journal_recovery_step(scene_journal_recovery_t *r) {
    if(!r) return SCENE_RECOVERY_FAILED;
    if(r->status!=SCENE_RECOVERY_BUSY) return r->status;
    if(r->cancel_requested) return fail(r,SCENE_RECOVERY_ERROR_CANCELLED);
    if(r->phase!=HEADER && r->phase!=PAYLOAD) return fail(r,SCENE_RECOVERY_ERROR_READ);
    unsigned n=r->phase==HEADER ? SCENE_JOURNAL_WORD_BYTES : r->length-r->offset;
    if(n>SCENE_JOURNAL_WORD_BYTES) n=SCENE_JOURNAL_WORD_BYTES;
    unsigned offset=r->slot*SCENE_JOURNAL_RECORD_BYTES;
    if(r->phase==PAYLOAD) offset+=SCENE_JOURNAL_WORD_BYTES+r->offset;
    scene_journal_read_result_t result=r->journal->io.read(r->journal->io.context,
                                                        r->sector,offset,r->word,n);
    if(result==SCENE_JOURNAL_READ_ERROR) return fail(r,SCENE_RECOVERY_ERROR_READ);
    if(result!=SCENE_JOURNAL_READ_OK) { advance(r); return r->status; }
    if(r->phase==HEADER) {
        if(!valid_header(r->word)) { advance(r); return r->status; }
        r->length=get32(r->word+12); r->sequence=get32(r->word+8);
        r->expected=get32(r->word+16); r->offset=0; r->crc=0xffffffffu;
        r->phase=PAYLOAD;
    } else {
        r->crc=crc_update(r->crc,r->word,n); r->offset+=n;
        if(r->offset==r->length) {
            uint32_t difference=r->sequence-r->chosen_sequence;
            if(~r->crc==r->expected && (!r->found ||
               (difference!=0 && difference<0x80000000u))) {
                r->found=true; r->chosen_sequence=r->sequence;
                r->chosen_sector=r->sector; r->chosen_slot=r->slot;
                r->chosen_length=r->length;
            }
            advance(r);
        }
    }
    return r->status;
}
