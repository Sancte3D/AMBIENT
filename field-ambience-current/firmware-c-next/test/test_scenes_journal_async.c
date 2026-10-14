#include "scenes_journal_async.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

enum { SECTOR=128*1024 };
static uint8_t flash[2][SECTOR],snapshot[2][SECTOR],used[2][SECTOR/32],saved_used[2][SECTOR/32];
static unsigned geometry,calls,starts,polls,reads,checks,saves,fault_cases,cancel_cases;
static unsigned delay_ticks,remaining,sector,offset,operation,fail_operation,prefix;
static unsigned sync_erases,async_erases;
static const uint8_t *borrowed;
static uint8_t borrowed_copy[32];
static bool active,is_erase,lie,start_error;
static unsigned read_error;
#define CHECK(x) do { ++checks; assert(x); } while(0)
static scene_journal_read_result_t read_flash(void *ctx,unsigned s,unsigned at,void *out,unsigned n) {
    (void)ctx; ++calls; ++reads;
    CHECK(s<2 && at<=geometry && n<=geometry-at);
    if(read_error) return read_error==1 ? SCENE_JOURNAL_READ_CORRUPT : SCENE_JOURNAL_READ_ERROR;
    memcpy(out,flash[s]+at,n); return SCENE_JOURNAL_READ_OK;
}
static bool erase_sync(void *ctx,unsigned s) {
    (void)ctx; CHECK(s<2); ++sync_erases;
    memset(flash[s],255,geometry); memset(used[s],0,sizeof used[s]); return true;
}
static bool program_sync(void *ctx,unsigned s,unsigned at,const uint8_t *p) {
    (void)ctx; CHECK(s<2 && at%32==0 && at+32<=geometry && (uintptr_t)p%32==0);
    CHECK(!used[s][at/32]); used[s][at/32]=1;
    for(unsigned i=0;i<32;++i) flash[s][at+i]&=p[i];
    return true;
}
static bool erase_start(void *ctx,unsigned s) {
    (void)ctx; ++calls; ++starts; CHECK(!active && s<2);
    if(start_error) return false;
    active=true; is_erase=true; sector=s; remaining=delay_ticks; ++operation; ++async_erases;
    return true;
}
static bool program_start(void *ctx,unsigned s,unsigned at,const uint8_t *p) {
    (void)ctx; ++calls; ++starts;
    CHECK(!active && s<2 && at%32==0 && at+32<=geometry && (uintptr_t)p%32==0);
    if(start_error) return false;
    CHECK(!used[s][at/32]); used[s][at/32]=1;
    active=true; is_erase=false; sector=s; offset=at; borrowed=p;
    memcpy(borrowed_copy,p,32); remaining=delay_ticks; ++operation; return true;
}
static scene_async_io_result_t poll_flash(void *ctx) {
    (void)ctx; ++calls; ++polls; CHECK(active);
    if(!is_erase) CHECK(!memcmp(borrowed,borrowed_copy,32));
    if(remaining) { --remaining; return SCENE_ASYNC_IO_BUSY; }
    bool failed=operation==fail_operation;
    unsigned n=failed ? prefix : (is_erase ? geometry : 32);
    if(is_erase) {
        CHECK(n<=geometry); memset(flash[sector],255,n);
        if(!failed) memset(used[sector],0,sizeof used[sector]);
    } else {
        CHECK(n<=32);
        for(unsigned i=0;i<n;++i) flash[sector][offset+i]&=borrowed[i];
        if(lie) flash[sector][offset]^=1;
    }
    active=false;
    return failed ? SCENE_ASYNC_IO_ERROR : SCENE_ASYNC_IO_DONE;
}
static scene_journal_io_t sync_io(void) {
    return (scene_journal_io_t){NULL,geometry,read_flash,erase_sync,program_sync};
}
static scene_journal_async_io_t async_io(void) {
    return (scene_journal_async_io_t){NULL,erase_start,program_start,poll_flash};
}
static void clear_faults(void) {
    calls=starts=polls=reads=operation=fail_operation=prefix=0;
    active=is_erase=lie=start_error=false; read_error=0; delay_ticks=2;
}
static void blank(unsigned n) {
    geometry=n; memset(flash,255,sizeof flash); memset(used,0,sizeof used);
    sync_erases=async_erases=0; clear_faults();
}
static void configure(scene_journal_t *j,scene_journal_async_t *w) {
    scene_journal_io_t io=sync_io(); scene_journal_configure(j,&io);
    CHECK(scene_journal_recover(j)); /* Boot recovery, never hidden in begin/step. */
    scene_journal_async_io_t ai=async_io(); scene_journal_async_configure(w,j,&ai);
}
static scene_async_status_t step(scene_journal_async_t *w) {
    unsigned before=calls;
    scene_journal_t prior; memcpy(&prior,w->journal,sizeof prior);
    scene_async_status_t status=scene_journal_async_step(w);
    CHECK(calls-before<=1); /* independently instrument every callback. */
    if(status==SCENE_ASYNC_BUSY) CHECK(!memcmp(&prior,w->journal,sizeof prior));
    return status;
}
static scene_async_status_t finish(scene_journal_async_t *w) {
    for(unsigned n=0;n<10000 && w->status==SCENE_ASYNC_BUSY;++n) step(w);
    CHECK(!active && w->status!=SCENE_ASYNC_BUSY); return w->status;
}
static void expect(const uint8_t *p,unsigned n) {
    scene_journal_t recovered; scene_journal_io_t io=sync_io();
    scene_journal_configure(&recovered,&io);
    uint8_t out[512]; CHECK(scene_journal_read(&recovered,out,n)); CHECK(!memcmp(out,p,n));
}
static void payload(uint8_t *p,unsigned n,unsigned salt) {
    for(unsigned i=0;i<n;++i) p[i]=(uint8_t)(17*i+29*salt);
}
static uint32_t crc(const uint8_t *p,unsigned n) {
    uint32_t c=0xffffffffu;
    for(unsigned i=0;i<n;++i) {
        c^=p[i]; for(unsigned b=0;b<8;++b) c=(c>>1)^(0xedb88320u & (0u-(c&1u)));
    }
    return ~c;
}
static void word32(uint8_t *p,uint32_t v) {
    for(unsigned i=0;i<4;++i) p[i]=(uint8_t)(v>>(8*i));
}
static void limits_and_lifetime(void) {
    blank(2*SCENE_JOURNAL_RECORD_BYTES);
    scene_journal_t j; scene_journal_async_t w; configure(&j,&w);
    uint8_t a[368],copy[368]; payload(a,sizeof a,1); memcpy(copy,a,sizeof a);
    unsigned before=calls;
    CHECK(!scene_journal_async_begin(NULL,a,sizeof a));
    CHECK(!scene_journal_async_begin(&w,NULL,368));
    CHECK(!scene_journal_async_begin(&w,a,0));
    CHECK(!scene_journal_async_begin(&w,a,513));
    j.initialized=false; CHECK(!scene_journal_async_begin(&w,a,368)); j.initialized=true;
    CHECK(calls==before); CHECK(scene_journal_async_begin(&w,a,368)); CHECK(calls==before);
    CHECK(!scene_journal_async_begin(&w,a,368)); /* no overlap or word reuse */
    memset(a,0,sizeof a); /* caller bytes need not remain live */
    CHECK(finish(&w)==SCENE_ASYNC_DONE); expect(copy,sizeof copy); ++saves;
    before=calls; CHECK(step(&w)==SCENE_ASYNC_DONE && calls==before);
    CHECK(!scene_journal_async_cancel(&w));
    CHECK(scene_journal_async_step(NULL)==SCENE_ASYNC_FAILED);
    /* Stop before any I/O leaves the validated base cache usable. */
    CHECK(scene_journal_async_begin(&w,copy,368)); before=calls;
    CHECK(scene_journal_async_cancel(&w)); CHECK(step(&w)==SCENE_ASYNC_FAILED);
    CHECK(calls==before && j.initialized); expect(copy,sizeof copy); ++cancel_cases;
    /* A backend that keeps waiting never frees/reuses its borrowed buffer. */
    CHECK(scene_journal_async_begin(&w,copy,368)); step(&w);
    remaining=100000; CHECK(scene_journal_async_cancel(&w));
    for(unsigned n=0;n<256;++n) {
        CHECK(step(&w)==SCENE_ASYNC_BUSY && active);
        CHECK(!scene_journal_async_begin(&w,copy,368));
    }
    remaining=0; CHECK(finish(&w)==SCENE_ASYNC_FAILED); ++cancel_cases;
}
static void faults(void) {
    const unsigned lengths[]={1,31,32,33,164,364,368,512};
    uint8_t a[512],b[512]; payload(a,512,1); payload(b,512,2);
    for(unsigned size=0;size<8;++size) for(unsigned rollover=0;rollover<2;++rollover) {
        unsigned n=lengths[size],words=(n+31)/32;
        blank(2*SCENE_JOURNAL_RECORD_BYTES);
        scene_journal_t base; scene_journal_async_t w; configure(&base,&w);
        CHECK(scene_journal_write(&base,a,n));
        if(rollover) CHECK(scene_journal_write(&base,a,n));
        memcpy(snapshot,flash,sizeof flash); memcpy(saved_used,used,sizeof used);
        scene_journal_t cached=base;
        unsigned total=words+1+rollover;
        for(unsigned op=1;op<=total;++op) for(unsigned cut=0;cut<=32;++cut) {
            memcpy(flash,snapshot,sizeof flash); memcpy(used,saved_used,sizeof used);
            base=cached; clear_faults(); scene_journal_async_io_t ai=async_io();
            scene_journal_async_configure(&w,&base,&ai);
            fail_operation=op; prefix=cut;
            CHECK(scene_journal_async_begin(&w,b,n));
            CHECK(finish(&w)==SCENE_ASYNC_FAILED && !base.initialized);
            bool committed=op==total && cut==32;
            clear_faults(); expect(committed ? b : a,n); ++fault_cases;
        }
    }
    /* Every erase prefix, including old records in the destination sector. */
    blank(2*SCENE_JOURNAL_RECORD_BYTES);
    scene_journal_t j; scene_journal_async_t w; configure(&j,&w);
    for(int i=0;i<4;++i) CHECK(scene_journal_write(&j,i<2 ? a : b,368));
    memcpy(snapshot,flash,sizeof flash);
    for(unsigned cut=0;cut<=geometry;++cut) {
        memcpy(flash,snapshot,sizeof flash); clear_faults(); configure(&j,&w);
        fail_operation=1; prefix=cut;
        CHECK(scene_journal_async_begin(&w,a,368)); CHECK(finish(&w)==SCENE_ASYNC_FAILED);
        clear_faults(); expect(b,368); ++fault_cases;
    }
}
static void cancellation_and_readback(void) {
    uint8_t a[368],b[368]; payload(a,368,3); payload(b,368,4);
    blank(2*SCENE_JOURNAL_RECORD_BYTES);
    scene_journal_t j; scene_journal_async_t w; configure(&j,&w);
    CHECK(scene_journal_write(&j,a,368)); memcpy(snapshot,flash,sizeof flash);
    memcpy(saved_used,used,sizeof used); scene_journal_t cached=j;
    /* Cancel at every yielding state, including in-flight final commit. */
    for(unsigned at=0;at<100;++at) {
        memcpy(flash,snapshot,sizeof flash); memcpy(used,saved_used,sizeof used);
        clear_faults(); j=cached; scene_journal_async_io_t ai=async_io();
        scene_journal_async_configure(&w,&j,&ai);
        CHECK(scene_journal_async_begin(&w,b,368));
        for(unsigned n=0;n<at && w.status==SCENE_ASYNC_BUSY;++n) step(&w);
        if(w.status==SCENE_ASYNC_DONE) break;
        CHECK(scene_journal_async_cancel(&w));
        bool was_active=active;
        if(was_active) {
            unsigned waiting=remaining; scene_async_status_t status=step(&w);
            CHECK(waiting ? status==SCENE_ASYNC_BUSY && active : status==SCENE_ASYNC_FAILED && !active);
        }
        CHECK(finish(&w)==SCENE_ASYNC_FAILED);
        /* Only a fully committed/CRC-valid record can survive the drain. */
        clear_faults(); scene_journal_t r; scene_journal_io_t io=sync_io();
        scene_journal_configure(&r,&io); uint8_t out[368]; CHECK(scene_journal_read(&r,out,368));
        CHECK(!memcmp(out,a,368) || !memcmp(out,b,368));
        CHECK(!memcmp(out,b,368)==(flash[0][SCENE_JOURNAL_RECORD_BYTES]!=255));
        ++cancel_cases;
    }
    for(unsigned fault=0;fault<4;++fault) {
        memcpy(flash,snapshot,sizeof flash); memcpy(used,saved_used,sizeof used);
        clear_faults(); j=cached; scene_journal_async_io_t ai=async_io();
        scene_journal_async_configure(&w,&j,&ai); CHECK(scene_journal_async_begin(&w,b,368));
        if(fault==0) start_error=true;
        if(fault==1) lie=true;
        if(fault>=2) read_error=fault-1;
        CHECK(finish(&w)==SCENE_ASYNC_FAILED && !j.initialized);
        clear_faults(); expect(a,368); ++fault_cases;
    }
    /* Corrupt an earlier verified payload after the physical header commits.
     * Final post-commit readback must still reject it before publishing cache. */
    memcpy(flash,snapshot,sizeof flash); memcpy(used,saved_used,sizeof used);
    clear_faults(); j=cached; scene_journal_async_io_t ai=async_io();
    scene_journal_async_configure(&w,&j,&ai); CHECK(scene_journal_async_begin(&w,b,368));
    while(w.status==SCENE_ASYNC_BUSY && flash[0][544]==255) step(&w);
    CHECK(w.status==SCENE_ASYNC_BUSY && j.sequence==cached.sequence);
    flash[0][544+32]^=1; CHECK(finish(&w)==SCENE_ASYNC_FAILED && !j.initialized);
    clear_faults(); expect(a,368); ++fault_cases;
}
static void geometry_and_parity(void) {
    uint8_t a[512]; payload(a,512,5);
    blank(SECTOR); scene_journal_t j; scene_journal_async_t w; configure(&j,&w);
    for(unsigned i=0;i<500;++i) {
        a[0]=(uint8_t)i; CHECK(scene_journal_async_begin(&w,a,368));
        CHECK(finish(&w)==SCENE_ASYNC_DONE); expect(a,368); ++saves;
    }
    CHECK(async_erases==3); /* fixed 240-record geometry, not erase per save */
    const unsigned lengths[]={1,31,32,33,164,364,368,512};
    for(unsigned k=0;k<8;++k) {
        blank(2*SCENE_JOURNAL_RECORD_BYTES); configure(&j,&w);
        CHECK(scene_journal_write(&j,a,lengths[k])); memcpy(snapshot,flash,sizeof flash);
        blank(2*SCENE_JOURNAL_RECORD_BYTES); configure(&j,&w);
        CHECK(scene_journal_async_begin(&w,a,lengths[k])); CHECK(finish(&w)==SCENE_ASYNC_DONE);
        CHECK(!memcmp(snapshot,flash,sizeof flash)); ++saves;
    }
    /* Cached sequence wrap and no-journal legacy data in sector1. */
    j.sequence=0xffffffffu; word32(flash[j.sector]+j.slot*544+8,0xffffffffu);
    word32(flash[j.sector]+j.slot*544+28,crc(flash[j.sector]+j.slot*544,28));
    CHECK(scene_journal_async_begin(&w,a,512)); CHECK(finish(&w)==SCENE_ASYNC_DONE && j.sequence==0); ++saves;
    scene_journal_t wrap; scene_journal_io_t io=sync_io(); scene_journal_configure(&wrap,&io);
    CHECK(scene_journal_recover(&wrap) && wrap.sequence==0 && wrap.slot==j.slot);
    blank(2*SCENE_JOURNAL_RECORD_BYTES); memcpy(flash[1],a,368); configure(&j,&w);
    CHECK(scene_journal_async_begin(&w,a,368)); CHECK(finish(&w)==SCENE_ASYNC_DONE);
    CHECK(!memcmp(flash[1],a,368)); ++saves;
}
int main(void) {
    limits_and_lifetime(); faults(); cancellation_and_readback(); geometry_and_parity();
    printf("{\"status\":\"passed\",\"checks\":%u,\"fault_cases\":%u,\"cancel_states\":%u,\"completed_saves\":%u,\"writer_bytes\":%zu,\"max_io_callbacks_per_step\":1}\n",
        checks,fault_cases,cancel_cases,saves,sizeof(scene_journal_async_t));
    return 0;
}
