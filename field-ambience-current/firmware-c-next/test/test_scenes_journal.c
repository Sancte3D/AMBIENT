#include "scenes_journal.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define SECTOR_BYTES (128u*1024u)
static uint8_t flash[2][SECTOR_BYTES], snapshot[2][SECTOR_BYTES];
static uint8_t programmed[2][SECTOR_BYTES/32u];
static unsigned sector_bytes, erase_count, program_count, checks;
static int fail_program, fail_erase_bytes, unreadable_sector, read_error;
static unsigned torn_bytes;
static bool lie_program;

#define CHECK(expression) do { ++checks; assert(expression); } while (0)

static scene_journal_read_result_t read_flash(void *context, unsigned sector, unsigned offset,
                                             void *out, unsigned length) {
    (void)context;
    CHECK(sector<2 && offset<=sector_bytes && length<=sector_bytes-offset);
    if (read_error) return SCENE_JOURNAL_READ_ERROR;
    if ((int)sector==unreadable_sector) return SCENE_JOURNAL_READ_CORRUPT;
    memcpy(out,flash[sector]+offset,length);
    return SCENE_JOURNAL_READ_OK;
}
static bool erase_flash(void *context, unsigned sector) {
    (void)context;
    CHECK(sector<2);
    ++erase_count;
    if (fail_erase_bytes>=0) {
        unsigned length=(unsigned)fail_erase_bytes;
        CHECK(length<=sector_bytes);
        memset(flash[sector],0xff,length);
        return false;
    }
    memset(flash[sector],0xff,sector_bytes);
    memset(programmed[sector],0,sizeof programmed[sector]);
    return true;
}
static bool program_flash(void *context, unsigned sector, unsigned offset, const uint8_t *word) {
    (void)context;
    CHECK(sector<2 && offset%32u==0 && offset+32u<=sector_bytes);
    CHECK((uintptr_t)word%32u==0);
    CHECK(!programmed[sector][offset/32u]);
    programmed[sector][offset/32u]=1;
    ++program_count;
    unsigned bytes=32;
    bool fail=fail_program>0 && (int)program_count==fail_program;
    if (fail) bytes=torn_bytes;
    CHECK(bytes<=32);
    for (unsigned i=0; i<bytes; ++i) flash[sector][offset+i]&=word[i];
    if (lie_program && program_count==1) flash[sector][offset]^=1;
    return !fail;
}
static scene_journal_io_t io(void) {
    scene_journal_io_t result={NULL,sector_bytes,read_flash,erase_flash,program_flash};
    return result;
}
static void reset_faults(void) {
    fail_program=0; fail_erase_bytes=-1; torn_bytes=0;
    unreadable_sector=-1; read_error=0; lie_program=false;
    program_count=0; erase_count=0;
}
static void blank(unsigned bytes) {
    sector_bytes=bytes;
    memset(flash,0xff,sizeof flash);
    memset(programmed,0,sizeof programmed);
    reset_faults();
}
static void reboot(scene_journal_t *j) {
    scene_journal_io_t backend=io();
    scene_journal_configure(j,&backend);
}
static void expect(scene_journal_t *j, const uint8_t *expected, unsigned length) {
    uint8_t out[512]; memset(out,0x5a,sizeof out);
    CHECK(scene_journal_read(j,out,length));
    CHECK(memcmp(out,expected,length)==0);
}
static void payload(uint8_t *p, unsigned length, unsigned salt) {
    for (unsigned i=0; i<length; ++i) p[i]=(uint8_t)(i*17u+salt*29u);
}

static void basic_and_limits(void) {
    scene_journal_t j; uint8_t a[512],b[512],out[512];
    blank(2u*SCENE_JOURNAL_RECORD_BYTES); reboot(&j);
    payload(a,sizeof a,1); payload(b,sizeof b,2);
    memset(out,0x5a,sizeof out);
    CHECK(!scene_journal_read(&j,out,368));
    for (unsigned i=0; i<sizeof out; ++i) CHECK(out[i]==0x5a);
    CHECK(!scene_journal_write(&j,NULL,368));
    CHECK(!scene_journal_write(&j,a,0));
    CHECK(!scene_journal_write(&j,a,513));
    CHECK(!scene_journal_read(&j,NULL,32));
    CHECK(!scene_journal_read(&j,out,0));
    CHECK(!scene_journal_read(&j,out,513));
    CHECK(erase_count==0 && program_count==0);
    CHECK(scene_journal_write(&j,a,368)); expect(&j,a,368);
    CHECK(!scene_journal_read(&j,out,364)); /* length mismatch cannot expose an old format */
    CHECK(scene_journal_write(&j,b,368)); expect(&j,b,368);
    CHECK(erase_count==1); /* ordinary append does not erase */
    reboot(&j); expect(&j,b,368);
    CHECK(scene_journal_write(&j,a,512)); expect(&j,a,512);
    CHECK(j.sector==1 && erase_count==2); /* reset starts a freshly erased sector */
    scene_journal_io_t bad=io(); bad.sector_bytes=543;
    scene_journal_configure(&j,&bad);
    CHECK(!scene_journal_write(&j,a,32));
    scene_journal_configure(&j,NULL);
    CHECK(!scene_journal_recover(&j));
    scene_journal_configure(NULL,NULL);
}

static void cut_every_program_byte(void) {
    const unsigned lengths[]={1,31,32,33,164,364,368,512};
    uint8_t a[512],b[512],out[512];
    payload(a,sizeof a,3); payload(b,sizeof b,7);
    for (unsigned n=0; n<sizeof lengths/sizeof lengths[0]; ++n) {
        unsigned length=lengths[n], words=(length+31u)/32u;
        for (unsigned rollover=0; rollover<2; ++rollover) {
            scene_journal_t j;
            blank(2u*SCENE_JOURNAL_RECORD_BYTES); reboot(&j);
            CHECK(scene_journal_write(&j,a,length));
            if (rollover) CHECK(scene_journal_write(&j,a,length));
            memcpy(snapshot,flash,sizeof flash);
            scene_journal_t saved=j;
            uint8_t word_snapshot[2][SECTOR_BYTES/32u];
            memcpy(word_snapshot,programmed,sizeof programmed);
            for (unsigned operation=1; operation<=words+1u; ++operation) {
                for (unsigned prefix=0; prefix<=32; ++prefix) {
                    memcpy(flash,snapshot,sizeof flash);
                    memcpy(programmed,word_snapshot,sizeof programmed);
                    j=saved; reset_faults();
                    fail_program=(int)operation; torn_bytes=prefix;
                    CHECK(!scene_journal_write(&j,b,length));
                    reset_faults(); reboot(&j);
                    CHECK(scene_journal_read(&j,out,length));
                    /* Only a complete last header may publish the new blob. */
                    bool committed=operation==words+1u && prefix==32;
                    CHECK(memcmp(out,committed ? b : a,length)==0);
                    /* Recovery must not reprogram any uncertain flashword. */
                    CHECK(scene_journal_write(&j,b,length)); expect(&j,b,length);
                }
            }
        }
    }
}

static void cut_every_erase_byte(void) {
    uint8_t a[368],b[368]; payload(a,sizeof a,1); payload(b,sizeof b,2);
    blank(2u*SCENE_JOURNAL_RECORD_BYTES);
    scene_journal_t j; reboot(&j);
    CHECK(scene_journal_write(&j,a,sizeof a));
    CHECK(scene_journal_write(&j,a,sizeof a));
    CHECK(scene_journal_write(&j,b,sizeof b));
    CHECK(scene_journal_write(&j,b,sizeof b));
    memcpy(snapshot,flash,sizeof flash);
    /* Sector 0 contains old, valid records; sector 1 has the latest record. */
    for (unsigned prefix=0; prefix<=sector_bytes; ++prefix) {
        memcpy(flash,snapshot,sizeof flash);
        memset(programmed,0,sizeof programmed); reset_faults(); reboot(&j);
        fail_erase_bytes=(int)prefix;
        CHECK(!scene_journal_write(&j,a,sizeof a));
        reset_faults(); reboot(&j); expect(&j,b,sizeof b);
    }
}

static void corruption_and_failed_reads(void) {
    uint8_t a[368],b[368],out[368]; payload(a,sizeof a,8); payload(b,sizeof b,9);
    blank(2u*SCENE_JOURNAL_RECORD_BYTES); scene_journal_t j; reboot(&j);
    CHECK(scene_journal_write(&j,a,sizeof a));
    CHECK(scene_journal_write(&j,b,sizeof b));
    memcpy(snapshot,flash,sizeof flash);
    /* Every header byte and every payload byte can invalidate the new record. */
    for (unsigned byte=0; byte<32u+sizeof b; ++byte) {
        memcpy(flash,snapshot,sizeof flash); flash[0][544u+byte]^=1;
        reset_faults(); reboot(&j); expect(&j,a,sizeof a);
    }
    /* Corruption after a successful read must also invalidate the cached head. */
    memcpy(flash,snapshot,sizeof flash); reset_faults(); reboot(&j);
    expect(&j,b,sizeof b);
    flash[0][544u+32u]^=1;
    expect(&j,a,sizeof a);
    memcpy(flash,snapshot,sizeof flash); reset_faults(); reboot(&j);
    CHECK(scene_journal_write(&j,a,sizeof a)); /* sector 1 */
    unreadable_sector=1; reboot(&j); expect(&j,b,sizeof b);
    unsigned previous_erases=erase_count, previous_programs=program_count;
    read_error=1; reboot(&j); memset(out,0x5a,sizeof out);
    CHECK(!scene_journal_read(&j,out,sizeof out));
    CHECK(!scene_journal_write(&j,a,sizeof a));
    CHECK(previous_erases==erase_count && previous_programs==program_count);
    for (unsigned i=0; i<sizeof out; ++i) CHECK(out[i]==0x5a);
    /* HAL-style success is not enough: readback must verify the data. */
    blank(2u*SCENE_JOURNAL_RECORD_BYTES); reboot(&j);
    CHECK(scene_journal_write(&j,a,sizeof a));
    program_count=0; lie_program=true;
    CHECK(!scene_journal_write(&j,b,sizeof b));
    reset_faults(); reboot(&j); expect(&j,a,sizeof a);
}

static uint32_t test_crc(const uint8_t *p,unsigned length) {
    uint32_t crc=0xffffffffu;
    for (unsigned i=0;i<length;++i) { crc^=p[i]; for(unsigned bit=0;bit<8;++bit)
        crc=(crc>>1)^(0xedb88320u & (0u-(crc&1u))); }
    return ~crc;
}
static void put_test32(uint8_t *p,uint32_t v) {
    for(unsigned i=0;i<4;++i)p[i]=(uint8_t)(v>>(8*i));
}
static void geometry_wrap_and_legacy(void) {
    uint8_t a[368],b[368]; payload(a,sizeof a,4); payload(b,sizeof b,5);
    blank(SECTOR_BYTES); scene_journal_t j; reboot(&j);
    /* Preserve a legacy raw SCN record until a new journal commit succeeds. */
    memcpy(flash[1],a,sizeof a);
    fail_program=1; torn_bytes=16;
    CHECK(!scene_journal_write(&j,b,sizeof b));
    CHECK(memcmp(flash[1],a,sizeof a)==0);
    reset_faults(); reboot(&j);
    CHECK(scene_journal_write(&j,b,sizeof b));
    CHECK(memcmp(flash[1],a,sizeof a)==0);
    CHECK(SECTOR_BYTES/SCENE_JOURNAL_RECORD_BYTES==240);
    for(unsigned save=1;save<500;++save) {
        a[0]=(uint8_t)save;
        CHECK(scene_journal_write(&j,a,sizeof a)); expect(&j,a,sizeof a);
    }
    CHECK(erase_count==3); /* initial fresh bank and two rollovers */
    reboot(&j); expect(&j,a,sizeof a);
    /* Exercise serial-number rollover without billions of flash writes. */
    blank(2u*SCENE_JOURNAL_RECORD_BYTES); reboot(&j);
    CHECK(scene_journal_write(&j,a,sizeof a));
    put_test32(flash[0]+8,0xffffffffu);
    put_test32(flash[0]+28,test_crc(flash[0],28));
    reboot(&j); expect(&j,a,sizeof a);
    CHECK(j.sequence==0xffffffffu);
    CHECK(scene_journal_write(&j,b,sizeof b)); CHECK(j.sequence==0);
    reboot(&j); expect(&j,b,sizeof b);
}

int main(void) {
    basic_and_limits(); cut_every_program_byte(); cut_every_erase_byte();
    corruption_and_failed_reads(); geometry_wrap_and_legacy();
    printf("Scene journal: %u checks passed; torn words/erase, corruption, readback, reset, legacy and wrap\n",checks);
    return 0;
}
