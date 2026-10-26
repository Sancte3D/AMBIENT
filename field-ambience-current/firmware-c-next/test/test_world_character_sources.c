/* Reuse the actual source fixtures, not a second DSP implementation. */
#define main cell_unfold_fixture_main
#include "test_cell_unfold_sources.c"
#undef main
#if defined(UNFOLD_BOWED)
#define CHARACTER(s,b) bowed_set_character(s,b)
#elif defined(UNFOLD_HORN)
#define CHARACTER(s,b) horn_set_character(s,b)
#else
#define CHARACTER(s,b) pluck_set_character(s,b)
#endif
int main(void) {
    dsp_init();shape_init();INIT();CHARACTER(42,false);
    NOTE(0,196);NOTE(1,246.94165f);render_frames(8*DSP_SAMPLE_RATE_HZ);
    voice_t first=VOICE[0],second=VOICE[1];
    INIT();CHARACTER(42,true);NOTE(0,196);
    CHARACTER(42,false);NOTE(1,246.94165f);render_frames(8*DSP_SAMPLE_RATE_HZ);
    assert(!memcmp(&second,&VOICE[1],sizeof second));
    assert(VOICE[0].character.age==4u*DSP_SAMPLE_RATE_HZ);
#if defined(UNFOLD_BOWED) || defined(UNFOLD_HORN)
    assert(first.freq==VOICE[0].freq && first.inc==VOICE[0].inc && first.ph==VOICE[0].ph);
    assert(first.env==VOICE[0].env && first.rng==VOICE[0].rng);
#else
    assert(first.N==VOICE[0].N && first.rho==VOICE[0].rho);
    assert(!memcmp(first.buf,VOICE[0].buf,sizeof first.buf));
    assert(first.env==VOICE[0].env && first.active==VOICE[0].active);
#endif
    /* A new future-note config must not mutate or restart an active voice. */
    first=VOICE[0];CHARACTER(1234,true);assert(!memcmp(&first,&VOICE[0],sizeof first));
    puts("World source: captured config, independent neighbour, exact oscillator/envelope and unchanged long string pass");
}
