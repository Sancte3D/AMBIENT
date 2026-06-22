# Engine Upgrade Analysis — Field Ambience (r18.42, post-LIQUID)

**Purpose:** sober look at the current sound engine, where the real leverage
is, and how the three external sources (DaisySP MIT, Surge GPL3,
SuperCollider GPL3) can — and cannot — help. Critical, not cheerleading.

## 1. What we have today, honestly

| Module | What it is | Honest assessment |
|---|---|---|
| `pad.c` (V1 warm-chorus) | 2 sides × (3 saw + 2 square) + SVF LP + Haas widening, 7-10 c detune | The accepted reference sound. Subtractive, no FM, no sub-sine fundament. Slightly hollow when soloed; "100x better" only when in the wash, drone, texture context. Don't break the timbre. |
| `texture.c` | brown noise → LP 220 Hz (body) + brown noise → BP 600 Hz swept (breath) | Body is the "Brumm" the user heard. Already off in the audition tools, but the engine default for it is still on. No air band (>2 kHz). |
| `reverb.c` (LIQUID, just landed) | pre-delay + 2 mod allpass diffusers + 8 mod combs + 4 mod output APs | New baseline. Genuinely lush. |
| `drone.c` | one pad-style voice, slow attack/release, follows key with portamento | Sustained but **timbre-static** — same saw stack as pad. No own pitch drift / breath. Sits flat in the mix. |
| `bass.c` (sub + deep) | sub-sine + tanh-saturated mid-band | Solid. Sub-sine + breath LFO; deep band has body. Underused at high song density. |
| `brain.c` | scale/mode → cell MIDI | Pure logic, fine. |
| `cells.c` | velocity from time-through-band, single note per press | Velocity scales amp linearly. No filter modulation. No micro-humanisation. Repeated taps are bit-identical → mechanical "feel". |
| inline noise generators in `tools/render_*` | wind, rain, waves, traffic, vinyl | Believable but parked in tools/. Not in the engine's signal path. |

## 2. The three external sources, gated by licence

| Source | Licence | Can link into product firmware? | What's actually useful |
|---|---|---|---|
| **DaisySP** | MIT ✅ | **Yes** — closed-firmware-compatible | `Limiter`, `Compressor`, `Decimator`, `Wavefolder`, `Overdrive`, `Chorus`, `Flanger`, `Phaser`, `Tremolo`, `Svf`/`OnePole`/`Biquad`/`Tone`, `DustNoise`/`ParticleNoise`/`FractalNoise`/`PinkNoise`/`BrownNoise`/`ClockedNoise`, `Resonator`/`String`/`Modal` (Karplus / physical modelling), `GranularPlayer`, `Looper`, drum synthesis. **No reverb.** |
| **Surge sst-effects, sst-filters, sst-waveshapers** | GPL3 ❌ | **No** — linking forces our firmware GPL3 (copyleft). Plus desktop SSE2. | Reference only. Algorithms (e.g. filter topologies) are not copyrightable; the code is. We can read and re-implement. |
| **SuperCollider + sc3-plugins** | GPL3 ❌ | **No** — same story. | Reference only. JPverb / Greyhole / GVerb algorithms can be re-implemented (already did Greyhole-style diffusion → LIQUID). |

**The honest takeaway:** the only library we can _link_ into the firmware
is DaisySP. The other two are textbooks we can read, not parts bins.

## 3. Where the real leverage is

Ranked by audible impact × engineering effort × risk to the V1 sound the
user accepted. Each item carries its own risk class — the V1 pad timbre is
"don't touch" territory, the master bus is safe to evolve.

### Tier A — pure-C, low risk, audible win (do these first)

| # | Win | Module | Why it matters | Risk |
|---|---|---|---|---|
| A1 | **Velocity → pad filter cutoff** | `pad.c` | Currently velocity only scales amp. A hard tap should also open the LP a touch (~+150 Hz at v=1.0). Tap _feel_ instantly improves; timbre at v=0 unchanged. | Low — symmetric around current behaviour at mid velocity. |
| A2 | **Micro-humanisation on every press** | `engine.c` `engine_note_on` | Repeated taps are bit-identical → mechanical. Inject ±0.3 % amp jitter, ±0.5 cent pitch jitter from a tiny LCG seeded per session. Imperceptible alone, makes a passage of repeats sound played. | Low — magnitudes are inside JND. |
| A3 | **Drone pitch drift (Eno trick)** | `drone.c` | Drone is currently pitch-static. Slow ±2 cent random walk + 0.04 Hz breath tremolo. Drone breathes, no longer "frozen oscillator". | Low — magnitudes within ensemble detune. |
| A4 | **Disable texture body by default** | `texture.c` or `engine_init` | The "Brumm" the user heard is the 220 Hz LP brown noise. Setting body weight to 0 by default (keep breath at 1.0) ends the complaint without changing the API. | Trivial — one line. |
| A5 | **Air-band texture layer** | `texture.c` | Add a third band: high-passed pink noise around 3–6 kHz at very low level. The "air" of dreamy/PS2 mixes that's currently missing. | Low — additive, off by default if it doesn't help. |

### Tier B — pure-C, medium risk, but high payoff

| # | Win | Module | Tradeoff |
|---|---|---|---|
| B1 | **Pad: optional sub-sine fundament** | `pad.c` | Adds a –12 dB sine at the root under each voice. Gives the saw stack a foundation it currently lacks. Risk: changes V1 timbre — needs A/B against the accepted reference. |
| B2 | **Filter drive (Surge idea)** | `pad.c` SVF input | Pre-filter tanh saturation. Adds analog character. Risk: same as B1 — affects the accepted V1. |
| B3 | **Stereo pre-diffusion** | new `src/diffuse.c` | Short modulated allpass chain in front of the reverb. Wider stereo image. Mostly invisible if reverb is high, audible when reverb is low. |

### Tier C — needs DaisySP linking, biggest engineering cost

| # | Win | Source | Engineering cost |
|---|---|---|---|
| C1 | **Master Limiter** | DaisySP `Limiter` | Currently we have tanh-soft-clip. A real lookahead limiter is louder + safer. Needs CMake to take a C++ object + a tiny C-shim. |
| C2 | **DaisySP `ParticleNoise` for ambience** | DaisySP Noise/* | Replaces our hand-built rain drops / vinyl pops with a tested module. Quality bump on the ambience layer the user already accepted as "fast gut genug". |
| C3 | **Karplus-Strong cell-tap option** | DaisySP `String` / `Resonator` | A second cell-voice flavour (per-world?) — plucked bells. Big new sound territory, but every world also needs its own voicing settings — design work, not just code. |

DaisySP's worth the integration cost, but it's a real sprint (CMake +
toolchain + maybe move the firmware build to a small C++ object). Best
done as its own PR, not as a side-effect of a sound pass.

### Tier D — explicitly not doing right now

| # | Why not |
|---|---|
| D1 | Replace `pad.c` with a different synth model | The user just rejected the soften'd profiles. Don't touch the timbre that won. |
| D2 | Drop in someone else's reverb | Just shipped LIQUID; revisit only if a real flaw surfaces. |
| D3 | Replace `texture.c` wholesale with DaisySP noises | Tier C-2 is the right path; don't conflate the modules. |
| D4 | Granular cell-voice | We already audited and rejected this direction ("Horror" v2 era). |

## 4. Recommendation

Two phases.

**Phase A (this session if the audition passes):** ship A1–A5 as one
coherent commit set. All pure-C, no link changes, low risk, audible
improvement to playing feel and ambience body. A/B render first so the user
hears it against the current reference.

**Phase B (separate PR, after Phase A is in):** decide on the DaisySP link
sprint (Tier C). If yes — limiter first (biggest safety/quality gain for
smallest design footprint), then ambience noises, then maybe a per-world
plucked voice. Each as its own PR so the user can stop at any point.

Phase B is the right place to spend the integration effort because by then
the engine sound is already better and we know exactly what we want
DaisySP for — rather than linking-then-deciding.

---

### Where the truth lives
- This file = recommendation snapshot, will go stale; trust `PROJECT_STATUS.md` for what's actually done
- Sound target: `tools/render_dreamy_warm.c` ("DAS IST ES")
- Pad source-of-truth: `src/pad.c` (V1 warm-chorus, do not break)
