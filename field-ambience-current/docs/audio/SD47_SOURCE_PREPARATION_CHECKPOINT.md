# SD47 — tatsächliche Quellenvorbereitung unter Audio-Unterbrechung

Stand2026-10-17, einzelne Verifikation auf PR171/Commit
`7af108cb02c80425c959b58dc8b58ad5299c2465`. Keine Firmwareänderung:
alle263 vorhandenen Firmware-/Header-/HAL-/Linker-/Startup-/CMakeinputs
bleiben bytegleich. Quellen, Hüllkurven und lange WOODLAND-Holds erhalten.
SD47 bleibt für das abschließende übrige Spitzenlastinventar offen.

## Tatsächlicher Gegenvergleich

Der Reviewrunner fügt ausschließlich in temporäre Kopien von `bowed.c`,
`horn.c` und `pluck.c` Beobachtungsaufrufe ein. Der Test bindet diese echten
Quellen ein, linkt echten DSP/SHAPE und hält Main an einer gewählten Stelle an.
Der Callback rendert eine oder drei vollständige Audiohälften mit64/512
Frames. Main läuft während einer Audiohälfte niemals weiter.

Referenz vor Veröffentlichung: alte Quelle laufen lassen, Audiohälften
rendern, dann neue Quelle vollständig vorbereiten und weiter rendern.
Unterbrechungsfall: alte Quelle laufen lassen, neue Quelle teilweise
vorbereiten, im Callback Audiohälften rendern, Vorbereitung abschließen und
weiter rendern. Nach Veröffentlichung: beide Vergleichswege veröffentlichen
vor den Audiohälften. Die gesamten Float-Dry-L/R- und Send-L/R-Daten sind
bytegleich, einschließlich1024 nachfolgenden Frames. Kein Ersatz-DSP und
keine Prüfung nur auf ähnliche Pegel oder einzelne Samples.

Vor Veröffentlichung zeigt die reale Besitzermaske nur den alten Besitzer;
danach enthält sie die neue Quelle. Ein Audio-Release für einen anderen
Besitzer verändert die Vorbereitung nicht. Frühe natürliche Retirementfälle
werden durch den vollständigen Verlauf verglichen, nicht durch angenommene
feste Releasezeiten. Poolvoll/Ownerduplikat/NaN/Nullamplitude verändern weder
die laufende PCM noch erreichen sie neue Vorbereitungshooks.

## Ergebnisse

| Quelle | Fälle | tatsächliche Audiohalb-Unterbrechungen |
|---|---:|---:|
| Bowed |161|320|
| Horn |161|320|
| Pluck |4283|4442|
| Gesamt |4605|5082|

21047797 Assertions; C11/O2/Wall/Wextra/Werror und ASan/UBSan liefern dasselbe
positive Ergebnis. Zehn markierte Vorbereitungs-/Publikationsgrenzen pro
Quelle. Dazu bei Pluck **jede Iteration** aller drei echten Pässe (Füllen,
Zentrieren, Normieren), sowohl bei60Hz als auch146,83238Hz. Die übrigen
Grenzfälle umfassen zusätzlich440/8000Hz,64/512 und eine/drei Audiohälften.

Uninstrumentierte Originalquelle gegen instrumentierte Quelle ohne
Unterbrechung: vier komplette1024-Frame-Spuren je Familie, je65536 Floatbytes,
exakt gleich. Die Instrumentierung verändert somit auch im Normalablauf
keine beobachtete Quellenausgabe. Keine Hooks oder neuen Testbuffer in der
Firmware; Beobachtungskopien werden nach dem Test entfernt.

Der normale Hostrunner ruft die Fixtures ohne historischen Git-Audit auf
(`--fixture-only`); er braucht dafür keine alten Gitobjekte. Der eigenständige
Review/CI lässt dieses Flag weg und prüft zusätzlich die263 bytegleichen
Firmwareinputs gegen PR171. Sanitizer- und normaler Hostmodus sind lokal
erfolgreich ausgeführt. Die unveränderte gesamte Basissuite war bereits auf
PR171 vollständig grün; hier wird keine erneute Gesamtsuite behauptet.

## Begrenzte tatsächliche Vorbereitungsarbeit

Die alte Spezifikation `im Produkt N≤315` galt nur für manuelle Admission.
Generate akzeptiert Hz≥105, daher ist die konservative feste Obergrenze dort
N≤420, n=floor(N)+1≤421. Das ist eine Dokumentationskorrektur, keine
Verlängerung oder Transposition von Tönen.

| Vertrag | Hz mindestens | N höchstens | n höchstens | Bufferbearbeitungen höchstens |
|---|---:|---:|---:|---:|
| Product manuell |140|315|316|1656|
| Product Generate |105|420|421|1866|
| öffentliche Pluck-API |60|735|736|2496|

Bufferbearbeitungen zählen die1024 Fülliterationen plus n Zentrier- und n
Normieriterationen. Im Füllpass höchstens acht harmonische LUT-Moden je
aktiver Periode; Modenvorbereitung höchstens acht Koeffizienten. Feste
32-B-Koeffiziententabelle auf Main-Stack, vorhandener1024-Float-Ring je Stimme.
Kein zusätzlicher Quellenbuffer, Heap, I/O oder Retrypfad durch diesen Test.

Bowed/Horn bereiten je einen festen Pending-Record mit vier/zwei SVF-Filtern
vor. DSP-Koeffizienten und SHAPE-Getter bleiben an ihren bestehenden Stellen;
SHAPE-Skalen sind vorab gecacht. Die Audioseite übernimmt erst nach atomarer
Publikation. Keine Quellenanregung wird in den Audio-IRQ verschoben.

## Grenzen und Reproduktion

`python3 tools/review_source_preparation.py build-source-preparation-review --sanitize`
im `firmware-c-next`. Der JSON-Proof
[SD47_SOURCE_PREPARATION_METRICS.json](SD47_SOURCE_PREPARATION_METRICS.json)
enthält genaue Original-/Test-/Reviewhashes und drei Normalspurhashes.

Dies sind gerichtete Unterbrechungen an tatsächlichen C-Grenzen und in allen
genannten Schleifeniterationen. Kein Beweis jeder Maschineninstruktion, kein
beliebiges paralleles C11-Threadmodell, keine Geräte-WCET oder Stack-High-water.
Unveränderte Firmwareinputs rechtfertigen keine erfundene neue ARM-Messung.
Der aktuelle H743-/Product-CI-Build bleibt dafür zuständig; reale Cache-/IRQ-/
Clear-/Filterspitzen bleiben SD48. Kein neuer Hör- oder Gerätehaken.
