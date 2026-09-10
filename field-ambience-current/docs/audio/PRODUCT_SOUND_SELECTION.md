# Product sound selection — review before further expansion

This is a product/architecture assessment grounded in the current sources,
not a claim to have listened to and approved every sound. Existing code and
passing DSP tests are not reasons to ship an engine. User feedback explicitly
rejects abrasive, tubular and generic science-fiction character.

## Selection rule

A shipped sound must contribute a distinct useful role, start musically at
its default settings, remain controllable across its intended register and
fit the shared room/mix. It must be worthwhile on the physical controls and
outputs. A characterful texture can be valid without being purely harmonic;
an unintentionally unstable pitch or abrasive default cannot be excused as
character. Reject duplication and effects used to conceal a weak dry source.

## Six-core assessment

| Core | Product role / recommendation | Evidence and unresolved acceptance |
|---|---|---|
| Mist | Primary candidate: warm sustained ensemble | Detuned saws, low-pass and stereo chorus. Must justify its distinction from Ambient Pad/PADsynth; test mono and avoid doubling chorus in shared FX. Not yet listening-approved. |
| Orbit | Primary candidate: slowly evolving harmonic tone | Phase-related waveform morph with slow movement, distinct from ensemble width. Test useful morph range and upper-register harmonics after the cancellation fix. |
| Bamboo | Supporting candidate: sparse woody events | Coupled LPG brightness/amplitude decay gives a distinct transient role. Judge against String/Guembri, including whether its independent mono mode is necessary. |
| FM Glass | Supporting candidate: restrained tonal accents | FM Index/Body interaction corrected. Keep only if useful at ordinary settings without excessive filtering/reverb; Ratio changes and aliasing remain unresolved. |
| Acid | Rework candidate: expressive dark reed/bass | Current design emphasizes resonant filter attacks, accent and drive. Subtractive synthesis is useful for ambient, but this particular default/identity must earn its role rather than survive as an imitation. Compare against Ember/Bass/Horn before deciding. |
| Storm | Highest-risk candidate: redesign or drop from primary palette | Current implementation explicitly pursues an aggressive hoover/stab, detuned saw/PWM stack, short attack and a velocity-dependent pitch bend. These conflict with the requested defaults and overlap other subtractive voices. Keep code for comparison; no commitment to shipping it unchanged. |

These are priorities and acceptance hypotheses. Nothing is silently removed,
renamed or remapped in saved scenes by this document. Before replacement,
compare the proposed role with the existing source at matched loudness in a
short musical phrase. Do not add another engine to fill a naming slot.

## Architecture finding — instrument continuity

engine_set_synth releases generated/held Ambient sources and bass, clears
active source tracking and selects a mono core. The render blend replaces
Ambient; generation paths explicitly return while s_synth_tgt > 0. These
six cores are alternative instrument modes, not simply timbres selected for
a melody above the existing Ambient bed.

For this product, prefer a continuous bed + one selected foreground role +
optional foundation as the design target. The existing Ambient architecture
already represents these roles. First determine which native timbres belong
in that foreground; reserve separate solo modes for a demonstrated benefit.
This does NOT authorize blindly running all current cores/voices together:
map CPU, voice and buffer costs, ownership, transitions and effect sends
before implementing coexistence. Reuse or substitute a role, not add a layer.
Do not assume that six mono engines equal six usable ambient instruments.

## Effects: hierarchy before more modes

| Priority | Effect | Product criterion |
|---|---|---|
| Foundation | Dark reverb / shared room | Depth without persistent tonal ringing, washed-out attacks or low-mid accumulation; coherent dry/send balance. |
| Supporting | Delay | Space and sparse repetition; bounded feedback, usable timing and old-tail behaviour through harmonic changes. |
| Source-dependent | Chorus / detune | Ensemble when the source benefits. Mist already has chorus: check combined phase cancellation and pitch spread. |
| Optional colour | Tape / Age | Restrained bandwidth/drive/drift. Hiss and wow must not become unavoidable features of every world. |
| Optional gesture | Blur / reverse swell | A clear musical use beyond making everything diffuse; preserve intended pitch, onset timing and control responsiveness. |
| Accent | Shimmer | Occasional register lift, not a permanent treble layer; assess source-pitch interaction and feedback. |
| Curated combinations | Dream Chain | Evaluate as a musically chosen combination; technical availability of the full chain is not a reason to make it the strongest/default product sound. |
| Reference path | Bypass | Must remain available for honest source/FX evaluation. |

The nine DSP modes may remain useful internally while the product exposes a
smaller, clearer set of musical choices. Do not change IDs/scene formats as a
side effect of a sound review. Width/Tone/Level and source sends must be judged
alongside each effect, not as disconnected knobs.

## Acceptance and next bounded unit

For each candidate: distinct role, useful defaults, intended low/mid/high
register, soft/normal/hard input, sustained/released sound, relevant extreme
controls, mono/stereo, room and one competing source. Verify signal behaviour
and then obtain listening feedback. Files <=30 seconds; no diagnostic sine in
a musical example. Hardware listening and H743 DWT/memory remain release gates.

Next: a bounded Storm review to test whether it can offer a restrained,
pitch-stable sustained texture distinct from Mist/Orbit. If that role is not
convincing, recommend removing it from the primary palette instead of polishing
its aggressive preset. FM Ratio/aliasing and Ambient held-note tuning remain
open functional tasks; this product decision changes their prioritization,
not their status.

Source anchors: firmware-c-next/src/v2/engines/engine_*.c;
include/synth_controls.h; src/engine.c (engine_set_synth, engine_render and
generation guards); src/menu.c (SYNTH_NAMES, FX_NAMES); docs/SOUND_WORLD.md.
