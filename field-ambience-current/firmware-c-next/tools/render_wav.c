/*
 * Offline render harness — renders the real engine to a stereo 16-bit WAV on
 * the host, with NO Pico SDK and NO hardware. This is the reference-audio
 * loop (the "export reference audio" step): listen to the WAV on a computer
 * and A/B it against field_ambience_webapp.html. If the WAV matches the
 * webapp, any remaining difference on the device is the hardware chain
 * (DAC / amp / speaker / ground / supply), not the DSP code.
 *
 * It drives the engine exactly like the firmware would (engine_note_on/off,
 * engine_set_*, engine_render) so the output is bit-for-bit what the Pico
 * computes before the I²S/DAC.
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_wav.c \
 *      src/dsp.c src/pad.c src/texture.c src/bass.c src/drone.c \
 *      src/reverb.c src/reverb_presets.c src/brain.c src/generative.c \
 *      src/engine.c -lm -o /tmp/render_wav
 *   /tmp/render_wav field_ambience.wav
 */

#include "engine.h"
#include "brain.h"
#include "dsp.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SR     44100
#define BLOCK  256

static void put_u32(FILE *f, uint32_t v) { fputc(v & 0xff, f); fputc((v >> 8) & 0xff, f); fputc((v >> 16) & 0xff, f); fputc((v >> 24) & 0xff, f); }
static void put_u16(FILE *f, uint16_t v) { fputc(v & 0xff, f); fputc((v >> 8) & 0xff, f); }

static void write_wav_header(FILE *f, uint32_t nframes) {
    uint32_t data_bytes = nframes * 2u * 2u;     /* stereo, 16-bit */
    fwrite("RIFF", 1, 4, f); put_u32(f, 36 + data_bytes);
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); put_u32(f, 16);
    put_u16(f, 1);                               /* PCM */
    put_u16(f, 2);                               /* channels */
    put_u32(f, SR);
    put_u32(f, SR * 2u * 2u);                    /* byte rate */
    put_u16(f, 4);                               /* block align */
    put_u16(f, 16);                              /* bits */
    fwrite("data", 1, 4, f); put_u32(f, data_bytes);
}

/* ---------------------------------------------------------------------------
 * A long-form, human-feel performance — ~8 minutes, six movements that travel
 * the entire instrument: cells, drone pedal, all four pad voices, every mode
 * + vibe, space/mood macros, bass-depth opens, generative bar advance, key
 * change, dynamic texture bed. No hammering, generous overlap, breath between
 * gestures, never more than three cells held at once (real polyphony budget).
 * Cell amplitude is the firmware's real CELL_VOICE_AMP (0.12).
 *
 * Movements (cue times in seconds, total = TOTAL_SECONDS):
 *   I.   0–80    Awakening      C ionian / warm pad — bed opens, first taps
 *   II.  80–180  Pedal          drone on, low triads, voice→strings, deeper
 *   III. 180–280 Cathedral      bigger space + mood up, generative bar
 *   IV.  280–380 Shift          key→G, mode→dorian, bright vibe, brass voice
 *   V.   380–460 Aeolian        mode→aeolian, mood down, sub-bass, drone glide
 *   VI.  460–500 Dissolve       everything releases into the long reverb tail
 * --------------------------------------------------------------------------- */

#define CELL_AMP   0.12f

typedef struct { float t; int kind; int a; float b; } evt_t;
enum {
    EV_CELL_ON, EV_CELL_OFF,
    EV_TEXTURE, EV_BASS, EV_SPACE, EV_MOOD,
    EV_DRONE, EV_VOICE, EV_KEY, EV_MODE, EV_VIBE,
    EV_BRIGHTNESS,                                       /* audio pad cutoff (Hz) */
    EV_GEN_ON, EV_GEN_OFF, EV_GEN_STEP,
    EV_ALL_OFF
};

/* Quick aliases for readability — cells are 0..6 = degrees I..VII. */
#define C_I    0
#define C_II   1
#define C_III  2
#define C_IV   3
#define C_V    4
#define C_VI   5
#define C_VII  6

static const evt_t TIMELINE[] = {
    /* ===== I. Awakening (0–80 s) — gentle opening, C ionian, warm pad ===== */
    { 0.0f,   EV_TEXTURE,    8, 0 },        /* very soft bed fade-in */
    { 0.0f,   EV_BASS,       0, 0 },
    { 0.0f,   EV_SPACE,     45, 0 },
    { 0.0f,   EV_MOOD,      35, 0 },
    { 6.0f,   EV_TEXTURE,   18, 0 },        /* bed settles in */
    { 9.0f,   EV_CELL_ON,  C_I,  0 },       /* first tap — let it bloom 0.8 s, ring 3 s */
    { 16.0f,  EV_CELL_ON,  C_V,  0 },       /* second tap overlaps */
    { 22.0f,  EV_CELL_OFF, C_I,  0 },
    { 25.0f,  EV_CELL_ON,  C_IV, 0 },
    { 29.0f,  EV_CELL_OFF, C_V,  0 },
    { 33.0f,  EV_CELL_ON,  C_VI, 0 },       /* vi — minor colour shift */
    { 36.0f,  EV_CELL_OFF, C_IV, 0 },
    { 41.0f,  EV_CELL_OFF, C_VI, 0 },
    { 45.0f,  EV_TEXTURE,  24, 0 },         /* bed opens a touch */
    { 47.0f,  EV_CELL_ON,  C_I,  0 },       /* return home */
    { 52.0f,  EV_CELL_ON,  C_III,0 },       /* third on top */
    { 58.0f,  EV_CELL_OFF, C_I,  0 },
    { 61.0f,  EV_CELL_OFF, C_III,0 },
    { 64.0f,  EV_BRIGHTNESS, 0,  -120.0f }, /* darken pad slightly */
    { 70.0f,  EV_SPACE,    55, 0 },         /* longer hall */

    /* ===== II. Pedal (80–180 s) — drone enters, low triads, strings voice ===== */
    { 82.0f,  EV_DRONE,     1, 0 },         /* root drone blooms in (C) */
    { 90.0f,  EV_CELL_ON,  C_I,  0 },
    { 93.0f,  EV_CELL_ON,  C_III,0 },
    { 96.0f,  EV_CELL_ON,  C_V,  0 },       /* held I-III-V triad over the pedal */
    { 105.0f, EV_BASS,     35, 0 },         /* let the low end in */
    { 108.0f, EV_CELL_OFF, C_III,0 },       /* release the third */
    { 114.0f, EV_VOICE,     1, 0 },         /* warm → strings (live crossfade) */
    { 118.0f, EV_CELL_OFF, C_V,  0 },
    { 122.0f, EV_CELL_ON,  C_IV, 0 },
    { 128.0f, EV_CELL_OFF, C_I,  0 },
    { 132.0f, EV_CELL_ON,  C_II, 0 },       /* ii minor against the pedal */
    { 138.0f, EV_BRIGHTNESS,0,  +180.0f },  /* lift the pad cutoff back up */
    { 140.0f, EV_CELL_OFF, C_IV, 0 },
    { 144.0f, EV_CELL_OFF, C_II, 0 },
    { 148.0f, EV_CELL_ON,  C_VI, 0 },
    { 153.0f, EV_CELL_ON,  C_V,  0 },
    { 160.0f, EV_CELL_OFF, C_VI, 0 },
    { 166.0f, EV_CELL_OFF, C_V,  0 },
    { 170.0f, EV_MOOD,     55, 0 },         /* shimmer rises */
    { 175.0f, EV_TEXTURE,  32, 0 },

    /* ===== III. Cathedral (180–280 s) — big space, generative bar enters ===== */
    { 184.0f, EV_SPACE,    78, 0 },         /* room expands — long tails */
    { 188.0f, EV_GEN_ON,    1, 0 },         /* program 1: generative on */
    { 192.0f, EV_GEN_STEP,  0, 0 },         /* bar 1 */
    { 200.0f, EV_GEN_STEP,  0, 0 },         /* bar 2 */
    { 204.0f, EV_CELL_ON,  C_I,  0 },       /* play a hand-tap against the bed */
    { 208.0f, EV_GEN_STEP,  0, 0 },
    { 212.0f, EV_CELL_OFF, C_I,  0 },
    { 216.0f, EV_GEN_STEP,  0, 0 },
    { 220.0f, EV_CELL_ON,  C_IV, 0 },
    { 224.0f, EV_GEN_STEP,  0, 0 },
    { 228.0f, EV_CELL_ON,  C_VI, 0 },       /* IV + vi overlap with the bed */
    { 232.0f, EV_GEN_STEP,  0, 0 },
    { 236.0f, EV_CELL_OFF, C_IV, 0 },
    { 240.0f, EV_GEN_STEP,  0, 0 },
    { 244.0f, EV_CELL_OFF, C_VI, 0 },
    { 248.0f, EV_GEN_STEP,  0, 0 },
    { 252.0f, EV_MOOD,     68, 0 },         /* peak shimmer */
    { 256.0f, EV_GEN_STEP,  0, 0 },
    { 260.0f, EV_GEN_OFF,   0, 0 },         /* let the generator release */
    { 264.0f, EV_CELL_ON,  C_I,  0 },
    { 268.0f, EV_CELL_ON,  C_V,  0 },
    { 274.0f, EV_CELL_OFF, C_I,  0 },
    { 278.0f, EV_CELL_OFF, C_V,  0 },

    /* ===== IV. Shift (280–380 s) — key change to G, dorian + brass ===== */
    { 282.0f, EV_DRONE,     0, 0 },         /* drone out (key is about to move) */
    { 286.0f, EV_KEY,      67, 0 },         /* G4: tonic shifts a fifth up */
    { 288.0f, EV_MODE,      1, 0 },         /* dorian */
    { 290.0f, EV_VIBE,      1, 0 },         /* bright */
    { 294.0f, EV_VOICE,     2, 0 },         /* strings → brass (crossfade) */
    { 298.0f, EV_DRONE,     1, 0 },         /* re-anchor the drone on G */
    { 302.0f, EV_CELL_ON,  C_I,  0 },
    { 307.0f, EV_CELL_ON,  C_IV, 0 },       /* IV in dorian = a major colour */
    { 312.0f, EV_CELL_OFF, C_I,  0 },
    { 314.0f, EV_CELL_ON,  C_II, 0 },       /* ii is the dorian flavour */
    { 318.0f, EV_BRIGHTNESS,0, +320.0f },   /* open the cutoff — bright section */
    { 320.0f, EV_CELL_OFF, C_IV, 0 },
    { 322.0f, EV_CELL_OFF, C_II, 0 },
    { 326.0f, EV_CELL_ON,  C_V,  0 },
    { 330.0f, EV_CELL_ON,  C_VI, 0 },       /* held V + vi */
    { 336.0f, EV_CELL_OFF, C_V,  0 },
    { 340.0f, EV_CELL_OFF, C_VI, 0 },
    { 346.0f, EV_BASS,     55, 0 },         /* more low-end ahead of the next move */
    { 352.0f, EV_SPACE,    62, 0 },         /* hall contracts a touch */
    { 358.0f, EV_CELL_ON,  C_I,  0 },
    { 364.0f, EV_CELL_ON,  C_III,0 },
    { 372.0f, EV_CELL_OFF, C_I,  0 },
    { 376.0f, EV_CELL_OFF, C_III,0 },

    /* ===== V. Aeolian (380–460 s) — minor colour, mood down, deep bass ===== */
    { 382.0f, EV_MODE,      5, 0 },         /* aeolian (natural minor) */
    { 384.0f, EV_VIBE,      2, 0 },         /* deep */
    { 386.0f, EV_VOICE,     0, 0 },         /* back to warm pad */
    { 388.0f, EV_MOOD,     22, 0 },         /* darker */
    { 390.0f, EV_BRIGHTNESS,0, -240.0f },   /* and darker pad cutoff */
    { 394.0f, EV_KEY,      62, 0 },         /* D minor (relative-ish move; drone glides) */
    { 398.0f, EV_BASS,     72, 0 },         /* sub-bass opens up */
    { 402.0f, EV_CELL_ON,  C_I,  0 },
    { 408.0f, EV_CELL_ON,  C_VI, 0 },       /* i + VI in aeolian */
    { 414.0f, EV_CELL_ON,  C_IV, 0 },
    { 420.0f, EV_CELL_OFF, C_VI, 0 },
    { 424.0f, EV_CELL_OFF, C_IV, 0 },
    { 428.0f, EV_CELL_ON,  C_VII,0 },       /* bVII flat-seven motion */
    { 432.0f, EV_CELL_OFF, C_VII,0 },
    { 436.0f, EV_CELL_ON,  C_V,  0 },       /* v minor (not V — aeolian) */
    { 442.0f, EV_CELL_OFF, C_V,  0 },
    { 446.0f, EV_CELL_OFF, C_I,  0 },
    { 452.0f, EV_VIBE,      3, 0 },         /* floating */
    { 456.0f, EV_BASS,     40, 0 },         /* ease the bottom out */

    /* ===== VI. Dissolve (460–500 s) — release into the long reverb tail ===== */
    { 462.0f, EV_SPACE,    90, 0 },         /* maximum tail */
    { 466.0f, EV_DRONE,     0, 0 },         /* drone fades */
    { 470.0f, EV_TEXTURE,  12, 0 },         /* bed thins */
    { 474.0f, EV_MOOD,     30, 0 },
    { 478.0f, EV_BASS,     10, 0 },
    { 482.0f, EV_BRIGHTNESS,0, -300.0f },   /* darken the last breath */
    { 486.0f, EV_TEXTURE,   0, 0 },         /* bed out */
    { 488.0f, EV_ALL_OFF,   0, 0 },         /* any stuck voices off, let reverb ring */
};
#define N_EVENTS (sizeof TIMELINE / sizeof TIMELINE[0])
#define TOTAL_SECONDS 500                   /* 8 min 20 s — final ~12 s = pure tail */

int main(int argc, char **argv) {
    const char *path = (argc > 1) ? argv[1] : "field_ambience.wav";

    dsp_init();
    brain_init();
    engine_init();
    /* defaults already: C4 ionian / warm; engine boots texture at 0 */

    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); return 1; }

    uint32_t total_frames = (uint32_t)TOTAL_SECONDS * SR;
    write_wav_header(f, total_frames);

    int16_t buf[BLOCK * 2];
    uint32_t frame = 0;
    size_t ev = 0;
    int peak = 0;
    int sec_logged = -1;

    while (frame < total_frames) {
        float t = (float)frame / SR;
        /* fire any due events */
        while (ev < N_EVENTS && TIMELINE[ev].t <= t) {
            const evt_t *e = &TIMELINE[ev];
            switch (e->kind) {
                case EV_CELL_ON: {
                    int midi = brain_cell_root(e->a);
                    engine_note_on((uint8_t)e->a, dsp_midi_to_hz((float)midi), CELL_AMP);
                } break;
                case EV_CELL_OFF:   engine_note_off((uint8_t)e->a); break;
                case EV_TEXTURE:    engine_set_texture(e->a / 100.0f); break;
                case EV_BASS:       engine_set_bass_depth(e->a / 100.0f); break;
                case EV_SPACE:      engine_set_space(e->a / 100.0f); break;
                case EV_MOOD:       engine_set_mood(e->a / 100.0f); break;
                case EV_DRONE:      engine_set_drone(e->a != 0); break;
                case EV_VOICE:      engine_set_pad_voice(e->a); break;
                case EV_KEY:        engine_set_key(e->a); break;
                case EV_MODE:       engine_set_mode(e->a); break;
                case EV_VIBE:       engine_set_vibe(e->a); break;
                case EV_BRIGHTNESS: engine_set_brightness(e->b); break;
                case EV_GEN_ON:     engine_set_generative(true, e->a); break;
                case EV_GEN_OFF:    engine_set_generative(false, 0); break;
                case EV_GEN_STEP:   engine_generative_advance(); break;
                case EV_ALL_OFF:    engine_all_off(); break;
                default: break;
            }
            ++ev;
        }

        int n = (int)((total_frames - frame) < BLOCK ? (total_frames - frame) : BLOCK);
        engine_render(buf, n);
        for (int i = 0; i < n * 2; ++i) { int a = buf[i] < 0 ? -buf[i] : buf[i]; if (a > peak) peak = a; }
        fwrite(buf, sizeof(int16_t), (size_t)n * 2, f);
        frame += (uint32_t)n;

        /* one-shot per-minute progress line so a render that takes a while is
         * not a silent black box. */
        int sec = (int)(t);
        if (sec / 60 != sec_logged) { sec_logged = sec / 60;
            fprintf(stderr, "  render %dm / %dm\n", sec_logged, TOTAL_SECONDS / 60); }
    }

    fclose(f);
    printf("wrote %s  (%d s, %u frames, stereo 16-bit @ %d Hz)\n",
           path, TOTAL_SECONDS, total_frames, SR);
    printf("peak sample = %d / 32767  (%.1f dBFS)\n",
           peak, 20.0 * log10((double)(peak ? peak : 1) / 32767.0));
    return 0;
}
