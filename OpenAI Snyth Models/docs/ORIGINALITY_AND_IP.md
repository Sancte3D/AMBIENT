# Originality and commercial IP record

## Provenance statement

The C implementation, test harnesses, preview score, documentation, names, and
display geometry in this directory were authored for the Sancte3D AMBIENT
project on 16 July 2026. No external source repository, sample pack, preset
file, MIDI sequence, score transcription, or recording was used as an
implementation input.

For the thirteen later display studies, user-supplied motion references were
reviewed only to identify general ideas such as negative space, radial motion,
particle flow, spectral streaks, and focus hierarchy. The translation and
measurements are recorded in `REFERENCE_MOTION_ANALYSIS.md`. Reference media
was not copied into this directory, traced, sampled, encoded into previews, or
used as device runtime input.

The ten later cinematic master loops were also authored in this directory.
Their renderer accepts no reference-image argument and contains no reference
pixels, extracted palette, coordinates, masks, optical-flow data, trained
weights, or copied frame timing. Full user-supplied sequences were reviewed to
separate general optical and temporal principles—such as a stable core plus
changing rays—from protectable source pixels and recognizable source forms.

The synthesis techniques employed—oscillation, phase modulation, delay
feedback, filtering, panning, envelopes, and procedural noise—are general
signal-processing techniques. Their specific combination, constants, state
layout, model behavior, and code here were authored for this package.

## Separation from references

- No artist or track name appears in implementation source, tools, or tests.
- No attempt was made to reproduce a melody, arrangement, exact timbre, preset,
  recording, or distinctive named sound from another work.
- Preview notes use a newly written interval walk to exercise the engines.
- Preview MP3s are rendered exclusively from this source and may be regenerated.
- Display concepts are code-generated from geometric primitives.
- Colour palettes and their anchor values were authored for this package.
- Human, animal, clothing, hand, and other recognizable reference silhouettes
  were deliberately replaced with non-figurative project-authored geometry.
- All 26 motion-study GIFs can be regenerated exclusively from the C renderer
  and the project-authored palette tables.
- All ten cinematic GIFs, stills, and frame sheets can be regenerated
  exclusively from `tools/render_high_quality_visuals.py`; the script does not
  read reference media.

Pillow and NumPy are external permissively licensed build tools used only to
render, analyse, and encode project-authored frames. They are not copied into
or linked with device firmware. See `BUILD_TOOL_LICENSES.md` for notices and
upstream license locations.

The automated audit is a guardrail, not proof of non-infringement. SHA-256
hashes establish which source version produced a review candidate but do not
replace authorship records or legal review.

## Commercial release checklist

- [ ] Confirm Sancte3D or the correct legal entity owns the commissioned output.
- [ ] Replace the provisional proprietary notice with counsel-approved terms.
- [ ] Run a trademark search for model, product, and customer-facing UI names.
- [ ] Choose a public product/folder name that does not imply affiliation with
      another company; the current folder label is an internal requested name.
- [ ] Keep all future samples outside this package unless their licenses,
      releases, attribution duties, and redistribution rights are documented.
- [ ] Require contributors to identify any external inputs before acceptance.
- [ ] Archive the source manifest, test log, rendered previews, and release tag.
- [ ] Have qualified counsel review the final shipped combination and marketing.

## Name note

Quick exact-name searching can reduce obvious collisions but is not a trademark
clearance. Model names should remain provisional until searched in the intended
countries and product classes.

## Scope

This record applies only to `OpenAI Snyth Models`. It makes no claim about any
other repository directory or pre-existing product asset and is not legal
advice.
