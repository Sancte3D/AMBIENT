# AMBIENT — Fehlerprüfung und Eigenständigkeit der Welten

27.10.2026. Geprüfter Klangstand: PR188,
`500bd362056540895f0f67cecccf26d995e509e0`. Auftrag: konkrete Fehler suchen,
Eigenständigkeit der drei Worlds beurteilen und den Nutzen einer vierten
World einschätzen. Keine neue World und keine Klangparameter wurden auf
Grund der Messungen ungefragt verändert. Lange WOODLAND-Töne bleiben erhalten.

## Ergebnis

Zwei tatsächlich reproduzierbare Fehler in der H743-Bedienlogik wurden
korrigiert. In den zusätzlichen tatsächlichen Audiofällen wurden keine
nicht-endlichen Samples, Übersteuerung des Ausgangsschutzes, überschrittenen
Quellgrenzen oder verlorenen Stop-Ausklänge gefunden. Dies ist keine Aussage,
dass jede denkbare Bedienfolge oder die echte Hardware fehlerfrei ist.

Die Welten unterscheiden sich durch Quellen und musikalisches Verhalten.
**WOODLAND hat die klarste andere zeitliche Materialwirkung. COAST und
HIGHLANDS sind im gehaltenen Grundklang noch nah beieinander.** Ihre
unterschiedlichen generativen Zeitverläufe lösen diese Nähe beim manuellen
Spielen nicht automatisch auf. Das ist ein Entwurfsbefund, kein bewiesener
Wahrnehmungstest und kein Beleg einer universell angenehmen Wirkung.

## Gefundene und korrigierte Fehler

| Befund | Reproduktion | Korrektur und Nachweis |
|---|---|---|
| Loslassen unter dem Szenenmenü verloren | Cell ohne HOLD drücken, Szenenmenü öffnen, Cell loslassen. Der Browser behandelte nur neue Slot-Drücke; die ursprüngliche Note bekam kein Off. COAST/HIGHLANDS konnten damit weiterklingen. | Ein bereits gerouteter Druck behält seinen ursprünglichen Release-Empfänger. Unbesessene reine Slot-Auswahlen erzeugen weder Note-Off noch erfundene REC-Ereignisse. Vollständige 12-s-Ausgabe entspricht den direkten Controls in allen Worlds, mit normalem Loslassen und absichtlich gehaltenem HOLD. Auch ein zuvor aufgezeichneter Druck bekommt sein REC-Off. |
| SHIFT im selben MCP-Snapshot zu spät | SHIFT und Cell treffen gemeinsam ein. Die Cells wurden vor SHIFT ausgewertet: falsches Register; im Szenenmenü konnte Laden statt Speichern gewählt werden. | Nach der Wake-Sperre wird der wirksame SHIFT-Zustand vor den Cell-Ereignissen angewandt. Oberes Register, Scene-Save und SHIFT-Loslassen mit Slot-Auswahl ohne versehentliches Schreiben sind direkt geprüft; die bestehenden Wake-only-Sperren bleiben aktiv. |

Die neuen Scene- und SHIFT-Prüfungen wurden zuerst auf dem alten Code rot:
fehlendes Off bzw. falscher Owner statt 9. Nach den Korrekturen bestehen sie
native und ASan/UBSan sowohl im bisherigen Product-Build als auch im
vollständigen Soundkandidaten. Neun weitere komplette 12-s-PCM-Paarvergleiche
pro Modus, zusätzlich Scene-Save und REC-Provenienz. Die bisherigen 126 Wake-
Vergleiche, physische Cell-Freigaben und der Nature-Editor bleiben bestanden.
Keine musikalische DSP-Datei wurde für diese Reparatur verändert.

## Kontrollierter Klangvergleich

Echte Product-Engine mit vollständigem Soundkandidaten, Nature 0, D-Dur/ET,
Velocity 0,7, Attack/Release 0,5, Master 0,75, Seed 1234. Derselbe manuell
zugelassene Ton in jeder World; physisch gehaltene Cell-Entfaltung. Note-On
bei 1 s, Note-Off bei 11 s, vollständige Aufnahme 18 s. Geprüfte MIDI-Töne
50/57/62; zusätzlich Color 0/1 bei 50. Alle Fälle trocken mit Room 0 und mit
gemeinsamem Room 0,5 sowie den unveränderten Standard-FX.

30 manuelle Fälle plus sechs echte 112-s-Generate-Verläufe über drei Worlds
und Seeds 1234/42. Sämtliche vollständigen Ausgaben, gehörten Ereignisse und
Quellbelegungen stimmen zwischen 37/512-Frame-Caps auf demselben 10-ms-Main-
Raster exakt überein: 53.449.200 Stereo-Frames verglichen. Tatsächliche
zugelassene Hz, endliche Ausgabe und maximal 3 globale / 2 WOODLAND-Quellen
werden geprüft. Alle Generate-Fälle sind im letzten ganzen Sample-Sekunden-
Fenster ohne künstliches Clear exakt still. Nicht hörbare oder unzulässige
Ersatznoten werden nicht in die Auswertung geschmuggelt.

## Was die Welten tatsächlich trennt

| World | Tonkörper im Kandidaten | Musikalisches Zeitverhalten | Urteil aus Code und Messung |
|---|---|---|---|
| COAST | Weiche siebenfach harmonische Sinusfläche; langsamerer Einsatz, gleichmäßiger gehaltener Pegel | Zwei gegenläufige Außenrollen um einen gemeinsamen Ton, verbundene Bewegung und gemeinsame Ziele | Eigenes fließendes Verhalten; im gehaltenen Material nahe an HIGHLANDS |
| WOODLAND | Einmal angeregter, frei abklingender Körper mit acht harmonischen Moden; obere Moden verlieren Energie schneller | Zwei lange, überwiegend tiefere Stimmen; eine bleibt, die andere antwortet | Deutlich andere natürliche Abklinggestalt und engere Registerwirkung; lange Töne bewahren |
| HIGHLANDS | Gefilterter Reed-/Sinus-Tonkörper, weicher Lufteinsatz; schnellerer Einsatz als COAST | Weiter Akkord, gehaltener gemeinsamer Ton, deutliche Quellenpause und Wiederkehr | Eigenes Phrasenverhalten, gehaltene Klangfarbe derzeit COAST-ähnlich |

Beim gemeinsamen tiefen D3 (146,83 Hz), Color 0,5 und trocken:

| Messung | COAST | WOODLAND | HIGHLANDS |
|---|---:|---:|---:|
| RMS bei 3–8 s, vor Hörgain | −30,94 dBFS | −34,37 dBFS | −30,03 dBFS |
| Pegeländerung von 2–3 zu 9–10 s trotz gehaltenem Key | +0,37 dB | −11,58 dB | +0,59 dB |
| Anteil harmonischer Energie oberhalb des Grundtons im Messfenster | 5,33 % | 10,65 % | 6,45 % |

Der RMS-Abfall ist eine Materialwirkung, keine fehlerhafte kurze Note.
WOODLAND darf nach Key-up natürlich weiterklingen. Die gemeinsamen Töne und
die harmonische Nähe sind Teil des Instruments; sie garantieren nicht die
gewünschte Unterscheidbarkeit der Worlds.

Über die drei gemeinsamen Tonhöhen beträgt die mittlere Kosinusähnlichkeit
der 16 harmonischen Amplitudenprofile COAST/HIGHLANDS trocken 0,99871 und
mit FX 0,99845; nur die Obertöne betrachtet 0,98536/0,98361. Das zeigt sehr
ähnliche gemessene Profile an diesen Tönen. **Es bedeutet nicht „99,9 % gleich
gehört“ und ersetzt keinen Blind-Hörvergleich.** Einschwingverhalten, Phase,
Restgeräusche, Dynamik und längere Entwicklung sind damit nicht vollständig
abgebildet. Der gemeinsame FX-Bus macht die Tonkörper nicht automatisch
eigenständiger; er erhält die gestalterische gemeinsame Umgebung.

Generate bei Activity 0,5, Messzeit 1–81 s, zwei Seeds:

| World | Bestätigte Starts | Zeit ohne aktive Quelle | Gehörtes Frequenzregister |
|---|---:|---:|---:|
| COAST | jeweils 11 | 6,9–7,1 % | 110,0–587,3 Hz |
| WOODLAND | jeweils 11 | 5,6–6,3 % | 123,5–220,0 Hz |
| HIGHLANDS | jeweils 11 | 33,0–34,8 % | 123,5–587,3 Hz |

„Ohne Quelle“ heißt nicht zwingend still: der gemeinsame Raum kann noch
ausklingen. HIGHLANDS' längere Quellenpausen sind geplant und hier kein
Audioaussetzer. Die gleichen Startanzahlen sind kein gleicher Score; die
beobachteten Abstände, Register, Rollen und Halteverläufe unterscheiden sich.

## Offene Produktgrenzen

Die Algorithmen für Delay/Grains/Freeze und weitere Makros sind eingebaut.
Eigene Menüregler und dauerhafte Speicherung dieser neuen Makros/Loops sind
im vollständigen Hörkandidaten noch nicht angebunden; das ist ausdrücklich
ein offener Integrationsumfang. Der frühere Drive-Encoder ist im reduzierten
Product-Profil absichtlich inaktiv, auch wenn das neue DSP eine eigene weiche
Sättigung bereitstellt. Diese Grenze darf bei einem „fertigen Instrument“
nicht als vollständig bedienbares/speicherbares Effektpaket verkauft werden.
Nature selbst besitzt bereits seinen vorhandenen Regler und Scene-Wert.

Die tatsächlichen H743-IRQ-/Stack-/DAC-/Kopfhörer-/Lautsprechertests und
persönliche Hörabnahme bleiben offen. Ein grüner ARM-Build misst keine
physische Ausgabe. Scene-Journal bleibt OFF; kein Merge, Retarget oder Flashen.

## Empfehlung zur vierten World

**Zuerst die drei vorhandenen Worlds schärfen.** COAST und HIGHLANDS sollen
auch als manuell gespielte gehaltene Töne erkennbar verschiedene Materialien
haben. WOODLANDs bestätigte lange Resonanz bewahren. Eine vierte World ist
anschließend sinnvoll, wenn ein neuer Tonkörper und ein eigenes Zeitverhalten
eine belegte gestalterische Lücke füllen. Der zusätzliche Ort allein liefert
keine klangliche Eigenständigkeit.

Ein denkbarer späterer Entwurf wäre GLACIER mit weich aufblühenden klaren
harmonischen FM-Körpern. Das ist eine **ungetestete Idee**, keine bestehende
oder gehörte World. Vor einer Umsetzung wären Eigenständigkeit, harmonische
Ratios, Hörabnahme und die einfache Bedienung ausdrücklich zu klären.
Diese Prüfung aktiviert keine vierte World und beschließt keine Umbenennung.

## Hördateien und Reproduktion

`AMBIENT_Worlds_Trocken` und `AMBIENT_Worlds_Mit_FX`, je 55 s, als MP3/WAV:
COAST 0:00, WOODLAND 0:18,5, HIGHLANDS 0:37. Gleicher Ton D3, gleiche Eingaben,
Nature aus. Pro World 18 s, dazwischen 0,5 s Pause. Ein fester Hörgain pro
World aus dem trockenen 3–8-s-Fenster auf ungefähr −26 dBFS RMS, mit demselben
Faktor auch in der FX-Datei: COAST +4,94, WOODLAND +8,37, HIGHLANDS +4,03 dB.
Damit verrät bloße Lautstärke die World weniger schnell; das natürliche
Abklingen wird nicht eingeebnet. Nur redaktionelle 40-ms-Schnittkanten, keine
Kompression, AGC oder externe Klangquelle. Alle vollständigen MP3s wurden
zur Längenprüfung wieder zu je 2.425.500 Stereo-Frames decodiert.

```sh
python3 -m pip install numpy
python3 tools/review_world_identity.py build-world-audit
python3 tools/review_ui_idle.py build-world-audit/ui --fixture-only --finish-candidate --sanitize
python3 tools/review_ui_idle.py build-world-audit/product --fixture-only --sanitize
```

ffmpeg muss verfügbar sein. Vor/nach dem Klangaudit werden sämtliche
verwendeten DSP-/Header-/Rendererinputs gehasht. Vollständige gehörte
Ereignisse und Quellbelegungen bleiben als CSV erhalten. Exakte Kennzahlen,
Quell- und Audiodateihashes: [WORLD_IDENTITY_METRICS.json](WORLD_IDENTITY_METRICS.json).
