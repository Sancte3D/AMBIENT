#ifndef FAM_KNOBS_H
#define FAM_KNOBS_H

/*
 * knobs.c — Encoder-Push-Belegung + Kurz/Lang-Klassifikation (r19.21,
 * Bedienlogik Runde 2). Bis r19.20 verarbeitete nur der DISPLAY-Encoder
 * seinen Druck; DRIVE/BRIGHTNESS/VOLUME erzeugten Push-Events, die
 * weggeworfen wurden. Belegung (Orchid-Prinzip — mehr aus denselben
 * Encodern, klanglich logisch):
 *
 *   Encoder    | drehen        | kurz druecken     | lang druecken (500 ms)
 *   -----------+---------------+-------------------+------------------------
 *   DRIVE      | Drive         | Bypass an/aus A-B | Reset auf Default (15)
 *   BRIGHTNESS | Helligkeit    | Neutral (0)       | Reset auf Default (0)
 *   DISPLAY    | Browse/Edit   | Browse/Edit       | reserviert (Scenes, R3)
 *   VOLUME     | Lautstaerke   | Mute/Unmute       | Batterie + Ausgang
 *
 * Jede Aktion zeigt sofort ein Overlay (overlay.c) und das Display kehrt
 * nach OVERLAY_HOLD_MS von selbst zurueck. Drehen an DRIVE im Bypass bzw.
 * VOLUME im Mute hebt den Zustand auf (der Knopf "nimmt" den Wert).
 *
 * Klassifikation: KURZ feuert auf der Loslass-Flanke (< KNOBS_LONG_MS),
 * LANG feuert EINMAL beim Erreichen der Schwelle waehrend des Haltens
 * (knobs_tick) — die Loslass-Flanke danach ist verbraucht.
 *
 * Hardware-unabhaengig: bekommt Flanken + Zeit, ruft params_* / overlay_*
 * und registrierte Callbacks. Host-testbar (test_knobs.c).
 */

#include <stdint.h>
#include <stdbool.h>

#define KNOBS_LONG_MS 500u

typedef struct {
    /* DISPLAY kurz — das bestehende menu_push() (Browse/Edit). */
    void (*display_push)(void);
    /* DISPLAY lang — reserviert fuer Scenes (Runde 3). NULL = no-op. */
    void (*display_long)(void);
    /* VOLUME lang — Statuszeilen fuellen (Batterie/USB + Ausgang).
     * NULL → generisches "STATUS n/a". */
    void (*status_lines)(char *l1, unsigned n1, char *l2, unsigned n2);
} knobs_callbacks_t;

void knobs_init(const knobs_callbacks_t *cb);

/* Rotationsereignis fuer DRIVE/BRIGHT/VOLUME: params + Overlay.
 * (DISPLAY-Rotation bleibt beim Menue — nicht hierher routen.) */
void knobs_rotate(uint8_t enc_id, int delta, uint32_t now_ms);

/* Push-Flanke jedes Encoders (pressed = down/up). */
void knobs_push(uint8_t enc_id, bool pressed, uint32_t now_ms);

/* Im Main-Loop aufrufen: feuert Lang-Druck beim Schwellen-Erreichen. */
void knobs_tick(uint32_t now_ms);

#endif
