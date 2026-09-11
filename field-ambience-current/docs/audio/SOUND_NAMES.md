# Sound type labels

User feedback rejects the old preset-like names. They mix genre, synthesis
technique, materials and fantasy imagery. Source headers trace several to
reference synth styles; that is not a coherent product vocabulary.

Use short descriptive working labels in the device menu and core metadata.
They identify the actual sound behaviour, not final branding or promised
acoustic emulation. Selection/voicing must earn a role before final naming.

| Device ID | Previous menu / metadata name | Working label | Actual behaviour |
|---|---|---|---|
| 1 | Acid / ACID RAIN | Resonant | Saw/square through an envelope-driven ladder filter |
| 2 | FM Glass / FM GLASS | Keys | FM key tone with attack/body contrast and gated sustain |
| 3 | Mist / CHORUS MIST | Ensemble | Detuned saw pad with chorus |
| 4 | Storm / ION STORM | Pulse | Sustained pulse-width texture |
| 5 | Orbit / GLASS ORBIT | Morph | Slow waveform morph, preserving the fundamental |
| 6 | Bamboo / BAMBOO CIRCUIT | Pluck | Short coupled amplitude/brightness decay |

`include/synth_names.h` supplies both menu and engine metadata. All labels
fit within eight characters; the existing value renderer also selects its
smaller font when needed. The related Orbit Rate control becomes Morph Rate.
Other control labels, musical parameters and world/voice/FX names are outside
this unit. Those vocabularies still need their own product review.

Internal enum values, symbols, filenames, ordering, scene format and parameter
IDs remain unchanged. Historical audio reports/render packs retain their
original names to keep comparisons traceable; use this table to translate.
Current SOUND_WORLD, product selection and review queue use the new labels.
This change does not declare any candidate ready to ship or change routing.
