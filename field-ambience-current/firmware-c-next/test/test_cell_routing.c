/* Compile the actual HAL routing functions through test_cell_routing.py.
 * Callbacks are probes; DSP/control state is covered by the existing suite. */
#include "cell_router.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

enum { CELL_NOTE, CELL_HARMONY, CELL_LAND };
enum { MOD_GENERATE, MOD_HOLD };
static int s_cell_mode, generate, on[3][5], off[3][5], nudges, cleared;
static cell_router_t s_cell_router;
static const float CELL_TAP_AMP = .2f;
/* Baseline compilation also includes the former steering branch. */
static const char *const STEER_NAME[5] = {"A","B","C","D","E"};
static bool controls_modifier_active(int mod) { return mod == MOD_GENERATE && generate; }
static void controls_cell_press(uint8_t c, float a) { (void)a; ++on[0][c]; }
static void controls_cell_release(uint8_t c) { ++off[0][c]; }
static void controls_release_cells(void) { ++cleared; }
static void bloom_press(uint8_t c,float a,bool h,uint32_t t) { (void)a;(void)h;(void)t;++on[1][c]; }
static void bloom_release(uint8_t c,uint32_t t) { (void)t;++off[1][c]; }
static void landscape_press(uint8_t c,bool h,uint32_t t) { (void)h;(void)t;++on[2][c]; }
static void landscape_release(uint8_t c,uint32_t t) { (void)t;++off[2][c]; }
static void bloom_all_off(void) {}
static void landscape_all_off(uint32_t t) { (void)t; }
static void gesture_clear(uint32_t t) { (void)t; }
static void engine_all_off(void) {}
static void engine_bass_follow(bool enabled) { (void)enabled; }
static uint32_t HAL_GetTick(void) { return 250; }
static void engine_generative_nudge(uint8_t c,uint32_t t) { (void)c;(void)t;++nudges; }
static void overlay_show(const char*a,const char*b,uint32_t t,int k) { (void)a;(void)b;(void)t;(void)k; }

#include "cell_routing_hal.inc"

static void reset(void) {
    memset(&s_cell_router,0,sizeof s_cell_router);
    memset(on,0,sizeof on);memset(off,0,sizeof off);
    s_cell_mode=generate=nudges=cleared=0;
}
int main(void) {
    /* Original reported failure: Generate changes between down and up. */
    reset(); route_cell(0,true,100);generate=1;route_cell(0,false,200);
    assert(on[0][0]==1 && off[0][0]==1);
    for(int mode=0;mode<3;++mode) {
        reset();s_cell_mode=mode;generate=1;
        route_cell(1,true,100);generate=0;route_cell(1,false,200);
        assert(on[mode][1]==0 && off[mode][1]==0 && nudges==0);
    }
    /* Up must reach its original recipient even if the requested mode moved. */
    for(int old=0;old<3;++old) for(int next=0;next<3;++next) {
        reset();s_cell_mode=old;route_cell(2,true,100);
        s_cell_mode=next;route_cell(2,false,200);
        assert(on[old][2]==1 && off[old][2]==1);
        if(next!=old) assert(off[next][2]==0);
    }
    /* Real mode setter releases all owned keys once and cleans Note latches. */
    reset();route_cell(0,true,100);route_cell(4,true,110);hal_set_cell(CELL_HARMONY);
    assert(off[0][0]==1 && off[0][4]==1 && cleared==1);
    route_cell(0,false,300);route_cell(4,false,300);
    assert(off[0][0]==1 && off[0][4]==1 && off[1][0]==0);
    /* A repeated down closes its predecessor; stray/invalid ups are harmless. */
    reset();route_cell(0,true,100);route_cell(0,true,120);route_cell(0,false,140);
    assert(on[0][0]==2 && off[0][0]==2);
    route_cell(0,false,150);route_cell(255,true,160);route_cell(255,false,170);
    assert(on[0][0]==2 && off[0][0]==2);
    assert(sizeof(cell_router_t)==5);
    reset();route_cell(0,true,100);prepare_listening(120);generate=1;
    route_cell(0,false,140);route_cell(1,true,150);
    assert(off[0][0]==1 && on[0][1]==0 && cleared==1);
    puts("cell routing PASS: listening lock, 9 ownership transitions, cleanup and stray edges; 5-byte state");
    return 0;
}
