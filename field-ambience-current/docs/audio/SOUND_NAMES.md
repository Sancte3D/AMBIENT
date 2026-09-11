# One ambient world: places and atmospheres

The user clarified that names should evoke an imagined ambient environment
in English. The interim Resonant/Keys/Ensemble/Pulse/Morph/Pluck labels were
the wrong direction. Avoid synthesis terminology, genre tags and unrelated
fantasy compounds in the sound selection. Build a shared vocabulary of light,
air, water and distance, with room for the listener's own interpretation.

## Product language

WORLD establishes a place: Alps, Open Sea, Fjords, Moss Fields or Desert.
CHARACTER replaces the Synth menu label and offers an atmospheric colour:
Dusk, Glimmer, Mist, Tide, Horizon or Dew. Ambient remains the existing default
selection. Examples such as Fjords + Mist or Desert + Dusk are the intended
mental combination, not new presets or a new combined display screen.

| Device ID | Original / interim label | Atmosphere | Imagery and sound-design target |
|---|---|---|---|
| 1 | Acid / Resonant | Dusk | Fading warmth near the ground; rounded, dark body, gentle arrival, no sharp filter chirp |
| 2 | FM Glass / Keys | Glimmer | Small distant reflections; restrained, clear accents with space between them |
| 3 | Mist / Ensemble | Mist | Soft air obscuring a landscape; gradual entry, diffuse width, stable harmonic centre |
| 4 | Storm / Pulse | Tide | A slow mass of water moving through space; weight and breathing movement without obvious wobble |
| 5 | Orbit / Morph | Horizon | An open line in the distance; extended tones that change colour without losing their centre |
| 6 | Bamboo / Pluck | Dew | Small rounded droplets on a quiet surface; brief delicate events, clean space around their tails |

These are evocative targets, not claims of literal wind, rain or instrument
simulation. The names share an environment without requiring a fixed story.
Do not manufacture weather noise to make a metaphor literal. A source must
still work dry, have a distinct role and fit the common harmonic/FX space.

## Implementation and honest limits

include/synth_names.h supplies menu and core metadata. Names are <=7 ASCII
characters, fitting the existing value renderer. The Character label uses
the existing smaller label font. Enum values, scene IDs/order, parameters,
saved data format and all DSP are unchanged by this naming pass.
Historical reports and render tools retain their measured-version names;
the table above translates them. Current product docs use the atmosphere names.
Technical control names such as FM Ratio remain for the later control-language
review; avoid replacing precise controls with ambiguous poetry wholesale.

Architecture still matters: selecting one of the six cores replaces the
Ambient render mode and suspends its automatic generation. It does not add
that core above a continuing landscape bed. WORLD still supplies shared world
parameters, but continuous bed + chosen foreground remains an open routing/
resource design task in PRODUCT_SOUND_SELECTION.md. Naming cannot implement it.

Dusk's current resonant attack and uneven register response do not yet satisfy
its target. That is the next bounded sound revision. Dew's uneven attacks,
Glimmer's high-ratio behaviour and the other acceptance gates stay open.
The earlier output calibration remains intact; no new audio buffers/state.

Verification: full bash test/run_tests.sh passes after the label change
(16436 device checks, 478860 effects checks, zero failures). No DSP changed.
Next-review evidence from SUPPORT_LEVEL_METRICS.json: at velocity .5, dry
Dusk body RMS in the 1..1.4 s window is -25.44 dBFS at C3 and -42.06 dBFS
at C5 (16.62 dB difference). This quantifies the register issue; its cause
still needs a controlled filter/envelope test.
