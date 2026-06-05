# Field Ambience — native C firmware · HÖRTEST-Snapshot

> **Diese Variante (`firmware-c/`) ist eingefroren** für den On-Device-Hörtest.
> Aktive Entwicklung läuft in `../firmware-c-next/` weiter. Wenn dort was bricht,
> ist diese Version der sichere Rückfall.

**Stand:** Steps 1–11 + 12a + DEMO_AUTOPLAY-Schalter. Alle vier Audio-Schichten
(famPadCore + famReverbMaster + famTexture + famSubBass/famDeepBass) im
Engine-Mix-Bus, Harmonic Brain mappt Cell → echte Skalen/Modi-Harmonie.
Offen ist Step 12b (Menü + TRS-MIDI + Encoder-Bindings) — den baue ich in
`../firmware-c-next/`.

## Wofür dieser Ordner gedacht ist

- **Hörtest**: anhand `hoertest/HOERTEST.html` einen Pico mit DAC verkabeln,
  diese Firmware bauen und flashen → das Instrument hörbar machen.
- **Vergleichs-Referenz**: wenn `firmware-c-next/` mal anders klingt, kann
  diese Version gegengehört werden.

## Bauen

```bash
cd field-ambience-current/firmware-c
mkdir -p build && cd build
cmake .. -DPICO_BOARD=pico2
make -j
```

Output: `build/field_ambience_native.uf2` → in BOOTSEL-Mode auf den Pico ziehen.

**Voraussetzungen**: Pico SDK 2.x, `PICO_SDK_PATH` gesetzt, `arm-none-eabi-gcc`,
`cmake ≥ 3.13`. Wenn `pico-examples` baut, baut das hier auch.

## Demo-Modus (für den Hörtest ohne MCP + Tasten)

In `src/main.c` Zeile `#define DEMO_AUTOPLAY 0` auf **`1`** ändern, neu bauen.
Damit spielt die Firmware beim Start selbst eine langsame Akkordfolge — du
brauchst nur Pico + DAC, keine Taster, kein MCP23017. Volle Anleitung in
`hoertest/HOERTEST.html`.

## Host-Tests (kein Gerät, kein SDK)

```bash
cd field-ambience-current/firmware-c
bash test/run_tests.sh
```

6 Suites: dsp+voices, pad, texture, bass, brain, reverb+engine. ~3,4 M
Assertions, sollten alle PASS sein.

## Verkabeln + hören

→ Siehe `hoertest/HOERTEST.html` (Bildanleitung) bzw. `hoertest/HOERTEST.md`
(Textanleitung).

## Was als Nächstes (in `firmware-c-next/`)

Step 12b: per-Mode/Vibe-Reverb-Presets, Drone mit Portamento, PadVoice live
smoothed (warm/strings/brass), Generative-Bed (Markov), Live-Parameter-
Wiring nach der „Sound darf nicht konkurrieren"-Regel, TRS-MIDI Out via
PIO-UART, v30-Menü auf dem OLED, Encoder→Engine-Bindings.
