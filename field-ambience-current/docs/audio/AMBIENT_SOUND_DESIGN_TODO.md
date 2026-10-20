# AMBIENT — verbindliche Sounddesign-To-do

## KERNURTEIL

AMBIENT besitzt jetzt einen tatsächlich reduzierten Produktkandidaten: drei
verschiedene Grammatiken, gemeinsame Quellen-/Besitz-/Harmonieprüfung,
echte Hz-/Besitzer-/Tailhistorie und einen gemeinsamen Raum. Die größte offene
Schwäche ist die ungehörte Familienauswahl: insbesondere HIGHLANDS, Bowed-Grain
und optionale Natur sind noch kein belegter ruhiger Produktklang.

Stand **2026-10-20**. Diese Liste umfasst alle SD00–SD53. Aktueller Vertrag:
[PRODUCT_SOUND_SPEC.md](PRODUCT_SOUND_SPEC.md), konsolidierter
[SD51-Softwarestand](SD51_SOFTWARE_CANDIDATE.md) und
[SD52-UI-Anbindung](SD52_UI_IDLE_CHECKPOINT.md).
`FAM_SOUND_PROFILE=product` bleibt Kandidat, `reference` ausdrücklicher Default.
Ältere Render-/Next-/Locked-Einträge sind keine Produktfreigabe.

## Offener Arbeitsstand2026-10-20

54 Hauptpunkte:25 erledigt,29 offen. SD52 ist für die bestehende H743-Bedienung
softwareseitig abgeschlossen. SD51 ist als reproduzierbarer Softwarekandidat
konsolidiert; sein finaler Sound-Freeze bleibt bei den Hör-/Geräteentscheidungen.
Es gibt keinen weiteren unabhängig vom Gerät belegten DSP-/UI-Softwarebefund.
Das bedeutet ausdrücklich nicht, dass die ganze Produktsoftware freigegeben ist:
Product-Live-Save bleibt synchron. SD41-Aktivierung benötigt zuerst die reale
ECC-/Power-cut-Abnahme; SD47 hängt an dieser Aktivierung und SD48-Messung.

| Einteilung | Offen | Punkte |
|---|---:|---|
|Softwareabschluss mit Geräteabhängigkeit|2|SD41 (Journalaktivierung nach Siliziumtest), SD47 (SD41 + IRQ/Stack)|
|Prüfungen am echten Gerät|2|SD48, SD49|
|Hör-/Klangentscheidungen und finaler Freeze|24|SD02, SD03, SD04, SD05, SD06, SD11, SD12, SD14, SD15, SD16, SD17, SD19, SD28, SD29, SD30, SD31, SD33, SD34, SD35, SD37, SD44, SD45, SD50, SD51|
|Abschließender Gesamtcheck|1|SD53|

SD11/12/31/33/34/35/37/45 sind softwareseitig geprüft. SD51 konsolidiert
Sound0.10/SCN7/Defaults,265 gehashte Firmwareinputs und die verbliebenen Gates.
Kein Hör-/Gerätehaken aus Messwerten oder aus „weiter“.

## Softwarefortschritt2026-10-20 — SD51 konsolidiert, SD52 angebunden

900000ms Human-idle → Display/LEDs weich dunkel, Musik/DMA/Zeit/Besitzer laufen.
Erste Geste samt Release und30-ms-Queue-/Debounce-Guard weckt ohne Aktion;
weitere Bedienung nutzt dieselben Audiooperationen. Gehaltene Tasten verhindern
idle.21 reale Wakewege in allen drei Worlds,64/512-Caps und Mainclockwrap:
126 vollständige12-s-PCMgegenproben pro Modus bytegleich,66679200 Stereo-Frames.
Native und ASan/UBSan geprüft. Kein neuer Klangpfad, keine Parameteränderung,
keine WOODLAND-Verkürzung. Product-/Reference-ARM-Builds und aktueller Hostrunner
prüfen den neuen Stand; konkrete Ergebnisse im SD52-Checkpoint/PR.

Vier bestehende UI/CMake/LED-Inputs geändert,259 übrige bytegleich PR179;
zwei neue UI-Dateien. SD41-Bench existiert bereits; nächste Grenze ist dessen
reale Siliziumabnahme, nicht ein weiteres gleichartiges Hostpaket. Kein Gerät
verbunden/geflasht. Keine Produktfreigabe, kein automatischer Merge/Retarget.

## Einzelner Fortschritt 2026-10-19 — SD45 durchgehende Recovery geprüft

Die noch fehlende gemeinsame Sequenz ist ergänzt: sechs World-/Scenezyklen,
native Savefehler/Retry, CRC-Fallback/Recovery und Start/Stop/Mute in einer
ununterbrochenen800-s-Engine-Sitzung, danach lange manuelle WOODLAND über
Worldwahl bis natürlichem Ende. Zwei Seeds×64/512-Obergrenzen, je normale/
wrappende Mainclock:141120000 ganze Stereo-Frames und komplette Hooks pro
Prüfmodus bytegleich. Native und ASan/UBSan gleich;16 Captures/213min20s echtes
Audio insgesamt,91 bestätigte Starts an91 frischen Reservierungen je Capture.
Keine NaN/Limiter/Stuck, geplante stille Endphasen exakt Null, physisch≤3/Pluck≤2.
Alle263 Firmwareinputs unverändert, eigener aktueller SD45-CI-Job hinzugefügt.

Kein Flash-/ECC-/Power-cut- oder Hardwarezeitpass; Product-Save synchron,
Journal OFF. Bekannte Partitions-/Quiet-Grenze bleibt, keine ganze64/512-
Kreuzgleichheit behauptet. Vorhandene lange Phrasen-/Stress-/Recoverybelege
zugeordnet, musikalische Langzeitform weiter hören. SD45 bleibt ungecheckt
und zählt zur Hörgruppe.24/54 geschlossen,30 offen; technische Blöcke7→6
(4 Software/Abschluss+2 Gerät), Hörgruppe22→23.
[SD45_LONG_RECOVERY_CHECKPOINT.md](SD45_LONG_RECOVERY_CHECKPOINT.md).
Weitergeführt durch den SD51-/SD52-Softwareabschluss2026-10-20 oben.

## Einzelner Fortschritt 2026-10-19 — SD33 technische Parameterbereiche geprüft

Activity/Color/Room sowie getrennte Attack-/Releasebereiche vollständig mit
aktuellen Settern, Kennlinien und vorhandenen Ensemble-/Kombinationsbelegen
zugeordnet. COAST-Activity und lange WOODLAND-Color sind bereits kalibriert;
kein weiterer belegter Softwarebefund. Zehn von zwölf Release-Checkpoint-
Corequellen und alle sieben öffentlichen Reglerfunktionen aktuell bytegleich.
Spätere Engine-/Roomkorrekturen und historische PCM-Belege ausdrücklich getrennt;
keine alte Aufnahme als frisch gerendert bezeichnet.263 Firmwareinputs exaktPR177,
Runtime identisch zum vollständigen grünen PR176-Host/H743-Stand (27/27 CI).
[SD33_CONTROL_CONSOLIDATION_CHECKPOINT.md](SD33_CONTROL_CONSOLIDATION_CHECKPOINT.md).

SD33 zählt jetzt zur Hörgruppe und bleibt ungecheckt. Musikalischer Nutzen,
gehörte Endwerte und physische Zuordnung bleiben offen; keine Klangänderung.
24/54 geschlossen,30 offen; technische Blöcke8→7 (5 Software/Prüfung+2 Gerät),
Hörgruppe21→22. SD41/SD47 warten auf ihre Geräteabhängigkeiten; nächste
unabhängige technische Einheit ist SD45.

## Einzelner Fortschritt 2026-10-18 — SD31 technische Geräuschwege geprüft

Vollständiges Product-Inventar mit Eintritt, Besitzer, Rolle und Idle-/Endverhalten:
alte Brown-Rumble-/Breath-/Warm-/Air-/Fjord-/Haze-/Desert-/Crackle-/Hiss-/Sea-Hum-
Wege fehlen im tatsächlichen ARM-Link. Aktuelles PR176-Product-Artefakt heruntergeladen,
API-Head/Digest/ZIP-CRC und73 Firmware-LOAD-Objekte sowie belegte Funktionen geprüft.
Alle263 bestehenden Firmwareinputs bytegleich; aktueller vollständiger Hostrunner
und H743-CI grün, vorhandene Nature-Handoffhashes stimmen. Keine neue DSP-Änderung
oder erneuter Langzeitrender für dieses Inventar.

Übrig nur deklarierte Quellenartikulation und optionale Nature, default0.
Kein weiterer Softwarebefund; Ort, Grain/Anblasen, Bandresonanz und Maskierung
bleiben gehörseitig offen. Die lange WOODLAND-Saite bleibt unverändert.
[SD31_NOISE_PATHS_CHECKPOINT.md](SD31_NOISE_PATHS_CHECKPOINT.md).
SD31 zählt jetzt zur Hörgruppe, bleibt ungecheckt.24/54 geschlossen,30 offen;
technische Blöcke9→8 (6 Software/Prüfung+2 Gerät), Hörgruppe20→21.
Nächste unabhängige technische Einheit: SD33.

## Einzelner Fortschritt 2026-10-18 — SD12 technische Tailhistorie abgeschlossen

24 reale Hold-/Release-/Roomfälle mit Equal→Just-Sperre, vollständiger
Rejektionskontrolle und Audioframe-/Epoch-Wrap:45962784 Stereo-Frames je
Prüfmodus bytegleich verglichen. Dazu6 echte warme Common-tone- und6 Cancel-
Fälle sowie10 klar getrennte synthetische Deadline-/Kapazitäts-/Refreshchecks.
Nativ und ASan/UBSan grün,263 Firmwareinputs bytegleich PR175.

Die Freigabegrenze unterscheidet sich bei64/512-Obergrenzen höchstens einen
10-ms-Main-Tick; keine samplegenaue Quiet-Frist oder ganze64/512-PCMgleichheit
für diesen neuen Grenzstart behaupten. Die lange WOODLAND-Saite bleibt erhalten.
[SD12_TAIL_HISTORY_CHECKPOINT.md](SD12_TAIL_HISTORY_CHECKPOINT.md).
SD12 zählt jetzt zur Hörgruppe;7,2s/250ms/1e-5 bleiben gehörseitig zu kalibrieren.
24/54 geschlossen,30 offen; technische Blöcke10→9 (7 Software/Prüfung+2 Gerät).
PR175 ist abschließend26/26 grün, sein echter CI-Proof geprüft. Nächste
unabhängige technische Einheit: SD31.

## Einzelner Fortschritt 2026-10-18 — SD11 technische Harmonik abgeschlossen

Die noch fehlende systematische öffentliche Zulassung ist geprüft:40032
Einzelton-/Paarfälle über3 Worlds×12 Keys×2 Collections×Equal/Just×beide
öffentlichen APIs. Unabhängiger Frequenzbezug und deklarierte Intervallgrenzen,
echte Quellen/DSP-Acks;3936 abgewiesene Paare erhalten8060928 ganze Stereo-
Frames exakt gegenüber dem unberührten Kontrollton. Register-/3-Cent-Grenzen
und fehlender privater Bypass bestehen nativ und unter ASan/UBSan.

Bestehende All-key-Generatoren und aktuelle Teilton-/Quellenbelege sind
zugeordnet;263 Firmwareinputs bleiben bytegleich PR174. Kein neuer belegter
DSPfehler. [SD11_PUBLIC_HARMONY_CHECKPOINT.md](SD11_PUBLIC_HARMONY_CHECKPOINT.md).
SD11 zählt jetzt zur Hörgruppe; Teilton-/Registerwahl bleibt ungecheckt.
24/54 geschlossen,30 offen; technische Blöcke11→10 (8 Software/Prüfung+2 Gerät).
Nächste unabhängige technische Einheit: SD12.

## Einzelner Fortschritt 2026-10-18 — SD47 Abschlussinventar und Nature

Das verbliebene SD47-DSP-/Übergabeinventar ist jetzt konkret abgeschlossen:
Callsites, Besitzer, feste Schleifengrenzen, Publikation und Spitzen-/Stack-
Kandidaten stehen in [SD47_HOTPATH_INVENTORY.md](SD47_HOTPATH_INVENTORY.md).
Kein neuer belegter DSPfehler; alle263 bestehenden Firmwareinputs bytegleich
PR173.120 echte Nature-Publikationsunterbrechungen und10580880 vollständig
verglichene Stereo-Floatframes bestehen einschließlich schneller World-/Seed-
Targets, Amount0/Stille, Clear und Neustart;64/512 bytegleich, Sanitizer grün.

SD47 bleibt nur für zwei ausdrücklich benannte Abhängigkeiten offen:
SD41 muss den weiterhin synchronen Live-Save ablösen, SD48 muss die ganze
DMA-IRQ-Spitze und reale Stackreserve messen. Keine weiteren bereits grünen
Room-/Quellen-Prep-/Stoppprüfungen als neue Arbeit beginnen. PR173 ist jetzt
vollständig24/24 grün; sein vorheriger18/24-Zwischenstand ist überholt.
24/54 geschlossen,30 offen,11 technische Hauptblöcke unverändert.
Nächster unabhängiger Prüfblock in der Reihenfolge: SD11.

## Einzelner Fortschritt 2026-10-18 — SD47 Room-Clear-Arbeit

Ein tatsächlicher Spitzenlastfall ist reduziert: Room-null/Dry und Clear/Mute
konnten dieselbe bereits leere Hallarena im Audioblock nochmals löschen.
Der vorhandene audioeigene Cold-Zustand vermeidet die zweite Löschung.
Beobachtete Clear-Zielbytes pro betroffenem Block121440→60720, kalter
Wiederholungs-Clear60720→0. Ein warmer Clear leert weiter vollständig.
Das sind C-Zielgrößen, kein gemessener Busverkehr oder halbierte CPUzeit.

36 echte Clear-/Mute-Fälle und24 ganze normale12-s-Product-Paare: PCM und
Hooks vorher/nachher bytegleich, Sanitizer grün. Gepaarter H743-Link:
Flash+8B, kein zusätzliches RAM, Room-Process-Instruktionsbytes gleich;
eigener Clear-Compilerframe8→16B, keine Gesamtstack-/WCET-Behauptung.
[SD47_ROOM_CLEAR_WORK_CHECKPOINT.md](SD47_ROOM_CLEAR_WORK_CHECKPOINT.md).
SD47 bleibt für das restliche Abschlussinventar offen;24/54 geschlossen,
30 offen,11 technische Hauptblöcke. PR172 ist jetzt vollständig23/23 grün,
sein2004-B-CI-Proof gegen committed/lokal geprüft; alter Queued-Stand überholt.

## Einzelner Fortschritt 2026-10-17 — SD47 Quellenvorbereitung

Die echte Vorbereitung aller drei Quellen ist jetzt innerhalb der Arbeit
unterbrochen und gegen einen vollständigen Float-Audioverlauf verglichen:
4605 Fälle,5082 Audiohälften, C11/O2/Werror und ASan/UBSan grün. Bei Pluck
jede Iteration der Füll-/Zentrier-/Normierpässe bei60 und146,83238Hz; vor
Publikation hört Audio ausschließlich die alte Quelle. Uninstrumentierte
Normalspuren bleiben exakt gleich. Keine Firmwareänderung, alle263 Inputs
bytegleich PR171. Die bestehende lange WOODLAND-Artikulation bleibt erhalten.

Eine falsche Ressourcenangabe ist korrigiert: N≤315 gilt nur manuell ab140Hz;
Generate ab105Hz braucht die konservative Grenze N≤420/n≤421. Höchstens1866
Pluck-Bufferiterationen pro Product-Start, keine neue Arbeit oder neue Töne.
[SD47_SOURCE_PREPARATION_CHECKPOINT.md](SD47_SOURCE_PREPARATION_CHECKPOINT.md).
SD47 wartet noch auf das übrige abschließende Spitzenlastinventar.24/54
Hauptpunkte geschlossen,30 offen und11 technische Hauptblöcke unverändert;
kein Hör-/Gerätehaken und keine reale WCET aus Hosttests.

## Einzelner Fortschritt 2026-10-17 — SD47 Quellenbesitzer

Ein tatsächlicher Übergabefehler ist behoben: Audio konnte die alte Quelle
beenden, bevor Main deren alten Stopp veröffentlichte. Bei erneuter Belegung
löschte dieser Befehl den neuen Ton. Gegenprobe am unveränderten PR170 scheitert
in allen drei Worlds. Main entfernt jetzt die beiden alten Besitzerbits vor
der neuen Vorbereitung; andere und aktuelle Stopps bleiben erhalten.

645 gezielte Fälle/48809 Assertions,1395 Audiohalb-Unterbrechungen an zehn
Main-Grenzen, alle neun Worldpaare und sieben manuelle/Generate-Besitzer;
C11/O2/Werror und ASan/UBSan grün.24 ganze12-s-Paare liefern identische normale
PCM und echte Hooks, auch64/512. H743-Gegenlink: Flash+136B, kein neues RAM,
Audio-Renderabschnitt und dessen88-B-Compilerframe unverändert. Zwei feste
Main-Wort-RMWs, mögliche Exclusive-Retries; keine neue Audioarbeit oder echte
Geräte-WCET behauptet. Quellen/Grammatiken und lange WOODLAND-Holds erhalten.
[SD47_SOURCE_HANDOFF_CHECKPOINT.md](SD47_SOURCE_HANDOFF_CHECKPOINT.md).

SD47 bleibt für die übrigen Hotpath-/Spitzenlastfälle offen.24/54 geschlossen,
30 offen und11 technische Hauptblöcke bleiben bestehen. PR170 ist abschließend
mit21/21 CI-Jobs grün; dessen frühere13/21-Meldung ist überholt. Kein neuer
Hörhaken und keine Gerätefreigabe aus „weiter“.

## Hörfeedback 2026-10-08 — musikalische Priorität

Der Nutzer bewertet die bisherigen trockenen Einzelquellenbeispiele als RAW,
langweilig, zu ähnlich und ohne musikalische Besonderheiten. Gewünscht ist ein
Ensemble aus gehaltenen Akkordtönen mit gemeinsamen Tönen, Oktavwechseln,
verschiedenen Dauern sowie hörbarer Spannung und Auflösung.
**Zuerst SD11/SD14: Akkordbogen und Stimmführung.** Ein komponierter 27-s-
Entwurf mit unseren echten C-Quellen ist vorbereitet; er ist noch kein autonomer
Generator und bestätigt keine isolierte Familienwahl aus SD02–SD04. Siehe
[MUSICAL_ENSEMBLE_DIRECTION.md](MUSICAL_ENSEMBLE_DIRECTION.md).
Weitere reine Quellen-/SHAPE-Pakete folgen nach diesem musikalischen Fundament.
**Nutzerurteil:** Die Ensemble-Probe wurde positiv gehört („ja besser!!!!“).
Gewünscht sind jetzt verschiedene musikalische Algorithmen pro World. COAST
hat einen seed-gesteuerten Gegenbewegungs-Entwurf: D3/D5 treffen sich in D4,
verbunden durch einen gehaltenen Akkordton und erneut positiv gehört.
HIGHLANDS-Akkord/Pause/Rückkehr (44 s) ist ebenfalls als Host-Algorithmus
vorbereitet. Die kurze WOODLAND-Ruf/Antwort-Probe wurde als zu gezupft verworfen.
Ein neuer langer Saiten-Kandidat (45 s, weicher Einsatz, lange Überlagerung)
ist gerendert und zeitlich/technisch geprüft; seine Hörwahl bleibt offen. Siehe [WORLD_ENSEMBLE_ALGORITHMS.md](WORLD_ENSEMBLE_ALGORITHMS.md).
COAST ist jetzt im echten Product-Generator integriert: zwei gegenläufige
Außenstimmen, gehaltener gemeinsamer Akkordton und ein einzelner Zielton.
48 vollständige Engine-Phrasen über alle Keys, Collections und Stimmungen,
Host-Suite und H743-Build bestehen. Die70-s-Firmwareprobe erreicht mit den
normalen Hüllkurven die Mitte bei43,8 s; sie braucht noch ein eigenes Hörurteil.
Siehe [COAST_GENERATOR_CHECKPOINT.md](COAST_GENERATOR_CHECKPOINT.md).
Auch WOODLAND ist jetzt als lange Saite mit eigener Akkordrollen-Regel im
Product-Generator integriert. Zwei vorhandene Saiten, sechs weiche Einsätze,
ungleiche Halteabsichten und echte gemeinsame Akkordtöne. Die50-s-Probe
verwendet die normale Engine;48 vollständige Phrasen und H743 sind geprüft.
Neue Quellen-/Firmware-Hörwahl bleibt offen. Siehe
[WOODLAND_GENERATOR_CHECKPOINT.md](WOODLAND_GENERATOR_CHECKPOINT.md).
HIGHLANDS ist jetzt ebenfalls im echten Generator integriert: weiter Akkord,
gehaltener gemeinsamer Ton, verwandte Lage, tatsächliches Quellenende und
offene Pause vor der Rückkehr.48 vollständige Keys/Collections/Stimmungen
plus4 echte Seed-/Timerwrap-Phrasen, Host-Suite und H743 bestehen. Die65-s-
Engineprobe zeigt die Rückkehr bei43,24 s; neue Quellen-/World-Hörabnahme
bleibt offen. Siehe [HIGHLANDS_GENERATOR_CHECKPOINT.md](HIGHLANDS_GENERATOR_CHECKPOINT.md).
SD17 ist jetzt auf diese tatsächlichen Generatoren übertragen: vollständige
Ausgangsphrase, begrenzte Variation, relative Major-/Minor-Akkordlage und
erinnerte Rückkehr mit seedabhängig2–4 Zwischenphrasen.2304 Pure-Cases über
je16 Phrasen und90 Minuten echte Engine-Läufe bestehen; drei29,5-s-Vergleiche
zeigen gehörte spätere Abschnitte aus Minute2–4. Gemeinsame Besitzer/Tails,
normale Quellen-/Makrogrenzen und eigener World-Verlauf bleiben wirksam.
Software-/H743-Nachweis: [PHRASE_DEVELOPMENT_CHECKPOINT.md](PHRASE_DEVELOPMENT_CHECKPOINT.md).
Wiedererkennbarkeit und Langzeitwirkung sind noch nicht vom Nutzer abgenommen.
SD19 hat jetzt drei27,5-s-Vergleiche derselben tatsächlichen Ensemble-Passage
mit Dry/Room0,24/0,50/1,00. Alle echten Note-on/off-Verläufe und64/512-PCM
stimmen überein; Float-Impuls/Decay/Mono und bestehende Room/Nature-Regression
bestehen. Der aktuelle Raum ist messbar sehr zurückhaltend: bei0,24/0,50
etwa0,2–1,0% RMS-Differenz zum trockenen Ausschnitt, selbst Maximum2,8–4,8%.
Dies ist ein Kalibrierbefund, keine positive Hörabnahme. Quelle, Default,
Send und Raum-DSP sind unverändert. Siehe [SD19_ROOM_CHECKPOINT.md](SD19_ROOM_CHECKPOINT.md).
SD19 ist jetzt als begrenzter Kandidat kalibriert: Rücklauf5*r statt der
bisherigen leisen Kurve; gleicher FDN, gleiche Quellen/Delays/Dämpfung/Feedback.
Bei Room0,5 steigt die tatsächliche RMS-Differenz zum trockenen Ensemble von
0,66–1,04% auf8,86–13,99%. Drei27,5-s-Vorher/Nachher-Dateien enthalten dieselbe
gehaltene Passage und denselben echten Stop-Ausklang. Noten-/Besitzertraces
identisch, normierter Float-Decay/Mono unverändert, Dry exakt. Volle Host-Suite
und H743-Baseline/Kandidat mit identischer verifizierter Toolchain bestehen;
Flash−8B, RAM unverändert. Spec0.8; noch keine positive Hörwahl oder SD19-Haken.
Siehe [SD19_ROOM_CALIBRATION_CHECKPOINT.md](SD19_ROOM_CALIBRATION_CHECKPOINT.md).
SD28 ist jetzt als Hörvergleich vorbereitet: drei 27,5-s-Dateien mit/ohne Nature
0,35 bei identischem tatsächlichem Ensemble und späterem leisen Verlauf. Zwei
Seeds, Nature 0/0,35/1 und beide Blockgrößen ergeben 18 vollständige 112-s-Fälle
mit gleichen Noten-/Besitzer-/Phrasenverläufen; echtes Stillwerden nach Stop,
Pegel/Mono und Unabhängigkeit vom Room bestehen. COAST hat keinen langen
Quellenstopp, WOODLANDs Vergleich enthält 4,49 s Quellenpause, HIGHLANDS ist im
späten Ausschnitt vollständig quellenfrei. Keine künstliche Pause, kein AGC.
Firmware und Spec0.8-Klang bleiben exakt PR153; Default Nature bleibt0, SD28 offen.
Siehe [SD28_NATURE_CHECKPOINT.md](SD28_NATURE_CHECKPOINT.md).
SD33 Activity ist jetzt mit den echten musikalischen Worlds als einzelne
Vorbereitung geprüft:21 vollständige6-min-Fälle einschließlich Live-Wechseln,
jeweils64/512 bytegleiche PCM/Noten-/Statusdaten. Min/default/max behalten die
gehörte Ton-/Rollenfolge und lange Haltezeiten; vier Entwicklungsstufen und
vollständig erinnerte Rückkehr bleiben erreichbar. Drei38,5-s-Dateien vergleichen
0/0,5/1 mit festem Gain. HIGHLANDS zeigt bewusst die echte Pause und Rückkehr.
WOODLAND33/39/41 beziehungsweise32/39/41 Einsätze; HIGHLANDS32/37/43 bei beiden
Seeds. COAST47/47/47 und fast gleiche Rückkehrzeiten: die nominelle Activity-
Kennlinie wirkt dort wegen realer Quellen-/Tailgrenzen noch zu schwach. Das ist
ein offener Softwarebefund, keine geeignete Reglerkennlinie oder Hörabnahme.
Firmware und Spec0.8 bleiben unverändert. Siehe [SD33_ACTIVITY_CHECKPOINT.md](SD33_ACTIVITY_CHECKPOINT.md).
SD33 COAST ist jetzt als einzelne Spec0.9-Kalibrierung umgesetzt: Activity0/0,5/1
wartet nach tatsächlichem Außenquellenende6/3/0s, nach Ende aller Quellen10/5,5/1s,
jeweils mit Entwicklungsfaktor. Gemeinsamer Owner15 bleibt stehen; bestätigte
Holds, gehörte Ton-/Rollenfolge und Memory bleiben erhalten. Beide Seeds zeigen
56/64/77 statt79/79/79 Einsätze bis568s.44 volle Renderläufe/309,87min, identische
64/512-PCM-/Score-/Statusdaten, Livewechsel, echte Waits/Stop und bytegleiche
WOODLAND/HIGHLANDS bestehen. Eine neue38,5-s-COAST-WAV zeigt0/0,5/1 bei gleichem
Gain. Volle Host-Suite und exakt reproduzierter Baseline-/neuer H743-Build:
Flash+456B, D1/D2 RAM unverändert. SCN7/Default0,5 bleiben, COAST-Zeitsemantik
ist bewusst geändert. Noch kein Hörurteil oder SD33-Haken. Siehe
[SD33_COAST_ACTIVITY_CHECKPOINT.md](SD33_COAST_ACTIVITY_CHECKPOINT.md).
SD33 Color ist jetzt im tatsächlich gehaltenen Ensemble vorbereitet: drei
38,5-s-WAVs vergleichen0/0,5/1 mit gleichen musikalischen Abläufen und einem
festen Gain pro World.18 feste112-s-Fälle plus drei Livefälle sind jeweils
64/512 bytegleich in vollständigem PCM/Noten/Status; zwischen Colorwerten und
bei Liveänderungen bleiben sämtliche Noten, Haltezeiten und Übergaben erhalten.
Sechs normale Defaultkontrollen sind exakt;48 volle Render/89,6min plus echter
Product-Vertragscheck bestehen.251 Firmware-Kerninputs bytegleich gegen grünen
PR156; Spec0.9/Core unverändert, keine neue ARM-/vollständige Hostbehauptung.
**Konkreter Befund:** WOODLANDs bestehendes Color verändert den echten gehaltenen
Klang nur um0,203/0,204% RMS zum Default; FFT-Leistungszentrum181,08→181,46Hz.
Die Kennlinie ist zu schwach für einen belegten musikalischen Endwertnutzen.
COAST/HIGHLANDS zeigen größere messbare Farbänderungen, Hörnutzen bleibt offen.
Siehe [SD33_COLOR_CHECKPOINT.md](SD33_COLOR_CHECKPOINT.md).
SD33 WOODLAND-Color ist jetzt als einzelne Spec0.10-Kalibrierung umgesetzt:
Ausgangsfarbe statt veränderter Loop-Dämpfung; Neutral0,5 exakt bisheriger Klang,
lange Schwingung, Tonhöhe und Halte-/Besitzerverlauf erhalten. Beide Seeds zeigen
jetzt etwa7,27% FFT-Endwertspanne statt0,21%.63 volle Render/117,6min PCM,
24 tatsächliche Float-Solo-Pitch-/Decayfälle und derselbe natürliche Retirement-
Block bestehen. COAST/HIGHLANDS inklusive Livewechsel vollständig bytegleich.
Zwei kurze WAVs vergleichen0/0,5/1 undalt/neu; ein Gain je Datei, reale Farb-/
Pegelunterschiede bleiben. Voller Hostrunner und exakt reproduzierter Baseline-
sowie neuer H743-Build bestehen: Flash+236B,D1+32B,
D2+0B,DTCM+0B. Musikalischer Hörnutzen bleibt offen.
Siehe [SD33_WOODLAND_COLOR_CHECKPOINT.md](SD33_WOODLAND_COLOR_CHECKPOINT.md).
SD33 Attack ist jetzt als einzelner Ensemblevergleich vorbereitet:0/0,5/1
vergleichen tatsächliche Tonanfänge, Release bleibt0,5.48 volle Render/89,6min,
zwei Seeds,64/512 identisch und vollständige identische musikalische Abläufe.
Bereits vorbereitete/held Töne bleiben beim Drehen im echten PCM unverändert;
erst zukünftige Einsätze übernehmen Attack. Drei38,5-s-WAVs haben keinen
Anfangsfade und denselben festen Gain pro World. Nominale Einsätze COAST
0,161/0,300/0,560s, WOODLAND0,429/0,800/1,493s, HIGHLANDS0,241/0,450/0,840s.
251 Coreinputs exakt zum inzwischen vollständig grünen PR158, Spec0.10 gleich.
Details: [SD33_ATTACK_CHECKPOINT.md](SD33_ATTACK_CHECKPOINT.md).
SD33 Release ist jetzt separat im Ensemble geprüft: 66 ganze Render/99,2 min,
zwei Seeds, 64/512 exakt, neun Stop-Gegenproben mit identischen Anfangstönen
und Holds. Größere Werte verlängern tatsächliche Belegung und Ausklangenergie.
Alle 250 Starts sind mit frischen physischen Quellenreservierungen verknüpft;
Übergaben warten bis nach dem echten Ende der vorherigen Quelle. Bereits
vorbereitete/held/releasing Töne bleiben bei 18 Liveänderungen sechs Sekunden
PCM exakt; neue Töne übernehmen Release. Drei 38,5-s-WAVs vergleichen 0/0,5/1
mit einem festen Gain je World. 251 Firmwareinputs exakt zum grünen PR159.
Keine Klang-/Geräte-/SD33-Gesamtabnahme. Details:
[SD33_RELEASE_CHECKPOINT.md](SD33_RELEASE_CHECKPOINT.md).
SD34 ist nun softwareseitig geprüft:105 Solo-,162 Maximalsummen- und24
Generatefälle bei64/512 plus volle Default-/Referenzkontrollen:596 WAV-Render,
209,2min. Gleiche Tonhöhe maximal2,329dB Körperunterschied; kleiner Velocityschritt
0,50→0,55 ergibt0,828–0,831dB. Feste Gains0,50/0,22/0,50 und lange WOODLAND-
Schwingung bleiben, alle251 Coreinputs exakt grüner PR160. Stärkste Summe9,5dB
Reserve bei Referenzmaster0,6,5,1dB bei Master1; kein Limiter/NaN. Ein40-s-WAV
erhält echte Worldunterschiede mit einem Gain. Finale Lautheitsbalance/SD34
bleibt offen. Details:[SD34_GAIN_CHECKPOINT.md](SD34_GAIN_CHECKPOINT.md).
SD35 ist jetzt softwareseitig geprüft:48 feste und drei Live-Generatefälle,
20 manuelle volle Belegungen und sechs manuelle Livefälle bei64/512 plus
drei Defaultgegenproben:157 ganze Render/429,067min PCM. Acht gezielte Profile,
zwei musikalische Kontexte; sämtliche PCM/Events/Status/Controls blockgenau.
1049 echte Starts mit frischen physischen Reservierungen; Quellen3/2, natürliche
Enden ohne Clear, manuelle WOODLAND-Fahnen35,68–45,04s nach Key-up erhalten.
18 kombinierte Activity/Attack/Release-Änderungen erhalten6s PCM; zwölf gepaarte
volle Akkordwechsel maximal3 PCM-Stufen zusätzlich im ersten Sample.
Pegel/Mono/DC/NaN/Limiter bestehen. Alle251 Coreinputs exakt grüner PR161.
Ein40-s-WAV zeigt reale Referenz-/Extremzustände mit einem Gain; musikalische
Helligkeit/Buzz/Pumpen und Endwertnutzen bleiben offen. SD35 zählt nun zur
Hörgruppe;beim damaligen Checkpoint12 technische Hauptblöcke,24 erledigt/30 offen.
Details:[SD35_PARAMETER_CHECKPOINT.md](SD35_PARAMETER_CHECKPOINT.md).
SD37 ist jetzt softwareseitig geprüft:54 aktuelle Fälle in allen sechs
Richtungen bei64/512,108 ganze Render/220,8min PCM. Volle erzeugte/manuelle
Pools,Pending,Release,Mehrfachziele mit/ohne Audio,Nature und Timerwrap.
670 DSP-Starts an678 frischen physischen Reservierungen; acht unbegonnene
alte Starts bleiben ohne PCM/Hook/History. Erzeugte Übergaben100ms,
nur jüngste Ziel-Familie startet; ganze Audio-/Event-/Besitzverläufe exakt.
27 gepaarte4-s-Fälle erhalten ganze manuelle Held/Release-,Source-free-Room-,
Latest-Nature- und idempotente Generate-PCMbytes. Kein Clear/Mute.
Leere Quiet-History-Diagnose bei78 Beobachtungen um einen10-ms-Tick verschieden;
keine Audio-/Ereignis-/Besitzabweichung. SD12-Hörkalibrierung bleibt offen.
Ein50,5-s-WAV zeigt alle sechs tatsächlichen Worldaufträge ohne zusätzliche
Überblendung. Die alte Pluck20-ms-Specstelle ist auf vorhandene100ms berichtigt;
alle251 Firmwareinputs exakt grüner PR162. Musikalische Worldübergänge noch hören.
SD37 zählt nun zur Hörgruppe;11 technische Hauptblöcke,24 erledigt/30 offen.
Details:[SD37_WORLD_TRANSITION_CHECKPOINT.md](SD37_WORLD_TRANSITION_CHECKPOINT.md).
SD41 besitzt jetzt den aktuellen echten Boot-/Recall-/Seednachweis: sechs
Boots aus vorher lauten Manual-/Generatezuständen,380 Native-Store-Fallbacks,
zehn Legacy- und20 Off-Recalls. Alle fünf nativen Slotindizes, Volume0/30/100
und Mute; kein restaurierter Ton-/Generate-/HOLD-Besitzer. Positives Nature
ist beim ausdrücklichen Recall weiterhin optional hörbar, Boot bleibt still.
Zwölf manuelle und drei fehlgeschlagene-Save-Triplets erhalten ganze8s PCM
und Hooks bei512/64; lange Saite und Original-Key-up bleiben erhalten.
Neun frische Seed-Triplets und neun Timerwrap-Paare sind ebenfalls ganz exakt;
laufender Seed-Recall beendet erzeugte Quellen100ms, erhält Room/Hz-History
und respektiert Autoplay-Gate0.14,3933min getrenntes PCM, alle251 Coreinputs
exakt grünerPR163. Keine DSP-/Storageaktivierung oder neue musikalische Freigabe.
SD41 bleibt Storage-/Gerätegate;11 technische Hauptblöcke,24 erledigt/30 offen.
Details:[SD41_BOOT_RECALL_CHECKPOINT.md](SD41_BOOT_RECALL_CHECKPOINT.md).
SD41 hat jetzt einen separat vorbereiteten Main-Journal-Schreiber: Begin ohne
I/O/Recovery, pro Step höchstens ein Start/Poll/32-B-Read, kein Warte- oder Scanloop.
Payload vor Commit; Cache erst nach vollständiger bestätigter Rückleseprüfung.
Abbruch hält einen laufenden I/O und seinen Puffer bis zum terminalen Poll fest.
1.126.722 Modellchecks,5252 gerichtete Fehlerfälle,79 Abbruchzustände und511
vollständige Saves bestehen; acht Längen sind bytegleich zum bestehenden JSC1.
Drei echte Product-PCM-Triplets bei512/64 erhalten sämtliche8s Audio/Hooks,
insgesamt72s. Modellierte20 Busy-Ticks pro I/O,333 Steps pro SCN7-Save;
das sind keine gemessenen Flashzeiten. Alle251 bisherigen Firmwareinputs
bleiben exakt PR164. Der neue Schreiber ist noch nicht an Scene/H743 angeschlossen,
Live-Save bleibt synchron und Journal OFF; SD41 bleibt offen, Zählung unverändert.
Details:[SD41_ASYNC_JOURNAL_CHECKPOINT.md](SD41_ASYNC_JOURNAL_CHECKPOINT.md).
SD41 hat jetzt auch eine separat vorbereitete schrittweise Recovery: Begin ohne
I/O, je Step höchstens ein32-B-Read und32-B-CRC, keine Record-/Sektorschleife.
Cacheverfügbarkeit wird beim Begin gesperrt; bestätigte Metadaten bleiben bis
zum ganzen erfolgreichen Scan erhalten. Fehler/Cancel veröffentlichen keinen
Teilfund.7288 Gegenproben zum alten Scanner,4983 Cut-/Retryfälle und35 Cancel-
Zustände bestehen;2.239.290 Checks, feste128-B-Hoststruktur, keine Flashmutation.
Neun tatsächliche12-s-PCM-Triplets in allen drei Worlds erhalten sämtliche
Audio-/Hookbytes bei64/512: Recovery, partieller Writefehler und Readfehler mit
erneutem Scan/Save. Drei Baselines werden geteilt;21 Render/252s getrenntes PCM.
Alle253 bisherigen Inputs einschließlich des vorbereiteten Writers exakt PR165,
darunter251 unveränderte Laufzeitinputs. Kein Scene-/H743-/CMake-Anschluss;
Live-Save bleibt synchron, Journal OFF und SD41 offen. Zählung unverändert.
Details:[SD41_JOURNAL_RECOVERY_CHECKPOINT.md](SD41_JOURNAL_RECOVERY_CHECKPOINT.md).
SD41 hat jetzt die vorbereitete Scene-Save-API: „Speichern läuft“ heißt angenommen,
„gespeichert“ erst nach vollständiger Rückleseprüfung. Echter368-B-SCN7-Snapshot
bei Begin ohne I/O; Regleränderungen während BUSY ändern ihn nicht. Bestätigte
Slots/aktive Scene bleiben bis DONE alt; Fehler/Cancel bestätigen keinen neuen
RAM-Stand. Ein physischer Commit kann trotz fehlgeschlagener Bestätigung bestehen;
vor Retry läuft bei unsicherem Cache die schrittweise Recovery, neuer Versuch
übernimmt aktuelle Werte.337 native Fälle/283.584 Checks,104 Writer-I/O-Fehler,
20 Recovery-Readfehler und206 Cancelpositionen bestehen.15 ganze12-s-PCM-Triplets
in allen Worlds bei64/512 sind exakt;33 Render/396s mit drei geteilten Baselines.
254/255 bisherige Inputs bytegleich; nur scenes.c enthält ausgeschaltete Hooks,
unaktivierte Reference-/Product-Übersetzungseinheiten vollständig exakt.
Product-only mit FAM_SCENE_SAVE_ASYNC_PREPARED separat geprüft; kein UI-/H743-/
CMake-/Defaultanschluss, Live-Save weiterhin synchron, Journal OFF. SD41 offen,
Zählung unverändert. Details:[SD41_SCENE_SAVE_CHECKPOINT.md](SD41_SCENE_SAVE_CHECKPOINT.md).
SD41 hat jetzt den separat vorbereiteten H743-Adapter: Bank-2-Erase/Program
per ST-HAL-Interrupt, DONE/ERROR erst nach quieszentem terminalem Poll. Bei
kombiniertem Erfolg/Fehler gewinnt der Fehler; Puffer und Besitz bleiben bis
zum Poll geschützt. Reads≤32B publizieren bei Fehler keine Teilbytes. Ein
begrenzter BusFault-Kandidat prüft genaue Leseinstruktion, präzisen Status,
BFAR und passende Bank-2-ECC-Adresse; andere Fehler werden nicht verschluckt.
328 echte Architektur-Framefälle (MSP/PSP, mit/ohne FPU),4480 Lesevarianten,
insgesamt70740 Checks sowie echte Recovery/Write/Cancel über denselben Adapter
bestehen, einschließlich ASan/UBSan. Cortex-M7 kompiliert und partiell mit den
Scene-Komponenten und echter ST-HAL gelinkt; geschütztes LDR/BX binär geprüft.
Alle258 bisherigen Firmwareinputs exakt PR167. Noch kein CMake-/Main-/Vektor-/
UI-Anschluss: Live-Save synchron, Journal OFF, keine reale ECC-/Zeitabnahme.
Details:[SD41_H743_SCENE_IO_CHECKPOINT.md](SD41_H743_SCENE_IO_CHECKPOINT.md).
SD41 hat jetzt auch diesen isolierten Geräte-Testbuild: eigener Main/Linker,
echte RESET-/SysTick-/FLASH-/BusFault-Vektoren und HAL-Callbacks im vollständigen
ARM-ELF geprüft. Alle263 bisherigen Firmwareinputs unverändert. Boot liest nur;
SWD-SAVE verlangt eine bestätigte, zeitlich begrenzte Einmalfreigabe.27752 native
Checks/2529 Ticks mit echtem Adapter/Writer/Recovery, max1 IO/Tick und ASan/UBSan
bestehen. BIN8668B ausschließlich Bank1, statisch2072B DTCM,16KiB Stackreserve.
Das Schreibmuster ist keine SCN7-Scene; Anleitung verlangt eigenes Testgerät
und Backup der benutzten Bank2-Journalsektoren. Kein Gerät programmiert, kein
Audio-/UI-/Produktanschluss. Vorheriger PR168 abschließend19/19 CI grün.
Details:[SD41_H743_SCENE_BENCH_CHECKPOINT.md](SD41_H743_SCENE_BENCH_CHECKPOINT.md).
Nächste Grenze: echte ECC-/Power-cut-/IRQ-Abnahme; ohne Gerät als nächste
einzelne unabhängige Softwareeinheit SD47. SD41 bleibt offen, Zählung unverändert.
SD47 Room-Übergabe ist jetzt als einzelner Softwarebefund behoben: Main
veröffentlicht alle Feedbackziele und Amount unter einer gemeinsamen Generation.
Audio übernimmt nur einen vollständigen Stand, mit einem Versuch ohne Warten;
max11 atomare Wortloads, bei laufendem Write der letzte gültige Zielcache.
Alter Teilpublikationsfehler reproduziert;335 gezielte Fälle/325 Unterbrechungen,
zehn Leser-Interleavings, Wrap/Invalid/Burstedits und ASan/UBSan bestehen.
24 ganze12-s-Product-Vergleiche über alle Worlds,Manual/Generate,Nature0/1 und
64/512 sind mit dem bisherigen Stand in PCM und echten Notenereignissen exakt.
Vollständiger gepaarter H743-Product-Link: Flash+80B,D2+40B,D1/DTCM gleich;
Compilerframes sind keine reale Stack-/Zeitabnahme. Alle262 anderen bisherigen
Firmwareinputs unverändert. Voriger PR169 jetzt vollständig20/20 CI erfolgreich.
Details:[SD47_ROOM_HANDOFF_CHECKPOINT.md](SD47_ROOM_HANDOFF_CHECKPOINT.md).
SD47 bleibt offen für die übrigen Hotpath-/Quellenübergaben; nächste einzelne
Einheit: Quellenvorbereitung/Start-Ack/Retirement bei gezielter Main-Audio-
Unterbrechung. Weiterhin24/54 geschlossen,30 offen,11 technische Blöcke.
Finale Quellen-/Raum-/Nature-Hörwahl
bleibt erforderlich. SD29/SD30-Windauswahl oder Naturumbau erst nach deren Nutzenentscheidung.
Keine UI vor Soundabnahme.
Weiterhin eine konkrete Arbeitseinheit nach der anderen.

## FUNDAMENTAL FALSCH

Die Referenzarchitektur mit fünf Presets, zusätzlichem Pad/Bass und neun FX
als fertige neue Produktidee auszugeben wäre falsch. Diese Produktpfade sind
im reduzierten Kandidaten tatsächlich ausgeschlossen und Scenes migriert.
Eine trockene alarmartige/summende/hohle Quelle mit Nature oder Hall zu
kaschieren bleibt ausgeschlossen. Harmonie-/Pegeltests belegen keine
Beruhigung und keine allgemeine Verträglichkeit.

## NOCH NICHT SELBSTVERSTÄNDLICH

Quellenregister, Color/SHAPE, Lautheitsbeziehung und Nutzen von Nature müssen
gehört werden. Der dritte Tonkörper wird bei fehlender eigener ruhiger Rolle
entfernt. Live-Save blockiert noch Main/Generate-Planung; der Default erhält
bei Power-loss während Sector-Erase keinen alten Save. Ein separat geprüfter
[Journal-Kandidat](SCENE_JOURNAL_CHECKPOINT.md) erhält den vorherigen Datensatz
im Flash-Modell, bleibt aber bis zu ECC-/Geräteabnahme deaktiviert.
Storage-/Geräteabnahme ist offen.
Die vorläufige physische Drive-Zuweisung ist funktionslos und muss vor einer
fertigen Produkt-UI verschwinden; die Audioarchitektur bekommt dafür kein FX.

## LOCKED

Als geprüfter technischer/produktlogischer Vertrag, nicht als gehörte Klangwahl:

- Erst tatsächlicher DSP-Start zählt in Hooks, Motiv und gehörter Historie.
- Drei gemeinsame Slots für Manual/Generate; Pluck zwei; Releases zählen.
- Originalbesitzer und angewandte Hz bleiben bis zum tatsächlichen Ende.
- Keine implizite Pad/Bass/Drone-/Rauschbegleitung; Nature default aus.
- Ein gemeinsamer interner Float-Raum; getrennte optionale Naturebene.
- Kein harter held Steal und kein neues Hall-/Pitchgedächtnis pro World.
- Kurze Quellenvergleiche bleiben kompakt; vollständige lange Ensemble-Phrasen
  dürfen45/50/70 s zeigen. Rohe Pegel und fester Hörgain sind getrennt dokumentiert.
- Konzept → Sound → UX/UI/Display → reale Geräteabnahme bleibt die Reihenfolge.

Die Tabellen A–E und Aufgabenbeschreibungen unten enthalten den ursprünglichen
Entwurfs-/Referenzausgangspunkt. Ihre **aktuelle** Umsetzung/Entscheidung steht
in der folgenden Statusmatrix und vollständig in der versionierten
Produktspezifikation. Historische Kandidatenbereiche sind keine zweiten Defaults.

## REMOVE / MERGE / REDESIGN

| Bestandteil | Entscheidung für die Zielarchitektur | Woran die Entscheidung hängt |
|---|---|---|
| Automatisches Bed, Bass, Eno-Loops, Drone | Kein Default; versteckte manuelle Begleitung entfernen | Gleiches Klangprinzip manuell und autonom; keine leeren Pausen mit Unterbau füllen |
| Sechs V2-Synths in `Synths_Archive` | Referenz/Kompatibilität; nach Scene-Migration aus Produktlink nehmen | Keine gespeicherte Bedeutung still ändern; tatsächliche Link-/RAM-Wirkung messen |
| Choir, Guembri, Ember, Glass und alternative Pad-Stimmen | Keine zusätzliche World allein aufgrund ihrer Existenz | SD00 entscheidet Erreichbarkeit; erhaltene Produktfunktion braucht eine eigene Rolle und volle Abnahme |
| Body | Zuerst mit/ohne vergleichen; bei fehlendem Nutzen entfernen | Eigenständiger Klangkörper ohne feste röhrenartige Peaks, Mono-/Übergangsschäden |
| Raum | Gemeinsamer reduzierter Raumpfad als Basis | Körper, Abstand und Tails wirken bei allen Welten zusammen |
| Delay | Nur behalten, wenn eine leise räumliche Antwort entsteht | Kein Taktgeber, keine automatisch neue kontrastierende Stimme |
| Chorus / Age / Blur | Rollen gegeneinander prüfen; redundante Bewegung zusammenführen oder entfernen | Quellen enthalten teils schon Verstimmung/Bewegung; zusätzlicher Effekt braucht hörbaren Mehrwert |
| Shimmer / Reverse Swell | Kein Default, kein autonomer Pflichtvorlauf; Produktnutzen zuerst beweisen | Kein hoher Alarmanteil, Phantomereignis oder harmonisch unverwalteter Ton |
| Dream Chain / neun Effektseiten | Keine obligatorische Kombination oder Anzahl | Kleinste verständliche Palette wählen; entfernte Funktionen wirklich aus Laufzeitpfaden nehmen |
| Wind / Regen / Wellen / weitere Texturen | Natur optional und getrennt vom Raum; zunächst aus für World-Abnahme | Ortswirkung statt Dauermaskierung; Aufwand muss hörbaren Nutzen rechtfertigen |
| Brightness, Mood, Resonance, Sweep, Envmod, Motion, Vibe | Bedeutungen zusammenführen oder begrenzen | Ein musikalischer Einfluss braucht eine klare, durchgehend brauchbare Wirkung |
| Bestehende fünf World-IDs | Versioniert migrieren, nicht umbenennen und umdeuten | Alte Scene lädt definiert oder erhält eine ausdrücklich dokumentierte Ersatzzuordnung |

## BESTE VERSION

AMBIENT erzeugt drei eigenständige Arten musikalischer Zeit aus einer gemeinsamen
Klanghaltung: **Zusammenhang weitergeben, einander antworten, Raum zwischen
Fragmenten lassen.** Jede Welt besitzt einen tragenden Tonkörper, begrenztes
Gedächtnis und eigene Regeln für Einsatz, Wiederholung, Variation und Ruhe.
Keine Welt ist eine zusätzliche Spur. Manuelles Spielen nutzt dieselbe Familie
und dieselben Klanggrenzen; der autonome Betrieb übernimmt die Zeitgestaltung.
Der gemeinsame Raum verbindet beide, ohne die Quellen unkenntlich zu machen.

### A. Drei hörbar verschiedene Systeme

Die Werte in dieser Tabelle sind **Startbereiche für die Konstruktion und den
Hörvergleich**, keine Eigenschaften des aktuellen Builds oder fertige Presets.
Aktuelle exakte Kandidatenwerte ersetzt ausschließlich das Parameter-/Grammatikregister
in PRODUCT_SOUND_SPEC.md; diese historische Entwurfstabelle ist kein Buildpreset.
Der finale Lock in SD51 bleibt von echten Abnahmen abhängig.

| Welt | Tonkörper und Identität | Ereignisgrammatik | Anfangsgrenzen und Verwerfungsgrund |
|---|---|---|---|
| COAST | Bowed-Kandidat: warmer, tragfähiger Ton; kein Flötenpfeifen, Bienen-/Orgelteppich oder synchrones Pumpen | Tragender Ton → verbindender gemeinsamer Ton → optionaler Farbton → individuelle Ablösung. Stimmen übergeben den Zusammenhang; nicht alle zugleich anschwellen | Gehaltene Töne zunächst 6–14 s; neue Einsätze ungefähr 4–10 s auseinander, individuell geplant. Meist 1–2 Quellen, dritte selten. Scheitert, wenn Verstimmung, gemeinsame Modulation oder Resonanzen die Identität dominieren |
| WOODLAND | Pluck-Kandidat: sanfter erkennbarer Anfang, hörbarer Grundton, abnehmende Obertonenergie; weder Klick noch Rassel | Kleine Figur → hörbare Antwort aus deren Merkmalen → begrenzte Variation → Ruhe. Motiverinnerung wirkt auf Kontur, Rhythmus und Register | Zunächst 2–3 Ereignisse pro Figur, 2–6 s innerhalb einer Figur, 6–16 s zwischen Figuren; lokale Pluck-Poolgrenze 2 beachten. Scheitert als bloßer Zufallsarpeggiator oder uniforme Glockenfolge |
| HIGHLANDS | Horn-Kandidat: runder Ton mit anderer Artikulation als COAST, ohne Rohr-/Signal-/Sirenencharakter | Kurzes Fragment → Nachklang → längere offene Pause → verwandtes oder neu kontrastiertes Fragment. Ruhe ist tragende Rolle | Zunächst 2–4 Töne pro Fragment, Tonlängen 1,5–5 s, 8–20 s Pause danach; meist 1 Quelle, 2 nur begründet. Keine regelmäßig alternierende Zweitonfolge. Quelle verwerfen, wenn Pfeifen/Hohlklang bleibt oder COAST nur dünner klingt |

Die Bereiche sind keine zufällig gewürfelten Timer. Eine Figur bekommt eine
zeitliche Absicht; eine Variation bewahrt mindestens ein erkennbares Merkmal.
Activity verkürzt später nicht blind jeden Abstand. Ruhe und lange Übergaben
bleiben auch am oberen brauchbaren Parameterende erhalten.

**HIGHLANDS-Abbruchregel:** Erst Ursache trocken isolieren, dann höchstens eine
begründete neue Horn-Fassung vergleichen. Bleibt sie unpassend, Horn als
Produktquelle verwerfen. Ein einfacherer harmonischer Tonkörper ist ein neuer,
gesondert abzunehmender Kandidat, kein zusätzlicher Layer. Solange kein eigener
ruhiger Klangkörper überzeugt, liefern zwei starke Welten die bessere Version
als eine künstlich erzwungene dritte.

### B. Gemeinsamer Klangvertrag

1. **Tonhöhe:** Geplante, im DSP tatsächlich angewandte und als klingend
   eingetragene Frequenz stimmen überein. Jeder Quellenbereich ist bekannt.
   Unzulässige Noten werden vor dem Commit abgelehnt; kein heimliches Clamp
   eines 20-Hz-Vorschlags zu einer realen 60-Hz-Note mit falscher Historie.
2. **Register:** Für den vergleichbaren Einstieg D als tonales Zentrum,
   gleiche Stimmung und das bisherige D3–A4 als Referenz. Danach wird pro Quelle
   ein hörbar tragfähiger Bereich festgelegt. Tiefe allein genügt nicht;
   Grundton/Obertöne müssen auch an der tatsächlichen Ausgangskette tragen.
3. **Harmonie:** Gemeinsamer Core, sparsame modale Farbe, weite tiefere Lagen,
   keine unkontrollierten tiefen Sekunden oder Tritoni. Farbwechsel prüfen
   laufende Quelltöne und relevante Effektnachklänge. Ruhe ist ein gültiges
   Ergebnis; ein dauerhaft leer blockiertes System ist ein Fehler.
4. **Belegung:** Ziel sind höchstens drei allozierte tonale Quellen über
   Familien, Weltwechsel und Releases hinweg; lokale kleinere Pools gelten
   zusätzlich. Eine Quelle belegt die tiefste tragende Rolle, keine unabhängige
   Grundtonverdopplung. Effekttails belegen Harmoniegedächtnis, keine neue Stimme.
5. **Pegel:** Feste Kalibrierung nach Quelle/Rolle/Register; keine laufende
   Normalisierung oder AGC. Generative Akzente bleiben klein. Aufnahmegain und
   tatsächliche Lautstärke sind getrennte Größen.
6. **Artikulation:** Erkennbarer weicher Anfang, natürliche Entwicklung,
   weicher kontrollierbarer Schluss. Attack darf einen Pluck nicht in einen
   anonymen Pad verwandeln; lange Releases dürfen keine versteckten Dauerstimmen
   erzeugen. Release und Raumfahne bleiben getrennte Zustände.
7. **Bewegung:** Zeitentwicklung stammt primär aus musikalischen Beziehungen.
   Mikrovariation ist begrenzt, pro Stimme und seed-reproduzierbar. Tonhöhe,
   Pegel, Farbe und Pan bewegen sich nicht alle gleichzeitig. Keine gemeinsame
   periodische Welle als Ersatz für Entwicklung.
8. **Raum:** Eine gemeinsame Raumtopologie, musikalisch kalibrierte Sends,
   erkennbare direkte Tonkörper, brauchbare Mono-Summe. Neue World schaltet keine
   zweite Arena ein und löscht nicht blind den vollen Nachhall.
9. **Stille:** Leerer Eingang erzeugt kein Effekt-Eigenbrummen/-rauschen.
   Ein musikalischer Stop beendet neue Ereignisse und lässt definierte Tails;
   Stillwerden beendet die gesamte Ausgabe weich und begrenzt. Master/Mute
   wirken nach dem Raum. Displayzustand ist kein Audiozustand.

### C. Vollständiges Inventar und Entscheidungspflicht

Jeder tatsächlich erreichbare Weg erhält in SD00 genau eines von **KEEP**
(Produktpfad, vollständig prüfen), **REMOVE** (Migration + alle Einstiegspunkte
entfernen) oder **REFERENCE** (nicht aus Produktbedienung/Scenes erreichbar).
Ein Name, Amount=0 oder ein verschobener Ordner beweist keine Entfernung.
Historische Codepfade werden nicht aufwendig retuned, wenn sie nicht ausliefern.

| Bereich | Tatsächlicher Ausgangspunkt / zu erfassende Wege | Ziel / zuständige Aufgaben |
|---|---|---|
| Primärquellen | `bowed.c`, `pluck.c`, `horn.c` | Drei Kandidaten trocken und in Mono; SD02–SD07 |
| Kompatibilitätsquellen | `choir.c`, `guembri.c`, `pad.c`, Pad-Stimmen/PADsynth, Ember/Glass, Bass/Sub/Deep/HarmonicBass, Drone; Oneshots und automatische Hooks | Erreichbarkeit statt Vermutung; SD00, SD08–SD10, SD36, SD46 |
| V2-Archiv | Sechs Familien in `src/v2/Synths_Archive`, verbundene native Parameter sowie Arp/Beat/Field-Wege | Weiter gelinkt; IDs/Scenes prüfen, danach nicht gewählte Produktpfade auslinken; SD00, SD36, SD46 |
| Tonführung | `harmony.c`, Pitch-Memory, `pitch_modes`, Equal/Just, Key/Mode/Vibe, Voice/Backend, Autoplay/Generate/Seed | Tatsächliche Töne statt nur Pitch Classes verwalten; SD07–SD17, SD40 |
| Tonkörper | `body.c`, `shape.c`, Gainadapter, Quellen-Farb-/Damp-Parameter | Kein pauschaler Weltmaterial-Filter; SD02–SD07, SD18, SD32–SD35 |
| Aktiver Masterraum | `fx_master.c` / `ambient_effects.h`: Bypass, Dark Reverb, Ping Pong Delay, Chorus Detune, Tape Age, Reverse Swell, Shimmer Reverb, Blur, Dream Chain | Alle neun erreichbaren Fälle beurteilen; kleinste gerechtfertigte Palette; SD19–SD27 |
| Weitere Raumwege | `reverb.c` und vorhandene Sends/Returnadapter; Legacy Echo/Tape/Blur/Shimmer-Dateien | H743 schließt die vier alten FX-Dateien bereits aus. Tatsächlich gelinkte/aufgerufene Wege erfassen; keine zweite Effektarena rekonstruieren; SD00, SD19–SD20, SD46 |
| Natur | `ambience.c`: Wind für alle bisherigen Welten bei Amount>0; Regen, Wellen/Sea-Hum, Fjord-/Desert-Varianten | Optional, kein default identity layer; Wind ist bereits unregelmäßig. Verbleibende klangliche Homogenität hören; SD28–SD31 |
| Texturen | `texture.c`: braunes Tiefrauschen, Breath, Warm/Air und erreichbare weitere Noise-/Crackle-Wege | Rumpeln, Bandpeaks, Maskierung und Idle-Ausgabe prüfen oder Pfad entfernen; SD00, SD31 |
| Endstufe | Dry/Send → Master-FX → Volume/Mute/DC/Peak/PCM und reale HAL-Ausgabe | Ganze Kette, nicht nur isolierte Synths; SD34, SD42–SD43, SD48–SD50 |
| Erinnerung | SCN6 mit SCN5-Migration, World-/Synth-IDs, Parameterdefaults, Scene-Recall/Seed | Kein unerwartetes Autoplay, keine heimliche Umdeutung; SD36, SD41 |

### D. Parametervertrag vor Tastenbelegung

SD32 führt ein versioniertes Register **jedes erreichbaren Soundparameters**.
Pflichtfelder: Zweck; Besitzer; Einheit; Minimum/Default/Maximum; hörbare Kurve;
Smoothing; Wirkung auf laufende oder nur neue Stimmen; Speicherverhalten;
Priorität gegenüber World-Preset; abhängige Parameter; Verhalten bei 0, Stille,
ungültigem Float und Szenenwechsel. Gültige Bereichsverletzungen werden begrenzt;
NaN/Inf werden verworfen und behalten den letzten gültigen Zielwert. Interne
Setter werden damit nicht automatisch zu Produktreglern.

| Vorhandene Parameterfamilie | Zielentscheidung, bevor sie erreichbar bleibt | Aufgabe |
|---|---|---|
| Key, Key-PC, sechs Modes, Equal/Just, Vibe, Mood | Welche musikalische Rolle bleibt? Keine doppelte Farbsteuerung. Alle freigegebenen Kombinationen innerhalb der Quellenregister; retune betrifft Besitzer/Fahnen definiert | SD11–SD12, SD32, SD40 |
| Source/Voice, Pad-Voice, Synth/Backend, native Param-Slots | In Produktwelten gebunden; Archivparameter nur mit bewusster Kompatibilitätsentscheidung erreichbar | SD00, SD10, SD32, SD36 |
| Attack, Release / SHAPE | Pro Familie erkennbare Artikulation über den gesamten Bereich; bestehende neutrale Defaults beibehalten, bis Hörvergleich bessere Werte liefert | SD02–SD07, SD32, SD35 |
| Pluck Damp; Bowed Colour und innere Bewegung; Horn Körper/Breath | Quellengebundene musikalische Grenzen, zuverlässiger Tonkörper, keine ungewollte Pitch-/Pegelbewegung | SD02–SD07, SD34 |
| Brightness, Resonance, Drive, Sweep, Envmod | Ein gemeinsamer Farbauftrag mit familiengeeigneter Umsetzung; wirkungslose/redundante Setter entfernen. Drive nie Voraussetzung für Körper | SD18, SD32–SD35 |
| Reverb Size/Damp/Drive, Wet-Amp, Source Send | Einen aktiven Raumvertrag wählen; Legacy- und Mastermappings nicht gleichzeitig widersprüchlich betreiben | SD19–SD20, SD32 |
| Space, Atmosphere, Texture | Raum und Natur trennen. Aktuell verändert Atmosphere sowohl Naturlevel als auch FX-Send | SD20, SD28, SD32–SD33 |
| Echo, Delay Seconds, Feedback | Antwort ohne Pulszwang; Zeitänderung ohne unkontrollierten Pitch-Sweep oder Pegelanstieg | SD21, SD32, SD35 |
| Motion, Age, Blur | Rollen/Modulationsbereiche trennen oder reduzieren; kein Summen, Flutter-Alarm, Grundtonverlust oder Doppeleffekt | SD22–SD24, SD27, SD32 |
| Shimmer, Reverse Swell, FX-Mode | Nur nach Nutzennachweis; Effekttöne zählen im Harmoniegedächtnis. Offline-Reverse ist kein Echtzeit-Feature | SD25–SD27, SD32 |
| FX Width, Tone, Level | Auch indirekte Presetwerte prüfen: Mono-Körper, kein schriller Maximalwert, kein versteckter World-Lautstärkeregler | SD19–SD20, SD32, SD34, SD42 |
| Bass Depth, Drone, Texture Amount, Autoplay Melody | Kein verdecktes Wiederaktivieren entfernter Begleitung durch Scene, Makro, Tuning oder Hook | SD00, SD10, SD32, SD36, SD41 |
| Generate Program/Seed, Dichte/Activity | Seed reproduziert musikalische Entscheidungen; Activity steuert Figuren/Übergaben, nicht beliebige Notenflut. Abgewiesener Start ist nicht gehört | SD13–SD17, SD33, SD38 |
| Master Volume, Mute, Clear/Stillwerden | Unabhängig von Quelle, World und Raum; keine ausgenommene Fahne oder Hintergrundquelle | SD39, SD41–SD42 |

Arbeitsmodell für spätere musikalische Makros: **Activity, Color, Room** plus
getrennte Lautstärke. Das legt weder Anzahl noch Position von Encodern/Tasten
fest. Eine vierte Klangdimension wird nur ergänzt, wenn sie einen unabhängigen,
sofort hörbaren Nutzen besitzt. Die elf Felder von `AmbientFxParameters` sind
damit kein Auftrag für elf Bedienelemente.

### E. Ressourcenvertrag

Gemessener Linkstand des oben genannten DSP-Commits, H743 Release,
Cortex-M7 Hard Float, `-O3 -DNDEBUG`:

| Bank | Belegt | Linkerbudget | Frei | Folgerung |
|---|---:|---:|---:|---|
| Flash | 255.316 B | 1.966.080 B | 1.710.764 B | Scene-Sektor bereits aus diesem Firmwarebudget ausgeklammert |
| DTCM | 119.440 B | 131.072 B | 11.632 B | Kritische Reserve; keine zusätzlichen großen automatischen/hot States hineinlegen |
| D1 | 417.408 B | 524.288 B | 106.880 B | Quellen/Übergänge anhand echter Sektionen budgetieren |
| D2 | 258.112 B | 294.912 B | 36.800 B | Aktive Master-FX-Arena bleibt hier; keine zweite große Arena |
| D3 / ITCM | je 0 B | je 65.536 B | je 65.536 B | Kein Beleg für freie DMA-taugliche oder austauschbare Speicherfläche |

Master-FX: 8 FDN-Linien, interne Arena bis 245.760 B, derzeit 44,1 kHz,
maximal 750 ms Delay und 220 ms Blur-Puffer. Drei Welten benötigen drei kleine
Grammatikzustände, keine drei Kopien davon. Jede DSP-Einheit weist Delta und
Bankplatzierung aus. Archivierung alleine hat keine Pools eingespart.

Der Produktplan arbeitet **ohne SD-Streaming und ohne erforderliche Samplebank**.
Eine SD-Nachrüstung kann erst aus PCB/Pins/Bus/Strom/Layout beurteilt werden;
sie ist keine Voraussetzung oder zugesagte Option. Eventuelle serielle externe
Speicher sind kein Ersatz für diese heißen Audio-Delay-Linien.

Projektziel am Gerät: DWT-`peak_load < 0.60`, null Deadline-Misses, nachgewiesene
Stackreserve im Worst Case. Das ist **noch nicht gemessen**. Linkerplatz sagt
nichts über Note-on-Spitzen, gleichzeitig retirierende alte Engines, Stack,
Interrupts oder die reale Ausgabekette.

### F. Historischer Referenznachweis — nicht erneut blind abarbeiten

Diese Tabelle beschreibt den Ausgangspunkt vor PR130–PR136; der aktuelle
Produktstand ist die Statusmatrix unten, nicht der alte manuelle/FX-Pfad.

| Ergebnis | Status und genaue Grenze |
|---|---|
| Generativer Start als Transaktion | Implementiert/Host geprüft; ungültige oder volle Starts verändern keine gehörte Historie. Allgemeine Parameter-/Actual-Pitch-Verträge bleiben SD07 |
| Keine automatische autonome Begleitung | Implementiert; Harmoniestufen starten keine Pad/Bass/Eno/Reverse-Pflichtstimmen. Alte manuelle Starts bleiben SD10 |
| Drei-Slot-Zulassung bei Generate und Handover | Implementiert; zählt auch bestehende Pad/Ember-, zwei Bass- und Drone-Zustände sowie Releases. Globale manuelle Belegung bleibt SD09 |
| Muted native/alte Ambient-Releases | Retirieren statt einzufrieren und später wiederzukehren. Zusätzliche Übergangs-CPU bleibt Geräteprüfung |
| WOODLAND-Pluck | Harmonische zentrierte Anregung, monotone symmetrische Dämpfung, feste Read-Delay-Phase, weicher 4–32-ms-Onset, Ausgabe-DC-Entfernung. Zwei lokale Stimmen und Besitz/Stop erhalten; kein fertiger World-Sound |
| Pluck-Nachweis | 35 statische + 6 Live-Pitch-Cases, worst static 0,034 Cent; zusätzliche Host-Regression und 26-s-Dry-A/B. Klang-/Decay-/Body-/Raumabnahme weiter offen |
| Frühere Bowed/Horn/FX-Korrekturen | Gemeinsames 5,1-Hz-Bowed-Vibrato entfernt, Grain reduziert; Horn-Suboktave/fester 950-Hz-Formant entfernt; Age-Eigenrauschen entfernt. Diese Ursachen nicht erneut als aktuelle Implementierung behaupten |
| H743/Host | Fünf CI-Jobs grün für den Referenzcode, obige Linkwerte. Kein CPU-, Lautsprecher- oder Nutzerurteil |

Belege: [WORLD_ROUTING_CHECKPOINT.md](WORLD_ROUTING_CHECKPOINT.md),
[WOODLAND_DRY_REVIEW.md](WOODLAND_DRY_REVIEW.md),
[WOODLAND_DRY_METRICS.json](WOODLAND_DRY_METRICS.json),
[BOWED_MOTION_REVIEW.md](BOWED_MOTION_REVIEW.md),
[ALPS_BODY_REVIEW.md](ALPS_BODY_REVIEW.md).

### G. Arbeitsmodus und Abnahmestufen

- **Entwurf:** Zweck, Signalweg, Grenzen, Default, Failure Modes und Entscheidung
  sind benannt. Ein Parameterbereich kann als ausdrücklich vorläufiger Kandidat
  festgelegt sein; die exakten Endwerte bleiben nicht unmarkiert offen.
- **Software geprüft:** Implementierung, relevante Hostprüfungen und bei DSP
  Änderungen H743-Release/Map stimmen mit dem Vertrag überein.
- **Gehört:** Kurzer reproduzierbarer Vergleich besteht die konkrete Hörfrage.
  Objektive Tonhöhen-/Pegelwerte sind kein Ersatz dafür. Offenes Nutzerurteil
  bleibt als offen protokolliert, statt durch eine Testzahl ersetzt zu werden.
- **Am Gerät locked:** Tatsächlicher Ausgang, CPU/Stack, Bedienintegration und
  Hörabnahme bestehen. Erst dann darf der Produktpfad als Sound-fertig gelten.

Pro Einheit: eine Frage beantworten, kleinste erforderliche Änderung, relevante
Prüfungen, gegebenenfalls ein <=30-s-Hörvergleich, Befund mit Commit/Seed/Pfad und
offener Restfrage sichern. Nicht Quellen-, Raum-, Grammatik- und UI-Umbau in
einem Hörvergleich vermischen. Keine Tests, die nur Tabellen abschreiben.
Ein entferntes Feature schließt seine Aufgabe erst, wenn alle Eintrittspunkte
und die Migration geprüft sind; ein ungehörter Klang wird nicht abgehakt.

### Umsetzung am 2026-10-06 nach Erstellung der Roadmap

PR130–PR134 sind in den Entwicklungsbranch gemerged: trockener COAST-/Inputfix,
reiner Score, echter reduzierter Kern, H743-Product-Profil / SCN7 und
Room-/Naturekorrekturen. Referenz bleibt Default, die drei Produktfamilien
bleiben Hörkandidaten.

PR135 liefert gehört bestätigte Motiv-/Intervallerinnerung und begrenzte
Kontextübergaben. Sein Code ist Bestandteil von PR136. PR136 schließt
natürliche Pluck-Key-up-Tails, samplegenaues Retirement, echten Mute,
vorbereitete Pitch-Kontextstornos, Endpoint-/Blindpakete und Summen-/Frameaudits.
Final gemerged über PR136: 476f4db068345c88469e141be2fa2f6dac81f6a3.
Geprüfter Head38f7d51b3f6e2dbe2c30026dc8cf26b8430dc364, CI37515501334,
sechs Jobs bestanden. Cacheguard, volle Übergänge und dichte Registerfälle
sind ebenfalls geprüft; PR135 ist als mitübernommener Draft geschlossen.

Zwölf +36 Minuten tatsächliches PCM pro vollem Hostlauf, konservative Hz-Tails,
64-/512-PCM exakt gleich; keine NaN/Inf oder Limiter-Eingriffe. 42 Summen-/
Registerprobes: mindestens 9,66 dB gemessene True-Peak-Reserve, schlechteste
Monoenergie 0,91874.
Hör-WAVs je 27 s, Raw und fixe Vergleichskopie; keine Lang-WAVs.

[PRODUCT_CORE_CHECKPOINT.md](PRODUCT_CORE_CHECKPOINT.md) dokumentiert SHA,
CI/Banken und Restgates. Die folgenden Checkboxes und Statusmatrix sind
**aktuell**, nicht die historischen pauschalen Open-/Next-Einträge.

### Aktuelle Statusmatrix — sämtliche 54 Aufgaben

Abgleich mit der wiedergefundenen PR129-To-do am 2026-10-07: deren pauschale
54 offenen Checkboxen sind überholt. PR130–PR136 liefern den unten belegten
Stand. Als nächster Softwareteil von SD41 liegt ein Zwei-Sektor-Journal mit
Abbruch-/Korruptionstests und Legacy-Erhalt vor; Aktivierung, ECC-sichere Reads
und nichtblockierendes Save sind noch offen. Siehe
[SCENE_JOURNAL_CHECKPOINT.md](SCENE_JOURNAL_CHECKPOINT.md).
Getesteter Code-Head `9768966cc6a3881ea9ea56cb9e8c2eff2b959a8a`,
[CI37602680211](https://github.com/Sancte3D/AMBIENT/actions/runs/37602680211):
6/6 Jobs bestanden, inklusive Default/Product/Journal-H743 und vollständiger
Host-Suite. Der Kandidat liegt im
[gemergten PR138](https://github.com/Sancte3D/AMBIENT/pull/138); die Hardware-Gates
und die 24/30-Zählung ändern sich dadurch nicht.

Fortsetzung einzeln ab dem ersten offenen Punkt: **SD02** hat jetzt getrennte
27-s-Gegenproben für 1,5f, 2f und Grain, beide Quellenkonstruktionen sowie
einen bytegleichen vollständigen Kontrollrender. Kein Produkt-DSP geändert,
keine Quellenwahl vorweggenommen. Messung und genaue Hörreihenfolge:
[SD02_COAST_COMPONENT_CHECKPOINT.md](SD02_COAST_COMPONENT_CHECKPOINT.md).
SD02 bleibt bis zur Hörentscheidung offen. Ein einzelner Durchlauf
hat **SD03 am bestehenden Kandidaten** geprüft: echte gemeinsame Stimmen
gegen separat gerenderte Summe, unabhängiges Grain, natürliche Releases und
bytegleiches Product-PCM bei 64/512 Frames. Die endgültige SD03-Klangabnahme
bleibt von SD02 abhängig; kein weiterer Sound-DSP-Umbau.
[SD03_COAST_INDEPENDENCE_CHECKPOINT.md](SD03_COAST_INDEPENDENCE_CHECKPOINT.md).
PR139 ist durch CI37608566520 (6/6 Jobs) vollständig software-/ARM-geprüft.
PR141 ist inzwischen durch CI37611452027 vollständig geprüft. **SD04** hat
jetzt einen trockenen, fest pegelgematchten COAST/HIGHLANDS-Vergleich über
D3/D4/A4, bytegleiche Ereignistraces und eine 27-s-Montage. Keine Klangwahl
oder neue Horn-Fassung vorweggenommen. Siehe
[SD04_HIGHLANDS_COAST_CHECKPOINT.md](SD04_HIGHLANDS_COAST_CHECKPOINT.md).
PR143 ist durch CI37783241714 vollständig geprüft. Das neue Nutzerfeedback
priorisiert jetzt den musikalischen Ensemble-Entwurf und seine Generatorübertragung.

`geschlossen` bedeutet: das genannte technische/Entfernungs-Kriterium ist
geprüft. Bei `Hörgate` ist der Code gebaut und softwareseitig auditiert, die
zugehörige Klangwahl bleibt offen. Geräte-/UX-Aufgaben werden nicht als erledigt
markiert. Geprüfte SHA/CI/Bankwerte und Hörartefakte stehen im Checkpoint.

| Aufgabe | Gesamtstand | Konkreter Nachweis / nächster tatsächlicher Gate |
|---|---|---|
| SD00 | geschlossen | Produkt-Compile-/Symbolinventar, Spec; jeder Eingang zugeordnet |
| SD01 | geschlossen | ARM-Banken/Compilerframes, keine zweite Arena; Gerätezeit separat SD48 |
| SD02 | Hörgate, isoliert vorbereitet | D3/D4/A4: voller Kontrollrender bytegleich; 1,5f/2f/Grain einzeln entfernt, 2×27-s-Montage; Quellenwahl offen |
| SD03 | Hörgate, unabhängig geprüft | Summenrest ≤1,50e-8, Grain-Korrelation <0,006, Sustain-Swing <0,012 dB, natürliche Releases, 64/512 PCM exakt; SD02/Hörwahl offen |
| SD04 | Hörgate / Keep-or-drop, vorbereitet | COAST/HIGHLANDS D3/D4/A4, identische Ereignisse, fixe −26-LUFS-Kopien und 27-s-A/B; Quellenwahl offen, zwei starke Worlds zulässig |
| SD05 | Hörgate | Horn-LFO/Sub/Formant aus; Onset-Air und SHAPE-Endpunkte prüfen |
| SD06 | Lange Quelle integriert; Hörgate | Product-Saite36-s-Nominaldecay,0,8-s-Attackbasis, eigener weicher Release; Default-Onset/Body0,031 und8-s/Body0,298, natürliche Manual-Tails; Quellenhörwahl offen |
| SD07 | geschlossen | Finite Input, tatsächliche Hz/Velocity, Bereiche, rejected Starts ohne Source/Hook |
| SD08 | geschlossen | Originalowner, DSP-Ack, Storno, tatsächliche One-shot-/Release-Enden |
| SD09 | geschlossen | Drei globale Slots / zwei Plucks auch Manual, kein held Steal |
| SD10 | geschlossen | Gemeinsame Familie/Admit ohne Pad/Bass/Archive; SCN7-Migration geprüft |
| SD11 | Software geprüft; Hörgruppe |40032 öffentliche Hz-/Paarfälle,8060928 ganze Rejektions-Frames exakt; zwei Cores/12 Keys/Equal/Just, private Register in je48 echten Phrasen; aktuelle Teiltonbelege zugeordnet, Teilton-/Registerhörwahl offen |
| SD12 | Software geprüft; Hörgruppe |24 echte Rejektions-/Room-/Wrapfälle,45962784 ganze Kontrollframes,6 Common-tone/6 Cancel,10 Stategrenzen; Freigabepartition maximal10ms verschieden;7,2s/250ms/1e-5 hören |
| SD13 | geschlossen | Alle drei Worlds: eigene begrenzte Rollen-/Phrasenzustände mit DSP-Ack; alte Einzeltickets/Alternativsuche im Product ersetzt; cancel/retry/Timer gültig, kein gehörtes Motiv ohne Start |
| SD14 | Generator integriert; Hör-/Gerätegate offen | Gegenbewegung, gemeinsamer Akkordton, einmaliger Mittelton; echte Besitzer/Releases/Tails,48 Engine-Phrasen, volle Host-Suite und H743 geprüft; neue70-s-Probe noch hören |
| SD15 | Lange Quelle und Generator integriert; Hör-/Gerätegate offen | Sechs lange Akkordrollen-Starts mit real gehaltenen gemeinsamen Tönen, zwei Saiten inklusive Release,48 echte Phrasen und50-s-Engineprobe; kein Wiederanschlag gemeinsamer Töne; neue Hörwahl offen |
| SD16 | Generator integriert; Hör-/Gerätegate offen | Weiter Akkord mit demselben gehaltenen Ton, Pause erst nach tatsächlichem Ende aller drei, verwandte Rückkehr;48 echte Phrasen +4 Seed/Wrap,65-s-Engineprobe, volle Host-Suite und H743 bestehen; finale Quellen-/Hörwahl offen |
| SD17 | Rollenentwicklung integriert; Hörgate |2304 Pure-Cases×16 Phrasen,90min tatsächliches Engine-PCM und30min64/512-genaue Traces je Partition; Home/Variation/relative Lage/erinnerte Rückkehr, begrenztes Gedächtnis,3 kurze spätere Vergleiche; Form und Wiedererkennbarkeit hören |
| SD18 | geschlossen durch Remove | Body nicht kompiliert; kein Materialreset oder additive Röhrenfärbung |
| SD19 | Kalibrierung integriert; Hörgate | Rücklauf5*r, Room0,5 jetzt8,86–13,99% statt0,66–1,04% RMS/Dry; echte Held/Release-A/B-Traces identisch, Float-Decay/Mono stabil, volle Host-Suite/H743, RAM gleich; finale Raumwahl offen |
| SD20 | geschlossen | Source-Send 0,35, ein Room, unabhängige Nature, gemeinsame Userwerte erhalten |
| SD21 | geschlossen durch Remove | Eigenständiger Echo-Pfad/Parameter/Scene-FX ausgeschlossen |
| SD22 | geschlossen durch Remove | Chorus/zusätzlicher Detune ausgeschlossen |
| SD23 | geschlossen durch Remove | Tape Age und Eigenhiss/-hum ausgeschlossen |
| SD24 | geschlossen durch Remove | Blur/Arena/Altparameter ausgeschlossen |
| SD25 | geschlossen durch Remove | Shimmer/Oktavregeneration ausgeschlossen |
| SD26 | geschlossen durch Remove | Reverse-/Generate-Vorswell ausgeschlossen |
| SD27 | geschlossen | Nur Dry/Room, exakte Null/kalt, kein Tankrevival; Scenes reaktivieren nichts |
| SD28 | Vergleich vorbereitet; Hörgate | Drei 27,5-s-Mit/Ohne-Proben, 18×112 s echte Fälle mit zwei Seeds und Nature0/.35/1; Noten/Besitzer/Phrasen64/512 gleich, Room unabhängig, Stop still; Nutzen und Weglassen offen |
| SD29 | Hörgate | Unregelmäßige Wetter-/Gustuhren und Seedstarts; Homogenität nicht numerisch freigeben |
| SD30 | Hörgate | Nur neue leise Wellen/Tropfen, kein Sea-Hum oder Sampleloop; Nutzen hören |
| SD31 | Software geprüft; Hörgruppe | Aktuelles ARM-Artefakt/73 LOAD-Objekte,263 gleiche Inputs und vollständiges Eintritts-/Idle-/Rolleninventar; Alttexturen auslinkt, nur deklarierte Nature/Quellenartikulation; gehörte Rolle/Resonanz/Maskierung offen |
| SD32 | geschlossen | Vollständiges normiertes Parameterregister samt Aliases/retired API und Priorität |
| SD33 | Software geprüft; Hörgruppe | Kennlinien und spätere Fixes konsolidiert,7 öffentliche Setter/10 Release-Corequellen bytegleich,263 Inputs aktueller Host/H743-Basis; musikalische Endwerte und Nutzen hören |
| SD34 | Softwareprüfung abgeschlossen; Hörgate | 596 Render/209,2min;105 Solos/162 Summen/24 Genfälle; feste Gains,9,5dB Referenz-/5,1dB Maxreserve; Lautheitsbalance hören |
| SD35 | Softwareprüfung abgeschlossen; Hörgate | 77 aktuelle Szenarien/157 Render;1049 echte Quellenreservierungen,64/512 exakt, schnelle Regler/volle natürliche Tails; musikalische Endwertwahl offen |
| SD36 | geschlossen | SCN5/6→SCN7, 368 B, CRC, Provenienz, fehlgeschlagener Save-RAM-Rollback |
| SD37 | Aktuelle Softwareprüfung abgeschlossen; Hörgate | 54 Fälle/108 Render;670 echte Starts,678 Reservierungen,alle sechs Richtungen/volle Pools/Pending/Rapid/Room/Nature/Wrap; tatsächliche Übergänge hören |
| SD38 | geschlossen | Entry ≤100 ms, Stop natürliche Releases, begrenztes Retry, echte Onsets |
| SD39 | geschlossen | Clear/Mute ≤40 ms exakt Null, kein alter Tank bei Unmute, Targets bewahrt |
| SD40 | geschlossen | Held Hz unverändert; neue Key/Mode/Tuning, ungehörte Vorbereitung storniert, Setter idempotent |
| SD41 | Storage-/Gerätegate | Boot/Recall, bestätigte Save-API, Writer/Recovery und H743-IO separat geprüft; isolierter Gerätebuild/Vektoranschluss und reale ECC-/Power-cut-/IRQ-Abnahme offen, Journal OFF |
| SD42 | geschlossen softwareseitig | 42 Summen-/Registerprobes: True Peak ≥6 dB Reserve, DC/Mono/NaN/Clip; physische Ausgabe SD49 |
| SD43 | geschlossen | Echte C-Kette, SHA/Parameter/Trace/rohe Pegel/fester Gain; jeder Hör-WAV 27 s |
| SD44 | Hörgate | Blindpaket A/B/C: gleiche Key/Tuning, ohne Room/Nature; Antwortschlüssel separat |
| SD45 | Software geprüft; Hörgruppe | Zusätzlich800-s-World-/Scene-/Recovery-Sitzungen,2 Seeds/64/512 und vier volle Mainwrap-PCM-/Hookpaare;141120000 Stereo-Frames je Modus, native/ASan/UBSan; Langzeitform hören, Flash/ECC weiterSD41 |
| SD46 | geschlossen | Tatsächlicher ARM-Compile-/Funktionssymbol-/DMA-/Bank-Audit, Archive nicht nur umbenannt |
| SD47 | DSP-/Übergabeinventar abgeschlossen; SD41/SD48 offen | Quellen/Room/Nature, feste Arbeit und Einzelframes geprüft; Rest exakt synchroner Product-Save (SD41) und ganze Geräte-IRQ-/Stackmessung (SD48), kein unbestimmtes Restinventar |
| SD48 | echtes Gerät | DWT <0,60, null Misses und realer Stack-High-water mit UI/MIDI/Storage |
| SD49 | echtes Gerät | Gebauter Ausgang, Pegel/DC/Noise/Pops/Lasten/Clock/Power/Cache/Flash prüfen |
| SD50 | echte Hörer + Gerät | Alarm/Tube/Buzz/Beep/Ermüdung konkret lösen, keine Heilbehauptung |
| SD51 | Software konsolidiert, finaler Freeze offen | Sound Candidate0.10/SCN7/265 Inputhashes; endgültige Endwerte erst nach Quellen-/Hör-/Geräteabnahme |
| SD52 | Softwareanbindung abgeschlossen | Bestehende H743-Bedienung,15-min-Display-/LED-Ruhe, Wake ohne Klangereignis geprüft |
| SD53 | Produkt-DoD offen | Alle zutreffenden Gates bestehen; erst dann Sounddesign fertig nennen |

## Verbindliche Arbeitsliste

**54 Aufgaben, SD00–SD53: 25 vollständig geschlossene Software-/Entfernungsaufgaben,
29 Aufgaben mit offener Quellen-, Hör-, Geräte-, Storage- oder UX-Abnahme.** Bereits erledigte Teilbefunde stehen oben und
in den entsprechenden Aufgaben. Jede Checkbox bleibt offen, bis die zugehörige
Abnahme besteht oder der gesamte betroffene Produktpfad nachweislich entfernt
wurde. Abhängigkeiten beschreiben die Reihenfolge, keine zusätzliche Architektur.
Bei Tailwerten und Migration gilt eine ausdrückliche Zweistufigkeit: zuerst
den benötigten Vertrag bzw. KEEP/REMOVE-Entscheid festlegen, dann nach Aufbau
des Zielpfads seine endgültigen Werte bzw. Migration bestätigen. SD19 benötigt
den Quellenvertrag aus SD12, nicht dessen noch ungemessene finale Raumtailwerte.
SD36 benötigt die Effektentscheidung aus SD27; die Entfernung wird anschließend
gegen die migrierten Scenes geprüft. Keine zirkulären Freigabebedingungen.

### 1. Inventar, trockene Quellen, ehrliche Stimmenführung

- [x] **SD00 — Erreichbare Soundpfade abschließend klassifizieren.**
  Für obiges Inventar sämtliche Einstiegspunkte aus Manual, Generate, Scene,
  Makro/Hook und V2 erfassen; KEEP/REMOVE/REFERENCE mit Begründung zuweisen.
  **Abnahme:** Kein gelinkter, bedienbarer oder aus Scene ladbarer Klangweg ohne
  Zuordnung. Nicht ausgewählte Stimmen erhalten keine neue Sounddesignserie.
  Voraussetzung: aktueller Routingcheckpoint; vor SD10/SD27/SD31/SD46.
- [x] **SD01 — Ressourcenbaseline als fortlaufenden Vertrag führen.**
  Obige gemessene Bankwerte und Compilerkonfiguration übernehmen; jede weitere
  Einheit mit State-/Flash-/Stack-/Bankdelta und Quelle des Nachweises erfassen.
  **Abnahme:** Tatsächlicher H743-Link statt Host-`sizeof`; keine nicht budgetierte
  zweite Arena, große Stackvorbereitung oder Samplebank. Gerätezeit offen lassen.
- [ ] **SD02 — COAST-Tonkörper trocken auswählen.**
  Bowed-Grundton, Begleitsäge mit Faktor 1,0041, sympatische Resonanzen bei
  1,5f/2f und spektrale Farbe getrennt vergleichen. Die 1,5f-Komponente auf
  harmonische/raue Wahrnehmung prüfen; keine pauschale Resonanzverstärkung.
  **Abnahme:** Tragfähiger eigener Ton über das gewählte Register, gleicher
  Pegel in beiden Farben, Mono erhalten; weder Pfeifen noch Orgel/Buzz/Tube.
  **Fortsetzung 2026-10-07:** Zehn getrennte Komponenten-/Registerproben und
  zwei 27-s-Hörmontagen, volle Kontrollquelle bytegleich, rohe Pegel und
  konstante Hörgains dokumentiert. Siehe SD02-Checkpoint; nicht als gehört abgehakt.
  Nächste konkrete Quelle; vor SD14/SD18.
- [ ] **SD03 — COAST-Bewegung von der Quelle aus ordnen.**
  Wiederkehrende 0,13-Hz-Körperphase, identischen Startwert und Grain getrennt
  prüfen. Das frühere 5,1-Hz-Gemeinschaftsvibrato ist bereits entfernt.
  **Abnahme:** Mehrere Stimmen beginnen/entwickeln sich eigenständig; keine
  mechanische gemeinsame Welle, störende Schwebung oder Rauschdecke. Gegen
  bewegungslose Referenz prüfen, nicht einfach zusätzliche Zufälligkeit einbauen.
  **Fortsetzung 2026-10-07:** Drei echte Stimmen einschließlich sämtlicher
  Releases gegen Einzelstimmensumme geprüft, aktuelles Grain gegen stationäre
  no-Grain-Referenz, zwei volle 27-s-Passagen und kurze Montage. Product-PCM
  bei 64/512 Frames bytegleich. SD03-Checkpoint; SD02-Auswahl/Hörabnahme offen.
  Voraussetzung: SD02.
- [ ] **SD04 — HIGHLANDS-Tonkörper behalten oder verwerfen.**
  Horn trocken bei gematchtem Pegel gegen COAST: Grundton, Obertöne, Register,
  Filterkörper. Keine Suboktave/festen 950-Hz-Formanten wieder hinzufügen.
  **Abnahme:** Eigenständige warme Artikulation ohne Signal-/Röhrencharakter.
  Bei Scheitern gilt die Abbruchregel oben; keine Umbenennung als Ersatz.
  **Fortsetzung 2026-10-08:** Echter trockener Product-Renderer, bytegleiche
  Ereignistraces, D3/D4/A4 in RAW und fest pegelgematchten LISTEN-Kopien sowie
  sechs kurze A/B-Ausschnitte in 27 s. SD04-Checkpoint; KEEP/REMOVE bleibt offen.
  Clear-/Ausschnittgrenzen sind dokumentiert, keine natürliche Release-Freigabe.
  Vor SD16; nicht mit Raum kaschieren.
- [ ] **SD05 — HIGHLANDS-Artikulation und Atembewegung begrenzen.**
  Verbleibende ungefähr 0,9-Hz-Filterbewegung und kurze Breath-Textur isolieren;
  Beginn/Ende bei minimaler und maximaler SHAPE prüfen.
  **Abnahme:** Kein hörbares Wah, Sirenenanstieg oder angesetztes Flötenpiepen;
  selbst kurze Fragmente sind rund. Dauer und Kontur tragen, statt Effektbewegung.
  Voraussetzung: SD04 oder freigegebener Ersatzkandidat.
- [ ] **SD06 — WOODLAND-Klang und musikalischen Decay fertig auswählen.**
  Den korrigierten Pluck hören: trockenes Register, Anregungsvariation, sämtliche
  Damp-/SHAPE-Grenzen, leise Wiederholungen und zwei lokale Releases. Technische
  Pitch-/Repeat-Pegelkorrektur ist erledigt; nicht denselben Fix erneut schreiben.
  **Abnahme:** Erkennbarer sanfter Tonbeginn und nützlicher Ausklang ohne Rassel,
  Klick, dominantes Oberton-Piepen oder anonymen Swell. Referenz und rohe Pegel
  dokumentieren. Vor SD15/SD18.
- [x] **SD07 — Gemeinsamen Quellen-/Inputvertrag schließen.**
  Actual Pitch, erlaubte Frequenz, Velocity/Gain, Attack/Release und Status pro
  Familie definieren. Pluck-Minimum 60 Hz gegenüber World-API 20 Hz angleichen
  oder vor Zulassung ausdrücklich ablehnen. Float-Parameter finite-validieren;
  `shape.c` verwirft inzwischen NaN/Inf; der gemeinsame Vertrag wird gegen echtes PCM geprüft.
  **Abnahme:** Ungültiger Input erzeugt weder Stimme noch Commit/Hook/RNG-Effekt;
  gültige Note wird mit ihrer tatsächlichen Frequenz eingetragen. Vor SD08–SD12.
- [x] **SD08 — Besitzer und sämtliche One-shot-Tails vollständig verfolgen.**
  Auch unowned Guembri-/Oneshot- und manuelle Hooks erfassen, solange erreichbar.
  Press → World/Modus wechseln → Release muss ursprüngliche Quelle treffen.
  **Abnahme:** Keine Phantomnote, hängenbleibende Quelle oder verlorene relevante
  Fahne; Hooks melden nur erfolgreiche reale Starts. Entfernte Pfade brauchen
  keine neue Stimmenarchitektur. Voraussetzung: SD00/SD07.
- [x] **SD09 — Globale Belegung auch manuell verbindlich machen.**
  Drei-Slot-Ziel über alle verbleibenden Quellen, Releases und Übergänge; lokale
  Pluck-Grenze zwei zusätzlich. Generate-Gate ist bereits vorhanden. Für Manual
  höchstens eine definierte weiche Übergabe bereits losgelassener Quellen;
  keine held Note hart stehlen, keine unbegrenzte Warteschlange.
  **Abnahme:** Full-pool/Press/Release/Wechsel kann Budget nicht umgehen. Alle
  held Slots voll bedeutet ehrlich abweisen; keine fünfte Quelle allein wegen
  fünf Eingabeflächen versprechen. Voraussetzung: SD07–SD08.
- [x] **SD10 — Manuelle Palette auf World-Quellen vereinheitlichen.**
  Manuell dieselbe ausgewählte Familie und Harmonik nutzen; implizites Pad,
  Bass-follow und konkurrierende Autoplay-Hooks aus diesem Produktweg entfernen.
  **Abnahme:** Ein manueller Einsatz startet genau den angekündigten Tonkörper;
  Loslassen/Stop folgt derselben Besitzerlogik. Kompatibilität erst mit SD36
  entfernen; keine vorweggenommene Tastenbelegung. Voraussetzung: SD00/SD08–SD09.

### 2. Harmonie und drei echte Kompositionssysteme

- [ ] **SD11 — Gemeinsame Harmonik und Quellenregister final konstruieren.**
  Core, modale Farben, gemeinsame Töne, minimale tiefe Abstände, Rollen und
  Voice Leading definieren. Alle sechs bestehenden Modes und Equal/Just prüfen;
  Modes ohne eigenen ruhigen Nutzen entfernen statt als Anzahl verteidigen.
  **Abnahme:** Tatsächliche Frequenzen/Grundtöne und relevante Teiltonkonflikte
  berücksichtigen; keine Registerflucht, tiefe Reibung oder bloßer Parallelakkord.
  Key-Transposition bleibt im akzeptierten Quellenbereich. Voraussetzung: SD02–SD07.
  **Technischer Abschluss2026-10-18:**40032 öffentliche Einzel-/Paarfälle,
  tatsächliche Hz/Acks und8060928 bytegleiche Rejektions-Frames; nativ und
  ASan/UBSan. Zusammen mit All-key-Generatoren und aktuellen Komponenten-
  belegen ist keine weitere technische Zulassungslücke belegt. SD11 jetzt
  Hörgruppe, weiterhin ungecheckt für Quellen-/Teilton-/Registerwahl.
  [SD11_PUBLIC_HARMONY_CHECKPOINT.md](SD11_PUBLIC_HARMONY_CHECKPOINT.md).
- [ ] **SD12 — Harmoniegedächtnis mit realen Releases/Effekttails verbinden.**
  Quellrelease, Raum, Echo/Blur und gegebenenfalls Shimmer-Oktave konservativ
  erfassen; Timeout/Schwellwert aus hörbarer Energie begründen.
  **Abnahme:** Farbwechsel wartet bei relevanter Kollision, gemeinsamer Ton darf
  verbinden, alte leise Fahne sperrt nicht dauerhaft jeden neuen Ton. Ein
  abgewiesener Vorschlag verbraucht keine gehörte Historie. Voraussetzung: SD08/SD11;
  endgültige Tailwerte nach SD19–SD27 nachführen.
  **Technischer Abschluss2026-10-18:**24 echte Source-/Room-/Wrapfälle,
  ganze Kontrollausgaben45962784 Stereo-Frames,6 Common-tone/6 Cancel und10
  getrennte Stategrenzen.263 Firmwareinputs unverändert; native/Sanitizer grün.
  Quiet-Freigabe aus Blockmaxima, beobachtet maximal10ms Partitionsabweichung;
  kein samplegenauer250-ms- oder globaler64/512-Bytegleichheitsclaim.
  SD12 jetzt Hörgruppe, weiterhin ungecheckt für gehörte Energie-/Wartewerte.
  [SD12_TAIL_HISTORY_CHECKPOINT.md](SD12_TAIL_HISTORY_CHECKPOINT.md).
- [x] **SD13 — World-Datenmodell und begrenzte Ereigniszustände bauen.**
  Je Welt kleine Zustandsmaschine, Motivspeicher, eigener Planungszustand/Seed,
  Quellenrollen und Defaults; gemeinsame Zulassung als Transaktion beibehalten.
  **Abnahme:** Vorschlag, Zulassung, Beginn, Ende und Ruhe getrennt; begrenzte
  Suche/retry; unterschiedliche Grammatik statt weiterer Preset-Flags. Keine
  World-ID still wiederverwenden. Voraussetzung: SD09/SD11–SD12.
- [ ] **SD14 — COAST-Grammatik implementieren.**
  Individuelle Stimmenübergaben mit gemeinsamem Ton; optionaler Farbton nur bei
  Platz, keinesfalls obligatorischer dritter Layer. Tonbeziehungen verändern,
  statt dieselbe Fläche in festen Abständen neu zu starten.
  **Abnahme:** Dry-Eventtrace zeigt eigenständige Hüllkurven/überlappende Übergaben,
  sinnvolle Pausen und begrenzte Rollen. Auch ohne Pan/Hall hörbare Kontinuität.
  **Neue Priorität 2026-10-08:** Ein hörbarer Akkord-/Voicing-Bogen mit
  gemeinsamen gehaltenen Tönen, gezieltem Registerwechsel und eigenen Dauern
  wurde als Richtung positiv gehört. Die COAST-Gegenbewegung ist jetzt im
  autonomen Product-Generator mit drei Rollen und acht tatsächlichen Starts
  integriert. Der Mittelton wird einmal gestartet; Slots und unveränderte
  Raumhistory warten echte Releases ab. Alle12 Keys, beide Collections und
  Stimmungen, Host-Suite und H743-Build geprüft. Neue70-s-Firmwareprobe und
  endgültige Quellen-/Gerätewahl bleiben offen; deshalb kein Gesamt-Haken.
  Siehe COAST-Generator-Checkpoint und Ensemble-Direction.
  Voraussetzung: SD02–SD03/SD13; technische Basis ist keine musikalische Abnahme.
- [ ] **SD15 — WOODLAND-Grammatik implementieren.**
  Seed-Figur mit Kontur/Abstandsmerkmalen; Antwort bewahrt ausgewählte Merkmale,
  Variation verändert nur begrenzte Eigenschaften; dann wirkliche Ruhe.
  **Abnahme:** Antworten sind aus gehörten Figuren ableitbar, fehlgeschlagene
  Starts nicht als Motiv gespeichert; kein regelmäßiger Zufallsarp oder
  endloses Einzelton-Ping. Zwei lokale Slots berücksichtigen. Voraussetzung: SD06/SD13.
  **Aktuell nach Nutzerkorrektur:** Die kurze gezupfte Richtung ist verworfen.
  Lange Saite und eigene Sechs-Schritt-Akkordrolle sind jetzt integriert:
  weicher Anfang, gehaltene gemeinsame Töne, individuelle Halteabsichten,
  eigener weicher Release und Wiederkehr erst nach echter Freigabe der Besitzer.
  Default-Dry/Room-Probe50 s,48 echte Engine-Phrasen über Keys/Collections/
  Stimmungen und H743 geprüft. Neue Quellen-/World-Hörwahl und Geräteabnahme
  bleiben offen; Gesamt-Haken folgt daraus noch nicht.
- [ ] **SD16 — HIGHLANDS-Grammatik implementieren.**
  Kurze zusammenhängende Fragmente, variierte Kontur, danach lange offene Pause;
  kein festes hin/her zweier Töne oder dauerhaft gerufener Grundton.
  **Abnahme:** Eigenes Zeitempfinden ohne Naturebene, Pitch-Sweep oder extremen
  Hall; Ruhe wirkt beabsichtigt. Nur implementieren, wenn SD04–SD05 bestehen;
  sonst produktseitig zurückstellen. Voraussetzung: SD13.
  **Aktueller Kandidat:** Acht-Schritt-Akkordrollen jetzt im Product-Generator:
  D3/A4/F#4 → B3/D4 mit gehaltenem F#4 → tatsächliches Quellenende → offene
  Pause → verwandte weite Rückkehr. Normale Horn-/Makroartikulation, private
  Core-MIDI45..75/105..650 Hz, öffentliche Grenzen bleiben.48 vollständige
  Keys/Collections/Stimmungen plus4 echte Seed/Timerwrap-Phrasen, zehn
  canceled-start-Pfade, volle Host-Suite und H743 geprüft. Neue65-s-Engineprobe
  braucht Hörurteil; Quelle/Hör-/Gerätegate bleibt offen, kein Gesamt-Haken.
- [ ] **SD17 — Langfristige Entwicklung mit Wiedererkennbarkeit bauen.**
  Ereignisse zu Episoden ordnen; Ausgangskandidat 45–120 s, ohne periodischen
  Pflichtreset. Wiederkehr, lokale Variation und Ruhe in musikalischer Reihenfolge;
  Rückkehr erinnert, ohne identischen Loop vorzuspielen.
  **Abnahme:** Mehrere Seeds erzeugen weder erkennbare kurze Endlosschleife noch
  orientierungslosen Wandel, unbegrenzte Dichte oder permanenten Halt. Gedächtnis
  und Suchaufwand bleiben begrenzt. Voraussetzung: SD14–SD16 bzw. ausgewählte Welten.
  **Aktueller Kandidat:**24B Heard-Memory pro World; erste vollständige Phrase
  lernen, lokal variieren, relativ revoicen, erste Ton-/Rollen-/Akzentfolge mit
  veränderter Zeitabsicht zurückholen.2–4 Zwischenphrasen, lebender RNG ohne
  Reset, normale Besitzer-/Tailprüfung, keine neue Audiostimme.2304 Pure-Cases
  über je16 Phrasen, neun10-Minuten-Engine-Läufe und drei600-s-Kontrollrender
  bei64/512 bestehen. Kurze29,5-s-Vergleiche zeigen Anfang/verwandte Lage/
  Rückkehr. Volle Host-Suite und H743 geprüft; endgültige Hör-/Langzeitabnahme
  bleibt offen, Gesamt-Haken wird nicht aus Testzahlen abgeleitet.

### 3. Tonkörper, gemeinsamer Raum und sämtliche Effekte

- [x] **SD18 — Body mit/ohne entscheiden und Übergänge korrigieren.**
  Feste 4–6-Moden-Weltmaterialien, additive dry+wet-Wirkung, Amount 0,38,
  Links/Rechts-Versatz und State-Reset beim Worldwechsel isolieren.
  **Abnahme:** Klarer hörbarer Nutzen bei gleicher Lautheit, kein Tube-/Metallpeak,
  Mono-Ausfall oder Tailabriss. Fehlt der Nutzen: Body entfernen. Bei Behalten
  neue Quellenfarben begründen statt alte fünf Materialien blind übernehmen.
  Voraussetzung: SD02–SD06; vor finaler Raum-/Gainabnahme.
- [ ] **SD19 — Einen gemeinsamen Raum als Produktbasis wählen.**
  FDN-Körper, frühe/diffuse Anteile, Dämpfung, Decay, Breite und Tail-Harmonie
  für alle Quellen kalibrieren. Keine zweite Arena; keine unkontrollierte
  Delaylängenänderung, die live Tonhöhe bewegt.
  **Abnahme:** Quelle bleibt ortbar und tragfähig, Raum verbindet alle Welten;
  Mono, Impuls, tiefer/mittlerer Sustain und volle drei Quellen stabil. Init-Fail
  bleibt definierter Dry-Fallback. Voraussetzung: SD18/SD12.
  **Vorbereitung2026-10-10:** gleicher Ausschnitt3,0–9,5s pro World in vier
  festen Roomstufen; nur konstanter Hörgain, drei0,5-s-Vergleichspausen und
  40-ms-Schnittrandfades.32s echte Engine pro Variante, Stop16s, alle Quellen
  natürlich beendet; vollständige Noten-/Besitzertraces zwischen Raumstufen
  identisch, PCM/Traces64/512 bytegleich. Vier Float-Impulse, gemessene
  Broadband-T30≈1,21/1,80/4,04s für.24/.50/1; Nominalfeedback getrennt.
  Mono und bestehende Low-Sustain/Dry-revival/Clear-Tests bestehen. Init ist
  statisch/ohne fallible Allokation; keine künstlich bestandene Init-Fail-Prüfung.
  Der kleine tatsächliche Raumanteil braucht gezielte Kalibrierung und Hörwahl.
  Noch kein Haken und kein neuer ARM-Build für dieses reine Tool-/Dokumentpaket;
  identische Firmwareinputs zum vollständig grünen PR151 sind nachgewiesen.
  **Kalibrierung2026-10-11:** ausschließlich Wet-Rücklauf auf5*r geändert;
  normales Amount/Enable-Smoothing und Null/Dry bleiben, kein Feedbackboost.
  Drei neue27,5-s-Vergleiche bei Room0,5: alter/neuer gehaltener Ausschnitt
  3,0–9,5s, danach alter/neuer Stop-Ausklang16,0–22,5s. Festgain je Fassung
  aus der gehaltenen Passage, derselbe Gain auf ihren Ausklang; keine AGC.
  Vollständige32-s-Noten-/Besitzertraces alt/neu identisch, kein künstliches
  Quellenende. Alle zwölf aktuellen Room-PCM/Traces64/512 bytegleich; vier
  Float-Impulse, reale Low-Sustain/Dry-revival/Clear/Mute/Worldwechsel und
  lange Maximalfälle bestehen. Vollständige Host-Suite und H743 mit exakt
  gleicherGCC13.2.1/Newlib4.4-Toolchain geprüft: Flash195928B(−8), D165664B/
  D260808B unverändert, maximaler einzelner Core-Compilerframe192B unverändert.
  Spec0.8 dokumentiert stärkeren Rücklauf bei gleicher SCN7-Roomposition;
  finale Hör-/Geräteabnahme bleibt offen, 24/30-Zählung unverändert.
- [x] **SD20 — Dry/Send/Room und Natur voneinander entkoppeln.**
  Alle Source-Sends, Wet-Amp/alte Reverbadapter, Atmosphere/Space und Tone-Pushes
  inventarisieren; einen eindeutigen Parameterbesitzer festlegen.
  **Abnahme:** Raumändern aktiviert kein Wind; Worldload überschreibt keine
  fremden Userwerte in undefinierter Reihenfolge. Drykörper und Master wirken
  über sämtliche Pfade. Livechanges zipperfrei und ohne tail reset. Voraussetzung: SD19.
- [x] **SD21 — Echo als räumliche Antwort prüfen oder entfernen.**
  Echolevel, Feedback, Delay Seconds, Ping-Pong, Zeitänderung und Nachklang prüfen.
  **Abnahme:** Kein rhythmischer Maschinenpuls, unharmonischer alter Ton oder
  zunehmende Rückkopplung; Zeitänderung erzeugt keine Sirene. Sustain und Stille
  bei maximal erlaubten Werten prüfen. Nur beibehalten, wenn Raum allein diesen
  Nutzen nicht erfüllt. Voraussetzung: SD12/SD19–SD20.
- [x] **SD22 — Chorus/Detune gegen Quellenbewegung abwägen.**
  Breite/Modulation, vorhandene Bowed-Verstimmung und Mono-Summe vergleichen.
  **Abnahme:** Direkter Grundton bleibt, kein hohler Swirl oder hörbarer Vibrato-
  Ersatz. Frühere Gainkorrektur nicht als vollständige Klangabnahme werten.
  Wenn kein unabhängiger Nutzen: Produktweg entfernen. Voraussetzung: SD03/SD19.
- [x] **SD23 — Age auf sinnvolle Klangalterung reduzieren.**
  Wow, Flutter, Bandverlust und Sättigung separat sowie gemeinsam prüfen.
  50-Hz-Hum/Bandrauschen sind im aktiven Master bereits entfernt.
  **Abnahme:** Tonzentrum und Dynamik bleiben ruhig, kein Eiern/Piepen/Fizz;
  Idle bleibt still. Redundanz zu Color/Motion/Blur führt zu Merge/Remove.
  Voraussetzung: SD19; keine neue Eigenrauschquelle.
- [x] **SD24 — Blur auf Grundton und musikalischen Nutzen prüfen.**
  Grainüberlappung, langes Eingangssignal, Artikulation, Quellewechsel und
  maximaler Amount; Output-/Send-/Tailerinnerung erfassen.
  **Abnahme:** Kein ausgedünnter Ton, fluktuierendes Loch oder homogenes Nebelrauschen;
  Wortlaut des Nutzens muss sich gegen gemeinsamen Raum abgrenzen lassen.
  Andernfalls entfernen. Voraussetzung: SD12/SD19.
- [x] **SD25 — Shimmer behalten oder vollständig aus Produkt entfernen.**
  Oktavregeneration und Feedback im zugelassenen Register, inklusive alter
  harmonischer Farben, hoher Spektralanteile und Worst-case-Tail.
  **Abnahme:** Kein Glasalarm oder schwebender Fremdton; Tail zählt harmonisch.
  Kein Nutzen ohne hohe Aufmerksamkeit: entfernen, nicht nur Default=0.
  Voraussetzung: SD12/SD19; keine neue Sample-/FFT-Engine als Ersatz.
- [x] **SD26 — Reverse Swell strikt auf Produktnutzen begrenzen.**
  Aktiver synthetischer Vorlauf und Offline-Reverse unterscheiden. Automatische
  Generate-Vorswells bleiben aus; Admission kann ein geplantes Event absagen.
  **Abnahme:** Kein Vorlauf zu einer später abgewiesenen Note, keine Stille-
  Verletzung, keine look-ahead-Latenz im manuellen Spielen. Wenn nicht klar
  erforderlich, aus Produktbefehlen entfernen. Voraussetzung: SD07/SD13/SD19.
- [x] **SD27 — Finale Effektpalette reduzieren und Moduswechsel prüfen.**
  Alle neun Modi auf SD19–SD26 abbilden. Dream Chain ist kein Qualitätsargument;
  benötigte Kombinationen explizit wählen. Modewechsel inklusive Bypass/Fallback
  mit vorhandenen Tails prüfen.
  **Abnahme:** Kein doppelt bearbeiteter Bus, Pegeleinbruch, Tailrevival oder
  ungelöster Effektparameter. Entfernte Modi nicht durch Scene/Worldload
  reaktivierbar. Zero-Amount-/Bypass-CPU und Speicher anhand Code/Map statt
  Annahme bewerten. Voraussetzung: SD00/SD19–SD26, Migration SD36.

### 4. Natur und Geräusche mit Entscheidung zum Weglassen

- [ ] **SD28 — Naturebene rechtfertigen und separat steuern.**
  Für jede World entscheiden: ganz ohne Natur oder eine leise ortgebende Ebene.
  **Abnahme:** Die trockene musikalische Welt ist vorher eigenständig; Nature
  erhöht Ortsgefühl im Vergleich, ohne Töne/Pausen zu maskieren. Kein Wind-
  Default für alle Welten und keine Kopplung an Room. Voraussetzung: SD14–SD16/SD20.
  **Vorbereitung 2026-10-11:** tatsächliche Engine mit Room0,5, gleichen normalen
  Makros und Nature0/0,35; gehaltene Passage6–12,5s sowie echter späterer Verlauf.
  Alle vier Abschnitte pro Datei mit demselben festen Gain, Pause nicht angehoben.
  COAST: leiser Übergang51,69–58,19s ohne lange Quellenpause; WOODLAND:
  40,32–46,82s mit4,49s echten freien Quellen; HIGHLANDS: Pause29,38–35,88s,
  ohne Nature dort PCM exakt Null. Quelle/Phrasenform wird nicht für Natur geändert.
  18 vollständige112-s-Fälle (zwei Seeds×drei Worlds×Nature0/.35/1), jeweils64/512
  bytegleiche PCM/Noten-/Statusdaten; Nature verändert keinen tatsächlichen Score,
  reale Belegung oder Phrasenzähler. Stop80s, spätestens88,84s alle Quellen frei,
  letzte Sekunde112s exakt still, NaN/Limiter0. Raw True Peak≤−20dBFS, Mono≥0,93079.
  Nature-only durch echte Engine mit Room0/1 bytegleich, keine Phantomstimme.
  Firmwareinputs unverändert zum vollständig grünen PR153 einschließlich H743;
  dies ist ein Tool-/Hörpaket, kein neuer ARM-Build oder Haken. Gemeinsamer Wind,
  Wellen/Tropfen und finale Entscheidung zum Weglassen bleiben ungehört.
- [ ] **SD29 — Wind nach vorhandenem Irregularitätsumbau hören.**
  Bestehende unabhängige Weather-/Eddy-Zufallsuhren und variable Gust-Zeiten
  erhalten; mehrere Zustände/Seeds getrennt auswählen. Spektrale Homogenität,
  Filterpeaks, Rumpeln und Lautstärkeverlauf prüfen.
  **Abnahme:** Kein regelmäßiges Schwingen, röhrenartiges Pfeifen oder dauerhaft
  gleiches Rauschband. Mehr Zufall alleine ist keine Abnahme. Bei unnötiger
  Präsenz wegnehmen; nur falls SD28 Wind behält.
- [ ] **SD30 — Regen und Wellen auf Ereignisse statt Looptrim prüfen.**
  Nur gewählte Naturquellen: Nah-/Fernanteil, Transienten, Schaum/Prasseln,
  spektraler Schwerpunkt und unabhängige längere Entwicklung.
  **Abnahme:** Kein nervöses Ticken, immer gleicher Schwall, pegelstarker Sweep
  oder verdecktes Sea-Hum. Mono/kleine Ausgabe prüfen; keine WAV-Samplebank.
  Voraussetzung: SD28, sonst Pfad entfernen.
- [ ] **SD31 — Alle übrigen Geräuschwege schließen.**
  Brown-Rumble, Breath, Warm/Air, Fjord/Haze/Desert, Crackle/Hiss/Sea-Hum sowie
  bisherige Texturen: Erreichbarkeit/Idle/Eintrittspunkte und Zweck dokumentieren.
  **Abnahme:** Jeder verbleibende Weg hat eine Orts- oder Artikulationsrolle
  und besteht ohne elektrischem Brumm-/Summcharakter, dominanter Bandresonanz,
  Maskierung oder nervöser Gleichförmigkeit. Sonst entfernen; keine neue Nature
  Library anlegen. Voraussetzung: SD00/SD28.
  **Softwareabschluss 2026-10-18:** Jeder entfernte und verbleibende Weg ist mit
  Eintritt/Besitz/Rolle/Idle/Ende zugeordnet. Das echte aktuelle PR176-ARM-Artefakt
  bestätigt den Ausschluss, nicht bloß Amount0:73 Firmware-LOAD-Objekte und belegte
  Funktionen geprüft, ZIP-Digest/CRC und Head stimmen.263 Firmwareinputs bytegleich;
  aktueller vollständiger Host-/H743-Stand grün, vorhandene Nature-Handoffbelege
  quellenidentisch. Kein weiterer Softwarebefund und kein neuer DSP-/Testumbau.
  SD31 zählt zur Hörgruppe; die gehörte Orts-/Artikulationsrolle, Bandresonanz
  und Maskierung bleiben offen. Kein Hauptpunkt-Haken aus dem Linkaudit.
  Details:[SD31_NOISE_PATHS_CHECKPOINT.md](SD31_NOISE_PATHS_CHECKPOINT.md).

### 5. Musikalische Einflussbereiche, Pegel und Zustandswechsel

- [x] **SD32 — Vollständiges versioniertes Parameterregister schreiben.**
  Alle Familien aus Tabelle D mit den Pflichtfeldern erfassen, jeden öffentlichen
  Setter/Presetpush/native Slot zuordnen. Defaults nicht zwischen Quelle, Scene
  und World durch Reihenfolge entstehen lassen.
  **Abnahme:** Kein zugänglicher Soundwert ohne Bereich, Besitzer und definierte
  Livewirkung; ungültige Floats halten letzten gültigen Wert. Entfernte Parameter
  sind nicht scheinbar aktive Regler. Voraussetzung: Quellen-/FX-Auswahl.
- [ ] **SD33 — Activity/Color/Room als musikalische Bereiche konstruieren.**
  Je Welt Kennlinie und Extremverhalten; Activity variiert Figur/Übergabe und
  Ruhe, Color bleibt im akzeptierten Tonkörper, Room verändert Entfernung.
  **Abnahme:** Kein Bereich wird Alarm, sinnlos dicht oder leer durch Fehler;
  mittlere/default/äußerste Werte haben klaren musikalischen Nutzen. Lautstärke
  bleibt separat. Physische Zuweisung offen. Voraussetzung: SD17/SD20/SD32.
  **Activity-Vorbereitung 2026-10-11:** reale lange COAST/WOODLAND/HIGHLANDS,
  Werte0/0,5/1, Seeds1234/42;18 vollständige360-s-Fälle plus drei Live-Wechsel-
  Läufe, jeder64/512 bytegleich auf10-ms-Main-Raster.42 Performances/252min PCM.
  576 Pure-Cases über acht Phrasen/33792 Intents prüfen unveränderte Tonhöhe,
  Rollen/Velocity/Halteabsichten, Memory und Timerwrap;1536 Pausenabsichten≥8s.
  Reale Tonfolge und Haltezeiten stimmen im gemeinsamen gehörten Präfix der
  festen Werte überein; bei Live-Wechseln41/36/28 abgeschlossene Rollenhaltezeiten
  exakt unverändert. Besitzer3/2/3, vier Phrasenarten und≥1 erinnerte Rückkehr,
  NaN/Limiter0; Stop328s, spätestens336,84s alle Besitzer frei, letzte Sekunde still.
  Drei38,5-s-Vergleiche zeigen0/0,5/1; ein Gain je World, echte HIGHLANDS-Ruhe.
  **Befund der Spec0.8-Vorbereitung:** COAST47 Einsätze/fünf Phrasen bei allen Werten und beiden Seeds,
  Rückkehrzeiten fast gleich. Nominelle Abstände werden von realen Quellen/Tails
  begrenzt. Dieser COAST-Softwarebefund ist jetzt mit Spec0.9 bearbeitet (unten).
  WOODLAND33/39/41 und32/39/41; HIGHLANDS32/37/43, längste Quellenpause bei
  Seed1234 von18,83 auf9,16s. Musikalischer Nutzen/Color/Room bleiben ungehört.
  Alle Firmwareinputs unverändert zum grünen PR154/PR153; gezielte Hostprüfung,
  kein behaupteter neuer vollständiger Host-Runner oder ARM-Build, kein SD33-Haken.
  **COAST-Kalibrierung Spec0.9, 2026-10-11:** zusätzliche Partnerwait6/3/0s
  und Schlusswait10/5,5/1s nach tatsächlichem Besitzerende, mit Entwicklungsfaktor.
  Gemeinsamer Ton bleibt stehen; laufende Waits/Holds und Memory/RNG bleiben.
  12 feste600-s-Fälle plus zwei Livefälle, je64/512 bytegleich; vier112-s-
  Gegenproben je Baseline/Kandidat und Blockgröße belegen unverändertes WOODLAND
  und HIGHLANDS.44 Render/309,87min; echte Waits, drei Quellen, NaN/Limiter0,
  Stop568s und final still. Seeds1234/42 beide56/64/77 statt79/79/79 Starts,
  Rückkehr248,66/215,13/181,58s beziehungsweise249,66/215,89/182,11s.
  Eine38,5-s-WAV, festes Gain+9,9dB,0/0,5/1. Volle Host-Suite und echte
  Baseline-/Kandidat-H743-Releasebuilds mit identischer verifizierter Toolchain;
  Baseline-ELF exakt reproduziert, Flash+456B, gemessene D1/D2-Gesamtbelegung
  gleich, COAST-Zustand+4B. Spec0.9 ändert zeitliche Bedeutung desselben SCN7-
  Activitywerts; Schema/Default bleiben. Historische Hörpacks bleiben PR155,
  aktuelle COAST-Prüfung separat. Hörnutzen/Color/Room weiter offen; kein Haken.
  Details: [SD33_COAST_ACTIVITY_CHECKPOINT.md](SD33_COAST_ACTIVITY_CHECKPOINT.md).
  **Color-Vorbereitung 2026-10-11:** drei echte musikalische Worlds,
  Color0/0,5/1, zwei Seeds;18 feste112-s-Fälle und drei Livefälle bei64/512,
  sechs vollständige normale Defaultkontrollen.48 Render/89,6min PCM.
  Vollständige PCMbytes blockgleich; alle Color- und Liveereignisse/Statusdaten
  identisch, Noten/Holds/Hz/Owner/Phrasen/Übergaben unverändert. NaN/Limiter0,
  Quellen3/2/3, Stop80s, spätestens88,84s Besitzer frei, finale Sekunde still.
  Zusätzlicher echter Product-Vertrag inkl. Invalid-Inputs und zwölf Minuten
  Musik besteht.251 getrackte Core-/Header-/Startup-/Linker-/CMake-Inputs exakt
  PR156, dessen CI38145968039/run687 jetzt frisch completed/success.
  Spec0.9/Firmware unverändert, kein neuer voller Hostrunner/ARM-Build behauptet.
  Drei38,5-s-WAVs vergleichen dieselbe Passage3–15,5s, ein Gain je World.
  COAST Color0/1 RMS-Differenz14,44/10,63%, HIGHLANDS11,28/8,50% zum Default;
  WOODLAND nur0,203/0,204%, FFT-Center181,08/181,27/181,46Hz (beide Seeds
  gleiche schwache monotone Richtung). Sample-/FFTbefund ist keine Hörabnahme.
  Dieser Softwarebefund ist mit Spec0.10 bearbeitet (unten). Kein SD33-Haken.
  Details: [SD33_COLOR_CHECKPOINT.md](SD33_COLOR_CHECKPOINT.md).
  **WOODLAND-Color-Kalibrierung Spec0.10,2026-10-12:** nur Ausgangsfarbe der
  langen Product-Saite; fester Defaultloop, Neutral0,5 exakt bisheriger Klang.
  Obertöne statt verkürzter Schwingung,80-ms-Livesmoother; Clear erhält Zielwert.
  Beide Seeds zeigen175,67/181,27/188,86Hz beziehungsweise175,24/180,82/188,37Hz;
  relative Min/Max-Spanne7,278/7,263% statt0,208/0,207%. Zwei kurze WAVs,
  echte Pegelunterschiede erhalten (−26,4/−25,3/−23,0LUFS), kein AGC.
  42 Kandidaten- plus21 Baseline-Render:117,6min PCM. Alle vollständigen
  Noten/Holds/Hz/Owner/Phrasen/Retirements/Zulassungen gegen Baseline identisch;
  64/512 PCM/Status/Events exakt, WOODLAND Default und andere Worlds bytegleich.
  24 Float-Solos: Pitchfehler≤0,03369Cent, Fundamentaldecayvergleich≤0,00151%
  Abweichung, alle acht Neutralaufnahmen exakt. Ungesteuerte110-Hz-Saite wird
  bei allen drei Werten im selben512-Block bei37,849s inaktiv; kurze Quelle
  unverändert. NaN/Inf/Clamp, Sourcebudgets, Stop/Clear, letzte Sekunde still.
  Voller Hostrunner und Baseline/Kandidat H743 mit identischer geprüfter Toolchain
  bestehen; BaselineELF exakt reproduziert, Flash+236B,D1+32B,
  D2+0B,DTCM+0B. Kein CPU-/ISR-Stack-/Hörnachweis.
  SCN7/Default0,5/Reference-Default/Journal-OFF erhalten; WOODLAND-Endwert-
  Klangsemantik bewusst neu.248/251 Coreinputs unverändert, nur drei geändert.
  Details: [SD33_WOODLAND_COLOR_CHECKPOINT.md](SD33_WOODLAND_COLOR_CHECKPOINT.md).
  Die Attack- und getrennte Release-Vorbereitung sind jetzt geprüft (unten).
  **Attack-Vorbereitung2026-10-12:**18 feste112-s-Fälle plus drei Livefälle,
  jeweils64/512, sechs komplette Defaultkontrollen:48 Render/89,6min PCM.
  Alle vollständigen Noten/Holds/Hz/Owner/Phrasen/Memory/Zulassung/Retirements
  und Übergaben über Attackwerte/Live identisch; alle PCM/Events/Status blockgleich.
  Sourcefaktor0,5358867/1/1,8660657 ergibt COAST0,161/0,300/0,560s,
  WOODLAND0,429/0,800/1,493s und HIGHLANDS0,241/0,450/0,840s nominale Attack.
  Tatsächlich normierte frühe Energie nimmt bei beiden Seeds in jeder World ab.
  Zwölf bereits vorbereitete/held Änderungen bleiben im echten viersekündigen
  PCM exakt, neun zukünftige Starts64/512, sechs Clampfälle/Invalidinputs bestehen.
  Release bleibt getrennt0,5/Faktor1. Setter keine Sofortnote; Quellen3/2/3,
  NaN/Limiter0, Raw−20,4dBTP, Mono≥0,93175, Stop80s, letzte Sekunde still.
  Drei38,5-s-WAVs: gleicher tatsächlicher Anfang0–12,5s, kein Anfangsfade,
  nur40-ms-Endschnitt und0,5-s-Vergleichslücken; ein Gain je World, keine AGC.
  Zusätzlich echter Product-Vertragscheck mit12min Musik besteht.251 Coreinputs
  exakt PR158, CI38284802990/run691 frisch completed/success; Spec0.10 gleich.
  Kein neuer voller Host-/ARM-Nachweis behauptet, kein Geräte-/Hörhaken.
  Details: [SD33_ATTACK_CHECKPOINT.md](SD33_ATTACK_CHECKPOINT.md).
  **Release-Vorbereitung2026-10-12:**18 feste112-s-Fälle plus drei Livefälle
  jeweils64/512, sechs volle Defaultkontrollen und neun32-s-Stop-Gegenproben
  jeweils64/512:66 vollständige Render/99,2min PCM. Ganze PCM/Events/Status
  blockgenau; Defaultaudio und gemeinsame Statusfelder zum normalen Renderer exakt.
  Gleiche Stop-Gegenproben-Noten/Hz/Velocity/Owner/Holds, streng steigende
  tatsächliche Quellenbelegung und Stereoenergie nach Stop bei0/0,5/1.
  Stop bis alle Quellen frei:COAST5,82/8,82/13,37s,
  WOODLAND1,32/2,00/3,04s,HIGHLANDS4,76/7,22/10,91s,10-ms-Auflösung.
  Releasefaktor0,65975392/1/1,51571643; Attack bleibt0,5/Faktor1.
  250 bestätigte Starts mit jeweils frischer vorangehender physischer Reservierung;
  wiederverwendeter Owner vorher tatsächlich frei, nicht nur NoteOff gemeldet.
  Autonome Übergabezeit und Keeper-Holds dürfen sich mit Release ändern;
  gemeinsame geordnete Note-on-Folgen bei beiden Seeds erhalten. Kein Steal.
  18 vorbereitete/held/releasing Änderungen bleiben volle sechs Sekunden PCM
  exakt; neun zukünftige Starts64/512, sechs Clamp-/Invalidinputverträge bestehen.
  Setter keine Sofortnote; Quellen3/2/3,NaN/Limiter0,Raw≤−20,4dBTP,
  Mono≥0,93058,Stop80s,alle Besitzer bis93,20s frei,letzte Sekunde exakt still.
  Drei38,5-s-WAVs:gleicher tatsächlicher Abschnitt2–14,5s,Stop nach4s,
  40-ms-Ausschnittkanten,0,5-s-Vergleichslücken,ein Gain je World,keine AGC.
  Lange WOODLAND-Quelle/Color erhalten; kontrollierter kurzer Stop ersetzt
  keine lange Phrase. Zusätzlich Product-Vertragscheck mit12min Musik besteht.
  251 Coreinputs exakt PR159,CI38286641272/run693 frisch completed/success;
  kein neuer ganzer lokaler Host-/ARM-Nachweis oder Geräte-/Hörhaken behauptet.
  Details:[SD33_RELEASE_CHECKPOINT.md](SD33_RELEASE_CHECKPOINT.md).
  Die anschließende SD34-Softwareprüfung ist abgeschlossen (unten).
  SD33 bleibt für Hören/Nutzen offen.
  **Softwareabschluss2026-10-19:** Aktuelle Kennlinien/Setter-/Quellenverträge und
  alle vorhandenen Einzel-/Kombinationsbelege konsolidiert. Die zwei früheren
  COAST-Activity-/WOODLAND-Colorbefunde sind bearbeitet; kein weiterer technischer
  Rest festgestellt. Zehn Corequellen und sieben komplette öffentliche Setter
  bytegleich zum Release-Checkpoint; Engine/Room später geändert und separat
  über aktuelle Übergabe-/Clearbelege gedeckt. Keine neuen PCM-Gleichheiten zu
  historischen Captures behauptet.263 aktuelle Inputs exaktPR177, Runtime wie
  grüner PR176-Host/H743-Stand. Kein neuer DSP-/Testumbau oder voller Renderlauf.
  SD33 bleibt ungecheckt und zählt zur Hörgruppe; physische Zuordnung SD52.
  Details:[SD33_CONTROL_CONSOLIDATION_CHECKPOINT.md](SD33_CONTROL_CONSOLIDATION_CHECKPOINT.md).
- [ ] **SD34 — Quellen-/Rollenpegel fest kalibrieren.**
  Bestehende Pluck/Bowed/Horn/Choir/Guembri-Verstärkungsfaktoren und Min/Max-Clamps
  gegen tatsächliches PCM prüfen. Register, Artikulation, Body, Send und Rollen
  gemeinsam berücksichtigen; Hookamplitude von DSP-Gain unterscheiden.
  **Abnahme:** Wechsel/Lagen überraschen nicht durch Lautheit; kleiner Velocity-
  Unterschied bleibt kontrolliert. Feste Gainkurve statt nachträglicher AGC.
  Defaultwelt und extremste erlaubte Summe haben dokumentierte Headroom.
  Voraussetzung: SD18–SD20/SD32.
  **Softwareprüfung2026-10-13 abgeschlossen:**105 echte Solo-PCM-Fälle,
  sämtliche20 Manualtonhöhen MIDI50..69 bei Velocity0,5 und drei Lagen bei
  0,01/0,25/0,5/0,55/0,75/1. Körper1,5–2,5s und spätes PCM8–9s getrennt.
  Registerspanne COAST0,027dB,WOODLAND3,149dB,HIGHLANDS0,045dB; gleiche Tonhöhe
  maximal2,329dB Quellenunterschied. Kleiner Velocityschritt0,50→0,55:
  0,828–0,831dB,alle geprüften Werte streng monoton. Keine Decay-/Gainkompensation.
  162 Maximalsummen:26 zulässige geordnete Dreifamilienmischungen in zwei Lagen
  und drei Colors plus Zwei-Saitenfälle; Velocity/Room/Nature/Master1.
  Worldwechsel erhalten Manualquellen; zusätzliche zulässige Note abgewiesen,
  global3/lokal2 ohne Steal. Stärkster TruePeak−5,1dBFS bei Master1.
  Stärkste Summe und drei reine Familienmaxima bei Referenzmaster0,6 wiederholt:
  mindestens9,5dB Reserve,≥6dB-SD42-Ziel erfüllt. Ganze Events/Status über Masterwerte gleich.
  24 echte112-s-Genfälle einschließlich äußerer Register/Major/Minor/Seeds/Maxpegel;
  bestätigte Rollen/Frequenzen/Velocities, sechs ganze Defaultkontrollen exakt.
  Zusammen596 Render/209,2min,ganze64/512 PCM/Events/Status exakt,NaN/Limiter0,
  Mono≥0,79865,DC≤9,54e−7,stille letzte Sekunden. Kein vollständiger Parametergitter-Sweep.
  Drei Ceiling-,sechs Legacy-,sechs Manual/Worldverträge erhalten4s-PCM exakt;
  15 Invalidinputs still abgewiesen, zusätzlicher12-min-Productvertrag besteht.
  Sourceinput0,50/0,22/0,50 undSend0,35 bleiben; Hookvelocity ist kein DSP-Peak.
  Choir/Guembri im Product ausgeschlossen. Alle251 Coreinputs exakt PR160,
  CI38305233488/run695 frisch completed/success;Spec0.10/SCN7/Defaults gleich.
  Ein40-s-WAV:echte verschiedene GenanfängeCOAST/WOODLAND/HIGHLANDS,ein Gain,
  keine AGC. Lange WOODLAND-Quelle/Color erhalten. Finale Lautheitsbalance hören;
  SD34 bleibt unchecked und wurde beim Pegelcheckpoint zur Hörgruppe zugeordnet.
  Details:[SD34_GAIN_CHECKPOINT.md](SD34_GAIN_CHECKPOINT.md).
  SD35-Softwareprüfung ist anschließend abgeschlossen (unten).
- [ ] **SD35 — Kritische Parameterkombinationen prüfen.**
  Erreichbare Spec0.10-Regler: Activity×Attack×Release,
  Color×Room×Nature×Volume, Dry/Room und erlaubte Register-/Pitch-Kontexte.
  Excitation/Damp, Resonance/Drive, Width/Chorus/Detune, Age/Blur und Shimmer
  sind entfernt; Feedback folgt Room und ist kein freier Regler.
  **Softwareprüfung2026-10-13 abgeschlossen:**48 feste192-s-Generatefälle
  (acht Profile, drei Worlds, D-Major/Equal1234 und A-Minor/Just42), drei
  Live-Generatefälle,20 manuelle112-s-Maxbelegungen in fünf Familienmischungen
  und zwei Lagen plus sechs manuelle Livefälle. Je64/512 plus drei bestehende
  Defaultkontrollen:157 ganze Render/429,067min. Ganze PCM/Events/Status/Controls
  exakt;1049 Starts an frischen wirklichen Quellenreservierungen, Wiederbelegung
  erst nach Retirement. Alle Quellen/letzte PCM-Sekunden still; kein Clear/Mute.
  Global3/lokal2, weitere Eingabe abgewiesen, tatsächliche Hz/Core/Tuning geprüft.
  Manuelle WOODLAND-Enden35,68–45,04s nach Key-up; Generate0–3,04s nach Stop.
  18 simultane Activity/Attack/Release-Edits vorbereitet/held/releasing erhalten6s
  PCM exakt. Sechs Endpoint-/sechs Clamp-/sechs Invalid-/retired-API-Fälle bestehen.
  Neun Livefälle mit100 Zieländerungen/s,0,5-/1-s-Rampen und Volume0/1;
  2469 Controlfenster. Zwölf gepaarte volle tiefe/hohe Akkordgegenproben maximal
  3/32768 zusätzliche Erstsampleänderung, Hz/Slots/Hooks erhalten.
  Natürlicher Samplegradient0,106781 in unveränderter heller hoher COAST-Summe
  wird protokolliert; kein automatisches Knackurteil aus einem Wellenformgefälle.
  Diese Auswahl maximal−6,8dBTP/Master1, Mono≥0,79576, DC≤1,694e−8,
  NaN/Limiter0. Allgemeines SD34-Maximum/Referenz−5,1/−9,5dBTP bleibt gültig.
  Alle251 Coreinputs exakt grüner PR161; Spec0.10/SCN7/Gains unverändert.
  Ein40-s-WAV: je World Referenz dann dicht/lang/hell/Room/Nature/Volume1,
  gemeinsamer Gain+1,9dB; tatsächliche unterschiedliche Pegel/Notenabläufe.
  **Abnahme offen:** Musikalische Endwertwahl, störende Bandpeaks/Buzz/Pumpen
  und wahrgenommene Gainlöcher hören. Messwerte sind keine Klangfreigabe;
  SD35 bleibt unchecked und zählt jetzt zur Hörgruppe. Kein vollständiger
  Parameter-/Pitch-Sweep oder Gerätebeleg. Voraussetzung: SD32–SD34.
  Details:[SD35_PARAMETER_CHECKPOINT.md](SD35_PARAMETER_CHECKPOINT.md).
  SD37-Softwareprüfung ist anschließend abgeschlossen (unten).
- [x] **SD36 — Katalog und Scenes ausdrücklich migrieren.**
  Fünf bestehende Worlds/sechs V2-Familien und SCN5/SCN6 kennen; neue World-IDs
  versionieren, Ersatzzuordnung und verworfene Parameter explizit dokumentieren.
  **Abnahme:** Alte ID bedeutet nicht unbemerkt etwas anderes; Recall konsistent,
  entfernte Layer/FX nicht reaktivierbar, Speicherformatbudget eingehalten.
  Migrationsfixtures mit echten bisherigen Werten. Voraussetzung: SD00/SD13/SD27/SD32.
- [ ] **SD37 — Sämtliche Worldwechsel musikalisch schließen.**
  Bei drei Welten sechs gerichtete Wechsel, einschließlich voller Releases,
  maximalem Raum, schneller mehrfacher Zieländerung und Pending-Ereignis.
  **Aktuelle Softwareprüfung2026-10-13 abgeschlossen:**54 Spec0.10-Szenarien,
  je64/512,108 ganze WAV-Render/220,8min. Je Richtung volle Generatebelegung
  in D-Major/Equal1234 und A-Minor/Just42;Pending0s,gehörte Belegung2,7s,
  Release/Stop2,7s→World2,71s→GenerateOn3,3s,Mehrfachziele ohne Audio,
  zehn Ziele mit10-ms-Audio,natürliche manuelle Key-ups4s,Nature1 undTimerwrap.
  Erzeugte Quellen wirklich nach100ms frei;Pending nach höchstens einem10-ms-Tick
  storniert. Neue Zielnote normal260ms nach letzter Wahl bestätigt; nur jüngste
  Familie/Score,keine alten Wiederstarts.670 Starts an678 frischen Reservierungen,
  acht alte unbegonnene Starts ohne Hook/PCM/History; global3/lokal2 ohne Steal.
  Manuelle Quellen bleiben überWorldwahl bei originalen Hz/Familien; ursprünglicher
  Key-up endet sie natürlich (WOODLAND42,91s nach Key-up). Freier dritter Slot
  aus alterWOODLAND-Belegung zulässig,volle Pools weisen weitere Eingabe ab.
  Ganze PCM/Events/physische Besitzer-/Zielstates64/512 exakt. Batch/direct und
  Wrap/Clock0 ganz exakt; Nature erhält ganzen Score/Besitz.27 gepaarte4-s-
  Fälle erhalten ManualHeld/Release,Source-free-Room,LatestNature undSameWorld.
  Room bleibt positiv beim Wechsel,alte Hz-history erhalten;kein Clear/Mute.
  Letzte Sekunden/alleSources0,NaN/Limiter0,Mono≥0,88248,DC≤3,355e−8.
  Last-block-Roompeaks sind blockabhängige Beobachtungsfenster;Quiet-History-
  Diagnose unterscheidet sich78mal um einen10-ms-Tick ausschließlich bei
  leeren Quellen. Audio/Ereignisse/Besitz/Ziel bleiben exakt;SD12-Hörgate offen.
  Alle251 Coreinputs exakt grünerPR162. Keine Firmwareänderung; altePluck20-ms-
  Specstelle ist auf bestehenden100-ms-Kontextstop berichtigt.50,5-s-WAV
  zeigt sechs echte Wechsel mit einemGain+4dB,ohne Überblendung am Worldauftrag.
  Noch keine musikalische/Knack-/Nature-/Geräteabnahme;SD37 bleibtunchecked und
  zählt jetzt zurHörgruppe. Keine vollständige Timing-/Key-/ISRbehauptung.
  Details:[SD37_WORLD_TRANSITION_CHECKPOINT.md](SD37_WORLD_TRANSITION_CHECKPOINT.md).
  Aktueller nächster SD41-Teil steht im SD41-Eintrag; keine Hörfreigabe daraus.
  **Abnahme:** Nur jüngstes Ziel geplant; bestehende Besitzer weich beendet,
  reale Belegung und harmonische Fahnen respektiert; kein Body-Resetknacks,
  frozen noise, Hallreset oder wiederkehrender alter Ton. Voraussetzung: SD12/SD27/SD36.
- [x] **SD38 — Autonomen Betrieb als Audiobefehl fertig definieren.**
  Start, Stop, erneuter Start, Einstieg aus dichtem Manual und fehlende zulässige
  Note; keine Taste/UI hier festschreiben. Bereits vorhandenes busy-opening
  pending/retry ohne Pitch-RNG-Verbrauch beibehalten.
  **Abnahme:** Start schafft zum frühestmöglichen musikalisch zulässigen Zeitpunkt
  einen Ton, Stop verhindert neue Pläne, Wiederstart dupliziert keine Besitzer;
  Ruhe bei Ablehnung ist begrenzt diagnostizierbar. Voraussetzung: SD09/SD13/SD37.
- [x] **SD39 — Stillwerden/Clear/Mute für die gesamte Kette bauen.**
  Quellen, Releases, Natur, Dry/Send, sämtliche Delay-/Raumtöne; musikalischen
  Stop von vollständigem Stillwerden trennen.
  **Abnahme:** Stillwerden endet weich innerhalb einer festgelegten, dokumentierten
  Zeit unabhängig von Sustain/Feedback; Mute wirkt sofort nach dem Raum ohne
  Click, kein Tail bei Unmute wiederbelebt. Audio-DMA läuft weiter; keine
  Audio-Unterbrechung als Workaround. Voraussetzung: SD27/SD31/SD38.
- [x] **SD40 — Live-Key/Mode/Tuning und Besitzerwechsel prüfen.**
  Retune während gehaltenem Ton, Release, maximaler Tail und manueller Eingabe;
  festlegen, ob neue Noten umgestellt oder bestehende Stimmen weich übergeben
  werden. Kein unfreiwilliges Portamento/Sirenenretune.
  **Abnahme:** Keine falsche Tonhistorie, Pitchclamps, stuck Note oder Release an
  falsche Quelle; gemeinsame Töne oder Ruhe verbinden. Voraussetzung: SD08/SD11–SD12/SD37.
- [ ] **SD41 — Boot/Recall/Seed ohne ungewollte Ausgabe schließen.**
  Boot still, Scene-Lautstärke/Worldtargets und Seedstart definiert; vorhandene
  held/generative Besitzer nicht blind aus Persistenz wiederherstellen.
  **Aktueller Softwarebeleg2026-10-13:** Sechs zuvor laute Bootzustände,380
  gerichtete Native-Fallbacks,zehn SCN5/6-Recalls und20 native Off-Recalls
  einschließlich aller fünf Slotindizes und Volume/Mute. Keine restaurierten
  Besitzer/Generate; nur ausdrücklich positives Nature kann beim Recall hörbar
  werden. Zwölf Manual-/drei Savefehler-/neun Seed-Triplets bei512/64 erhalten
  ganze8s PCM/Hooks; neun Wrap-Paare exakt. Erzeugter Live-Seed-Recall100ms,
  Room/alte Hz-History erhalten, Autoplay-Gate0 bleibt.14,3933min gerichtetes
  PCM, Core251 Inputs unverändert; keine Geräte-/ECC-/Live-Save-Freigabe.
  [SD41_BOOT_RECALL_CHECKPOINT.md](SD41_BOOT_RECALL_CHECKPOINT.md).
  **Vorbereiteter Softwareteil2026-10-14:** Separater Main-Journal-Schreiber
  ohne versteckten Scan/Wait, höchstens ein I/O je Step; vollständige Rückleseprüfung,
  sichere Pufferbindung bis Ende eines abgebrochenen I/O.5252 Modellfehler,
  79 Abbruchzustände,511 Saves und drei ganze8s-PCM-Triplets bei512/64 bestehen.
  Kein Scene-/H743-Anschluss, bestehendes Live-Save bleibt synchron; Journal OFF.
  [SD41_ASYNC_JOURNAL_CHECKPOINT.md](SD41_ASYNC_JOURNAL_CHECKPOINT.md).
  **Vorbereitete Recovery2026-10-14:** Je Step maximal ein32-B-Read und32-B-CRC;
  Cache während des Scans gesperrt, kein Teilfund bei Fehler/Cancel.7288 Scanner-
  Gegenproben,4983 Cut-/Retryfälle,35 Cancelzustände und neun volle12-s-PCM-
  Triplets bei64/512 bestehen.253 bisherige Inputs unverändert. Noch kein
  Scene-/H743-Anschluss und keine Gerätezeitmessung; bestehendes Save synchron.
  [SD41_JOURNAL_RECOVERY_CHECKPOINT.md](SD41_JOURNAL_RECOVERY_CHECKPOINT.md).
  **Vorbereitete Scene-API2026-10-14:** Fester nativer Snapshot, Begin ohne I/O;
  BUSY bedeutet Speichern läuft, DONE erst nach ganzer bestätigter Rückleseprüfung.
  RAM/aktive Scene bei Fehler/Cancel unverändert; physischer Commit kann dennoch
  recoverbar sein. Neuer Versuch scannt bei unsicherem Cache und übernimmt neue
  Reglerwerte.337 native Fälle,104 Writer-/20 Recovery-I/O-Fehler und206
  Abbruchpositionen;15 volle12-s-PCM-Triplets/33 Render sind exakt bei64/512.
  Neue API nur separat mit Product/Prepared-Flag, bestehendes Save synchron und
  Journal OFF; kein UI-/H743-/CMake-/Defaultanschluss.254 bisherige Inputs exakt,
  ausgeschaltete Scene-Übersetzungseinheiten Reference/Product vollständig gleich.
  [SD41_SCENE_SAVE_CHECKPOINT.md](SD41_SCENE_SAVE_CHECKPOINT.md).
  **Vorbereiteter H743-Adapter2026-10-15:** Bank-2-Interruptstart und quieszenter
  Poll, Fehler gewinnt über EOP; begrenzte Reads und strenge präzise ECC-Fault-
  Zuordnung.70740 Checks/4480 Reads/328 Framefälle, ASan/UBSan und echter
  Cortex-M7/ST-HAL-Teillink bestehen. Alle258 bisherigen Inputs bytegleich.
  Eigenständige Komponente, keine Main-/Vektor-/UI-Aktivierung oder Siliziumabnahme.
  [SD41_H743_SCENE_IO_CHECKPOINT.md](SD41_H743_SCENE_IO_CHECKPOINT.md).
  **Stand2026-10-20:** Der isolierte Geräte-Testbuild mit Anschluss ist bereits
  implementiert, nativ/sanitized und ARM-gelinkt geprüft:
  [SD41_H743_SCENE_BENCH_CHECKPOINT.md](SD41_H743_SCENE_BENCH_CHECKPOINT.md).
  Nächste Grenze: physische ECC-/Power-cut-/IRQ-/Geräteprüfung; erst danach
  Productaktivierung. Kein neuer unabhängig umsetzbarer Softwareteil offen.
  **Abnahme:** Kein Überraschungsstart oder Lautstärkesprung, kaputte/alte Scene
  führt in dokumentierten stillen Fallback; Reset beim laufenden Raum gezielt
  testen. Keine blockierende Speicherung im Audiopfad. Voraussetzung: SD36/SD39–SD40.
- [x] **SD42 — Ganze Ausgabekette gegen harte/hohle Ergebnisse absichern.**
  Pre-/Post-FX-Gain, DC, Limiter/PCM-Konversion, Stereo/Mono, volle Belegung und
  alle Quellen/Tails einschließlich Natur; keine falsche Float-/PCMskalierung.
  **Abnahme:** Keine NaN/Inf, Clipping/Intersample-Überraschung, anhaltender DC oder
  Monoauslöschung. Startziel bei definierter Referenzlautstärke mindestens 6 dB
  digitaler True-Peak-Headroom im Worst-case-Probe; physischer SPL separat.
  Limiter fängt Ausreißer, repariert keine schlechte Gainstruktur. Voraussetzung: SD34–SD41.

### 6. Nachweis, Produktreduktion und Abschluss

- [x] **SD43 — Hör-/Renderverfahren für die ganze Kette festlegen.**
  Firmware korrekt in zulässigen Blöcken (Engine höchstens 512 Frames) rendern;
  andere zulässige Blockgrößen gegen gleiche Seed-/Eventfolge vergleichen.
  **Abnahme:** Keine stille Überschreitung oder Referenztonkontamination, SHA,
  Sample-Rate, Sourcefingerprint, Seed, Parameter, Onsettrace und feste Gainwerte
  vorhanden. Rohe Pegel plus gematchte Hörkopie; kein sampleweises Normalisieren.
  Kurze Vergleiche kompakt halten; vollständige lange Phrasen als solche
  kennzeichnen statt sie vor der musikalischen Auflösung abzuschneiden.
  Bestehende passende Harnesses weiterverwenden.
- [ ] **SD44 — World-Unterschied ohne Labels prüfen.**
  Gleiche Tonart/Stimmung, vergleichbarer Registerbereich, gleiche feste
  Referenzlautheit; Quellen-/Grammatikvergleich zunächst ohne Nature/FX, danach
  gemeinsame Roomfassung. Mehrere Seeds, unbekannte Reihenfolge, keine Namen.
  **Abnahme:** Unterschied wird durch Artikulation, Beziehungen und Zeitverhalten
  beschrieben; Erkennbarkeit beruht nicht nur auf Key/Hall/Dichte. Kurzer Vergleich
  plus längere interne Entwicklung; offenes Hörerurteil ausdrücklich markieren.
  Voraussetzung: SD17/SD35/SD43.
- [ ] **SD45 — Lange reale Audioläufe und Recovery prüfen.**
  Eventtrace UND PCM-Auswertung über mehrere Seeds/Modes/Worldzyklen, einschließlich
  Timerwrap, langen Tails, Start/Stop-Folgen, fehlender Zulassung und Scenewechsel.
  **Abnahme:** Keine eingefrorene Stimme, Endlos-Retry, verlorene Historie,
  unerklärte Dauerstille, kurzer Dauermotifloop, NaN oder zunehmende Energie.
  Aus gewählten Zeitpunkten kurze Hörproben; kein langes WAV an Nutzer.
  Voraussetzung: SD37–SD43.
  **Softwareabschluss2026-10-19:** Vorhandene lange Phrasen-/Stress-/Statebelege
  durch echte gemeinsame World-/Scene-/Fehler-/Recovery-/Neustartsequenz ergänzt.
  Genau eine Engineinitialisierung je800s,2 Seeds×64/512×Normal/Mainwrap;
  ganze PCM/Hookpaare141120000 Stereo-Frames pro Modus exakt. Native/ASan/UBSan
  gleiche Metrik,16 ganze Captures/213min20s über beide Modi; bestätigte Quellen
  an frischer physischer Reservierung, Quellen-/Clear-/Stillegrenzen bestehen.
  Native512-B-RAM-Scene-Backend statt erfundener Siliziumprüfung. Product-Save
  bleibt synchron/Journal OFF; echter Flash-/ECC-/Storageanschluss weiter SD41.
  Keine ganze64/512-PCM-Kreuzgleichheit an bekannten Quiet-Grenzen behauptet.
  Alle263 Firmwareinputs gleich. Eigener aktueller strikter SD45-CI-Job.
  SD45 bleibt ungecheckt und zählt für gehörte Langzeitform zur Hörgruppe.
  Details:[SD45_LONG_RECOVERY_CHECKPOINT.md](SD45_LONG_RECOVERY_CHECKPOINT.md).
- [x] **SD46 — Entfernte Produktpfade tatsächlich auslinken.**
  Nach Migration Archiv-/Altquellen, doppelte Raumwege und unbenötigte Buffer
  aus H743-Produktauswahl entfernen; Referenzcode nachvollziehbar behalten.
  **Abnahme:** Map/Callsites bestätigen Wegfall, kein Scene/Hook/Setter stellt
  sie wieder her; Bankdelta ausgewiesen. Keine vermeintliche Einsparung aus
  Ordnernamen oder Amount=0. Voraussetzung: SD00/SD10/SD27/SD31/SD36.
- [ ] **SD47 — Hotpath und Note-on-Arbeit auf Spitzenlast prüfen.**
  Quellenanregung, Parametercoefs, Schedulerretry, Worldvorbereitung und Storage-
  Handoff: keine unbounded Suche/Allocation/I/O. Vorbereiten außerhalb DMA mit
  begrenztem Statewechsel; nicht unbemerkt neue große Doppelbuffer bauen.
  **Abnahme:** Worst-case-Arbeit pro Tick/Block benannt, Output unabhängig von
  Datenrennen und Control-/Audioraten, CPU-/Stackkandidaten für SD48 dokumentiert.
  Voraussetzung: SD17/SD27/SD37/SD46.
  **Abschlussinventar2026-10-18:** DSP-/Übergabecallsites und Arbeit vollständig
  zugeordnet; Nature-Publikationen120 Fälle/10580880 ganze Stereo-Floatframes
  inklusive64/512, Stille/Clear/Neustart und ASan/UBSan.263 bestehende Inputs
  unverändert PR173. Siehe [SD47_HOTPATH_INVENTORY.md](SD47_HOTPATH_INVENTORY.md).
  Kein weiterer belegter DSPbefund. Hauptpunkt bleibt für den synchronen
  Product-Save/SD41 und echte IRQ-/Stackspitzen/SD48 offen; keine neue
  Room-/Quellen-Prep-Kleinstprüfung als vermeintliches Restinventar.
- [ ] **SD48 — Echte H743-Zeit-/Stackabnahme durchführen.**
  Nach UX-Integration: DWT inklusive Onset, maximale SHAPE-Releases, native/alte
  Engine-Retirement, Masterraum, schnell bewegte Targets, Display/LED und Storage;
  lange Läufe bei tatsächlicher Samplerate/Blockkonfiguration.
  **Abnahme:** `peak_load < 0.60`, null Deadline-Misses, nachgewiesene Stackreserve
  pro Kontext; keine underruns oder Watchdog-Recovery im Normalbetrieb. Fehlende
  Reserve zuerst durch Reduktion lösen. Erst am Gerät möglich; SD47 + reales Build.
- [ ] **SD49 — Reale Ausgangskette und Hardwarezustände abnehmen.**
  Tatsächliche PCB/BOM zuerst feststellen: verfügbare Ausgänge, Codec/Amp,
  Lautsprecher falls vorhanden, Kopfhörer/Line-Funktion nur soweit gebaut.
  Prüfen: Gain/Impedanz, Stereo/Mono, Netz-/Batteriezustände falls unterstützt,
  DC, Clocking, Einschalt-/Mute-/Ausschaltpop und elektrisches Eigenbrummen.
  **Abnahme:** Akustischer Körper/Headroom bei echter Lautstärke, kein physischer
  Störklang; keine erfundene Hardwareeigenschaft als Prüfergebnis. Nach SD48.
- [ ] **SD50 — Ruhige Wahrnehmung mit echten Hörern abnehmen.**
  Nutzer plus freiwillige geeignete Hörer; sensible Stellen mit Zeitpunkt und
  Quelle/Parameter erfassen: Alarm/Piep, Tube, Buzz/Hum, nervöse Bewegung,
  Ermüdung, Maskierung. Auch leise und reale typische Lautstärke testen.
  **Abnahme:** Konkrete Einwände sind gelöst oder betroffener Bereich entfernt;
  keine durchschnittliche Bewertung kaschiert wiederkehrenden Störklang.
  Ergebnisse sind Hörbefunde, keine Aussage über Neurodivergenz/Heilwirkung.
  Voraussetzung: SD44/SD49; echte Nutzung länger als kurze Vergleichsdatei.
- [ ] **SD51 — Sound-Spezifikation mit exakten Endwerten einfrieren.**
  Quellen/Register/Gains, Grammatikzustände/Timing/Seeds, harmonische Regeln,
  ausgewählte FX/Natur, Parameterkennlinien/Defaults und sämtliche Übergänge
  aus den bestanden Befunden konsolidieren. Kandidatenwerte oben ersetzen.
  **Abnahme:** Ein Versionsstand reproduziert alle Produktwelten; kein ungelöster
  Pflichtparameter, Widerspruch zwischen Docs und Firmware oder stiller
  Legacy-Default. Softwarestand vor UX, Hardwarekorrekturen danach versionieren.
  **Softwarekonsolidierung2026-10-20 abgeschlossen:** Sound0.10/SCN7,
  Defaults/Parameter-/Grammatik-/Übergabeverträge und265 Inputhashes dokumentiert:
  [SD51_SOFTWARE_CANDIDATE.md](SD51_SOFTWARE_CANDIDATE.md). Candidate0.3-Zeile
  korrigiert; alte Entwurfsbereiche ausdrücklich historisch, kein Parallelpreset.
  Finaler Haken erst nach gehörten Endwerten und SD48–SD50; zählt zur Hörgruppe.
  Voraussetzung: SD00–SD47; endgültiger Lock zusätzlich SD48–SD50.
- [x] **SD52 — Bestehende UI auf unveränderte Audioverträge anbinden.**
  Neue Display-/Button-/Encoderzuordnung darf UIzustände ändern; Start/Stop,
  Worldtargets, Besitzer, Volume und Stillwerden nutzen obige Audiooperationen.
  Human-idle/Displayruhe dürfen keine musikalische Inaktivität simulieren.
  **Abnahme:** UIwechsel/Wake kein Retrigger, Parameterjump oder neuer Klangpfad;
  gleiche musikalischen Eingaben liefern gleiche Audioereignisse. Konkrete
  **Softwareabschluss2026-10-20:** Bestehende H743-Button-/Encoder-/Display-/LED-
  Routen angebunden;900000ms Ruhe, erste Wakegeste verbraucht,30-ms-Guard und
  gehaltene/verbrauchte Releases geprüft.126 ganze12-s-PCMvergleiche je Modus,
  alle drei Worlds/21 Wakewege/64-512/Mainwrap; native+ASan/UBSan, kein
  Parameterjump, Save oder Retrigger. [SD52_UI_IDLE_CHECKPOINT.md](SD52_UI_IDLE_CHECKPOINT.md).
  Neues Grafiklayout hier nicht als entworfen behauptet; physische Bedien-/
  Deadline-/Stack-/Ausgangsabnahme weiterhin SD48/SD49. Nach SD51-Softwarestand.
- [ ] **SD53 — Sound-Definition-of-Done abschließen.**
  Jede KEEP-Quelle, jedes behaltene FX/Naturelement, jeder Parameterbereich und
  jeder Zustandswechsel hat Entwurf + Software + Hörbefund + nötigen Gerätetest.
  **Abnahme:** Kein offener soundkritischer Befund, kein ungeprüfter erreichbarer
  Legacyweg; entfernte Funktionen sauber migriert, Ressourcen-/Ausgangsreserve
  belegt, exakte Referenzversion und kurze Hörbelege verlinkt. Erst dann
  Sounddesign als Produktstand fertig nennen. Voraussetzung: sämtliche zutreffenden Aufgaben.

### Entwicklungscheckpoints und Merge-Regeln

| Checkpoint | Abgeschlossener Umfang | Integration / nächster Schwerpunkt |
|---|---|---|
| M0 | Aktuelle Transaktionen, autonome Quellenführung, Generate-Belegung, Release-Fixes, WOODLAND-Technikkorrektur und diese Roadmap | PR #129 in seine vorhandene Entwicklungsbasis integrieren; keine Produkt-/Hardwarefreigabe. Danach COAST trocken |
| M1 | Auswahl der trockenen Quellen, lokale Artikulation/Actual-Pitch-Verträge | Kleiner Quellen-Checkpoint; Hörfragen explizit offen, wenn noch unbeantwortet |
| M2 | Gemeinsame Harmonie, vollständiger Besitz/Belegung, manuelle World-Palette | Keine versteckte Begleitung; API-/Scene-Kompatibilität kennt ihre Migration |
| M3 | Ausgewählte World-Grammatiken und versionierter Katalog | Unterschiedliche musikalische Systeme; Gegenprobe bei gleichen Key/Pegeln |
| M4 | Ein Raum, gerechtfertigte kleine FX-Palette, gewählte Naturebene | Sämtliche übrigen erreichbaren FX/Geräusche entfernt oder referenz-only |
| M5 | Parameterbereiche, feste Gains, Übergänge, lange Hostläufe, H743-Ressourcen | Sounddesign softwareseitig spezifiziert; UX/UI kann darauf aufbauen |
| M6 | UX-Anbindung und reale Zeit-/Ausgangs-/Hörabnahme | Sound-Definition-of-Done; erst dieser Stand ist Produktfreigabe-Kandidat |

Nicht auf eine bestimmte Anzahl Aufgaben pro Merge festlegen: integrieren,
wenn ein zusammenhängender Vertrag geprüft ist und keine bekannte Regression
mitgeht. Vor jedem Merge tatsächlichen Head/Diff, Konflikte/Reviewbefunde und
relevante Checks prüfen; erwarteten Head verwenden. Entwicklungsmerge ist keine
Freigabe des gesamten Sounds. Keine automatische Retarget-/Merge-Kette nach
`main`; den konkreten Entwicklungszweig und Checkpoint benennen.

## TEST AM GERÄT

Die Geräteserie beginnt nach Sound-Softwarestand und UX/UI-Integration. Schon
jetzt ist definiert, was sie entscheiden muss:

| Test | Konkrete Durchführung | Bestanden, wenn |
|---|---|---|
| Trockener Körper | Jede gewählte Quelle tief/mittel/hoch, beide Ausgabekanäle und Mono, Attack/Decay/SHAPE-Grenzen | Keine Alarm-/Tube-/Buzz-Stelle; Körper bei realer leiser Wiedergabe vorhanden |
| Worldidentität | Gleiche Tonart/Referenzlautheit, unbekannte Reihenfolge, trockener und Raumdurchlauf | Drei gewählte Welten durch Tonkörper und musikalische Zeit unterscheidbar; schwache dritte wird gestrichen |
| Volle Belegung | Drei Quellen bzw. lokale kleinere Pools, maximal lange Releases; weitere Eingabe und Worldwechsel | Kein Steal-Klick, Overbudget, stuck Note oder unerklärter Pegelsprung |
| Tail-Harmonie | Farbwechsel bei langem Raum/Echo und gegebenenfalls Shimmer, dann Ruhe/Wiederstart | Alte Farben erzeugen keinen hörbaren Konflikt; System kehrt musikalisch zurück |
| Makrogrenzen | Jede Grenze plus die SD35-Risikokombinationen langsam und schnell bewegen | Ganze zugängliche Fläche bleibt musikalisch brauchbar, keine Sirene/Zipper-/NaN-Ausgabe |
| Stillwerden / Boot | Aus maximaler Ausgabe Stop, Clear, Mute/Unmute; Boot/Scene-Recall bei verschiedener gespeicherter Lautstärke | Definierte weiche Ruhe, keine alte Fahne/Note oder Überraschungslautstärke |
| UX-Verkehr | Spielen/Generate mit Display-/LED-/Encoder-/Storagearbeit und Wake | Keine Audioänderung durch idle/wake, kein verlorener Release oder Deadline-Miss |
| Ressourcen | Worst-case-DWT, Stack-Highwater und lange Sessions im endgültigen Build | Zielreserve tatsächlich vorhanden, nicht bloß mittlere CPU oder Linkerplatz |
| Physische Ausgabe | Tatsächliche Ausgänge/Lasten/Stromzustände; Pegel/DC/Noise/Pops und reale Lautstärke | Keine elektrische oder akustische Störursache; digitale Headroom und physischer Pegel separat dokumentiert |
| Langes Zuhören | Mehrere echte Sitzungen/Seeds ohne ständige Eingriffe; gezielte sensible Stellen protokollieren | Wiederkehr schafft Zusammenhang, Entwicklung bleibt ruhig; keine ungelösten störenden Stellen oder Wirkungsbehauptung |

**Aktuelle Einheiten: SD00/SD32/SD36/SD46 — reduzierte Produktauswahl und
explizite Migration; danach Quellenhörproben, Room/Nature-Messung,
langfristige Grammatik und Übergänge.** Quellen- und Gerätehörgates bleiben
offen; Referenzhistorie wird nicht als Kandidaten-Laufzeit beschrieben.
