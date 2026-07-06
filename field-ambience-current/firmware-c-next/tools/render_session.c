/*
 * render_session.c — a HUMAN play-session simulation (r18.95).
 *
 * Simulates a real player on the real device paths — nothing is driven
 * through engine internals directly:
 *
 *   - cells      → controls_cell_press/release (the host-tested hold-latch
 *                  state machine main_h743 routes MCP edges into)
 *   - modifiers  → controls_modifier (DRONE / HOLD / CLEAR)
 *   - DRIVE/BRIGHT encoders → params_encoder (real detents incl. the
 *                  acceleration tiers, driven with real timestamps)
 *   - menu edits → engine_set_space/atmosphere/age/echo in 2–5 % DETENT
 *                  STEPS 90–160 ms apart (exactly what menu_rotate does in
 *                  edit mode), never as a jump
 *   - world switch → engine_set_world + controls_refresh_held_pitches,
 *                  the same pair main_h743's hal_set_world calls
 *   - GENERATE is never touched — a player doesn't run the autopilot
 *     while playing (user brief).
 *
 * Timing is humanized at score-build time with a fixed-seed LCG: taps
 * jitter ±120 ms, hold lengths vary, velocities 0.10–0.16, detent gaps
 * 70–160 ms. Deterministic — the same session renders bit-identically.
 *
 * Arc (~4:10, Tokyo → After Hours):
 *   I.   bed + atmosphere bloom, DRONE in, first slow cells
 *   II.  SPACE opens mid-phrase, two-finger overlaps
 *   III. HOLD-latched chord, taps over it, BRIGHTNESS dips darker
 *   IV.  CLEAR → drone alone → world change to After Hours, AGE up
 *        (vinyl), ECHO in, sparser darker phrases
 *   V.   SPACE wide, last note, DRONE off, long tail.
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_session.c \
 *      src/dsp.c src/dsp_ladder.c src/pad.c src/padsynth.c src/body.c \
 *      src/texture.c src/ambience.c src/tape.c src/echo.c src/blur.c \
 *      src/bass.c src/drone.c src/reverb.c src/reverb_presets.c \
 *      src/brain.c src/worlds.c src/generative.c src/cells.c \
 *      src/pluck.c src/controls.c src/params.c src/engine.c \
 *      -lm -o /tmp/render_session
 */

#include "engine.h"
#include "brain.h"
#include "controls.h"
#include "params.h"
#include "dsp.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define SR     44100
#define BLOCK  256
#define TOTAL_S 250

/* ---- WAV writer ---- */
static void put_u32(FILE *f, uint32_t v) { fputc(v & 0xff, f); fputc((v >> 8) & 0xff, f); fputc((v >> 16) & 0xff, f); fputc((v >> 24) & 0xff, f); }
static void put_u16(FILE *f, uint16_t v) { fputc(v & 0xff, f); fputc((v >> 8) & 0xff, f); }
static void wav_header(FILE *f, uint32_t nframes) {
    uint32_t data = nframes * 4u;
    fwrite("RIFF", 1, 4, f); put_u32(f, 36 + data); fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f); put_u32(f, 16); put_u16(f, 1); put_u16(f, 2);
    put_u32(f, SR); put_u32(f, SR * 4u); put_u16(f, 4); put_u16(f, 16);
    fwrite("data", 1, 4, f); put_u32(f, data);
}

/* ---- event score ---- */
typedef enum {
    EV_CELL_DN, EV_CELL_UP, EV_MOD,
    EV_SPACE, EV_ATMOS, EV_AGE, EV_ECHO, EV_TEXTURE,
    EV_WORLD, EV_ENC,
    EV_VOICE, EV_KEY,          /* r18.98: menu VOICE + KEY slots */
    EV_SHIM,                   /* r18.99: SHIMMER menu slot */
} ev_type_t;

typedef struct { uint32_t ms; ev_type_t t; int a; float f; } ev_t;

#define MAX_EV 512
static ev_t score[MAX_EV];
static int  n_ev = 0;

static uint32_t rng = 0x5E551011u;
static float rnd01(void) {
    rng = rng * 1664525u + 1013904223u;
    return (float)(rng >> 8) / 16777216.0f;
}
/* human jitter: ± range_ms around t */
static uint32_t hj(uint32_t t, int range_ms) {
    return t + (uint32_t)((rnd01() * 2.0f - 1.0f) * (float)range_ms);
}
static float vel(void) { return 0.10f + rnd01() * 0.06f; }

static void ev(uint32_t ms, ev_type_t t, int a, float f) {
    if (n_ev >= MAX_EV) { fprintf(stderr, "score full\n"); exit(1); }
    score[n_ev++] = (ev_t){ ms, t, a, f };
}

/* a cell tap: press at t (humanized), release after hold_ms (humanized) */
static void tap(uint32_t t, int cell, uint32_t hold_ms) {
    uint32_t dn = hj(t, 120);
    ev(dn, EV_CELL_DN, cell, vel());
    ev(dn + hj(hold_ms, 250), EV_CELL_UP, cell, 0);
}

/* a latch tap while HOLD is on: short press-release toggles the latch */
static void latch_tap(uint32_t t, int cell) {
    uint32_t dn = hj(t, 100);
    ev(dn, EV_CELL_DN, cell, vel());
    ev(dn + 140 + (uint32_t)(rnd01() * 80.0f), EV_CELL_UP, cell, 0);
}

/* a menu value turned in detent steps from `from` to `to` */
static void menu_turn(uint32_t t, ev_type_t what, float from, float to, float step) {
    float v = from;
    uint32_t at = t;
    while ((step > 0 && v < to - 1e-4f) || (step < 0 && v > to + 1e-4f)) {
        v += step;
        ev(at, what, 0, v);
        at += 90 + (uint32_t)(rnd01() * 70.0f);
    }
}

/* an encoder turned n detents in one direction */
static void enc_turn(uint32_t t, int enc_id, int dir, int detents) {
    uint32_t at = t;
    for (int i = 0; i < detents; ++i) {
        ev(at, EV_ENC, enc_id, (float)dir);
        at += 70 + (uint32_t)(rnd01() * 60.0f);
    }
}

static int ev_cmp(const void *a, const void *b) {
    uint32_t ta = ((const ev_t *)a)->ms, tb = ((const ev_t *)b)->ms;
    return ta < tb ? -1 : (ta > tb ? 1 : 0);
}

static void build_score(void) {
    /* I. awakening (Tokyo is the boot world) */
    ev(500,  EV_TEXTURE, 0, 0.20f);
    ev(800,  EV_SHIM, 0, 0.12f);                  /* Tokyo preset halo */
    menu_turn(2000, EV_ATMOS, 0.0f, 0.30f, 0.05f);
    ev(6000, EV_MOD, MOD_DRONE, 0);              /* pedal blooms (6 s attack) */

    tap(14000, 2, 4200);
    tap(21000, 0, 3600);
    tap(27500, 3, 3400);
    tap(33000, 1, 4800);                          /* two fingers overlap */
    tap(34400, 4, 4600);

    /* II. the room opens mid-phrase */
    menu_turn(42000, EV_SPACE, 0.50f, 0.72f, 0.02f);
    tap(46500, 2, 3500);
    tap(53000, 0, 3000);

    /* III. a chord is latched, playing over it, light dips.
     * r18.98: the player flips VOICE to STRING first — the short taps that
     * follow get a plucked attack in front of the pad swell. */
    ev(59000, EV_VOICE, 1, 0);
    ev(60000, EV_MOD, MOD_HOLD, 0);
    latch_tap(61000, 0);
    latch_tap(62400, 3);
    ev(66000, EV_MOD, MOD_HOLD, 0);               /* HOLD off — taps momentary */
    tap(68000, 4,  900);
    tap(72500, 4,  700);
    tap(78000, 4, 1100);
    enc_turn(84000, PARAM_ENC_BRIGHT, -1, 8);     /* darker */
    tap(96000, 2, 3500);
    enc_turn(104000, PARAM_ENC_DRIVE, +1, 5);     /* a little warmer */
    enc_turn(108000, PARAM_ENC_BRIGHT, +1, 5);    /* light comes back */

    /* IV. clear → alone with the drone → new world */
    ev(112000, EV_MOD, MOD_CLEAR, 0);
    ev(116000, EV_WORLD, 3, 0);                   /* After Hours */
    menu_turn(118000, EV_AGE, 0.0f, 0.50f, 0.05f);
    tap(124000, 1, 4000);
    tap(132000, 3, 4500);
    menu_turn(140000, EV_ECHO, 0.0f, 0.35f, 0.05f);
    tap(144500, 0, 3500);
    /* r18.98: After Hours gets the GLASS voice — FM chimes over the dark
     * C-minor bed instead of the string. */
    ev(149000, EV_VOICE, 2, 0);
    tap(152000, 2, 4000);
    tap(162000, 4, 4800);

    /* V. wide space, a modulation, last word, lights out.
     * r18.98: the player turns KEY two detents up (C → D) before the final
     * phrase — a real modulation, held notes re-pitch with it. */
    menu_turn(172000, EV_SPACE, 0.72f, 0.86f, 0.02f);
    menu_turn(174000, EV_SHIM, 0.12f, 0.34f, 0.02f);  /* halo blooms for the end */
    ev(175800, EV_KEY, 1, 0);
    ev(175960, EV_KEY, 2, 0);
    tap(178000, 1, 5000);
    ev(188000, EV_MOD, MOD_DRONE, 0);             /* drone released */

    qsort(score, (size_t)n_ev, sizeof(ev_t), ev_cmp);
}

int main(int argc, char **argv) {
    const char *path = argc > 1 ? argv[1] : "field_ambience_played_session.wav";
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); return 1; }

    dsp_init(); brain_init(); engine_init();
    controls_init(); params_init();
    build_score();

    const uint32_t frames_total = (uint32_t)SR * TOTAL_S;
    wav_header(f, frames_total);

    int16_t buf[BLOCK * 2];
    uint32_t done = 0; int next = 0;
    while (done < frames_total) {
        uint32_t now_ms = (uint32_t)((uint64_t)done * 1000u / SR);
        while (next < n_ev && score[next].ms <= now_ms) {
            const ev_t *e = &score[next++];
            switch (e->t) {
                case EV_CELL_DN: controls_cell_press((uint8_t)e->a, e->f); break;
                case EV_CELL_UP: controls_cell_release((uint8_t)e->a);     break;
                case EV_MOD:     controls_modifier((mod_id_t)e->a, true);  break;
                case EV_SPACE:   engine_set_space(e->f);                   break;
                case EV_ATMOS:   engine_set_atmosphere(e->f);              break;
                case EV_AGE:     engine_set_age(e->f);                     break;
                case EV_ECHO:    engine_set_echo(e->f);                    break;
                case EV_TEXTURE: engine_set_texture(e->f);                 break;
                case EV_WORLD:   engine_set_world(e->a);
                                 controls_refresh_held_pitches();          break;
                case EV_VOICE:   engine_set_voice(e->a);                   break;
                case EV_KEY:     engine_set_key_pc(e->a);
                                 controls_refresh_held_pitches();          break;
                case EV_SHIM:    engine_set_shimmer(e->f);                 break;
                case EV_ENC:     params_encoder((uint8_t)e->a,
                                                (int)e->f, now_ms);        break;
            }
        }
        int n = (int)(frames_total - done < BLOCK ? frames_total - done : BLOCK);
        engine_render(buf, n);
        fwrite(buf, sizeof(int16_t), (size_t)n * 2, f);
        done += (uint32_t)n;
        if ((done % (SR * 30)) < BLOCK)
            fprintf(stderr, "  render %us / %us\n", done / SR, TOTAL_S);
    }
    fclose(f);
    fprintf(stderr, "wrote %s (%u s, %d events)\n", path, TOTAL_S, n_ev);
    return 0;
}
