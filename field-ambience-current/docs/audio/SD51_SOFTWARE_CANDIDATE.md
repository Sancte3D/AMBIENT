# SD51 — reproduzierbarer Softwarestand vor UI

Stand2026-10-20. Sound Candidate **0.10**, SCN7. Dies konsolidiert den
Softwarevertrag; der endgültige Sound-Freeze benötigt weiterhin gehörte
Quellen-/Parameterentscheidungen und SD48–SD50. SD51 bleibt deshalb offen.

Die verbindlichen aktuellen Werte stehen vollständig im
[Produktvertrag](PRODUCT_SOUND_SPEC.md): Register/Admit und Ownership,
alle drei Grammatiken einschließlich Seeds und Phrasengedächtnis,
Parameterregister/Kennlinien/Defaults, Gain-/Ausgabekette, Kontextwechsel/
Persistenz und Ressourcenvertrag. Der ältere Entwurfsabschnitt „BESTE VERSION“
der To-do beschreibt erste Suchbereiche; er ist kein zweites Presetregister.
Die veraltete Candidate-0.3-Zuordnung in der Aufgabenmatrix ist korrigiert.

| Vertrag | Aktueller Stand / maßgeblicher Nachweis |
|---|---|
| Konfiguration | `FAM_TARGET=h743`, `FAM_SOUND_PROFILE=product`, Release, `FAM_LCD_PANEL=1_9inch`; Reference bleibt ausdrücklich CMake-Default |
| Klang | COAST/Bowed, WOODLAND/lange Saite, HIGHLANDS/Horn; drei unterschiedliche begrenzte Grammatiken, gemeinsame Zulassung und Room, Nature optional/default0 |
| Festes Gain | Familien0,50/0,22/0,50; Send0,35; Master erst nach Room/Nature; kein neuer UI-Audiopfad |
| Makros | Activity/Attack/Release/Color/Room je0..1/default0,5; Nature0; Gerätvolume0,30, Host0,60; genaue Kurven und Glättungen im Parameterregister |
| Persistenz | SCN7, alte SCN5/6 nur Migration; kein Volume/Generate/Owner/Roomsnapshot; Seed0→0xA6B13E7D |
| Besitz/Harmonie | [SD11](SD11_PUBLIC_HARMONY_CHECKPOINT.md), [SD12](SD12_TAIL_HISTORY_CHECKPOINT.md); reale Quellen inklusive Vorbereitung/Release≤3, Pluck≤2 |
| Parameter/Pegel | [SD35](SD35_PARAMETER_CHECKPOINT.md); vorhandene separate SD33/34-/Gainbelege im Produktvertrag |
| Wechsel/Langlauf | [SD37](SD37_WORLD_TRANSITION_CHECKPOINT.md), [SD45](SD45_LONG_RECOVERY_CHECKPOINT.md); letzter Vor-UI-Head b29246fde0c66618bbfc25742f929dc1f362ebf6, CI736:28/28 erfolgreich |
| Begrenzte Arbeit | [SD47-Inventar](SD47_HOTPATH_INVENTORY.md); tatsächliche whole-IRQ-/Stackabnahme bleibt SD48 |
| Save | SD41-Writer, Recovery, bestätigte Scene-API und H743-Adapter vorbereitet/geprüft; Product-Live-Save weiterhin synchron, Journal OFF; Silizium-ECC-/Power-cut-Abnahme vor Aktivierung |

Die UI darf darauf aufbauen. Displayruhe ändert weder musikalische Zeit,
Quellenbesitz noch Targets; erstes Wake ist eine verbrauchte Bediengeste,
kein Note-on, Mute, Scene Save oder Parameterinkrement. Spätere tatsächliche
Bediengesten verwenden ausschließlich dieselben geprüften Audiooperationen.
Die vorhandene Menü-/Encoderzuordnung bleibt erkennbar; der im Product
entfernte Drive-Regler aktiviert keinen Legacyeffekt. Eine neue grafische
Gestaltung ist keine Voraussetzung dieses Softwarevertrags.

Der neue UI-Checkpoint enthält eine gehashte Momentaufnahme aller bestehenden
Buildinputs plus der beiden neuen UI-Dateien. Vier bestehende Inputs ändern
sich ausschließlich für UI/CMake/LED; alle259 übrigen bleiben bytegleich zum
Vor-UI-Head. Klangquellen, Engine, Grammatiken, Parameter, Scene, Audio-DMA,
HAL-Flash und Linker bleiben gleich. Das ist kein finaler Sound-Lock.

Unabhängig implementierbare DSP-/Bedienarbeit wird damit geschlossen;
SD41/SD47 enthalten weiterhin die ausdrücklich geräteabhängige Aktivierung/
Abnahme. Diese Grenzen werden nicht durch einen zusätzlichen Hosttest ersetzt.
