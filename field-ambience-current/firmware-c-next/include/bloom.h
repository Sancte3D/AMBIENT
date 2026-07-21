#ifndef FAM_BLOOM_H
#define FAM_BLOOM_H

/*
 * bloom.c — Chord-Bloom-Zellmodus (r19.23, Bedienlogik Runde 4;
 * HiChord/Orchid-Prinzip "das staerkste musikalische Element").
 *
 * Die Akkorde sind in brain.c laengst berechnet (brain_chord), wurden aber
 * nie gespielt — die Cells triggern im NOTE-Modus nur den tiefsten Einzelton.
 * BLOOM ergaenzt einen zweiten Cell-Modus:
 *
 *   Cell 1–5 → der harmonisch passende Akkord der 5 Skalenstufen, dessen
 *   Toene langsam nacheinander aufbluehen (~100–170 ms Abstand, gesamt
 *   200–900 ms). Automatisches Voice-Leading legt den neuen Akkord per
 *   Oktav-Verschiebung in die Naehe des vorherigen (kuerzester Weg).
 *
 * BEWUSST MONOPHON-AKKORDISCH (wie HiChord): ein Druck ERSETZT den Akkord
 * (Voice-Leading braucht einen "vorherigen Akkord"; das Voice-Budget —
 * 5 Cell-Sources 0..4, Rest belegt Bett/Shift/Sparkles — traegt keine
 * 5 unabhaengigen Akkorde). HOLD haelt den Akkord ueber das Loslassen
 * hinaus; ohne HOLD klingt er nur solange die Zelle gehalten wird.
 *
 * Chord-Pool = Engine-Sources 0..4 (dieselben wie die Base-Cells; in BLOOM
 * routet das Device NICHT durch controls.c). Hardware-unabhaengig + host-
 * testbar: Zeit kommt als now_ms herein, ruft engine_note_on/off + brain_*.
 * bloom_tick() im Main-Loop feuert die faelligen Einsaetze (control-rate,
 * NICHT im Audio-ISR).
 */

#include <stdint.h>
#include <stdbool.h>

void bloom_init(void);

/* Cell gedrueckt: neuen Akkord der Stufe (cell 0..4 → brain-Degree 1..5)
 * mit Voice-Leading legen und gestaffelt starten. hold = HOLD gelatcht. */
void bloom_press(uint8_t cell, float velocity_amp, bool hold, uint32_t now_ms);

/* Cell losgelassen. Bei momentanem Spiel (kein hold beim Druck) endet der
 * Akkord; ein gelatchter Akkord bleibt stehen. */
void bloom_release(uint8_t cell, uint32_t now_ms);

/* Im Main-Loop aufrufen: startet die faelligen Akkordtoene. */
void bloom_tick(uint32_t now_ms);

/* Alles sofort aus + Zustand zuruecksetzen (Modewechsel / CLEAR). */
void bloom_all_off(void);

/* Observability (Tests/UI). */
/* r19.31 — separate HARMONY bass under the chord (own octave/glide, dry):
 *   OFF   kein Bass · ROOT Grundton (schneller Wechsel) ·
 *   FIFTH Root oder Quinte (kleinster Uebergang) · DRIFT Root, langsames Glide. */
typedef enum { BLOOM_BASS_OFF = 0, BLOOM_BASS_ROOT, BLOOM_BASS_FIFTH,
               BLOOM_BASS_DRIFT, BLOOM_BASS_COUNT } bloom_bass_t;
void bloom_set_bassmode(int mode);
int  bloom_bassmode(void);          /* aktueller Modus (Test/UI) */
int  bloom_cycle_bassmode(void);    /* zum naechsten Modus, gibt den neuen zurueck */

/* r19.32 — Chord-Color (0 PURE / 1 OPEN / 2 WARM / 3 DEEP): Intervallform des
 * Akkords. Wirkt ab dem naechsten Cell-Druck. */
void bloom_set_color(int color);
int  bloom_color(void);

int  bloom_pending(void);        /* noch nicht gestartete Akkordtoene */
int  bloom_active_cell(void);    /* zuletzt gedrueckte Zelle, -1 = keine */
int  bloom_centroid(void);       /* MIDI-Schwerpunkt des akt. Akkords (Test/UI) */
int  bloom_voice_pitch(int i);   /* r19.29: Tonhoehe der Akkordstimme i, -1 leer */
int  bloom_voice_count(void);    /* r19.29: Anzahl klingender Akkordstimmen (≤4) */

#endif
