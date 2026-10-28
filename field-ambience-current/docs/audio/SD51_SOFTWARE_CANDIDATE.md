# SD51 — aktueller Softwarekandidat0.12

Stand28.10.2026. [Produktvertrag](PRODUCT_SOUND_SPEC.md): Materialmodell2,
vollständige Effektsteuerung, korrigierte Raum-/Nature-Hierarchie und SCN8/JSS1-Sessions. Michalis'
„Besser“ ist positives V2-Klangurteil, keine pauschale Parameter-/Gerätefreigabe.
Der finale Freeze bleibt offen.

| Vertrag | Aktueller Stand |
|---|---|
| Build | H743/Product/Full-Sound/Session, Release; Reference CMake-Default, Kandidatenflags OFF |
| Tonkörper | COAST evenreiche Fläche, WOODLAND langer angeregter Modalkörper, HIGHLANDS oddreicher Atemton |
| Gain | Familien0,50/0,32/0,50, Send0,35, Master nach Room/Nature; keine Quellen-/Master-AGC, begrenzte Raum-Rückwegkalibrierung und Nature-Absenkung |
| Effekte | Zehn Float-Ziele + Freeze/Plate in einer Main-/Audio-Transaktion, keine Encoderzuordnung |
| Persistenz | FXP1/GLP1 in fünf SCN8-Slots,5848B,8192-B-JSS1, echter yielding IRQ-/ECC-H743-Adapter |
| Recall | Targets/gestoppter Loop, keine Stimmen/Generate/Volume oder Freeze-Audio |
| Besitz | Drei reale Quellen inklusive Pending/Release, WOODLAND höchstens zwei; alte Hz/Tails geschützt |
| UI | Bestehende Eingaben/15-min-Ruhe/Wake, Scene-Status; neue Encoderbelegung zuletzt |
| Hierarchie | Beide Räume vor gemeinsamer Färbung; Grain0 deaktiviert Freeze; leere Capture blockiert nicht; sichtbare Ablehnungsgründe |
| Messung | Separater Bench mit startup-MSP-Paint/SAI-DMA-Fehlern/Savezählern und rein lesendem Snapshot |

[SOUND_CANDIDATE_INPUTS.json](SOUND_CANDIDATE_INPUTS.json) enthält aktuelle
eigene Runtime-/Header-/HAL-/Startup-/Linker-/CMake-Hashes und aus C ermittelte
Grenzen/Defaults. freeze_sound_candidate.py --check erkennt spätere Änderungen.
Vendor/Compiler ersetzt dieser eigene Quellhashvertrag nicht; tatsächlicher
ARM-Build/Audit bleibt nötig. Flags stehen vollständig im Produktvertrag.

SD51_SOFTWARE_INPUTS.json bleibt die unveränderte265-Input-Momentaufnahme
von0.10. Der volle vorherige Text steht im [0.10-Archiv](PRODUCT_SOUND_SPEC_0_10_ARCHIVE.md).
Historische Werte sind keine aktuellen Full-Sound-Werte.

Ohne echte Hörer/Hardware kein finaler Sound-Lock. Das neue Blindpaket hat
unbekannte A/B/C-Zuordnung, identische drei Register/Regler, zuerst trocken,
dann Standard-FX. Erzeugung bedeutet keine durchgeführte Blindabnahme.
SD41/47/48/49 benötigen das Gerät, SD53 alle zutreffenden Hör-/Gerätenachweise.
