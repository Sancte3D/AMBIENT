# ADR-0014: Engine V2 — Constrained Ambient Field Instrument

**Status:** BACKLOG (geplant, **nicht** umgesetzt) — User-Direktive 2026-06-19
**Date:** 2026-06-19
**Scope:** Komplette neue Sound-Engine + UX-Sprache, **parallel** zu V1.

## Direktive

> „speicher das als to do im backlog. Ich hab pläne für eine engine v2.
> Also separater code. Einfach zum ausprobieren.. nicht überschreiben
> sondern sound v2 erstellen"

→ Engine V1 (`src/engine.c`, `src/pad.c`, `src/texture.c`, `src/bass.c`,
`src/reverb.c`, `src/brain.c`, `src/generative.c`, `src/drone.c`) bleibt **wie
sie ist**. V2 entsteht **daneben** in eigenen Dateien.

V2 kann später per Compile-Flag oder Menü-Toggle aktiviert werden; bis dahin
ist V1 weiter der Default-Pfad in `engine_init() / engine_render()`.

## Kontext

V1 klingt zu „kirchlich" — Amen-Kadenz, Orgel-Pad, Kathedrale.

### Hauptursache

```c
{ 1, 4, -1 }  /* slow_bed default */
```

I→IV→I→IV ist die plagale Kadenz (Church Cadence). In Kombination mit
`musical_mode = 0; /* ionian */`, `musical_space = 0.5f`,
`texture_set_amount(0.0f)`, langem Reverb-Wet-Range (0.40-0.70) und einem
Generative-Bed, das nur den Root spielt, entsteht zwangsläufig Sakralklang.

Aktuelle Engine-Architektur:
```
Chord Brain → Pad Voice → Bass → Texture Noise → Reverb
```
ist „Akkord + Hall". V2 soll „Feld + Bewegung + Material" sein.

## Phase 1 — Quick Fixes (gegen V1, **nicht** Teil dieses ADRs)

Diese 8 Punkte könnten in V1 direkt gefixt werden, **aber nur falls der User
explizit grünes Licht gibt**. Default: V1 bleibt eingefroren, V2 ersetzt
später komplett.

1. `slow_bed` raus als Default → `{ 1, 6, 3, 5, -1 }` (ambient_drift) oder
   `{ 1, 5, 6, 2, -1 }` (floating_minor) oder `{ 1, 3, 6, 4, -1 }` (soft_modal).
2. `musical_mode = 1` (Dorian) statt 0 (Ionian) als Default.
3. Reverb-Wet-Range: `linlin(space, 0,1, 0.22, 0.52)` statt `0.40, 0.70`.
4. `musical_space = 0.32f` Default statt `0.5f`.
5. `texture_set_amount(0.18f)` statt `0.0f` beim Init.
6. Generative-Bed mehrstimmig (3 Stimmen aus `brain_chord()` statt nur Root).
7. Stimmen nicht gleichzeitig wechseln (voice leading statt block change).
8. Bass nicht immer dem niedrigsten Pitch folgen — Gravity-Wahrscheinlichkeit
   (30% bleiben, 40% Root, 20% Quinte, 10% verschwinden).

## Phase 2 — Engine V2 daneben aufbauen

### Ordnerstruktur

```
firmware-c-next/src/v2/
  engine_v2.c            # Mix-Bus + render-Pfad (Pendant zu engine.c)
  harmony_field.c        # Voice-Leading + Consonance Budget
  field_voice.c          # field_voice_t (Glass / Tape / Particle)
  particle.c             # Granular Dust Layer
  material_texture.c     # Air + Dust + Body als drei Layer
  motion.c               # slow1/slow2 + random walk + breath + entropy
  diffuser.c             # Allpass-Kette vor Reverb
  mod_delay.c            # Stereo-Delay mit modulierten Zeiten
  beauty_guard.c         # erzeugt keinen Sound, verhindert hässliche Zustände

firmware-c-next/include/v2/
  engine_v2.h
  harmony_field.h
  field_voice.h
  ... (Header zu jedem .c)
```

### Signalfluss V2

```
HARMONY FIELD
    ↓
FIELD VOICES (8 Stimmen, einzeln gepant/gedriftet)
    ↓
PARTICLE CLOUD
    ↓
MATERIAL TEXTURE (Air + Dust + Body)
    ↓
DIFFUSER (4 Allpass: 7/11/17/23 ms, fb 0.35-0.55)
    ↓
MOD DELAY (L 280-420 ms, R 370-610 ms, fb 0.15-0.35, sehr langsame Wow)
    ↓
REVERB
    ↓
SOFT LIMITER
```

### Harmonic Field statt Akkordfolge

```c
typedef struct {
    float freq;
    float target_freq;
    float amp;
    float pan;
    float drift;
    float age;
    bool  active;
} field_voice_t;
```

Regeln:
- Root bleibt als Gravitationszentrum.
- 3-7 Stimmen, jede mit eigener Wahrscheinlichkeit.
- Nicht jeder Akkord enthält den Root.
- Nicht jeder Wechsel passiert gleichzeitig.
- Stimmen bewegen sich einzeln, nicht als Block.
- Manche Stimmen kommen 2 s später dazu.

### Drei neue Voice-Typen

**Glass Voice**: 2× Sine + 1× Triangle, leichte FM, leiser Octave-Partial,
sehr langsame Pitch Drift. Kein Saw-Block.

**Tape Voice**: bandlimited Saw + Lowpass + Wow/Flutter + leichte Pitch-
Instabilität + Noise-Bleed + Soft Saturation.

**Particle Voice**: kurze Grains, Random Start Phase, Bandpass,
Scale-quantized Pitch, Stereo Scatter, lange Release-Cloud.

### Texture als zweiter Instrumentenkörper

Drei Layer (nicht ein Brown-Noise-Bed):
- **Air**: 3-9 kHz, sehr leise → gibt Raum.
- **Dust**: kurze gefilterte Mikro-Klicks, weich → gibt Bewegung.
- **Body**: 80-250 Hz, dunkler Rumble → gibt Gewicht.

Default-Amount: 0.15-0.25.

### Motion Engine

```c
typedef struct {
    float slow1;
    float slow2;
    float random_walk;
    float breath;
    float entropy;
} motion_state_t;
```

Moduliert: Filter Cutoff, Grain Density, Stereo Width, Reverb Send, Delay
Time, Voice Detune, Partial Amount, Texture Air, Bass Depth.

Wichtig: **keine schnellen LFOs**. Random Walk + Sample-and-Hold mit
Smoothing + sehr langsame Sinuskurven + asymmetrische Drift.

### Voice Leading

Nicht `C → F → C`. Stattdessen:
- Bass bleibt C.
- Eine Stimme wandert G → A.
- Eine andere bleibt D.
- Eine hohe driftet E → F.
- Ein leiser Partialton kommt später dazu.

### Beauty Guard

`beauty_guard.c` erzeugt **keinen Sound**, sondern verhindert schlechte
Zustände:
- Zu viele Stimmen im gleichen Frequenzbereich → eine leiser machen.
- Bass + Pad matschen → Pad unten highpassen.
- Reverb zu dicht → Wet reduzieren.
- Dissonanz zu laut → Spannung absenken.
- Alle Stimmen wechseln gleichzeitig → Wechsel verzögern.
- Sound zu statisch → Motion erhöhen.
- Sound zu nervös → Density reduzieren.
- Clipping → internes Gain reduzieren.

### Consonance Budget

Stabile Intervalle: Root, Quinte, Oktave, None, Sext, Elfte.
Spannungsintervalle: kleine Sekunde, Tritonus, große Septime.

Regeln:
- Immer mindestens 2 stabile Intervalle.
- Maximal 1 starke Spannung gleichzeitig.
- Dissonanz nie im Bass.
- Dissonanz nur leise oder hoch.
- Dissonanz langsam einblenden.

### Tuning Drift (leichte Verstimmung als Lebendigkeit)

- Root: exakt
- Quinte: +2 Cent
- None: -4 Cent
- Sext: +6 Cent
- Oktave: -3 Cent
- Hoher Partial: random walk ±5 Cent

### Bass als Gravity

Nicht „folgt niedrigstem Ton". Stattdessen Wahrscheinlichkeiten pro
Akkordwechsel:
- 30%: Bass bleibt
- 40%: Bass gleitet zum Root
- 20%: Bass gleitet zur Quinte
- 10%: Bass verschwindet

## Phase 3 — Worlds + Makro UX

### Kuratierte Worlds (6-8 statt 100 Presets)

| World        | Charakter                                              |
|--------------|--------------------------------------------------------|
| Glass        | klar, digital, sauber, leise Partials, wenig Dissonanz |
| Warm         | analog, leicht detuned, soft saturation, stabile Tiefe |
| Dust         | luftig, texturiert, nicht sakral, mehr Air & Dust      |
| Fog (Deep)   | dunkel, langsam, bassiger, cinematic, dunkler Reverb   |
| Tape         | instabil, leicht kaputt, melancholisch, Wow + Flutter  |
| Machine      | mechanisch, rhythmisch angedeutet, kein Beat           |

Jede World hat eigene Grenzen: Detune-Range, Dissonanz-Limit, Motion-Speed,
Texture-Verteilung, Reverb-Charakter.

**Gleicher Macro-Regler, anderes Verhalten pro World**:
- Density in Glass = mehr klare Partials.
- Density in Fog = mehr tiefe Stimmen + Rumble.
- Density in Dust = mehr Partikel + Air.

### Macro-UI (alles 0-100)

Sichtbar:
- **World** (Picker)
- **Center** (tonales Zentrum, C/D/E/F/G/A/B — nicht „Key")
- **Density** — Voice-Count + Particle + Layer + Chord-Spread + Texture
- **Motion** — Pitch-Drift + Stereo + Filter + Delay-Mod + Particle-Random
- **Color** — Filter + Brightness-als-Klangfarbe + Partial Balance + Sat
- **Blur** — Diffusion + Reverb + Delay Smear + Attack-Softness + Release
- **Texture** — Air + Dust + Rumble + Noise + Grain
- **Glow** — High Partials + Soft Shimmer + Bright Reverb Send + Octave Dust
- **Volume** (Output Gain, **kein** Engine-Gain)

**Niemals sichtbar**: Oscillator, LFO, ADSR, Scale, Chord, MIDI Note, Delay
Feedback, Reverb Damp, Filter Cutoff, Detune Cents, Ionian/Dorian/Aeolian,
Maj7/Min11/Sus2, I/IV/V/vi.

**Konflikt**: „Brightness" steht aktuell als Encoder-Beschriftung sowohl für
Display-Helligkeit als auch Klangfarbe. In V2: Klangfarbe → **Color**.
Display-Brightness gehört nur in System (Shift + lang).

### Display UX

```
┌────────────────────────────────────┐
│ DEEP FOG             C        03   │  Top Bar (World, Center, Snapshot)
├────────────────────────────────────┤
│        ·      ◌       ○            │
│    ◌        ·      ○        ·      │  Field Visual (funktional, nicht Deko)
│          ·       ◌                │
├────────────────────────────────────┤
│ [DENS 42]  MOT 31   COL 58         │
│  BLUR 46   TEX 22   GLOW 12        │  Macro Area
└────────────────────────────────────┘
```

Field Visual zeigt:
- Punkte = aktive Stimmen
- Größe = Lautstärke
- Position X = Stereo / Pan
- Position Y = Pitch / Höhe
- Bewegung = Motion
- Helligkeit = Density / Glow
- Unschärfe = Blur

### Bedienlogik

| Geste | Aktion |
|---|---|
| Encoder drehen | ausgewählten Wert ändern |
| Encoder drücken | nächsten Wert auswählen |
| Encoder lang drücken | World-Picker öffnen |
| Shift + drehen | Center ändern (Overlay solange Shift hält) |
| Shift + drücken | Snapshot-Overlay |
| Shift + lang drücken | System |

Keine separaten Edit-Modes. Drehen verändert sofort.

### UI States

```
UI_HOME
UI_WORLD_PICKER
UI_CENTER_OVERLAY
UI_SNAPSHOT
UI_SYSTEM
UI_CALIBRATION
UI_ABOUT
UI_ALERT
```

Sound-State und UI-State strikt trennen (nicht alles in `main_h743.c`).

### World Crossfade

Beim World-Wechsel **kein** Hard-Switch — 1.5 s Crossfade zwischen den beiden
Engine-Zuständen.

### Freeze

Performance-Feature: aktuelles Feld halten, generative Wechsel stoppen,
Motion läuft optional weiter, Makros bleiben bedienbar.

Display:
```
DEEP FOG              C      03
             FROZEN
```

### New Field / Seed

Random nicht „Randomize All", sondern **New Field** mit Seed-ID. Jeder gute
Zustand reproduzierbar. Snapshot speichert World + Center + Macros + Seed +
Beauty-Guard-Zustand.

### Output Safety

Speaker- und Headphone-Volume **getrennt** speichern. Beim Einstecken Kopf-
hörer → Volume automatisch sicher reduzieren. Auto / Speaker / Headphones /
Mute als Modus-Auswahl. Bei Clipping kein Tech-Text (`DSP CLIP BUFFER`),
sondern `LEVEL PROTECTED`.

## Konsequenzen

### Positive
- V1 bleibt für Vergleich + Notfall-Fallback erhalten.
- V2 kann ohne Risiko entwickelt werden — Tests + Performance-Renderer
  laufen parallel auf beiden.
- UX-Sprache wird zum Instrument („Field" statt „Synth").
- Beauty Guard schützt Endkunden vor selbst gemachten hässlichen Sounds.

### Negative
- Doppelter Code für eine Weile (V1 + V2). Code-Größe wächst — H7-Flash hat
  2 MB, das ist tragbar.
- V2 muss komplett neu auf Performance gemessen werden (Field Voices +
  Particles + Diffuser + Mod Delay + Reverb können CPU-intensiv werden).
- UX-Switch ist disruptiv: alte Snapshots aus V1 sind nicht V2-kompatibel.
- Macro-Mapping pro World ist Designarbeit (nicht nur Code).

### Risiken
- Particles + Grains auf STM32H7 ohne FPU-spezifische Optimierung könnten
  zu langsam sein. → Phase 2 muss mit Profiling beginnen.
- Beauty Guard kann „korrekte" User-Eingaben übersteuern und so unintuitiv
  wirken. → Brauchbar nur mit guten Defaults + Toggle für „Raw Mode" (intern,
  nicht sichtbar).

## Implementierungs-Reihenfolge

1. **Phase 2 Schritt 1**: `harmony_field.c` + `field_voice.c` + minimaler
   `engine_v2.c`-Renderpfad → erste Klangprobe gegen V1 hörbar machen.
2. **Phase 2 Schritt 2**: `motion.c` + `material_texture.c` ausbauen.
3. **Phase 2 Schritt 3**: `diffuser.c` + `mod_delay.c` vor existierenden
   Reverb hängen.
4. **Phase 2 Schritt 4**: `particle.c` + `beauty_guard.c`.
5. **Phase 3 Schritt 1**: Worlds als Datenstruktur (Macro-Mapping-Tables).
6. **Phase 3 Schritt 2**: Display-Layout V2 (Top Bar + Field Visual + Macro
   Area), neue UI-States, Crossfade-Logik.
7. **Phase 3 Schritt 3**: Snapshot + Seed-System + Output Safety.

## Entwickler-Brief (für spätere Implementation)

> Do not build a random generative synth. Build a **constrained ambient
> field engine** with harmonic guardrails, voice-leading rules, curated
> detune ranges, controlled dissonance, slow modulation, gain-safe
> layering, and a beauty guard that prevents harsh, muddy, overly tonal,
> overly church-like, or chaotic states.
>
> The sound engine may contain many internal parameters, but the device UI
> must expose only macro controls. No oscillator pages. No LFO pages. No
> envelope pages. No scale-degree menus. No music-theory terminology. No
> debug language in the user interface.
>
> The user interacts with curated **Worlds** and macro gestures: World,
> Center, Density, Motion, Color, Blur, Texture, Glow. All deeper synthesis
> parameters are mapped internally per World and protected by musical
> guardrails.
>
> **Complex engine, simple instrument. No deep menus. No exposed synthesis
> parameters. Only curated Worlds and macro gestures.**

## r2 (2026-06-19) — Sound-Rework: „klingt wie Horror" → schön by construction

User-Feedback nach der ersten Klangprobe:
> „v2 klingt allerdings richtig schäbig und wie horror. Null schöne töne.
> Kein appregatio … es klingt wie in einem thriller, dabei sollen es schön
> klingende töne sein. … Ich glaube du hast blind gebaut ohne die sound logik
> zu beachten."

Berechtigt. r1 hatte die Architektur, aber den musikalischen Inhalt nicht
festgezurrt. Die drei Horror-Ursachen + Fixes:

1. **Random Halbton-Cluster.** r1 ließ 8 Voices unabhängig zufällige
   Semitone-Offsets wählen und streute sogar aktiv Spannungsintervalle
   (kleine Sekunde, Tritonus) ein. → **Pentatonik-Lock**: jede Voice ist auf
   eine Major-/Minor-Pentatonik gesperrt. Pentatonik enthält *keine*
   Halbtöne und *keinen* Tritonus → ein harter Cluster ist mathematisch
   unmöglich. Bewiesen im Test: 289.380 Intervall-Checks über 60 s bei
   max Motion + max „dissonance" = **0 Clashes**. Die „dissonance"-Macro
   steuert jetzt nur noch, wie weit obere Voices unter Skalentönen wandern.

2. **Metallische Particle-Pings.** Die Particle-Voice (High-Q-Bandpass auf
   zufällig getriggerten Grains) war reines Thriller-Sounddesign und lief in
   Deep Fog als Default-Sekundärstimme. → **Particle aus den Default-Voice-
   Rollen entfernt.** Pads tragen jetzt Glass + Tape; die Melodie der Arp.

3. **Keine Melodie / „kein appregatio".** → Neuer **`arp.c`**: sanfter
   Pentatonik-Glocken-Arpeggiator (weicher Attack, langer exp. Release,
   leichte Inharmonizität — Felt-Piano / Music-Box / Eno-Bell). Jede Note
   ist ein Skalenton → immer konsonant zum Pad-Feld. Pro World eigenes Tempo
   + Pegel; Glow hebt den Bell-Anteil.

Zusätzlich: Tape-Voice entbuzzt (Saw mit Grund-Sinus gerundet, dunklerer
LP-Ceiling 4.2 kHz statt 6 kHz, sanftere Sättigung). Bells werden NACH dem
Diffuser gerendert, damit sie klar bleiben statt zu verwaschen.

Neue Dateien: `src/v2/arp.c`, `include/v2/arp.h`. Geändert: harmony_field
(Pentatonik-Voicing + Stimmführung in Skalenschritten), worlds (scale_minor +
arp-Felder, Particle raus), field_voice (Tape-Wärme), engine_v2 (Arp-Layer).
Test: +Konsonanz-Test +Arp-Test → 79 Checks grün.

## r3 (2026-06-19) — Richtungswechsel: Crystal-Castles-Energie statt „Kirche"

User nach r2:
> „einfach so dass es geil klingt. geil spielbar. nix kirche. sondern
> entfaltung, wie crystal castles das macht mit deren beats!! ja genau so!!"

Klare Ansage: weg vom reinen Eno-Ambient-Drone, hin zu treibender,
melancholisch-energetischer Elektronik mit Beats. Moll-Pentatonik passt dazu
perfekt (CC ist sehr moll-lastig) — die Konsonanz-Garantie aus r2 bleibt also.
Drei neue Bausteine:

1. **`beat.c` — Synth-Drum-Machine.** Kick (Sinus mit 126→48 Hz Pitch-Drop +
   Click), Snare/Clap (bandpass-Noise + 185 Hz Body), Hi-Hats (highpass-Noise,
   closed/open). Vier 16-Step-Patterns (FOUR / CC-broken / HALF / DRIVE),
   getriggert vom Master-16tel-Clock. Kick bleibt trocken, Snare+Hat-Tails
   gehen in den Reverb-Send.

2. **Master-Tempo-Grid + Tempo-sync Arps.** engine_v2 hat jetzt einen BPM-
   getriebenen 16tel-Clock. Der Arp ist von Free-Run auf grid-locked
   umgestellt (`arp_on_step`, `arp_division`): division 1 = treibende 16tel
   (CRYSTAL), 4 = Viertel (Ambient-Welten). Pro World eigenes BPM
   (76 Ambient … 138 CRYSTAL).

3. **Lo-Fi-Grit.** Drive + Bitcrush (11→5 bit) + Sample&Hold-SR-Reduktion auf
   dem Master, geblendet per `grit` (0 = clean Ambient-Welten, 0.65 CRYSTAL).
   Das ist der körnige CC-Dreck.

World-Set überarbeitet: GLASS/WARM/DEEP FOG bleiben beatlos-ambient; DUST
(four-on-floor), TAPE (gritty half-time) und **CRYSTAL** (ex-MACHINE, voller
broken CC-Beat + 16tel-Arps + Grit, 138 BPM) sind die beat-getriebenen.
„Entfaltung" entsteht über die Macro-Automation (Density/Glow/Grit steigen in
die Climax-Sektion). Demo-Render: Intro −27 dBFS → CRYSTAL-Drop −10 dBFS →
Outro, kein Clipping (Peak 0.79).

Neu: `src/v2/beat.c`, `include/v2/beat.h`. Geändert: arp (Step-Trigger),
worlds (bpm/arp_division/beat/grit + CRYSTAL), engine_v2 (Clock + Beat + Grit).
Test: +Beat-Test (alle 4 Patterns) → 83 Checks grün. Konsonanz weiter 0
Clashes (Pentatonik unverändert).

## Verhältnis zu anderen ADRs

- **ADR-0008 r2** (Cell-LED Independent Latches) → Hold/Shift-Cells bleiben
  als Eingabe in V2; die Voice-Routing-Schicht (`controls.c`) ist
  Engine-agnostisch.
- **ADR-0012** (Encoder-Strategie) → 4 Encoder bleiben; Belegung ändert
  sich (DRIVE/BRIGHT/VOL → Density/Color/Blur etc.).
- **ADR-0004** (MIDI deferred) → V2 spielt erst recht keine Rolle für MIDI;
  bleibt deferred.
- **ADR-0010** (Audio-Buffer + SAI) → Buffer-Größe + Sample-Rate bleiben
  identisch; V2 nutzt denselben SAI-DMA-Pump.
