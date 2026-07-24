# Was macht unser Gerät zu einem SYNTHESIZER?

Designanalyse, r19.58. Ausgangsfrage: *„Ein Behringer TD-3 spielt mit Envelope,
Attack, Decay, Cutoff, Resonance. Was macht unser Ambient-Synth — und was fehlt
ihm noch, um ein echter Synth zu sein?"*

---

## 1. Was einen Synth zum Synth macht

Nicht die Klangerzeugung. **Ein Synthesizer ist ein Instrument, bei dem die Hand
den Klang in Echtzeit formt** — und zwar so, dass man hört, *was* man tut.

Beim TD-3 sind das fünf Regler: `Cutoff · Resonance · EnvMod · Decay · Accent`.
Der Witz: **man spielt den Filter, nicht die Noten.** Die Tonfolge ist fast egal,
die Musik entsteht aus der Handbewegung am Cutoff. Deshalb ist der TD-3 ein
Instrument und kein Abspielgerät.

Daraus lassen sich drei Säulen ableiten, die jeder Synth braucht:

| Säule | Bedeutung | TD-3 |
|---|---|---|
| **SHAPE** | Wie der Ton *anfängt und aufhört* | Decay, Accent |
| **COLOUR** | Wie der Ton *klingt* — und wie man ihn verbiegt | Cutoff + **Resonance** |
| **MOTION** | Wie sich der Ton *von selbst bewegt* | EnvMod (Env → Cutoff) |

**Resonanz ist dabei nicht ein Effekt unter vielen.** Ein Filter ohne Resonanz
ist ein Klangregler. Erst die Resonanz macht ihn zur *Stimme* — sie singt,
quäkt, schreit. Das ist der Unterschied zwischen „dumpfer" und „gespielt".

---

## 2. Was WIR heute haben — ehrlich

| Was | Status |
|---|---|
| 5 Welten, Makros (Space/Atmos/Motion/Age/Echo/Blur/Shimmer) | ✅ aber das sind **Mix- und Effektparameter**, keine Synthese |
| Brightness-Encoder | ⚠️ nur ein Cutoff-*Offset* — **ohne Resonanz** = Klangregler |
| 5 Charakterstimmen (Pad/String/Ember/Bowed/Horn) | ✅ aber jede hat **eine feste Persönlichkeit** |
| Harmonie (Key/Mode/Colour/Bass), Generative, FX-Kette | ✅ stark — das ist unser Alleinstellungsmerkmal |
| **Resonanz** | ❌ existiert nirgends in der Engine-API |
| **Hüllkurven (Attack/Release)** | ❌ in jeder Stimme **fest einkompiliert**, nicht spielbar |
| **Modulations-Routing** | ❌ LFOs existieren, sind aber **fest verdrahtet** |
| `dsp_ladder.c` (Moog-Ladder-Filter, 111 Zeilen) | ❌ **gebaut und nie angeschlossen — toter Code** |

**Fazit:** Wir haben heute einen exzellenten *generativen Klangkörper* — aber die
Hand des Spielers formt den Klang nicht. Man **wählt** Klänge (Welt, Voice),
statt sie zu **machen**. Das ist der Unterschied zum Synth.

---

## 3. Was für einen AMBIENT-Synth die richtigen Regler sind

Ein Acid-Synth lebt vom Filter. Ein Ambient-Synth lebt von **Zeit und Raum**.
Die Übersetzung der drei Säulen auf unser Instrument:

### SHAPE → `ATTACK` / `RELEASE` (die wichtigste Lücke)
Für Ambient ist die Hüllkurve *das* Ausdrucksmittel. Dieselbe Stimme wird durch
Attack zu zwei völlig verschiedenen Instrumenten:
- Attack 5 ms = Pluck, perkussiv, rhythmisch
- Attack 2 s = Swell, Pad, atmend

Heute hat jede Stimme genau eine feste Hüllkurve → jede Stimme kann genau eine
Sache. Mit spielbarem Attack/Release werden aus 5 Stimmen ein **Kontinuum**.
Das ist der grösste Zugewinn pro Zeile Code.

### COLOUR → `RESONANCE` (das billigste Upgrade)
Der Moog-Ladder liegt fertig im Repo und ist nicht angeschlossen. Brightness +
Resonanz zusammen = ein *gespielter* Filter statt einer Tonblende. Bei hoher
Resonanz + langsamem Filtersweep entsteht genau das singende, sich öffnende
Ambient-Timbre (Vangelis/Eno-Territorium).

### MOTION → eine kleine Modulationsmatrix
Nicht 20 Ziele. Zwei Quellen (**LFO**, **Hüllkurve**) auf wenige Ziele
(Cutoff, Pitch, Amplitude, Pan). Das TD-3-Äquivalent ist EnvMod — bei uns wäre
das „Filter atmet mit jeder Note" bzw. „Klang wandert von selbst".

---

## 4. Woraus wir bei den Fremdcodes wirklich lernen können

Der Wert des STM32-Packs liegt **nicht** in DSP-Algorithmen — unsere sind gut
und eigenständig. Er liegt im **Parameter-Design**:

- **DaisySP (MIT)** — saubere, lesbare Referenz für die kanonischen Bausteine
  (`adsr`, `moogladder`, `svf`) und vor allem für deren **Wertebereiche und
  Glättung**. Genau das, was wir für Attack/Release/Resonance brauchen.
- **Mutable Instruments (MIT, STM32F)** — Émilie Gillets Code ist der Goldstandard
  für **musikalisches Parameter-Mapping**: wie ein Regler auf eine *wahrgenommene*
  Skala abgebildet wird (exponentiell, mit sinnvollen Endpunkten).

> **Die eigentliche Lektion:** Was Hardware-Synths gut anfühlen lässt, ist selten
> der Algorithmus — es sind **Mapping, Glättung und Wertebereiche**. Ein
> Attack-Regler von 1 ms bis 4 s *exponentiell* fühlt sich musikalisch an;
> derselbe Bereich linear fühlt sich kaputt an.

Abwandeln statt kopieren: Prinzip verstehen → eigene Implementierung. Das ist
sowohl lizenzrechtlich sauber als auch technisch besser, weil es zu unserer
Engine passt (Blockrate, Ramping, Hot-Path-Regeln).

---

## 5. Vorschlag: Reihenfolge nach Wirkung/Aufwand

1. ~~**Ladder-Filter anschliessen + `RESONANCE` als Parameter**~~ ✅ **ERLEDIGT (r19.59)**
   Der Moog-Ladder sitzt jetzt als **Stereo-Masterfilter auf dem Pad-Bus** (nicht
   pro Stimme: 12 Resonanzspitzen wären Matsch, und 4x-Oversampling x12 hätte das
   IRQ-Budget gesprengt — 2 Instanzen statt 12). BRIGHT fährt den Cutoff,
   RESONANCE (Menü-Slot, 0..100 %) lässt ihn singen. Bei 0 % vollständig
   umgangen → Klang wie vorher. Messung: die dominante Frequenz wandert bei 90 %
   Resonanz mit dem Sweep von 872 → 3607 → 1104 Hz (ohne Resonanz statisch bei
   ~450 Hz). Der Filter ist damit hörbar geworden statt eine Tonblende zu sein.
2. **`ATTACK` / `RELEASE` global spielbar** — jede Stimme bekommt Hüllkurven-
   Skalierung statt fester Zeiten. Verwandelt 5 Stimmen in ein Kontinuum.
3. **Mini-Mod-Matrix** — LFO + Env auf Cutoff/Pitch/Amp. Bringt Eigenleben.

**Bedienung:** Wir haben 4 Encoder (Drive, Bright, Display, Volume) + SHIFT.
Die neuen Parameter gehören auf die **SHIFT-Ebene der bestehenden Encoder**
(z. B. SHIFT+Bright = Resonance, SHIFT+Drive = Attack), damit die Oberfläche
nicht wächst — Teenage-Engineering-Prinzip: wenige Regler, mehrere Ebenen.

---

## 6. Was uns von anderen Synths unterscheiden soll

Wichtig: Wir bauen **keinen** weiteren subtraktiven Synth. Unsere Identität ist
und bleibt der **generative, harmonisch sichere Klangkörper** — die Welten, die
Harmonie-Engine, die Effektkette. Die drei Säulen oben sind kein Kurswechsel,
sondern das, was fehlt, damit der Spieler diesen Klangkörper **formen** kann
statt ihn nur auszuwählen.

Ziel: *Ein Instrument, das von selbst schön spielt — und unter der Hand sofort
reagiert, wenn man eingreift.*
