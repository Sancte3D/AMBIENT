#include "scenes_journal_async.h"
#include <string.h>

enum { ERASE_START,ERASE_WAIT,PAYLOAD_START,PAYLOAD_WAIT,PAYLOAD_VERIFY,
       COMMIT_START,COMMIT_WAIT,COMMIT_VERIFY,FINAL_VERIFY };
static void put32(uint8_t *p,uint32_t v) {
    for(unsigned i=0;i<4;++i) p[i]=(uint8_t)(v>>(8u*i));
}
static uint32_t crc32(const uint8_t *p,unsigned n) {
    uint32_t crc=0xffffffffu;
    for(unsigned i=0;i<n;++i) {
        crc^=p[i];
        for(unsigned b=0;b<8;++b) crc=(crc>>1)^(0xedb88320u & (0u-(crc&1u)));
    }
    return ~crc;
}
static scene_async_status_t fail(scene_journal_async_t *w,scene_async_error_t error) {
    /* Any attempted flash mutation can leave uncertain words. Recover, then
     * erase the other sector before a new write, just like the sync contract. */
    if(w->touched) w->journal->initialized=false;
    w->error=error; w->status=SCENE_ASYNC_FAILED; return w->status;
}
void scene_journal_async_configure(scene_journal_async_t *w,scene_journal_t *j,
                                   const scene_journal_async_io_t *io) {
    if(!w) return;
    memset(w,0,sizeof *w); w->journal=j;
    if(io) w->io=*io;
}
bool scene_journal_async_begin(scene_journal_async_t *w,const void *blob,unsigned n) {
    if(!w || w->status==SCENE_ASYNC_BUSY || !blob || !n || n>SCENE_JOURNAL_PAYLOAD_BYTES) return false;
    scene_journal_t *j=w->journal;
    if(!j || !j->initialized || !j->io.read || !w->io.start_erase ||
       !w->io.start_program || !w->io.poll || j->io.sector_bytes<SCENE_JOURNAL_RECORD_BYTES) return false;
    unsigned slots=j->io.sector_bytes/SCENE_JOURNAL_RECORD_BYTES;
    if(j->sector>=2 || (j->has_record && (j->slot>=slots || !j->payload_length ||
       j->payload_length>SCENE_JOURNAL_PAYLOAD_BYTES))) return false;
    bool fresh=j->fresh_sector_required || j->next_slot>=slots;
    w->sector=fresh ? (j->has_record ? 1u-j->sector : 0u) : j->sector;
    w->slot=fresh ? 0u : j->next_slot;
    w->sequence=j->has_record ? j->sequence+1u : 0u;
    w->length=n; w->word=1; w->words=(n+31u)/32u;
    memmove(w->staging+32,blob,n);
    memset(w->staging,0xff,32);
    memset(w->staging+32+n,0xff,SCENE_JOURNAL_PAYLOAD_BYTES-n);
    /* Exact JSC1 wire used by the existing synchronous reader. */
    put32(w->staging,SCENE_JOURNAL_MAGIC); put32(w->staging+4,1);
    put32(w->staging+8,w->sequence); put32(w->staging+12,n);
    put32(w->staging+16,crc32(w->staging+32,n));
    put32(w->staging+28,crc32(w->staging,28));
    w->phase=fresh ? ERASE_START : PAYLOAD_START;
    w->cancel_requested=w->touched=false; w->error=SCENE_ASYNC_ERROR_NONE;
    w->status=SCENE_ASYNC_BUSY; return true;
}
bool scene_journal_async_cancel(scene_journal_async_t *w) {
    if(!w || w->status!=SCENE_ASYNC_BUSY) return false;
    w->cancel_requested=true; return true;
}
scene_async_status_t scene_journal_async_step(scene_journal_async_t *w) {
    if(!w) return SCENE_ASYNC_FAILED;
    if(w->status!=SCENE_ASYNC_BUSY) return w->status;
    bool waiting=w->phase==ERASE_WAIT || w->phase==PAYLOAD_WAIT || w->phase==COMMIT_WAIT;
    if(w->cancel_requested && !waiting) return fail(w,SCENE_ASYNC_ERROR_CANCELLED);
    unsigned offset=w->slot*SCENE_JOURNAL_RECORD_BYTES;
    switch(w->phase) {
    case ERASE_START:
        w->touched=true;
        if(!w->io.start_erase(w->io.context,w->sector)) return fail(w,SCENE_ASYNC_ERROR_START);
        w->phase=ERASE_WAIT; break;
    case PAYLOAD_START:
        w->touched=true;
        if(!w->io.start_program(w->io.context,w->sector,offset+w->word*32u,
                                w->staging+w->word*32u)) return fail(w,SCENE_ASYNC_ERROR_START);
        w->phase=PAYLOAD_WAIT; break;
    case COMMIT_START:
        w->touched=true;
        if(!w->io.start_program(w->io.context,w->sector,offset,w->staging))
            return fail(w,SCENE_ASYNC_ERROR_START);
        w->phase=COMMIT_WAIT; break;
    case ERASE_WAIT: case PAYLOAD_WAIT: case COMMIT_WAIT: {
        scene_async_io_result_t result=w->io.poll(w->io.context);
        if(result==SCENE_ASYNC_IO_BUSY) break;
        if(w->cancel_requested) return fail(w,SCENE_ASYNC_ERROR_CANCELLED);
        if(result!=SCENE_ASYNC_IO_DONE) return fail(w,SCENE_ASYNC_ERROR_POLL);
        w->phase=w->phase==ERASE_WAIT ? PAYLOAD_START :
                 w->phase==PAYLOAD_WAIT ? PAYLOAD_VERIFY : COMMIT_VERIFY;
        break;
    }
    case PAYLOAD_VERIFY: case COMMIT_VERIFY: case FINAL_VERIFY: {
        unsigned word=w->phase==COMMIT_VERIFY ? 0u : w->word;
        if(w->journal->io.read(w->journal->io.context,w->sector,offset+word*32u,
                              w->readback,32)!=SCENE_JOURNAL_READ_OK ||
           memcmp(w->readback,w->staging+word*32u,32)) return fail(w,SCENE_ASYNC_ERROR_VERIFY);
        if(w->phase==PAYLOAD_VERIFY) {
            ++w->word;
            w->phase=w->word>w->words ? COMMIT_START : PAYLOAD_START;
        } else if(w->phase==COMMIT_VERIFY) {
            w->word=1; w->phase=FINAL_VERIFY;
        } else if(++w->word>w->words) {
            scene_journal_t *j=w->journal;
            j->sector=w->sector; j->slot=w->slot; j->next_slot=w->slot+1u;
            j->sequence=w->sequence; j->payload_length=w->length;
            j->has_record=true; j->fresh_sector_required=false;
            w->status=SCENE_ASYNC_DONE;
        }
        break;
    }
    default: return fail(w,SCENE_ASYNC_ERROR_VERIFY);
    }
    return w->status;
}
