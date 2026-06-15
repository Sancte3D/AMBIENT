# NATIVE PORT PLAN — Pi-frei auf den RP2350

> **Ziel**: die Audio-Engine + UI nativ in C auf dem Pico 2 (RP2350) laufen
> lassen, sodass der Raspberry Pi Zero 2 W komplett aus dem Gerät rausfällt.
> Eine UF2-Datei per BOOTSEL flashen, fertig.
>
> Status: **Steps 1–11 + 12a done** (Engine-Steps hörbarkeits-first gebaut:
> 9 → 11 → 10 → 8 → 12). Alle vier Audio-Schichten (famPadCore + famReverbMaster
> + famTexture + famSubBass/famDeepBass) laufen im Engine-Mix-Bus; der
> **Harmonic Brain (12a)** mappt Cells auf echte Skalen/Modi/Familien-Harmonie.
> Cell-Tap blooms zu Pad + Reverb-Fahne über Noise-Bed, mit zwei-Schicht-Bass.
>
> **Repo-Layout ab 2026-06-03:** der Step-12a-Stand ist als Hörtest-Snapshot in
> `firmware-c/` eingefroren (inkl. `firmware-c/hoertest/`-Anleitung). Aktive
> Entwicklung von Step 12b läuft parallel im neuen `firmware-c-next/`-Ordner —
> sicheres Zurückfallen jederzeit möglich.

---

## Was nach dem Port wegfällt

Wenn die Engine auf dem RP2350 läuft, wird die Hardware **echt schlanker**.
Das sind die Teile, die aus der Stückliste verschwinden:

| Ref | Was | Warum weg |
|---|---|---|
| **Pi Zero 2 W** | Linux-Audio-Host | Nicht mehr nötig — RP2350 macht alles |
| **J2** | 40-pin 2×20 GPIO-Header | War nur dafür da, den Pi aufzustecken |
| **R1** | 1 kΩ UART-RX-Serien-R | UART zum Pi entfällt |
| **R_BCK · R_LRCK · R_DOUT** | 3× 33 Ω I²S-Serien-R auf Pi-Side | Neue I²S kommt vom Pico, andere Trace-Längen |

> **Korrektur (Step 6):** D2 (SMAJ5.0A TVS) bleibt drin. Sie sitzt im
> Power-Tree auf der **+5V-Hauptschiene**, nicht am Pi-Header — ihre
> ursprüngliche „Pi-WLAN-Spike"-Begründung entfällt zwar, aber als
> allgemeiner Rail-Surge-Schutz bleibt sie sinnvoll. Real entfernt werden
> **5 Bauteile**: J2, R1, R_BCK, R_LRCK, R_DOUT (+ das Pi-Modul, das auf J2 saß).

Plus **bedeutsame Sekundär-Effekte**:

| Punkt | Heute | Nach dem Port |
|---|---|---|
| Power-Budget (Worst-Case) | 2,45 A | ~1,75 A (700 mA Pi raus) |
| Sicherung F1 | 3 A hold / 6 A trip | überdimensioniert, kann später auf 2 A/4 A runter (optional) |
| C_BULK | 1000 µF (war für Pi-Boot + Bass-Transienten) | nur noch Bass-Transienten → 470 µF würden reichen, aber 1000 µF schadet nicht |
| Boot-Zeit | „Pi booten + SC laden" (10–20 s) | „Pico powerup" (<1 s) |
| Update-Mechanismus | WLAN/SSH zum Pi | USB-C einstecken, BOOTSEL drücken, UF2 droppen |
| Determinismus | Linux-Scheduling, mögliches X-Run | Echtzeit-DMA, sample-genau |
| Stromaufnahme idle | ~440 mA | <100 mA |

---

## Was bleibt (kein Hardware-Redesign jenseits der Pi-Sektion)

Alle anderen 90+ Bauteile bleiben. Insbesondere:
- PCM5102A DAC (jetzt vom Pico per I²S gespeist statt vom Pi)
- PAM8403 Amp + Speaker + Line-Out + Jack-Detect
- OLED + 4× Encoder + 10× Choc-Switches + MCP23017
- USB-C + F1 + ESD + Decoupling
- Alle Pico-seitigen Pull-Ups, Pull-Downs, Debounce-Caps

---

## Pico-Pin-Belegung — die Änderungen

Aktuell (SPEC §5) sind alle 24 GPIOs belegt. Wenn der Pi rausfällt, werden
GP0/GP1 frei (kein UART zum Pi mehr) und GP4 ist eh schon ungenutzt
(OLED_MISO_NC, reserviert). Diese drei werden zu **I²S Out**.

| Pico Pin | GP | Alt (heute) | Neu (Pi-frei) |
|---|---|---|---|
| 1 | GP0 | UART0 TX → Pi RX | **I²S BCK → PCM5102A pin 13** |
| 2 | GP1 | UART0 RX ← Pi TX | **I²S LRCK → PCM5102A pin 15** |
| 6 | GP4 | SPI0 RX (OLED MISO, unused) | **I²S DIN → PCM5102A pin 14** |

Kein GPIO-Engpass. I²S geht über das PIO-Subsystem (RP2350 hat 3 PIO-Blöcke
zu je 4 State-Machines, mehr als genug).

**Wichtig — PCM5102A 3-wire-Mode**: SCK (pin 12) → GND, FMT (pin 16) → GND.
Kein MCLK nötig, der DAC erzeugt sich seine interne Clock aus BCK. Steht
schon so in SPEC §8 → keine Änderung nötig.

---

## Die 12 Steps

Jeder Step ist ein eigener kleiner PR, damit du jederzeit abbrechen oder
umsteuern kannst. Keine Mega-Dumps.

- **Step 1** ✅ — Plan-Doku + nackte C-Firmware-Hülle (CMake +
  blinkende LED). Baut zu einer UF2. Keine Audio, kein UI, nur Lebenszeichen.
- **Step 2** ✅ — OLED-Treiber portiert (SSD1322 256×64 via SPI0, identische
  Init-Sequenz wie die MicroPython-Variante). Statischer Banner-Text
  *„FIELD AMBIENCE / V0.9 STEP 2"*. Starter-Font wächst mit jedem nächsten Step.
- **Step 3** ✅ — I²C1 @ 400 kHz, MCP23017 @ 0x20, byte-für-byte
  identische Init zur MicroPython-Variante. GP22 falling-edge IRQ für INTA.
  Live-State-Render auf OLED (5 Zellen + 5 Modifier + Jack-Detect), USB-CDC-
  Trace bei jedem Tap. XSMT-Pin als Output verfügbar für Step 5.
- **Step 4** ✅ — 4× EC11 (DRIVE/BRIGHT/DISPLAY/VOLUME) auf
  GP10–GP21 quadrature-dekodiert per 1 kHz Repeating-Timer + 4-State-Tabelle
  + 4 Sub-Steps pro Detent (identisch zur Python-Logik). Push-Switches
  per 3-of-N-Bounce-Filter. Lock-free Ring-Buffer für Events → kein I/O im
  Timer-Callback. OLED zeigt Position + Push-State pro Encoder.
- **Step 5** ✅ — **Erster Sound.** PIO0-SM0 mit dem
  pico-extras-`audio_i2s`-Programm (copy mit BSD-Attribution), DMA-
  Ping-Pong (2 × 256 Frames) in den PIO-TX-FIFO bei 44,1 kHz / 16-Bit-Stereo.
  Pins: BCK=GP0, LRCK=GP1, DIN=GP4 (die freigewordenen UART-/MISO-Pins).
  Pop-suppressed Power-Sequenz nach SPEC §8 (Rails → /SHDN → 50 ms →
  /MUTE+XSMT, alles während die I²S schon Stille pumpt). Continuous
  440-Hz-Sinus @ -20 dBFS als „it works". **Ab hier ist der Pi funktional
  redundant** — der Audio-Pfad geht Pico → DAC → Amp ohne Linux.
- **Step 6** ✅ — Schaltplan Pi-frei: `pi.kicad_sch` gelöscht,
  J2/R1/R_BCK/R_LRCK/R_DOUT raus (5 Bauteile, real 97→92), GP0/GP1/GP4 im
  Pico-Sheet auf I²S_BCK/LRCK/DOUT umverdrahtet, Root-Sheet brückt Pico-I²S →
  Audio-Sheet, kicad_pro-Sheetliste bereinigt. Analyzer: 6 VM-001-Blocker —
  **alle die dokumentierte False-Positive-Klasse** (Heuristik taggt den Pico
  wegen VBUS-Pin als 5V-Domain; real sind GP0/1/4 @ 3V3 → PCM5102A @ 3V3,
  kein Level-Shifter nötig). Warnings sogar von 19 → 16 gesunken (Pi-Nets weg).
  GUI-ERC (B3) bleibt die maßgebliche Instanz. **Erstes BOM-Schrumpfen.**
- **Step 7** ✅ — `dsp.{h,c}` (1024-Punkt-Sinus-LUT mit
  Interpolation + midi→Hz) als Basis für alle folgenden Steps. `voices.{h,c}`:
  8-stimmiger Voice-Pool, Sinus + klick-freie ASR-Hüllkurve (per-Sample-Rampen),
  Voice-Stealing nach leisester Stimme. `audio.c` bekommt einen pluggable
  Renderer (`audio_set_renderer`) statt hartem Test-Sinus. Cell-Tap → note_on,
  Release → note_off; Cell→Pitch vorerst C-Moll-Pentatonik (Platzhalter bis
  Harmonic Brain in Step 12). OLED zeigt aktive Stimmenzahl.

> **Reihenfolge-Entscheidung (2026-06-02):** Die Engine-Steps 8–12 werden
> **nicht** in numerischer Reihenfolge gebaut, sondern **hörbarkeits-first**:
> **9 → 11 → 10 → 8 → 12**. Grund: der Pad (Step 9) ersetzt den Platzhalter-
> Sinus → größter hörbarer Sprung; Reverb (Step 11) gibt ihm den Raum. Der
> Bass (Step 8) kommt zuletzt, weil der 380-Hz-Onboard-Treiber (siehe SPEC §8
> Acoustic-Refactor) famSubBass/famDeepBass ohnehin kaum abstrahlt — das ist
> primär ein Line-Out-Layer und onboard nur über den Reverb-Send hörbar, der
> erst ab Step 11 existiert. Port-Vorlage: `../software/webapp/field_ambience_webapp.html`
> (imperativer Web-Audio-DSP, konkrete Hz/Q/Gain-Werte), `../software/supercollider_reference/field_ambience_v29o.scd` als Cross-Check.

- **Step 9** ✅ — `famPadCore`. Ported aus `_makePadVoice` der Webapp.
  Neue DSP-Primitive in `dsp.{h,c}`: polyBLEP-Saw/Square (`dsp_poly_saw/square`,
  anti-aliased), TPT-State-Variable-Filter (`dsp_svf_*`, Cytomic-Form — stabil
  unter laufender Cutoff-Modulation, kein Zipper) und ein One-Pole-Smoother
  (`dsp_smooth_coef`). Neues Modul `pad.{h,c}`: pro Stimme 2 verstimmte Seiten
  × (3 Saw + 2 Square), je eigener resonanter LP, Cutoff von LFO + Filter-ADSR
  + Brightness-Offset geschwenkt (Coeff-Update auf Control-Rate SR/16), Haas-
  Mikrodelay (8/14 ms) + Gegen-Pan → echtes Stereo, eine Bloom/Decay-Amp-
  Hülle. Drop-in zum Step-7-Voice-API: `main.c` registriert `pad_render` statt
  `voices_render`, Cell-Tap → `pad_note_on/off`. Host-Tests (`test/test_pad.c`):
  polyBLEP bounded, SVF Pass/Stop (−41 dB @ 8×Cutoff), Bloom click-frei, echte
  L≠R-Dekorrelation, Drain auf Stille, Pool-Bounded + Soft-Clip unter Volllast.
  `voices.c` bleibt kompiliert + getestet (Step-7-Artefakt), aber nicht mehr
  live. Cell→Pitch weiter Platzhalter-Pentatonik bis Step 12.
  *Hinweis: host-getestet; UF2-Build/On-Device-Audio braucht Pico-SDK-Hardware.*
- **Step 11** ✅ — `famReverbMaster` + Engine-Mix-Bus. Ported aus der
  Web-Audio-`createConvolver`-Variante zu einem **Freeverb-Stil-Algorithmic**
  (8 parallele Feedback-Combs + 4 serielle Allpässe pro Kanal, klassische
  Freeverb-Tunings 1116/1188/.../1617 @ 44.1 kHz, +23-Sample-Stereo-Spread,
  pre-Reverb tanh-Drive nach `driven = tanh(in·(1+d·4)) / (1+d·0.6)`).
  Convolution scheidet auf RP2350 aus (RAM + Cycles).
  Neues Modul `reverb.{h,c}` — block-orientiertes `reverb_render(in_L,
  in_R, out_L, out_R, n)`, smooth-coefficient-updates pro Block (~120 ms
  Time-Constant, kein Zipper). Neues Modul `engine.{h,c}` als Mix-Bus-Owner
  und neuer Live-Renderer: pro Block werden Dry-/Send-Float-Akkumulatoren
  geleert, `pad_render_mix` schreibt Dry + Send (skaliert mit `send_amount`),
  `reverb_render` rechnet Send→Wet, dann Sum + Master-tanh-Soft-Clip → int16.
  `pad.{h,c}` refactored: inner Per-Sample-Loop in `render_block_float`
  extrahiert; alte `pad_render(int16_t*)` bleibt als Standalone-Wrapper
  erhalten (Tests grün), neue `pad_render_mix(float*, …)` für den Engine-
  Bus. `main.c` registriert jetzt `engine_render` und ruft `engine_note_on/
  off`. Defaults am Boot: size=0.7, damp=0.3, drive=0.15, wet=0.40, send=0.45
  (entspricht dem Webapp-Mid-Mode bei space≈0.5/mood≈0.5).
  Host-Tests `test/test_reverb_engine.c`: Impuls dekayt sauber (Peak 0.037,
  Tail nach 4 s = 0.0), Silent-In→exakt-Silent-Out (kein DC, keine Selbst-
  oszillation), 5 s Noise@1.0 bleibt unter Peak 2.0, Engine-Boot silent,
  Pad-Note hörbar mit Stereo (L≠R), Reverb-Tail überlebt das Dry-Release
  (3.5-4.5 s nach release noch ~880-1130 LSB), unter 15 s Stille schließlich
  Decay auf <200 LSB, 8-Voice-Volllast bleibt int16-bounded (Peak ~32740).
  RAM-Budget: 8×2 Combs × 1640 + 4×2 Allpässe × 580 + Engine-Scratch ≈ 65 KB.
  v0.8-Reverb-Reconciliation (`computeReverb` mit space/mood-Multipliern)
  ist als API-Surface vorbereitet (Engine-Setter), aber das Mapping space→
  size+wet kommt zusammen mit Encoder-Bindings/Brain in Step 12.
  *Hinweis: host-getestet; UF2-Build + On-Device-Hörtest brauchen Pico-SDK-
  Hardware.*
- **Step 10** ✅ — `famTexture`. Ported aus `ensureTexture`. Dauer-Bed (kein
  per-Note): pro Kanal Brown-Noise (leaky-integrator-White, identisch zum
  Webapp-`_buildNoiseBuffer`) → **Rumble** (LP 220 Hz, gain 0.35) + **Breath**
  (BP 600 Hz Q1.6, Mitte von 0.052-Hz-Sinus ±260 Hz gesweept, Amp von 0.04-Hz-
  Sinus pulsiert 0.5±0.22) → Mix → Warm-LP 4500 Hz → langsamer Amp → dry +
  Reverb-Send (0.55). Neue DSP-Primitive: **`dsp_svf_bp`** (Bandpass-Ausgang
  des Cytomic-SVF, k-normalisiert auf ≈unity-Peak — k jetzt im svf-Struct).
  Neues Modul `texture.{h,c}`: `texture_render_mix(dry, send, …)` ADDed in die
  Engine-Buses, Control-Rate-LFOs (SR/16), Early-Out bei amount≈0. **Stereo-
  Verbesserung ggü. dem mono Webapp-Bed**: L/R laufen unabhängige Noise-
  Streams (geteilte LFOs/Coeffs) → breites dekorreliertes Bett.
  Engine: `engine_set_texture(amount)`, eigener Texture-Send 0.55 (vs Pad 0.45).
  **Boot bei amount 0** (silent power-up, SPEC §8); `main.c` blendet nach der
  Un-Mute-Sequenz 0.20 ein → sanfter ~2-s-Glide, pop-frei, Gerät idlet als
  Ambience. Host-Tests `test/test_texture.c`: BP-Shape (centre 0.707 = unity,
  −15 dB Oktave drunter, −22 dB drüber), silent-at-0, Bloom + Stereo-
  Dekorrelation (avg|L-R|≈0.006), über 30 s bounded (kein DC-Runaway).
  *Hinweis: host-getestet; UF2-Build + On-Device-Hörtest brauchen Pico-SDK-
  Hardware.*
- **Step 8** ✅ — `famSubBass` + `famDeepBass`. Ported aus `_makeSubBass` /
  `_makeDeepBass`. Zwei mono Bass-Voices an der Akkord-Wurzel:
  **Sub** = Sinus + Tri\@2× (0.08) → LP 90 Hz, 3-s-Bloom, 0.04-Hz-Breath-LFO
  auf der Amplitude, 2 Oktaven unter der Wurzel, Reverb-Send 0.03.
  **Deep** = Sinus + Tri\@2× (0.06) → tanh-Saturation → HP 50 → LP 350 (Q 1.8),
  2.5-s-Bloom, 1 Oktave unter der Wurzel, Reverb-Send 0.08.
  Neue DSP-Primitive: **`dsp_svf_hp`** (Highpass-Ausgang des Cytomic-SVF) +
  **`dsp_tri`** (naiver Dreieck, Harmonische ~1/n² → kein Aliasing nötig bei
  Bass-Frequenzen). Neues Modul `bass.{h,c}`: beide Voices folgen der
  **tiefsten gehaltenen Note** — Engine trackt aktive Cell-Frequenzen
  (`active_freq[16]`, `lowest_held()`), `bass_note(lowest)` glided die Tonhöhe
  legato (Portamento) solange gehalten und bloomt nur aus dem Idle bei der
  ersten Note; `bass_release` bei allen-Noten-los. Mono (nicht-direktional →
  gleich auf L/R). `engine_set_bass_depth` (default 0.5 → sub-amp 0.13,
  deep-amp 0.10). Per-Layer-Sends intern im Modul (0.03/0.08, anders als der
  globale Pad-Send). Host-Tests `test/test_bass.c`: dsp_tri-Form + Periodizität,
  HP-Shape (pass 500 Hz = unity, −25 dB @ 12 Hz), Idle-silent, Bloom + Mono,
  **Legato-Glide ohne Re-Trigger** bei Wurzelwechsel, Drain auf Stille (exp-
  Tail bis −60-dB-Idle-Cutoff ≈ 7 s), über 20 s bounded.
  **Onboard-Realität** (380-Hz-Treiber, SPEC §8 r14): famSubBass (LP 90) liegt
  komplett unter dem Treiber, untere Hälfte von famDeepBass auch — onboard
  primär über den Reverb-Send (Wärme der Fahne) + Line-Out hörbar. Layer
  trägt trotzdem zum Gesamtmix + externem Monitoring bei.
  *Hinweis: host-getestet; UF2-Build + On-Device-Hörtest brauchen Pico-SDK-
  Hardware.*
- **Step 12a** ✅ — Harmonic-Brain (Cell→Pitch). Ported aus den Webapp-
  Harmonie-Funktionen: `SCALES` (6 Modi), `CHORD_FAMILIES` (über VIBE_FAMILY
  erreichbar: add9/maj7/min11/sus2), `chordAtDegree` + `voiceCentered`
  (Oktav-Shift sodass der Akkord-Mittelwert nahe MIDI 64 liegt). Neues Modul
  `brain.{h,c}`: reine Integer-Theorie, kein Audio. `brain_cell_root(cell)`
  liefert die Tonhöhe für einen Cell-Tap = tiefste Note des center-voiced
  Akkords der Skalenstufe (exakt wie Webapp-`cellOn`). State `brain_set_key/
  mode/vibe` (default C4 ionian warm/add9) für die 12b-Encoder-Bindings.
  `main.c` ersetzt die Platzhalter-Pentatonik durch `brain_cell_root`.
  Host-Tests `test/test_brain.c`: gegen handberechnete Referenzwerte (ionian I
  add9 = [60 64 67 74], V = [55 59 62 69], aeolian-Moll-Terz, min11/sus2-
  Familien), Pitch-Class-vs-Oktave-Verhalten von voiceCentered, alle 6 Modi ×
  4 Vibes × 7 Stufen zentriert (Mean 58–70) + in MIDI-Range.
- **Step 12b** — *offen, hardware-nah*: v30-Menü (PLAY/SETUP), **MIDI Out
  via TRS Type A (3.5-mm-Klinke)** auf PIO-UART (r15-Entscheidung — *kein*
  USB-MIDI/TinyUSB), Encoder→Engine-Param-Bindings (DRIVE→reverb-drive,
  BRIGHT→brightness, VOLUME→master, DISPLAY→Menü/Key/Mode), opt-in
  Generative-Bed (PROGRESSIONS + DEGREE_TRANSITIONS-Markov) + Drone. Doku
  finalisieren. Diese Teile brauchen On-Device-Hörtests + UX-Entscheidungen
  (Menü-Struktur, MIDI-Mapping) und sind in der host-only-Umgebung weder
  voll testbar noch sinnvoll ohne User-Input festlegbar.

  **MIDI-Architektur (r15, 2026-06)**: PIO-State-Machine auf PIO1/PIO2
  (PIO0 macht I²S), 31250 Baud 8N1, sendet auf GP21 → 220 Ω → TRS-Tip;
  +3V3 → 220 Ω → TRS-Ring; GND → Sleeve. Pegel 3,3 V, MMA-Spec-Update
  CA-033 (2020) explizit erlaubt. Kein Optokoppler nötig (nur am IN; und
  MIDI IN kommt nicht — siehe SPEC §8 r15-Begründung).
  Note-On/Off-Builder mit Channel + Velocity-Mapping; sendet die Akkord-
  Töne, die der Harmonic Brain pro Cell-Tap erzeugt → Gerät wird zum
  „denkenden Controller" für externe Synths/DAWs.

  **Design-Regel für Live-Parameter-Wechsel (2026-06, vom User festgelegt):**
  „Der Sound darf nicht konkurrieren." → Klarer Schnitt zwischen *globaler
  Klanglandschaft* und *einzelner getriggerter Note*:

  | Wechsel | Folgt live (smooth) | Erst neue Noten |
  |---|---|---|
  | **Key** | Drone (Glide ~1–2 s wie der Bass) · Generate (nächster Akkord) | Pitch schon gehaltener Cells |
  | **Mode** | Reverb-Preset (per-Mode t60/damp/size/high) · Generate-Stufe | Akkord-Töne schon gehaltener Cells |
  | **Vibe** | Reverb-Bias · Generate-Familie | Akkord-Familie schon gehaltener Cells |
  | **PadVoice (warm/strings/brass)** | **alle Voices gleiten gemeinsam** ins neue Timbre (globaler smooth-Crossfade des voiceMix) | – |

  Begründungen, damit's nicht später hinterfragt wird:
  - **Drone live statt capture-at-spawn (Abweichung vom Webapp)**: ein
    eingefrorener Drone-Grundton auf C, während neue Cells in D spielen,
    arbeitet harmonisch direkt gegen den neuen Key — schlimmste Konkurrenz.
    Glide löst es; Drone-Voice bekommt Portamento analog zum Bass.
  - **PadVoice global statt nur-neue-Voices**: sonst hörst du gleichzeitig
    die alte warme Saw-Stimme und die neue Brass-Stimme nebeneinander → das
    *ist* Konkurrenz. Gemeinsamer Crossfade bewegt den Klang als Einheit.
    voiceMix wird im Pad zum gesmooth-globalen Live-Parameter (statt am
    Note-On gebackener Per-Voice-Konstante).
  - **Cell-Pitch *nicht* live nachpitchen**: eine Cell ist ein expliziter
    User-Touch; deren Tonhöhe live umzupitchen wäre Eingriff in die vom
    Spieler getroffene Note. Stattdessen verklingt sie sanft im 3-s-Release,
    während der Drone (Fundament) sich schon angepasst hat → die alte Note
    „schmilzt" musikalisch in die neue Tonart.
  - **Reverb live (per-Mode/Vibe)**: der Hall ist die gemeinsame Atmosphäre,
    sein Wechsel ist eine Stimmungs-Umschaltung. Reverb hat Coefficient-
    Smoothing intern → kein Sprung.

Nach Step 12 ist die alte MicroPython-Firmware obsolet — wird nach
Sign-off entweder gelöscht oder als `firmware-mpy-legacy/` archiviert.

---

## Performance-Sanity-Check (warum das überhaupt geht)

RP2350: dual Cortex-M33 @ 150 MHz, FPU für single-precision, 520 KB SRAM.

| Layer | Grobe Schätzung CPU/Core |
|---|---|
| 5× Pad-Voice (3 Saws + 2 Pulse + 2-Pol-LPF + ADSR + LFO + Haas-Delay) | ~12 % |
| 1× famSubBass + 1× famDeepBass | ~3 % |
| 1× famTexture (2 BPF + LPF + 2 LFOs) | ~4 % |
| Algorithmischer Reverb (8 Comb + 4 Allpass + DC-Block + Limiter) | ~8 % |
| Master + I²S-DMA + Hilfsmath | ~3 % |
| **Audio total auf Core 0** | **~30 %** |
| OLED + Encoder-Scan + MCP23017-IRQ + USB-MIDI | **~5 % auf Core 1** |

Sample-Buffer: 256 × 2ch × 4 byte = 2 KB pro Block, 2 Blöcke = 4 KB. Reverb-
Delay-Lines (alle 12 Delays bei ≤ 50 ms) = ~100 KB. Insgesamt ~150–200 KB
SRAM-Nutzung. Bleibt **viel** Spielraum.

Realistisch ist sogar **8 Pad-Stimmen** drin, falls man später mehr Polyphonie
will. 5 reichen aber per Sound-Constitution.

---

## Risiken / offene Fragen, die ich beobachte

- **Convolution-Reverb → Freeverb-Klasse**. Die Web-Audio-Version nutzt
  `createConvolver()` mit einer generierten Impulsantwort. Das geht auf
  dem RP2350 nicht (zu wenig RAM, zu wenig Cycles). Stattdessen ein
  Schroeder/Freeverb-Stil-Algorithmic. Klingt nicht *identisch*, aber sehr
  nah — und beide implementieren die Sound Constitution. Tuning-Phase
  in Step 11.
- **PIO-I²S-Reliability**. Standard pico-extras `audio_i2s` lib funktioniert
  zuverlässig, müssen wir aber an unsere Buffer-Größen anpassen.
- **USB-MIDI auf RP2350 mit TinyUSB**. Bewährter Pfad, sollte sauber laufen.
- **Update-Workflow**: BOOTSEL+drag-drop ersetzt SSH zum Pi. Für den User
  Workflow simpler, aber bei einem ausgelieferten Gerät müssen wir den
  BOOTSEL-Button auf der Front zugänglich machen oder einen „Update-Mode"
  per UI bauen (BOOTSEL aus Software triggerbar via `reset_usb_boot()`).

---

## Was Step 1 *jetzt* liefert

Siehe `firmware-c/`:
- `CMakeLists.txt` — Pico SDK 2.x, Board `pico2` (RP2350), USB-Serial an
- `pico_sdk_import.cmake` — Standard-Boilerplate
- `src/main.c` — STATUS-LED auf GP26 blinkt @ 1 Hz, USB-Serial sendet alle 2 s
  „FIELD AMBIENCE v0.9-dev pico2"
- `include/version.h` — Versions-Konstante
- `README.md` — Build-Anleitung
- `.gitignore` — Build-Artefakte raushalten

Auf deinem Rechner mit installiertem Pico SDK:

```bash
cd field-ambience-current/firmware-c
mkdir build && cd build
cmake .. -DPICO_BOARD=pico2
make -j
# field_ambience_native.uf2 entsteht — auf den Pico mit BOOTSEL droppen
```

Das ist der erste Atemzug. Step 2 macht das Display sprechen.

---

## Step 13 — MCU-Migration RP2350 → STM32H743VIT6 (NEU 2026-06-07)

**Auslöser:** UX-Review der Display-Sim → Re-Evaluation der Pico-2-Wahl als
Audio-Produkt-Plattform. Op-Counting-Schätzung war methodisch unzureichend
(siehe SPEC v0.7-r18 Header + CHANGELOG-Eintrag für volle Begründung).
**Entscheidung:** STM32H743VIT6 in LQFP100 als Bare-Chip. 3-4× CPU-Headroom,
1 MB internes RAM, native SAI/QEI/USART ersetzen Pico-PIO-Tricks. Alle
peripheren Komponenten (DAC, Amp, LCD, I²C, Encoder, USB-C, Battery) bleiben
elektrisch identisch.

### Architektur-Vertrag (was sich ändert, was nicht)

**Ändert sich:**
- MCU + sein direkter Power/Clock-Tree (Pico-Modul → H743 Bare-Chip + HSE-
  Crystal + LDO/SMPS + Decoupling)
- HAL-Schicht (~1000 LOC in `firmware-c-next/src/`): `audio.c`, `encoders.c`,
  `lcd_st7789.c`, `midi_pio.c`, `mcp23017.c`, `main.c`
- Build-System: CMake bekommt `-DTARGET=pico2|h743|host` Schalter
- `kicad/pico.kicad_sch` → archiviert als `kicad/legacy_pico2/`, neu:
  `kicad/stm32h743.kicad_sch`

**Bleibt 1:1:**
- DSP-Schicht (3600 LOC, Standard-C ohne Pico-Deps): pad, texture, bass,
  drone, reverb, generative, brain, engine, menu, oled_draw, battery
- Alle 11 Host-Test-Suites
- Sound Constitution (MCU-agnostisch)
- KiCad-Sheets: `oled`, `mcp`, `encoder`, `audio`, `battery` (5 von 8)
- BOM peripher (DAC, Amp, LCD, Encoder, Speaker, USB-C, Battery-Path)

### Phasen (sequenziell)

**Step 13.1 — Doku (1 Tag, niedrigstes Risiko) ✓ DIESER PR**
- SPEC v0.6 → v0.7, §1 + §3 + §4 + §5 umgeschrieben
- CHANGELOG r18-Eintrag mit Begründung
- ROADMAP.md + PITCH.md → `docs/archive/`
- Dieser Step-13-Block

**Step 13.2 — Firmware HAL-Reorg (DONE r18.27, 2026-06-15)** ✅
- 8 Pico-spezifische Files nach `src/hal_pico/` verschoben (audio.c,
  audio_i2s.pio, encoders.c, lcd_st7789.c, main.c, mcp23017.c, midi_pio.c,
  midi_tx.pio) — `git mv`, kein Code-Touch
- 6 STM32-Skelett-Files unter `src/hal_h743/` angelegt: audio_h743.c
  (SAI1+DMA-Plan), encoders_h743.c (TIM1/2/3/4 Hardware-Encoder-Mode),
  lcd_st7789_h743.c (SPI1+DMA), mcp23017_h743.c (I²C1@400 kHz für
  MCP+PCA), midi_uart_h743.c (USART2@31250 für TRS-MIDI), main_h743.c
  (Boot-Sequenz + Hold-Latch-Logik-Plan)
- CMakeLists.txt mit `-DTARGET=h743|pico|host`-Schalter (default=h743)
- **User-Direktive r18.27: „kein Pico mehr im Produkt"** — Pico-Build bleibt
  ausschließlich als Bench-Tool (`display_hw_test`) für die ST7789+Encoder-
  Bring-Up-Tests verfügbar; kein Pico-Produktions-Build mehr
- Reine DSP/Logic-Module (19 .c) bleiben unverändert in `src/` —
  test/run_tests.sh (13 Suiten) linkt nur diese, kein HAL-Touch
- **Gate ✅:** Host-Tests 13/13 PASS unverändert. Skelette enthalten
  `TODO(Step 13.3)`-Marker pro Funktion mit klarem ST-HAL-Plan

**Step 13.3 — ST-HAL-Integration (offen, ~5-7 Tage)**
- STM32CubeH7-HAL-Sources + Linker-Script + Startup-File integrieren
- ARM-GCC-Toolchain-File für `-DTARGET=h743`-Build
- Skelett-TODOs in `src/hal_h743/*.c` mit echten HAL_*-Calls füllen
- Erstes UF2-Image via ST-Link auf real STM32H743 (Steckbrett / erstes PCB)

**Step 13.3 — KiCad-Schaltplan-Migration (7-12 Tage)**
- `kicad/pico.kicad_sch` → `kicad/legacy_pico2/pico.kicad_sch`
- `kicad/stm32h743.kicad_sch` neu via `generate_kicad_project.py`:
  STM32H743VIT6-Symbol, LQFP-100-Footprint, SWD-Header, BOOT0-Pull-Down,
  NRST-Reset, HSE 8 MHz Crystal + Caps, VDD/VDDA-Decoupling
- `kicad/power_tree.kicad_sch` erweitert: AP7361A LDO aktiviert, VCAP-Bulk-Caps,
  VDDA-Ferrit-Filter, Reset-Sequencing
- Root-Sheet anpassen
- ERC clean, Generator deterministisch (2× Lauf bit-identisch)

**Step 13.4 — Firmware H743-Implementation (5-10 Tage)**
- STM32CubeMX-Projekt generieren als HAL-Basis (Cube nur für Initial-Gen,
  danach CMake)
- `src/hal_h743/audio_h743.c` (SAI1-A + DMA Ping-Pong, 256-Frame-Block)
- `src/hal_h743/encoders_h743.c` (TIM-QEI auf TIM2/3/5/8)
- `src/hal_h743/lcd_h743.c` (SPI1 + DMA)
- `src/hal_h743/midi_h743.c` (USART2 @ 31250 baud)
- `src/hal_h743/mcp23017_h743.c` (I2C1)
- `src/hal_h743/main.c` mit Boot-Sequenz (HSE→PLL→480 MHz, PAM-Sequencing)

**Step 13.5 — Profiling (CRITICAL ACCEPTANCE GATE)**
- DWT->CYCCNT um `engine_render()` für jeden Block
- Worst-Case-Last via Reproduktion des 8-Min-Performance-WAV
  (alle Voices + Reverb + Texture + Bass + Drone + Generative + UI)
- **Acceptance: < 40 % Block-Zeit Worst-Case** (Cycles < 1.1 M von 2.8 M Budget
  @ 480 MHz / 256 Frames / 44.1 kHz)
- 1-Stunden-Dauerlauf ohne Crash/Underrun/Reset
- **Vor diesem Gate: kein PCB-Layout.** Wenn Gate fällt: zurück in Phase 4 zum
  Profile-Optimieren (DTCM-Allokation, DMA-Optimierung).

**Step 13.6 — Erst dann PCB-Layout (separate Plan-Session)**

### Risiken

| Risiko | Mitigation |
|---|---|
| H743-Verfügbarkeit | Verifiziert (LCSC C114409 lagernd, Mouser/DigiKey ships today) ✓ |
| Power-Integrity | ST AN3318 als Vorlage, Decoupling nach Datasheet |
| Pinmux-Konflikte | H743 hat 4× SPI + 2× SAI + 12× TIM → genug Reserve, in Phase 3 finalisiert |
| Pico-Regression | Phase 2 Gate: Pico-Build muss grün bleiben |
| Profiling zeigt Insuffizienz | 480 MHz M7 + DTCM = pessimistisch sicher; Fallback Daisy-Modul-Plug-In |
