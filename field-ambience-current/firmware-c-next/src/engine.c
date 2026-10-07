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
#include "fx_master.h"
#include "texture.h"
#include "ambience.h"
#include "bass.h"
#include "drone.h"
#include "reverb_presets.h"
#include "brain.h"
#include "worlds.h"
#include "generative.h"
#include "cells.h"
#include "pluck.h"
#include "ember.h"
#include "bowed.h"
#include "horn.h"
#include "choir.h"
#include "guembri.h"
#include "shape.h"
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
static float active_freq[MAX_SOURCES];
/* Fixed pitch-indexed memory: no eviction of still-protected older tails.
 * Envelope/FX horizons are conservative estimates, not spectral analysis. */
static uint32_t sound_ms, sound_fraction, tail_until[128], source_tail_ms[MAX_SOURCES];
static uint8_t tail_valid[128];
static int bass_root_midi, drone_root_midi, memory_fx_mode;
static bool drone_on, auto_has_onset;
static float memory_space, memory_echo;
static uint32_t auto_last_ms;
static int pitch_of(float hz) {
    return hz>1.0f ? (int)lrintf(69.0f+12.0f*log2f(hz/440.0f)):-1;
}
static uint32_t tail_horizon(void) {
    uint32_t ms=(uint32_t)(8800.0f*shape_release_scale()+800.0f*shape_attack_scale());
    int mode=fx_master_mode();
    if((mode!=0 && mode!=3 && mode!=4) ||
       (memory_fx_mode!=0 && memory_fx_mode!=3 && memory_fx_mode!=4))
        ms+=(uint32_t)(6000.0f+memory_space*12000.0f+memory_echo*8000.0f);
    return ms;
}
static void remember_pitch(int midi,uint32_t horizon) {
    if(midi<0 || midi>127) return;
    uint32_t until=sound_ms+horizon;
    if(!tail_valid[midi] || (int32_t)(until-tail_until[midi])>0) tail_until[midi]=until;
    tail_valid[midi]=1;
}
static void remember_source(int source) {
    if(source<0 || source>=MAX_SOURCES || active_freq[source]<=0.0f) return;
    uint32_t horizon=tail_horizon();
    if(source_tail_ms[source]>horizon) horizon=source_tail_ms[source];
    remember_pitch(pitch_of(active_freq[source]),horizon);
}
static void remember_bass(void) {
    if(bass_root_midi>0) {
        remember_pitch(bass_root_midi-12,tail_horizon());
        remember_pitch(bass_root_midi-24,tail_horizon());
    }
}
int engine_sounding_notes(int *out,int max) {
    uint8_t present[128]={0};
    for(int m=0;m<128;++m) present[m]=tail_valid[m] && (int32_t)(tail_until[m]-sound_ms)>0;
    for(int i=0;i<MAX_SOURCES;++i) {
        int m=pitch_of(active_freq[i]); if(m>=0 && m<128) present[m]=1;
    }
    if(bass_root_midi>=24 && bass_root_midi<128) {
        present[bass_root_midi-12]=1; present[bass_root_midi-24]=1;
    }
    if(drone_on && drone_root_midi>=12 && drone_root_midi<128) {
        present[drone_root_midi]=1; present[drone_root_midi-12]=1;
    }
    int n=0;
    for(int m=0;m<128 && n<max;++m) if(present[m]) out[n++]=m;
    return n;
}
static bool auto_ready(void) {
    return !auto_has_onset || (uint32_t)(sound_ms-auto_last_ms)>=1400u;
}
static void auto_onset(void) { auto_last_ms=sound_ms; auto_has_onset=true; }
static void release_generated(void);


/* Optional note-event tap (MIDI out lives behind this so the engine keeps no
 * link dependency on midi.c — the product wires it in main_h743). on: 1=on,
 * 0=off, -1=all-off. */
static engine_note_hook_t s_note_hook = 0;
void engine_set_note_hook(engine_note_hook_t h) { s_note_hook = h; }

/* r19.16 SYNTH-mode state (logic lives beside engine_render below). */
static const engine_synth_backend_t *s_synth_be = 0;
static float s_synth_blend;
/* Audio-owned gains; control only requests a bounded release overlap. */
static float s_ambient_gain, s_background_gain;
static volatile uint32_t s_ambient_tail_frames;
static volatile uint32_t s_muted_tail_frames;
/* Audio publishes once per block; control does not read a per-sample gain. */
static volatile bool s_native_overlap;
static uint32_t s_tail_quiet_frames;
#define AMBIENT_TAIL_MAX_FRAMES (64u * DSP_SAMPLE_RATE_HZ)
static uint8_t s_note_stack[MAX_SOURCES], s_note_count;
static float s_note_amp[MAX_SOURCES];
static float s_synth_macros[4];
static volatile int s_synth_tgt = 0;       /* set at control rate            */
static int s_manual_synth, s_world_index;
static void activate_synth(int idx, bool force);
static int   melody_voice;          /* r18.98 VOICE: 0 PAD, 1 STRING, 2 GLASS */

/* Autonomous World state: harmony evolves independently of sound. Only
 * admitted World events create tones; no universal pad, bass or loop bed. */
static bool     gen_on = false;
static bool     gen_timing_valid = false;
static uint32_t gen_tick_rng     = 0x5EEDBA55u;
/* r19.33 player-priority state (declared here so engine_init can reset it). */
#define GEN_RETURN_MS   8000u            /* auto-content returns ~8 s after play */
static uint32_t s_last_active_ms = 0;
static bool     s_ever_active     = false;
static bool     s_gen_suppressed  = false;
static bool s_user_present = false;
/* Explicit low-level World-event gate; disabling it adds no substitute bed. */
static bool     s_mel_enabled = true;
static bool     s_gentle_return = false; /* r19.34: first melody note after the
                                          * player-priority silence swells in
                                          * softly (no bright ding, low register) */
void engine_set_autoplay_melody(int on) { s_mel_enabled = on ? true : false; }
int  engine_autoplay_melody(void)        { return s_mel_enabled ? 1 : 0; }
/* Source 15 owns the World's tone, including its scheduled release. */
#define MEL_SRC 15
static uint32_t mel_next_ms;          /* next decision time              */
static uint32_t mel_off_ms;           /* scheduled note-off              */
static int      mel_sounding;
static int      mel_phrase_left;      /* notes left in current phrase    */
static int      mel_last_midi;        /* voice-leading memory (0 = none) */
static int      mel_repeat_run;
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

/* Lowest currently-held frequency, or 0 if nothing is held. */
static float lowest_held(void) {
    float lo = 0.0f;
    for (int i = 0; i < MAX_SOURCES; ++i) {
        float f = active_freq[i];
        if (f > 0.0f && (lo == 0.0f || f < lo)) lo = f;
    }
    return lo;
}

/* Re-point the bass at the current lowest note, or release it if none held. */
/* r19.31: when a cell mode owns the bass explicitly (HARMONY bass modes), the
 * engine stops auto-following the lowest held note. NOTE/LAND keep following. */
static bool s_bass_follow = true;
void engine_bass_follow(bool on) { s_bass_follow = on; }
void engine_bass_set(float freq_hz) {
    if(!isfinite(freq_hz) || freq_hz<=1.0f) return;
    int midi=pitch_of(freq_hz);
    if(midi!=bass_root_midi) { remember_bass(); bass_root_midi=midi; }
    bass_note(freq_hz);
}
void engine_bass_off(void) { remember_bass(); bass_root_midi=0; bass_release(); }
void engine_bass_glide(float tau_s) { bass_set_glide(tau_s); }
bool engine_bass_active(void)       { return bass_active(); }

static void refresh_bass(void) {
    /* A World release must never wake a bass underneath another World tone.
     * Manual bass-follow remains a separate preference, unchanged by listening. */
    if (gen_on || !s_bass_follow) return;
    float lo = lowest_held();
    if (lo > 0.0f) engine_bass_set(lo);
    else           engine_bass_off();
}

#define BLOCK     AUDIO_BUFFER_FRAMES

static float dryL [BLOCK];
static float dryR [BLOCK];
static float sendL[BLOCK];
static float sendR[BLOCK];
static float foreground_beforeL[BLOCK], foreground_beforeR[BLOCK];
static float foreground_level, bed_gain=1.0f;

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
/* r19.52: the AUDIT found the worlds were ~90 % mono sub-bass (spectral
 * centroid 93–151 Hz, stereo width ~0.02) — the drone/bass low end masked all
 * the mid character and collapsed the stereo. A 2nd master high-pass at ~62 Hz
 * removes the useless sub-rumble (the 40 mm speakers can't reproduce <150 Hz
 * anyway, and on headphones it just muddies) so the mids + stereo breathe. */
#define HP2_R 0.9912f                          /* 2nd-order HP stage, ≈62 Hz    */
static float dc_x1L, dc_y1L, dc_x1R, dc_y1R;
static float hp2_x1L, hp2_y1L, hp2_x1R, hp2_y1R;   /* r19.52: 2nd master-HP stage */
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
    sound_ms=sound_fraction=0; bass_root_midi=drone_root_midi=0; drone_on=false;
    auto_has_onset=false; auto_last_ms=0; s_user_present=false; s_bass_follow=true;
    memory_space=0.5f; memory_echo=0.0f; memory_fx_mode=8;
    memset(tail_valid,0,sizeof tail_valid); memset(source_tail_ms,0,sizeof source_tail_ms);
    pad_init();
    reverb_init();      /* tank still owned by the V2 synth hosts (SYNTH mode) */
    texture_init();
    ambience_init();
    /* r19.41: the legacy master processors (tape/echo/blur/shimmer + master
     * reverb render) are replaced by the ambient effects engine. Init runs
     * here — outside the audio callback — and FAILS CLOSED to dry audio. */
    fx_master_init();
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
    s_gen_suppressed = false;        /* r19.33 player-priority state reset */
    s_ever_active    = false;
    s_last_active_ms = 0;
    s_mel_enabled    = true;
    s_gentle_return  = false;
    composer_init();                 /* r18.96 top-level intent reset   */
    harmony_init();                  /* r19.0 harmonic safety core */
    mel_next_ms = 0; mel_off_ms = 0; mel_sounding = 0;
    mel_phrase_left = 0; mel_last_midi = 0; mel_repeat_run=0; mel_note_count = 0;
    mel_hist_len = mel_cur_len = 0;
    mel_replay = 0; mel_replay_idx = 0;
    mel_dejavu_count = 0;

    /* Master stage: DC-block cleared, moderate default volume (no on-device
     * volume knob bound yet — keeps headphones from being slammed). */
    dc_x1L = dc_y1L = dc_x1R = dc_y1R = 0.0f;
    hp2_x1L = hp2_y1L = hp2_x1R = hp2_y1R = 0.0f;
    master_vol_cur = master_vol_tgt = 0.6f;
    drive_cur = drive_tgt = 0.0f;
    foreground_level=0.0f; bed_gain=1.0f;
    pluck_init();                    /* r18.89 sparkle plucks */
    ember_init();                    /* r19.28 warm subtractive analog voice */
    bowed_init();                    /* r19.47 bowed lyra/Hardanger voice (Open Sea / Fjords) */
    horn_init();                     /* r19.53 alphorn/brass voice (Alps) */
    choir_init();                    /* r19.61 damp organ/choir (Moss)    */
    guembri_init();                  /* r19.61 plucked low lute (Desert)  */
    shape_init();                    /* r19.60 envelope shape (neutral)   */
    s_synth_tgt = 0; s_synth_blend = 0.0f; s_manual_synth = 0;
    s_ambient_gain = s_background_gain = 1.0f;
    s_ambient_tail_frames = s_muted_tail_frames = s_tail_quiet_frames = 0;
    s_native_overlap = false;
    s_note_count = 0;
    memset(s_synth_macros,0,sizeof s_synth_macros);
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
 * swell. This is the legacy manual palette; Generate uses the World
 * descriptor directly and never adds this manual pad/attack combination. */
void engine_set_voice(int voice_idx) {
    if (voice_idx < 0) voice_idx = 0;
    if (voice_idx > 6) voice_idx = 6;    /* r19.61: 5 Choir (Moss), 6 Guembri (Desert) */
    melody_voice = voice_idx;
}

/* Fire the selected melody voice (used by cell presses + sparkles). */
static void melody_strike(float freq_hz, float amp) {
    int voice = melody_voice;
    /* Legacy manual one-shots retain their compatibility gains. The
     * generated sustained path above preserves dynamics down to silence;
     * amplitude floors would defeat its deliberately softer return. */
    if      (voice == 6) guembri_note(freq_hz, dsp_clampf(amp * 2.8f, 0.35f, 0.60f));
    else if (voice == 5) choir_note  (freq_hz, dsp_clampf(amp * 2.6f, 0.32f, 0.55f));
    else if (voice == 4) horn_note (freq_hz, dsp_clampf(amp * 2.6f, 0.34f, 0.58f));
    else if (voice == 3) bowed_note(freq_hz, dsp_clampf(amp * 3.0f, 0.38f, 0.62f));
    else if (voice == 2) ember_note(freq_hz, amp);   /* r19.28 analog */
    else                        pluck_note(freq_hz, amp);   /* 0 Pad / 1 String */
}

/* r19.28 — Landscape "Motif" layer: a warm subtractive ANALOG voice (ember.c:
 * detuned saws + sub, a resonant filter with its own decay envelope, and a
 * delayed vibrato LFO). The FM bell (r19.27.1) read as dead/static; this is a
 * moving, nostalgic tone — independent of the global VOICE menu. */
void engine_motif_strike(float freq_hz, float amp) {
    ember_note(freq_hz, dsp_clampf(amp, 0.0f, 0.30f));
}

/* r19.30 — a bare bell bloom for the HARMONY "extension" role: a sparkle that
 * lets a chord's top ring over the sustained pad body, decaying into the shared
 * hall. No pad, no bass. r19.51: was the FM glass (removed as harsh); now the
 * plucked-string voice, which is gentler and already on the pluck bus. */
void engine_sparkle_strike(float freq_hz, float amp) {
    pluck_note(freq_hz, dsp_clampf(amp, 0.0f, 0.30f));
}

/* Tier A #2: tiny LCG for micro-humanisation. Inside JND so it doesn't drift
 * audibly, but enough that two consecutive identical cell taps aren't
 * bit-identical → no "mechanical" feel on repeats. */
static uint32_t humanize_rng = 0xA5C3F19Du;
static inline float humanize_rand_unit(void){      /* in [-1, +1] */
    humanize_rng = humanize_rng * 1664525u + 1013904223u;
    return ((int32_t)humanize_rng) * (1.0f / 2147483648.0f);
}

int engine_world_source_count(void) {
    /* World-family DSP slots in any role, including releases and prepared
     * onsets. Pad/Ember/bass/drone are added by the Ambient counter below. */
    return bowed_active_count() + horn_active_count() + choir_active_count() +
           guembri_active_count() + pluck_active_count();
}

int engine_ambient_source_count(void) {
    return engine_world_source_count() + pad_active_count() + ember_active_count() +
           bass_active_count() + (drone_active() ? 1 : 0);
}

static bool world_capacity_available(void) {
    return engine_ambient_source_count() < ENGINE_WORLD_SOURCE_LIMIT &&
           !(s_synth_be && s_native_overlap);
}

bool engine_try_world_note_on(uint8_t source, float freq_hz, float amp) {
    if (!gen_on || source >= MAX_SOURCES || !isfinite(freq_hz) ||
        !isfinite(amp) || freq_hz < 20.0f || freq_hz > 8000.0f || amp <= 0.0f ||
        active_freq[source] > 0.0f ||
        !world_capacity_available()) return false;
    int family = worlds_get(s_world_index)->voice;
    if ((family == 0 || family == 1) && freq_hz < PLUCK_MIN_HZ) return false;
    uint32_t previous_rng = humanize_rng;
    float pitch_jitter = humanize_rand_unit() * (0.5f / 1200.0f);
    if (tuning_mode()) pitch_jitter = 0.0f;
    freq_hz *= 1.0f + pitch_jitter;
    amp *= 1.0f + humanize_rand_unit() * 0.003f;
    bool accepted = false;
    switch (worlds_get(s_world_index)->voice) {
    case 0: case 1:
        accepted = pluck_note_on(source, freq_hz, dsp_clampf(amp*2.0f,0.0f,1.0f)); break;
    case 3:
        accepted = bowed_try_note_on(source, freq_hz, dsp_clampf(amp*6.0f,0.0f,0.62f)); break;
    case 4:
        accepted = horn_try_note_on(source, freq_hz, dsp_clampf(amp*5.2f,0.0f,0.58f)); break;
    case 5:
        accepted = choir_try_note_on(source, freq_hz, dsp_clampf(amp*5.2f,0.0f,0.55f)); break;
    case 6:
        /* Compatibility only: the Desert source remains a natural one-shot. */
        accepted = guembri_try_note(freq_hz,dsp_clampf(amp*5.6f,0.35f,0.60f)); break;
    default: break;
    }
    if (!accepted) { humanize_rng = previous_rng; return false; }
    /* Commit only the admitted pitch. No pad, bass refresh, or phantom MIDI. */
    source_tail_ms[source] = tail_horizon();
    active_freq[source] = freq_hz;
    if (s_note_hook) s_note_hook(1, source, freq_hz, amp);
    return true;
}

static void synth_note_hz(float hz, float amp) {
    if (s_synth_be->note_on_hz) s_synth_be->note_on_hz(hz,amp);
    else if (s_synth_be->note_on) {
        int midi=(int)lrintf(69.0f+12.0f*log2f(hz/440.0f));
        s_synth_be->note_on(midi<0 ? 0 : (midi>127 ? 127:midi),amp);
    }
}

void engine_note_on(uint8_t source, float freq_hz, float amp) {
    if (source>=MAX_SOURCES || !isfinite(freq_hz) || !isfinite(amp) ||
        freq_hz<20.0f || freq_hz>8000.0f || amp<=0.0f) return;
    if (gen_on && source == MEL_SRC) {
        (void)engine_try_world_note_on(source, freq_hz, amp);
        return;
    }
    /* ±0.5 cent pitch jitter, ±0.3 % amp jitter. Bass / drone get the same
     * freq downstream (refresh_bass) so the jitter is consistent per press. */
    float pitch_jitter = humanize_rand_unit() * (0.5f / 1200.0f);
    if(tuning_mode()) pitch_jitter=0.0f; /* preserve the requested pure ratio */   /* cents */
    float amp_jitter   = humanize_rand_unit() * 0.003f;
    freq_hz *= 1.0f + pitch_jitter;                    /* 2^(jit) ≈ 1+jit at tiny jit */
    amp     *= 1.0f + amp_jitter;
    remember_source(source);
    source_tail_ms[source]=tail_horizon();
    if (s_note_hook) s_note_hook(1, source, freq_hz, amp);
    /* r19.16 SYNTH mode: played cells drive the V2 sound-core instead of the
     * pad. Generative/drone sources never reach V2 (it's a played mono synth). */
    if (s_synth_tgt > 0 && s_synth_be &&
        (source <= 4 || (source >= 9 && source <= 13))) {
        for (int i=0; i<s_note_count; ++i) if (s_note_stack[i] == source) {
            memmove(&s_note_stack[i], &s_note_stack[i+1], (size_t)(--s_note_count-i)); break;
        }
        s_note_stack[s_note_count++] = source;
        s_note_amp[source] = amp;
        synth_note_hz(freq_hz, dsp_clampf(amp, 0.0f, 1.0f));
        if (source < MAX_SOURCES) active_freq[source] = freq_hz;
        return;
    }
    pad_note_on(source, freq_hz, amp);
    if (source < MAX_SOURCES) active_freq[source] = freq_hz;
    /* Legacy manual cell sources also strike the selected voice in front
     * of their pad. Autonomous source 15 already returned above. */
    if (melody_voice != 0 &&
        (source <= 4 || (source >= 9 && source <= 13)))
    {
        /* Preserve the player's dynamic range; only generated one-shots use
         * the curated minimum level in melody_strike(). */
        if (melody_voice==3) bowed_note_on(source,freq_hz,dsp_clampf(amp*3.0f,0.0f,0.62f));
        else if (melody_voice==4) horn_note_on(source,freq_hz,dsp_clampf(amp*2.6f,0.0f,0.58f));
        else if (melody_voice==5) choir_note_on(source,freq_hz,dsp_clampf(amp*2.6f,0.0f,0.55f));
        else if (melody_voice==1) (void)pluck_note_on(source, freq_hz, dsp_clampf(amp*1.4f,0.0f,0.30f));
        else if (melody_voice==6) guembri_note(freq_hz,dsp_clampf(amp*2.8f,0.0f,0.60f));
        else melody_strike(freq_hz,dsp_clampf(amp*1.4f,0.0f,0.30f));
    }
    refresh_bass();
}
void engine_note_off(uint8_t source) {
    if (source >= MAX_SOURCES) return;
    remember_source(source);
    if (s_note_hook && source < MAX_SOURCES)
        s_note_hook(0, source, active_freq[source], 0.0f);
    if (s_synth_tgt > 0 && s_synth_be &&
        (source <= 4 || (source >= 9 && source <= 13))) {
        int was_top = s_note_count && s_note_stack[s_note_count-1] == source;
        for (int i=0; i<s_note_count; ++i) if (s_note_stack[i] == source) {
            memmove(&s_note_stack[i], &s_note_stack[i+1], (size_t)(--s_note_count-i)); break;
        }
        if (was_top) {
            if (!s_note_count) s_synth_be->note_off();
            else {
                int prev=s_note_stack[s_note_count-1];
                synth_note_hz(active_freq[prev], dsp_clampf(s_note_amp[prev],0.0f,1.0f));
            }
        }
        if (source < MAX_SOURCES) active_freq[source] = 0.0f;
        return;
    }
    bowed_note_off(source); horn_note_off(source); choir_note_off(source);
    pluck_note_off(source);
    pad_note_off(source);
    if (source < MAX_SOURCES) active_freq[source] = 0.0f;
    refresh_bass();
}
void engine_all_off(void) {
    for(int i=0;i<MAX_SOURCES;++i) remember_source(i);
    s_note_count = 0;
    if (s_note_hook) s_note_hook(-1, 0, 0.0f, 0.0f);   /* all-off sentinel */
    if (s_synth_tgt > 0 && s_synth_be) s_synth_be->panic();
    bowed_all_off(); horn_all_off(); choir_all_off(); pluck_all_off();
    pad_all_off();
    memset(active_freq, 0, sizeof active_freq);
    engine_bass_off();
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
    if (!isfinite(v)) return;
    reverb_size = dsp_clampf(v, 0.0f, 1.0f);
    reverb_apply();
}
void engine_set_reverb_damp(float v) {
    if (!isfinite(v)) return;
    reverb_damp = dsp_clampf(v, 0.0f, 1.0f);
    reverb_apply();
}
void engine_set_reverb_drive(float v) {
    if (!isfinite(v)) return;
    reverb_set_drive(dsp_clampf(v, 0.0f, 1.0f)); }
void engine_set_wet_amp(float v)      {
    if (!isfinite(v)) return;
    wet_amp_tgt    = dsp_clampf(v, 0.0f, 1.0f); }
void engine_set_send(float v)         {
    if (!isfinite(v)) return;
    send_amount_tgt = dsp_clampf(v, 0.0f, 1.0f); }
void engine_set_master_volume(float v){
    if (!isfinite(v)) return;
    master_vol_tgt  = dsp_clampf(v, 0.0f, 1.0f); }
void engine_boot_mute(void)           { master_vol_cur  = master_vol_tgt = 0.0f; }
void engine_set_drive(float v)        {
    if (!isfinite(v)) return;
    drive_tgt       = dsp_clampf(v, 0.0f, 1.0f); }
static void synth_macro(int slot, float value) {
    s_synth_macros[slot]=value;
    if(s_synth_be && s_synth_be->set_macro) s_synth_be->set_macro(slot,value);
}
/* r18.90 macro: one emotional dimension, three destinations. hz is the
 * legacy pad-cutoff offset (-600..+800); the hall and the plucks follow.
 * At hz=0 all trims are exactly 0 — the bench-tuned default is untouched. */
void engine_set_brightness(float hz)  {
    if (!isfinite(hz)) return;
    synth_macro(0,hz);
    pad_set_brightness(hz);
    bright_damp_trim = dsp_clampf(-hz * (0.18f / 800.0f), -0.135f, 0.18f);
    reverb_apply();
    pluck_set_damp(dsp_clampf(0.42f - hz * (0.12f / 800.0f), 0.28f, 0.55f));
    /* r19.41: the master-effects tone follows the same macro (hz -600..+800
     * → 0..1, centre unchanged at the world default until the user moves it). */
    fx_master_set_tone((hz + 600.0f) / 1400.0f);
}

/* r19.59 RESONANCE — see docs/SYNTH_IDENTITY.md. The pad bus gets a real
 * resonant ladder; BRIGHT sweeps its cutoff, RESONANCE makes it sing. */
void engine_set_resonance(float amount_0_1) {
    if (!isfinite(amount_0_1)) return;
    synth_macro(1,amount_0_1);
    pad_set_resonance(dsp_clampf(amount_0_1, 0.0f, 1.0f));
}
float engine_resonance(void) { return pad_resonance(); }

/* r19.60 SHAPE — global envelope scaling (see shape.c). */
void engine_set_attack (float v01) {
    if (!isfinite(v01)) return;
    shape_set_attack(v01);
}
void engine_set_release(float v01) {
    if (!isfinite(v01)) return;
    shape_set_release(v01); }

/* r19.60 MOTION — LFO + envelope follower onto the pad-bus filter cutoff. */
void engine_set_sweep (float v01) {
    if (!isfinite(v01)) return;
    synth_macro(2,v01); pad_set_sweep(dsp_clampf(v01,0.0f,1.0f));
}
void engine_set_envmod(float v01) {
    if (!isfinite(v01)) return;
    synth_macro(3,v01); pad_set_envmod(dsp_clampf(v01,0.0f,1.0f)); }
void engine_set_texture(float v)      {
    if (!isfinite(v)) return;
    texture_set_amount(dsp_clampf(v, 0.0f, 1.0f)); }
void engine_set_atmosphere(float v)   {
    if (!isfinite(v)) return;
    v = dsp_clampf(v, 0.0f, 1.0f);
    ambience_set_level(v);               /* per-world atmospheric sound layer */
    fx_master_set_atmosphere(v);         /* r19.41: global spatial send       */
}
/* World change applies BOTH the texture layer (ambience) AND the harmonic
 * identity (key/mode/vibe) so each world sounds musically distinct, not just
 * texturally. The cell taps then play in the world's key/mode (brain), the
 * drone follows the root, and the reverb character shifts via the per-mode/
 * vibe preset table. Values live in worlds.c (audition-derived). */
void engine_set_world(int idx) {
    if (idx < 0) idx = 0;
    if (idx >= WORLD_COUNT) idx = WORLD_COUNT - 1;
    /* Release the old owner even if two Worlds happen to share key/mode.
     * Existing releases retain their slots; no accompanying pad is started. */
    if (gen_on && idx != s_world_index) release_generated();
    s_world_index = idx;
    ambience_set_world(idx);
    /* r18.93: (re)build the PADsynth bed table for the world's timbre
     * profile. Blocking a few ms — worlds change from the UI loop, never
     * the audio IRQ; the pad keeps reading the old table until the final
     * copy, and the world-change re-bloom masks the swap. */
    padsynth_build(idx, 0);
    body_set_world(idx);             /* r18.94: the pluck's resonant material */
    /* r19.47: bowed-voice colour follows the world identity. Only worlds that
     * carry voice==Bowed hear it, but setting it unconditionally keeps the
     * engine self-consistent (Fjords = colour 1 = darker, more sympathetic
     * ring; every other world = colour 0 = the warmer Open-Sea lyra). */
    bowed_set_colour(idx == 2 ? 1 : 0);
    const world_t *w = worlds_get(idx);
    engine_set_key ((int)w->key_midi);   /* brain key + drone root          */
    engine_set_mode((int)w->mode);       /* brain mode + reverb recompute   */
    engine_set_vibe((int)w->vibe);       /* brain vibe + reverb recompute   */
    /* r19.41: master-effects world voicing (Tokyo/Coast/Drive/Hours map 1:1
     * onto the engine worlds). The menu pushes its macro values right after
     * this call, overwriting the user-facing fields — the engine keeps its
     * curated width/tone/level/delay for the world. */
    fx_master_set_world(idx);
}
void engine_set_bass_depth(float v)   {
    if (!isfinite(v)) return;
    bass_set_depth(dsp_clampf(v, 0.0f, 1.0f)); }

/* Perform-macros: combine multiple internal params under one user knob. */
void engine_set_motion(float v) {
    if (!isfinite(v)) return;
    v = dsp_clampf(v, 0.0f, 1.0f);
    /* user 0..1 → pad-LFO depth 0..2 (centre 0.5 = default movement). */
    pad_set_motion(v * 2.0f);
    fx_master_set_motion(v);             /* r19.41: chorus/slow modulation */
}
void engine_set_age(float v) {
    if (!isfinite(v)) return;
    /* r19.41: wow, flutter, bandwidth loss and saturation all
     * live in the master-effects engine now (its `age` parameter) — the
     * legacy tape module left the audio path with the engine swap. */
    fx_master_set_age(dsp_clampf(v, 0.0f, 1.0f));
}

/* SHIMMER macro (menu slot). r19.41: restrained octave regeneration inside
 * the master-effects engine (replaces the legacy shimmer wrap-loop). */
void engine_set_shimmer(float v) {
    if (!isfinite(v)) return;
    fx_master_set_shimmer(dsp_clampf(v, 0.0f, 1.0f));
}

/* Echo macro. r19.41: filtered ping-pong delay inside the master-effects
 * engine (replaces the legacy echo.c tape-style delay). */
void engine_set_echo(float v) {
    if (!isfinite(v)) return;
    memory_echo=dsp_clampf(v,0.0f,1.0f);
    fx_master_set_echo(dsp_clampf(v, 0.0f, 1.0f));
}

/* Blur macro. r19.41: deterministic temporal blur inside the master-effects
 * engine (replaces the legacy blur.c granular cloud). */
void engine_set_blur(float v) {
    if (!isfinite(v)) return;
    fx_master_set_blur(dsp_clampf(v, 0.0f, 1.0f));
}

/* r19.41 FX effect page: 0=Bypass..8=Dream Chain (menu slot; Dream Chain is
 * the boot default, single modes stay selectable for A/B listening). */
void engine_set_fx_mode(int idx) {
    if(idx<0 || idx>=fx_master_mode_count()) return;
    memory_fx_mode=idx; fx_master_set_mode(idx);
}
int  engine_fx_mode(void)              { return fx_master_mode(); }
int  engine_fx_mode_count(void)        { return fx_master_mode_count(); }
const char *engine_fx_mode_name(int i) { return fx_master_mode_name(i); }

/* Step 12b #1 — musical-state setters. Each triggers a preset recompute so
 * the reverb shifts character with the mode/vibe/macro change. The reverb
 * itself smooths internally (~120 ms), so the transition is glide-not-step
 * — matches the "sound darf nicht konkurrieren" rule for global params.
 * Also keeps the harmonic brain's view of mode/vibe in sync so cells played
 * after the change pick up the new harmony. */
void engine_set_mode(int mode_idx) {
    if(mode_idx!=musical_mode) release_generated();
    if (mode_idx < 0)               mode_idx = 0;
    if (mode_idx >= RP_MODE_COUNT)  mode_idx = RP_MODE_COUNT - 1;
    musical_mode = mode_idx;
    brain_set_mode(mode_idx);
    harmony_set_mode(brain_get_key(), mode_idx);   /* r19.0 */
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
    if (!isfinite(v)) return;
    musical_space = dsp_clampf(v, 0.0f, 1.0f);
    memory_space=musical_space;
    recompute_reverb_from_presets();     /* V2 synth hosts still use the tank */
    fx_master_set_space(musical_space);  /* r19.41: master-effects room scale */
}
void engine_set_mood(float v) {
    if (!isfinite(v)) return;
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
    if(tonic_midi!=brain_get_key()) release_generated();
    if(drone_on && tonic_midi!=drone_root_midi) {
        remember_pitch(drone_root_midi,tail_horizon()); remember_pitch(drone_root_midi-12,tail_horizon());
    }
    drone_root_midi=tonic_midi;
    brain_set_key(tonic_midi);
    harmony_set_mode(tonic_midi, musical_mode);    /* r19.0 */
    tuning_set_key(tonic_midi);        /* r19.6 anchor just intonation      */
    drone_set_root_midi(tonic_midi);   /* glides live if the drone is sounding */
}
void engine_set_drone(bool on) {
    if(drone_on && !on) {
        remember_pitch(drone_root_midi,tail_horizon()); remember_pitch(drone_root_midi-12,tail_horizon());
    }
    drone_on=on; drone_enable(on);
}

/* r19.6 — tuning: 0 = equal temperament (reference), 1 = just intonation. */
void engine_set_tuning(int just) {
    just = just ? 1 : 0;
    if (just == tuning_mode()) return;
    /* Preserve source order and velocity. Replaying note_on here would move
     * stack priority and restart attacks. Snapshot the old tuning reference
     * before changing mode; ratios preserve any fine detuning of the note.
     * Ambient sustained-voice retuning is a separate implementation step. */
    float old_hz[MAX_SOURCES];
    int can_retune = s_synth_tgt > 0 && s_synth_be && s_synth_be->retune_hz;
    if (can_retune) for (int i=0; i<s_note_count; ++i) {
        int source=s_note_stack[i];
        old_hz[i]=tuning_hz((float)pitch_of(active_freq[source]));
    }
    tuning_set_mode(just);
    if (can_retune) {
        for (int i=0; i<s_note_count; ++i) {
            int source=s_note_stack[i];
            float hz=tuning_hz((float)pitch_of(active_freq[source]));
            active_freq[source] *= hz / old_hz[i];
        }
        if (s_note_count)
            s_synth_be->retune_hz(active_freq[s_note_stack[s_note_count-1]]);
    }
}

/* PAD_VOICE_MIXES from the webapp: warm / strings / brass. */
void engine_set_pad_voice(int voice_idx) {
    static const float MIX[] = { 0.0f, 0.6f, 1.2f };
    if (voice_idx < 0) voice_idx = 0;
    if (voice_idx > 2) voice_idx = 2;
    pad_set_voice_mix(MIX[voice_idx]);
}

/* Physical product cells are locked during Generate. Explicit low-level
 * callers can still inject presence for musical priority: a finger pauses
 * future events, a latched pitch only participates in the collision memory. */
void engine_set_user_presence(bool any_key_down) { s_user_present = any_key_down; }

/* r19.33 — player takes priority: World events hold
 * off while the user plays AND for GEN_RETURN_MS after the last release, then
 * return gently. State lives up top so engine_init can reset it. */
int engine_generative_suppressed(void) { return s_gen_suppressed ? 1 : 0; }

/* Control-rate World scheduler. The first decision follows an explicit
 * Generate/field/World entry immediately; natural source attacks provide
 * the onset. Later decisions follow World phrasing, composer density and
 * pitch safety. Harmony changes create no additional audio sources. */

uint32_t engine_gen_seed(void)          { return gen_tick_rng; }
void     engine_set_gen_seed(uint32_t s) { gen_tick_rng = s ? s : 0x5EEDBA55u; }

static float gen_rand01(void) {
    gen_tick_rng = gen_tick_rng * 1664525u + 1013904223u;
    return (float)(gen_tick_rng >> 8) / 16777216.0f;
}

static void release_generated(void) {
    if(active_freq[MEL_SRC]>0) engine_note_off(MEL_SRC);
    mel_sounding=0; gen_timing_valid=false;
    mel_phrase_left=mel_cur_len=mel_hist_len=0; mel_replay=0; mel_last_midi=mel_repeat_run=0;
}
void engine_set_generative(bool on,int program) {
    generative_set_program(program);
    if (on == gen_on) return;
    if (on) {
        /* Listening owns the World engine; remember the manual Character.
         * Release old sources and reuse the existing bounded crossfade. */
        s_ambient_tail_frames = 0;
        gen_on = true; /* suppress bass-follow throughout the release handover */
        activate_synth(0, true);
        engine_set_drone(false);
        s_user_present = s_ever_active = s_gen_suppressed = false;
        s_gentle_return = false;
    } else {
        release_generated(); /* still listening: releases cannot restart bass */
        engine_bass_off();
        gen_on = false;
        /* Keep released World sources audible while manual notes immediately
         * reach their Character. Do not fade these sources with the core. */
        if (s_manual_synth > 0) s_ambient_tail_frames = AMBIENT_TAIL_MAX_FRAMES;
        activate_synth(s_manual_synth, false);
    }
}

/* r19.24 — the five cells as composer intents (see engine.h). Deliberately
 * a MAPPING of gestures to the existing composer states, not a claim that
 * harmony pitch-picks encode tension: the directional FEEL comes from the
 * composer table (density/depth/rest), and harmony_advance() moves the
 * actual voicing so the answer is heard. */
static const composer_state_t CELL_INTENT[5] = {
    COMPOSER_RETURN,   /* 0 Home / Resolve — warmth back            */
    COMPOSER_OPEN,     /* 1 Lift           — more light, denser      */
    COMPOSER_DEEP,     /* 2 Dark           — floor rises, melody recedes */
    COMPOSER_CALM,     /* 3 Open           — spacious resting rate    */
    COMPOSER_EMPTY,    /* 4 Tension        — the held breath          */
};

void engine_generative_nudge(int cell, uint32_t now_ms) {
    if (!gen_on || cell < 0 || cell > 4) return;
    composer_nudge(CELL_INTENT[cell], now_ms);
    harmony_advance();               /* move the harmonic state NOW */
}

void engine_generative_new_field(uint32_t seed) {
    release_generated();
    engine_set_gen_seed(seed);
    composer_reseed(seed ^ 0x9E3779B9u);
    harmony_reseed(seed ^ 0x85EBCA6Bu);
    /* restart the field at state 0 in the SAME pitch world (key/mode) */
    harmony_set_mode(brain_get_key(), musical_mode);
    composer_init();                 /* fresh intent clock; reseed below   */
    composer_reseed(seed ^ 0x9E3779B9u);
    gen_timing_valid = false;
    s_gentle_return = false;
}

int engine_generative_advance(void) {
    if (!gen_on || s_synth_tgt>0) return -1;
    if (s_user_present || s_gen_suppressed) return -1;   /* r19.33: player + return-delay */
    harmony_advance();
    return harmony_state_index() + 1;
}

int engine_generative_last_melody_midi(void) { return mel_last_midi; }
int engine_generative_melody_count(void)     { return mel_note_count; }
int engine_generative_dejavu_count(void)     { return mel_dejavu_count; }

void engine_generative_tick(uint32_t now_ms) {
    /* Control-only tests may advance time without rendering every sample. */
    if((int32_t)(now_ms-sound_ms)>0) { sound_ms=now_ms; sound_fraction=0; }
    /* Remember physical playing even in a Character or with Generate off.
     * Returning to Ambient must respect the same quiet return interval. */
    if (s_user_present) { s_last_active_ms = now_ms; s_ever_active = true; }
    if(!gen_on || s_synth_tgt>0) return;
    const world_phrase_t *phrase = worlds_phrase(s_world_index);
    int sounding[128]; int occupied=engine_sounding_notes(sounding,128);
    composer_listen((float)occupied/12.0f,s_user_present); composer_tick(now_ms);
    /* r19.33 — player-priority "listening": suppressed while a key is down AND
     * for GEN_RETURN_MS after the last release, so the machine steps back and
     * lets the player breathe, then schedules a softer World return 1.5–4 s
     * later. No underlying tonal bed is sustained during that pause. */
    bool suppressed = s_user_present ||
        (s_ever_active && (uint32_t)(now_ms - s_last_active_ms) < GEN_RETURN_MS);
    if (suppressed != s_gen_suppressed) {
        s_gen_suppressed = suppressed;
        if (suppressed) {
            if (mel_sounding) {             /* hush the auto melody as the player takes over */
                engine_note_off((uint8_t)MEL_SRC);
                mel_sounding = 0;
            }
        } else {
            s_gentle_return = true;         /* the piece comes back softly, not with a ding */
        }
    }
    if (suppressed) {
        gen_timing_valid = false;
        return;
    }

    /* Harmony continues while an entry waits for outgoing sources. */
    harmony_tick(now_ms);
    bool opening = false;
    if (!gen_timing_valid) {
        /* A busy entry is not a rejected musical event: keep its first
         * decision pending without drawing pitches/RNG or starting a retry
         * timer. The next control tick can enter when audio frees capacity. */
        if (!s_gentle_return && (!world_capacity_available() || !auto_ready())) return;
        gen_timing_valid = true;
        opening = !s_gentle_return;
        mel_next_ms = opening ? now_ms :
            now_ms + 1500u + (uint32_t)(gen_rand01() * 2500.0f);
        if (mel_sounding && (int32_t)(now_ms - mel_off_ms) >= 0) {
            engine_note_off((uint8_t)MEL_SRC);   /* stale note from before
                                                  * a long user hold      */
            mel_sounding = 0;
        }
    }

    /* --- r19.0 LONG MELODY VOICE -----------------------------------------
     * One voice. World-specific long tones and real silences (stretched by
     * the composer's rest_add), phrases of 2-5 notes with a longer breath
     * after each phrase. EVERY tone runs the full safety chain in
     * harmony_melody_next: pitch world → register mask → interval table →
     * collision filter against everything sustaining → next-best
     * fallback. Probability is the LAST stage, not the first. The onset
     * and release belong to the selected World family. Plucked sources
     * decay naturally; there is no sustaining pad behind them. */
    if (!s_mel_enabled) {                /* r19.34: melody layer off */
        if (mel_sounding) { engine_note_off((uint8_t)MEL_SRC); mel_sounding = 0; }
    } else {
    if (mel_sounding && (int32_t)(now_ms - mel_off_ms) >= 0) {
        engine_note_off((uint8_t)MEL_SRC);
        mel_sounding = 0;
        float rest = phrase->rest_min + gen_rand01() * (phrase->rest_max - phrase->rest_min);
        rest *= 1.0f + composer_params()->rest_add * 2.0f;    /* composer */
        if (mel_phrase_left <= 0)
            rest += 3.0f + gen_rand01() * 5.0f;               /* breath  */
        mel_next_ms = now_ms + (uint32_t)(rest * 1000.0f);
    }
    if (!mel_sounding && auto_ready() && (int32_t)(now_ms - mel_next_ms) >= 0) {
        if (mel_phrase_left <= 0) {
            bool varied=false;
            for(int i=1;i<mel_cur_len;++i) if(mel_cur[i]!=mel_cur[0]) varied=true;
            if(mel_cur_len>=2 && varied) { /* no stuck-note phrase in memory */
                for (int i = 0; i < mel_cur_len; ++i) mel_hist[i] = mel_cur[i];
                mel_hist_len = mel_cur_len;
            }
            mel_cur_len = 0;
            mel_phrase_left = 2 + (int)(gen_rand01() * 4.0f);   /* 2..5  */
            mel_replay = (mel_hist_len >= 2 && gen_rand01() < 0.35f);
            mel_replay_idx = 0;
            /* mel_last_midi is NOT reset: the new phrase steps off from
             * where the old one ended, so voice-leading survives the
             * breath (a reset caused over-octave leaps between phrases —
             * caught by the grammar audit). */
        }
        /* Entry gets one immediate proposal, never a forced admission.
         * All later proposals retain density, rest and collision rules. */
        if (opening || gen_rand01() < dsp_clampf(0.01f * phrase->density_pct * composer_params()->mel_density,
                                      0.05f, 0.95f)) {
            /* everything currently sustaining, for the collision filter */
            int sus[128]; int nsus=engine_sounding_notes(sus,128);

            int replay_before = mel_replay_idx;
            int tone = -1;
            if (mel_replay && mel_replay_idx < mel_hist_len) {
                int want = mel_hist[mel_replay_idx++];
                if (harmony_in_world(want) &&
                    (!mel_last_midi || (want-mel_last_midi<=12 && mel_last_midi-want<=12)) &&
                    harmony_collision_ok(want, sus, nsus))
                    tone = want;                 /* the motif survives   */
            }
            if (tone < 0) {
                /* r19.34: the first note coming back after the player-priority
                 * silence stays low (no high_p reach) so it re-enters calmly. */
                float hp = s_gentle_return ? 0.0f
                                           : (0.05f + composer_params()->high_p);
                tone = harmony_melody_next(mel_last_midi, sus, nsus, hp);
            }
            if(tone==mel_last_midi && mel_repeat_run>=2) tone=harmony_melody_move(mel_last_midi,sus,nsus);
            if (tone > 0) {
                float hz  = tuning_hz((float)tone);
                float amp = 0.062f + gen_rand01() * 0.014f;
                float level = s_gentle_return ? amp * 0.5f : amp;
                if (!engine_try_world_note_on(MEL_SRC, hz, level)) {
                    /* Capacity is a musical rest, not a fast retry or a note.
                     * Do not consume the remembered motif on a failed start. */
                    mel_replay_idx = replay_before;
                    mel_next_ms = now_ms + 2000u;
                } else {
                    s_gentle_return = false;
                    if (mel_replay && mel_cur_len == 0) ++mel_dejavu_count;
                    auto_onset(); mel_repeat_run=tone==mel_last_midi ? mel_repeat_run+1:1;
                    mel_sounding=1; mel_last_midi=tone;
                    ++mel_note_count;
                    if (mel_cur_len < MEL_PHRASE_MAX) mel_cur[mel_cur_len++] = tone;
                    --mel_phrase_left;
                    mel_off_ms = now_ms + (uint32_t)(1000.0f * (phrase->note_min +
                                 gen_rand01() * (phrase->note_max - phrase->note_min)));
                } /* admitted note */
            } else {
                /* nothing SAFE right now — silence is the correct note */
                mel_next_ms = now_ms + 2000u +
                              (uint32_t)(gen_rand01() * 3000.0f);
            }
        } else {
            mel_next_ms = now_ms + 2000u + (uint32_t)(gen_rand01() * 4000.0f);
        }
    }
    }   /* r19.34: end of s_mel_enabled branch */


}

static void render_ambient(int frames, bool retiring) {
    /* Reused for the background transition and bass; no additional buffer. */
    static float subL[BLOCK], subR[BLOCK], subJL[BLOCK], subJR[BLOCK];
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
    /* The foreground gently makes room in the pad (maximum 2.2 dB).
     * Measured character-voice tails drive the release; no master pumping. */
    float bed_target=1.0f-0.22f*dsp_clampf(foreground_level*28.0f,0.0f,1.0f);
    for(int n=0;n<frames;++n) {
        float k=bed_target<bed_gain ? 0.0002834f : 0.00001890f;
        bed_gain+=k*(bed_target-bed_gain);
        dryL[n]*=bed_gain; dryR[n]*=bed_gain; sendL[n]*=bed_gain; sendR[n]*=bed_gain;
    }
    if (!retiring && s_background_gain >= 1.0f) {
        texture_render_mix(dryL, dryR, sendL, sendR, frames, TEXTURE_SEND);
        ambience_render_mix(dryL, dryR, sendL, sendR, frames, AMBIENCE_SEND);
    } else if (!retiring || s_background_gain > 0.0f) {
        memset(subL, 0, sizeof(float) * frames); memset(subR, 0, sizeof(float) * frames);
        memset(subJL, 0, sizeof(float) * frames); memset(subJR, 0, sizeof(float) * frames);
        texture_render_mix(subL, subR, subJL, subJR, frames, TEXTURE_SEND);
        ambience_render_mix(subL, subR, subJL, subJR, frames, AMBIENCE_SEND);
        for (int n=0;n<frames;++n) {
            /* Two-second fade on both buses; a quick re-entry reverses it
             * from the current gain without touching the user's settings. */
            s_background_gain = dsp_clampf(s_background_gain +
                (retiring ? -1.0f : 1.0f) / (2.0f * DSP_SAMPLE_RATE_HZ), 0.0f, 1.0f);
            dryL[n]+=subL[n]*s_background_gain; dryR[n]+=subR[n]*s_background_gain;
            sendL[n]+=subJL[n]*s_background_gain; sendR[n]+=subJR[n]*s_background_gain;
        }
    }
    /* r19.52: the AUDIT found the low end (bass + drone) was ~90 % of the mix
     * energy — masking every mid voice and pulling the stereo image to mono.
     * The real culprit is the BASS (spectral centroid ~25 Hz, deep sub the
     * 40 mm speakers can't even reproduce): render it onto its own bus and trim
     * it hard so it supports instead of dominates. The DRONE is left at full
     * level — it is a musical ~110 Hz voice the player deliberately holds, not
     * mud. The master high-pass (HP2 above) cleans both. */
    {
        memset(subL, 0, sizeof(float) * (size_t)frames);
        memset(subR, 0, sizeof(float) * (size_t)frames);
        memset(subJL, 0, sizeof(float) * (size_t)frames);
        memset(subJR, 0, sizeof(float) * (size_t)frames);
        bass_render_mix(subL, subR, subJL, subJR, frames);
        const float BASS_TRIM = 0.5f;
        for (int n = 0; n < frames; ++n) {
            dryL[n]  += subL[n]  * BASS_TRIM;  dryR[n]  += subR[n]  * BASS_TRIM;
            sendL[n] += subJL[n] * BASS_TRIM;  sendR[n] += subJR[n] * BASS_TRIM;
        }
    }
    drone_render_mix(dryL, dryR, sendL, sendR, frames);
    memcpy(foreground_beforeL,dryL,sizeof(float)*frames);
    memcpy(foreground_beforeR,dryR,sizeof(float)*frames);
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
        body_process(plkL, plkR, frames);
        for (int n = 0; n < frames; ++n) {
            dryL[n]  += plkL[n];
            dryR[n]  += plkR[n];
            sendL[n] += plkL[n] * 0.5f;            /* pluck VERB_SEND kept */
            sendR[n] += plkR[n] * 0.5f;
        }
    }
    /* r19.28: the warm analog voice runs its OWN clean bus (its filter is the
     * character) straight into dry + hall send — no modal body coloring. */
    ember_render_mix(dryL, dryR, sendL, sendR, frames);

    /* r19.47: the bowed lyra/Hardanger voice carries its own resonant wood
     * body + sympathetic resonators, so it also bypasses the modal body and
     * mixes straight to dry + hall send. Idle voices cost nothing (the inner
     * loop early-outs), so it runs unconditionally regardless of the world. */
    bowed_render_mix(dryL, dryR, sendL, sendR, frames, 0.5f);

    /* Rounded Alps source (legacy Horn ID), with its own body filter.
     * No modal-body colour; idle voices early-out. */
    horn_render_mix(dryL, dryR, sendL, sendR, frames, 0.5f);

    /* r19.61: Moss-Chor und Desert-Guembri — eigene Koerper, daher wie
     * bowed/horn direkt in dry + Hall-Send, ohne Modal-Body. */
    choir_render_mix  (dryL, dryR, sendL, sendR, frames, 0.55f);
    guembri_render_mix(dryL, dryR, sendL, sendR, frames, 0.35f);
    float energy=0.0f;
    for(int n=0;n<frames;++n)
        energy+=0.5f*(fabsf(dryL[n]-foreground_beforeL[n])+fabsf(dryR[n]-foreground_beforeR[n]));
    foreground_level=energy/(float)frames;

}

static void render_master(int16_t *buf, int frames) {
    /* Per-block drive coefficients, sample-rate volume AFTER every effect.
     * A mute therefore also silences stored tails and generated tape noise. */
    drive_cur += (1.0f - expf(-(float)frames / (0.12f * DSP_SAMPLE_RATE_HZ))) *
                 (drive_tgt - drive_cur);
    /* r18.89 master drive (per block: curve params + makeup are constant
     * inside a 5.8 ms block; the amount itself is smoothed above). */
    const bool  drv_on   = drive_cur > 1.0e-3f;
    const float drv_g    = 1.0f + 3.5f * drive_cur;          /* 1 .. 4.5  */
    const float drv_bias = 0.28f * drive_cur;                /* asymmetry */
    const float drv_mk   = dsp_drive_makeup(drv_g, drv_bias);
    const float drv_mix  = drive_cur < 0.25f ? drive_cur * 4.0f : 1.0f;

    for (int n = 0; n < frames; ++n) {
        float L = dryL[n];
        float R = dryR[n];

        if (drv_on) {
            /* dry/wet fade over the first quarter of the knob so tiny
             * settings colour instead of switch. */
            float dL = dsp_drive_shape(L, drv_g, drv_bias) * drv_mk;
            float dR = dsp_drive_shape(R, drv_g, drv_bias) * drv_mk;
            L += drv_mix * (dL - L);
            R += drv_mix * (dR - R);
        }

        dryL[n] = L; dryR[n] = R;
    }
    fx_master_process_buses(dryL, dryR, sendL, sendR, frames);
    for (int n = 0; n < frames; ++n) {
        float L = dryL[n], R = dryR[n];
        /* one-pole DC blocker per channel: y = x - x1 + R·y1 (also eats the
         * small DC offset the drive bias introduces) */
        float yL = L - dc_x1L + DC_R * dc_y1L; dc_x1L = L; dc_y1L = yL;
        float yR = R - dc_x1R + DC_R * dc_y1R; dc_x1R = R; dc_y1R = yR;
        /* r19.52: 2nd high-pass stage (~62 Hz) — kills the sub-bass mud that
         * masked the mids + collapsed the stereo (see AUDIT). */
        float zL = yL - hp2_x1L + HP2_R * hp2_y1L; hp2_x1L = yL; hp2_y1L = zL;
        float zR = yR - hp2_x1R + HP2_R * hp2_y1R; hp2_x1R = yR; hp2_y1R = zR;

        master_vol_cur += 0.000188947f * (master_vol_tgt - master_vol_cur);
        dryL[n] = zL * master_vol_cur; dryR[n] = zR * master_vol_cur;
    }
    for (int n = 0; n < frames; ++n) {
        /* soft_limit is now a safety net for the rare residual peak above
         * 0.75 — saturation usually already keeps us in range. */
        float yL = soft_limit(dryL[n]);
        float yR = soft_limit(dryR[n]);
        buf[n * 2 + 0] = (int16_t)(yL * 32767.0f);
        buf[n * 2 + 1] = (int16_t)(yR * 32767.0f);
    }
}

/* ===== r19.16 — SYNTH mode: V2 sound-cores behind the ambient engine =====
 *
 * mode 0 = the ambient engine (identity, default). 1..SYNTH-count = a V2
 * sound-core rendered by the backend (synth_host, registered by the product
 * main — the engine keeps NO link dependency on src/v2, so every existing
 * host test still links). Switching crossfades ~15 ms with complementary gains between
 * the two rendered paths (REALTIME_AUDIO_RULES §4: algorithm changes need a
 * crossfade, never a hard swap). Generate exit additionally drains released
 * Ambient sources without fading them with the core. Both paths run during
 * this bounded overlap; after it only the selected path runs. Shared FX
 * continue throughout. */
#define SYNTH_XFADE_SAMPLES 662            /* ~15 ms at 44.1 kHz */

static int16_t s_v2buf[BLOCK * 2]; /* legacy backend compatibility */
static float s_coreL[BLOCK], s_coreR[BLOCK], s_coreSL[BLOCK], s_coreSR[BLOCK];

void engine_set_synth_backend(const engine_synth_backend_t *be) {
    s_synth_be = be;
    if (be && be->set_macro) for (int i=0;i<4;++i) be->set_macro(i,s_synth_macros[i]);
}
void engine_set_synth_param(int slot, float value) {
    if (!isfinite(value)) return;
    if (s_manual_synth > 0 && s_synth_be && s_synth_be->set_param && slot >= 0 && slot < 6) {
        s_synth_be->select(s_manual_synth - 1);
        s_synth_be->set_param(slot, dsp_clampf(value, 0.0f, 1.0f));
    }
}
void engine_set_synth(int idx) {
    if (idx < 0 || idx > 6 || (idx > 0 && !s_synth_be)) return;
    s_manual_synth = idx;
    if (!gen_on) activate_synth(idx, false);
}
static void activate_synth(int idx, bool force) {
    if (idx == 0) s_ambient_tail_frames = s_muted_tail_frames = 0;
    if (!force && idx == s_synth_tgt) return;
    /* A manual Ambient→native switch keeps its existing 15 ms crossfade.
     * Continue advancing the released, muted pool afterwards; otherwise old
     * envelopes freeze and reappear on the next Ambient/Generate visit. */
    if (idx > 0 && s_synth_tgt == 0 && !s_ambient_tail_frames)
        s_muted_tail_frames = AMBIENT_TAIL_MAX_FRAMES;
    /* Release, don't panic: old core/ambient can decay during the crossfade. */
    release_generated(); s_note_count=0;
    for(int i=0;i<MAX_SOURCES;++i) remember_source(i);
    memset(active_freq,0,sizeof active_freq);
    bowed_all_off(); horn_all_off(); choir_all_off(); pluck_all_off(); pad_all_off(); engine_bass_off();
    if (s_synth_tgt > 0 && s_synth_be->note_off) s_synth_be->note_off();
    if (idx > 0) s_synth_be->select(idx - 1);
    s_synth_tgt = idx;
}
int engine_synth(void) { return s_synth_tgt; }
bool engine_listening_tail_active(void) { return s_ambient_tail_frames > 0; }

static bool ambient_sources_active(void) {
    return pad_active_count() || bowed_active_count() || horn_active_count() ||
           choir_active_count() || guembri_active_count() || pluck_active_count() ||
           ember_active_count() || bass_active() || drone_active();
}

void engine_render(int16_t *buf, int frames) {
    if (frames <= 0) return;
    if (frames > BLOCK) frames = BLOCK;
    int tgt = s_synth_tgt;
    bool retiring = tgt > 0 && s_ambient_tail_frames > 0;
    bool muted_draining = tgt > 0 && !retiring && s_muted_tail_frames > 0;
    bool need_v1 = tgt == 0 || s_ambient_gain > 0.0f || retiring || muted_draining;
    bool need_v2 = tgt > 0 || s_synth_blend > 0.0f;
    if (need_v1) render_ambient(frames, retiring || tgt > 0);
    else {
        memset(dryL, 0, sizeof(float)*frames); memset(dryR, 0, sizeof(float)*frames);
        memset(sendL, 0, sizeof(float)*frames); memset(sendR, 0, sizeof(float)*frames);
    }
    float ambient_target = tgt == 0 || retiring ? 1.0f : 0.0f;
    if (retiring || muted_draining) {
        /* Envelope idleness alone misses the modal body's residual ringing.
         * Observe dry AND send before adding the new core; its notes cannot
         * keep this drain alive. 50 ms of quiet avoids a zero-crossing exit. */
        bool quiet = !ambient_sources_active() && s_background_gain == 0.0f;
        if (quiet) for (int n=0;n<frames;++n) {
            if (fabsf(dryL[n])+fabsf(dryR[n])+fabsf(sendL[n])+fabsf(sendR[n]) > 0.000001f) {
                quiet = false; break;
            }
        }
        s_tail_quiet_frames = quiet ? s_tail_quiet_frames + (uint32_t)frames : 0;
        uint32_t left = retiring ? s_ambient_tail_frames : s_muted_tail_frames;
        /* Fault containment for a future source that fails to retire. Current
         * maximum Shape releases complete before 64 s. Fade the last second. */
        if (retiring && left < DSP_SAMPLE_RATE_HZ) ambient_target = (float)left / DSP_SAMPLE_RATE_HZ;
        uint32_t remaining = left > (uint32_t)frames ? left - (uint32_t)frames : 0;
        if (s_tail_quiet_frames >= DSP_SAMPLE_RATE_HZ / 20u) remaining = 0;
        if (retiring) s_ambient_tail_frames = remaining;
        else s_muted_tail_frames = remaining;
    } else s_tail_quiet_frames = 0;
    if (muted_draining && s_ambient_gain == 0.0f) {
        /* Advance sources without sending any of this muted pool to shared FX.
         * Clear explicitly rather than multiplying an invalid sample by zero. */
        memset(dryL, 0, sizeof(float)*frames); memset(dryR, 0, sizeof(float)*frames);
        memset(sendL, 0, sizeof(float)*frames); memset(sendR, 0, sizeof(float)*frames);
    }
    if (need_v2 && s_synth_be) {
        memset(s_coreL, 0, sizeof(float)*frames); memset(s_coreR, 0, sizeof(float)*frames);
        memset(s_coreSL, 0, sizeof(float)*frames); memset(s_coreSR, 0, sizeof(float)*frames);
        if (s_synth_be->render_mix) {
            s_synth_be->render_mix(s_coreL, s_coreR, s_coreSL, s_coreSR, frames);
        } else if (s_synth_be->render) {
            s_synth_be->render(s_v2buf, frames);
            for (int n=0; n<frames; ++n) {
                s_coreL[n]=s_v2buf[2*n]/32768.0f; s_coreR[n]=s_v2buf[2*n+1]/32768.0f;
            }
        }
        for (int n=0; n<frames; ++n) {
            s_synth_blend = dsp_clampf(s_synth_blend + (tgt > 0 ? 1.0f : -1.0f) /
                                       SYNTH_XFADE_SAMPLES, 0.0f, 1.0f);
            float step=1.0f/SYNTH_XFADE_SAMPLES;
            if (s_ambient_gain < ambient_target) s_ambient_gain=fminf(ambient_target,s_ambient_gain+step);
            else if (s_ambient_gain > ambient_target) s_ambient_gain=fmaxf(ambient_target,s_ambient_gain-step);
            float t=s_synth_blend, a=s_ambient_gain;
            dryL[n]=a*dryL[n]+t*s_coreL[n]; dryR[n]=a*dryR[n]+t*s_coreR[n];
            sendL[n]=a*sendL[n]+t*s_coreSL[n]; sendR[n]=a*sendR[n]+t*s_coreSR[n];
        }
    } else if (s_ambient_gain < 1.0f) {
        /* A reversal can finish the native fade before Ambient reaches unity. */
        for (int n=0;n<frames;++n) {
            s_ambient_gain=fminf(1.0f,s_ambient_gain+1.0f/SYNTH_XFADE_SAMPLES);
            dryL[n]*=s_ambient_gain; dryR[n]*=s_ambient_gain;
            sendL[n]*=s_ambient_gain; sendR[n]*=s_ambient_gain;
        }
    }
    s_native_overlap = s_synth_be && s_synth_blend > 0.0f;
    render_master(buf,frames);
    sound_fraction+=(uint32_t)frames*1000u;
    sound_ms+=sound_fraction/DSP_SAMPLE_RATE_HZ; sound_fraction%=DSP_SAMPLE_RATE_HZ;
}

int engine_active_voices(void) { return pad_active_count(); }
