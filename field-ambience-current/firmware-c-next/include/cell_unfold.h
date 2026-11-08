#ifndef FAM_CELL_UNFOLD_H
#define FAM_CELL_UNFOLD_H
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include "dsp.h"
/* Main publishes only digital ownership. Audio owns each voice's progress.
 * Four seconds to the bounded plateau, 1.2 seconds back to neutral; the
 * smoothstep has zero slope at both ends. No pitch or feedback modulation.
 * Integer progress makes endpoints exact at every audio block size. */
static inline void cell_unfold_publish(_Atomic uint32_t *mask,int owner,bool on) {
    if(owner<0 || owner>=16) return;
    if(on) atomic_fetch_or_explicit(mask,1u<<owner,memory_order_release);
    else atomic_fetch_and_explicit(mask,~(1u<<owner),memory_order_release);
}
static inline float cell_unfold_step(uint32_t *phase,uint32_t mask,int owner) {
    const uint32_t top=12u*DSP_SAMPLE_RATE_HZ;
    bool down=owner>=0 && owner<16 && (mask&(1u<<owner));
    if(down) *phase=*phase>top-3u ? top : *phase+3u;
    else *phase=*phase<10u ? 0u : *phase-10u;
    float t=(float)*phase * (1.0f/(12.0f*DSP_SAMPLE_RATE_HZ));
    return t*t*(3.0f-2.0f*t);
}
#endif
