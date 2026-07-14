/*
 * knobs.c — Encoder-Push-Belegung + Kurz/Lang-Klassifikation.
 * Verhalten + Belegungstabelle: siehe knobs.h.
 */

#include "knobs.h"
#include "params.h"
#include "overlay.h"
#include <stdio.h>

#define ENC_MAX 5   /* IDs 1..4 (params.h), Index 0 unbenutzt */

typedef struct {
    bool     down;
    bool     long_fired;
    uint32_t t_down;
} push_state_t;

static push_state_t     s_push[ENC_MAX];
static knobs_callbacks_t s_cb;

void knobs_init(const knobs_callbacks_t *cb) {
    for (int i = 0; i < ENC_MAX; ++i) {
        s_push[i].down = false;
        s_push[i].long_fired = false;
        s_push[i].t_down = 0;
    }
    if (cb) s_cb = *cb;
    else    { s_cb.display_push = 0; s_cb.display_long = 0; s_cb.status_lines = 0; }
}

/* ---- Overlay-Formatierer -------------------------------------------------- */

static void show_drive(uint32_t now) {
    char v[16];
    if (params_drive_bypassed()) snprintf(v, sizeof v, "BYPASS");
    else                         snprintf(v, sizeof v, "%d", params_drive_pct());
    overlay_show("DRIVE", v, now, 0);
}

static void show_bright(uint32_t now) {
    char v[16];
    int hz = (int)params_bright_hz();
    snprintf(v, sizeof v, "%+d", hz);
    overlay_show("BRIGHT", v, now, 0);
}

static void show_volume(uint32_t now) {
    char v[16];
    if (params_muted()) snprintf(v, sizeof v, "MUTED");
    else                snprintf(v, sizeof v, "%d", params_volume_pct());
    overlay_show("VOLUME", v, now, 0);
}

/* ---- Aktionen -------------------------------------------------------------- */

static void short_action(uint8_t id, uint32_t now) {
    switch (id) {
        case PARAM_ENC_DRIVE:
            params_toggle_drive_bypass();
            show_drive(now);
            break;
        case PARAM_ENC_BRIGHT:
            params_set_bright_neutral();
            show_bright(now);
            break;
        case PARAM_ENC_VOLUME:
            params_toggle_mute();
            show_volume(now);
            break;
        case PARAM_ENC_DISPLAY:
            if (s_cb.display_push) s_cb.display_push();
            break;
        default: break;
    }
}

static void long_action(uint8_t id, uint32_t now) {
    switch (id) {
        case PARAM_ENC_DRIVE:
            params_reset_drive();
            show_drive(now);
            break;
        case PARAM_ENC_BRIGHT:
            params_reset_bright();
            show_bright(now);
            break;
        case PARAM_ENC_VOLUME: {
            char l1[24] = "STATUS", l2[24] = "n/a";
            if (s_cb.status_lines)
                s_cb.status_lines(l1, sizeof l1, l2, sizeof l2);
            overlay_show(l1, l2, now, 2000);   /* Status darf laenger stehen */
            break;
        }
        case PARAM_ENC_DISPLAY:
            /* Reserviert: Scenes-Browser (Runde 3). Bewusst kein Overlay —
             * ein Lang-Druck tut heute einfach nichts. */
            if (s_cb.display_long) s_cb.display_long();
            break;
        default: break;
    }
}

/* ---- Eingaenge ------------------------------------------------------------- */

void knobs_rotate(uint8_t enc_id, int delta, uint32_t now_ms) {
    if (delta == 0) return;
    /* params_encoder hebt Bypass/Mute selbst auf (der Knopf nimmt den
     * Wert) — hier nur weiterreichen und das Overlay nachziehen. */
    params_encoder(enc_id, delta, now_ms);
    switch (enc_id) {
        case PARAM_ENC_DRIVE:  show_drive(now_ms);  break;
        case PARAM_ENC_BRIGHT: show_bright(now_ms); break;
        case PARAM_ENC_VOLUME: show_volume(now_ms); break;
        default: break;
    }
}

void knobs_push(uint8_t enc_id, bool pressed, uint32_t now_ms) {
    if (enc_id >= ENC_MAX) return;
    push_state_t *p = &s_push[enc_id];
    if (pressed) {
        p->down = true;
        p->long_fired = false;
        p->t_down = now_ms;
    } else {
        if (p->down && !p->long_fired &&
            (uint32_t)(now_ms - p->t_down) < KNOBS_LONG_MS)
            short_action(enc_id, now_ms);
        p->down = false;
    }
}

void knobs_tick(uint32_t now_ms) {
    for (uint8_t id = 1; id < ENC_MAX; ++id) {
        push_state_t *p = &s_push[id];
        if (p->down && !p->long_fired &&
            (uint32_t)(now_ms - p->t_down) >= KNOBS_LONG_MS) {
            p->long_fired = true;
            long_action(id, now_ms);
        }
    }
}
