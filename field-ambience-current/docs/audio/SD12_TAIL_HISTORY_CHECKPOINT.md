# SD12 — Ausklang und Tonhistorie technisch geprüft

Stand2026-10-18, auf PR175/Commit
`659cb04df4d9e145c6de1f7aea9ecd17fe2b8db6`. Dessen
[Run39111491995/732](https://github.com/Sancte3D/AMBIENT/actions/runs/39111491995)
ist vollständig26/26 grün; das echte öffentliche Harmonie-Proof-Artefakt wurde
gegen committed/lokal und sämtliche Quellenhashes geprüft. Kein neuer
Runtimebefund in dieser Einheit; alle263 bestehenden Firmware-/Header-/HAL-/
Startup-/Linker-/CMakeinputs bleiben bytegleich zur Basis.

**SD12 ist softwareseitig geprüft und zählt jetzt zur Hörgruppe.** Seine
gehörte Energie-/Wartezeitschwelle bleibt ungecheckt.24/54 Hauptpunkte sind
geschlossen,30 offen. Technische Blöcke10→9:7 Software/Prüfung+2 Gerät;
20 Hörentscheidungen+1 abschließender Gesamtcheck. Kein Hör-/Gerätehaken.

## Tatsächlicher heutiger Vertrag

- Eine klingende oder ausklingende Quelle belegt weiter ihren echten Besitzer
  und ihre angewandte Hz. Key/Collection/Tuning ändern diese Hz nicht rückwirkend.
- Erst ein tatsächlich gestarteter und anschließend physisch retirierter Ton
  wird in die16 Raum-Hz-Einträge übernommen. Ein ungeborener Pendington erzeugt
  keine gehörte Pitchhistory. Ablehnung erzeugt keinen neuen Ton.
- Derselbe gestimmte Ton darf eine vorhandene Raumfahne verbinden; ein live
  Unisono und die nahe Equal-/Just-Variante einer noch vertretenen Hz bleiben
  gesperrt. Die übrigen niedrigen Intervallgrenzen aus SD11 bleiben wirksam.
- Ein Eintrag endet spätestens317520 Audioframes nach seiner Main-beobachteten
  Retirement: nominell7,2s =1,5×maximalem nominalem Room-T604,8s. Eine erneute
  tatsächliche Retirement derselben Hz darf diesen Zeitpunkt konservativ erneuern.
- Früher frei wird die Historie bei nominal250ms ruhigem Room unter1e-5 ohne
  aktive Quellen. Diese Ruhe wird aus **Audioabschnittsmaxima** abgeleitet,
  nicht aus einer samplegenauen psychoakustischen Unhörbarkeitsmessung.
- Neue Zulassung stoppt bei13 vertretenen Tails;3 Plätze bleiben für ausgehende
  Quellen reserviert. Bestehende relevante Einträge werden nicht verdrängt.
  Clear/Mute leeren die gesamte Historie erst hinter dem abgeschlossenen Nullgate.

## Neue reale Grenzverläufe

`test_product_tail_history.c` verwendet die echte Product-Engine, alle echten
Quellen und den gemeinsamen Room. Für die gesonderten Stategrenzen bindet die
Fixture die unveränderte `engine_product.c` direkt ein. Es gibt keinen
Ersatzsynth, kein verändertes Firmwareobjekt und keine zusätzlichen Produktbuffer.

24 Parameterfälle:3 Worlds×Release0/1×Room0/1×Blockobergrenze64/512.
F#3 in D-Major/Equal, Velocity0,9, Attack/Color1, Master0,6, Nature0,
Defaultseed. Tuning wird nach2s auf Just gestellt, der alte manuelle Ton
bei3s losgelassen. Ein neuer Besitzer versucht danach dieselbe Tonstufe in
Just auf dem10-ms-Main-Raster. Die alte angewandte Hz bleibt bis zur Freigabe
vertreten und verhindert die kollidierende Variante.

Das10-ms-Raster hat441 Frames. Bei Obergrenze512 wird ein441er Abschnitt
gerendert, bei64 entsprechend aufgeteilt; hier wird keine512-Frame-DMA-
Abnahme behauptet. Audio-Frames und Audio-Epoch werden zusätzlich vor dem
realen Start nahe UINT32_MAX gesetzt, sodass beide während des Verlaufs wrappen.

| Reale Version je Parameterfall | Ganze Kontrolle |
|---|---|
| Wiederholte abgewiesene Anfragen bis zur echten Freigabe |Alter Ton/Room laufen; neuer Ton erst nach physischer Retirement und verschwundener kollidierender Historie |
| Ohne abgewiesene Anfragen, neuer Einsatz nur am zuvor beobachteten Freigabeframe |Ganze PCM bis einschließlich Neustart und Clear bytegleich zur ersten Version |
| Derselbe Verlauf mit Audioframe-/Epoch-Wrap |Ganze PCM und Retirement-/Freigabeframes bytegleich zur ersten Version |

72 vollständige reale Captures in24 Parameterfällen,48 ganze Paarvergleiche:
**45962784 Stereo-Frames vollständig bytegleich verglichen** je Prüfmodus.
Native C11/O2/Wall/Wextra/Werror und ASan/UBSan bestehen mit gleicher Metrik.
Kein Nonfinite-/Limiterfall. Der neue Ton wird2s gespielt, danach vollständiger
Clear mit Nullausgabe, leerem Quellenpool und leerer Tonhistorie geprüft.

Bei diesen Parametern und10-ms-Beobachtung:

| Quelle | Tatsächliches Ende nach manuellem Key-up | Wartezeit von beobachteter Retirement bis neuer Just-Zulassung |
|---|---|---|
| COAST |6,36 /14,61s bei Release0/1 |0,24..0,25s |
| WOODLAND |18,14 /42,31s bei Release0/1 |0,24..1,03s, abhängig vom Room |
| HIGHLANDS |4,95 /11,36s bei Release0/1 |0,24..0,25s |

Diese Werte gelten für diesen F#3-/Velocityfall, nicht für jede Tonhöhe,
Velocity oder Worldphrase. Die lange manuelle WOODLAND-Saite bleibt erhalten;
ein Key-up wird nicht in den kurzen historischen Pluck-Stopp umgedeutet.
Die7,2-s-Obergrenze wird in diesen realen Fällen nicht ausgeschöpft. Deren
exakte Ablauf-/Refreshgrenze prüft der unten getrennte Statefall.

## Beobachtete Partitionsgrenze, keine falsche Bytegleichheitsbehauptung

Die64er-/512er Obergrenzen liefern an der Quiet-Freigabe teilweise einen
anderen Zulassungstick: maximal441 Frames =10ms. Source-Retirement-Acks und
der letzte Room-Peak sind Blockbeobachtungen; der250-ms-Zähler übernimmt
entsprechend ganze Abschnittslängen. Er ist keine exakt samplegezählte
250-ms-Frist ab physischem Quellenende. Der Main-Beobachtungstick kommt hinzu.

Die obige ganze PCM-Gleichheit gilt **innerhalb derselben Partition** für
Rejektionskontrolle und Wrap. Hier wird ausdrücklich keine ganze64/512-
Bytegleichheit des neu zugelassenen Tons behauptet. Der Unterschied liegt
innerhalb der bestehenden Block-/Main-Beobachtungslatenz; die konservative
Fixturegrenze ist ein Audioblock plus ein Main-Tick. Bestehende feste
Source-/Room-/Mastertrajektorien bleiben ihre getrennten Bytegleichheitsbelege.

Die bereits im SD37-Nachweis dokumentierten Quiet-Diagnoseabweichungen waren
ohne neue Audiostarts. Diese SD12-Gegenprobe benennt nun ausdrücklich auch die
gezielt an der Schwelle versuchte neue Zulassung. Musikalische Angemessenheit
der Freigabe und verbleibender Tailenergie bleiben Teil des SD12-Hörgates.

## Weitere direkte Fälle und strikt getrennte Stategrenzen

6 echte Common-tone-Fälle (3 Worlds×2 Partitionen): warmer Room, tatsächliche
100-ms-Kontextretirement des alten manuellen Besitzers, positive Roomausgabe
und vertretene alte Hz. Ein neuer Besitzer darf genau dieselbe gestimmte Hz
beginnen, ohne die alte Roomfahne zu löschen.

6 echte Cancel-Fälle: Key-up vor dem ersten Audio startet keine Quelle, erzeugt
keinen Onset und hinterlässt keinen Tonhistorieneintrag.

10 **synthetische Statechecks** verwenden die echte `remember`-/`reap`-Logik,
erzeugen aber bewusst keine angebliche akustische Raumfahne: Deadline genau0
am32-Bit-Wrap, noch geschützt bei UINT32_MAX, frei bei0;12/13-Eintrags-
Zulassungsgrenze, voller16er Speicher ohne Verdrängung, ausschließliches
Refresh der gleichen Hz und spätestes Ende aller Einträge. Der16er Vollfall
ist ein defensiver Statefall, kein behaupteter durch Zulassung erzeugter
akustischer Verlauf. Keine mathematische Unhörbarkeit aus einem Statebit.

## SD12-Kriterienmatrix

| Kriterium | Technischer Stand | Noch erforderlich |
|---|---|---|
| Reale Source-Releases und Effekttails verfolgen |Angewandte Hz bis tatsächlicher Retirement, dann gemeinsamer Room/16er Ledger. Echo/Blur/Shimmer sind aus Product entfernt, kein ungetrackter Oktavlayer |Gehörte Relevanz der tatsächlich behaltenen Quelle und des Rooms |
| Farb-/Tuningkollision warten lassen |Reale Equal→Just-Nachfrage bleibt während Hold/Release/History gesperrt; ganzer Kontrollrender unverändert |Kein universeller Konsonanz-/Schwebungsentscheid aus dieser festen Probe |
| Gemeinsamer Ton verbindet |6 echte warme Common-tone-Fälle, alte Hz/Room erhalten |Verbindung im musikalischen Ensemble hören |
| Keine dauerhafte Sperre |Alle24 echten Fälle kommen frei; exakte Deadline-/Refresh-/Kapazitätsgrenzen separat geprüft |Nominale7,2s/250ms/1e-5 gehörseitig kalibrieren |
| Kein gehörtes Gedächtnis aus Ablehnung oder Cancel |45962784 ganze Rejektions-/Wrap-Kontrollframes,6 echte Cancel-Fälle |Keine neue Programmierung allein wegen ungecheckter Hörwahl |

## Reproduktion und nächste einzelne Einheit

Im `firmware-c-next`:
`python3 tools/review_product_tail_history.py build-product-tail-history --sanitize`.
[SD12_TAIL_HISTORY_METRICS.json](SD12_TAIL_HISTORY_METRICS.json),7936B, SHA256
`92d3948d84aed07ba868ee5ed14fde58e2fe7e78c25552a4fb1d9ac06df7dd04`.
Normaler Hostrunner verwendet `--fixture-only`; der eigene aktuelle CI-Job
prüft zusätzlich die263 unveränderten Firmwareinputs gegen den exakten PR175.
Kein neuer ARM-Zeit-/Stackwert, Hardwaretest oder Hör-WAV aus dieser Einheit.

Als nächster unabhängiger technischer Block folgt **SD31 — übrige
Geräuschwege**. Bestehendes Product-Compile-/Quellen-/Nature-Inventar zuerst
lesen, dann nur einen tatsächlich verbliebenen Softwarebefund bearbeiten.
SD12 nicht weiter als offene Programmierung zählen, solange nur die gehörte
Tail-/Wartezeitschwelle fehlt. Kein Hauptpunkthaken ohne diese Entscheidung.
