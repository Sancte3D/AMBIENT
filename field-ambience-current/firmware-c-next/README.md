# Field Ambience — native C firmware · ENTWICKLUNG (Step 12b+)

> **Hier wird aktiv weitergebaut.** Der Schwester-Ordner `../firmware-c/` ist
> der eingefrorene Hörtest-Stand (Steps 1–11 + 12a). Wenn hier was kaputt
> geht: dorthin zurückfallen.

**Stand:** identisch zu `firmware-c/` als Ausgangspunkt, plus die Step-12b-
Erweiterungen, die schrittweise dazukommen.

## Step-12b-Plan (Reihenfolge nach „host-testbar zuerst")

| # | Was | Host-testbar? |
|---|---|---|
| ✅ 1 | **Reverb-Presets pro Mode + Vibe-Bias** + Space/Mood-Macros · neues Modul `reverb_presets.{h,c}` + Engine-Setter `engine_set_mode/vibe/space/mood` (alle live-smoothed) | ✅ done |
| ✅ 2 | **Drone mit Portamento-Glide** · neues Modul `drone.{h,c}` + `engine_set_key/drone`. Folgt dem Key live (Glide), bloomt 6 s, friert nicht ein wie die Webapp | ✅ done |
| ✅ 3 | **PadVoice global smoothed** · `pad_set_voice_mix` + `engine_set_pad_voice` (warm/strings/brass). voiceMix ist jetzt globaler, gesmoothter Saw↔Square-Crossfade über alle Voices statt per-Voice gebacken | ✅ done |
| ✅ 4 | **Generative-Bed** · neues Modul `generative.{h,c}` (PROGRESSIONS + Markov DEGREE_TRANSITIONS, seedbarer RNG) + `engine_set_generative/advance`. Bed spielt Akkord-Wurzel pro Step, Cells überschreiben | ✅ done |
| ✅ 5 | **Live-Parameter-Verdrahtung** nach der „nicht konkurrieren"-Regel · alle Engine-Setter (key/mode/vibe/space/mood/pad-voice/drone/generative) inkrementell verdrahtet + Integrationstest: 8 Globals gleichzeitig ändern während eine Cell hält → Voice lebt, Output bounded, kein Klick | ✅ done |
| 6 | TRS-MIDI Out (PIO-UART 31250 Baud auf GP21) | ❌ Hardware-nah |
| 7 | OLED v30-Menü (PLAY/SETUP) | ❌ Hardware-nah |
| 8 | Encoder→Engine-Bindings | ❌ Hardware-nah |

Erstmal bis Schritt 5 host-testbar treiben, dann auf Hardware (Tasten + OLED
+ Encoder + MIDI-Jack) wechseln.

## Architektur-Notiz: Live-Parameter-Regel

Im Plan (`../NATIVE_PORT_PLAN.md`, Step 12b) und im CHANGELOG dokumentiert:
*„Der Sound darf nicht konkurrieren."* → Key/Mode/Vibe/PadVoice-Wechsel
folgen global mit Smoothing; bereits gehaltene Cell-Noten frieren ihre Pitch
bis zum Release ein. Drone folgt dem Key live mit Glide (bewusste Abweichung
von der Webapp-Capture-at-Spawn-Logik).

## Bauen + Testen

Identisch zu `firmware-c/`.

```bash
cd field-ambience-current/firmware-c-next
bash test/run_tests.sh
```

## Was *nicht* hier ist

Die Hörtest-Anleitung (`hoertest/HOERTEST.html` etc.) lebt im Schwester-Ordner
`firmware-c/`, da sie an diesen festen Stand gebunden ist. Wenn `firmware-c-next/`
mal stabil ist und einen eigenen Hörtest verdient, kommt eine aktualisierte
Anleitung dort dazu.
