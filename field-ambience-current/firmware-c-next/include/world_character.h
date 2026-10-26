#ifndef FAM_WORLD_CHARACTER_H
#define FAM_WORLD_CHARACTER_H
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "dsp.h"

/* Model 1: family-bound designs with coherent motion, inspired by Tonitor's
 * ANIMA and mod.c design principles. Independent float implementation; no
 * Tonitor code/tables imported. See WORLD_CHARACTER_CHECKPOINT.md.
 * Main prepares each snapshot; audio advances its private 32-sample clock.
 * There is deliberately no pitch, feedback, envelope or event destination. */
#define WORLD_CHARACTER_MODEL 1

typedef struct {
    uint32_t phase_a,phase_b,inc_a,inc_b,rng,age;
    float from_a,to_a,from_b,to_b,tone_bias,detail_bias;
    float tone,detail,gain,pan,shade;
    uint8_t family,clock;
    bool enabled;
} world_character_t;

static inline uint32_t world_character_hash(uint32_t x) {
    x^=x>>16; x*=0x7feb352du; x^=x>>15; x*=0x846ca68bu;
    return x^(x>>16);
}
static inline float world_character_random(world_character_t *c) {
    c->rng=world_character_hash(c->rng+0x9e3779b9u);
    return (float)(c->rng>>8)*(1.0f/8388608.0f)-1.0f;
}
static inline void world_character_prepare(world_character_t *c,int family,
                                           uint32_t seed,int source,float hz,bool on) {
    memset(c,0,sizeof *c); c->tone=c->detail=c->gain=1.0f;
    c->family=(uint8_t)family; c->enabled=on;
    if(!on) return;
    c->rng=world_character_hash(seed ^ (uint32_t)(source+2)*0x85ebca6bu ^
                               (uint32_t)(hz*128.0f) ^ (uint32_t)(family+1)*0xc2b2ae35u);
    c->phase_a=world_character_hash(c->rng+3u);
    c->phase_b=world_character_hash(c->rng+7u);
    c->tone_bias=.075f*world_character_random(c);
    c->detail_bias=.08f*world_character_random(c);
    /* Periods vary within a World, never the sound family. WOODLAND's
     * interpolated targets are aperiodic; the other paths are curved. */
    float seconds=family==0 ? 18.0f : family==1 ? 12.0f : 26.0f;
    seconds*=1.0f+.22f*world_character_random(c);
    c->inc_a=(uint32_t)(4294967296.0*(32.0/DSP_SAMPLE_RATE_HZ)/seconds);
    c->inc_b=(uint32_t)(c->inc_a*(.57f+.09f*world_character_random(c)));
    c->from_a=world_character_random(c); c->to_a=world_character_random(c);
    c->from_b=world_character_random(c); c->to_b=world_character_random(c);
}
static inline float world_character_ease(float t) {return t*t*(3.0f-2.0f*t);}
static inline void world_character_tick(world_character_t *c) {
    if(!c->enabled || ++c->clock<32) return;
    c->clock=0;
    uint32_t old_a=c->phase_a,old_b=c->phase_b;
    c->phase_a+=c->inc_a; c->phase_b+=c->inc_b;
    if(c->family==1) {
        if(c->phase_a<old_a) {c->from_a=c->to_a;c->to_a=world_character_random(c);}
        if(c->phase_b<old_b) {c->from_b=c->to_b;c->to_b=world_character_random(c);}
    }
    float a=(float)c->phase_a*(1.0f/4294967296.0f);
    float b=(float)c->phase_b*(1.0f/4294967296.0f),x,y;
    if(c->family==0) { /* orbit + slower current; timbre and space share a hand */
        x=dsp_sin(a); y=.7f*dsp_sin(a+.25f)+.3f*dsp_sin(b);
    } else if(c->family==1) { /* smooth, non-repeating drift; no re-excitation */
        x=c->from_a+(c->to_a-c->from_a)*world_character_ease(a);
        y=c->from_b+(c->to_b-c->from_b)*world_character_ease(b);
    } else { /* broad figure-eight, with a slower asymmetry */
        x=.7f*dsp_sin(a)+.3f*dsp_sin(b); y=dsp_sin(2.0f*a+.12f);
    }
    const uint32_t fade_frames=4u*DSP_SAMPLE_RATE_HZ;
    if(c->age<fade_frames) c->age=c->age>fade_frames-32u ? fade_frames : c->age+32u;
    float fade=world_character_ease((float)c->age/fade_frames);
    c->tone=1.0f+fade*(c->tone_bias+.24f*x);
    c->detail=1.0f+fade*(c->detail_bias+.18f*y);
    c->gain=1.0f+.055f*fade*(.6f*x+.4f*y);
    c->pan=.035f*fade*y;
    c->shade=fade*(c->tone_bias+.24f*x);
}
#endif
