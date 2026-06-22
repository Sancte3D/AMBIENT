# ADR-0017: Engine World-Refactor — von Inline-Render-Tools zu echten Modulen

**Status:** PROPOSED — Plan dokumentiert, Phase 1 (Worlds-Modul) in r18.48
implementiert.
**Date:** 2026-06-22

## Kontext

Heute (r18.47) hat das Gerät zwar UI-seitig eine *World* (Tokyo / Coast /
Drive / After Hours), aber die *Engine* weiß nicht wirklich, was eine World
ist. Konkret:

- Per-World-Daten (Name, Subtitle, Preset, Akzentfarbe) sind in `menu.c`
  vergraben — vier separate Tabellen (`WORLD_NAMES`, `WORLD_SUBTITLE`,
  `WORLD_PRESET`, `WORLD_ACCENT`), `MENU_WORLD_COUNT=4` in `menu.h`.
- Die Per-World-**Ambience** (Tokyo-Rain, Coast-Waves, Drive-Wind,
  AfterHours-Vinyl) lebt nur **inline** in `tools/render_worlds.c` als
  Render-Tool für A/B-Auditions — nichts davon ist auf dem Gerät hörbar.
- **Tape-Hiss** und **warm-tanh master saturation** leben inline in
  `tools/render_dreamy_warm.c`, ebenfalls nur Audition.
- `engine_set_world(int)` existiert nicht — die `menu_callbacks_t` ruft
  heute nur `set_world(idx)` als no-op weil der HAL-Hook in `main_h743.c`
  noch nicht verdrahtet ist.

Resultat: World-Wechsel ändert auf dem Gerät nur die UI-Akzentfarbe + die
Macro-Defaults. Der Sound bleibt identisch.

## Ziel

Eine World ist ein **erstklassiges Konzept** in der Engine. Wenn der User
zu "Crystal Coast" wechselt, muss das Gerät auch nach Crystal Coast klingen
(Wellen-Ambience, andere Pad-Färbung, andere Reverb-Charakteristik) — nicht
nur anders aussehen.

## Plan (3 Phasen)

### Phase 1 — Worlds-Modul (diese ADR / r18.48)

`worlds.c`/`worlds.h` als **Single Source of Truth** für World-Metadaten.
- `world_t`-Struct mit `name`, `subtitle`, `accent_r/g/b`, `space/tone/atmos`-
  Preset, plus Header-Slots für die noch zu implementierenden Ambience- und
  Drums-Konfigurationen (Phase 2/3).
- `menu.c` zieht die Tabellen von dort statt eigener Kopien.
- `MENU_WORLD_COUNT` → `WORLD_COUNT` (in `worlds.h`), `menu.h` re-exportiert
  für Backward-Compat einen Alias.
- Kein neuer Sound — reine Code-Bewegung; volle Test-Suite bleibt grün.

### Phase 2 — Ambience-Modul (Folge-PR)

`ambience.c`/`ambience.h` als zweiter Render-Layer im Engine-Mix-Bus
(*neben* Pad/Texture/Bass/Drone/Reverb, nicht *statt*).

Lift aus `tools/render_worlds.c`:
- Universeller resonanter **Wind-Generator** (band-passed pink noise mit
  langsamem LFO-Sweep) → für Coast, Drive, AfterHours
- **Regen**-Schicht (noise bursts durch resonanten BP mit exponential decay)
  → für Tokyo
- **Waves**-Schicht (long-period amplitude LFO auf BP-Rauschen) → für Coast
- **Vinyl-Crackle**-Schicht (sparse impulse noise + 1/f filter) → für
  AfterHours

API: `ambience_set_world(world_idx)` + `ambience_render_mix(L, R, frames)`.
Wird im `engine_render` zwischen `texture_render_mix` und `reverb_render`
gestapelt. Level über `engine_set_atmosphere(v)`.

### Phase 3 — Tape-Hiss + Warm-Saturation (Folge-PR)

`hiss.c` + `saturation.c` aus `tools/render_dreamy_warm.c` lifften:
- Hiss als always-on subtler Layer (RMS ~-44 dBFS, mit Highpass bei ~2 kHz
  für "tape" feel)
- Warm-tanh-saturation als letzter Stage im Master vor dem DC-Blocker (heute
  `soft_limit` in engine.c — extend, nicht ersetzen)

Beide hängen am World-Konfig (After Hours = mehr Hiss + mehr warmth; Drive =
weniger).

### Phase 4 — HAL-Wiring (Folge-PR)

`main_h743.c` ruft `menu_callbacks_t` mit echten Engine-Settern. Heute
existiert die Skelettdatei aber bindet die Callbacks nicht. Damit ist der
gesamte World-Pfad endlich Hardware-aktiv.

## Entscheidung (Phase 1 — diese PR)

Worlds-Modul anlegen. Klein. Vier neue Dateien (Header + Source + Test +
Wiring). Bestehende Funktionalität bit-für-bit erhalten — keine
Soundänderung.

## Consequences

**Positiv:**
- World-Metadaten endlich an *einer* Stelle (heute 4× verstreut)
- Ambience/Drums-Erweiterung in Phase 2/3 hat klaren Aufnahmepunkt
- `worlds.h` ist die Datei, die ein neuer Engineer für "wie sieht eine neue
  World aus" liest
- Spätere Persistenz (Flash-gespeicherte User-Worlds) hat einen Container

**Negativ:**
- 1 zusätzliche Header/Source-Pair für die DSP-Sources-Liste
- `menu.c` shrinkt um ~20 Zeilen — vernachlässigbar

**Bewusst nicht in dieser ADR:**
- Save/Load von Worlds (Flash-Persistenz) — wenn jemals nötig, separater ADR
- User-defined Worlds (custom names + presets) — derzeit out of scope; die
  4 curated Worlds sind das Produktversprechen

## Related

- ADR-0014 — Engine V2 (rejected ambient-field) — wird *nicht* das Substrat;
  V1-Pfad bleibt
- `field-ambience-current/firmware-c-next/tools/render_worlds.c` — Quelle
  der zu liftenden Generatoren in Phase 2
- `field-ambience-current/firmware-c-next/tools/render_dreamy_warm.c` —
  Quelle für hiss + saturation in Phase 3
- ADR-0015 — Display-Pivot (entkoppelt; läuft parallel)
