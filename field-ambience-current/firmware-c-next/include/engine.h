#ifndef FAM_ENGINE_H
#define FAM_ENGINE_H

/*
 * Audio engine — Step 11 mix-bus owner.
 *
 * Replaces pad_render as the renderer that audio.c calls. Per audio block:
 *
 *   pad → dry_LR (direct, full level)
 *   pad → send_LR (scaled, controls reverb amount per voice → matches the
 *                  webapp's `verbSend` parameter)
 *   send → famReverbMaster → wet_LR
 *   master_LR = dry_LR + wet_LR * wet_amp
 *   master → tanh soft-clip → int16 interleaved
 *
 * Defaults (medium hall — matches `computeReverb` mid-mode at space=0.5,
 * mood=0.5): size=0.7, damp=0.3, drive=0.15, wet_amp=0.4, send=0.45.
 *
 * The engine forwards note_on/off to pad — `main.c` only calls engine_*
 * APIs after Step 11.
 */

#include <stdint.h>
#include <stdbool.h>

void engine_init(void);

/* Cell-tap forwarders (thin wrappers around pad_note_on/off for now;
 * the harmonic brain in Step 12 will interpose voicing logic here). */
void engine_note_on(uint8_t source, float freq_hz, float amp);
void engine_note_off(uint8_t source);
void engine_all_off(void);

/* r19.20 — "user is playing" for the generative gate. controls.c feeds the
 * PHYSICAL key state (any cell down) via press/release edges. Latched hold
 * voices deliberately do NOT count: they are standing texture, and gating
 * on active voices froze the generator forever under HOLD+GENERATE. */
void engine_set_user_presence(bool any_key_down);

/* r19.20 — SPEC boot sequence: start the master volume hard at 0 (call
 * once right after engine_init(), before the audio pump). The next
 * engine_set_master_volume() target then fades in over the standard
 * parameter ramp (~120 ms time constant, click-free). Host tools/tests
 * that never call this keep the bench 0.6 reference level. */
void engine_boot_mute(void);

/* Note-event tap for MIDI out (or any observer). `on` = 1 note-on, 0 note-off,
 * -1 all-off. Called from the control-rate note path (not the audio ISR), so
 * the sink may enqueue freely. NULL (default) = no tap. The product wires this
 * to MIDI in main_h743; the freq→note/velocity mapping uses midi_note_from_hz.
 * Kept as a hook so the engine keeps no link dependency on midi.c. */
typedef void (*engine_note_hook_t)(int on, uint8_t source, float freq_hz, float amp);
void engine_set_note_hook(engine_note_hook_t h);

/* r19.16 — SYNTH mode: swappable V2 sound-cores behind the ambient engine.
 * mode 0 = ambient (default identity); 1..N = a V2 core rendered through the
 * registered backend. The engine has NO link dependency on src/v2 — the
 * product main (or a test) registers the backend, so all existing host-test
 * link lines stay untouched. Switching crossfades ~15 ms; played cells drive
 * the active core (mono), the generative bed never does. Without a backend,
 * engine_set_synth(>0) is a no-op and the device stays ambient. */
typedef struct {
    void (*select)   (int id);                 /* 0-based V2 core id       */
    void (*note_on)  (int midi, float vel01);
    void (*note_off) (void);
    void (*panic)    (void);
    void (*render)   (int16_t *buf, int frames);   /* interleaved stereo   */
} engine_synth_backend_t;
void engine_set_synth_backend(const engine_synth_backend_t *be);
void engine_set_synth(int idx);                /* 0 ambient, 1..N = core   */
int  engine_synth(void);

/* ADR-0013 — feed one normalised Hall position sample (0=rest, 1=bottom-out)
 * for cell `cell` (0..4) at `now_ms`. The cell-velocity model (cells.c) turns
 * the position stream into note events: a PRESS sounds that cell's chord root
 * (via the harmonic brain) at a velocity-scaled amplitude; a RELEASE stops it.
 * This is the single cell entry point the STM32 ADC loop calls; the RP2040
 * bench can synthesise positions from its digital buttons. Returns true when a
 * note started or stopped this sample (for LED / debug). */
bool engine_cell_sample(uint8_t cell, float pos_0_1, uint32_t now_ms);

/* Live-tunable engine knobs (smoothed by the reverb internally; the wet/
 * send mix is smoothed here). All are 0..1; defaults applied at engine_init. */
void engine_set_reverb_size(float size_0_1);
void engine_set_reverb_damp(float damp_0_1);
void engine_set_reverb_drive(float drive_0_1);
void engine_set_wet_amp(float wet_0_1);
void engine_set_send(float send_0_1);          /* pad reverb send */
void engine_set_master_volume(float vol_0_1);  /* master level (VOLUME encoder) */
/* r18.89 — master DRIVE 0..1. 0 = bit-transparent. Drives the whole mix
 * through an asymmetric soft saturator (even harmonics, small-signal makeup)
 * ahead of the tape stage — the DRIVE encoder finally shapes the SOUND, not
 * just the reverb input. */
void engine_set_drive(float drive_0_1);
void engine_set_brightness(float hz);          /* pass-through to pad */
void engine_set_texture(float amount_0_1);     /* famTexture bed amount */
void engine_set_atmosphere(float amount_0_1);  /* per-world ambience layer (ADR-0017) */
void engine_set_motion(float amount_0_1);      /* Pad LFO depth (perform macro) */
void engine_set_age(float amount_0_1);         /* tape hiss + saturation combo */
void engine_set_echo(float amount_0_1);        /* tape-style stereo delay macro */
void engine_set_blur(float amount_0_1);       /* granular cloud / smear macro */
void engine_set_shimmer(float amount_0_1);     /* r18.99: octave-up hall regeneration */
void engine_set_bass_depth(float depth_0_1);   /* famSubBass/DeepBass depth */
void engine_set_world(int world_idx);          /* pick ambience generator (ADR-0017) */

/* Step 12b #1 — musical state. Setting any of these recomputes the four
 * Freeverb parameters (size/damp/drive/wet) from the per-mode preset +
 * vibe bias + space/mood macros (reverb_presets.h). Updates are applied
 * live; the reverb's own coefficient smoothing prevents zipper, matching
 * the "sound darf nicht konkurrieren" rule for global parameters. */
void engine_set_mode(int mode_idx);            /* 0..5 (ionian..aeolian) */
void engine_set_vibe(int vibe_idx);            /* 0..3 (warm..floating) */
void engine_set_space(float space_0_1);        /* room size / tail macro */
void engine_set_mood(float mood_0_1);          /* darkness↔brightness macro */

/* Step 12b — key tonic (MIDI). Updates the harmonic brain (future cells use
 * the new key) and glides the drone root live; already-held cells keep their
 * pitch ("global follows, held notes freeze" rule). */
void engine_set_key(int tonic_midi);

/* r18.98 — menu KEY slot speaks pitch class (0=C..11=B); anchored into the
 * world-tonic register MIDI 54..65 so key changes transpose, never jump. */
void engine_set_key_pc(int pc_0_11);

/* r18.98 — melody VOICE: 0 = PAD (reference: cells swell, sparkles are KS
 * strings), 1 = STRING, 2 = GLASS (2-op FM). With 1/2 every cell press also
 * strikes the voice (attack in front of the swell); sparkles follow. */
void engine_set_voice(int voice_idx);

/* r19.6 — instrument tuning: 0 = equal temperament (bench reference),
 * 1 = just intonation (5-limit pure ratios anchored to the key — the
 * Sonicware Ø "harmonies without beating"). Applies to all tonal voices. */
void engine_set_tuning(int just);

/* Step 12b #2 — drone toggle (DRONE modifier). Blooms a sustained root pad
 * in/out; it follows engine_set_key live with portamento. */
void engine_set_drone(bool on);

/* Step 12b #3 — pad timbre (warm/strings/brass = 0/1/2). All sounding voices
 * glide together into the new timbre (smoothed global crossfade), so warm→
 * brass never leaves the two timbres competing in the air. */
void engine_set_pad_voice(int voice_idx);

/* Step 12b #4 — generative bed. on=false stops it (releases its voice).
 * program <0 selects Markov auto, >=0 selects a fixed progression index. */
void engine_set_generative(bool on, int program);

/* Advance the generative bed one step: pick the next degree, sound its chord
 * root as a pad voice (a reserved source), and let the bass follow. Returns
 * the new degree (1..7) or -1 when generative is off. No-op while any USER
 * note is held (cells 0..4 or shift octaves 9..13) — live playing overrides
 * the bed. Manual step API for offline renderers/tests; the device uses
 * engine_generative_tick(). */
int engine_generative_advance(void);

/* r18.88 — generative AUTOPLAY. Call frequently from the UI loop (any rate
 * ≥ ~20 Hz); all timing derives from now_ms. Plays the bed by itself:
 * immediate first note after enabling, humanized ±10 % bars (base 8 s),
 * plus 0-2 quiet chord-tone "sparkles" an octave up per bar (r18.89:
 * Karplus-Strong PLUCKS — see pluck.h — that self-decay in ~3 s). While the
 * user holds any note, no new bed/sparkle notes start; the bed resumes on
 * the tick after release. */
void engine_generative_tick(uint32_t now_ms);

/* r19.22 (Scenes): reproduzierbarer Generator-Zustand. Der Seed treibt die
 * Bar-Humanisierung + Sparkle-Streuung; save/recall einer Scene stellt ihn
 * wieder her, damit "dasselbe Feld" wieder dasselbe Feld ist. */
uint32_t engine_gen_seed(void);
void     engine_set_gen_seed(uint32_t seed);

/* r18.90 melody-grammar observability (tests + a future UI readout):
 * last melody tone (MIDI, 0 = none yet) and total scheduled melody notes. */
int engine_generative_last_melody_midi(void);
int engine_generative_melody_count(void);
/* r18.93: phrases replayed by the déjà-vu memory (Marbles concept). */
int engine_generative_dejavu_count(void);

/* The renderer audio.c registers via audio_set_renderer(). Writes `frames`
 * interleaved stereo int16 samples (L,R,L,R,…). Audio-context safe. */
void engine_render(int16_t *buf, int frames);


/* Pass-through for main.c's OLED stat line. */
int engine_active_voices(void);

#endif
