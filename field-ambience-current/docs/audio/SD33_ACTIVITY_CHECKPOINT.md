# SD33 — tatsächliche Activity-Grenzen und Hörvergleich

Stand 2026-10-11. Diese einzelne Arbeitseinheit bereitet die Activity-Abnahme
vor und benennt einen konkreten Softwarebefund. SD33 bleibt offen: Klangfarbe,
Raum und musikalischer Nutzen brauchen weiterhin Hörentscheidungen.

## Ergebnis

WOODLAND und HIGHLANDS ändern bei Activity 0 / 0,5 / 1 ihre tatsächlichen
Übergaben beziehungsweise Pausen. Tonfolge, Besitzer, Velocity, Halteabsichten
und gehörte Phrasenerinnerung bleiben erhalten. Activity 0 spielt weiter.

COAST reagiert derzeit zu schwach. Bei beiden geprüften Seeds entstehen bei
allen drei Werten 47 Einsätze und fünf vollständige Phrasen vor Stop328s.
Die erste erinnerte Rückkehr liegt bei Seed1234 zwischen178,23 und178,38s,
bei Seed42 zwischen178,91 und179,31s. Kleine Unterschiede im tatsächlichen
Retry-/Startverlauf sind vorhanden; dies ist keine behauptete PCM-Gleichheit
zwischen verschiedenen Activity-Werten und keine ausreichende musikalische
Reglerwirkung. Quellenende, Besitzer und harmonische Ausklangzulassung begrenzen
den Verlauf stärker als die bisherige nominelle Abstandskennlinie.

| World | Seed1234 Einsätze bei 0 / 0,5 / 1 | Seed42 Einsätze | erste erinnerte Rückkehr bei Seed1234 |
| --- | --- | --- | --- |
| COAST | 47 / 47 / 47 | 47 / 47 / 47 | 178,31 / 178,23 / 178,38s |
| WOODLAND | 33 / 39 / 41 | 32 / 39 / 41 | 185,01 / 150,38 / 145,67s |
| HIGHLANDS | 32 / 37 / 43 | 32 / 37 / 43 | 254,44 / 223,08 / 192,11s |

Einsätze werden beim wirklichen DSP-Start gezählt, einschließlich gemeinsamer
Rollen und gleichzeitig gestarteter Paare. Diese Zahlen sind keine
Anschlagsdichte pro Sekunde und keine Qualitätswertung. Über328s sind auch die
jeweils anders entwickelten Phrasen und Pausen enthalten.

Die längste echte HIGHLANDS-Quellenpause beträgt bei Seed1234
18,83 / 13,99 / 9,16s, bei Seed42 19,00 / 14,13 / 9,30s. Die nominelle
Pausengrenze bleibt mindestens8s und beginnt nach wirklichem Besitzerende.
WOODLAND bleibt eine lange Saite mit ungleichen Haltezeiten; höhere Activity
verkürzt ihre bestehenden Halteabsichten nicht.

Nächste einzelne Arbeitseinheit: die COAST-Kennlinie für Abstände und Ruhe
kalibrieren. Zuerst die hörbare Wirkung trotz tatsächlicher Quellen-/Tailgrenzen
herstellen und erneut messen; gehaltene Stimmen, gemeinsame Akkordtöne und
sichere Zulassung bewahren. Die jetzige schwache Wirkung wird nicht abgehakt.

## Unveränderter Klangstand

Exakter Ausgangspunkt ist PR154, Commit
9f9aafc3fc44ba53a8496f6ec8e501e6efdd4def, Tree
31a3a0a1511c63d637ce12227bca133cae7086cd. Seine Firmwareinputs entsprechen dem
vollständig geprüften PR153-Klangstand. Der lokale Bytevergleich erfasst256 getrackte Dateien aus src, include,
CMakeLists und cmake; sie sind unverändert.

Spec0.8, Reference als CMake-Default, Product als Kandidat, JournalOFF.
Normale echte Engine, D-major/Equal, Seeds1234 und42. Activity ist der einzige
veränderte Klangparameter. Color/Attack/Release/Room0,5, Volume0,6, Nature0.
Drei globale Besitzer inklusive Releases, WOODLAND höchstens zwei. Bestehende
Quellen, Room, DSP, Phrasengedächtnis und Slot-/Tailregeln bleiben wirksam.

Bestehende Kennlinie: nominelle Abstände mit1,35−0,70*Activity. Die wirklichen
Starts warten zusätzlich Besitzer, Release und harmonische Room-Tails ab.
Einige Phrasenabschlüsse besitzen eine eigene Ruheabsicht. HIGHLANDS skaliert
seine nominelle12–14-s-Pause ebenfalls, mit8-s-Untergrenze. Deshalb sind
nominelle Abstände keine Zusage derselben Änderung der tatsächlichen Starts.

## Drei kurze Hördateien

Je World38,5s, stereo PCM16/44100Hz,1697850Frames,6791444Bytes:

- AMBIENT_COAST_Activity_0_50_100_38p5s.wav
- AMBIENT_WOODLAND_Activity_0_50_100_38p5s.wav
- AMBIENT_HIGHLANDS_Activity_0_50_100_38p5s.wav

| Dateiposition | Activity |
| --- | --- |
| 0–12,5s | 0, langsam |
| 13–25,5s | 0,5, Default |
| 26–38,5s | 1, dichter |

Alle drei Abschnitte einer World verwenden denselben festen Gain. Ruhige
Abschnitte werden nicht separat angehoben. Zwei0,5-s-Vergleichslücken und
40-ms-Schnittränder kennzeichnen eine Montage getrennter fester Reglerwerte.
Die Dateien simulieren keinen kontinuierlichen Reglerwechsel.

COAST und WOODLAND verwenden denselben echten Zeitabschnitt8–20,5s.
WOODLANDs nächster Rollenwechsel liegt bei18,93 / 14,03 / 13,73s.
COASTs nächstes Paar beginnt bei12,15 / 12,11 / 12,06s: bewusst ein dokumentierter
schwacher Vergleich, kein behauptetes gelungenes Langsam-/Dicht-Verhalten.

HIGHLANDS verwendet34–46,5s derselben drei tatsächlichen Läufe, damit Pause und
zeitlich verschobene Rückkehr sichtbar beziehungsweise hörbar werden.
Der langsame Abschnitt enthält eine echte Ruhephase und kann sehr leise
beziehungsweise quantisiert ganz still sein. Das ist kein fehlgeschlagener
Download und keine künstlich eingesetzte Pause. Dieselbe feste Verstärkung
gilt auch dort; exakte Stille bekommt null-LUFS/Peak und exact_silence=true.
Room-Tails dürfen nach dem Quellenende weiterhin ausklingen.

Das Paket ist für eine konkrete Hörwahl vorbereitet. Es belegt keine
Beruhigung, bessere Musik oder endgültig geeigneten Reglerbereich.

## Prüfung

tools/review_activity_comparison.py kompiliert den wirklichen Product-Core
und tools/render_activity_comparison.c mit C11/O2/Wall/Wextra/Werror.

- 576 World/Key/Collection/Seed-Cases über je acht vollständig gehörte Phrasen:
  33792 Intents und1536 Pausenabsichten. Min/default/max verändern keine
  Tonhöhe, Rolle, Velocity oder nominelle Haltezeit; Vorschläge bleiben rein,
  gehörte Memory/Returns und Timerwrap bestehen.
- 18 vollständige360-s-Fälle: drei Worlds × zwei Seeds × drei Activity-Werte.
  Dazu je World ein weiterer360-s-Lauf mit Live-Wechseln bei10/32/64/96/128/160s
  auf0/1/0,5/0/1/0,5. Jeder Fall läuft bei64 und512Frames auf demselben10-ms-
  Main-Raster. Insgesamt42 vollständige Performances beziehungsweise252min PCM.
- Vollständige WAVs, Note-on/off- und Besitzer/Phrasen-/Reject-CSV sind für
  dieselben Fälle bei64/512 bytegleich. Verschiedene Activity-Werte dürfen
  und sollen andere tatsächliche Zeitverläufe haben.
- Gehörte Ton-/Besitzer-/Velocity-/Phrasenfolge stimmt im gemeinsamen
  vollständigen Start-Präfix aller drei festen Werte überein. Reale Haltezeiten
  der Rollen6/7 sind ebenfalls unverändert innerhalb eines10-ms-Ticks.
  Die gemeinsame gehaltene Rolle15 kann abhängig von Partnerstarts länger
  stehen; sie wird deshalb nicht fälschlich als feste Dauer behandelt.
- Live-Wechsel erhalten ebenfalls die gehörte Folge und41/36/28 abgeschlossene
  Haltezeiten bei COAST/WOODLAND/HIGHLANDS exakt: größte Differenz0s.
  Kein unmittelbarer Note-on durch den Setter, alle vier Entwicklungsstufen
  und mindestens eine vollständig erinnerte Rückkehr werden erreicht.
- Besitzergrenzen3/2/3, richtige Sammlung/Equal-Frequenzen, Gap<90s,
  kleiner DC, Mono>0,65 und TruePeak≤−6dBFS bestehen. NaN/Limiter0.
  GenerateStop328s endet natürlich; spätestens336,84s sind alle Besitzer frei.
  Die letzte Sekunde359–360s ist in allen21 Fällen exakt still.
- Jede vollständige Aufnahme wird nach der Messung erneut auf Länge/Hashes
  geprüft. Lange PCM bleiben temporär; exportiert werden drei kurze WAVs,
  Messwerte und vollständige musikalische CSV. Finale Kopien sind bytegeprüft.

Reproduktion im Firmwareverzeichnis:

```bash
OPENBLAS_NUM_THREADS=1 python3 tools/review_activity_comparison.py build-activity-comparison
```

CI lädt das exakte Baseline-Commit auch beim flachen Checkout und exportiert
kurze WAVs, SD33_ACTIVITY_METRICS.json sowie vollständige Ereignis-/Statusdaten.
Quell-/Header-/Tool-/Audio-Hashes und genaue Pegel stehen im selben JSON.

Dieser reine Tool-/Hörcheckpoint beansprucht keinen neuen vollständigen
Host-Runner oder ARM-Build. Die identischen Firmwareinputs sind durch die
erfolgreichen PR153-/PR154-Checks einschließlich H743 gedeckt:
Flash195928B, D1RAM65664B, D2RAM60808B, reservierter DTCM-Stack16384B,
größter einzelner Core-Compilerframe192B. Geräte-CPU/ISR/verschachtelter
Stack/DAC/Verstärker/Lautsprecher und Storage-/ECC-Abnahme bleiben offen.

Dieselbe To-do bleibt24 geschlossen/30 offen. SD33, Quellen-/World-/Room-/
Nature-Hörwahl bleiben ohne Haken. Die nächste Arbeitseinheit ist nur die
COAST-Activity-Kalibrierung; Color und weitere Punkte folgen danach.
