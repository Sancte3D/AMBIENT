# Field Ambience — Sound Audit (schonungslos)

Messtechnische + klangliche Bewertung jedes Katalog-Samples (r19.50). Basis:
Spektralschwerpunkt (Helligkeit), Bandverteilung (low <200 Hz / mid / high
>2 kHz), Rauschanteil, Dynamik (Crest), Stereobreite, Flatness (tonal↔Rauschen).
Ziel: wissen, **was gut ist, was raus muss, was kaputt ist** — Prioritäten setzen.

---

## Die zwei GROSSEN Befunde (wichtiger als jeder Einzelsound)

### 1. Die Welten sind ~90 % mono Sub-Bass → dunkel, matschig, ununterscheidbar
Gemessen am vollen Engine-Output je Welt:

| World | Schwerpunkt | low % | mid % | Stereobreite |
|---|---|---|---|---|
| Alps | **99 Hz** | 93 | 7 | 0.01 |
| Open Sea | 93 Hz | 90 | 10 | 0.02 |
| Fjords | 132 Hz | 76 | 24 | 0.02 |
| Moss | 151 Hz | 78 | 22 | 0.03 |
| Desert | 98 Hz | 83 | 17 | 0.01 |

Der Spektralschwerpunkt liegt bei **93–151 Hz** — das ist Bass-Region. 76–93 %
der Energie sitzt unter 200 Hz (Drone + Bass, beide mono). Der ganze
melodische Charakter (mid) ist 7–24 % und wird vom Sub-Bass **maskiert**.
**Das ist DER Grund, warum alles ähnlich, dumpf und „billig" klingt.**

Folge: **Stereobreite 0.01–0.03 — praktisch MONO.** Die Betten (0.22–0.57) und
die Ambience (1.0) sind für sich breit, aber der dominante mono Sub-Bass zieht
L=R. Ein Ambient-Instrument, das mono und bassmatschig ist, kann nicht „würdig"
klingen. **Priorität 1.**

Fix-Richtung: Drone/Bass im Pegel deutlich zurück + Hochpass, Sub-Bass-Anteil
deckeln, mid/high anheben, Stereo der Betten/Voices durchlassen. Das allein hebt
die gefühlte Qualität am meisten.

### 2. Drei Welten teilen sich exakt dieselbe Ambience
`amb_alps`, `amb_fjords`, `amb_desert` sind **bit-identisch** (Schwerpunkt 956 Hz,
Breite 1.0, Rauschanteil gleich) — alle nur „Wind". Und dieser Wind ist mit
Schwerpunkt ~956 Hz + Flatness messbar **eher gefiltertes Rauschen als Wind**.
Fjords/Desert brauchen eigene Layer; der Wind selbst braucht mehr Körper/weniger
Zisch.

---

## Voices (Melodie-Instrumente)

| Voice | rms | Schwerpunkt | Crest | Rausch | Urteil |
|---|---|---|---|---|---|
| **bowed_opensea** | 0.077 | 573 | 13.0 | 0.00 | **BEHALTEN** — warm, fokussiert, sauber. Der Referenz-Charakter. |
| **bowed_fjords** | 0.078 | 517 | 13.4 | 0.00 | **BEHALTEN** — wie oben, dunkler. Gut. |
| **horn_alps** | 0.104 | 565 | 13.7 | 0.00 | **BEHALTEN (verdrahten)** — präsent, sauber, eigener Charakter. Noch PROTO. |
| **ember** | 0.027 | 339 | 16.2 | 0.00 | **SCHWACH** — leise (rms 0.027) und dumpf (339 Hz). Geht unter, wenig Charakter. Kandidat zum Streichen oder Aufwerten. |
| **pluck/string** | 0.033 | 1117 | 24.5 | **0.51** | **MITTEL** — hell, aber messbarer HF-Zisch (Karplus-Noise) + sehr spitz (Crest 24). Kann „billig" klingen. Entrauschen/abrunden. |
| **glass** | 0.051 | 655 | 14.5 | 0.00 | **RAUS** (beschlossen). FM-Glocke, inharmonisch-schrill. Auch intern als „Sparkle" genutzt → Ersatz nötig. |

## Beds (PADsynth-Bett pro Welt)
Alle sauber, warm, tonal (Flatness 0), Schwerpunkt 426–581 Hz, Crest ~11.
Breite variiert (Alps 0.22 eng … Open Sea 0.57 breit). **Solide Basis — behalten.**
Einziger Punkt: im vollen Mix zu leise gegen den Sub-Bass (siehe Befund 1).

## Ambience
- **opensea** — Schwerpunkt 106 Hz, 91 % low: warmer See-Hum, klar eigen. **Gut.**
- **moss** — Regen bringt HF (17 % high, Rausch 0.43); als Regen ok, grenzwertig zisch.
- **alps/fjords/desert** — identischer Wind, zu hell/rauschig. **Fjords/Desert: eigene Layer nötig.**

## Low
- **bass_root/deep** — reiner Sub (25 Hz, 100 % low, mono). Für sich ok, aber **Mitverursacher von Befund 1** (zu dominant im Mix).
- **drone** — 51 % low / 49 % mid, ok. Ebenfalls Pegel im Mix prüfen.

## FX (nach Harness-Fix jetzt korrekt)
- **tape / delay / blur / chorus / dream** — hörbar, funktionieren.
- **shimmer** — **real zu schwach** (nur 35 % Δ selbst bei voller Stärke). Fixen.
- **reverb** — Basis, ok. **swell** — der reverse-Swell war der „zschhh", jetzt aus.

---

## Priorität (Vorschlag)

1. **Sub-Bass-Dominanz + Mono fixen** (Befund 1) — größter Qualitätssprung.
2. **Glass raus** (beschlossen) + internen Sparkle ersetzen.
3. **Shimmer hörbar machen.**
4. **Alphorn in Alps verdrahten.**
5. **Ember + Pluck aufwerten oder Ember streichen.**
6. **Fjords/Desert eigene Ambience; Wind entzischen.**
7. **Pro-Welt-Charakterstimmen** (Moss/Desert) wie Open Sea.
