/* Musical intent: constrained relationships, memory and room to breathe. */
#include "composer.h"

static const composer_params_t TABLE[COMPOSER_STATE_COUNT] = {
    {0.70f,  0.10f, 0.04f, 1.00f, 0.50f}, /* CALM */
    {1.30f, -0.10f, 0.15f, 1.05f, 0.40f}, /* OPEN */
    {0.45f,  0.20f, 0.02f, 0.90f, 0.85f}, /* DEEP */
    {0.15f,  0.45f, 0.00f, 0.60f, 0.30f}, /* EMPTY */
    {1.00f,  0.00f, 0.08f, 1.00f, 0.55f}, /* RETURN */
};
static const char *NAMES[COMPOSER_STATE_COUNT]={"CALM","OPEN","DEEP","EMPTY","RETURN"};
/* Empty resolves through Return. Opening can deepen or breathe; no fixed
 * lap and no unconstrained jump. Under-visited destinations gain weight. */
static const uint8_t PATHS[5][5]={
    {0,5,3,1,1}, {2,0,4,4,1}, {3,2,0,4,2}, {0,0,0,0,10}, {4,5,2,1,0}
};
static composer_state_t state, previous;
static composer_params_t current;
static uint32_t until_ms, last_ms, rng;
static unsigned age[5], since_breath;
static int valid, player;
static float occupancy;

static float rnd01(void) {
    rng=rng*1664525u+1013904223u;
    return (float)(rng>>8)/16777216.0f;
}
static uint32_t dwell(void) { return 40000u+(uint32_t)(rnd01()*40000.0f); }
void composer_init(void) {
    state=previous=COMPOSER_CALM; current=TABLE[state];
    until_ms=last_ms=0; valid=player=0; occupancy=0; since_breath=0;
    for(int i=0;i<5;++i) age[i]=0;
    rng=0xC0400511u;
}
void composer_listen(float occupied, int player_active) {
    occupancy=occupied<0 ? 0 : (occupied>1 ? 1:occupied);
    player=player_active!=0;
}
static composer_state_t next_state(void) {
    if(state==COMPOSER_EMPTY) return COMPOSER_RETURN;
    /* A phrase needs a breath within a bounded number of intentions. */
    if(since_breath>=5) return COMPOSER_EMPTY;
    float weights[5],total=0;
    for(int i=0;i<5;++i) {
        float w=(float)PATHS[state][i]*(1.0f+0.35f*(float)age[i]);
        if(i==(int)previous) w*=0.45f; /* resist two-state ping-pong */
        if(i==COMPOSER_OPEN) w*=1.0f-0.75f*occupancy;
        if(i==COMPOSER_EMPTY || i==COMPOSER_DEEP) w*=1.0f+occupancy;
        weights[i]=w; total+=w;
    }
    float pick=rnd01()*total;
    for(int i=0;i<5;++i) { pick-=weights[i]; if(pick<0) return (composer_state_t)i; }
    return COMPOSER_CALM;
}
void composer_tick(uint32_t now_ms) {
    if(!valid) { until_ms=now_ms+dwell(); last_ms=now_ms; valid=1; return; }
    uint32_t elapsed=now_ms-last_ms; last_ms=now_ms;
    if(player) { until_ms+=elapsed; return; } /* listen without losing the phrase */
    if((int32_t)(now_ms-until_ms)>=0) {
        composer_state_t next=next_state(); previous=state; state=next;
        for(int i=0;i<5;++i) if(age[i]<12) ++age[i];
        age[state]=0;
        since_breath=state==COMPOSER_EMPTY ? 0:since_breath+1;
        until_ms=now_ms+dwell();
    }
    /* Intent crossfades over seconds; sustained beds and bass follow it. */
    float k=(float)elapsed/(4000.0f+(float)elapsed);
#define GLIDE(field) current.field+=k*(TABLE[state].field-current.field)
    GLIDE(mel_density); GLIDE(rest_add); GLIDE(high_p); GLIDE(bed_amp); GLIDE(bass_depth);
#undef GLIDE
}
void composer_nudge(composer_state_t target,uint32_t now_ms) {
    if((unsigned)target>=COMPOSER_STATE_COUNT) return;
    previous=state; state=target; age[state]=0;
    if(state==COMPOSER_EMPTY) since_breath=0;
    until_ms=now_ms+dwell(); last_ms=now_ms; valid=1;
}
void composer_reseed(uint32_t seed) { rng=seed ? seed:0xC0400511u; }
const composer_params_t *composer_params(void) { return &current; }
composer_state_t composer_state(void) { return state; }
const char *composer_state_name(void) { return NAMES[state]; }
