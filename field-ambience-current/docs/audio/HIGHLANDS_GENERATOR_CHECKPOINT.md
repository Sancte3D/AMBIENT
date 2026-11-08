# HIGHLANDS — weiter Akkord, gemeinsamer Ton, offene Pause, Rückkehr

Stand2026-10-08, Product-Kandidat/Spec0.6, auf dem langen WOODLAND-Stand PR149.
Ein einzelner Integrationscheckpoint. HIGHLANDS verwendet jetzt den wirklichen
Product-Generator; bisheriger44-s-Hostentwurf bleibt als Vergleich erhalten.
Horn-Quelle, Color, Quellenkalibrierung und normale Attack-/Release-Makros
sind unverändert. Keine neue Source, Begleitung, Raumkopie, UI oder Defaultwahl.

## Musikalische Rollen und tatsächliche Zeit

Seed1234/D-Dur: **D3/A4/F#4 → B3/D4/F#4 → offene Pause → D3/A4/F#4**.
Versetzte Einsätze und ungleiche Dauern. F#4 bleibt durch beide Partnerwechsel
in derselben wirklichen Stimme; kein Wiederanschlag, Still-Retune oder Steal.
Die beiden Partner beginnen erst nach Retirement ihres alten Besitzers.
Der zweite neue Partner plant per DSP-Ack das Ende des gemeinsamen Tons;
alle drei Quellen enden über ihren normalen, gespeicherten Horn-Release.

Der Main-Automat hat acht Startphasen und drei feste Rollen/Owner6/7/15.
Ein reiner Vorschlag verändert keine RNG-/Phasen-/gehörte Historie. Erst ein
bestätigter DSP-Start zählt und setzt Holds/Abstände. Eine abgewiesene oder vor
DSP abgebrochene Vorbereitung zählt nicht. Keine Ersatztonsuche, höchstens eine
Vorbereitung je HIGHLANDS-Tick. Drei gemeinsame Slots inklusive Releases,
16 Raum-/Pitchtails und7,2-s-Historiengrenze sind unverändert wirksam.

Basis-Holds6/7/0/7/7/7,5/7,2/7,2 s plus0..250 ms Seedabweichung;0 ist hier
der bis zum Partner-Ack gehaltene gemeinsame Ton. Geplante Zwischenräume
0,8/0,8/8,4/1 s, danach Pause, dann1/1 s in der Reprise. Activity skaliert
kommende Zwischenräume mit1,35−0,70×a, minimale Versatzlücke0,5 s.
Erst nach tatsächlichem Ende aller drei beginnt eine Quellenpause12..14 s
vor Activity, anschließend mindestens8 s, höchstens18,9 s. Keine geschätzte
Releasezeit wird von dieser Pause abgezogen. Room läuft weiter, Pitchtails
werden vor einer neuen Aufnahme weiterhin geprüft. Nach der Reprise erneut
vollständiges Quellenende und Pause; kein periodischer DSP-Reset.

Major kann den Zwischenbass zwischen verwandter Sext und unterer Quinte
variieren; Minor verwendet kleine Terz und untere Quinte. Seed verändert
kleine Akzente/Holds, kein beliebiges Arpeggio. Interne Core-MIDI45..75,
105..650 Hz, Major/Minor und Equal/5-limit Just ausdrücklich geprüft.
Manual und öffentliche World-Note-API behalten140..470 Hz/MIDI50..69.
Key/Collection/Tuning verwerfen generierte Absichten und lassen gehörte
Generate-Quellen natürlich enden, ohne manuelle Hz zu verändern. Stop,
Autoplay, Presence, Seed, World, Mute und Clear nutzen dieselben geprüften
Cancel-/Retirementpfade. Alte einzelne `world_grammar_t`-Angebote werden im
Product nicht mehr dispatcht; ihr reiner historischer Testhelfer bleibt.

## Echte65-s-Engine-Hörprobe

`render_highlands_generator.c` startet nur über Engine-APIs, mit10-ms-Main-
Raster und normalen Defaults: D-Dur/Equal, Seed1234, Activity/Color/Attack/
Release0,5, Volume0,6, Room0,24, Nature0. Kein direkt komponierter Quellenstart,
SHAPE-Bypass, temporärer DSP, Clear oder künstlicher Schlussfade.

| Gemessener Main-Ack-/Retirementpunkt | Zeit |
|---|---:|
| D3 / A4 / F#4 öffnen |0,01 /0,82 /1,63 s|
| B3 / D4 ersetzen die Partner |13,29 /15,05 s|
| erster Abschnitt: alle Quellen tatsächlich beendet |29,38 s|
| Quellenpause ohne neue Starts |13,86 s|
| D3 / A4 / F#4 kehren zurück |43,24 /44,25 /45,26 s|
| letztes tatsächliches Quellenende |59,65 s|

Generate wird7,8 s nach dem letzten Start gestoppt; bis dahin sind dessen
geplante Holds schon normal beendet. Raum kann leise weiterklingen.
Acht Starts, maximal drei Quellen inklusive Releases, kein Limiter/NaN.
Dry/Room-RAW-WAVs, PCM und Main-Ack-Traces sind bei64/512-Frame-Blöcken jeweils
bytegleich auf demselben10-ms-Controlraster. Keine beliebige Controlraten-
Identitätsbehauptung. Hörkopien sind vollständig als65-s-Stereo-PCM16 geprüft.
RAW−30LUFS/−22,7 Dry bzw.−22,6dBFS Room-True-Peak; fester+7dB-Gain ergibt
−23LUFS/−15,6dBFS True Peak. Keine AGC/Kompression. Mono-Energieratio≥0,97469,
DC nahe0; Datei-/Quellen-/Header-/Toolhashes und Rendererberichte in
[HIGHLANDS_GENERATOR_METRICS.json](HIGHLANDS_GENERATOR_METRICS.json).

## Software- und ARM-Nachweise

- Reine12 Keys×2 Collections×32 Seeds: deterministische unveränderte Angebote,
  falsche/duplizierte Acks abgewiesen, Pausengültigkeit und Timing-Deadline0
  am Timerwrap geprüft, Register-/Corevertrag eingehalten.
- 48 vollständige echte Engine-Phrasen über alle Keys/Collections/Stimmungen;
  Attack/Release/Activity/Color-Endpunkte über Keys verteilt, Room/Volume1.
  Dazu4 echte alternative Seed-/Timerwrap-Phrasen, insgesamt52. Jede endet
  natürlich mit acht Starts, maximal drei Quellen, demselben noch gehaltenen
  gemeinsamen Ton, mindestens8-s-Pause nach Retirement, genau einer Reprise.
  Finite PCM, kein Limiter und Samplepeak unter−6dBFS.
- Zehn Cancellation-Fälle vor erstem DSP-Sample: Stop, Autoplay, Presence,
  Mute, Clear, Seed, World, Key, Collection, Tuning. Null tatsächliche Starts
  und null gehörte Zähler; private tiefe Töne über öffentliche APIs abgewiesen.
- Vollständige `test/run_tests.sh`-Suite bestanden:12-Minuten-Product-PCM,
  36-Minuten-Stress, sechs Worldrichtungen, Besitzer/Context/Mute/Presence/
  Scene/Room/Nature, Referenzquellen, Journalmodell und Hotpathchecks.
  Voller Lauf enthält die48-Key-Prüfung; zusätzliche4 Seed-/Wrap-Phrasen
  bestehen im separat wiederholten finalen52-Phrasen-Test. CI nutzt dessen
  aktuelle vollständige Fassung.
- COAST und WOODLAND aus aktueller Engine erneut gerendert: Dry/Room-RAW,
  Hörkopien und Ereignistraces exakt bytegleich zum vorherigen Checkpoint.

ARM-GCC13.2.1/Newlib4.4, H743 Product Release, Journal OFF, Link-/Frame-/DMA-
Audit bestanden. Gleicher Compiler wie PR149:

| Ressource | PR149 | HIGHLANDS | Änderung |
|---|---:|---:|---:|
| Bank1-Flash-Ladebild |195496 B|194316 B|−1180 B|
| RAM_D1 |65760 B|65600 B|−160 B|
| RAM_D2 |60808 B|60808 B|0 B|
| reservierter DTCM-Stack |16384 B|16384 B|0 B|
| größter einzelner Product-Kern-Compilerframe |216 B|192 B|−24 B|

Der kleinere Stand ersetzt den alten Einzelticket-/Alternative-Scorepfad.
Ein interner D2-Raum, ausgerichteter4096-B-D1-DMA-Puffer; keine Archiv- oder
Referenz-DSP-Familien im Link. ELF-/Compiler-/Bank-/Testlognachweis:
[HIGHLANDS_GENERATOR_ARM_METRICS.json](HIGHLANDS_GENERATOR_ARM_METRICS.json).
Compilerframes beweisen keine echte Stackreserve oder DWT-Zeit. Tatsächliche
Deadline-/Stack-/Ausgangs-/Storage-/ECC-/Power-cut-Geräteabnahme bleibt offen.

SCN7 behält Felder/IDs/normierte Parameter. Spec0.6 dokumentiert ausdrücklich
den neuen Generator und private Register; frühere musikalische Folgen bleiben
in ihrem jeweiligen Git-Checkpoint reproduzierbar. CMake-Default bleibt
`reference`, `product` Kandidat; kein Merge/Produktfreeze.

SD16 ist integriert, Hör-/Quellen-/Gerätegate bleibt ungekreuzt. Weiterhin
24 technisch geschlossene/30 offene Gesamtaufgaben. Nächste einzelne Einheit:
SD17, langfristige Entwicklung der integrierten musikalischen Rollenregeln.
