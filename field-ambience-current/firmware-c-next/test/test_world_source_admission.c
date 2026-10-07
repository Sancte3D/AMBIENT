/* Strict source admission must preserve all old waveforms and release slots.
 * Exercise actual renderers, including queued starts before the first sample. */
#include "bowed.h"
#include "horn.h"
#include "choir.h"
#include "guembri.h"
#include "dsp.h"
#include "shape.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

typedef void (*render_fn)(float*,float*,float*,float*,int,float);
typedef struct {
    const char *name;
    void (*init)(void);
    bool (*start)(int,float,float);
    void (*off)(int);
    int (*count)(void);
    render_fn render;
} source_t;
static float l[256],r[256],sl[256],sr[256],reference[256];
static int fails,checks;
#define CHECK(x) do { ++checks; if(!(x)) { ++fails; fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); } } while(0)
static void render(render_fn f,int frames) {
    memset(l,0,sizeof l);memset(r,0,sizeof r);memset(sl,0,sizeof sl);memset(sr,0,sizeof sr);
    f(l,r,sl,sr,frames,.5f);
}
int main(void) {
    const source_t sources[]={
        {"Bowed",bowed_init,bowed_try_note_on,bowed_note_off,bowed_active_count,bowed_render_mix},
        {"Horn",horn_init,horn_try_note_on,horn_note_off,horn_active_count,horn_render_mix},
        {"Choir",choir_init,choir_try_note_on,choir_note_off,choir_active_count,choir_render_mix}
    };
    dsp_init();shape_init();
    for(int family=0;family<3;++family) {
        const source_t *s=&sources[family];
        s->init();
        CHECK(!s->start(16,220,.3f)); CHECK(!s->start(1,NAN,.3f));
        CHECK(!s->start(1,220,0)); CHECK(!s->start(1,9000,.3f));
        for(int i=0;i<3;++i) CHECK(s->start(i,220+110*i,.3f));
        CHECK(s->count()==3); /* pending onsets reserve all real slots */
        CHECK(!s->start(0,550,.3f)); CHECK(!s->start(3,550,.3f));
        render(s->render,256);memcpy(reference,l,sizeof l);
        s->init();for(int i=0;i<3;++i) CHECK(s->start(i,220+110*i,.3f));
        render(s->render,256);CHECK(memcmp(l,reference,sizeof l)==0);
        /* Maximum release must keep its slot after an owned stop. */
        s->init();shape_set_release(1.0f);
        for(int i=0;i<3;++i) CHECK(s->start(i,220+110*i,.3f));
        for(int n=0;n<44100;n+=256) render(s->render,256);
        s->off(0);render(s->render,256);
        CHECK(s->count()==3); CHECK(!s->start(3,550,.3f));
        for(int n=0;n<60*44100;n+=256) render(s->render,256);
        CHECK(s->count()==2);CHECK(s->start(3,550,.3f));CHECK(s->count()==3);
        shape_init();
        printf("%s: full pool, pending onset and maximum-release admission verified\n",s->name);
    }
    guembri_init();CHECK(!guembri_try_note(NAN,.3f));CHECK(!guembri_try_note(220,0));
    for(int i=0;i<3;++i) CHECK(guembri_try_note(220+110*i,.3f));
    CHECK(!guembri_try_note(550,.3f));render(guembri_render_mix,256);memcpy(reference,l,sizeof l);
    guembri_init();for(int i=0;i<3;++i) CHECK(guembri_try_note(220+110*i,.3f));
    render(guembri_render_mix,256);CHECK(memcmp(l,reference,sizeof l)==0);
    printf("World source admission: %d checks, %d failures\n",checks,fails);
    return fails?1:0;
}
