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

enum { SR = 44100, TICK = 441, SECONDS = 14, FRAMES = SECONDS * SR };
static const double PI_D = 3.14159265358979323846;
static float left[FRAMES], right[FRAMES];

static void render_note(int world, float room, int midi) {
    engine_init(); gesture_init_owned(engine_gesture_note); engine_set_world(world); engine_set_gen_seed(1234);
    engine_set_color(.5f); engine_set_master_volume(.75f); engine_set_nature(0); engine_set_room(room);
    for (int frame = 0; frame < FRAMES; frame += TICK) {
        uint32_t ms = (uint32_t)((uint64_t)frame * 1000 / SR);
        if (ms == 1000) assert(engine_try_note_on(0, tuning_hz((float)midi), .7f));
        if (ms == 5000) engine_note_off(0);
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
static double db(double a, double b) { return 20 * log10(a / b); }
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

    /* 2. Material: A4 dry, sustained part of the note. */
    static const char *name[3] = {"COAST", "WOODLAND", "HIGHLANDS"};
    double h2db[3], h3db[3], h4db[3], near_db[3];
    for (int world = 0; world < 3; ++world) {
        render_note(world, 0, 69);
        double t0 = world == 1 ? 1.2 : 2.5, len = world == 1 ? .8 : 2.0;
        double h1 = tone(440, t0, len), h2 = tone(880, t0, len), h3 = tone(1320, t0, len), h4 = tone(1760, t0, len);
        h2db[world] = db(h2, h1); h3db[world] = db(h3, h1); h4db[world] = db(h4, h1);
        printf("ambient V3 %-9s H2 %6.1f dB  H3 %6.1f dB  H4 %6.1f dB re H1\n", name[world], db(h2, h1), db(h3, h1), db(h4, h1));
        assert(db(h2, h1) < -6 && db(h3, h1) < -10 && db(h4, h1) < -14);
        /* Breath at the note: 20..40 Hz beside the fundamental, clear of the
         * HIGHLANDS vibrato sidebands and of the COAST ensemble partner. */
        double near = 0; for (int k = 0; k < 5; ++k) { double a = tone(460 + 5 * k, t0, len), b = tone(420 - 5 * k, t0, len); near += a * a + b * b; }
        near_db[world] = 10 * log10(near / 10 / (h1 * h1));
        /* Air instead of aspiration: energy between the partials above 2.5 kHz. */
        double air = 0, total = 0;
        for (double f = 2600; f < 6000; f += 37.0) { double m = tone(f, t0, len); air += m * m; }
        for (int h = 1; h <= 5; ++h) { double m = tone(440.0 * h, t0, len); total += m * m; }
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
        render_note(world, 0, 69); memcpy(dry_l, left, sizeof left); memcpy(dry_r, right, sizeof right);
        render_note(world, .5f, 69);
        int a = SR, b = 11 * SR;
        double wet = energy(left, dry_l, a, b, true) + energy(right, dry_r, a, b, true);
        double dry = energy(dry_l, NULL, a, b, false) + energy(dry_r, NULL, a, b, false);
        double share = 10 * log10(wet / dry);
        printf("ambient V3 %-9s room share at Room .5 %6.1f dB re Dry\n", name[world], share);
        assert(share > -9 && share < -3);
    }
    puts("ambient V3: soft spectra, quiet air, audible bounded room pass");
    return 0;
}
