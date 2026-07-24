# Aus dem Source-Paket entfernte Dateiklassen

Entfernt wurden nur für das Lernen und Portieren entbehrliche oder unnötig
große Bestandteile:

- alle `.git`-Historien und Git-Objektdaten;
- STM32CubeH7-Demovideos im AVI-Format;
- bereits kompilierte `.bin`, `.elf`, `.hex` und `.map`-Artefakte;
- DaisyExamples-Distributionsartefakte;
- mehrfach wiederholte SVD-Dateien in DaisyExamples;
- der große CMSIS-DSP-Testkorpus unter `Testing`.

Erhalten blieben:

- C/C++/Assembler-Quellcode;
- Header;
- Linkerskripte;
- CubeMX- und IDE-Projekte;
- Make- und CMake-Dateien;
- Beispiele und technische Dokumentation;
- Audio-Testassets, sofern sie innerhalb des STM32-Pakets benötigt werden;
- alle gefundenen Lizenz- und Notice-Dateien;
- ausgecheckte Submodule.

Die exakten Upstream-Commits ermöglichen bei Bedarf eine spätere
Rekonstruktion der ausgelassenen Dateien.
