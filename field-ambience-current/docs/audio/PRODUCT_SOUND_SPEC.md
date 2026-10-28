# AMBIENT — Sound Software Candidate 0.12

Stand28.10.2026, Hierarchiekorrekturen auf PR194/254ee97. Aktueller vollständiger
Softwarevertrag. Michalis beurteilt die V2-Worlds als „Besser“; das ist keine
pauschale Parameter-/Gerätefreigabe. Der endgültige Freeze bleibt offen.

## Eindeutige Buildauswahl

| Build | Schalter zusätzlich zu H743/Release | Vertrag |
|---|---|---|
| Reference | keiner | Ausdrücklicher CMake-Default, historische Referenz |
| Grundprodukt0.10 | FAM_SOUND_PROFILE=product | Vorheriger reduzierter Klang/SCN7; [unveränderter Archivvertrag](PRODUCT_SOUND_SPEC_0_10_ARCHIVE.md) |
| Aktueller Sound0.12 | Product + FAM_SOUND_FINISH_CANDIDATE=ON | Materialmodell2, FX-Modell2 mit gemeinsamer Pegelregel/Färbung, geregelte Nature, GLP1; ohne Sessionflag weiter SCN7, neue FX/Loops dann nicht dauerhaft gespeichert |
| Sound-Session | zusätzlich FAM_SOUND_SESSION_CANDIDATE=ON | SCN8/JSS1 mit echtem asynchronem H743-Speicher |
| Messbuild | zusätzlich FAM_DEVICE_BENCH_CANDIDATE=ON | Derselbe Klang, startup-MSP-Paint, SAI-/DMA-Fehler und Savebeobachtungen |

Neue Flags standardmäßig OFF. Session erfordert H743/Product/Full-Sound,
Bench erfordert Session; altes FAM_SCENE_JOURNAL_CANDIDATE ist damit exklusiv.
Keine neue Encoderbelegung und keine automatische Flash-/Mergeaktion.
[Aktuelle Quellhashes und aus C ermittelte FX-Grenzen](SOUND_CANDIDATE_INPUTS.json).
SD51_SOFTWARE_INPUTS.json bleibt historische0.10-Provenienz, kein aktueller Freeze.

## Tonkörper, Besitz und musikalische Zeit

| World | Tatsächlicher Tonkörper | Neutrale Basis |
|---|---|---|
| COAST | Kontinuierliche harmonische Fläche mit starker zweiter/vierter Harmonischer und langsamer Obertonentwicklung | 1,8-s-Bloom, Quellenfaktor0,50 |
| WOODLAND | Einmal angeregter harmonischer Modalkörper, tragende vierte/sechste Körperregion, keine Nachanregung | Weicher120-ms-Einsatz, nominal36-s-Grundton-T60, Quellenfaktor0,32 |
| HIGHLANDS | Hohler1/3/5/7/9-harmonischer Pfeifen-/Atemkörper, leise notengebundene Aspiration | 700-ms-Einsatz, Quellenfaktor0,50 |

Die begrenzten Grammatiken, Register, Major/Minor-Cores, Equal/Just,
DSP-Startbestätigung und Eigentümerverträge des Archivvertrags gelten weiter:
drei globale reale Quellen inklusive Vorbereitung/Release, WOODLAND höchstens
zwei. Keine vierte World, tonale Zusatzschicht oder inharmonische Verstimmung.
Manual und Generate benutzen dieselben Familien/Zulassung; gehaltene Hz
werden nicht retuned. [Materialmodell2](WORLD_MATERIALS_V2_CHECKPOINT.md)
ersetzt alte Tonkörper-/Attack-/WOODLAND-Gain-/Colorbeschreibungen von0.10.

Activity steuert Phrasenzeiten; Attack/Release werden beim Start eingefangen.
Physische Entfaltung bleibt notengebunden.128 Gestenereignisse enthalten
Zeit, Cell, Register und Velocity; eigene Playback-Besitzer, offene Noten
über den Looprand gebunden. Restore gestoppt, keine autonomen neuen Stimmen.

## Vollständiger Effektvertrag vor Encoderbelegung

Ein Main-eigener sound_parameters_t besitzt zehn Float-Ziele plus Freeze/
Plate. Get/Apply und alte Makrosetter verwenden denselben Zustand und erhalten
unabhängige Feinwerte. Nichtendliche Zahlen lehnen die ganze Transaktion ab,
gültige Zahlen werden begrenzt. Bei unterbrochenem Publish übernimmt Audio
keinen gemischten Snapshot; kein Spin und keine IRQ-Sperre.

| Ziel | Bereich | Default | Verhalten |
|---|---|---|---|
| Delay-Anteil | 0..1 | 0,16 | 80-ms-Zeitkonstante |
| Grain-Anteil | 0..1 | 0,12 | 80 ms, zwei überlappende untransponierte Grains |
| Drive-Färbung | 0..1 | 0,04 | 80 ms, weiche Sättigung des Rückwegs |
| Crush-Färbung | 0..1 | 0 | 80 ms, optionale Quantisierung des Rückwegs |
| Phaser-Anteil | 0..1 | 0 | 80 ms, vier Allpassstufen |
| Delay links/rechts | je40..700 ms | 210/267 ms | 40-ms-Überblendung fester Taps, keine Doppler-Tapfahrt |
| Feedback | 0..0,5 | 0,32 | 80 ms, gekreuztes Delayfeedback |
| Grain-Länge | 40..260 ms | 8192 Samples, etwa185,7596 ms | Grain-Rückweg40 ms ausblenden, Fenster neu, wieder einblenden; Inhalt erhalten |
| Phaser-Tempo | 0,01..0,5 Hz | 0,07 Hz | 80 ms, kontinuierliche Phase |
| Freeze | false/true | false | Nur bei Grain mindestens einem Q16-Schritt aktiv; Grain0 deaktiviert Freeze. Tatsächlich gespeicherter Inhalt schützt Pitchhistory, leere Aufnahme sperrt nicht. Keine dauerhafte Audioaufnahme. |
| Raummodell plate | false/true | true | Eigenständiger vereinfachter Float-Plate oder vorhandener FDN;40-ms-Rückwegfade und Clear vor Wiederverwendung |

Ein gemeinsamer Musik-Effektbus, keine neuen Tonquellen pro Effekt. Grain-
Rückweg geht nicht in Delayfeedback. Room0..1/default0,5: nominal T60
1,2+3,6*r*r s,80-ms-Amount und40-ms-Enable-Rampe; Room0/Dry wird kalt.
Beide Räume erhalten eine gemeinsame Rückwegkalibrierung: nominal0,25*r
RMS gegenüber Dry, also−24/−18/−14,5/−12dB bei r0,25/0,5/0,75/1. Audio-
eigene250-ms-Leistungsmessung, begrenzter Gain0,125..8 mit500-ms-Smoothing;
unter dem Eingangsfloor bleibt der Gain für die Fahne stehen. Nur der Raum
wird kalibriert, keine Quellen-/Master-AGC. Drive/Crush/Phaser bearbeiten den
ganzen Rückweg beider Räume einschließlich Delay/Grain. Die reduzierte
OFF-Referenz behält ihren alten5*r-FDN. Keine Archiv-Echo-/Blur-/
Tape-/Chorus-/Shimmer-/Reversequellen zurück im Compile. Neue Delay/Grains/
Färbungen sind explizite Full-Sound-Module. Frühere Remove-Belege gelten
weiter für die alten Module, nicht als Hörfreigabe der neuen Palette.

## Nature und Ausgabe

Nature default0: COAST Wellen, WOODLAND Regen mit seltenem sehr leisem Donner,
HIGHLANDS drei Brisen-/Windprofile mit unregelmäßigen Intervallen. Eigenes
Wetter-RNG, keine Änderung von Zulassung/Planer-RNG/Belegung. Mischung nach
Musik-FX; Nature-Regler und Wetter bleiben separat. [Nature-Vertrag](NATURE_ACCENTS_CHECKPOINT.md).
Gemessene Lautheitsabstände sind Testfälle, keine universale Wahrnehmung.

[Ursprüngliche Hierarchieprüfung](SOUND_HIERARCHY_REVIEW.md) und
[Korrekturen](SOUND_HIERARCHY_FIXES.md): Nature folgt einer Rückwegregel,
Ziel etwa28dB unter Dry. Bei verstummtem Dry bleibt ein überlebender Musik-/
Freeze-Rückweg Referenz.20-ms-AC-Leistungsmessung,10-ms-Absenkung,2-s-Erholung;
nach wirklicher musikalischer Ruhe kommt die ursprüngliche Kulisse langsam
zurück. Keine Änderung am Cell-Pegel oder natürlichen WOODLAND-Verlauf.
Nominale Wetter-/Donnergrenzen und solo gerendertes Wetter bleiben erhalten.
Die Regel ist kein universaler Wahrnehmungs- oder Verdeckungstest.

Send0,35, Master nach Musik/Nature. Volume Host0,60/Gerät0,30 mit120-ms-
Zeitkonstante. Ganze Kette über40-ms-Gate zu exakter Null, dann Historyclear;
Clear erhält Reglerwerte, Mute außerdem Generateabsicht. Owner-/Tailharmonie,
DC-Tracker, Peakschutz und PCM16 wie im Archivvertrag. Volume/Mute/Generate/
physische Besitzer und laufende Effekt-History werden nicht restauriert.
Die trockenen V2-Tonkörper bleiben gleich. Die vollständige96-s-Musik ändert
sich durch die beauftragte Raumkalibrierung; alte Hördateien bleiben Historie.

Abgewiesene physische Noten erhalten im H743-Frontend konkrete Rückmeldung:
FREEZE/CLEAR TO PLAY, NOTES/VOICES FULL beziehungsweise HARMONY/NOTE ACTIVE
oder TAIL ACTIVE. Clear verwirft den eingefrorenen Inhalt nach dem bisherigen
40-ms-Gate; Harmoniehistory wird nicht ungeschützt freigegeben. Encoder zuletzt.

## Persistenz und Fehlerverhalten

FXP1:52B, zehn Float32-Ziele explizit Little Endian, Flags, reservierte
Nullbytes, CRC32. Ungültiger Packetwert auch mit korrekter CRC abgelehnt.
GLP1 maximal1040B, explizite Ereignisse/FNV-Prüfsumme; ganze Prüfung vor
Änderung des Loops. Ein Ereignis genau am Loopende ist gültig.

SCN8: fünf1168-B-Slots, Store5848B. JSS1:8192-B-Payload, Bank2-Sektoren6/7,
andere Magic/Geometrie als JSC1. Payload vor Header, ganzer Readback vor RAM-/
UI-Erfolg, letzter gültiger Commit erhalten. Boot liest vor Audioinit;
live pro Main-Schritt höchstens ein Start/Poll oder32-B-Read. Ein Besitzer
für HAL-Flash/IRQ/ECC; im Session-Build kein synchroner Flashfallback.

SCN5/6/7 nur lesend migrieren, Standard-FX/leerer Loop; erster neuer Commit
erhält den alten Rohsektor. Erkanntes experimentelles JSC1 oder unverfügbare
Reads blockieren Schreiben. REC muss vor Save abgeschlossen sein, BUSY
sperrt weitere Save/Recall. Angenommener Snapshot bleibt trotz späterer
Bedienung unverändert. Freeze als false gespeichert, Audio/Histories flüchtig.
SHIFT+Cell/Cell bleibt Save/Recall mit SAVING/SAVED/SAVE FAILED.

## Ressourcen, Messung und Freigabe

Delay262144B in internem D1, SF-State109948B in D2, Nature-Pegelzustand56B
zusätzlich. Je Pegelregel ein sqrtf nur alle64 echten Samples, keine Allokation.
Gültige Delay-Frameanzahl
schützt NOLOAD-History, kein zusätzlicher256-KiB-Clear im Audio-IRQ. Echte
Bank-/DMA-/Symbol-/Compilerframeprüfung im ARM-Linkaudit, Code/Konstanten
vollständig Bank1. Startup-Paint/Fehlererfassung nur im separaten Messbuild.

[Lesender Exporter](../../firmware-c-next/tools/DEVICE_CAPTURE_README.md):
exaktes ELF/BIN gegen Ziel, optional tiefster beobachteter MSP-Schreibzugriff,
SAI-/DMA-Fehler und Savezähler. Kein minimum-SP-/vollständiger Exception-/
Dispatch-/per-Kontext-Stack-/Ausgangspass. device_accepted bleibt false.
SD41/47/48/49, Hörentscheidungen einschließlich SD44/50 und finaler
Freeze/SD53 bleiben echte Gates, keine aus einem Hostmodell erfundene Abnahme.
