/*
 * render_pad_profiles.c — A/B audition tool for the four pad timbre profiles.
 *
 * Renders the PAD VOICE IN ISOLATION (no drone, no bass, no reverb, no
 * texture) for each of the four profiles, so the listener can hear the
 * character without engine-wide wash masking it. Same musical phrase per
 * profile → direct A/B.
 *
 * Output: one stereo 16-bit WAV per profile, plus one combined WAV with
 * 30 s per profile back-to-back (so it's easy to share / compare in a
 * single file).
 *
 * Build (from firmware-c-next/):
 *   cc -std=c11 -O2 -Iinclude tools/render_pad_profiles.c \
 *      src/dsp.c src/pad.c -lm -o /tmp/render_pad_profiles
 *   /tmp/render_pad_profiles /tmp/pad_profile_
 */

#include "pad.h"
#include "dsp.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define SR              44100
#define BLOCK           256
#define PROFILE_SECS    30
#define N_PROFILES      4

static void put_u32(FILE *f,uint32_t v){fputc(v&0xff,f);fputc((v>>8)&0xff,f);fputc((v>>16)&0xff,f);fputc((v>>24)&0xff,f);}
static void put_u16(FILE *f,uint16_t v){fputc(v&0xff,f);fputc((v>>8)&0xff,f);}
static void wav_header(FILE *f,uint32_t nframes){
    uint32_t data=nframes*4u;
    fwrite("RIFF",1,4,f);put_u32(f,36+data);fwrite("WAVE",1,4,f);
    fwrite("fmt ",1,4,f);put_u32(f,16);put_u16(f,1);put_u16(f,2);
    put_u32(f,SR);put_u32(f,SR*4u);put_u16(f,4);put_u16(f,16);
    fwrite("data",1,4,f);put_u32(f,data);
}

/* Same phrase per profile so the listener compares timbre, not arrangement.
 * Mix of held chord pads (cells 5-7) + melodic taps (cells 0-4), the way
 * a player would actually use the instrument. MIDI numbers around C4 (60). */

typedef struct { float t; uint8_t src; int midi; float vel; float dur; } evt_t;

/* ~28 s of musical material; the final 2 s lets the last note ring. */
static const evt_t EVENTS[] = {
    /* chord layer enters first — three sustained voices forming a Cm9-ish pad */
    { 0.20f, 5, 60, 0.10f, 22.0f },  /* C4 */
    { 0.30f, 6, 67, 0.09f, 22.0f },  /* G4 (open 5th) */
    { 0.45f, 7, 74, 0.08f, 22.0f },  /* D5 (9th) */

    /* melodic taps over the top — wandering, dynamic */
    { 1.50f, 0, 72, 0.13f, 1.4f  },
    { 2.40f, 1, 75, 0.11f, 1.2f  },
    { 3.20f, 2, 79, 0.16f, 1.6f  },
    { 4.30f, 3, 77, 0.10f, 1.0f  },
    { 5.10f, 4, 84, 0.14f, 1.8f  },

    /* chord change at 7 s — move to Ab/Eb (relative) for a warm progression */
    { 7.00f, 5, 56, 0.10f, 16.0f }, /* Ab3 */
    { 7.05f, 6, 63, 0.09f, 16.0f }, /* Eb4 */
    { 7.15f, 7, 70, 0.08f, 16.0f }, /* Bb4 */

    { 8.40f, 0, 75, 0.13f, 1.4f  },
    { 9.20f, 2, 80, 0.12f, 1.3f  },
    {10.30f, 4, 82, 0.10f, 1.6f  },
    {11.20f, 1, 77, 0.14f, 1.2f  },
    {12.00f, 3, 72, 0.11f, 1.5f  },
    {13.10f, 0, 80, 0.15f, 1.8f  },

    /* chord change at 14 s — back to a Cm/Bb6 colour */
    {14.00f, 5, 58, 0.10f, 12.0f }, /* Bb3 */
    {14.10f, 6, 65, 0.09f, 12.0f }, /* F4 */
    {14.20f, 7, 72, 0.08f, 12.0f }, /* C5 */

    {15.30f, 2, 77, 0.13f, 1.4f  },
    {16.40f, 4, 82, 0.11f, 1.6f  },
    {17.20f, 1, 75, 0.16f, 1.2f  },
    {18.10f, 3, 80, 0.12f, 1.0f  },
    {19.00f, 0, 84, 0.14f, 1.8f  },
    {20.40f, 2, 79, 0.10f, 1.3f  },
    {21.50f, 4, 75, 0.13f, 1.6f  },
    {22.30f, 1, 72, 0.11f, 1.4f  },

    /* sparse outro — last few taps with longer gaps to hear the tail */
    {24.20f, 0, 77, 0.12f, 2.5f  },
    {26.00f, 3, 72, 0.10f, 3.0f  },
};
#define N_EVT (sizeof EVENTS / sizeof EVENTS[0])

typedef struct { float t; uint8_t src; } pend_t;
static pend_t pend[64]; static int npend = 0;
static void sched_off(float t, uint8_t src){ if(npend<64){pend[npend].t=t;pend[npend].src=src;npend++;} }

static int render_profile(int profile_id, const char *out_path,
                          FILE *combined_f) {
    pad_init();
    pad_set_profile(profile_id);
    npend = 0;

    FILE *f = NULL;
    if (out_path) {
        f = fopen(out_path, "wb");
        if (!f) { fprintf(stderr, "cannot open %s\n", out_path); return -1; }
        wav_header(f, (uint32_t)PROFILE_SECS * SR);
    }

    int16_t buf[BLOCK*2];
    uint32_t total = (uint32_t)PROFILE_SECS * SR;
    uint32_t frame = 0;
    size_t  evt_i = 0;
    int peak = 0;
    double sumsq = 0; long sumN = 0;

    while (frame < total) {
        float t = (float)frame / SR;

        while (evt_i < N_EVT && EVENTS[evt_i].t <= t) {
            const evt_t *e = &EVENTS[evt_i++];
            pad_note_on(e->src, dsp_midi_to_hz((float)e->midi), e->vel);
            sched_off(t + e->dur, e->src);
        }
        for (int i = 0; i < npend; ) {
            if (pend[i].t <= t) { pad_note_off(pend[i].src); pend[i] = pend[--npend]; }
            else ++i;
        }

        int n = (int)((total - frame) < BLOCK ? (total - frame) : BLOCK);
        pad_render(buf, n);

        for (int i = 0; i < n*2; ++i) {
            int a = buf[i] < 0 ? -buf[i] : buf[i];
            if (a > peak) peak = a;
            double s = buf[i] / 32768.0;
            sumsq += s * s; ++sumN;
        }

        if (f)            fwrite(buf, sizeof(int16_t), (size_t)n*2, f);
        if (combined_f)   fwrite(buf, sizeof(int16_t), (size_t)n*2, combined_f);
        frame += (uint32_t)n;
    }

    if (f) fclose(f);
    double rms = sqrt(sumsq / (double)sumN);
    double rms_db = 20.0 * log10(rms + 1e-12);
    double peak_db = 20.0 * log10((double)(peak ? peak : 1) / 32767.0);
    printf("  profile %d (%s): peak %d (%.1f dBFS), RMS %.4f (%.1f dBFS)\n",
           profile_id, pad_profile_name(profile_id), peak, peak_db, rms, rms_db);
    return 0;
}

int main(int argc, char **argv) {
    const char *prefix = (argc > 1) ? argv[1] : "/tmp/pad_profile_";
    dsp_init();

    /* Open the combined file (4 profiles back-to-back). */
    char combined_path[512];
    snprintf(combined_path, sizeof combined_path, "%sall.wav", prefix);
    FILE *cf = fopen(combined_path, "wb");
    if (!cf) { fprintf(stderr, "cannot open %s\n", combined_path); return 1; }
    wav_header(cf, (uint32_t)PROFILE_SECS * SR * N_PROFILES);

    printf("Rendering %d profiles × %ds each → %s and per-profile files\n",
           N_PROFILES, PROFILE_SECS, combined_path);

    for (int i = 0; i < N_PROFILES; ++i) {
        char path[512];
        snprintf(path, sizeof path, "%s%d_%s.wav", prefix, i, pad_profile_name(i));
        if (render_profile(i, path, cf) != 0) { fclose(cf); return 1; }
    }

    fclose(cf);
    printf("Done. Combined: %s (%d s total)\n", combined_path, PROFILE_SECS * N_PROFILES);
    return 0;
}
