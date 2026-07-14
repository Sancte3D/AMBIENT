#ifndef FAM_OVERLAY_H
#define FAM_OVERLAY_H

/*
 * overlay.c — transient value overlay (r19.21, Bedienlogik Runde 2).
 *
 * DRIVE / BRIGHTNESS / VOLUME veraendern den Klang; bis r19.20 voellig
 * ohne Display-Feedback. Jetzt zeigt jede Knob-Aktion sofort ein kurzes
 * Overlay (Label klein oben, Wert gross — gleiche Typo wie das Menue),
 * danach kehrt das Display von selbst zum vorherigen Zustand zurueck.
 *
 * Hardware-unabhaengig: zeichnet in den oled_draw-Framebuffer, Zeit kommt
 * als now_ms von aussen — host-testbar wie menu.c.
 *
 * Integration (main loop):
 *   - overlay_gen() aendert sich bei jedem overlay_show() → ui_dirty
 *   - overlay_active(now) entscheidet overlay_render() vs menu_render()
 *   - der Aktiv→Inaktiv-Uebergang braucht EIN weiteres Redraw (Menue
 *     wiederherstellen) — der Aufrufer vergleicht overlay_active() mit
 *     dem letzten Frame.
 */

#include <stdint.h>
#include <stdbool.h>

#define OVERLAY_HOLD_MS 1200u   /* Anzeigedauer nach der letzten Aktion */

void overlay_init(void);

/* Zeigt `label` (klein) + `value` (gross) fuer hold_ms ab now_ms.
 * Strings werden kopiert (kurz, gekappt). hold_ms 0 → OVERLAY_HOLD_MS. */
void overlay_show(const char *label, const char *value,
                  uint32_t now_ms, uint16_t hold_ms);

bool     overlay_active(uint32_t now_ms);
uint32_t overlay_gen(void);      /* inkrementiert bei jedem show() */

/* Zeichnet das Overlay in den oled-Framebuffer (oled_fill + Text). */
void overlay_render(void);

#endif
