# SD33 — technische Parameterbereiche konsolidiert, 2026-10-19

SD33 ist softwareseitig geprüft. Die zwei konkreten früheren Befunde sind
bearbeitet: COAST-Activity wartet zusätzlich nach tatsächlichem Quellenende;
WOODLAND-Color färbt den Ausgang bei erhaltenem langen Saitenloop. Die weiteren
Kennlinien, Setter-/Quellenverträge und gerichteten Kombinationsprüfungen sind
vorhanden. Der aktuelle Audit hat keinen weiteren technischen Rest ergeben.
**Musikalischer Nutzen und gehörte Endwerte bleiben offen; SD33 ungecheckt.**

Technische Blöcke8→7:5 Software/Prüfung und2 Geräteabnahmen. Hörgruppe21→22;
24/54 Hauptpunkte geschlossen,30 offen. Dieser Abschluss ändert nur Dokumente
und Belege, keine Quelle, Kennlinie, Hüllkurve, Buffer oder Parametergrenze.
Lange WOODLAND-Ensembles bleiben gültig. Reference Default, Product Kandidat,
Spec0.10/SCN7, Journal OFF und synchroner Live-Save bleiben der Stand.

## Aktueller Vertrag und tatsächlicher Rest

| Bereich | Aktuelle technische Bedeutung | Vorhandener Beleg / verbleibende Entscheidung |
|---|---|---|
| Activity |0..1, default0,5. Kommende Abstände mit1,35−0,70a. COAST zusätzlich nach wirklichem Quellenende Partnerwait6/3/0s und Schlusswait10/5,5/1s, mit Entwicklung. Geplante Waits/Holds bleiben erhalten;0 spielt weiter | [Grundprüfung](SD33_ACTIVITY_CHECKPOINT.md), wirksame [COAST-Kalibrierung](SD33_COAST_ACTIVITY_CHECKPOINT.md). Nominelle Abstände sind keine Garantie gleicher tatsächlicher Dichte. Gehörte Ruhe/Figur weiterhin offen |
| Color |0..1, default0,5. Bowed-Cutoff0,8..1,2; Horn0,85..1,15. WOODLAND: bisheriger Defaultloop, Ausgang x+(2Color−1)(x−LP), LP bei2f/≤0,45SR; Neutral exakt vorheriger Ausgang.80-ms-Smoothing | [Colorbefund](SD33_COLOR_CHECKPOINT.md) und [lange Saite kalibriert](SD33_WOODLAND_COLOR_CHECKPOINT.md). Alte fast wirkungslose WOODLAND-Dampkurve ist überholt. Helligkeit, Pegelwirkung und störende Obertöne hören |
| Room |Ein gemeinsamer Raum, r0..1/default0,5. Nominales Feedback-T60=1,2+3,6r²s, Rücklauf5r×Enable. Amount80ms linear, Feedback120-ms-Zeitkonstante. Dry/Room-Enable40ms; kalt nach fertigem Dry, kein alter Tank bei Rückkehr | [Room-Kalibrierung](SD19_ROOM_CALIBRATION_CHECKPOINT.md), [Übergabe](SD47_ROOM_HANDOFF_CHECKPOINT.md), [Clear-Arbeit](SD47_ROOM_CLEAR_WORK_CHECKPOINT.md). Nominal-T60 ist keine gemessene vollständige Fahne. Finale Entfernung/Maskierung hören |
| Attack |Engine begrenzt SHAPE auf x=0,35+0,30a: Faktor0,5358867/1/1,8660657. Basis COAST0,30s, WOODLAND0,80s, HIGHLANDS0,45s; nur neue Quellenvorbereitung | [Attack](SD33_ATTACK_CHECKPOINT.md): vorbereitete/held PCM erhalten, zukünftige Starts/Clamp/Invalidinputs und volle Ensembles geprüft. Gewünschte Einsatzlänge hören |
| Release |Engine x=0,35+0,30r: Faktor0,65975392/1/1,51571643, nur neue Quellenvorbereitung. Bowed/Horn erfassen Releasekoeffizienten. WOODLAND behält natürlichen langen Loop und getrennten weichen Generate-Stop | [Release](SD33_RELEASE_CHECKPOINT.md): reale Belegung/Retirement und geordnete Übergaben. Keeper-Holds/Zeitpläne dürfen mit längerem Partnerrelease wachsen. Musikalische Ausklangwahl hören |
| Getrennte Volume/Nature |Master nach Room/Nature,120-ms-Zeitkonstante, kein Klangfarben-AGC. Nature bootmäßig0, Amount2-s-Zeitkonstante, eigener Target-/Fadepfad ohne Room-/Scorekopplung | [Parameterregister](PRODUCT_SOUND_SPEC.md), [gerichtete Kombinationen](SD35_PARAMETER_CHECKPOINT.md), [Geräuschwege](SD31_NOISE_PATHS_CHECKPOINT.md). Physische Regler-/Buttonzuordnung erst SD52 |

Die Product-Engine benutzt für Attack/Release die begrenzten Setter; die
allgemeinen Reference-SHAPE-Endpunkte0,125..8 und0,25..4 sind nicht die
Product-Reglerbereiche. Color ändert keinen Tonplan; Activity setzt keine
Sofortnote. Attack/Release ändern vorbereitete/held/releasing Envelopes nicht
rückwirkend. Release darf durch echte Belegungsdauer die nächste Zulassung
verschieben. Nature und Room sind separate Wege; Volume bleibt separat.

## Belege richtig zuordnen

Die alten Ensemble-/Kombinationsaufnahmen bleiben an ihre exakten Checkpoints
gebunden. Ihr gesamtes PCM wird hier nicht als frisch gerendert oder zum heutigen
Engine-/Roomfile bytegleich bezeichnet. Der JSON-Audit macht aktuelle gleiche
und abweichende Coredateien für jeden historischen SD33-/SD35-Beleg sichtbar.

Gegen den Release-Checkpoint PR159/Commit
`4cd07cd63c82b251320bce3be8676473a2ee9e3e` sind zehn der zwölf dokumentierten
Corequellen bytegleich: insbesondere World-Grammar, drei Quellen, SHAPE, Nature
und DSP. `engine_product.c` und `ambient_room.c` haben spätere Besitzer-/Target-/
Clearkorrekturen. **Alle sieben öffentlichen Setter** für Activity, Color,
Attack, Release, Room, Volume und Nature sind als vollständige Funktionen
bytegleich zum Release-Checkpoint. Aktuelle Room-Kennlinie/Routing sind daneben
direkt geprüft; spätere Room-/Quellenhandoffbelege behalten ihre eigenen Grenzen.

Die ursprüngliche Spec0.8-Activity- und Spec0.9-Colorvorbereitung sind als
Befunde vorhanden, nicht die endgültigen Kurven: COAST0.9 ersetzt den schwachen
Dichteeffekt, WOODLAND0.10 ersetzt die schwache Colorwirkung. Beide Änderungen
sind umgesetzt und mit früheren vollen Host-/gepaarten ARM-Builds dokumentiert.
Es gibt keinen weiteren durch diese Vorbereitungen offenen Programmierauftrag.

## Aktueller Software-/Buildbezug

Exakte Basis PR177: HEAD `fe3da78264a57963a5b8063cf158a7f6bb59f3ea`, Tree
`84f8a9d15fbbfd405a265e9fe59ebc4704815bb3`. Alle263 bestehenden Firmware-/
Header-/HAL-/Startup-/Linker-/CMakeinputs sind aktuell bytegleich.
Der unveränderte Runtimebezug ist PR176
`e2104094cfda3f4af563c608cf4c0f94430c945f`:
[Run734](https://github.com/Sancte3D/AMBIENT/actions/runs/39156738516) frisch
completed/success,27/27 Jobs erfolgreich, einschließlich vollständigem aktuellem
Hostrunner und H743-Productbuild. PR177 ist ein reiner Dokumentcheckpoint.

Der aktuelle normale Hostrunner enthält echte Product-/Stress-/Room-/Nature-/
Activity-Ranges-/Pluck-Color-/Phrasen-/Ownership-/Clear-/Scene-/Parameterchecks.
Der in [SD31](SD31_NOISE_PATHS_CHECKPOINT.md) tatsächlich geprüfte ARM-Beleg
bleibt quellenidentisch: Product-BIN196844B, SHA256
`7ba0c2db6914cdc581fcfc3ad72a9adbeb5712835ef9f5a5ef3dd1276f77c285`.
Kein neuer Gesamttest, ARM-Build oder Langzeitrender für diese Dokumentänderung.
Map/BIN/CI-Audit sind kein CPU-, kompletter Stack- oder Gerätetest.

Maschinenlesbar: [SD33_CONTROL_CONSOLIDATION_AUDIT.json](SD33_CONTROL_CONSOLIDATION_AUDIT.json).
Enthält263-Input-Manifesthash, alle historischen Metrikhashes samt aktueller
Quellenzuordnung, sieben Funktionshashes und den aktuellen grünen Buildbezug.

## Offene Arbeit nach SD33

SD33 ist nun Hörgruppe. Software insgesamt ist weiterhin nicht fertig:
SD41 benötigt die reale Storageabnahme vor Productintegration; SD47 hängt
ausdrücklich an SD41/SD48. SD51 hält die endgültigen Befunde fest, SD52 bindet
Display/Tasten/Encoder an. Nächster unabhängiger Software-/Prüfblock ist
**SD45 — lange reale Audioläufe und Recovery**, zuerst vorhandene Langzeitbelege
gegen seine restlichen Kriterien prüfen. Keine bereits grünen Kalibrierungen
allein zur Erzeugung eines neuen Prüfpunktes wiederholen.
