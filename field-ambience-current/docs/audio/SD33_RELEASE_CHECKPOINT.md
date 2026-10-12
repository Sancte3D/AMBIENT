# SD33 — Release und tatsächliche Stimmenübergaben

Stand 2026-10-12. Eine einzelne Prüf- und Hörvorbereitungseinheit; Attack bleibt
0,5. Firmware und Spec0.10 sind unverändert. SD33 und die Hörabnahme bleiben offen.
Exakte Basis ist PR159 `4cd07cd63c82b251320bce3be8676473a2ee9e3e`, Tree
`cc526a6e3e965302ccb6d33383ea91046688558b`. Alle 251 Core-, Header-, Startup-,
Linker- und CMakeinputs sind vor und nach der Prüfung bytegleich. PR159s
CI38286641272/run693 wurde frisch als completed/success geprüft.
Reference bleibt Default, Product Kandidat, Journal OFF und SCN7 unverändert.

## Parametervertrag und hörbarer Ausklang

Release 0/0,5/1 wird über 0,35+0,30*r an SHAPE übertragen. Der tatsächlich
ausgelesene Faktor beträgt 0,65975392/1/1,51571643. Eine neue Quelle nimmt ihn
bei der Vorbereitung auf. Bereits vorbereitete, gehaltene und ausklingende
Töne behalten ihren erfassten Release. Attack bleibt getrennt beim Faktor 1.

| World | Quellenparameter bei 0 / 0,5 / 1 | Gemessene Zeit von Stop bis alle Quellenplätze frei sind |
|---|---|---|
| COAST | Exponentielle Release-Zeitkonstante 0,594 / 0,900 / 1,364 s | 5,82 / 8,82 / 13,37 s |
| WOODLAND | Generierter weicher Stop 1,320 / 2,000 / 3,031 s | 1,32 / 2,00 / 3,04 s |
| HIGHLANDS | Exponentielle Release-Zeitkonstante 0,462 / 0,700 / 1,061 s | 4,76 / 7,22 / 10,91 s |

Die gemessenen Zeiten stammen aus denselben tatsächlichen Anfangstönen bei
Seed1234 mit Generate-Stop nach 4 s. Alle Note-on/off-Zeitpunkte, Frequenzen,
Velocities und Besitzer dieser Gegenproben sind über Releasewerte exakt gleich.
Die vollständige Stereoenergie nach Stop und die Belegungsdauer steigen bei
allen drei Worlds strikt mit Release. Die Messauflösung der Quellenfreigabe
beträgt 10 ms. Eine Zeitkonstante ist keine vollständige Ausklangdauer und kein
T60; die gemeinsame Raumfahne bleibt von der Quellenbelegung getrennt.

WOODLANDs langer Feedbackloop bleibt erhalten: nominales Feedback-T60
23,751/36,000/54,566 s; Interpolation und Dämpfung verkürzen den tatsächlichen
Decay. Manuelles Key-up lässt diese natürliche Schwingung stehen. Nur erzeugte
Tonrollen verwenden beim NoteOff den weichen Stop. Die 4-s-Stop-Gegenprobe ist
ein kontrollierter Vergleich und ersetzt keine lange musikalische WOODLAND-Phrase.

## Vollständige Engine und physischer Besitz

18 feste 112-s-Fälle (drei Worlds × zwei Seeds1234/42 × drei Releasewerte)
plus drei Livefälle, jeweils mit 64/512 Frames: 42 vollständige Render.
Sechs ganze Defaultkontrollen mit dem vorhandenen normalen Renderer und neun
32-s-Stop-Gegenproben jeweils mit 64/512 ergeben insgesamt 66 WAV-Render und
99,2 Minuten PCM. Alle kompletten Kandidaten sind in PCM, Events und Status
blockgenau gleich. Die sechs Defaultaufnahmen stimmen vollständig in PCM,
Events und gemeinsamen Statusfeldern mit dem bestehenden Renderer überein.
Das neue Statusformat ergänzt Memoryreturns und echte Quellenmasken.

Jeder der 250 bestätigten Starts in den 30 untersuchten Fällen ist mit einer
frischen Reservierung eines physischen DSP-Platzes im vorangehenden Tick
verknüpft. Quellenmasken zählen vorbereitete Starts und Releases. Vor jeder
erneuten Reservierung desselben Besitzers ist dessen vorherige Quelle tatsächlich
beendet; NoteOff allein genügt nicht. Kein Kontrolltick entfernt eine aktive
Quelle. Das belegt hier geordnete musikalische Übergaben, keine ISR-Qualifikation.

Release verändert zulässigerweise Belegungsdauer und Übergabezeit: in 80 s
Generate bei Seed1234 entstehen in COAST/HIGHLANDS 13/11/8 Starts und in
WOODLAND 11/11/11. Bei beiden Seeds bleibt die geordnete gemeinsame Note-on-
Folge einschließlich Rolle, Tonhöhe, Velocity und Phrasentyp identisch:
acht Starts bei COAST/HIGHLANDS, elf bei WOODLAND. Auch Livefälle erhalten
ihre gemeinsame Folge mit Default. Vollständige Zeitpläne, Statusdaten und
Keeper-Holds über verschiedene Releasewerte müssen nicht identisch sein.
Ein gemeinsamer gehaltener Ton kann während längerer Partnerausklänge länger
stehen; neue Partner warten auf die wirkliche Quellenfreigabe.

Live initial 0,5; bei 10/20/32/48/64/72 s auf 0/1/0,5/0/1/0,5. Setter startet
keinen Ton. Quellenbudget 3/2/3 inklusive Releases, NaN/Limiter 0,
Raw True Peak ≤−20,4 dBFS, Monoenergiequote ≥0,93058, DC ≤3,06e−8.
Stop nach 80 s, alle Besitzer spätestens nach 93,20 s frei; letzte Sekunde
jedes vollständigen Renders exakt still, ohne künstliches Stop-Endfade.
Maximaler benachbarter PCM-Sprung um Livewechsel 0,020325; keine Klick-Hörabnahme.

Separater tatsächlicher Product-PCM-Vertrag: 18 Änderungen an vorbereiteten,
gehaltenen und ausklingenden Tönen lassen die kompletten sechs Sekunden PCM
exakt stehen. Neun zukünftige Startfälle bei 64/512, sechs Clampfälle und
NaN/±Inf-Eingaben bestehen; Attack bleibt unverändert. Zusätzlich besteht der
vorhandene Product-Vertragscheck mit 12 Minuten tatsächlicher Musik, Besitz,
Kapazität, Storno, Block-/Invalidinput-Invarianz und Tuningtails.
Kein neuer kompletter Hostrunner oder lokaler ARM-Build wird behauptet: alle
251 Firmwareinputs entsprechen exakt dem grünen PR159 und seinem Host/H743-
Beleg. Keine Geräte-, CPU-, Runtime-, ISR-Stack- oder Hörabnahme aus diesen Tests.

## Drei kurze WAVs

Je 38,5 s: derselbe tatsächliche Abschnitt 2–14,5 s bei Release 0/0,5/1.
Ausgabepositionen 0–12,5 / 13–25,5 / 26–38,5 s; Stop jeweils nach den ersten
2 s des Abschnitts. Zwei Vergleichslücken von 0,5 s und 40-ms-Fades an beiden
Ausschnittkanten. Kein künstlicher Fade am Generate-Stop und keine Rekonstruktion
des Ausklangs. Jeder vollständige 32-s-Rohvergleich wird bis zur Stille geprüft;
die kurze Ausgabe darf eine sehr leise spätere Restfahne abschneiden.
Ein festes Gain je World für alle drei Teile, keine AGC oder Einzelnormalisierung.

| Datei | Gemeinsames Gain | LUFS / dBTP |
|---|---:|---|
| AMBIENT_COAST_Release_0_50_100_38p5s.wav | +8,8 dB | −23,4 / −13,5 |
| AMBIENT_WOODLAND_Release_0_50_100_38p5s.wav | +6,5 dB | −23,9 / −15,1 |
| AMBIENT_HIGHLANDS_Release_0_50_100_38p5s.wav | +5,7 dB | −23,5 / −16,6 |

RAWs lagen ausschließlich in OS/tmp und sind nach versiegelter Prüfung entfernt.
Exportiert sind drei WAVs, 60 vollständige Event-/Status-CSVs und JSON. Ganze
Aufnahmen, Exporte und sämtliche Quellen-/Auditinputs wurden abschließend erneut
geprüft. Die aktuelle Release-CI reproduziert diese Tools; die vorherige
Attackprüfung ist zur Wahrung ihres exakten Checkpoints auf PR159 gepinnt.
Alle Messwerte, physischen Tickets und Hashes:
[SD33_RELEASE_METRICS.json](SD33_RELEASE_METRICS.json).

Nächste einzelne Arbeitseinheit: SD34 Quellen-/Rollenpegel im tatsächlichen PCM
über Register und erlaubte Summen prüfen. SD33 bleibt für die musikalische
Bereichs- und Hörabnahme offen; weiterhin 24 Hauptpunkte erledigt und 30 offen,
darunter 12 Software-/Prüfblöcke und zwei Gerätequalifikationen.
