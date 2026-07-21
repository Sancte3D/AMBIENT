#ifndef FAM_SCENES_H
#define FAM_SCENES_H

/*
 * scenes.c — 5 Scene-Slots ueber die vorhandenen Cells (r19.22,
 * Bedienlogik Runde 3; HiChord/Orchid-Prinzip "speicherbare Zustaende").
 *
 * Bedienung:
 *   DISPLAY lang (im BROWSE)  → Scenes-Modus oeffnen (knobs.c routet;
 *                               im EDIT toggelt derselbe Lang-Druck den
 *                               Parameter-Lock, menu.c)
 *   Cell 1–5 kurz             → Scene laden
 *   SHIFT + Cell 1–5          → aktuellen Zustand in den Slot speichern
 *   DISPLAY kurz / 8 s idle   → Scenes-Modus verlassen
 *   Cell-LED gelb             → Slot belegt · gruen → aktive Scene
 *
 * Gespeichert: World, Key, Tuning, Voice, Synth, alle 7 Makros,
 * Parameter-Locks, Drive, Brightness, Generator-Seed.
 * NICHT gespeichert: gehaltene Noten, Modi (Drone/Generate), Volume
 * (Lautstaerke springt beim Recall nie).
 *
 * Persistenz ueber injizierte Backends (Device: interner STM32-Flash-
 * Sektor; Host-Test: RAM-Puffer). Das Modul selbst ist hardware-frei.
 */

#include <stdint.h>
#include <stdbool.h>

#define SCENES_COUNT     5
#define SCENES_UI_IDLE_MS 8000u

/* Persistenz-Backend: blob ist das komplette gepackte Scene-Array.
 * write gibt false zurueck, wenn der Store nicht schreibbar ist. */
typedef bool (*scenes_write_fn)(const void *blob, unsigned len);
typedef bool (*scenes_read_fn)(void *blob, unsigned len);

/* Init + Laden aus dem Backend (beide Fn duerfen NULL sein → RAM-only). */
void scenes_init(scenes_write_fn write_fn, scenes_read_fn read_fn);

/* Speichern/Laden. Rueckgabe false = Slot ungueltig/leer. save persistiert
 * sofort ueber das Backend. */
bool scenes_save(int slot, uint32_t now_ms);
bool scenes_recall(int slot, uint32_t now_ms);

bool scenes_used(int slot);
int  scenes_active(void);        /* zuletzt geladener/gespeicherter Slot, -1 */

/* --- Scenes-UI-Modus ---------------------------------------------------- */
void scenes_ui_open(uint32_t now_ms);
void scenes_ui_close(void);
bool scenes_ui_active(void);
void scenes_ui_tick(uint32_t now_ms);        /* Idle-Timeout */
/* Cell-Ereignis im Scenes-Modus (shift = speichern statt laden). */
void scenes_ui_cell(uint8_t cell, bool shift, uint32_t now_ms);
/* Zeichnet den Scenes-Screen in den oled-Framebuffer. */
void scenes_ui_render(void);

#endif
