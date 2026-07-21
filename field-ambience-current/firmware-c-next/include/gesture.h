#ifndef FAM_GESTURE_H
#define FAM_GESTURE_H

/*
 * gesture.c — Gesten-Schleife statt Audio-Looper (r19.25, Bedienlogik
 * Runde 6; HiChord/Orchid haben grosse Audio-Looper — wir bauen bewusst
 * etwas Eigenstaendigeres).
 *
 * Aufgenommen werden CELL-EREIGNISSE (Druck/Loslassen mit Zeitstempel),
 * KEIN Audio. Die Schleife wiederholt die gespielten Zellen mit dem
 * AKTUELL geladenen World + Klang + Zellmodus (Note/Bloom/Generate-Steer)
 * — die Gesten laufen also durch dieselbe Routing-Logik wie das Live-Spiel.
 * Sehr wenig RAM (GESTURE_MAX Events × 6 B ≈ 0,75 KB), kein Schreiben in
 * den PSRAM, kein Echtzeit-kritischer Pfad (control-rate, Main-Loop).
 *
 * Bedienung (r19.25): **SHIFT+HOLD** zykelt
 *   IDLE → REC (aufnehmen) → PLAY (loopen) → IDLE (leeren).
 * (Mischis Vorschlag war SHIFT+GENERATE — das ist seit r19.24 "New Field",
 * daher SHIFT+HOLD.) Die HOLD-LED pulsiert waehrend REC, leuchtet ruhig in
 * PLAY. CLEAR (voll) stoppt + leert die Schleife ebenfalls.
 *
 * Grenze v1: nur Zell-Gesten. Encoder-/World-/Modifier-Aufnahme ist bewusst
 * ausgeklammert — die Schleife spielt ueber das LIVE-Setup (das ist RAM-
 * arm und deckt "wiederholt mit dem aktuell geladenen World und Klang").
 *
 * Hardware-unabhaengig: Wiedergabe ruft einen injizierten Cell-Callback
 * (= dieselbe route_cell-Funktion, die das physische Spiel nutzt).
 */

#include <stdint.h>
#include <stdbool.h>

#define GESTURE_MAX      128
#define GESTURE_MIN_LOOP 500u    /* kuerzeste Schleife (ms) */

typedef enum { GESTURE_IDLE = 0, GESTURE_REC, GESTURE_PLAY } gesture_state_t;

/* Cell-Callback: fuehrt einen Zell-Druck/-Loslass aus (route_cell). */
typedef void (*gesture_cell_fn)(uint8_t cell, bool pressed, uint32_t now_ms);

void gesture_init(gesture_cell_fn on_cell);

/* SHIFT+HOLD: Zustandszyklus IDLE→REC→PLAY→IDLE. */
void gesture_toggle(uint32_t now_ms);

gesture_state_t gesture_state(void);

/* Vom physischen Cell-Handler aufgerufen: nimmt das Ereignis auf, WENN
 * gerade REC laeuft (sonst no-op). Die Wiedergabe ruft dies NICHT (kein
 * Re-Recording). */
void gesture_record_cell(uint8_t cell, bool pressed, uint32_t now_ms);

/* Im Main-Loop: spielt in PLAY die faelligen Ereignisse ueber den Callback. */
void gesture_tick(uint32_t now_ms);

/* Stoppen + leeren (CLEAR / Abbruch). Gibt evtl. offene Play-Stimmen frei. */
void gesture_clear(uint32_t now_ms);

int  gesture_count(void);        /* aufgenommene Ereignisse (Test/UI) */

#endif
