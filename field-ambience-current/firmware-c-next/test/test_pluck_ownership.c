/* Audible release contract: stop preserves loop waveform under a bounded
 * ramp, pool exhaustion never truncates it, release occupies a real slot. */
#include "pluck.h"
#include "dsp.h"
#include "shape.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

static float l[1024], r[1024], sl[1024], sr[1024], reference[1024];
static int failures;
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x); ++failures; } } while (0)
static void render(int count) {
    memset(l,0,sizeof l); memset(r,0,sizeof r);
    memset(sl,0,sizeof sl); memset(sr,0,sizeof sr);
    pluck_render_mix(l,r,sl,sr,count);
}
int main(void) {
    dsp_init(); shape_init(); pluck_init();
    CHECK(!pluck_note_on(1,NAN,.4f)); CHECK(!pluck_note_on(1,220,INFINITY));
    CHECK(!pluck_note_on(1,220,0)); CHECK(!pluck_note_on(1,30000,.4f));
    CHECK(pluck_active_count()==0);
    CHECK(pluck_note_on(1,220,.4f));
    CHECK(!pluck_note_on(1,330,.4f)); /* same owner still rings */
    CHECK(pluck_note_on(2,330,.4f));
    CHECK(!pluck_note_on(3,440,.4f)); CHECK(pluck_active_count()==2);
    render(1024); memcpy(reference,l,sizeof l);
    pluck_init(); CHECK(pluck_note_on(1,220,.4f)); CHECK(pluck_note_on(2,330,.4f));
    render(1024); CHECK(memcmp(l,reference,sizeof l)==0); /* refused onset changes nothing */

    pluck_init(); CHECK(pluck_note_on(7,220,.4f)); render(64); render(1024);
    memcpy(reference,l,sizeof l);
    pluck_init(); CHECK(pluck_note_on(7,220,.4f)); render(64);
    pluck_note_off(9); /* wrong owner cannot stop it */
    pluck_note_off(7); render(441);
    for(int i=0;i<441;++i) CHECK(fabsf(l[i]-reference[i]*(1.0f-i/882.0f))<1e-7f);
    CHECK(pluck_active_count()==1);
    pluck_note_off(7); /* repeated release cannot reset the deadline */
    render(440); CHECK(pluck_active_count()==1);
    render(1); CHECK(pluck_active_count()==0);
    CHECK(pluck_note_on(7,220,.4f)); /* owner freed only after full release */
    pluck_note(330,.4f); CHECK(pluck_active_count()==2);
    pluck_all_off(); render(882); CHECK(pluck_active_count()==0);
    render(1024); for(int i=0;i<1024;++i) CHECK(l[i]==0 && sl[i]==0);
    printf("pluck ownership/release: %d failures\n",failures);
    return failures?1:0;
}
