/*
 * Audio engine — Step 11 mix-bus owner.
 *
 * Static float scratch buffers (BLOCK frames) keep allocation out of the
 * audio path. BLOCK matches AUDIO_BUFFER_FRAMES (256) — audio.c calls in
 * chunks of that size, so we always process a whole audio block in one pass.
 *
 * `send` and `wet_amp` are smoothed per block (~120 ms) to avoid zipper
 * when the user knobs them; size/damp/drive are smoothed inside the reverb
 * itself.
 */

#include "engine.h"
#include "pad.h"
#include "reverb.h"
#include "texture.h"
#include "ambience.h"
#include "tape.h"
#include "echo.h"
#include "blur.h"
#include "bass.h"
#include "drone.h"
#include "reverb_presets.h"
#include "brain.h"
#include "worlds.h"
#include "generative.h"
#include "cells.h"
#include "pluck.h"
#include "glass.h"
#include "shimmer.h"
#include "padsynth.h"
#include "body.h"
#include "composer.h"
#include "harmony.h"
#include "tuning.h"
#include "dsp.h"
#include "audio.h"                    /* AUDIO_BUFFER_FRAMES */
#include <math.h>
#include <string.h>

/* Per-layer reverb sends (match the webapp's per-voice verbSend values).
 * Pad uses the user-tunable engine_set_send; texture has its own fixed send;
 * the bass applies its two per-layer sends internally. */
#define TEXTURE_SEND  0.55f
#define AMBIENCE_SEND 0.35f           /* slightly less wet than texture — ADR-0017 */

/* Active note tracking so the bass can follow the lowest held pitch. Sources
 * are cell indices today (0..4), with headroom for MIDI later. freq 0 = idle. */
#define MAX_SOURCES   16
#define GEN_SOURCE    8           /* reserved pad-voice source for the bed */
#define GEN_VOICE_AMP 0.10f
static float active_freq[MAX_SOURCES];
static int   melody_voice;          /* r18.98 VOICE: 0 PAD, 1 STRING, 2 GLASS */

/* r18.99 ENO LOOPS — the Music-for-Airports principle (studied via the
 * teropa "loop" essay on Reich's It's Gonna Rain and Eno's airport loops;
 * technique reconstructed, nothing to copy): several long loops, ONE note
 * per loop, with INCOMMENSURATE periods that never re-align — the bed
 * stops being one held chord root and becomes a slowly recombining choir.
 * Three loops on pad sources 5..7, periods chosen pairwise non-multiple
 * (13.7 / 21.3 / 33.1 s — no common divisor within hours). Each cycle the
 * loop re-picks ITS chord member (root/third/fifth) from the CURRENT
 * harmony, so the recombination always lands inside the key. Autoplay
 * only — a playing human owns the bed. */
#define ENO_LOOPS 3
static const uint32_t ENO_PERIOD_MS[ENO_LOOPS] = { 13700u, 21300u, 33100u };
static const float    ENO_AMP[ENO_LOOPS]       = { 0.050f, 0.044f, 0.038f };
static uint32_t eno_next_ms[ENO_LOOPS];
static uint32_t eno_off_ms[ENO_LOOPS];
static uint8_t  eno_on[ENO_LOOPS];
static int      eno_timing_valid;
#define ENO_SRC(i) ((uint8_t)(5 + (i)))

/* Generative state (r19.0: rebuilt on the HARMONIC SAFETY CORE, see
 * harmony.h — pitch world → register rules → state mutation → collision
 * filter → long melody → probability LAST). The old per-bar chord walk +
 * sparkle scheduler is gone: the bed follows the harmonic STATE, the
 * melody is one LONG voice with real silences. */
static bool     gen_on = false;
static bool     gen_timing_valid = false;
static uint32_t gen_tick_rng     = 0x5EEDBA55u;
static int      gen_state_seen   = -1;   /* bed re-strikes on state change */

/* Long melody voice: pad source 15 sustains 4-16 s, the selected VOICE
 * (string/glass) strikes the onset. */
#define MEL_SRC 15
static uint32_t mel_next_ms;          /* next decision time              */
static uint32_t mel_off_ms;           /* scheduled note-off              */
static int      mel_sounding;
static int      mel_phrase_left;      /* notes left in current phrase    */
static int      mel_last_midi;        /* voice-leading memory (0 = none) */
static int      mel_note_count;       /* observability (tests/UI)        */

/* déjà-vu phrase memory (Marbles concept, kept from r18.93): the last
 * completed phrase replays with p=0.35 — but EVERY remembered tone must
 * still pass the current pitch world + collision filter, else the picker
 * falls through to a fresh safe choice. The motif survives, the safety
 * rules always win. */
#define MEL_PHRASE_MAX 6
static int mel_hist[MEL_PHRASE_MAX]; static int mel_hist_len = 0;
static int mel_cur [MEL_PHRASE_MAX]; static int mel_cur_len  = 0;
static int mel_replay = 0, mel_replay_idx = 0;
static int mel_dejavu_count = 0;

/* sounding-note registry for the collision filter (0 = silent) */
static int snd_bed_midi = 0;
static int snd_eno_midi[ENO_LOOPS];

/* r19.0 spectral undulation (Blendwave principle from the Liven Ambient
 * Ø study): a held tone must LIVE. Correlated random walk on the pad's
 * brightness tilt — small persistent steps, never a jump — so the bed
 * undulates without a single new note event. */
static float    vmix_walk = 0.0f;
static uint32_t vmix_next_ms = 0;

/* Lowest currently-held frequency, or 0 if nothing is held. */
static float lowest_held(void) {
    float lo = 0.0f;
    for (int i = 0; i < MAX_SOURCES; ++i) {
        if (i >= 5 && i <= 7) continue;   /* r18.99: Eno loops are colour,
                                           * not fundament — bass ignores them */
        float f = active_freq[i];
        if (f > 0.0f && (lo == 0.0f || f < lo)) lo = f;
    }
    return lo;
}

/* Re-point the bass at the current lowest note, or release it if none held. */
static void refresh_bass(void) {
    float lo = lowest_held();
    if (lo > 0.0f) bass_note(lo);
    else           bass_release();
}

#define BLOCK     AUDIO_BUFFER_FRAMES

static float dryL [BLOCK];
static float dryR [BLOCK];
static float sendL[BLOCK];
static float sendR[BLOCK];
static float wetL [BLOCK];
static float wetR [BLOCK];

static float send_amount_cur, send_amount_tgt;
static float wet_amp_cur,     wet_amp_tgt;
static float reverb_size, reverb_damp;        /* cached, so the two setters
                                                  can change one independently */
/* r18.90: BRIGHTNESS is a macro, not a filter knob — it also tilts the
 * hall damping (dark = duller tail) and the pluck damping. This trim rides
 * ON TOP of the preset/manual damp so preset changes keep working. */
static float bright_damp_trim = 0.0f;

static void reverb_apply(void) {
    reverb_set(reverb_size,
               dsp_clampf(reverb_damp + bright_damp_trim, 0.0f, 1.0f));
}
static const float SMOOTH_COEF = 0.05f;       /* per-block, ~120 ms time-const */

/* Master stage (fixes the listening-test "earrape / brummt"):
 *   1. one-pole DC blocker (~35 Hz highpass) removes DC + subsonic rumble that
 *      otherwise builds up from the brown-noise bed, the bass and the reverb
 *      feedback — this is the "LeakDC" the Step-11 plan called for.
 *   2. master volume gives headroom (no on-device volume knob yet).
 *   3. a soft limiter that is PERFECTLY LINEAR below the knee and only rounds
 *      true peaks — replaces the old tanf() that distorted everything above
 *      ~0.5 (that continuous saturation was the harshness). */
#define DC_R 0.995f                           /* one-pole HP, ≈35 Hz at 44.1 k */
static float dc_x1L, dc_y1L, dc_x1R, dc_y1R;
static float master_vol_cur, master_vol_tgt;

/* r18.89 — master DRIVE stage. The DRIVE encoder used to reach only the
 * reverb-input saturation, so on a dry-ish patch the knob did almost
 * nothing. Now it drives the WHOLE mix through an asymmetric soft
 * saturator (dsp_drive_shape: tanh with a bias skew → even harmonics)
 * with small-signal makeup, placed BEFORE the DC blocker (the bias makes
 * a touch of DC — the blocker eats it) and before the tape stage, so
 * drive pushes INTO the tape knee like a real chain. 0 = bit-transparent
 * bypass. */
static float drive_cur, drive_tgt;

static inline float soft_limit(float x) {
    const float k = 0.75f;                    /* clean below this */
    float a = x < 0.0f ? -x : x;
    if (a <= k) return x;                     /* transparent for normal levels */
    float over = a - k;                       /* smoothly map (k..∞) → (k..1) */
    float comp = k + (1.0f - k) * (over / (over + (1.0f - k)));
    return x < 0.0f ? -comp : comp;
}

/* Step 12b #1 — musical state driving the preset-based reverb mapping. */
static int   musical_mode  = 0;     /* ionian */
static int   musical_vibe  = 0;     /* warm */
static float musical_space = 0.5f;
static float musical_mood  = 0.5f;

/* Recompute Freeverb settings from current mode/vibe/space/mood and push to
 * the reverb. Cached size/damp keep manual reverb setters orthogonal. */
static void recompute_reverb_from_presets(void) {
    reverb_settings_t s = reverb_presets_compute(musical_mode, musical_vibe,
                                                 musical_space, musical_mood);
    reverb_size = s.size;
    reverb_damp = s.damp;
    reverb_apply();
    reverb_set_drive(s.drive);
    wet_amp_tgt = s.wet_amp;        /* still smoothed per block in render */
}

void engine_init(void) {
    pad_init();
    reverb_init();
    texture_init();
    ambience_init();
    tape_init();
    echo_init();
    blur_init();
    bass_init();
    drone_init();

    /* Musical state defaults: C ionian / warm / space=mood=0.5. The reverb
     * parameters fall out of the preset table — same shape as the webapp's
     * mid-mode landing point. */
    musical_mode  = 0;
    musical_vibe  = 0;
    musical_space = 0.5f;
    musical_mood  = 0.5f;
    send_amount_cur = send_amount_tgt = 0.45f;
    wet_amp_cur     = wet_amp_tgt     = 0.40f;
    recompute_reverb_from_presets();
    wet_amp_cur = wet_amp_tgt;       /* snap at boot, no glide-from-silence */

    /* Texture bed boots at 0 (silent power-up); raise via engine_set_texture
     * or the brain in Step 12. */
    texture_set_amount(0.0f);
    bass_set_depth(0.5f);
    memset(active_freq, 0, sizeof active_freq);
    generative_init();
    cells_init();                    /* ADR-0013 cell-velocity state */
    gen_on = false;
    gen_timing_valid = false;        /* r18.88 autoplay scheduler reset */
    composer_init();                 /* r18.96 top-level intent reset   */
    harmony_init();                  /* r19.0 harmonic safety core */
    gen_state_seen = -1;
    mel_next_ms = 0; mel_off_ms = 0; mel_sounding = 0;
    mel_phrase_left = 0; mel_last_midi = 0; mel_note_count = 0;
    mel_hist_len = mel_cur_len = 0;
    mel_replay = 0; mel_replay_idx = 0;
    mel_dejavu_count = 0;
    snd_bed_midi = 0;
    memset(snd_eno_midi, 0, sizeof snd_eno_midi);
    vmix_walk = 0.0f; vmix_next_ms = 0;

    /* Master stage: DC-block cleared, moderate default volume (no on-device
     * volume knob bound yet — keeps headphones from being slammed). */
    dc_x1L = dc_y1L = dc_x1R = dc_y1R = 0.0f;
    master_vol_cur = master_vol_tgt = 0.6f;
    drive_cur = drive_tgt = 0.0f;
    pluck_init();                    /* r18.89 sparkle plucks */
    glass_init();                    /* r18.98 FM glass voice */
    shimmer_init();                  /* r18.99 octave-up hall regeneration */
    memset(eno_next_ms, 0, sizeof eno_next_ms);
    memset(eno_off_ms,  0, sizeof eno_off_ms);
    memset(eno_on,      0, sizeof eno_on);
    eno_timing_valid = 0;
    melody_voice = 0;                /* PAD — the bench-tuned reference */
    body_init();                     /* r18.94 modal body (per-world material) */

    /* Boot world 0 (Tokyo) — align the harmonic identity with the displayed
     * world so the first cell tap already plays in A-major, not the bare
     * C-ionian default. Sets brain key/mode/vibe + ambience world + reverb
     * preset; produces no sound (no notes held). */
    engine_set_world(0);
}

/* r18.98 VOICE — the melody-instrument selector (user: "das Instrument
 * kann nicht genug"). 0 = PAD (reference sound, cells swell as before),
 * 1 = STRING, 2 = GLASS: every cell press ADDITIONALLY strikes the chosen
 * melody voice, so playing gets an articulate attack in front of the pad
 * swell — piano-into-pad feel. The generative sparkles follow the same
 * choice (GLASS replaces the KS string; PAD keeps the string — the bed
 * needs SOME second colour, that was the whole r18.89 point). */
void engine_set_voice(int voice_idx) {
    if (voice_idx < 0) voice_idx = 0;
    if (voice_idx > 2) voice_idx = 2;
    melody_voice = voice_idx;
}

/* Fire the selected melody voice (used by cell presses + sparkles). */
static void melody_strike(float freq_hz, float amp) {
    if (melody_voice == 2) glass_note(freq_hz, amp);
    else                   pluck_note(freq_hz, amp);
}

/* Tier A #2: tiny LCG for micro-humanisation. Inside JND so it doesn't drift
 * audibly, but enough that two consecutive identical cell taps aren't
 * bit-identical → no "mechanical" feel on repeats. */
static uint32_t humanize_rng = 0xA5C3F19Du;
static inline float humanize_rand_unit(void){      /* in [-1, +1] */
    humanize_rng = humanize_rng * 1664525u + 1013904223u;
    return ((int32_t)humanize_rng) * (1.0f / 2147483648.0f);
}

void engine_note_on(uint8_t source, float freq_hz, float amp) {
    /* ±0.5 cent pitch jitter, ±0.3 % amp jitter. Bass / drone get the same
     * freq downstream (refresh_bass) so the jitter is consistent per press. */
    float pitch_jitter = humanize_rand_unit() * (0.5f / 1200.0f);   /* cents */
    float amp_jitter   = humanize_rand_unit() * 0.003f;
    freq_hz *= 1.0f + pitch_jitter;                    /* 2^(jit) ≈ 1+jit at tiny jit */
    amp     *= 1.0f + amp_jitter;
    pad_note_on(source, freq_hz, amp);
    if (source < MAX_SOURCES) active_freq[source] = freq_hz;
    /* r18.98 VOICE: cell sources (base 0..4 + shift 9..13) also strike the
     * selected melody voice — an articulate attack in front of the pad
     * swell. The generative bed (8) and sparkles (14/15) are excluded:
     * the bed must stay a bed. Amp is scaled to sit like the sparkles. */
    if (melody_voice != 0 &&
        (source <= 4 || (source >= 9 && source <= 13)))
        melody_strike(freq_hz, dsp_clampf(amp * 1.4f, 0.0f, 0.30f));
    refresh_bass();
}
void engine_note_off(uint8_t source) {
    pad_note_off(source);
    if (source < MAX_SOURCES) active_freq[source] = 0.0f;
    refresh_bass();
}
void engine_all_off(void) {
    pad_all_off();
    memset(active_freq, 0, sizeof active_freq);
    bass_release();
}

/* ADR-0013 — Hall cell sample → velocity note. The cell index doubles as the
 * pad-voice source (0..4), matching the digital-cell path, so re-pressing a
 * cell re-blooms its own voice rather than stacking. */
bool engine_cell_sample(uint8_t cell, float pos_0_1, uint32_t now_ms) {
    cell_event_t ev = cells_update(cell, pos_0_1, now_ms);
    if (ev.kind == CELL_EVENT_PRESS) {
        int midi = brain_cell_root(ev.cell);
        engine_note_on(ev.cell, tuning_hz((float)midi), ev.amp);
        return true;
    }
    if (ev.kind == CELL_EVENT_RELEASE) {
        engine_note_off(ev.cell);
        return true;
    }
    return false;
}

void engine_set_reverb_size(float v) {
    reverb_size = dsp_clampf(v, 0.0f, 1.0f);
    reverb_apply();
}
void engine_set_reverb_damp(float v) {
    reverb_damp = dsp_clampf(v, 0.0f, 1.0f);
    reverb_apply();
}
void engine_set_reverb_drive(float v) { reverb_set_drive(dsp_clampf(v, 0.0f, 1.0f)); }
void engine_set_wet_amp(float v)      { wet_amp_tgt    = dsp_clampf(v, 0.0f, 1.0f); }
void engine_set_send(float v)         { send_amount_tgt = dsp_clampf(v, 0.0f, 1.0f); }
void engine_set_master_volume(float v){ master_vol_tgt  = dsp_clampf(v, 0.0f, 1.0f); }
void engine_set_drive(float v)        { drive_tgt       = dsp_clampf(v, 0.0f, 1.0f); }
/* r18.90 macro: one emotional dimension, three destinations. hz is the
 * legacy pad-cutoff offset (-600..+800); the hall and the plucks follow.
 * At hz=0 all trims are exactly 0 — the bench-tuned default is untouched. */
void engine_set_brightness(float hz)  {
    pad_set_brightness(hz);
    bright_damp_trim = dsp_clampf(-hz * (0.18f / 800.0f), -0.135f, 0.18f);
    reverb_apply();
    pluck_set_damp(dsp_clampf(0.42f - hz * (0.12f / 800.0f), 0.28f, 0.55f));
}
void engine_set_texture(float v)      { texture_set_amount(dsp_clampf(v, 0.0f, 1.0f)); }
void engine_set_atmosphere(float v)   { ambience_set_level(dsp_clampf(v, 0.0f, 1.0f)); }
/* World change applies BOTH the texture layer (ambience) AND the harmonic
 * identity (key/mode/vibe) so each world sounds musically distinct, not just
 * texturally. The cell taps then play in the world's key/mode (brain), the
 * drone follows the root, and the reverb character shifts via the per-mode/
 * vibe preset table. Values live in worlds.c (audition-derived). */
void engine_set_world(int idx) {
    ambience_set_world(idx);
    /* r18.93: (re)build the PADsynth bed table for the world's timbre
     * profile. Blocking a few ms — worlds change from the UI loop, never
     * the audio IRQ; the pad keeps reading the old table until the final
     * copy, and the world-change re-bloom masks the swap. */
    padsynth_build(idx, 0);
    body_set_world(idx);             /* r18.94: the pluck's resonant material */
    const world_t *w = worlds_get(idx);
    engine_set_key ((int)w->key_midi);   /* brain key + drone root          */
    engine_set_mode((int)w->mode);       /* brain mode + reverb recompute   */
    engine_set_vibe((int)w->vibe);       /* brain vibe + reverb recompute   */
}
void engine_set_bass_depth(float v)   { bass_set_depth(dsp_clampf(v, 0.0f, 1.0f)); }

/* Perform-macros: combine multiple internal params under one user knob. */
void engine_set_motion(float v) {
    v = dsp_clampf(v, 0.0f, 1.0f);
    /* user 0..1 → pad-LFO depth 0..2 (centre 0.5 = default movement). */
    pad_set_motion(v * 2.0f);
}
void engine_set_age(float v) {
    v = dsp_clampf(v, 0.0f, 1.0f);
    /* user 0..1 → hiss 0..0.015 (≈ -36 dBFS at max) + sat drive 1.0..1.40.
     * v=0.30 lands near the dreamy_warm reference (hiss 0.005, drive 1.10). */
    tape_set_hiss_amount(0.0012f + v * v * 0.005f);   /* r18.97: square, quiet */
    tape_set_saturation_drive(1.0f + v * 0.40f);
    tape_set_crackle(v);             /* r18.89: vinyl ticks join the macro */
    /* r18.99: age also un-steadies the transport — square curve again, so
     * the reference band (v ≤ 0.3) barely moves and old tape gets drunk. */
    tape_set_wow_depth(v * v);
}

/* r18.99 SHIMMER macro (menu slot). Pure pass-through to the module. */
void engine_set_shimmer(float v) {
    shimmer_set_amount(dsp_clampf(v, 0.0f, 1.0f));
}

/* Echo macro — single setter, internally maps to time + feedback + wet +
 * tone in echo.c. See echo.c::recompute_internals for the mapping. */
void engine_set_echo(float v) {
    echo_set_amount(dsp_clampf(v, 0.0f, 1.0f));
}

/* Blur macro — granular smear. See blur.c::recompute_params for the
 * internal mapping (density + grain size + pitch jitter + wet). */
void engine_set_blur(float v) {
    blur_set_amount(dsp_clampf(v, 0.0f, 1.0f));
}

/* Step 12b #1 — musical-state setters. Each triggers a preset recompute so
 * the reverb shifts character with the mode/vibe/macro change. The reverb
 * itself smooths internally (~120 ms), so the transition is glide-not-step
 * — matches the "sound darf nicht konkurrieren" rule for global params.
 * Also keeps the harmonic brain's view of mode/vibe in sync so cells played
 * after the change pick up the new harmony. */
/* r19.0: the harmony core needs TONALITY, not the full mode. Dorian /
 * phrygian / aeolian read as minor pitch worlds; ionian / lydian /
 * mixolydian as major. */
static int mode_is_minor(int mode_idx) {
    return mode_idx == 1 || mode_idx == 2 || mode_idx == 5;
}

void engine_set_mode(int mode_idx) {
    if (mode_idx < 0)               mode_idx = 0;
    if (mode_idx >= RP_MODE_COUNT)  mode_idx = RP_MODE_COUNT - 1;
    musical_mode = mode_idx;
    brain_set_mode(mode_idx);
    harmony_set_world(brain_get_key(), mode_is_minor(mode_idx));   /* r19.0 */
    recompute_reverb_from_presets();
}
void engine_set_vibe(int vibe_idx) {
    if (vibe_idx < 0)               vibe_idx = 0;
    if (vibe_idx >= RP_VIBE_COUNT)  vibe_idx = RP_VIBE_COUNT - 1;
    musical_vibe = vibe_idx;
    brain_set_vibe(vibe_idx);
    recompute_reverb_from_presets();
}
void engine_set_space(float v) {
    musical_space = dsp_clampf(v, 0.0f, 1.0f);
    recompute_reverb_from_presets();
}
void engine_set_mood(float v) {
    musical_mood = dsp_clampf(v, 0.0f, 1.0f);
    recompute_reverb_from_presets();
}
/* r18.98 KEY as a menu slot: the menu speaks PITCH CLASS (0=C..11=B); the
 * engine anchors it in the register the four world tonics live in
 * (F#3..F4, MIDI 54..65) so a key change transposes, never jumps octaves:
 * pc 6..11 → 54..59, pc 0..5 → 60..65. Tokyo A=57, Coast D=62,
 * Drive F#=54, Hours C=60 all round-trip exactly. */
void engine_set_key_pc(int pc) {
    pc %= 12; if (pc < 0) pc += 12;
    engine_set_key(pc >= 6 ? 48 + pc : 60 + pc);
}

void engine_set_key(int tonic_midi) {
    brain_set_key(tonic_midi);
    harmony_set_world(tonic_midi, mode_is_minor(musical_mode));    /* r19.0 */
    tuning_set_key(tonic_midi);        /* r19.6 anchor just intonation      */
    drone_set_root_midi(tonic_midi);   /* glides live if the drone is sounding */
}
void engine_set_drone(bool on) { drone_enable(on); }

/* r19.6 — tuning: 0 = equal temperament (reference), 1 = just intonation. */
void engine_set_tuning(int just) { tuning_set_mode(just); }

/* PAD_VOICE_MIXES from the webapp: warm / strings / brass. */
void engine_set_pad_voice(int voice_idx) {
    static const float MIX[] = { 0.0f, 0.6f, 1.2f };
    if (voice_idx < 0) voice_idx = 0;
    if (voice_idx > 2) voice_idx = 2;
    pad_set_voice_mix(MIX[voice_idx]);
}

/* r18.88 AUDIT-FIX: this used to check only cells 0..4 — the Shift-octave
 * latch voices live on sources 9..13 (ADR-0008 r2, controls.c SHIFT_SRC),
 * so the generative bed kept playing OVER the user's held shift notes.
 * "User is playing" = any cell source, base or shift octave. The bed's own
 * voice (8) and the sparkle sources (14/15) are deliberately excluded. */
static bool any_user_note(void) {
    for (int i = 0; i < 5; ++i)  if (active_freq[i] > 0.0f) return true;
    for (int i = 9; i < 14; ++i) if (active_freq[i] > 0.0f) return true;
    return false;
}

/* --- Generative autoplay (r18.88) -----------------------------------------
 * The GENERATE modifier was always meant to make the instrument PLAY BY
 * ITSELF (passive mode = music without hands). The old wiring gave one
 * chord-root swell per fixed 8 s bar, with up to 8 s of silence after
 * enabling. engine_generative_tick() replaces the caller-side bar timer:
 *
 *   - the FIRST bed note sounds on the first tick after enabling (no wait),
 *   - bar length is humanized (±10 % per bar, LCG — never metronomic),
 *   - each bar scatters 0-2 "sparkle" chord tones an octave up (sources
 *     14/15, ~3 s ring, quiet), so the bed breathes as actual music,
 *   - live playing still overrides everything: while any user note is held
 *     (base or shift) no new bed/sparkle notes start; ringing sparkles are
 *     still released on schedule. When the user lets go, the bed resumes on
 *     the next tick.
 *
 * All timing derives from the passed now_ms — hardware-independent and
 * host-testable. The old engine_generative_advance() stays as the manual
 * step API (offline renderers, tests). State lives up by gen_on. */

static float gen_rand01(void) {
    gen_tick_rng = gen_tick_rng * 1664525u + 1013904223u;
    return (float)(gen_tick_rng >> 8) / 16777216.0f;
}

void engine_set_generative(bool on, int program) {
    generative_set_program(program);
    if (on && !gen_on) gen_timing_valid = false;   /* first tick plays NOW */
    gen_on = on;
    if (!on) {
        engine_note_off(GEN_SOURCE);    /* release the bed voice */
        snd_bed_midi = 0;
        engine_note_off((uint8_t)MEL_SRC);   /* r19.0: melody voice too */
        mel_sounding = 0;
        for (int i = 0; i < ENO_LOOPS; ++i) {   /* r18.99: loops out too */
            if (eno_on[i]) engine_note_off(ENO_SRC(i));
            eno_on[i] = 0;
            snd_eno_midi[i] = 0;
        }
        eno_timing_valid = 0;
    }
}

/* r19.0: manual step = one harmonic-state MUTATION (≥3 common pitch
 * classes, ≤2 voices move — harmony.c enforces the contract). The pad
 * bed voice sits an octave over the bass register; bass.c derives the
 * low fundament from it (lowest_held). Returns state index + 1 (1..4). */
int engine_generative_advance(void) {
    if (!gen_on) return -1;
    if (any_user_note()) return -1;          /* live playing overrides the bed */
    harmony_advance();
    int midi = harmony_bass_midi() + 12;
    snd_bed_midi = midi;
    gen_state_seen = harmony_state_changes();
    engine_note_on((uint8_t)GEN_SOURCE, tuning_hz((float)midi),
                   GEN_VOICE_AMP * composer_params()->bed_amp);
    return harmony_state_index() + 1;
}

int engine_generative_last_melody_midi(void) { return mel_last_midi; }
int engine_generative_melody_count(void)     { return mel_note_count; }
int engine_generative_dejavu_count(void)     { return mel_dejavu_count; }

void engine_generative_tick(uint32_t now_ms) {
    if (!gen_on) return;
    /* r18.96 — the COMPOSER above the grammar (Atmoscapia/Eno principle:
     * a slow top-level intent that only re-weights probabilities; see
     * composer.h). Ticks on the same clock as everything else. */
    composer_tick(now_ms);

    if (any_user_note()) {
        /* Player takes over: freeze everything and re-arm so the piece
         * resumes promptly after release. */
        gen_timing_valid = false;
        return;
    }

    if (!gen_timing_valid) {           /* just enabled / just released */
        gen_timing_valid = true;
        gen_state_seen = -1;           /* strike the bed on THIS tick   */
        mel_next_ms = now_ms + 1500u + (uint32_t)(gen_rand01() * 2500.0f);
        if (mel_sounding && (int32_t)(now_ms - mel_off_ms) >= 0) {
            engine_note_off((uint8_t)MEL_SRC);   /* stale note from before
                                                  * a long user hold      */
            mel_sounding = 0;
        }
    }

    /* --- r18.99 ENO LOOPS (see the block comment at the top) ------------
     * Three one-note tape loops with incommensurate periods. Staggered
     * first entries (0.4 / 0.62 / 0.81 of a period) so the choir fades in
     * voice by voice instead of striking a chord. Each cycle: release the
     * old tone, sound THIS loop's chord member of the CURRENT harmony,
     * hold for 62 % of the period, rest for the remainder — the gaps are
     * where the recombination shows. */
    if (!eno_timing_valid) {
        eno_next_ms[0] = now_ms + (uint32_t)(ENO_PERIOD_MS[0] * 0.40f);
        eno_next_ms[1] = now_ms + (uint32_t)(ENO_PERIOD_MS[1] * 0.62f);
        eno_next_ms[2] = now_ms + (uint32_t)(ENO_PERIOD_MS[2] * 0.81f);
        for (int i = 0; i < ENO_LOOPS; ++i) { eno_on[i] = 0; eno_off_ms[i] = 0; }
        eno_timing_valid = 1;
    }
    for (int i = 0; i < ENO_LOOPS; ++i) {
        if (eno_on[i] && (int32_t)(now_ms - eno_off_ms[i]) >= 0) {
            engine_note_off(ENO_SRC(i));
            eno_on[i] = 0;
            snd_eno_midi[i] = 0;
        }
        if ((int32_t)(now_ms - eno_next_ms[i]) >= 0) {
            /* r19.0: each loop owns ONE voice of the harmonic STATE (the
             * voiced, collision-safe upper harmony) — the recombination
             * always lands inside the pitch world. */
            int hv[HARMONY_VOICES];
            harmony_voices(hv, HARMONY_VOICES);
            {
                int midi = hv[i];
                while (midi > 74) midi -= 12;    /* keep the choir mid-low */
                while (midi < 50) midi += 12;
                if (eno_on[i]) engine_note_off(ENO_SRC(i));
                engine_note_on(ENO_SRC(i), tuning_hz((float)midi),
                               ENO_AMP[i] * composer_params()->bed_amp);
                eno_on[i]       = 1;
                snd_eno_midi[i] = midi;
                eno_off_ms[i] = now_ms + (uint32_t)(ENO_PERIOD_MS[i] * 0.62f);
            }
            /* phase NEVER resets — the drift is the composition. The while
             * catches up after a long user hold (ticks pause under a
             * playing human) without machine-gunning retriggers. */
            while ((int32_t)(now_ms - eno_next_ms[i]) >= 0)
                eno_next_ms[i] += ENO_PERIOD_MS[i];
        }
    }

    /* --- r19.0 HARMONIC STATE (harmony.h) -------------------------------
     * The state machine dwells 24-48 s, then MUTATES: ≥3 common pitch
     * classes, ≤2 voices move, common tones frozen at pitch. The bed
     * re-sounds only when the state actually changed — held notes get
     * REINTERPRETED by the new bass, not re-struck (the user's research
     * brief: "Wir sollten Akkorde nicht auswählen, sondern den nächsten
     * harmonischen Zustand aus dem vorherigen mutieren"). */
    harmony_tick(now_ms);
    bass_set_depth(composer_params()->bass_depth);
    if (harmony_state_changes() != gen_state_seen) {
        gen_state_seen = harmony_state_changes();
        int midi = harmony_bass_midi() + 12;   /* pad bed over the bass reg;
                                                * bass.c derives the low
                                                * fundament via lowest_held */
        snd_bed_midi = midi;
        engine_note_on((uint8_t)GEN_SOURCE, tuning_hz((float)midi),
                       GEN_VOICE_AMP * composer_params()->bed_amp);
    }

    /* --- r19.0 LONG MELODY VOICE -----------------------------------------
     * One voice. Long tones (4-16 s), real silences (1-8 s, stretched by
     * the composer's rest_add), phrases of 2-5 notes with a longer breath
     * after each phrase. EVERY tone runs the full safety chain in
     * harmony_melody_next: pitch world → register mask → interval table →
     * collision filter against everything sustaining → next-best
     * fallback. Probability is the LAST stage, not the first. The onset
     * is doubled by the selected VOICE strike (string/glass) so the tone
     * has an attack in front of the pad swell — not an arpeggiator, a
     * slow cinematic line. */
    if (mel_sounding && (int32_t)(now_ms - mel_off_ms) >= 0) {
        engine_note_off((uint8_t)MEL_SRC);
        mel_sounding = 0;
        float rest = 1.0f + gen_rand01() * 7.0f;              /* 1-8 s   */
        rest *= 1.0f + composer_params()->rest_add * 2.0f;    /* composer */
        if (mel_phrase_left <= 0)
            rest += 3.0f + gen_rand01() * 5.0f;               /* breath  */
        mel_next_ms = now_ms + (uint32_t)(rest * 1000.0f);
    }
    if (!mel_sounding && (int32_t)(now_ms - mel_next_ms) >= 0) {
        if (mel_phrase_left <= 0) {
            if (mel_cur_len >= 2) {              /* archive the phrase   */
                for (int i = 0; i < mel_cur_len; ++i) mel_hist[i] = mel_cur[i];
                mel_hist_len = mel_cur_len;
            }
            mel_cur_len = 0;
            mel_phrase_left = 2 + (int)(gen_rand01() * 4.0f);   /* 2..5  */
            mel_replay = (mel_hist_len >= 2 && gen_rand01() < 0.35f);
            mel_replay_idx = 0;
            if (mel_replay) ++mel_dejavu_count;
            /* mel_last_midi is NOT reset: the new phrase steps off from
             * where the old one ended, so voice-leading survives the
             * breath (a reset caused over-octave leaps between phrases —
             * caught by the grammar audit). */
        }
        if (gen_rand01() < dsp_clampf(0.80f * composer_params()->mel_density,
                                      0.05f, 0.95f)) {
            /* everything currently sustaining, for the collision filter */
            int sus[2 + ENO_LOOPS]; int nsus = 0;
            if (snd_bed_midi) sus[nsus++] = snd_bed_midi;
            for (int i = 0; i < ENO_LOOPS; ++i)
                if (snd_eno_midi[i]) sus[nsus++] = snd_eno_midi[i];

            int tone = -1;
            if (mel_replay && mel_replay_idx < mel_hist_len) {
                int want = mel_hist[mel_replay_idx++];
                if (harmony_in_world(want) &&
                    harmony_collision_ok(want, sus, nsus))
                    tone = want;                 /* the motif survives   */
            }
            if (tone < 0)
                tone = harmony_melody_next(mel_last_midi, sus, nsus,
                                           0.05f + composer_params()->high_p);
            if (tone > 0) {
                float hz  = tuning_hz((float)tone);
                float amp = 0.062f + gen_rand01() * 0.014f;
                engine_note_on((uint8_t)MEL_SRC, hz, amp);
                melody_strike(hz, amp * 2.0f);   /* articulate the onset */
                mel_sounding  = 1;
                mel_last_midi = tone;
                ++mel_note_count;
                if (mel_cur_len < MEL_PHRASE_MAX) mel_cur[mel_cur_len++] = tone;
                --mel_phrase_left;
                mel_off_ms = now_ms + 4000u +
                             (uint32_t)(gen_rand01() * 12000.0f);   /* 4-16 s */
            } else {
                /* nothing SAFE right now — silence is the correct note */
                mel_next_ms = now_ms + 2000u +
                              (uint32_t)(gen_rand01() * 3000.0f);
            }
        } else {
            mel_next_ms = now_ms + 2000u + (uint32_t)(gen_rand01() * 4000.0f);
        }
    }

    /* --- r19.0 spectral undulation (Blendwave principle) ------------------
     * "Same note, evolve timbre": every 400 ms the pad brightness tilt
     * takes one small CORRELATED step (bounded random walk: 20 21 23 26,
     * never 20 97 4) — the bed undulates without new events. */
    if ((int32_t)(now_ms - vmix_next_ms) >= 0) {
        vmix_walk += (gen_rand01() - 0.5f) * 0.10f;
        if (vmix_walk < -0.30f) vmix_walk = -0.30f;
        if (vmix_walk >  0.50f) vmix_walk =  0.50f;
        pad_set_voice_mix(0.45f + vmix_walk);
        vmix_next_ms = now_ms + 400u;
    }
}

void engine_render(int16_t *buf, int frames) {
    /* audio.c always calls with frames == AUDIO_BUFFER_FRAMES, but be safe. */
    if (frames > BLOCK) frames = BLOCK;

    /* Smooth per-block engine controls. */
    send_amount_cur += SMOOTH_COEF * (send_amount_tgt - send_amount_cur);
    wet_amp_cur     += SMOOTH_COEF * (wet_amp_tgt     - wet_amp_cur);

    /* Clear the dry + send accumulators (pad ADDS into them). */
    memset(dryL,  0, sizeof(float) * frames);
    memset(dryR,  0, sizeof(float) * frames);
    memset(sendL, 0, sizeof(float) * frames);
    memset(sendR, 0, sizeof(float) * frames);

    pad_render_mix(dryL, dryR, sendL, sendR, frames, send_amount_cur);
    texture_render_mix(dryL, dryR, sendL, sendR, frames, TEXTURE_SEND);
    ambience_render_mix(dryL, dryR, sendL, sendR, frames, AMBIENCE_SEND);
    bass_render_mix(dryL, dryR, sendL, sendR, frames);
    drone_render_mix(dryL, dryR, sendL, sendR, frames);
    /* r18.94: plucks render onto their own bus, run through the MODAL
     * BODY (fixed per-world resonances — the string varies, the body does
     * not; Rings/Elements concept, see body.h), then join dry + hall send.
     * Only the melody voice gets a body: the bed stays clean. */
    {
        static float plkL[BLOCK], plkR[BLOCK];
        static float plkJL[BLOCK], plkJR[BLOCK];   /* pluck's own send: unused */
        memset(plkL, 0, sizeof(float) * (size_t)frames);
        memset(plkR, 0, sizeof(float) * (size_t)frames);
        memset(plkJL, 0, sizeof(float) * (size_t)frames);
        memset(plkJR, 0, sizeof(float) * (size_t)frames);
        pluck_render_mix(plkL, plkR, plkJL, plkJR, frames);
        glass_render_mix(plkL, plkR, plkJL, plkJR, frames);   /* r18.98 */
        body_process(plkL, plkR, frames);
        for (int n = 0; n < frames; ++n) {
            dryL[n]  += plkL[n];
            dryR[n]  += plkR[n];
            sendL[n] += plkL[n] * 0.5f;            /* pluck VERB_SEND kept */
            sendR[n] += plkR[n] * 0.5f;
        }
    }

    /* Echo sits AFTER all generators but BEFORE the reverb so the delay
     * picks up the whole mix; send_amount=0.40 routes a copy of the
     * delayed signal into the reverb send so the echo also picks up room. */
    echo_render_mix(dryL, dryR, sendL, sendR, frames, 0.40f);

    /* Blur (granular cloud) — captures the dry+echo mix and re-emits as a
     * cloud of grains. Sits after echo so it grains the echoes too; before
     * the reverb so the cloud picks up room. send_amount=0.50 — cloud +
     * reverb is the classic "frozen-shimmer" wash. */
    blur_render_mix(dryL, dryR, sendL, sendR, frames, 0.50f);

    /* Tape character (ADR-0017 Phase 3): subtle hiss into the DRY bus only —
     * we don't want the reverb to amplify the noise floor. r18.92: feed the
     * program follower first so hiss/crackle duck when the music breathes. */
    {
        float pk = 0.0f;
        for (int n = 0; n < frames; ++n) {
            float a = dryL[n] < 0.0f ? -dryL[n] : dryL[n];
            float b = dryR[n] < 0.0f ? -dryR[n] : dryR[n];
            if (a > pk) pk = a;
            if (b > pk) pk = b;
        }
        tape_set_program_level(pk);
    }
    tape_hiss_render_add(dryL, dryR, frames);

    /* r18.89: vinyl crackle (AGE macro) — dry bus only, like the hiss. */
    tape_crackle_render_add(dryL, dryR, frames);

    /* Reverb writes (does not add) wet from send. r18.99: the shimmer
     * loop wraps it — last block's wet, one octave up and darkened,
     * re-enters the send; each pass climbs and dies (Sonicware/Eno
     * shimmer principle, see shimmer.h). */
    shimmer_feed_add(sendL, sendR, frames);
    reverb_render(sendL, sendR, wetL, wetR, frames);
    shimmer_capture(wetL, wetR, frames);

    /* Master: sum + DC-block + volume → outL/outR; then tanh warmth
     * (block call); then soft-limit safety net → int16. Two-pass keeps
     * the saturation block-call cheap (one loop body, one tanh call/sample). */
    master_vol_cur += SMOOTH_COEF * (master_vol_tgt - master_vol_cur);
    drive_cur      += SMOOTH_COEF * (drive_tgt      - drive_cur);
    const float wa = wet_amp_cur;
    const float mv = master_vol_cur;

    /* r18.89 master drive (per block: curve params + makeup are constant
     * inside a 5.8 ms block; the amount itself is smoothed above). */
    const bool  drv_on   = drive_cur > 1.0e-3f;
    const float drv_g    = 1.0f + 3.5f * drive_cur;          /* 1 .. 4.5  */
    const float drv_bias = 0.28f * drive_cur;                /* asymmetry */
    const float drv_mk   = dsp_drive_makeup(drv_g, drv_bias);
    const float drv_mix  = drive_cur < 0.25f ? drive_cur * 4.0f : 1.0f;

    static float outL[BLOCK], outR[BLOCK];
    for (int n = 0; n < frames; ++n) {
        float L = dryL[n] + wetL[n] * wa;
        float R = dryR[n] + wetR[n] * wa;

        if (drv_on) {
            /* dry/wet fade over the first quarter of the knob so tiny
             * settings colour instead of switch. */
            float dL = dsp_drive_shape(L, drv_g, drv_bias) * drv_mk;
            float dR = dsp_drive_shape(R, drv_g, drv_bias) * drv_mk;
            L += drv_mix * (dL - L);
            R += drv_mix * (dR - R);
        }

        /* one-pole DC blocker per channel: y = x - x1 + R·y1 (also eats the
         * small DC offset the drive bias introduces) */
        float yL = L - dc_x1L + DC_R * dc_y1L; dc_x1L = L; dc_y1L = yL;
        float yR = R - dc_x1R + DC_R * dc_y1R; dc_x1R = R; dc_y1R = yR;

        outL[n] = yL * mv;
        outR[n] = yR * mv;
    }
    /* r18.99 WOW & FLUTTER: the whole mix (hall tail included) breathes
     * like a tape recording of the performance. AGE drives the depth;
     * depth 0 is bit-exact bypass. Before the saturation — pitch wobble
     * happens on the tape, the head saturation after it. */
    tape_wow_process(outL, outR, frames);
    /* Warm tanh saturation — "tape" colour. Always-on at the dreamy_warm
     * reference drive (1.10). Peaks roll into the tanh knee, even harmonics
     * appear, makeup-gain implicit (×0.78 inside). */
    tape_saturation_process(outL, outR, frames);
    for (int n = 0; n < frames; ++n) {
        /* soft_limit is now a safety net for the rare residual peak above
         * 0.75 — saturation usually already keeps us in range. */
        float yL = soft_limit(outL[n]);
        float yR = soft_limit(outR[n]);
        buf[n * 2 + 0] = (int16_t)(yL * 32767.0f);
        buf[n * 2 + 1] = (int16_t)(yR * 32767.0f);
    }
}

int engine_active_voices(void) { return pad_active_count(); }
