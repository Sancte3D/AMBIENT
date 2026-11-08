/* Explicit large-session candidate. The existing ECC-safe IRQ adapter is
 * exclusively owned here; no synchronous flash writer is linked alongside it.
 * Boot recovery runs before audio starts. Each live Save step is <=one bounded
 * start/poll/32-byte read, with audio IRQs enabled throughout erase/program. */
#include "scenes_storage_h743.h"
#include "scenes_flash_h743_async.h"
#include "scenes_save_async.h"
#include "sound_parameters.h"
#include <string.h>
#if !defined(FAM_SOUND_SESSION_CANDIDATE) || !defined(FAM_SOUND_FINISH_CANDIDATE)
#error "Large session backend is an explicit complete-sound candidate"
#endif
static scenes_flash_h743_async_t storage_adapter;
static scene_journal_t storage_journal;
static scene_journal_async_t storage_writer;
static scene_journal_recovery_t storage_recovery;
static scene_journal_io_t raw_io;
static bool configured,available;

static scene_journal_read_result_t read_chunks(void *context,unsigned sector,unsigned offset,
                                              void *bytes,unsigned length) {
    uint8_t *p=bytes;
    for(unsigned at=0;at<length;) {
        unsigned n=length-at;if(n>32)n=32;
        scene_journal_read_result_t result=raw_io.read(context,sector,offset+at,p+at,n);
        if(result!=SCENE_JOURNAL_READ_OK)return result;
        at+=n;
    }
    return SCENE_JOURNAL_READ_OK;
}
static bool old_journal_present(void) {
    /* Do not silently erase an incompatible experimental JSC1 journal. That
     * older OFF-by-default format needs its own recovery/export build. Raw
     * SCN5/6/7 stores are migrated by scenes.c without any boot-time writes. */
    uint8_t word[32];
    for(unsigned sector=0;sector<2;++sector)
        for(unsigned offset=0;offset+544<=raw_io.sector_bytes;offset+=544) {
            scene_journal_read_result_t result=raw_io.read(raw_io.context,sector,offset,word,32);
            if(result==SCENE_JOURNAL_READ_ERROR)return true;
            if(result==SCENE_JOURNAL_READ_OK && sound_wire_get32(word)==0x3143534au &&
               sound_wire_get32(word+4)==1 && sound_wire_get32(word+12)>0 &&
               sound_wire_get32(word+12)<=512 && sound_wire_get32(word+28)==sound_wire_crc(word,28))return true;
        }
    return false;
}
static bool configure(void) {
    if(configured)return available;
    configured=true;
    scene_journal_async_io_t io;
    if(!scenes_flash_h743_async_init(&storage_adapter) ||
       !scenes_flash_h743_async_make_io(&storage_adapter,&raw_io,&io))return false;
    scene_journal_io_t read_io=raw_io;read_io.read=read_chunks;
    scene_journal_configure(&storage_journal,&read_io);
    scene_journal_async_configure(&storage_writer,&storage_journal,&io);
    scene_journal_recovery_configure(&storage_recovery,&storage_journal);
    if(!scene_journal_recovery_begin(&storage_recovery))return false;
    /* Boot-only scan, before audio_init. No erase/program in this loop. */
    while(scene_journal_recovery_step(&storage_recovery)==SCENE_RECOVERY_BUSY) { }
    if(storage_recovery.status!=SCENE_RECOVERY_DONE || (!storage_journal.has_record && old_journal_present()))return false;
    available=true;return true;
}
bool scenes_flash_read(void *blob,unsigned length) {
    if(!blob || !length || length>SCENE_JOURNAL_PAYLOAD_BYTES || !configure() ||
       storage_writer.status==SCENE_ASYNC_BUSY || storage_recovery.status==SCENE_RECOVERY_BUSY)return false;
    if(storage_journal.has_record)return scene_journal_read(&storage_journal,blob,length);
    return read_chunks(raw_io.context,1,0,blob,length)==SCENE_JOURNAL_READ_OK;
}
bool scenes_flash_write(const void *blob,unsigned length) {
    /* No accidental synchronous fallback or false 'saved' acknowledgement. */
    (void)blob;(void)length;return false;
}
bool scenes_storage_h743_bind(void) {
    return configure() && scenes_save_async_configure(&storage_writer,&storage_recovery);
}
