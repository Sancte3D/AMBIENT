# SD31 — verbleibende Geräuschwege, 2026-10-18

SD31 ist softwareseitig geprüft. Alte Texturen sind tatsächlich aus dem
Product-Link entfernt; die verbleibenden Zufallsanteile haben benannte
Einstiegspunkte, Besitzer und Rollen. Der Audit hat keinen weiteren
Softwarefehler ergeben. **SD31 bleibt für die gehörte Rollen-, Resonanz- und
Maskierungsentscheidung offen.** Insbesondere rechtfertigt dieser Nachweis
weder gemeinsamen Wind noch Bowed-Grain als endgültigen Produktklang.

Es gibt keine neue Naturebibliothek, keine DSP-/Parameteränderung und keinen
neuen Hörvergleich. Lange WOODLAND-Ensembles bleiben unverändert. Reference
bleibt Default, Product Kandidat, SCN7 unverändert, Journal OFF und Live-Save
weiterhin synchron. Technische Hauptblöcke9→8, davon6 Software/Prüfung und2
Geräteabnahmen.24/54 Hauptpunkte geschlossen,30 offen; nun21 Hörentscheidungen.

## Tatsächlicher Product-Link

Exakte Basis: PR176, Commit `e2104094cfda3f4af563c608cf4c0f94430c945f`,
Tree `2438882d671b67b457559c4c7c94581471b3ab73`.
Alle263 bestehenden C-/Header-/HAL-/Startup-/Linker-/CMakeinputs sind dazu
bytegleich. Dieser Checkpoint ändert ausschließlich Dokumentation und Belege.

Der H743-Product-Build aus [Run734](https://github.com/Sancte3D/AMBIENT/actions/runs/39156738516)
ist completed/success. Das tatsächlich heruntergeladene Artefakt12147027941
gehört laut API genau zu diesem Head. ZIP342655B, SHA256
`d9e3a318883a3b9feca1121e851e6978b5889209ffed20c3fb5b118bd79a8306`;
API-Digest und ZIP-CRC geprüft. Seine73 Firmware-LOAD-Objekte enthalten die
drei Quellen, `engine_product.c`, `nature.c`, `ambient_room.c` und die
Product-Worlds, keine der ausgeschlossenen Dateien und kein V2-Objekt.
Die tatsächlich belegten `.text`-Abschnitte enthalten ebenfalls keine der
retired DSP-Funktionsfamilien. Discarded sections zählen dabei nicht als aktiv.

| Datei aus dem echten CI-Artefakt | Bytes | SHA256 |
|---|---:|---|
| Product BIN |196844|`7ba0c2db6914cdc581fcfc3ad72a9adbeb5712835ef9f5a5ef3dd1276f77c285`|
| Linkmap |449921|`c55b1e2acd29b9b6dab7b0e9673d0d06ac659c1a5247122743dcc385800cdf18`|
| Product-Buildaudit |60206|`2527b262e84103be07d348541080c7e2c87c62b4929284c7817ace8f257333ba`|

Das Artefakt enthält BIN, HEX, Map und den CI-Buildaudit, **keinen ELF**.
Der bestehende CI-Checker `tools/check_product_link.py` hat den echten ELF und
die Compilecommands geprüft. Hier werden heruntergeladene Map/BIN/Audit und
aktuelle Quellen geprüft; kein neuer lokaler ELF-Link oder WCET behauptet.
Der im CI-Audit vermerkte ELF-SHA steht gesondert im maschinenlesbaren Beleg.

## Entfernte Wege und Eintrittspunkte

| Bisheriger Geräuschweg | Product-Entscheidung und technische Grenze |
|---|---|
| `texture.c`: Brown-Rumble, Breath, Warm/Air und Textur-Crackle | Nicht kompiliert/geladen. `engine_set_texture` ist retired no-op; kein versteckter Amount-Null-Layer |
| `ambience.c`: alte Wetter-/Wasserwege, Sea-Hum, Fjord, Desert-Haze/Sand, Vinyl | Nicht kompiliert/geladen. Die neue `nature.c` ist ein eigener deklarierter Pfad; alte geografische Presets können diese Generatoren nicht aktivieren |
| `tape.c`, `ambient_effects.c`: Eigenhiss/-hum und alte Masterpalette | Nicht kompiliert/geladen. `engine_set_age` ist retired no-op; die Reference-Hisskorrektur ist keine weitere Product-Rauschquelle |
| V2/`material_texture.c`, Archive sowie alte Pad-/Bass-/Drone-Familien | V2-Liste im Product leer; kein geladener Ersatzlayer. Kein automatisch mitlaufender Bass oder sonstiger Besitzer ohne zugelassenen Start |
| Alte Scenes/Worldwerte und Kompatibilitätsregler | Aktuelle Product-Worlds und SCN5/6→SCN7-Migration; retired Parameter bleiben ohne Klangwirkung. Aktuelle Scene-/Boot-/Parameterprüfungen bleiben gültig |

Der Nachweis folgt CMake-Auswahl, echten LOAD-Objekten, belegten Funktionen
und dem Render-/Setterrouting. Das Vorhandensein von Archivdateien oder
allgemeinen Zufalls-/Pink-Noise-Helfern in `dsp.c` ist kein autonomer Audiopfad.
Ein umbenannter Ordner oder ein auf Null gestellter Regler wäre kein Remove.

## Vollständiges verbleibendes Geräuschinventar

| Weg / Ort | Einstieg, Funktion und Besitz | Idle, Ende und offenes Hörkriterium |
|---|---|---|
| COAST: `bowed.c`, `bowbp` | Nur erfolgreiche Quellenzulassung aus Manual oder Generate. Weißrauschen durch pitchrelativen BP bei2,6×Grundton/Q1,4, im gemeinsamen Tonkörper; Pegel folgt Bogendruck und vollständiger Quellenhüllkurve. Kein eigener Slot/Loop | `V_IDLE` überspringt Noise/Filter; Retirement beendet dieselbe Quelle. Grain ist auch im Sustain vorhanden. Ob es wie Saite oder störendes Summen/Resonanz klingt, bleibt SD03/SD31-Hörwahl |
| HIGHLANDS: `horn.c`, `chiffbp` | Dieselbe strenge Quellenzulassung.1700-Hz-BP/Q1,1 nur während der ersten90ms; quadratisch abfallend unter der Amplitudenhüllkurve | Danach kein neuer Chiff-Noiseinput. Gesamte Quelle endet mit ihrer Hüllkurve. Kein eigenständiger Breath-Teppich; Piepen/Anblascharakter bleibt SD04/SD31-Hörwahl |
| WOODLAND: `pluck.c`, Anregungsvariation | Zufall variiert ausschließlich die Anschlagposition bei zugelassenem Note-on. Bis8 harmonische Anregungsmoden werden vor Audio vorbereitet und zentriert; kein kontinuierlich addiertes Rauschen | Ganze vorhandene lange Saite und Release bleiben erhalten. Keine Textur-/Hissquelle; trockener Tonkörper bleibt SD05/SD06-Hörwahl |
| Alle Worlds: `nature.c`, Wind | Optionaler Amount, bootmäßig0. Getrennte Kanal-/Weather-RNGs und unregelmäßige Gust-/Eddy-Uhren; LP Q0,707 plus Low-Rumble-Abzug | Kalt bei Amount0 kein Weather-/Noise-/Filter-DSP. Warmes Ausschalten folgt der bestehenden Glättung bis zur Idlegrenze. Gemeinsamer Wind ist ausdrücklich unakzeptiert, Hören/Weglassen SD28/SD29/SD31 |
| COAST: `nature.c`, Wellen | Nur positive optionale Nature. Pink Noise durch Wasser-LPs/Q0,707, DC-/Low-Abzug und unregelmäßige Anstieg-/Abfall-/Pausenfolge; kein Sea-Hum | Mit demselben Naturetarget ausblendend; keine tonale Belegung. Gleichförmiger Schwall, Rumpeln, Maskierung und Ortseindruck bleiben SD28/SD30/SD31 |
| WOODLAND: `nature.c`, Tropfen | Nur positive optionale Nature. Höchstens8 ungetunte37-ms-Rauschereignisse, weicher2-ms-Eintritt und Wasser-LP; keine gestimmte Tropfenmelodie | Feste Ereignisplätze; keine unendliche Platzsuche. Nervöses Ticken und Maskierung bleiben SD28/SD30/SD31 |

`engine_render` mischt Quellen in Dry und denselben0,35-Send, verarbeitet
einmal den gemeinsamen Room und addiert danach Nature. Natur hat weder
Pitchowner noch Roomsends. `engine_set_atmosphere` ist nur ein Alias für
`engine_set_nature`; kein weiterer Generator. World/Seed/Amount werden mit
dem bereits geprüften Main-/Audio-Vertrag übergeben. Clear/Mute verwenden den
vorhandenen40-ms-Ausgangsgate; Nature-/Roomzustand wird erst hinter Null
zurückgesetzt. Allgemeines Amount0 ist keine zugesagte sofortige Stummschaltung.

## Wiederverwendete aktuelle Prüfungen und Grenzen

| Beleg | Aussage für diesen unveränderten Stand |
|---|---|
| Vollständiger lokaler PR176-Hostrunner | Exit0 unmittelbar vor diesem Dokumentaudit; reale `test_room_nature.c`, Quellen-/Stille-/Clear-/Parameter-/Scenesuiten und abschließend489609 Effektchecks ohne Fehler. Log-SHA im JSON. Kein weiterer Gesamtlauf für diese Dokumentänderung |
| Aktueller CI-Host und H743-Job in Run734 | Beide completed/success am exakten Basishead. Audit-Snapshot:26/27 Jobs erfolgreich, nur archiviertes Spec0.8-Hörpaket noch laufend; kein Gesamtworkflow-Pass vorweggenommen |
| [Nature-Handoff](SD47_HOTPATH_INVENTORY.md) / [Metrik](SD47_NATURE_HANDOFF_METRICS.json) | Alle4 gespeicherten Quellen-/Test-/Toolhashes stimmen aktuell.120 echte Storegrenzen plus3×38-s-Bursts,10580880 Stereo-Frames, nativ/ASan/UBSan. Bestehende Messung, hier nicht neu gerendert |
| [SD28-Hörvorbereitung](SD28_NATURE_CHECKPOINT.md) | Frühere gleich behandelte Mit/Ohne-Nature-Proben und offene Orts-/Maskierungsfragen. Historische Parameter/Messungen bleiben als solche bezeichnet; keine neue PCM-Freigabe aus diesem Inventar |
| [SD12-Tailhistorie](SD12_TAIL_HISTORY_CHECKPOINT.md) | Aktuelle Eigentümer-/Ausklanggrenzen sind geprüft. Die dokumentierte blockweise Quiet-Grenze bleibt gültig; keine neue samplegenaue Behauptung |

Maschinenlesbar: [SD31_NOISE_PATHS_AUDIT.json](SD31_NOISE_PATHS_AUDIT.json).
Es enthält alle73 Objektpfade, Quellenhashes, den263-Input-Manifesthash und
die tatsächlich geprüften Artefaktgrößen/-hashes. Der bestehende vollständige
ARM-Check lässt sich mit einem Product-Build und `tools/check_product_link.py`
reproduzieren; in CI gehört er zum H743-Crossbuild.

Keine Messung beweist elektrische Brummfreiheit der Hardware, angenehme
Bandresonanz, akustische Maskierungsfreiheit oder ruhige Wiederholung.
Diese Entscheidungen bleiben bei SD03/SD04/SD28–SD31 und der Geräteausgabe
SD49. Kein Hörhaken aus der bloßen Endlichkeit eines Generators.

## Nächste technische Einheit

SD31 zählt jetzt korrekt zur Hörgruppe. Nächster unabhängiger Softwareblock
ist **SD33**: zunächst die vorhandenen Activity-/Color-/Room-/Attack-/Release-
Belege gegen seine noch offenen Kriterien konsolidieren; nur einen tatsächlichen
Restbefund ändern. Keine bereits grünen Einzelkalibrierungen blind wiederholen.
