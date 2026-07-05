# PROJECT STATUS

**Updated: 2026-06-27 (r18.66 — Live-Level-Meter: 2. PCA9685 U10 @ 0x41 → 8 VU-LEDs (6 blau + 2 weiß), firmware-driven; 4× Push-Encoder bestätigt; Doku verschlankt (1 Engineer-Übersicht, PCB_TODO archiviert); pinmap + JLC BOM export + handoff; LED revert; 1.9in freeze)**

> **r18.92 (2026-07-05, User: „Grundrauschen zu praesent/zu dirty" + Rest-Ausbau):**
> Rausch-Forensik ergab: **ATMOS wirkte linear → konstanter −36-dBFS-
> Teppich bei 0.35** (Hauptverursacher), plus Dauer-Hiss in Stille. Fixes:
> quadratische Ambience-Kurve + Rain/Vinyl dunkler+leiser, **Programm-
> Follower-Ducking** fuer Hiss/Crackle (Floor −10,5/−6 dB), Air-Trim.
> **Gemessen: Demo-Floor −35,9 → −47,4 dBFS**, Idle −56,6 → −67,3; als
> Regression-Test verankert. Rest-Ausbau: **adc_h743** (BAT_SENSE PA3,
> 16-bit, kalibriert) + 1-Hz-Battery-Poll + USB-Detect (GPA7,
> MCP_BIT_VBUS) → Menu-Glyphs; **SHIFT+DISPLAY = Backlight** (10–100 %).
> 26 Suiten / 0 Failures; h743 161,9 KB; Demo neu. Details: CHANGELOG r18.92.
>
> **r18.91 (2026-07-05, HALL-Reverb + kritischer LIQUID-Fund):** A/B-Messung
> deckte auf: **der LIQUID-Hall hatte seit r18.42 keinen Tail** (Comb-
> Leserichtung falsch — Loop 6–10 Samples statt ~1200; tail@1s = 0.000).
> Neuer Default **HALL**: kreuzgekoppelter Figur-8-Tank nach Dattorro-
> Topologie (gelernt, nicht kopiert; eigene 44,1-kHz-Laengen, Drift-
> Modulation 0,101/0,127 Hz). EIN Raum statt zwei Mono-Hallen (xcorr 0.01),
> T60 4,5 s @ size 0.5 / ~15 s @ 0.9, weniger RAM (−30 KB) und ~halbe CPU.
> Legacy-Bug zusaetzlich gefixt. **Kohärenz-Audit** als Test: Sub-Floor
> ≥ 28 Hz ueber alle Welten×Vibes×Stufen, Melodie ueber Pluck-Floor,
> Hall monoton mit SPACE. 26 Suiten / 0 Failures; h743 baut; Autoplay-Demo
> neu. Details: CHANGELOG r18.91.
>
> **r18.90 (2026-07-05, Sound-World-Runde unter Elite-Audio-Kontrakt):**
> **docs/SOUND_WORLD.md** = bindende Klang-Verfassung (Identitaet, Verbote,
> Melodie-Grammatik mit harten Zahlen, Makro-Regeln). Code dazu: Pad-
> **Ensemble-Drift** (Juno-Prinzip: ±1,8-Cent-Wobble statt statischem
> Detune, an MOTION), **Melodie-Grammatik** im Autoplay (Phrasen, Pausen,
> Wiederholung, Schrittbewegung, Oktav-Faltung — komponiert statt
> wuerfelt; statistisch getestet ueber 206 Bars), **BRIGHTNESS als
> 3-Ziel-Makro** (Pad + Hall-Daempfung + Pluck-Daempfung), **FTZ** auf dem
> M7 (Denormal-CPU-Spikes in leisen Tails abgedichtet). 26 Suiten /
> 0 Failures; h743 baut; Autoplay-Demo neu. Details: CHANGELOG r18.90.
>
> **r18.89 (2026-07-04, Sound-Engine-Ausbau — „lerne von SuperCollider"):**
> Neue DSP-Primitiven (Pink/Dust/Crackle/Drive-Shaper, Konzepte aus SC-UGens
> studiert, frisch implementiert). **DRIVE ist jetzt eine echte Master-
> Saettigungsstufe** (vorher nur Reverb-Eingang — auf trockenen Sounds fast
> unhoerbar); AGE bringt zusaetzlich **Vinyl-Crackle** (Dust→2,6-kHz-
> Resonator + Chaos-Fry); Texture-Breath mit Pink-Anteil; **Generative-
> Sparkles sind Karplus-Strong-PLUCKS** (eigene Glocken/Koto-Farbe ueber dem
> Bed, exakte fraktionale Stimmung, selbst-abklingend). Neues 3-min-Demo
> field_ambience_autoplay.flac (reiner Passiv-Modus, Device-Pfad). Neue
> Suite test_sound_upgrades.c (Pluck-Pitch per Autokorrelation, 1/f-Proxy,
> Shaper-Makeup …); 26 Suiten / 0 Failures; h743 cross-baut. Details:
> CHANGELOG r18.89.
>
> **r18.88 (2026-07-04, Sound-Logik-Audit + Generative-Autoplay):** 5 echte
> Musik-Logikfehler gefunden + gefixt: (1) Generative lief ueber gehaltene
> SHIFT-Noten (any_cell_held sah Sources 9–13 nicht), (2) Momentary-Tap
> toetete beim Release die gelatchte Voice derselben Zelle (State-Desync),
> (3) GENERATE brauchte bis zu 8 s fuer die erste Note, (4) World-Wechsel
> liess Latches in der alten Tonart weiterklingen (Clash mit Drone/Bed) →
> controls_refresh_held_pitches(), (5) Bass-Sustain ignorierte Depth-
> Aenderungen. **NEU: echtes Autoplay** — engine_generative_tick():
> Sofort-Note beim Einschalten, humanisierte 8-s-Bars (±10 %), 0–2 leise
> Akkordton-„Sparkles" (+1 Okt, Sources 14/15) pro Bar, Live-Spiel
> unterbricht sauber und resumed sofort nach Loslassen. Neue Test-Suite
> (7629 Checks); 25 Suiten / 0 Failures; h743 cross-baut. Speicherfrage:
> nicht 512 KB gesamt, sondern 1 MB in Domaenen — aktuell ~35 % frei;
> Erweiterung ginge per QSPI-PSRAM (~$1.5, PB2+PE7–10 laut Pin-Tabelle
> frei), ist aber derzeit nicht noetig. Details: CHANGELOG r18.88.
>
> **r18.87 (2026-07-04, User: „only keep LEDs above cells + modifiers" + neue Audio-Renders):**
> **VU-Meter (U10 + 8 LEDs) und Heartbeat-LED (PD8) komplett entfernt** —
> Schematic (Generator), BOM (57 Parts / 188 Placements, −22; ~$89/Geraet
> Economic), Docs (ADR-0020 SUPERSEDED) und Firmware (vu.c + pca2-API +
> engine_render_peak raus; 24 Suiten gruen, h743 cross-baut). Es bleiben:
> 10 Cell-LEDs + 5 Modifier-LEDs an U6 (+ Backlight ch15). **LED_CHRG
> bewusst behalten** (laedt im AUS-Zustand — hardware-getriebenes STAT-
> Feedback, ADR-0016); auf Zuruf streichbar. **demos/audio/ neu gerendert**
> mit aktueller Engine: master_tape (8:20) + performance (5:00), FLAC-
> verifiziert. Speaker (J6/J7) + Display (J3) sind Steck-/Loet-Header —
> Module sitzen off-board im Gehaeuse (ADR-0007/0011).
>
> **r18.86 (2026-07-04, Firmware-Engine Teil 2 — Step 13.3 CubeH7-Bring-up):**
> **Die H743-Firmware ist jetzt eine echte, cross-kompilierte Firmware** —
> `cmake -DFAM_TARGET=h743` baut mit arm-none-eabi-gcc ein flashbares
> `.elf/.bin/.hex` (156 KB Flash, Vektoren @0x08000000). Neu: vendored
> CubeH7 HAL v1.11.6 + CMSIS (`src/hal_h743/vendor/`, BSD-3), Startup/
> Linker-Script/Toolchain-File, `SystemClock_Config` (HSE 8 MHz → PLL1
> 480 MHz VOS0; PLL3 fraktional → SAI-Kernel 11,289609 MHz = exakt
> 44,1 kHz), und ALLE sechs HAL-Treiber real: I²C1 @≈400 kHz (MCP23017 +
> 2× PCA9685, EXTI PC13), SAI1-A + DMA1-Circular-Pump mit D-Cache-Clean +
> SPEC-§8.3-Pop-Suppression, SPI1 @30 MHz ST7789 (Pico-Init-Sequenz 1:1),
> TIM1/2/3/4-Hardware-Encoder-Mode + 1-kHz-SysTick-Sampling (EN4-Push via
> MCP GPB5), USART2-MIDI 31250 Baud (Treiber fertig, Aktivierung weiter
> per ADR-0004 deferred), Main-Loop komplett (INT-getriebener MCP-Pfad,
> Jack-Detect-Debounce → nur Amp-Mute, 8-s-Generative-Bar, 60-Hz-LED/VU,
> 30-fps-Menu-Flush). RAM-Split im Linker-Script, da .bss (~630 KB) >
> AXI-SRAM: pad→DTCM, echo+blur→D2, Rest+DMA-Puffer→D1 (main() nullt die
> Extra-Regionen). Host-Tests unveraendert gruen (25 Suiten, 0 Failures).
> Naechster Schritt: Flash auf echter Hardware (Step 13.4/13.5 Bench).
>
> **r18.85 (2026-07-02, Firmware-Engine Teil 1 — VU-Meter):** Die r18.83-
> Luecke ist zu: `engine_render_peak()`-Tap (Block-Peak des finalen
> limitierten Outputs) → neues host-getestetes Modul `vu.c` (8 Segmente
> −36…−0,5 dBFS, Instant-Attack, 30 dB/s Release, 900 ms Peak-Hold-Dot,
> interpoliertes Bar-Ende) → `pca2_*`-API fuer U10 @ 0x41 → 60-Hz-Tick in
> main_h743. Neue Suite test_vu.c (44 Checks) inkl. End-to-End gegen den
> echten Engine-Render. Naechster Schritt: **Step 13.3** (CubeH7-Bring-up:
> Clocks/SAI-DMA/SPI/I²C/USART/TIM — dann macht die H7-Firmware real Sound).
>
> **r18.84 (2026-07-02, User: „missing must-have components? + neue JLC-Kostenschätzung"):**
> **KRITISCH gefunden: beide PCA9685-/OE-Pins hatten Pull-up ohne jeden
> Treiber** — /OE permanent HIGH = alle 15 Status-LEDs + Backlight + 8
> VU-LEDs für immer aus → Pull-DOWN (Boot-Dark garantiert der Chip: NXP-DS
> Tab. 7, LEDn_FULL_OFF Default=1, PDF-verifiziert). **Ergänzt:** 4×
> M2.5-Mounting-Holes (mech. Spec §6, fehlten im Schematic) + 7 Testpunkte
> (TP_5V/3V3/5VSW/VBUS/GND×2/BAT) — alles DNP, BOM unverändert 58/210.
> Sonst keine fehlenden Must-haves (USB-Schutz, Power-Pfad, NRST/BOOT0,
> VCAP, HSE, Pull-ups, SWD ✓; Akku-Schutz gehört ins LiPo-Pouch — Kaufnote).
> **COST_ESTIMATE.md neu (Preise live):** 5er-Run ~$92/Gerät (~$48 JLC +
> ~$44 Hand) — ⚠ Outline 252 mm liegt 2 mm über JLCs Economic-Limit
> (250×250): ums Kürzen bitten oder Standard-PCBA ~$102/Gerät. Stock-Watch:
> Tantal C444831 nur ~121 St. Details: CHANGELOG r18.84.
>
> **r18.83 (2026-07-02, User: „Will everything work with the firmware/software?"):**
> Firmware↔Hardware-Konsistenz-Audit. **Alle H743-HAL-Pin-Claims = Schematic**
> (SAI PE4/5/6, LCD SPI1 + Backlight-Ch15, I²C PB6/7+PC13, alle 4 Encoder-
> TIM-Paare, AMP PB14/15, MIDI PD5, BAT_SENSE PA3, MCP-Bitmap, Jack-Detect-
> Polaritaet). **Geschlossen: der r18.76-Gap** — main_h743 trug noch den
> stalen Hall-ADC-TODO fuer die Cells → jetzt digitaler MCP-Edge-Pfad
> (wie Pico-Bench, durch host-getestete controls.c) + Jack-Detect-Routing
> (nur Amp muten, Line-Out bleibt live) + stale Kommentare fixiert. **Ehrlich
> offen:** (1) Step 13.3 = CubeH7-Peripherie-Bring-up (Clocks/SAI-DMA/SPI/
> I²C/USART/GPIO) — Logik ist host-getestet + Pico-verifiziert, H7-Anbindung
> fehlt; (2) VU-Meter U10@0x41: keinerlei Firmware (8 LEDs bleiben dunkel,
> eigener Schritt); (3) MIDI-TX deferred per ADR-0004 (Hardware komplett).
> H743-Target baut+linkt, 352298 Test-Checks gruen. Details: CHANGELOG r18.83.
>
> **r18.82 (2026-07-02, User: „All the other components right? check datasheets"):**
> Systematischer Datenblatt-Abgleich aller restlichen Komponenten + jedes
> Symbol↔Footprint-Pad-Paars. **2 kritische Pad-Mapping-Fehler:** (1) alle
> 4 Encoder waren KOMPLETT unverbunden — der offizielle KiCad-EC11E-Footprint
> hat BENANNTE Pads A/B/C/S1/S2, das Symbol nutzte 1–5 (KiCad matcht Strings)
> → Pin-Nummern korrigiert; (2) PJ-320D-Jacks (Line-Out + MIDI): Pad-Map
> falsch — Audio-L lag auf dem geerdeten Sleeve-Barrel, JACK_DETECT auf dem
> Tip → korrigiert (S=1/A, R=2/D, DET=3/C, T=4/B, gegen SHOU-HAN-Zeichnung;
> TODO B0b geschlossen) + R_DET 10k/C_DET 1µF ergänzt (Detect ruht unplugged
> am TIP = DAC-Ausgang → Clamp-Schutz nötig). **2 Lücken:** MCP73831 ohne
> DS-gefordertem 4,7-µF-VDD-Cap (→ C_CHG_IN); STM32-VSS-Pin 99 nur zufällig
> geerdet (→ explizit, DS12110 Fig. 5). **Ohne Befund verifiziert:** MCP23017-
> SSOP, PCA9685 (beide), USBLC6, komplette H743-LQFP100-Map, Crystal CL18pF→
> 27pF-Caps, TVS-Polarität, USB-C-Pads, HX-B3F-Paarung. ERC 0 Fehler, 58
> Parts/210 Placements, Tests grün. Details: CHANGELOG r18.82.
>
> **r18.81 (2026-07-02, User-Frage „side on/off switch — mountable auf horizontaler PCB?"):**
> **Ja — datenblatt-verifiziert:** MST-12D18G3 liegt flach auf der PCB, der
> Schiebe-Stem ragt HORIZONTAL 3 mm über die Body-Kante (z ≈ 2,3–3,8 mm über
> Board), Travel 2 mm → am Board-Rand platzieren, Slot in der Gehäuse-
> Seitenwand; vendored Footprint = exakt das offizielle MSK12D-Land-Pattern.
> Die Frage deckte zwei kritische Lücken auf: (1) **der beschlossene
> ADR-0016-Power-Off-Block (U_PWR TPS22918 + SW_PWR + R_PWR_PD + C_PWR_SW)
> war NIE im Schematic** — das Board hätte keinen Ein/Aus gehabt → jetzt in
> power_tree verdrahtet (+5V_RAIL → U_PWR → +5V_SW → LDO; Lader davor =
> „dunkel, aber lädt"; QOD→VOUT aktive Entladung, CT floatet per TI-DS);
> (2) **das globale „+5V"-Netz war eine quellenlose Insel** (keinerlei Brücke
> zur Diode-OR-Rail auf irgendeinem Sheet) — PAM8403 + alle 23 LED-Anoden
> wären stromlos gewesen → Rail trägt jetzt das +5V-Flag. Netz-Trace 11/11 ✓,
> ERC 0 Fehler, jlc_bom 58 Parts/207 Placements. OFFEN für Aron: SW_PWR-
> Terminal-Belegung (Common=Mitte angenommen) vor Fab durchpiepen; Falsch-Fall
> fail-safe. Details: CHANGELOG r18.81.
>
> **r18.80 (2026-07-02, geometrisches Pin-Level-ERC über alle 7 Sheets):**
> 12 Fehler gefunden + behoben, davon 7 Kupfer-Kurzschlüsse und 4 komplette
> Unterbrechungen — jede „totes Board"-Klasse: **USB D+/D− auf +5V** (U5-LDO-Block
> sass im USB-Korridor → verlegt); **I2C_SCL auf GND** (C5-GND-Pin exakt auf dem
> SCL-Wire — die 7,62-mm-Pitch-Falle, VDD- und SCL-Pin liegen genau 3 Zeilen
> auseinander → C5/C5b verschoben); **PCM_XSMT auf GND** + **I2S_LRCK auf GND**
> (gleiche Falle im Audio-Sheet → Caps hochgeklappt); **BOOT0 permanent HIGH**
> (+3V3-Flag auf dem BOOT0-Punkt UND SW_BOOT-Wire durch den Pin-2-Anker → H7
> hätte nie Firmware gebootet); **VDDA↔VDD-Kurzschluss** (Pin-21-Stub durch
> Pin-100-Anker) bei gleichzeitig **beidseitig floatendem FB2** (rot=90);
> **C_BOOT-Bootstrap kurzgeschlossen** (Label auf Wire-Kreuzung); **LCD-J3 alle
> 8 Pins floatend** + **SWD-J4 alle 3 Pins floatend** (Custom-Lib-Pins rechts,
> Verdrahtung nahm KiCad-Standard links an); **MCU-lib_symbols ohne „MCU:"-Prefix**
> (KiCad hätte U1 nicht aufgelöst). Dazu **Boost-Loop-Fix nach TI-Datenblatt**:
> R_COMP 22k→6,2k (C4260 neu, live verifiziert), C_COMP 1nF→10nF, C_BOOST_OUT
> 1×→3× 22 µF — alte Werte legten den Crossover ÜBER die RHP-Nullstelle
> (Oszillation unter Last). MPN-Hygiene: C45783=22µF/C15850=10µF-Verwechslung
> in VDD-Bank + Doku korrigiert, 15 MPN↔LCSC-Mismatches vereinheitlicht (JLC
> bestückt nach C-Nummer). Geometrisches ERC jetzt **0 Fehler auf allen Sheets**;
> Hier-Pins↔Root matchen; Generator deterministisch. KiCad-9-GUI-ERC (Aron)
> bleibt finaler Gate. Details: CHANGELOG r18.80.
>
> **r18.79 (2026-07-01, Elektrik-Audit BOM+Verbindungen):** 3 kritische Fehler im
> Power-Design gefunden + behoben (alle gegen das TI-TPS61089-Datenblatt
> verifiziert): (1) **Boost-FB-Teiler ergab 7,43 V statt 5 V** (R23 200k→121k,
> C25809) — hätte Amp + Load-Switch zerstört; (2) **R_ILIM 20k = 51 A ≈ kein
> Stromlimit** (→174k, C22890, 5,9 A ≤ L1-Isat); (3) **Q1-Power-Path-Backfeed**:
> Boost speiste VBUS/Lader rückwärts (Selbstladeschleife, Akku-Drain, USB-Detect
> dauerhaft HIGH) + Q1 überbrückte die Sicherung → Q1/R22 entfernt, Dioden-OR
> mit 2. SS34 (D3B, C8678). Plus Doku-Fixes: Fsw real ~440 kHz (nicht 1,21 MHz),
> PINMAP AP7361A→C, PCB_BOM-Restdrift (C_BULK2/LED_CHRG/VCAP auf r18.70-Stand).
> Strukturell verifiziert: Hier-Pins↔Root-Wiring vollständig, 0 Refdes-Dubletten,
> I²C 0x20/0x40/0x41 kollisionsfrei, BOOT0/CC/FSW korrekt. ERC in KiCad (Aron)
> bleibt als finaler Gate.
>
> **r18.78 (2026-07-01, Cell-Switch-Sourcing + Cap-Länge):** User fragte, ob
> andere Kailh-Choc-Farben gehen (das Schematic-Beispiel C400229 zeigte
> "Not available now") und ob 2–3 cm lange Cell-Caps später ein Problem sind.
> **Sourcing:** alle 3 Choc-V1-Farben bei LCSC aktuell 0 Lagerbestand
> (C400229/C400230/C400231, alle real verifiziert) — kein Blocker, da
> handgelötet, nicht auf LCSC-Bestand angewiesen; jede Farbe/jeder Vendor
> passt in denselben Footprint. **Cap-Länge:** 2–3 cm braucht keinen
> Stabilizer (weit unter der 2u/~38mm-Schwelle), kollidiert aber mit dem
> Nachbar-Cell bei der aktuellen 19mm-Teilung, wenn links-rechts orientiert.
> User: Aron entscheidet das beim eigentlichen PCB-Layout — als offener Punkt
> dokumentiert (zwei Lösungswege + verfügbarer Platz), nicht vorher fixiert.
> Details: `mechanical_coordinates.md` §3.4, `MECHANICAL_REQUIREMENTS.md`,
> `BOM_MASTER.md` §7.
>
> **r18.76 (2026-07-01, BOM/Funktionalitäts-Audit):** User bat um einen
> Check von BOM + Geräte-Funktionalität. Ergebnisse:
> - **BOM-Fix (real, behoben):** J3 (LCD-Header) und J6/J7 (Speaker-Header)
>   hatten im Generator Platzhalter-`"TBD"` im LCSC-Feld, obwohl `BOM_MASTER.md`
>   längst die echten Codes dokumentierte (**C124383** für J3, **C124375** für
>   J6/J7) — Doku↔Generator-Drift. Generator jetzt synchronisiert; `jlc_bom.csv`
>   hat jetzt 0 verbliebene `TBD`-Zeilen außer dem bereits geprüften
>   Tag-Connect-J4 (echtes „kein LCSC", r18.37 bereits auditiert).
> - **BOM sonst sauber:** keine doppelten Referenzdesignatoren (201 eindeutige
>   Instanzen über alle Sheets), CSV parst fehlerfrei trotz eingebetteter
>   Kommata, Generator-Output deterministisch über mehrere Läufe verifiziert.
> - **Firmware-Funktionalitätslücke gefunden (⚠ NICHT behoben, nur dokumentiert):**
>   `src/hal_h743/main_h743.c` hat noch einen TODO-Stub, der die Cells über
>   analoge Hall-ADCs liest (`adc_read_norm(c)`) — das passt nicht mehr zur
>   Hardware seit r18.73 (Cells sind jetzt digitale MCP23017-GPIO-Buttons,
>   kein Hall-Sensor mehr, die ADC-Pins sind freigegeben/unbeschaltet). Der
>   Pico-Bench (`main_pico.c`) hat den korrekten digitalen Pattern schon
>   längst implementiert und getestet (liest MCP-GPIO-Edges direkt, ruft
>   `engine_note_on/off` auf, umgeht die alte Velocity-State-Machine
>   komplett) — das ist die Vorlage für den H743-Fix. `mcp23017.h` selbst ist
>   bereits korrekt (GPA0-4 = CELL1..5, dokumentiert genau wie das Schematic).
>   Ohne diesen Fix würden die Cells auf echter H743-Hardware nicht
>   funktionieren, wenn der TODO wörtlich implementiert wird. War schon vorher
>   als "⏳ optional cleanup later" in §3 vermerkt — hiermit konkretisiert.
>
> **r18.75 (2026-07-01, User-Nachfrage "wie wird das gelötet?"):** der r18.74-
> Hot-Swap-Socket hatte keine saubere Hersteller-/LCSC-Teilenummer und hätte
> eine nicht offensichtliche Klein-SMD-Handlöttechnik gebraucht. **Fix:** SW1–5
> jetzt **Kailh Choc V1 (CPG135001D01) direkt gelötet** — 2 THT-Beinchen,
> gleiche Technik wie jeder andere Button hier. Footprint + 3D-STEP direkt von
> LCSC/EasyEDA für **C400229** gezogen (verifiziert real, ⚠ 0 Lagerbestand zum
> Prüfzeitpunkt). Kein Socket mehr, Switch jetzt fest verlötet (nicht mehr
> tauschbar). Elektrisch unverändert seit r18.73/74. BOM/PCB/HTML/Schematics
> aktualisiert.
>
> **r18.74 (2026-07-01, User-UX-Korrektur):** r18.73 hatte die Cells auf
> dasselbe kleine HX-B3F-Tactile wie die Modifier gesetzt — machte die
> spielbaren Cells ununterscheidbar von simplen Modifier-Knöpfen, zerstörte
> das "Tastatur"-Gefühl. Cells bekamen einen Kailh Choc Hot-Swap-Socket für
> echtes Keyswitch-Gefühl zurück (~3mm Hub) — später in r18.75 vereinfacht.
>
> **r18.73 (2026-06-30, User-Direktive):** **Cells → digitale I²C-Switches** statt
> Gateron-Magnetic + DRV5056A4-Hall (HiChord-Batch-4+-Weg: Switch → I²C-Expander
> → MCU); 5× Hall + RC entfernt; STM32-ADC-Pins PC0/PC1/PA4/PB0/PB1 frei.
> ADR-0013 SUPERSEDED.
>
> **r18.67:** MIDI-Out **reaktiviert + implementiert** als **J10** (TRS Type A, OUT-only, 3,3 V/CA-033; MIDI_TX=PD5 → 2× 220 Ω → Tip/Ring). Refdes-Kollision behoben: **J9 = Akku, J10 = MIDI**. Power-Aus: Schiebeschalter auf der **Boost-EN-Leitung** entschieden (signal-level, Laden bleibt) — **noch zu implementieren**.
>
> **Offen:** (a) blaue VU-LED + 220-Ω-MIDI LCSC verifizieren (NO-LCSC-Liste), (b) Firmware: Level-Meter (U10-I²C) + MIDI-UART (PD5) + Encoder-Push-Mapping, (c) Power-Schiebeschalter im Generator umsetzen, (d) GUI-ERC board-weit (Blocker B3), (e) Doc-Sweep J_BAT→J9 in Restdocs.

**Purpose:** persistent orientation document so the assistant (and the user)
can pick up the project without re-scanning the full history each session.
Source of truth for "what is done, what is in progress, what is open" —
overrides anything an old commit message says. Keep concise; update at the
end of every session that changes a shipping state.

**Conventions:**

- ✅ ready / merged-mergeable
- 🟡 in branch, working, not merged yet
- 🟠 audition-only / demo (renders, no engine integration)
- 🔴 broken / blocked
- ⏳ planned / next-up

---

## 1. Where we are right now

**Working branch:** `claude/read-start-here-YDlCd` → PR #37 merged to `main` 2026-06-22
**Latest user direction:**
- Sound stays synthetic (no field-recording samples). Stage is "fast gut genug".
- No web simulator — Pico 2 is the bench, not a browser
- Merge what's ready, keep an overview, write your own status doc

---

## 2. Project identity (what the device IS)

A handheld ambient instrument that loads **curated WORLDS** — sound presets
the user thinks in pictures (night city, sunset coast, night highway, jazz
bar) rather than synth-engine parameters. Cells trigger notes inside the
world; modifier buttons shape the playing; encoders nudge a few global
macros over the world preset.

**Audio target:** PS2-era / dreamy / warm-pop / chillout. NOT meditative,
NOT yoga app, NOT dark horror, NOT crystal-castles-aggressive. The
user-accepted reference render is `tools/render_dreamy_warm.c`.

**Hardware target product:** STM32H743 (`src/hal_h743/`). The Pico 2 is a
BENCH tool for LCD + encoder bring-up (`tools/display_hw_test.c`), NOT a
product build.

---

## 3. Status by layer

### Sound DSP (audio chain)

| Item | State |
|---|---|
| V1 warm-chorus pad ("100x better" sound) | ✅ `src/pad.c`, reverted from soften'd profiles in r18.37 |
| Tier A #1: velocity → filter cutoff | ✅ `src/pad.c` r18.43 — bright hits open up |
| Tier A #2: micro-humanisation (±0.5 cent / ±0.3% amp jitter) | ✅ `src/engine.c` r18.43 |
| Tier A #3: drone drift + breath (±2 cent walk, 0.04 Hz tremolo) | ✅ `src/drone.c` r18.43 |
| Tier A #4: texture body weight 0.35 → 0.10 (removes the Brumm) | ✅ `src/texture.c` r18.43 |
| Tier A #5: air band (+HP 3 kHz on white noise, 0.18×) | ✅ `src/texture.c` r18.43 |
| LIQUID modulated FDN reverb (default) | ✅ `src/reverb.c` r18.42 — Freeverb as `-DFAM_REVERB_MODE=0` fallback |
| `drone.c`, `bass.c`, `brain.c` | ✅ |
| 4-world sound spec (Tokyo / Coast / Drive / After Hours) | 🟠 only as `tools/render_worlds.c` |
| Universal wind generator (resonant BP, pink noise, gusts) | 🟠 inline in render_worlds.c |
| Per-world ambience layer in engine (ADR-0017 Phase 2 KOMPLETT) | ✅ r18.49-.52 — Wind universal + Rain Tokyo + Waves Coast + Vinyl After Hours. Drive bekommt Wind als „highway" (kein dedizierter Traffic-Generator, Wind ist character-genug). |
| Worlds-Modul (single source of truth) | ✅ r18.48 `src/worlds.c` + `worlds.h` (ADR-0017 Phase 1) — Lift aus menu.c, exakte RGB/Preset-Erhaltung per Test |
| **Per-World musikalische Identität (Tonart/Mode/Vibe)** | ✅ r18.63 — Tokyo A-Dur ionian warm · Coast D-Dur ionian bright · Drive Fis-Moll dorian deep · Hours C-Moll aeolian floating. `engine_set_world` pusht key/mode/vibe in den brain; Cell-Roots verifiziert distinct (57/62/54/60) |
| Tape-hiss generator | 🟠 inline in render_dreamy_warm.c |
| Tape character (hiss + warm-tanh saturation) im Master | ✅ r18.53 `src/tape.c` (ADR-0017 Phase 3); always-on, Default = dreamy_warm-Referenz (hiss 0.005, drive 1.10) |
| **Echo: tape-style stereo delay (Reddit #1 perform effect)** | ✅ r18.58 `src/echo.c` — Macro 0..1 maps zu time/feedback/wet/tone; LP-im-Feedback = Tape-Charakter |
| **Blur: granular cloud / smear (Reddit ambient block)** | ✅ r18.60 `src/blur.c` — 16 Grains × 200 ms Ring; Macro hides density/size/jitter/scatter/wet |
| World preset application from engine | ⏳ next refactor step |
| Per-world drums system (menu toggle exists) | ⏳ `beat.c` lives in `src/v2/` and can be lifted out |
| Aliasing pre-filter on noise sources | ⏳ noted "macht prinzipiell Sinn", not done |

**Audition tools (do not ship, useful for A/B):**
- `tools/render_dreamy_warm.c` — accepted reference 60 s
- `tools/render_worlds.c` — all 4 worlds, has `measure` mode for level audits
- `tools/render_oled.c` — host-side menu preview PGMs

### Engine V2 (`src/v2/`)

| Item | State |
|---|---|
| Whole v2 engine (worlds, harmony field, particle, motion, beauty guard) | 🟠 dead-end direction the user rejected ("Horror", "Crystal Castles") |
| `src/v2/beat.c` (kick/snare/hat + bitcrush + drive) | 🟠 keep — will be lifted out for the per-world drums system |
| All other v2 modules | ⏳ likely removed when engine refactor lands; keeping for now |

### Menu / Display UI

| Item | State |
|---|---|
| `menu.c` / `menu.h` on WORLD model (World / Space / Tone / Atmos / Drums) | ✅ r18.38 |
| Renderer + tween engine in `display_hw_test.c` | ✅ unchanged, drives new menu |
| World subtitle ("night . rain" etc) under big value | ✅ |
| World preset loads on world change (Space/Tone/Atmos snap) | ✅ |
| `tools/render_oled.c` host preview | ✅ updated to world model |
| `tools/display_sim.html` JS port | ✅ updated to world model (sim still committed, just no auto-deploy) |
| Pico 2 bench build (`display_hw_test.uf2`) | ✅ CI RP2350 build green |
| Engine ↔ menu callbacks wired in HAL | ✅ r18.54/.58/.60 — `main_h743.c` `menu_init()` mit world/space/atmos/motion/age/echo/blur Bindings (ADR-0017 Phase 4 + Reddit-Macros) |
| **Reddit-style 7-slot Perform-Menü** | ✅ r18.58/.60 — World · Space · Atmosphere · Motion · Age · Echo · Blur; per-World-Presets in `worlds.c`; Encoder dedicated (Drive · Brightness · Volume · Display-Nav) — keine Duplikate |
| **Display Akzent-Farbe pro World (Grau→RGB565-Tint)** | ✅ r18.44 `src/oled_color.c` — ADR-0015 Schritt 1; Default=Mono, pro World dezenter Cast; Host-Preview farbig |
| **Akzent-Crossfade beim World-Wechsel (erste UI-Animation)** | ✅ r18.45 — `oled_accent_tick/settle`, libm-frei; auf Pico-Bench testbar |
| **Pill-Bloom-Animation (zweite UI-Animation)** | ✅ r18.55 — Aktive Pill macht Scale-Pop (1.25× → 1.0×) bei Cursor-Wechsel + 1.15× bei discrete Value-Change |
| **Bench `display_hw_test` aktuell zur World-UI (Subtitle + panel-agnostische Verdrahtung)** | ✅ r18.44/45 — 1.9″ Adafruit *und* Waveshare Silk; `display_hw_test.uf2` als CI-Artefakt |
| **Schaltbild-Walkthrough (A–Z PCB-Engineer-Tour)** | 🟡 r18.46 — `docs/hardware/SCHEMATIC_WALKTHROUGH.md`; Power + MCU + Audio voll, übrige 4 Sheets in Folge-PRs |
| **Power/Sleep-Architektur (ADR-0016, kein Switch)** | ⏳ ADR-0016 PROPOSED — `U9` TPS22918 Load-Switch + `SW_BOOT` Dual-Use |
| **Panel-Selector Firmware-Pfad (1.9″ ↔ 2.0″ via CMake-Flag)** | ✅ r18.47 — `oled.h` + Pico-Treiber + CMake; Default 1.9″ unverändert, 2.0″-Build kompiliert sauber. UI-Layout-Rebalance offen (70 px Dead-Space) |
| **Display für diesen PCB-Rev: 1.9″ EINGEFROREN** | ✅ r18.64 — User-Entscheidung „1,9 zoll reicht safe"; verifiziert + im Schematic. Entblockt das Layout. |
| **Panel-Hardware-Pivot 1.9″ → 2.0″ (physisches Modul)** | ⏳ **Rev-B** ADR-0015 — kein Blocker mehr; später wenn User reales 2.0″-Modul (SKU/Pin-Order/Maße) verifiziert |
| **Voller RGB565-FB + DMA-Animationen** | ⏳ ADR-0015 D4 — nach Hardware-Pivot |

### Cells / Input

| Item | State |
|---|---|
| **Cells → digital I²C switches, real keyswitch feel, direct-solder (r18.75, ADR-0013 SUPERSEDED)** | ✅ SW1–SW5 on MCP23017 GPA0–GPA4, HiChord-Batch-4+ pattern. Removed 5× DRV5056A4 Hall + RC; freed PC0/PC1/PA4/PB0/PB1. r18.73 first put cells on the same small HX B3F tactile as the modifiers (killed the keyboard-key UX) — r18.74 tried a Kailh Choc hot-swap socket (unverified sourcing, fiddly hand-soldering) — r18.75 simplified to **direct-solder Kailh Choc V1 (CPG135001D01, LCSC C400229)**, footprint + 3D STEP pulled straight from LCSC/EasyEDA, real verified part, plain THT soldering. |
| ~~Gateron LP Magnetic + DRV5056A4 Hall plan (ADR-0013)~~ | ⛔ superseded r18.73 — Hall kept as documented option only if expressive press-depth/velocity is wanted later |
| `cells.c` velocity state machine | ✅ host-tested; with digital cells it degrades to on/off trigger (full-press position). FW engine read-path unchanged (bench already synthesizes positions from digital buttons). ⏳ optional cleanup later |
| Pressure/aftertouch from `cells_position()` | ⛔ N/A with digital cells — needs the Hall variant (ADR-0013) |
| Modifier set Shift / Hold / Drone / Generate (auto-play) / Clear | ⏳ specified, not implemented in world engine |

### HAL

| Target | State |
|---|---|
| `src/hal_pico/` (Pico 2 SDK, bench-only) | ✅ display_hw_test builds + runs |
| `src/hal_h743/` (STM32H743, product) | 🟡 skeleton compiles syntactically; CubeH7 toolchain integration is the final step. ⚠ r18.76-Audit: `main_h743.c`'s cell-read TODO still describes the retired Hall-ADC path (`adc_read_norm`) — needs porting to the digital MCP-GPIO-edge pattern already proven in `hal_pico/main_pico.c` (mirrors the modifier-button code 2 lines above it in the same loop) |
| `tools/display_hw_test.c` (deliverable for "test on Pico") | ✅ |

### PCB / BOM

| Item | State |
|---|---|
| `BOM_MASTER.md` | ✅ r18.75 — §7 cells digital on MCP + real Kailh Choc V1 keyswitch, direct-solder (Hall path removed); FP links clickable |
| **Cells digital-switch change (r18.73) + keyswitch-feel correction (r18.74) + direct-solder simplification (r18.75)** | ✅ generator (mcp_sheet SW1–5 on GPA0–4 + STM32 ADC pins freed; r18.75 footprint = `field_ambience:SW_KailhChoc_CPG1350_THT_2P`, pulled from LCSC/EasyEDA for C400229, vendored with 3D STEP), schematics regenerated, jlc_bom.csv, Aron `bom_overview.html`, BOM_MASTER/PCB_BOM/PINMAP/KICAD_BLUEPRINT/SCHEMATIC_WALKTHROUGH/MECHANICAL_REQUIREMENTS/PCB_FOOTPRINT_RISK_AUDIT/mechanical_coordinates/ADR-0013 all updated |
| `field-ambience-current/PCB_FOOTPRINT_RISK_AUDIT.md` (risk-based per user brief) | ✅ r18.37 |
| 6 custom KiCad footprints in `field_ambience.pretty/` | ✅ all actively referenced |
| 7 STEP models in `field_ambience.3dshapes/` | ✅ |
| BOM ↔ generator LCSC string-diff | ✅ 0 actual mismatches |
| Layout-blocker: ERC walkthrough of 9 ICs in KiCad | ⏳ user task (KiCad GUI) |
| Layout-blocker: TPS61089 datasheet §10 pre-study | ⏳ user task |
| Order-blocker: 1:1 print of 10 mech-critical parts vs enclosure | ⏳ user task |
| Order-blocker: Waveshare LCD pin-order verify on actual module | ⏳ user task |

### CI / Build

| Workflow | State |
|---|---|
| `firmware-c.yml` host unit tests | ✅ 83/83 + 46 display-bench |
| `firmware-c.yml` RP2350 build (display_hw_test) | ✅ |
| `pages.yml` (GitHub Pages deploy of display_sim.html) | ❌ REMOVED r18.39 per user — Pages was never enabled, deploy always failed |

---

## 4. What is mergeable to `main` right now

Everything since `main` is mergeable. Nothing in `claude/...` is broken or
experimental in a way that would make merging risky.

- All `tools/render_*.c` audition renderers (no firmware change, demos only)
- `BOM_MASTER.md` r18.36/37 cleanups
- `PCB_FOOTPRINT_RISK_AUDIT.md`
- Pad-profile revert to V1 warm-chorus (r18.37)
- World UI in `menu.c` / `menu.h` / `display_sim.html` / `render_oled.c` (r18.38)
- Test updates for world model
- 6 obsolete audition tools deleted in r18.37
- `pages.yml` deletion (r18.39)

The DSP/engine sound integration of the worlds is NOT in here yet — that's a
separate ⏳ chunk, lives only as audition tools.

---

## 5. What's next, in priority order

1. **Display-Pivot 1.9″ → 2.0″ + RGB565 + Animations-Architektur** ⏳ ADR-0015
   (r18.43, PROPOSED). User-Side: Modul-SKU + Pin-Order + Maße verifizieren.
   Firmware-Side: `oled_*` API von 4-bit Grau auf RGB565 portieren, FB nach
   AXI-SRAM, SPI-DMA + DMA2D in `lcd_st7789_h743.c` ausimplementieren
   (Step 13.3 TODO). Generator-Sheet erst nach Pin-Order-Bestätigung anfassen.
2. **Engine refactor — world model** ⏳ `worlds.c` + `ambience.c` (lift the
   inline generators from `tools/render_worlds.c` into real modules),
   `hiss.c`, warm-saturation module, rewrite `engine.c` to be world-driven,
   wire `menu_callbacks_t` in `src/hal_h743/main_h743.c`. Eigene ADR (0016)
   beim Start drafting.
3. **Per-world drums system:** extract `src/v2/beat.c` into `src/drums.c`,
   per-world pattern + tempo selection, wired to the menu Drums toggle.
   Retire the rest of `src/v2/` afterwards.
4. **Aliasing pre-filter** on noise sources when they move from tools/ into
   engine modules.
5. **PCB layout-blockers** (user-side KiCad work — ERC pass + TPS61089
   layout study).
6. **AI-Ready Schematic Standard compliance** (binding rule, see
   `docs/hardware/AI_READY_SCHEMATIC_STANDARD.md` §"compliance snapshot"):
   safe generator edits — rename active-low nets to `_N` suffix
   (`nSHDN`→`AMP_SHDN_N`), populate per-sheet title-block revision/date/author,
   add `TP_*` test pads. Plus the GUI-ERC pin-map + NC audit (= blocker B3).

---

## 6. Update protocol for the assistant

When ending a session, do one pass through §3 and §5 and update lines that
changed. Don't rewrite the whole file. The point: anyone reading this knows
in 90 seconds what's true today, without trusting old commit-message claims.

**This file replaces** the old "Project Status" snapshot (r18.6, 2026-06-11)
which was 10 days stale. Other docs intentionally NOT replaced:

- `field-ambience-current/CHANGELOG.md` — full revision history
- `field-ambience-current/ROADMAP.md` — long-term plan
- `BOM_MASTER.md` — single source of truth for parts
- `field-ambience-current/PCB_FOOTPRINT_RISK_AUDIT.md` — risk-based PCB pass
