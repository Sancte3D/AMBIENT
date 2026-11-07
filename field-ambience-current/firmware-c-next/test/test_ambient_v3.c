/* Ambient voicing V3 (FAM_SOUND_AMBIENT_V3) through the real Product engine.
 * Register: Cells D4..B4, SHIFT an exact octave. Material: no dominant
 * octave/double-octave/third, quiet air instead of aspiration noise.
 * Room: audible return around -6 dB of Dry at Room .5. Every live Cell plays,
 * HARMONY/LANDSCAPE pitches of the previous register stay admitted.
 * Identity: a rich smooth COAST pad and a pure breathy HIGHLANDS whistle.
 * Host proof of the voicing contract, not a listening or device acceptance. */
#include "engine.h"
#include "engine_product.h"
#include "gesture.h"
#include "dsp.h"
#include "tuning.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

enum { SR = 44100, TICK = 441, SECONDS = 16, FRAMES = SECONDS * SR };
static const double PI_D = 3.14159265358979323846;
static float left[FRAMES], right[FRAMES];

static void render_note(int world, float room, int midi, uint32_t off_ms) {
    engine_init(); gesture_init_owned(engine_gesture_note); engine_set_world(world); engine_set_gen_seed(1234);
    engine_set_color(.5f); engine_set_master_volume(.75f); engine_set_nature(0); engine_set_room(room);
    for (int frame = 0; frame < FRAMES; frame += TICK) {
        uint32_t ms = (uint32_t)((uint64_t)frame * 1000 / SR);
        if (ms == 1000) assert(engine_try_note_on(0, tuning_hz((float)midi), .7f));
        if (ms == off_ms) engine_note_off(0);
        engine_generative_tick(ms);
        int16_t pcm[2 * TICK]; engine_render(pcm, TICK);
        for (int k = 0; k < TICK; ++k) { left[frame + k] = pcm[2 * k] / 32768.0f; right[frame + k] = pcm[2 * k + 1] / 32768.0f; }
    }
    assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
}
/* Goertzel magnitude of the mono mix at hz over [start, start+len) seconds. */
static double tone(double hz, double start, double len) {
    int a = (int)(start * SR), n = (int)(len * SR);
    double w = 2 * PI_D * hz / SR, c = 2 * cos(w), s1 = 0, s2 = 0;
    for (int i = 0; i < n; ++i) {
        double hann = .5 - .5 * cos(2 * PI_D * i / (n - 1));
        double s = hann * .5 * (left[a + i] + right[a + i]) + c * s1 - s2; s2 = s1; s1 = s;
    }
    return sqrt(s1 * s1 + s2 * s2 - c * s1 * s2);
}
/* RMS Goertzel magnitude at hz averaged over frames of `frame` seconds (half
 * overlap): a held note's slow ensemble beat and drifts average out over
 * whole cycles, so a ratio describes the instrument, not one instant of its
 * breathing. Compare only values measured with the same frame length. */
static double band(double hz, double start, double len, double frame) {
    double p = 0; int n = 0;
    for (double t = start; t + frame <= start + len; t += frame / 2, ++n) { double m = tone(hz, t, frame); p += m * m; }
    return sqrt(p / n);
}
static double db(double a, double b) { return 20 * log10(a / b); }
/* Heard Generate notes: count, register and the longest silence between starts. */
static double score_t, score_last, score_gap;
static int score_n;
static float score_lo, score_hi;
static void score_hook(int on, uint8_t source, float hz, float amp) {
    (void)source; (void)amp;
    if (!on) return;
    if (score_t - score_last > score_gap) score_gap = score_t - score_last;
    score_last = score_t; ++score_n;
    if (hz < score_lo) score_lo = hz;
    if (hz > score_hi) score_hi = hz;
}
/* A5 held for 20 s, dry (left/right hold seconds 0..16 of it, from 6 s). */
enum { LONG = 26 * SR };
static float long_mono[LONG];
static void render_long_note(int world, int midi) {
    engine_init(); gesture_init_owned(engine_gesture_note); engine_set_world(world); engine_set_gen_seed(1234);
    engine_set_color(.5f); engine_set_master_volume(.75f); engine_set_nature(0); engine_set_room(0);
    for (int frame = 0; frame < LONG; frame += TICK) {
        uint32_t ms = (uint32_t)((uint64_t)frame * 1000 / SR);
        if (ms == 1000) assert(engine_try_note_on(0, tuning_hz((float)midi), .6f));
        if (ms == 23000) engine_note_off(0);
        engine_generative_tick(ms);
        int16_t pcm[2 * TICK]; engine_render(pcm, TICK);
        for (int k = 0; k < TICK; ++k) long_mono[frame + k] = .5f * (pcm[2 * k] + pcm[2 * k + 1]) / 32768.0f;
    }
}
/* Level envelope of 4..22 s in 20 ms frames (dB, linear trend removed): its
 * peak-to-peak swing, and its 2-8 Hz modulation power relative to .03-.3 Hz. */
static void motion(double *swing, double *ratio) {
    enum { FR = SR / 50, N = 18 * 50 };
    static double e[N];
    double sx = 0, sy = 0, sxx = 0, sxy = 0;
    for (int i = 0; i < N; ++i) {
        double p = 0; for (int k = 0; k < FR; ++k) { double v = long_mono[4 * SR + i * FR + k]; p += v * v; }
        e[i] = 10 * log10(p / FR + 1e-18); sx += i; sy += e[i]; sxx += (double)i * i; sxy += i * e[i];
    }
    double slope = (N * sxy - sx * sy) / (N * sxx - sx * sx), icpt = (sy - slope * sx) / N, mn = 1e9, mx = -1e9;
    for (int i = 0; i < N; ++i) { e[i] -= icpt + slope * i; if (e[i] < mn) mn = e[i]; if (e[i] > mx) mx = e[i]; }
    double slow = 0, fast = 0;
    for (int b = 1; b < N / 2; ++b) {
        double f = b * 50.0 / N, re = 0, im = 0;
        if (!((f >= .03 && f < .3) || (f >= 2 && f < 8))) continue;
        for (int i = 0; i < N; ++i) {
            double w = .5 - .5 * cos(2 * PI_D * i / (N - 1));
            re += w * e[i] * cos(2 * PI_D * b * i / N); im -= w * e[i] * sin(2 * PI_D * b * i / N);
        }
        if (f < .3) slow += re * re + im * im; else fast += re * re + im * im;
    }
    *swing = mx - mn; *ratio = 10 * log10(fast / (slow + 1e-30) + 1e-30);
}
static double energy(const float *x, const float *ref, int a, int b, bool diff) {
    double e = 0; for (int i = a; i < b; ++i) { double v = diff ? x[i] - ref[i] : x[i]; e += v * v; } return e;
}

int main(void) {
    dsp_init();
    /* 1. Register and SHIFT octave, every Cell admitted on the live path. */
    static const int base[5] = {62, 64, 66, 69, 71};
    for (int world = 0; world < 3; ++world) {
        engine_init(); engine_set_world(world);
        for (int c = 0; c < 5; ++c) {
            assert(engine_product_cell_midi(c, false) == base[c]);
            assert(engine_product_cell_midi(c, true) == base[c] + 12);
        }
        for (int upper = 0; upper < 2; ++upper) for (int c = 0; c < 5; ++c) {
            engine_init(); engine_set_world(world);
            uint8_t owner = (uint8_t)(upper ? 9 + c : c);
            assert(engine_try_cell_on(owner, tuning_hz((float)engine_product_cell_midi(c, upper != 0)), .545f));
        }
    }
    puts("ambient V3 register: Cells D4..B4, SHIFT exact octave, all 30 live Cells admitted");

    /* 1b. Live admission keeps the previous register: HARMONY voicings and the
     * LANDSCAPE bed still voice from brain.c in octave 3/4. Every pentatonic
     * pitch D3..D6, in every World, key and mode, is admitted on its own. */
    int admitted = 0;
    for (int world = 0; world < 3; ++world) for (int minor = 0; minor < 2; ++minor)
        for (int key = 0; key < 12; ++key) for (int m = 50; m <= 86; ++m) {
            static const int major[5] = {0, 2, 4, 7, 9}, dark[5] = {0, 3, 5, 7, 10};
            const int *p = minor ? dark : major;
            int pc = ((m - key) % 12 + 12) % 12; bool in = false;
            for (int i = 0; i < 5; ++i) in |= pc == p[i];
            if (!in) continue;
            engine_init(); engine_set_world(world); engine_set_mode(minor);
            engine_set_key_pc((key + 1) % 12); engine_set_key_pc(key);
            assert(engine_try_note_on(0, tuning_hz((float)m), .5f)); ++admitted;
        }
    printf("ambient V3 live admission: %d single pentatonic pitches D3..D6, all Worlds/keys/modes admitted\n", admitted);

    /* 2. Material: A4 dry. WOODLAND: the struck ring just after the attack.
     * COAST/HIGHLANDS: 9 s of the held note, longer than the slowest beat. */
    static const char *name[3] = {"COAST", "WOODLAND", "HIGHLANDS"};
    double h2db[3], h3db[3], h4db[3], near_db[3];
    for (int world = 0; world < 3; ++world) {
        render_note(world, 0, 69, 13000);
        bool ring = world == 1;
        double t0 = ring ? 1.2 : 3.5, len = ring ? .8 : 9.0;
        /* Partial ratios: 0.5 s frames. Noise-to-partial metrics below: 2 s
         * frames, the grid the -56 dB air limit was calibrated on. */
#define AT(hz, frame) (ring ? tone((hz), t0, len) : band((hz), t0, len, (frame)))
        double h1 = AT(440, .5), h2 = AT(880, .5), h3 = AT(1320, .5), h4 = AT(1760, .5), f1 = AT(440, 2.0);
        h2db[world] = db(h2, h1); h3db[world] = db(h3, h1); h4db[world] = db(h4, h1);
        printf("ambient V3 %-9s H2 %6.1f dB  H3 %6.1f dB  H4 %6.1f dB re H1\n", name[world], db(h2, h1), db(h3, h1), db(h4, h1));
        assert(db(h2, h1) < -6 && db(h3, h1) < -10 && db(h4, h1) < -14);
        /* Breath at the note: 20..40 Hz beside the fundamental, clear of the
         * HIGHLANDS vibrato sidebands and of the COAST ensemble partner. */
        double near = 0; for (int k = 0; k < 5; ++k) { double a = AT(460 + 5 * k, 2.0), b = AT(420 - 5 * k, 2.0); near += a * a + b * b; }
        near_db[world] = 10 * log10(near / 10 / (f1 * f1));
        /* Air instead of aspiration: energy between the partials above 2.5 kHz. */
        double air = 0, total = 0;
        for (double f = 2600; f < 6000; f += 37.0) { double m = AT(f, 2.0); air += m * m; }
        for (int h = 1; h <= 5; ++h) { double m = AT(440.0 * h, 2.0); total += m * m; }
#undef AT
        double air_db = 10 * log10(air / total);
        printf("ambient V3 %-9s air/noise %6.1f dB re partials\n", name[world], air_db);
        assert(air_db < -56); /* old HIGHLANDS aspiration: -52 dB on this grid */
    }
    /* Three identities, not three soft sines: COAST is a rich pad on one smooth
     * slope (no horn bump), HIGHLANDS a pure whistle with breath at its note. */
    printf("ambient V3 breath beside the note: COAST %.1f dB, WOODLAND %.1f dB, HIGHLANDS %.1f dB re H1\n",
           near_db[0], near_db[1], near_db[2]);
    assert(h2db[0] > h3db[0] && h3db[0] > h4db[0]);
    assert(h3db[0] - h3db[2] > 8 && h2db[2] < -14);
    assert(near_db[2] - near_db[0] > 10 && near_db[2] - near_db[1] > 10);
    puts("ambient V3 identities: rich smooth COAST pad, pure breathy HIGHLANDS whistle");

    /* 3. Room: audible, bounded return at the default Room .5. */
    static float dry_l[FRAMES], dry_r[FRAMES];
    for (int world = 0; world < 3; ++world) {
        render_note(world, 0, 69, 5000); memcpy(dry_l, left, sizeof left); memcpy(dry_r, right, sizeof right);
        render_note(world, .5f, 69, 5000);
        int a = SR, b = 11 * SR;
        double wet = energy(left, dry_l, a, b, true) + energy(right, dry_r, a, b, true);
        double dry = energy(dry_l, NULL, a, b, false) + energy(dry_r, NULL, a, b, false);
        double share = 10 * log10(wet / dry);
        printf("ambient V3 %-9s room share at Room .5 %6.1f dB re Dry\n", name[world], share);
        assert(share > -9 && share < -3);
    }
    puts("ambient V3: soft spectra, quiet air, audible bounded room pass");

    /* 4. Generate lives in the comfortable middle and keeps flowing: every
     * World, key and mode; every heard note within 200 Hz..the World ceiling,
     * no phrase stall. (Long low drones and long ~1 kHz whistles were what made
     * the earlier voicings feel unsettling.) */
    static const float ceiling[3] = {850, 470, 650};
    for (int world = 0; world < 3; ++world) {
        int notes_min = 1 << 30; double gap_max = 0; float lo = 1e9f, hi = 0;
        for (int minor = 0; minor < 2; ++minor) for (int key = 0; key < 12; ++key) {
            engine_init(); gesture_init_owned(engine_gesture_note); engine_set_world(world);
            engine_set_mode(minor); engine_set_key_pc((key + 1) % 12); engine_set_key_pc(key);
            engine_set_gen_seed(1234u + (uint32_t)key); engine_set_note_hook(score_hook);
            score_n = 0; score_last = 1.0; score_gap = 0; score_lo = 1e9f; score_hi = 0;
            for (int frame = 0; frame < 100 * SR; frame += TICK) {
                uint32_t ms = (uint32_t)((uint64_t)frame * 1000 / SR); score_t = frame / (double)SR;
                if (ms == 1000) engine_set_generative(true, -1);
                engine_generative_tick(ms);
                int16_t pcm[2 * TICK]; engine_render(pcm, TICK);
            }
            engine_set_note_hook(NULL);
            if (100.0 - score_last > score_gap) score_gap = 100.0 - score_last;
            assert(!engine_nonfinite_samples() && !engine_output_limited_samples());
            if (score_n < notes_min) notes_min = score_n;
            if (score_gap > gap_max) gap_max = score_gap;
            if (score_lo < lo) lo = score_lo;
            if (score_hi > hi) hi = score_hi;
        }
        printf("ambient V3 %-9s Generate 24 key/mode runs: %.0f-%.0f Hz (ceiling %.0f), >= %d notes/100 s, longest gap %.1f s\n",
               name[world], lo, hi, ceiling[world], notes_min, gap_max);
        assert(lo >= 200 && hi <= ceiling[world] * 1.01f && notes_min >= 8 && gap_max < 45);
    }

    /* 5. A held note breathes slowly, never flutters: A5 held 20 s. At least
     * 2 dB of slow level motion, and modulation in the 2-8 Hz band (the
     * fluctuation-strength region around 4 Hz) far below the slow motion. */
    for (int world = 0; world < 3; world += 2) {
        render_long_note(world, 81);
        double swing, ratio; motion(&swing, &ratio);
        printf("ambient V3 %-9s held A5: slow level motion %.1f dB, 2-8 Hz modulation %+.1f dB re slow\n", name[world], swing, ratio);
        assert(swing > 2 && swing < 12 && ratio < -10);
    }
    puts("ambient V3: middle register, flowing Generate, slowly breathing held notes pass");
    return 0;
}
