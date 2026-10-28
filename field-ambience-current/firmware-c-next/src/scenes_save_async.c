#include "scenes_save_async_internal.h"
#include "scenes.h"
#include <stddef.h>
#ifdef FAM_DEVICE_BENCH_CANDIDATE
#include "device_bench.h"
#endif
#if !defined(FAM_SOUND_PRODUCT) || !defined(FAM_SCENE_SAVE_ASYNC_PREPARED)
#error "Prepared Scene save requires Product and FAM_SCENE_SAVE_ASYNC_PREPARED"
#endif

enum { RECOVER, START_WRITE, WRITE };
static scene_journal_async_t *writer;
static scene_journal_recovery_t *recovery;
static const void *snapshot;
static unsigned length,phase;
static scenes_save_info_t info={.slot=-1};

bool scenes_save_async_configure(scene_journal_async_t *w,scene_journal_recovery_t *r) {
    if(info.status==SCENES_SAVE_BUSY || (writer && writer->status==SCENE_ASYNC_BUSY) ||
       (recovery && recovery->status==SCENE_RECOVERY_BUSY)) return false;
    if(w || r) {
        if(!w || !r || !w->journal || w->journal!=r->journal ||
           w->status==SCENE_ASYNC_BUSY || r->status==SCENE_RECOVERY_BUSY ||
           !w->io.start_erase || !w->io.start_program || !w->io.poll ||
           !w->journal->io.read || !w->journal->io.erase || !w->journal->io.program ||
           w->journal->io.sector_bytes<SCENE_JOURNAL_RECORD_BYTES) return false;
    }
    writer=w; recovery=r; snapshot=NULL; length=0;
    info=(scenes_save_info_t){.slot=-1}; return true;
}
bool scenes_save_begin(int slot,uint32_t now_ms) {
    if(info.status==SCENES_SAVE_BUSY || slot<0 || slot>=SCENES_COUNT || !writer || !recovery ||
       writer->status==SCENE_ASYNC_BUSY || recovery->status==SCENE_RECOVERY_BUSY) return false;
    unsigned n=0;
    const void *p=scenes_save_snapshot(slot,&n);
    if(!p || !n || n>SCENE_JOURNAL_PAYLOAD_BYTES) return false;
    bool ready=writer->journal->initialized;
    if(ready ? !scene_journal_async_begin(writer,p,n) : !scene_journal_recovery_begin(recovery)) return false;
    snapshot=p; length=n; phase=ready ? WRITE : RECOVER;
    info=(scenes_save_info_t){.status=SCENES_SAVE_BUSY,.slot=slot,.requested_ms=now_ms};
#ifdef FAM_DEVICE_BENCH_CANDIDATE
    device_bench_save_begin();
#endif
    return true;
}
static scenes_save_status_t failed(scenes_save_error_t error,uint32_t now_ms) {
    info.status=SCENES_SAVE_FAILED; info.error=error; info.finished_ms=now_ms;
    info.storage_uncertain=!writer->journal->initialized;
    snapshot=NULL; return info.status;
}
scenes_save_status_t scenes_save_step(uint32_t now_ms) {
    if(info.status!=SCENES_SAVE_BUSY) return info.status;
    if(phase==RECOVER) {
        scene_recovery_status_t s=scene_journal_recovery_step(recovery);
        if(s==SCENE_RECOVERY_FAILED) return failed(recovery->error==SCENE_RECOVERY_ERROR_CANCELLED ?
            SCENES_SAVE_ERROR_CANCELLED : SCENES_SAVE_ERROR_RECOVERY,now_ms);
        if(s==SCENE_RECOVERY_DONE) phase=START_WRITE;
    } else if(phase==START_WRITE) {
        if(info.cancel_requested) return failed(SCENES_SAVE_ERROR_CANCELLED,now_ms);
        if(!scene_journal_async_begin(writer,snapshot,length)) return failed(SCENES_SAVE_ERROR_WRITE,now_ms);
        phase=WRITE;
    } else {
        scene_async_status_t s=scene_journal_async_step(writer);
        if(s==SCENE_ASYNC_FAILED) return failed(writer->error==SCENE_ASYNC_ERROR_CANCELLED ?
            SCENES_SAVE_ERROR_CANCELLED : SCENES_SAVE_ERROR_WRITE,now_ms);
        if(s==SCENE_ASYNC_DONE) {
            scenes_save_publish(info.slot,now_ms);
            info.status=SCENES_SAVE_DONE; info.finished_ms=now_ms; snapshot=NULL;
        }
    }
    return info.status;
}
scenes_save_info_t scenes_save_info(void) { return info; }
bool scenes_save_cancel(void) {
    if(info.status!=SCENES_SAVE_BUSY) return false;
    if(phase==RECOVER) {
        if(!scene_journal_recovery_cancel(recovery)) return false;
    } else if(phase==WRITE) {
        if(!scene_journal_async_cancel(writer)) return false;
    }
    info.cancel_requested=true; return true;
}
