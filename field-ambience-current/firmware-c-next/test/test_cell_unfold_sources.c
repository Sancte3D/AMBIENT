/* Observe real private voices without a new firmware diagnostic API.
 * The unheld neighbour, including filter/RNG, must remain byte-identical.
 * WOODLAND's circulating string, loss, pitch and retirement stay identical. */
#include <assert.h>
#include <string.h>
#include <stdio.h>
#if defined(UNFOLD_BOWED)
#include "../src/bowed.c"
typedef bvoice_t voice_t;
#define VOICE V
#define INIT() bowed_init()
#define NOTE(s,f) assert(bowed_try_note_on(s,f,.3f))
#define SET(s,b) bowed_set_cell_held(s,b)
#define RENDER(n) bowed_render_mix(l,r,sl,sr,n,.35f)
#elif defined(UNFOLD_HORN)
#include "../src/horn.c"
typedef hvoice_t voice_t;
#define VOICE V
#define INIT() horn_init()
#define NOTE(s,f) assert(horn_try_note_on(s,f,.3f))
#define SET(s,b) horn_set_cell_held(s,b)
#define RENDER(n) horn_render_mix(l,r,sl,sr,n,.35f)
#else
#include "../src/pluck.c"
typedef pluck_voice_t voice_t;
#define VOICE v
#define INIT() do {pluck_init();pluck_set_ambient(true);} while(0)
#define NOTE(s,f) assert(pluck_note_on(s,f,.2f))
#define SET(s,b) pluck_set_cell_held(s,b)
#define RENDER(n) pluck_render_mix(l,r,sl,sr,n)
#endif
static float l[512],r[512],sl[512],sr[512];
static void render_frames(int frames) {
    while(frames>0) {
        int n=frames>512 ? 512 : frames;
        memset(l,0,sizeof l);memset(r,0,sizeof r);
        memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
        RENDER(n);frames-=n;
    }
}
int main(void) {
    dsp_init();shape_init();INIT();NOTE(0,196);NOTE(1,246.94165f);
    render_frames(5*DSP_SAMPLE_RATE_HZ);
    voice_t first=VOICE[0],second=VOICE[1];
    INIT();NOTE(0,196);NOTE(1,246.94165f);SET(0,true);
    render_frames(5*DSP_SAMPLE_RATE_HZ);
    assert(!memcmp(&second,&VOICE[1],sizeof second));
    assert(VOICE[0].unfold==12u*DSP_SAMPLE_RATE_HZ);
#if defined(UNFOLD_BOWED) || defined(UNFOLD_HORN)
    assert(first.freq==VOICE[0].freq && first.inc==VOICE[0].inc && first.ph==VOICE[0].ph);
    assert(first.env==VOICE[0].env);
#else
    assert(first.N==VOICE[0].N && first.rho==VOICE[0].rho);
    assert(!memcmp(first.buf,VOICE[0].buf,sizeof first.buf));
    assert(first.active==VOICE[0].active && first.env==VOICE[0].env);
#endif
    SET(0,false);render_frames(DSP_SAMPLE_RATE_HZ*6/5);
    assert(VOICE[0].unfold==0 && VOICE[1].unfold==0);
    SET(0,true);SET(1,true);render_frames(DSP_SAMPLE_RATE_HZ);
    assert(VOICE[0].unfold==VOICE[1].unfold && VOICE[0].unfold>0);
    SET(0,false);render_frames(DSP_SAMPLE_RATE_HZ*6/5);
    assert(VOICE[0].unfold==0 && VOICE[1].unfold>0);
    INIT();assert(!atomic_load(&unfold_mask));
    puts("Cell source: neighbour state identical, pitch/envelope unchanged, independent holds, smooth return and init reset pass");
}
