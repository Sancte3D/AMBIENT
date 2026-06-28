#ifndef FAM_V2_SYNTH_ENGINE_H
#define FAM_V2_SYNTH_ENGINE_H

/*
 * synth_engine.h — the contract every selectable sound-core implements.
 *
 * OP-1-Field model: the device (UI, clock, transport, FX, master, output)
 * stays the same; only the sound-core is swapped. An engine is the synthesis
 * (Acid / FM Glass / Chorus / Wavetable / LPG); a World/preset is a saved set
 * of this engine's parameters. The existing V2 ambient stack stays the FIELD
 * engine — this host does NOT modify it.
 *
 * Rules for every engine: no malloc, no samples, no per-sample powf/sinf, no
 * UI logic in the audio path. Use the dsp.h toolkit. render_mix ADDS into the
 * dry + reverb-send buses (mono engines write equal L/R); the host owns the
 * global FX + master limiter, so engines never touch reverb directly.
 */

/* Parameter slots — each engine maps these 0..1 knobs to its own meaning, so
 * the same 4 push-encoders (+ shift for E/F) drive any engine. */
typedef enum {
    SP_A = 0, SP_B, SP_C, SP_D, SP_E, SP_F, SP_COUNT
} synth_param_t;

typedef struct {
    const char *name;
    void (*init)(void);                     /* one-time, at boot */
    void (*activate)(void);                 /* on select: reset phases/voices */
    void (*deactivate)(void);               /* on leave: silence cleanly */
    void (*note_on)(int midi, float vel);   /* vel 0..1; vel high = accent */
    void (*note_off)(void);
    void (*set_param)(synth_param_t p, float v01);
    void (*render_mix)(float *dryL, float *dryR,
                       float *sendL, float *sendR, int frames);  /* ADDS */
    void (*panic)(void);
} synth_engine_t;

#endif /* FAM_V2_SYNTH_ENGINE_H */
