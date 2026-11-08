#include "scenes_journal_recovery.h"
#include "scenes_journal_async.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SECTOR (128u*1024u)
static uint8_t flash[2][SECTOR],snapshot[2][SECTOR],scan_snapshot[2][SECTOR];
static uint8_t programmed[2][SECTOR/32u],program_snapshot[2][SECTOR/32u];
static unsigned bytes,reads,erases,programs,checks,cases,cut_cases,cancel_states;
static unsigned step_calls,max_step_calls,max_step_bytes,trace_count;
static unsigned trace_sector[8160],trace_offset[8160];
static bool in_step,trace,active,erasing;
static unsigned active_sector,active_offset;
static const uint8_t *borrowed;
static int error_sector=-1,error_word=-1,corrupt_sector=-1,corrupt_word=-1;
static int fail_program=-1,fail_erase=-1;
static unsigned prefix;
#define CHECK(x) do {++checks;assert(x);} while(0)

static scene_journal_read_result_t read_flash(void *c,unsigned s,unsigned at,void *p,unsigned n) {
    (void)c;++reads;
    CHECK(s<2 && n && at<=bytes && n<=bytes-at);
    if(in_step) {
        ++step_calls;CHECK(n<=32);
        if(n>max_step_bytes)max_step_bytes=n;
        if(trace) {CHECK(trace_count<8160);trace_sector[trace_count]=s;trace_offset[trace_count++]=at;}
    }
    unsigned first=at/32u,last=(at+n-1u)/32u;
    if((int)s==error_sector && error_word>=(int)first && error_word<=(int)last) return SCENE_JOURNAL_READ_ERROR;
    if((int)s==corrupt_sector && corrupt_word>=(int)first && corrupt_word<=(int)last) return SCENE_JOURNAL_READ_CORRUPT;
    memcpy(p,flash[s]+at,n);return SCENE_JOURNAL_READ_OK;
}
static bool erase_flash(void *c,unsigned s) {
    (void)c;CHECK(!in_step && s<2);++erases;
    unsigned n=fail_erase>=0 ? (unsigned)fail_erase : bytes;CHECK(n<=bytes);
    memset(flash[s],255,n);
    if(fail_erase>=0)return false;
    memset(programmed[s],0,sizeof programmed[s]);return true;
}
static bool program_flash(void *c,unsigned s,unsigned at,const uint8_t *p) {
    (void)c;CHECK(!in_step && s<2 && at%32u==0 && at+32u<=bytes);
    CHECK((uintptr_t)p%32u==0 && !programmed[s][at/32u]);
    programmed[s][at/32u]=1;++programs;
    bool fail=(int)programs==fail_program;
    unsigned n=fail ? prefix : 32u;CHECK(n<=32);
    for(unsigned i=0;i<n;++i)flash[s][at+i]&=p[i];
    return !fail;
}
static scene_journal_io_t io(void) {
    scene_journal_io_t x={NULL,bytes,read_flash,erase_flash,program_flash};return x;
}
static void faults_off(void) {
    error_sector=error_word=corrupt_sector=corrupt_word=-1;
    fail_program=fail_erase=-1;prefix=0;reads=erases=programs=0;active=false;
}
static void blank(unsigned n) {
    bytes=n;memset(flash,255,sizeof flash);memset(programmed,0,sizeof programmed);faults_off();
}
static void configure(scene_journal_t *j) {scene_journal_io_t x=io();scene_journal_configure(j,&x);}
static void payload(uint8_t *p,unsigned n,unsigned salt) {
    for(unsigned i=0;i<n;++i)p[i]=(uint8_t)(i*17u+salt*29u);
}
static scene_recovery_status_t step(scene_journal_recovery_t *r) {
    step_calls=0;in_step=true;
    scene_recovery_status_t s=scene_journal_recovery_step(r);
    in_step=false;CHECK(step_calls<=1);
    if(step_calls>max_step_calls)max_step_calls=step_calls;
    return s;
}
static unsigned finish(scene_journal_recovery_t *r,const scene_journal_t *busy) {
    unsigned count=0;
    while(r->status==SCENE_RECOVERY_BUSY) {
        CHECK(++count<=2u*r->slots*17u+1u);
        if(step(r)==SCENE_RECOVERY_BUSY) CHECK(!memcmp(r->journal,busy,sizeof *busy));
    }
    return count;
}
static void cache_equal(const scene_journal_t *a,const scene_journal_t *b) {
    CHECK(a->initialized==b->initialized && a->has_record==b->has_record);
    CHECK(a->sector==b->sector && a->slot==b->slot && a->sequence==b->sequence);
    CHECK(a->payload_length==b->payload_length && a->next_slot==b->next_slot &&
          a->fresh_sector_required==b->fresh_sector_required);
}
static unsigned parity(scene_journal_t *j) {
    ++cases;scene_journal_t golden=*j;
    bool expected=scene_journal_recover(&golden);
    scene_journal_t busy=*j;busy.initialized=false;
    scene_journal_recovery_t r;scene_journal_recovery_configure(&r,j);
    unsigned before=reads,e=erases,p=programs;
    memcpy(scan_snapshot,flash,sizeof flash);
    CHECK(scene_journal_recovery_begin(&r));CHECK(reads==before);
    CHECK(!memcmp(j,&busy,sizeof busy));
    unsigned count=finish(&r,&busy);
    CHECK(erases==e && programs==p && !memcmp(scan_snapshot,flash,sizeof flash));
    CHECK((r.status==SCENE_RECOVERY_DONE)==expected);
    if(expected) {
        cache_equal(j,&golden);
        if(j->has_record) {
            uint8_t a[512],b[512];
            CHECK(scene_journal_read(j,a,j->payload_length));
            CHECK(scene_journal_read(&golden,b,golden.payload_length));
            CHECK(!memcmp(a,b,j->payload_length));
        }
    } else CHECK(!memcmp(j,&busy,sizeof busy));
    return count;
}
static bool start_erase(void *c,unsigned s) {
    (void)c;CHECK(!active && s<2);active=true;erasing=true;active_sector=s;return true;
}
static bool start_program(void *c,unsigned s,unsigned at,const uint8_t *p) {
    (void)c;CHECK(!active);active=true;erasing=false;
    active_sector=s;active_offset=at;borrowed=p;return true;
}
static scene_async_io_result_t poll(void *c) {
    (void)c;CHECK(active);active=false;
    bool ok=erasing ? erase_flash(NULL,active_sector) : program_flash(NULL,active_sector,active_offset,borrowed);
    return ok ? SCENE_ASYNC_IO_DONE : SCENE_ASYNC_IO_ERROR;
}
static void retry(scene_journal_t *j,const uint8_t *p,unsigned n) {
    CHECK(j->initialized && j->fresh_sector_required);
    bool old=j->has_record;unsigned preserved=j->sector;
    uint8_t old_sector[SECTOR];if(old)memcpy(old_sector,flash[preserved],bytes);
    scene_journal_async_t w;scene_journal_async_io_t x={NULL,start_erase,start_program,poll};
    scene_journal_async_configure(&w,j,&x);unsigned before=reads;
    CHECK(scene_journal_async_begin(&w,p,n) && reads==before);
    CHECK(w.sector==(old ? 1u-preserved : 0u));
    unsigned limit=0;
    while(w.status==SCENE_ASYNC_BUSY) {
        CHECK(++limit<200);scene_journal_async_step(&w);
        if(old)CHECK(!memcmp(old_sector,flash[preserved],bytes));
    }
    CHECK(w.status==SCENE_ASYNC_DONE && !active);
    uint8_t out[512];CHECK(scene_journal_read(j,out,n));CHECK(!memcmp(out,p,n));
}
static void inputs_and_empty(void) {
    scene_journal_t j;scene_journal_recovery_t r;uint8_t p[32]={0};
    CHECK(scene_journal_recovery_step(NULL)==SCENE_RECOVERY_FAILED);
    CHECK(!scene_journal_recovery_begin(NULL) && !scene_journal_recovery_cancel(NULL));
    scene_journal_recovery_configure(NULL,NULL);
    scene_journal_recovery_configure(&r,NULL);CHECK(!scene_journal_recovery_begin(&r));
    blank(SCENE_JOURNAL_RECORD_BYTES-1);configure(&j);scene_journal_recovery_configure(&r,&j);
    CHECK(!scene_journal_recovery_begin(&r));
    blank(2u*SCENE_JOURNAL_RECORD_BYTES+17u);configure(&j);
    scene_journal_io_t good=j.io;
    j.io.read=NULL;CHECK(!scene_journal_recovery_begin(&r));j.io=good;
    j.io.erase=NULL;CHECK(!scene_journal_recovery_begin(&r));j.io=good;
    j.io.program=NULL;CHECK(!scene_journal_recovery_begin(&r));j.io=good;
    CHECK(reads==0 && erases==0 && programs==0);
    CHECK(parity(&j)==4 && j.initialized && !j.has_record && j.fresh_sector_required);
    memset(flash,0,sizeof flash);CHECK(parity(&j)==4 && !j.has_record);
    blank(2u*SCENE_JOURNAL_RECORD_BYTES);configure(&j);CHECK(scene_journal_write(&j,p,sizeof p));
    scene_journal_recovery_configure(&r,&j);CHECK(scene_journal_recovery_begin(&r));
    scene_journal_t busy=j;scene_journal_recovery_t saved=r;unsigned before=reads;
    CHECK(!scene_journal_recovery_begin(&r));CHECK(!memcmp(&r,&saved,sizeof r));
    CHECK(reads==before && !memcmp(&j,&busy,sizeof j));
    scene_journal_async_t w;scene_journal_async_io_t ai={NULL,start_erase,start_program,poll};
    scene_journal_async_configure(&w,&j,&ai);
    CHECK(!scene_journal_async_begin(&w,p,sizeof p));CHECK(reads==before);
    CHECK(scene_journal_recovery_cancel(&r));CHECK(step(&r)==SCENE_RECOVERY_FAILED);
    CHECK(reads==before && r.error==SCENE_RECOVERY_ERROR_CANCELLED && !j.initialized);
    CHECK(!scene_journal_recovery_cancel(&r));CHECK(step(&r)==SCENE_RECOVERY_FAILED);
    CHECK(scene_journal_recovery_begin(&r));finish(&r,&busy);CHECK(r.status==SCENE_RECOVERY_DONE);
    CHECK(step(&r)==SCENE_RECOVERY_DONE && !scene_journal_recovery_cancel(&r));
}
static void four_records(scene_journal_t *j) {
    blank(2u*SCENE_JOURNAL_RECORD_BYTES);configure(j);
    const unsigned lengths[]={368,512,31,33};uint8_t p[512];
    for(unsigned i=0;i<4;++i){payload(p,sizeof p,i+1);CHECK(scene_journal_write(j,p,lengths[i]));}
}
static void classified_and_cancel(void) {
    scene_journal_t j;four_records(&j);scene_journal_t base=j;
    trace=true;trace_count=0;unsigned states=parity(&j);trace=false;
    CHECK(states==trace_count && states==35);
    unsigned ts[8160],to[8160];memcpy(ts,trace_sector,sizeof ts);memcpy(to,trace_offset,sizeof to);
    for(unsigned i=0;i<states;++i) {
        for(unsigned kind=0;kind<2;++kind) {
            j=base;faults_off();
            if(kind){corrupt_sector=(int)ts[i];corrupt_word=(int)(to[i]/32u);}
            else{error_sector=(int)ts[i];error_word=(int)(to[i]/32u);}
            parity(&j);
            if(!kind) {
                scene_journal_async_t w;scene_journal_async_io_t ai={NULL,start_erase,start_program,poll};
                scene_journal_async_configure(&w,&j,&ai);uint8_t p[32]={0};unsigned before=reads;
                CHECK(!scene_journal_async_begin(&w,p,sizeof p) && reads==before);
                faults_off();parity(&j); /* explicit recovery after a failed read */
            }
        }
    }
    faults_off();
    for(unsigned at=0;at<states;++at) {
        ++cancel_states;j=base;
        scene_journal_recovery_t r;scene_journal_recovery_configure(&r,&j);
        CHECK(scene_journal_recovery_begin(&r));scene_journal_t busy=j;
        for(unsigned n=0;n<at;++n){CHECK(step(&r)==SCENE_RECOVERY_BUSY);CHECK(!memcmp(&j,&busy,sizeof j));}
        unsigned before=reads;CHECK(scene_journal_recovery_cancel(&r));
        CHECK(!scene_journal_recovery_begin(&r));CHECK(step(&r)==SCENE_RECOVERY_FAILED);
        CHECK(reads==before && !memcmp(&j,&busy,sizeof j));
        CHECK(scene_journal_recovery_begin(&r));finish(&r,&busy);CHECK(r.status==SCENE_RECOVERY_DONE);
    }
}
static void corruption_and_lengths(void) {
    scene_journal_t j;four_records(&j);scene_journal_t base=j;memcpy(snapshot,flash,sizeof flash);
    for(unsigned s=0;s<2;++s)for(unsigned at=0;at<2u*SCENE_JOURNAL_RECORD_BYTES;++at) {
        memcpy(flash,snapshot,sizeof flash);flash[s][at]^=1;j=base;parity(&j);
    }
    const unsigned lengths[]={1,31,32,33,164,364,368,512};uint8_t p[512];
    for(unsigned i=0;i<8;++i) {
        blank(2u*SCENE_JOURNAL_RECORD_BYTES);configure(&j);payload(p,sizeof p,i+1);
        CHECK(scene_journal_write(&j,p,lengths[i]));CHECK(parity(&j)==4u+(lengths[i]+31u)/32u);
    }
    blank(2u*SCENE_JOURNAL_RECORD_BYTES);configure(&j);
    memcpy(flash[1],"SCN7",4);memcpy(snapshot,flash,sizeof flash);
    CHECK(parity(&j)==4 && !j.has_record && !memcmp(snapshot,flash,sizeof flash));
}
static void cut_and_retry(void) {
    const unsigned lengths[]={1,31,32,33,164,364,368,512};uint8_t a[512],b[512];
    payload(a,sizeof a,3);payload(b,sizeof b,8);
    for(unsigned i=0;i<8;++i)for(unsigned rollover=0;rollover<2;++rollover) {
        scene_journal_t j;blank(2u*SCENE_JOURNAL_RECORD_BYTES);configure(&j);
        CHECK(scene_journal_write(&j,a,lengths[i]));
        if(rollover)CHECK(scene_journal_write(&j,a,lengths[i]));
        scene_journal_t base=j;memcpy(snapshot,flash,sizeof flash);memcpy(program_snapshot,programmed,sizeof programmed);
        unsigned words=(lengths[i]+31u)/32u;
        for(unsigned op=1;op<=words+1u;++op)for(unsigned cut=0;cut<=32;++cut) {
            ++cut_cases;memcpy(flash,snapshot,sizeof flash);memcpy(programmed,program_snapshot,sizeof programmed);
            j=base;faults_off();fail_program=(int)op;prefix=cut;
            CHECK(!scene_journal_write(&j,b,lengths[i]) && !j.initialized);faults_off();
            parity(&j);
            uint8_t out[512];CHECK(scene_journal_read(&j,out,lengths[i]));
            bool committed=op==words+1u && cut==32;
            CHECK(!memcmp(out,committed ? b : a,lengths[i]));
            retry(&j,b,lengths[i]);
        }
    }
    scene_journal_t j;blank(2u*SCENE_JOURNAL_RECORD_BYTES);configure(&j);
    for(unsigned n=0;n<4;++n)CHECK(scene_journal_write(&j,a,368));
    scene_journal_t base=j;memcpy(snapshot,flash,sizeof flash);memcpy(program_snapshot,programmed,sizeof programmed);
    for(unsigned cut=0;cut<=bytes;++cut) {
        ++cut_cases;memcpy(flash,snapshot,sizeof flash);memcpy(programmed,program_snapshot,sizeof programmed);
        j=base;faults_off();fail_erase=(int)cut;
        CHECK(!scene_journal_write(&j,b,368));faults_off();parity(&j);
        uint8_t out[368];CHECK(scene_journal_read(&j,out,sizeof out));CHECK(!memcmp(out,a,sizeof out));
        retry(&j,b,368);
    }
}
static uint32_t crc(const uint8_t *p,unsigned n) {
    uint32_t c=0xffffffffu;for(unsigned i=0;i<n;++i){c^=p[i];for(unsigned b=0;b<8;++b)c=(c>>1)^(0xedb88320u & (0u-(c&1u)));}return ~c;
}
static void put32(uint8_t *p,uint32_t v){for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(8u*i));}
static void sequence(uint8_t *p,uint32_t n){put32(p+8,n);put32(p+28,crc(p,28));}
static void malformed_authenticated_headers(void) {
    scene_journal_t j;four_records(&j);scene_journal_t base=j;memcpy(snapshot,flash,sizeof flash);
    const unsigned offsets[]={0,4,12,12,12,20,24};
    const uint32_t values[]={0,2,0,513,0xffffffffu,0,0};
    for(unsigned i=0;i<7;++i) {
        memcpy(flash,snapshot,sizeof flash);j=base;
        uint8_t *p=flash[1]+SCENE_JOURNAL_RECORD_BYTES;
        put32(p+offsets[i],values[i]);put32(p+28,crc(p,28));
        parity(&j);CHECK(j.sequence==2 && j.sector==1 && j.slot==0);
    }
}
static void geometry_and_sequences(void) {
    scene_journal_t j;uint8_t p[512];payload(p,sizeof p,7);
    blank(SECTOR);configure(&j);
    for(unsigned save=0;save<500;++save){p[0]=(uint8_t)save;CHECK(scene_journal_write(&j,p,sizeof p));}
    CHECK(parity(&j)==480u+260u*16u && j.sequence==499 && j.sector==0 && j.slot==19);
    four_records(&j);
    const uint32_t seeds[][4]={{0xfffffffeu,0xffffffffu,0,1},{0,0x80000000u,0,0},
                              {17,17,17,17},{0xfffffffeu,0,0xffffffffu,1}};
    for(unsigned i=0;i<4;++i) {
        for(unsigned s=0;s<2;++s)for(unsigned slot=0;slot<2;++slot)
            sequence(flash[s]+slot*SCENE_JOURNAL_RECORD_BYTES,seeds[i][s*2u+slot]);
        parity(&j);
    }
    CHECK(j.sequence==1 && j.sector==1 && j.slot==1);
}
int main(void) {
    inputs_and_empty();classified_and_cancel();corruption_and_lengths();cut_and_retry();
    malformed_authenticated_headers();geometry_and_sequences();
    printf("{\"status\":\"passed\",\"checks\":%u,\"parity_cases\":%u,\"cut_and_retry_cases\":%u,\"cancel_states\":%u,\"recovery_bytes\":%zu,\"max_read_callbacks_per_step\":%u,\"max_read_bytes\":%u}\n",
           checks,cases,cut_cases,cancel_states,sizeof(scene_journal_recovery_t),max_step_calls,max_step_bytes);
    return 0;
}
