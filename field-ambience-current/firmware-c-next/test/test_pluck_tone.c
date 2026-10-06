/* Audible PCM contract for the WOODLAND source. Tests spectral identity and
 * seeded level/DC stability, rather than duplicating the excitation formula. */
#include "pluck.h"
#include "dsp.h"
#include "shape.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define FRAMES 26460
#define START 4410
#define WINDOW (FRAMES - START)
#define PI 3.14159265358979323846
static float pcm[4][FRAMES], window[WINDOW];
static int checks, failures;
#define CHECK(x) do { ++checks; if (!(x)) { ++failures; fprintf(stderr, "FAIL %d: %s\n", __LINE__, #x); } } while (0)

static void render(int n) {
    memset(pcm, 0, sizeof pcm);
    pluck_render_mix(pcm[0], pcm[1], pcm[2], pcm[3], n);
}
static double spectral_power(double hz) {
    double c = 2 * cos(2 * PI * hz / DSP_SAMPLE_RATE_HZ), a = 0, b = 0;
    for (int n = 0; n < WINDOW; ++n) {
        double x = (pcm[0][n + START] + pcm[1][n + START]) * window[n];
        double next = x + c * a - b;
        b = a; a = next;
    }
    return a*a + b*b - c*a*b;
}
static double early_energy(float shape) {
    shape_set_attack(shape); pluck_init();
    CHECK(pluck_note_on(0, 220, .4f)); render(176);
    double energy = 0;
    for (int n = 0; n < 176; ++n) energy += pcm[0][n]*pcm[0][n];
    return energy;
}
int main(void) {
    dsp_init(); shape_init();
    for (int n = 0; n < WINDOW; ++n)
        window[n] = (float)(.5 - .5*cos(2*PI*n/(WINDOW-1)));
    const float pitches[] = {60, 110, 146.832f, 220, 440};
    const float damping[] = {0, .42f, .9f};
    const double pan_sum = cos(.65*PI/4) + sin(.65*PI/4);
    double worst_spread = 0, worst_mean = 0, weakest_fundamental = 1e9;
    for (unsigned f = 0; f < sizeof pitches/sizeof pitches[0]; ++f) {
        for (unsigned d = 0; d < sizeof damping/sizeof damping[0]; ++d) {
            pluck_init(); pluck_set_damp(damping[d]);
            double low = 1e9, high = 0;
            for (int repeat = 0; repeat < 12; ++repeat) {
                CHECK(pluck_note_on(0, pitches[f], .4f)); render(FRAMES);
                CHECK(pcm[0][0] == 0 && pcm[1][0] == 0 && pcm[2][0] == 0);
                double sum = 0, power = 0, peak = 0;
                int finite = 1;
                for (int n = 0; n < FRAMES; ++n) {
                    double x = (pcm[0][n] + pcm[1][n]) / pan_sum;
                    if (!isfinite(x)) finite = 0;
                    if (fabs(x) > peak) peak = fabs(x);
                    if (n >= START) { sum += x; power += x*x; }
                }
                double rms = sqrt(power/WINDOW), mean = fabs(sum/WINDOW)/rms;
                CHECK(finite);
                CHECK(rms > .001 && peak < .416);
                CHECK(mean < .035);
                if (mean > worst_mean) worst_mean = mean;
                if (rms < low) low = rms;
                if (rms > high) high = rms;
                double fundamental = spectral_power(pitches[f]), overtone = 0;
                for (int harmonic = 2; harmonic <= 5; ++harmonic) {
                    double p = spectral_power(harmonic * pitches[f]);
                    if (p > overtone) overtone = p;
                }
                double ratio = sqrt(fundamental/overtone);
                CHECK(ratio > 1.8);
                if (ratio < weakest_fundamental) weakest_fundamental = ratio;
                pluck_all_off(); render(882); CHECK(pluck_active_count() == 0);
            }
            double spread = 20*log10(high/low);
            CHECK(spread < .5);
            if (spread > worst_spread) worst_spread = spread;
        }
    }
    /* SHAPE may soften the onset, while the source remains recognizably
     * articulated rather than growing an unbounded swell. */
    double quick = early_energy(0), soft = early_energy(1);
    CHECK(quick > 100*soft && soft > 0);
    shape_init();
    /* More damping must not turn back into brightness. Compare rendered
     * second/fundamental energy over the entire normalized control range. */
    const float brightness_pitches[] = {110, 220, 440};
    for (unsigned f = 0; f < sizeof brightness_pitches/sizeof brightness_pitches[0]; ++f) {
        double previous = 1e9;
        for (int step = 0; step <= 9; ++step) {
            pluck_init(); pluck_set_damp(step*.1f);
            CHECK(pluck_note_on(0, brightness_pitches[f], .4f)); render(FRAMES);
            double ratio = spectral_power(2*brightness_pitches[f]) /
                           spectral_power(brightness_pitches[f]);
            CHECK(ratio < previous);
            previous = ratio;
        }
    }
    /* Upper regression range and near-Nyquist inputs must remain finite and
     * non-silent; only the low/mid register is a product sound candidate. */
    const float edge[] = {1760, 19850, 22049};
    for (unsigned f = 0; f < sizeof edge/sizeof edge[0]; ++f) {
        pluck_init(); CHECK(pluck_note_on(0, edge[f], 1)); render(4410);
        double energy = 0;
        int finite = 1;
        for (int n = 0; n < 4410; ++n) {
            if (!isfinite(pcm[0][n])) finite = 0;
            energy += pcm[0][n]*pcm[0][n];
        }
        CHECK(finite);
        CHECK(energy > 1e-8);
    }
    printf("pluck tone: %d checks, %d failures; repeat spread %.3f dB, "
           "window mean/RMS %.4f, fundamental/overtone >= %.2f\n",
           checks, failures, worst_spread, worst_mean, weakest_fundamental);
    return failures ? 1 : 0;
}
