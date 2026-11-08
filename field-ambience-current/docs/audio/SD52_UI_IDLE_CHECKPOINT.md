# SD52 — H743-Bedienung und Displayruhe

Stand2026-10-20. Die vorhandene Display-/Button-/Encoder-/LED-Bedienung ist
auf den konsolidierten [Softwarevertrag](SD51_SOFTWARE_CANDIDATE.md) angebunden.
Sound0.10/SCN7, lange WOODLAND, gemeinsame Quellen-/Roomverträge unverändert.
Dieser Checkpoint schließt die Softwareanbindung SD52; ein neues grafisches
Layout ist darin nicht als fertig entworfen behauptet. Reale Bedien-/Last- und
Ausgangsabnahme bleiben SD48/SD49, finaler Klang SD51/SD50.

## Verhalten

| Situation | Wirkung |
|---|---|
| 900000ms ohne menschliche Eingabe | Displaybacklight und alle15 Status-LED-Kanäle blenden mit vorhandener120-ms-Rampe auf0; neue LCD-DMA-Frames pausieren |
| Musik, Generate, Tail, MIDI, Batterie, Jack/VBUS | Zählen nicht als menschliche Aktivität; Audio-DMA, Musikzeit, Besitzer und Targets laufen unverändert weiter |
| Gedrückte physische Taste | Verhindert idle bis Release; danach beginnt die volle15-Minuten-Frist neu |
| Erste Dreh-/Druckgeste im Dunkeln | Weckt die Anzeige, führt keine Bedienaktion aus; auch kein Clear/Mute/Save/Note-on/Parameterinkrement |
| Wake-Release und zeitversetzte Eingänge | Loslassen verbrauchter Tasten bleibt verbraucht;30ms Guard für weitere queued/debounced Signale einschließlich des indirekten Volume-Drucks |
| Weitere bewusste Bedienung | Gleiche vorhandene Audiooperationen und gemerkte Parameter; Hintergrund-Generate und latched Besitzer werden nicht neu gestartet |
| Beleuchtung beim Wake | Gemerkter Backlightwert und aktueller LED-Status werden eingeblendet; ein LCD-Redraw wird angefordert, bestehende SPI-DMA wird normal fertig |

`ui_idle.c` besitzt nur Main-State: letzte Aktivität, Wakezeit, physisch unten,
verbrauchte Tasten und Quiet-/Guardflags. Kein Engine-/Audio-/Parameteraufruf,
keine Allocation, Suche oder Wartefunktion. Init berücksichtigt bei Boot schon
gehaltene Tasten; EN4 wird zuvor aus dem echten Expanderpegel initialisiert,
sein bei Boot eingequeueeter Druck und sein späterer Release wirken nicht als Mute.
Die zusätzliche tatsächliche H743-Initialisierung ist direkt geprüft.
`poll_human_inputs()` ist aus dem tatsächlichen H743-Mainloop
herausgezogen; der Loop ruft genau diese Funktion weiter auf. Vorhandene
Encoder-, MCP-, Knob-, Scene-, Gesture- und Cell-Routen bleiben darin erhalten.
Jack/VBUS umgehen das Human-Gate; EN4 geht einmal durch die Encoderqueue.
`leds_set_quiet` verändert nur Lichttargets, nicht musikalische Zustände.

## Softwareprüfung

Die Fixture kompiliert die unverändert aus H743 extrahierten tatsächlichen
Dispatch-/Routingfunktionen mit dem echten Product-/Menu-/Knob-/Params-/
Controls-/Scene-/Gesture-/LED-Code. Nur Hardwarequeues und Mainclock werden
injiziert. Kein Ersatzsynth oder nachimplementierter Bedienrouter.

21 Wakewege: fünf Cells, fünf Modifier, vier Encoderdrücke, vier Drehregler,
simultane Eingabebatches, indirekter EN4-Druck und Drehung plus verzögerter
EN4-Druck. Alle drei Worlds,64/512-Obergrenzen; in512-Konfiguration tatsächlich
441 Frames pro10-ms-Maintick. Normale bzw. wrappende Mainclock; derselbe Seed42.
Keine pauschale64/512-PCMgleichheit oder echte512-Frame-DMAbehauptung.

Pro Prüfmodus126 ganze12-s-PCMvergleiche gegen je unberührten wachen Verlauf,
66679200 Stereo-Frames bytegleich; tatsächliche On/Off-Zähler gleich.
Danach identische Volume-/Generate-/Zellenaktionen funktionieren weiter.
Kein Mute, Menüshort/-long, Scene-Save, Shift/Hold oder Parameterjump beim Wake;
alle Lichter nach Quiet exakt0 und Backlight nach Wake wieder70%.
Getrennte Grenztests prüfen900000ms−1/exakt, gehaltene/initial gehaltene/
doppelte Tasten, verspätete Releases,30-ms-Guard und uint32-Wrap.

C11/O2/Wall/Wextra/Werror sowie ASan/UBSan prüfen dieselben Fälle. Metrik und
Quellhashes: [SD52_UI_IDLE_METRICS.json](SD52_UI_IDLE_METRICS.json).
Insgesamt264 vollständige Render/52min48s Audio in beiden Prüfmodi; komplette
PCM bleibt temporär, kein langes WAVpaket. Kein NaN-/Limiterereignis.

## ARM-Builds

Product und Reference mit GCC13.2.1 vollständig für Cortex-M7 gelinkt.
Product: Text197644B, Data240B, BSS142704B, BIN197892B;
BIN-SHA256 `f2560485b552cff4ca93f73d8c9f5c3cd29eb583f65ee50296305276f6c0cd6e`.
Reference: Text257744B, Data364B, BSS794640B, BIN258116B.
Der reale Product-Linkaudit bestätigt ausgeschlossene Legacy-DSPpfade,
einen D2-Raum, ausgerichtetes D1-Audio-DMA und das Bank1-Imagelimit.
Der neue Main-Idle-State ist im ARM-ELF20B, der LED-Quietstate1B;
Gesamtlayout/Padding oder Stackreserve werden daraus nicht hochgerechnet.
Keine IRQ-/Highwater-/CPUabnahme aus dem Compilerbuild.

## Reproduzieren und Grenzen

`python3 tools/review_ui_idle.py <output> --sanitize` prüft auch259 bestehende
unveränderte Firmwareinputs gegen Vor-UI-Head b29246fde0c66618bbfc25742f929dc1f362ebf6.
Vier bestehende Inputs ändern sich: H743-Main, CMake, LEDs C/Header;
zwei neue UI-Dateien. [SD51_SOFTWARE_INPUTS.json](SD51_SOFTWARE_INPUTS.json)
identifiziert die gesamte265-Dateien-Momentaufnahme samt Buildflags.
Klangquellen/Engine/Grammatiken/Params/Scenes/DMA/Flash/Linker bleiben gleich.

Der normale Hostrunner enthält die aktuelle native UI-Prüfung; ein eigener
CI-Job prüft zusätzlich Sanitizer und Quellgleichheit. Fünf zuvor aktuelle
strikte Historienjobs sind auf den vollständig grünen PR179-Head gepinnt und
als archiviert bezeichnet; ihre Guards bleiben unverändert. Die aktuellen
Host- und H743-Buildjobs prüfen weiterhin den neuen Head. Kein archivierter
Beleg wird als Ausführung auf aktuellem UI-Code bezeichnet.

Gerätetest: Generate und manuell gehaltene Quellen mit langfristigem Room
laufen lassen,15min keine Handbewegung; Anzeige wird dunkel, Klang läuft.
Nacheinander alle Wakewege einschließlich EN4 und simultaner Eingabe prüfen;
erst folgende bewusste Geste wirkt. Jack/VBUS/MIDI sollen nicht wecken.
Danach denselben Build unter Display/LED/MIDI/Storageverkehr für vollständige
DMA-IRQ-Spitze/Stack/Deadline und reale Ausgabe messen. Hostclocktests sind
kein realer15-Minuten-, Stromverbrauchs-, Schalterbounce- oder Timingpass.

Default-Live-Save bleibt synchron, Journal OFF. Der isolierte SD41-Benchbuild
ist vorbereitet. Reale ECC-/Power-cut-Abnahme bleibt Voraussetzung seiner
Produktaktivierung; SD41 und damit der abhängige SD47-Abschluss bleiben offen.
