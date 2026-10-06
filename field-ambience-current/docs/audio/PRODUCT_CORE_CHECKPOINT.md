# Sound implementation checkpoint — 2026-10-06

## KERNURTEIL

Der reduzierte Kandidat besitzt jetzt dieselbe echte Quellenzulassung für
Manual und Generate, drei globale Slots, tatsächliche Tonhöhen und ein
gemeinsames Raumgedächtnis. Drei unterschiedliche Abläufe sind an echtes DSP
gebunden. Die grundlegende offene Frage bleibt das Hören: insbesondere
HIGHLANDS hat seinen eigenen ruhigen Tonkörper noch nicht bewiesen.

## FUNDAMENTAL FALSCH

Der Referenzbuild bleibt ein Entwicklungsarchiv mit fünf alten Worlds,
zusätzlichen Klangpfaden und neun Effektmodi. Er ist keine fertige Version
der neuen Produktidee. Ein grüner Build oder niedriger Pegel beweist weder
angenehmen Klang noch Beruhigung.

## NOCH NICHT SELBSTVERSTÄNDLICH

Langfristige Motivwiederkehr ist noch nicht über mehrere Episoden konstruiert.
Quellenpegel und Color/SHAPE/Room-Endwerte sind Kandidaten. Physische Controls,
Display, LED und Speicherinteraktion werden nach dem Soundstand gestaltet.
Unterscheidbare Grammatiken allein rechtfertigen keine schwache dritte Quelle.

## LOCKED

Technisch geprüft im tatsächlichen Kern, nicht perceptual Sound-Lock:
- Drei globale Slots, Pluck zusätzlich höchstens zwei; Releases belegen weiter.
- Originalbesitzer und angewandte Hz bis zum wirklichen Quellenende.
- Kein Pad/Bass/Drone-Bed, keine versteckte zweite Quelle pro Einsatz.
- DSP-Start wird erst nach Audioblock bestätigt; vorheriges Storno erzeugt
  keinen hörbaren Einsatz, MIDI-On oder Score-Commit.
- Gleiche PCM-Trajektorie bei 64 und 512 Frames; ungültige Floats bewahren
  den letzten gültigen Zustand.
- Clear fährt die gesamte Kette in 40 ms auf Null und leert Zustände erst dort.
- Ein gemeinsamer Float-Raum, separate optionale Naturebene; Nature default 0.

PR132 ist gemerged: `aebdc4414bd4f7c09bd39e042641946813977ff3`,
Baum `6711cace481d97f321eade12cbee71178d23b876`.
Geprüfter Head `64d12400da9a787848bc86ef5c1e10425ca0f49b`,
CI 37500531754, alle fünf Jobs bestanden. Zwölf Minuten tatsächliches PCM
(3 Worlds × 2 Seeds × 120 s): keine NaN/Inf, keine Limiter-Eingriffe.
HIGHLANDS: 12/14 gehörte Einsätze, 0/0 abgewiesene Starts statt zuvor 49/26
Selbstkollisionen. 64-vs-512-Frame-Differenz: exakt 0 PCM-Schritte.

H743 in diesem Nachweis ist noch der Referenzbuild:
FLASH 256372 B, DTCM 119440 B, D1 417408 B, D2 258112 B.
Keine Aussage über CPU/Stack oder Ausgangs-/Klangabnahme.

## REMOVE / MERGE / REDESIGN

PR133 baut `FAM_SOUND_PROFILE=product` ausdrücklich separat;
`reference` bleibt Default bis Quellen-/Geräteabnahme. Der Kandidat kompiliert
keine Archiv-Synths, Pad/Bass/Drone, Body, zweite Hallengine oder alte FX.
Der ARM-Linkprüfer kontrolliert tatsächliche Kompilierung, Symbole,
Room in internem D2 und DMA-Buffer in internem D1.

SCN7 migriert genaue SCN5/SCN6-Layouts. Alps→HIGHLANDS,
Open Sea/Fjords→COAST, Moss Fields/Desert→WOODLAND.
Das sind dokumentierte Ersatzzuordnungen, keine Rekonstruktion alter Sounds.
Zurückgesetzte native/Layer-/FX-Werte, Persistenz-CRC, fehlgeschlagener Save,
stille Initialisierung und Volume-Erhalt erhalten reale Integrationstests.

## BESTE VERSION

Worldwahl verändert Tonkörper und musikalische Zeit; gemeinsame Key-, Room-,
Nature-, Activity- und Color-Werte bleiben stehen. Manuell gehaltene Töne
bleiben bei World-/Keywechsel tatsächlich unverändert und werden beim
Loslassen an ihrer ursprünglichen Quelle beendet. Neue Töne respektieren
diese Geschichte. COAST übergibt Zusammenhang, WOODLAND antwortet,
HIGHLANDS lässt nach Fragmenten Raum oder entfällt nach Hörprüfung.

Noch abzuarbeiten: Quellen-/Register-/Artikulations-Hörvergleich, volle
Raum-/Naturprüfung, feste Kalibrierung, langfristige Motiverinnerung,
sechs gerichtete Übergänge, Parameter-Risikokombinationen, lange Recovery-
Läufe, exakte Software-Spezifikation. SD48–SD50 benötigen ein echtes Gerät.

## TEST AM GERÄT

DWT-Peak <0,60, null Deadline-Misses und gemessene Stackreserve; tatsächliche
DAC-/Amp-/Ausgangslasten, DC/Noise/Pops und ruhige längere Hörsitzungen.
Worlds ohne Labels unterscheiden, Mono-Körper und Parametergrenzen beurteilen.
Keine Heilwirkung oder allgemeine Verträglichkeit aus Codeprüfungen ableiten.
