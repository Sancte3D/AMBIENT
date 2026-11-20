# Referenzaufnahme: was sie misst und was wir daraus lernen

Stand 16.11.2026. Vermessen wurde `ambient sample.mp3.part1` aus `main` (Commit be65259c, 15 Minuten,
192 kbit/s). Die Aufnahme ist eine Referenz des Besitzers für Atmosphäre, Tiefe und Geschlossenheit. Sie soll
nicht kopiert werden. **Gemessen, nicht gehört:** Welche Klangquellen es sind und wie sie wirken, ist Deutung.

## Messwerte

| Messwert | Referenz | Die sechs V3-Biome (Generate mit DRONE, Natur aus) |
|---|---|---|
| Spektraler Schwerpunkt | 240 Hz | 293–778 Hz |
| 85 % der Energie unter | 345 Hz | 366–1098 Hz |
| Links/Rechts-Korrelation | 0,16 | 0,69–0,82 |
| Seite gegen Mitte | −1 dB, in jedem Band | −7 bis −10 dB |
| Energie unter 80 Hz | −10 dB | −40 bis −84 dB |
| Ereignisse unter 1 kHz je Minute | 1,3 | 8–23 Notenstarts |
| Ereignisse 1–8 kHz je Minute | 16 | nicht gemessen |
| Pegel über 14 Minuten | ±0,7 dB | – |

- **Tonales Bett:** stabile Spitzen auf D2, A2, D3, A3, D4, D5, D6 und schwächer Fis, E, G, C; alles etwa
  18 Cent unter Normalstimmung. Oberhalb von 2,5 kHz nur Rauschen, 30–45 dB unter dem Bett.
- **Bewegung in zwei Zeitebenen:** Im Band 80–320 Hz liegt die Hälfte der Pegelschwankung bei Perioden von
  0,5–2 s (Schweben nah beieinander liegender Töne). Über 1,25 kHz liegt sie bei 8–120 s.
- **Keine Entwicklung im Großen:** Nach der ersten Minute bleiben Pegel und Schwerpunkt praktisch gleich.

## Sechs Prinzipien für jedes Biom

1. **Das Bett ist das Biom.** Ein dichtes, tonales Bett auf dem Tonvorrat trägt den Ort. Gespielte Töne
   kommen dazu, sie sind nicht die Hauptsache.
2. **Vier Schichten:** Bett (schwebende Teiltöne), Luft (gefärbtes Rauschen, kommt und geht), Tupfer
   (seltene, leise, hohe Ereignisse aus der Natur), Töne (die Klangquelle, wenige je Minute).
3. **Ein Grundton für alles.** Bett, Töne, Resonanzen und DRONE teilen den Tonvorrat; so verschmelzen die Schichten.
4. **Zwei Uhren.** Unten ständiges Wogen (0,5–2 s), oben langsames Wetter (8–120 s).
5. **Breite aus den Quellen, nicht aus dem Hall.** Links und rechts bekommen eigene Phasen und Modulatoren.
   Das Wogen muss in Mono erhalten bleiben, weil die zwei Lautsprecher dicht beieinander stehen.
6. **Gleichmaß.** Kein Aufbau, kein Höhepunkt; das Interesse entsteht aus der Bewegung im Kleinen.

Dunkelheit ist kein Mangel: Die Referenz ist dunkler als jedes unserer Biome. Die frühere Regel
„Schwerpunkt 250–1200 Hz“ ist gestrichen.

## Grenzen des Geräts (Annahmen, nicht gemessen)

- Der Hochpass bei 91 Hz und die 40-mm-Treiber geben den Bass der Referenz (75 Hz) nicht wieder. Das Bett
  beginnt deshalb bei D3 (147 Hz); tiefere Anteile sind nur für Kopfhörer.
- Laut Spezifikation (`field_ambience_pcb_SPEC_v0.7.md`) hat der Treiber CMS-402811-28SP seine Resonanz bei
  F0 = 450 Hz (Zweitquelle AS04008PS: 380 Hz). Darunter fällt jeder Treiber steil ab, eine geschlossene Kammer
  verschiebt die Grenze eher nach oben. Über die eingebauten Lautsprecher kommt vom Bett bei 150–300 Hz daher
  deutlich weniger an als über Kopfhörer. Wie viel, ist ohne Thiele-Small-Daten und Messung nicht bekannt.
  Gegenmittel in Software: der Baustein „Virtueller Bass“ (siehe Entscheidungspaket, Effektkatalog).
- Die Stereobreite wirkt über Kopfhörer voll, über die Lautsprecher kaum. Dort tragen Wogen und Bewegung.
- Mehr Oszillatoren kosten Rechenzeit: Ein Bett aus 6 Tönen mit je 2 Stimmen und 3 Teiltönen sind 36 Sinus
  je Kanal. Das ist als Tabellen-Oszillator machbar, muss aber in den Lasttest.

## Hörskizzen, Fassung 3

Vorgabe des Besitzers nach Fassung 2: keine hässlichen Glocken, kein starkes Rauschen, es ist ein Synthesizer.
Fassung 3 hat nur harmonische Töne mit weichem Anschlag und Rauschen nur als Hauch.

`tools/world_sketches/sketch_worlds.py` baut CAVE, DESERT und RAINFOREST (bis 17.11. DUNES und JUNGLE) nach diesen Prinzipien (40 s, numpy,
**kein Firmware-Klang**, gemessen, nicht gehört):

| Skizze | L/R-Korrelation | Seite gegen Mitte | Schwerpunkt | 85 % unter | Schwankung des Betts |
|---|---|---|---|---|---|
| CAVE | −0,05 | +0,4 dB | 257 Hz | 296 Hz | 2,7 dB |
| DESERT | 0,20 | −1,8 dB | 319 Hz | 441 Hz | 4,5 dB |
| RAINFOREST | −0,04 | +0,3 dB | 299 Hz | 436 Hz | 4,2 dB |
| Referenz | 0,16 | −1 dB | 240 Hz | 345 Hz | 8 dB (über 15 min, mit Einblendung) |

Energie über 2,5 kHz: CAVE −40 dB, DESERT −38 dB, RAINFOREST −53 dB (Referenz etwa −32 dB).
Breite und Dunkelheit treffen die Referenz. Das Wogen ist etwa halb so tief; ob das reicht, entscheidet das Ohr.
