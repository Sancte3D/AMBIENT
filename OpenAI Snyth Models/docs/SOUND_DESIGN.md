# Sound design architecture

## Design goal

The collection aims for spacious, emotionally clear ambient sound while staying
small enough for an embedded instrument. Beauty here comes from slow internal
movement, controlled imperfection, open interval fields, gentle transients, and
long but stable spatial memory—not from imitating an existing recording.

## Shared signal path

Each note passes through a model-specific analytic generator, an inexpensive
brightness stage, equal-power stereo placement, a modulated short delay, and a
four-line feedback spatial network. Controls are smoothed sample by sample so
encoder movement does not zipper.

```text
note events
    │
    ├─ model oscillator / resonator / feedback string
    │
    ├─ COLOR damping
    │
    ├─ WIDTH + model motion
    │
    ├─ MOTION short chorus
    │
    └─ SPACE four-line feedback field ── soft limit ── stereo PCM
```

Only one model context is active. Delay memory is shared, all random behavior is
seeded and deterministic, and the render callback allocates nothing.

## Musical behavior

The engine accepts frequency, velocity, and pan rather than embedding a scale
or melody. Harmony therefore remains owned by the AMBIENT brain and user input.
The preview renderer uses an original non-functional interval walk solely to
exercise the models; it is not part of firmware and is not a product song.

## Macro intent

| Macro | Low | High |
|---|---|---|
| COLOR | damped, nacreous, distant | open partials, sharper definition |
| MOTION | nearly frozen stereo | breathing detune and orbital movement |
| SPACE | close field | long, diffuse spatial memory |
| TEXTURE | pure and stable | air, drift, grain, or extra density |
| WIDTH | centered | broad equal-power placement and cross-delay |
| LEVEL | silent | calibrated master gain |

## Model differentiation

The models do not merely swap presets on one oscillator. They use distinct
excitation and decay structures: percussive resonators, phase modulation,
detuned pads, bounded stochastic accents, inharmonic partials, a feedback
string, open harmonic beds, drifting modulation ratios, clustered partials,
and a vowel-like field. The shared spatial path makes them feel like one
instrument family.
