/* Actual HAL functions, synthetic clocks/hardware, real Product PCM. */
#include "audio.h"
#include "audio_profiler.h"
#include "engine.h"
#include "engine_product.h"
#include "brain.h"
#include "dsp.h"
#include "oled.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef int SAI_HandleTypeDef;
typedef int DMA_HandleTypeDef;
static SAI_HandleTypeDef h743_hsai1a;
static DMA_HandleTypeDef h743_hdma_sai;
static struct { volatile uint32_t CYCCNT; } ticks;
#define DWT (&ticks)
static uint32_t SystemCoreClock=480000000, render_cost, scan_cost, clean_cost;
static unsigned halves, scans, cleans, cases, pairs, rows;
static int cap;
static int16_t expected[32*512*2];
void real_scan_clips(audio_profiler_t *,const int16_t *,int);
void audio_profiler_scan_clips(audio_profiler_t *p,const int16_t *b,int n) {
    real_scan_clips(p,b,n);ticks.CYCCNT+=scan_cost;++scans;
}
static void SCB_CleanDCache_by_Addr(uint32_t *b,int n) {
    assert((uintptr_t)b%32==0 && n==2048);ticks.CYCCNT+=clean_cost;++cleans;
}
static void HAL_DMA_IRQHandler(DMA_HandleTypeDef *h);
#include "scope_audio.inc"
#include "scope_irq.inc"
static void HAL_DMA_IRQHandler(DMA_HandleTypeDef *h) {
    assert(h==&h743_hdma_sai);ticks.CYCCNT+=17;
    if(halves&1) HAL_SAI_TxHalfCpltCallback(&h743_hsai1a);
    if(halves&2) HAL_SAI_TxCpltCallback(&h743_hsai1a);
    ticks.CYCCNT+=23;
}
static void pattern(int16_t *b,int n) {
    memset(b,0,(size_t)n*4);b[0]=32767;b[1]=-32768;b[2]=-32767;
    ticks.CYCCNT+=render_cost;
}
static void product(int16_t *b,int n) {
    for(int i=0;i<n;i+=cap) engine_render(b+2*i,n-i<cap?n-i:cap);
    ticks.CYCCNT+=render_cost;
}
static void reset(void) {
    reset_profilers();ticks.CYCCNT=0;scans=cleans=0;
    render_cost=scan_cost=clean_cost=0;s_renderer=pattern;
}
static void init_product(int w) {
    dsp_init();brain_init();engine_init();engine_set_world(w);
    engine_set_room(.5f);engine_set_nature(.3f);
    assert(engine_try_note_on(0,220,.8f));
}
void oled_fill(uint8_t g) {(void)g;}
void oled_text(int x,int y,const char *s,uint8_t g) {
    (void)g;assert(x>=0 && y>=0 && y+10<=OLED_HEIGHT);
    assert(x+8*(int)strlen(s)<=OLED_WIDTH);++rows;
}
void oled_text_scaled(int x,int y,const char *s,uint8_t g,int scale) {
    (void)g;assert(x>=0 && y>=0 && y+10*scale<=OLED_HEIGHT);
    assert(x+8*(int)strlen(s)*scale<=OLED_WIDTH);++rows;
}
static float bat_adc_read_volts(void) {return 4.2f;}
#include "scope_diag.inc"
int main(void) {
    uint32_t budget=(uint32_t)(((uint64_t)512*SystemCoreClock)/44100);
    uint32_t costs[]={0,1000,budget/2,budget-41,budget-40,budget-1,budget,budget+1};
    for(unsigned wrap=0;wrap<2;++wrap) for(halves=0;halves<4;++halves)
    for(unsigned c=0;c<8;++c) {
        reset();render_cost=costs[c];scan_cost=31;clean_cost=43;
        ticks.CYCCNT=wrap?UINT32_MAX-5:100;DMA1_Stream0_IRQHandler();
        unsigned count=(halves&1)+((halves>>1)&1);
        uint32_t elapsed=40+count*(render_cost+scan_cost+clean_cost);
        assert(s_irq_prof.last_cycles==elapsed && s_irq_prof.max_cycles==elapsed);
        assert(s_irq_prof.render_count==1 && s_irq_prof.budget_cycles==budget);
        assert(s_irq_prof.deadline_miss_count==(elapsed>=budget));
        assert(s_prof.render_count==count && s_prof.clip_count==3*count);
        assert(scans==count && cleans==count);
        if(count) assert(s_prof.last_cycles==render_cost);
        ++cases;
    }
    reset();halves=1;render_cost=budget-100;clean_cost=61;DMA1_Stream0_IRQHandler();
    assert(!s_prof.deadline_miss_count && s_irq_prof.deadline_miss_count==1);
    reset();assert(!s_prof.render_count && !s_irq_prof.render_count);
    for(int w=0;w<3;++w) for(int partition=0;partition<2;++partition)
    for(int wrap=0;wrap<2;++wrap) {
        reset();cap=partition?512:64;init_product(w);
        for(int block=0;block<32;++block) product(expected+block*1024,512);
        init_product(w);reset_profilers();s_renderer=product;
        render_cost=111;scan_cost=13;clean_cost=37;
        ticks.CYCCNT=wrap?UINT32_MAX-50:0;halves=3;
        for(int call=0;call<16;++call) {
            DMA1_Stream0_IRQHandler();
            assert(!memcmp(s_buffer,expected+call*2048,sizeof s_buffer));
        }
        assert(s_prof.render_count==32 && s_irq_prof.render_count==16);
        assert(s_irq_prof.budget_cycles==budget);
        assert(!engine_nonfinite_samples() && !engine_output_limited_samples());++pairs;
    }
    s_prof.max_cycles=s_irq_prof.max_cycles=UINT32_MAX;
    s_prof.deadline_miss_count=s_irq_prof.deadline_miss_count=UINT32_MAX;
    diag_draw(0,0);diag_draw(1,1);diag_draw(0,1);assert(rows==33);
    printf("{\"accounting_cases\":%u,\"pcm_pairs\":%u,\"stereo_frames_compared\":%u,\"diagnostic_rows\":%u,\"added_profiler_bytes\":%zu}\n",cases,pairs,pairs*32*512,rows,sizeof s_irq_prof);
}
