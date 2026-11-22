> **Herkunft:** sechste Recherche des Besitzers (generative Ambient-Systeme, die stundenlang laufen), am 22.11.2026 ins Repo gelegt.
> Unverändert. Die Marker `citeturn…` sind kaputte Quellverweise; Quellen nicht geprüft. Zahlenwerte sind laut Text selbst Startwerte, keine Befunde.

# Generative Ambient Systems That Can Run for Hours or Indefinitely

## Core finding: long-running ambient is not “random music”

The strongest common pattern across genuinely generative systems is that **randomness is subordinated to a persistent musical state**. A successful engine does not repeatedly ask “what random note should happen next?” It maintains a slowly changing state—tonal center, register, density, timbral family, activity, spatial depth, event permissions—and randomness is used only to choose among options that are valid inside that state. This is explicit in Brian Eno’s description of *Reflection*: first define sonic materials and a “constellation of musical relationships,” then let algorithms permute those materials, then listen for days or weeks and retune the rules. The point is not maximal novelty but a bounded possibility space whose outputs remain recognizable as the same work. citeturn226274search0turn226274search4

That distinction also appears in the lineage from SSEYO Koan to Wotja. Intermorphic describes Koan as an “envelope of possibilities”: outputs vary, but the author specifies boundaries the music cannot cross. Current Wotja exposes multiple generator types and separate rule objects, with proprietary heuristics plus chance rather than chance alone. citeturn226274search13turn226274search12 SuperCollider and TidalCycles expose the same design vocabulary at a lower level: weighted choice, random walks, finite-state patterns, Markov transitions, Poisson distributions, deterministic seeds, degradation/removal of events, smooth noise and explicit time structure. citeturn685261search4turn685261search5

The practical conclusion for an autonomous embedded ambient device is therefore:

> **Build a hierarchical rule system first, then inject stochasticity into each layer at a controlled amount.**

The most useful hierarchy is approximately:

```text
hours
│
├─ macro-form / world state
│   ├─ tonal region
│   ├─ density target
│   ├─ brightness / spectral centroid
│   └─ allowed instrument families
│
minutes
│
├─ scene / phrase state
│   ├─ harmonic field
│   ├─ register
│   ├─ activity envelope
│   └─ spatial depth
│
seconds
│
├─ event generators
│   ├─ drones
│   ├─ notes / bells
│   ├─ textures
│   └─ environmental events
│
milliseconds
│
└─ DSP
    ├─ envelopes
    ├─ modulation
    ├─ filtering
    ├─ delay
    └─ reverb
```

This hierarchy is an engineering synthesis rather than a published description of any one commercial product, but it follows the verified architectures of systems that separate musical rules, event scheduling, playback and DSP. Generative.fm, for example, separates the generator packages from sample-loading libraries and uses Tone.js as the timing/audio layer; its generators expose a `schedule()` function that places the performance onto Tone’s transport. citeturn662780view0turn351605search1

## What the real systems actually reveal

### Brian Eno: Bloom and Reflection

What is publicly established about *Bloom* is relatively high-level. Eno and Peter Chilvers describe it as an “endless music machine”; the user can generate material by touching the interface or allow the system to play autonomously. The public material does **not** disclose enough code-level information to reconstruct its scheduler, probability distributions or exact voice-allocation logic, so claims such as “Bloom uses Poisson arrivals” or “Bloom is a Markov system” would be speculation. citeturn226274youtube14turn226274search2

*Reflection* is much more informative. Eno explicitly describes three stages: select sonic material and a musical mode; use algorithms to vary and permute the elements; then spend substantial listening time tuning materials and rules. Peter Chilvers further states that the software changes its rules according to time of day: morning harmony is brighter, it transitions through the afternoon, and late-night conditions reduce note density and slow the music. citeturn226274search0

This is important because *Reflection* is not merely “random playback.” It contains at least two time scales:

```text
short-term:
probabilistic generation of individual musical events

long-term:
slow external state = time of day
      ↓
changes rule parameters
      ↓
harmony / density / event rate / temporal character
```

That architecture solves one of the central problems of infinite music: **the engine itself changes its statistical behavior over time**. If the statistics are stationary forever, even a huge random space eventually develops a recognizable texture. A slowly changing meta-state prevents that without requiring conventional song form.

### Generative.fm

Generative.fm is especially useful because its generator implementation is open. The current documentation separates platform code from reusable generative-music libraries and from the actual generator collection. The generator repository contains individual pieces distributed as npm packages; the runtime activates a system, loads sample resources, calls a `schedule()` function, and then starts Tone.js’s shared transport. citeturn226274search5turn662780view0

Tone.js is not merely a timer wrapper. Its Transport supplies a global musical clock, schedules callbacks slightly ahead of playback, and passes the exact Web Audio timestamp into the callback so audio events can be started sample-accurately even though JavaScript callbacks themselves are not sample-accurate. citeturn351605search1turn351605search3 This separation is directly transferable to embedded design:

```text
RULE ENGINE
   │ produces future events
   ▼
EVENT QUEUE
   │ timestamp + voice + parameters
   ▼
AUDIO ENGINE
   │ sample-accurate execution
   ▼
DSP
```

Do **not** run all generative decisions directly inside the audio interrupt. Compute decisions at control rate and push timestamped events into a queue. This is the embedded equivalent of Tone’s transport/audio-clock separation.

The Generative.fm repository is under MIT, and individual packages such as `piece-zed` declare MIT as well. Tone.js is also MIT. citeturn907431view2turn907431view1turn935078search5 The sample situation requires more care: the sample repository contains a mixture of Alex Bainter’s recordings, public-domain Freesound material and external libraries such as VSCO2, VCSL and Sonatina. Therefore the **software license must not be assumed to cover every audio file**; each audio-source license needs to be followed independently. citeturn935078search3

### Endel

Endel documents an architecture closer to an adaptive control system than to a conventional composition. Its engine receives contextual inputs such as time of day, natural-light level, heart rate, weather and motion; a central logic layer maps those inputs onto predefined soundscape logic; sound layers, modulation and effects are then recombined or altered in real time. Endel states that the sound logic and source elements themselves are designed by its sound team rather than invented autonomously by an AI model at runtime. citeturn226274search9turn226274search10

That distinction is useful:

```text
bad interpretation:
sensor → AI invents arbitrary music

closer to documented Endel:
sensor
  ↓
state estimation
  ↓
parameter mapping
  ↓
pre-authored generative rules
  ↓
sound layers / modulation / effects
```

For an embedded product, this is attractive because state estimation can remain computationally trivial. A sensor value does not need to synthesize music itself; it can simply move a handful of high-level parameters such as `density`, `brightness`, `pulse_rate`, `reverb_send`, or `harmonic_state`.

### Koan and Wotja

Koan/Wotja provides the clearest conceptual precedent for constrained autonomous composition. Intermorphic explicitly describes each generated work as operating inside author-defined boundaries. Wotja’s current engine has several generator types plus rule objects rather than a single universal randomizer. citeturn226274search13turn226274search12

The transferable lesson is to **separate generators by musical role**. A drone generator should not use the same probability law as a bell generator, and neither should use the same rules as a texture generator.

A useful internal model is:

```text
DroneGenerator
    continuous / very slow
    changes every 20–180 s

ToneGenerator
    sparse point events
    changes every 3–45 s

TextureGenerator
    semi-continuous
    changes every 5–90 s

RareEventGenerator
    extremely sparse
    changes every 1–20 min
```

The system then combines them under one global density budget instead of allowing every generator to operate independently.

## The rule system I would actually implement

A strong architecture needs four nested stochastic processes rather than one.

### Macro-state

Update every roughly **5–30 minutes**.

State example:

```text
CALM
OPEN
DARK
ACTIVE
SPARSE
TRANSITION
```

Use a constrained Markov matrix, not independent random selection:

```text
          Calm  Open  Dark  Active  Sparse
Calm      .50   .25   .10   .05     .10
Open      .30   .35   .10   .15     .10
Dark      .20   .10   .45   .05     .20
Active    .25   .20   .05   .35     .15
Sparse    .35   .10   .15   .05     .35
```

This prevents absurd jumps such as `SPARSE → ACTIVE → SPARSE` every few seconds. Markov-based choice is directly supported by systems such as TidalCycles, while SuperCollider includes finite-state and weighted-random pattern primitives. citeturn685261search5turn685261search4

Add minimum dwell times:

```text
CALM    >= 4 min
DARK    >= 6 min
ACTIVE  >= 2 min
SPARSE  >= 3 min
```

This is critical. A state machine without dwell constraints often produces perceptually twitchy behaviour even if its transition matrix looks reasonable.

### Harmonic state

Do not generate arbitrary MIDI notes. Maintain a small **pitch-field object**.

Example:

```cpp
struct HarmonicState {
    int root;
    uint16_t pitch_mask;
    int register_low;
    int register_high;
    float consonance_bias;
};
```

Example field:

```text
D Dorian:
D E F G A B C
```

But events should select **roles**, not merely scale degrees:

```text
anchor       45%
color        25%
upper_extension 15%
neighbor     10%
rare_tension  5%
```

Then constrain transitions:

```text
anchor → any consonant role
color → anchor / color / neighbor
rare_tension → anchor only
```

This creates local grammar.

A particularly effective embedded approach is to precompute 8–16 pitch-field tables rather than calculate advanced harmony at runtime.

### Event timing

Pure uniform random intervals sound artificial because every interval inside the range is equally likely.

For independent sparse events, start with an exponential waiting-time model:

\[
\Delta t = -\frac{\ln(1-u)}{\lambda}
\]

where `u` is a uniform pseudo-random value and `λ` is expected events per second.

Example:

```text
λ = 0.1/s
mean gap = 10 s
```

But do not use a stationary Poisson process for everything. Realistic long-form ambience benefits from a **time-varying λ**:

\[
\lambda(t)=\lambda_0 \cdot D(t)\cdot S(t)
\]

where:

```text
D(t) = slowly varying density envelope
S(t) = current macro-state multiplier
```

Example:

```text
λ0 = 0.07 event/s
D(t) = 0.5 ... 1.4
CALM multiplier   = 0.7
ACTIVE multiplier = 1.6
SPARSE multiplier = 0.25
```

SuperCollider directly includes `Ppoisson`, but the more important design point is that Poisson timing should be **one ingredient**, not the entire composition. citeturn685261search4

For ambient systems I would usually prefer a **renewal process with a refractory period**:

```cpp
gap = min_gap + exponential(lambda);
```

For example:

```text
min_gap = 4 s
mean stochastic component = 9 s

effective mean ≈ 13 s
```

This immediately prevents rapid accidental retrigger clusters.

### Density budget and collision control

Every generator requests permission from a global budget.

```cpp
bool request_event(Event e)
{
    if (activeVoices >= MAX_VOICES)
        return false;

    if (densityScore + e.cost > targetDensity)
        return false;

    if (timeNow - lastSimilarEvent[e.class] < e.cooldown)
        return false;

    if (spectralOccupancy(e.band) > bandLimit[e.band])
        return false;

    return true;
}
```

This is one of the most important differences between **controlled emergence** and “random soup.”

Suggested starting costs:

```text
drone       0.45
texture     0.30
melodic     0.20
bell        0.15
noise event 0.10
```

Typical total budget:

```text
SPARSE   0.5–0.9
CALM     0.8–1.4
OPEN     1.2–1.8
ACTIVE   1.6–2.3
```

These numbers are **engineering starting values**, not findings from Eno, Endel or Generative.fm.

## Three embedded architectures worth building

### Sample-based engine

```text
                 ┌──────────────┐
                 │ Macro State  │
                 └──────┬───────┘
                        │
             ┌──────────▼──────────┐
             │ Event Generators    │
             │ drone / tone / fx   │
             └──────────┬──────────┘
                        │
                  Event Queue
                        │
        ┌───────────────▼───────────────┐
        │ Sample Voices + Pitch/Gain    │
        └───────────────┬───────────────┘
                        │
                   Filter / Delay
                        │
                     Reverb
                        │
                      DAC
```

Recommended if **natural timbre matters more than minimal flash usage**.

Representative design target for a 48 kHz stereo Cortex-M7-class platform:

```text
polyphony       6–12 voices
sample format   16-bit mono
sample RAM      streaming preferred
control rate    50–200 Hz
audio block     16–64 samples
```

If 10 minutes of mono 48 kHz/16-bit material were stored uncompressed, it would require about 57.6 MB, so a small embedded product should either stream from external flash/SD, use short source grains, lower-rate material where appropriate, or use compressed storage with decode overhead.

A strong strategy is to store **20–80 short source assets** and vary:

```text
start point
playback rate
octave
gain
filter
pan
envelope
reverb send
reverse flag
```

rather than storing hours of finished ambience.

### DSP/synthesis engine

```text
State Machine
     │
     ├─ Oscillator Bank
     ├─ Noise Generator
     ├─ Resonator
     ├─ Modal / Karplus voices
     └─ Envelopes
            │
         Filters
            │
     Modulated Delay
            │
          Reverb
```

Best when flash is very limited.

Representative target:

```text
Flash samples        < 1 MB
synthesis voices     4–8
LFOs                 4–12
control rate         100 Hz
reverb delay memory  ~50–250 kB
```

The main risk is **timbre fatigue**: a compact synthesis engine may offer enormous note-level variation yet still reveal the same oscillator/filter character after 30 minutes.

Therefore use multiple excitation types:

```text
sine / triangle
filtered noise
impulse → resonator
short wavetable
sampled transient
feedback delay
```

### Hybrid engine — preferred

```text
                 GLOBAL STATE
                     │
     ┌───────────────┼────────────────┐
     ▼               ▼                ▼
Samples          Synth voices     Noise/textures
     └───────────────┬────────────────┘
                     ▼
               Density Manager
                     ▼
                Spatial Mix
                     ▼
              Delay + Reverb
```

This is the best balance for an autonomous ambient object.

I would start with:

```text
4 sample voices
3 synth/resonator voices
1 continuous texture voice
2 FX sends

maximum actual concurrency: 6–8 voices
```

Let samples provide recognizable material and synthesis provide endless fine variation.

For a 480 MHz-class Cortex-M7 at 48 kHz, a sensible design goal is to keep average DSP well below the CPU limit—roughly **<50% steady-state CPU**, leaving margin for worst-case voice overlap, storage interrupts and control logic. That figure is an engineering safety target, not a benchmark for a specific processor.

## DSP, reverberation and modulation

DaisySP is particularly relevant for an embedded product because the library is explicitly MIT-licensed and intended for embedded audio DSP. citeturn935078search7 Mutable Instruments’ STM32 firmware is also MIT, although its AVR projects are GPLv3 and hardware files use a different license. Those categories must not be mixed when evaluating reuse. citeturn935078search8

Mutable Instruments’ Clouds source contains an MIT-licensed reverb implementation by Émilie Gillet, making it a useful reference when investigating compact embedded reverberation. citeturn935078search6

For this application I would favor an FDN/Schroeder-style topology rather than convolution:

```text
input
 │
 ├─ pre-delay
 │
 ├─ diffusion allpass
 │
 ▼
4–8 delay feedback network
 │      ▲
 │      └─ damping LPF
 ▼
stereo decorrelation
 │
 wet/dry
```

Starting ranges:

```text
pre-delay        5–40 ms
decay            3–18 s
high damping     2–8 kHz equivalent LPF region
wet              15–55%
modulation       extremely slow
delay modulation ~0.05–0.5 Hz
```

Those are **heuristic ambient-design ranges**, not parameters copied from Mutable or DaisySP.

Avoid modulating every parameter independently. Instead create a handful of slow latent variables:

```text
mood        = low-pass random
air         = low-pass random
distance    = low-pass random
activity    = low-pass random
```

Then derive several parameters from each:

```text
air →
    filter cutoff
    reverb HF damping
    noise brightness

distance →
    dry level
    pre-delay
    LPF
    stereo width
```

Tidal’s use of smoothly changing Perlin noise is a useful conceptual analogue: slowly varying correlated randomness produces coherent motion whereas independent random values create parameter jitter. citeturn685261search5

## Determinism, repetition control and long-run validation

A generative product should be reproducible during development. SuperCollider explicitly documents deterministic pseudo-random streams and reproducibility via seeds; resetting a seed recreates the same random sequence. citeturn685261search1turn685261search2 Tidal similarly treats its pseudo-random patterns as deterministic functions of logical time. citeturn685261search6

Therefore store:

```cpp
master_seed
session_seed
generator_seed[NUM_GENERATORS]
```

Prefer independent RNG streams per subsystem. Otherwise adding one new random call to the reverb can accidentally alter every later note choice.

A robust seed architecture is:

```text
masterSeed
   │
   ├─ hash("macro")   → RNG A
   ├─ hash("harmony") → RNG B
   ├─ hash("notes")   → RNG C
   ├─ hash("texture") → RNG D
   └─ hash("fx")      → RNG E
```

This makes debugging dramatically easier.

For repetition control, do not try to store the entire event history. Store short signatures:

```cpp
struct EventFingerprint {
    uint8_t voice;
    uint8_t pitchClass;
    uint8_t registerBand;
    uint8_t durationClass;
};
```

Maintain the last 32–128 fingerprints and penalize candidates that recreate recent `n`-grams.

Example:

```text
candidate probability
        × 0.15 if exact event occurred in last 8 events
        × 0.35 if same pitch/register pair occurred in last 16
        × 0.55 if same instrument occurs >3× in last 10
```

This is preferable to simply forbidding repetition because some recurrence is musically necessary.

The testing programme should be automated.

**One-hour test:** verify crashes, clipping, voice exhaustion, event clusters and obvious loops.

**Eight-hour test:** inspect state occupancy, long silent periods, event-density drift, repeated event signatures and spectral drift.

**Twenty-four-hour test:** verify numerical stability, RNG/state rollover, resource leakage, storage robustness and macro-state balance.

Log at least:

```text
timestamp
macro_state
harmonic_state
event_type
pitch
velocity
pan
duration
active_voice_count
RMS
peak
spectral centroid
```

Useful metrics are:

\[
\text{event density}=\frac{N_\text{events}}{\text{minute}}
\]

\[
\text{state occupancy}(s)=
\frac{t_s}{t_\text{total}}
\]

\[
\text{repeat ratio}_n=
\frac{\text{repeated n-grams}}{\text{all n-grams}}
\]

plus moving RMS, spectral centroid, crest factor, stereo correlation and inter-event-interval distribution.

Do not optimize these toward one universal “ideal” value. The correct ranges should ultimately come from the reference tracks you plan to measure. The generator should expose these quantities as controllable parameters so those later measurements can become target envelopes rather than subjective adjectives.

The main failure modes are predictable:

- **Too much entropy:** independent randomness in pitch, time, timbre and space produces incoherent noise.
- **Too little entropy:** fixed loops with cosmetic parameter randomization become recognizable quickly.
- **Stationary statistics:** the exact events never repeat, but the texture feels unchanged after ten minutes.
- **No density arbitration:** individually reasonable generators collide and create accidental crescendos.
- **No refractory periods:** several sparse events occasionally cluster into conspicuous bursts.
- **Too many states:** the engine changes character constantly and loses identity.
- **Too few states:** the piece has identity but no long-term development.
- **Global RNG only:** reproducibility breaks whenever implementation details change.
- **Constant reverb:** every state occupies the same perceived space, weakening macro-form.

The strongest architecture is therefore neither a playlist nor an unrestricted procedural composer. It is a **slowly changing stochastic ecosystem**:

```text
LONG-TERM STATE
      ↓
defines probability distributions
      ↓
ROLE-SPECIFIC GENERATORS
      ↓
constraint + density arbitration
      ↓
timestamped event queue
      ↓
sample/synthesis voices
      ↓
correlated slow modulation
      ↓
shared spatial DSP
```

That model is much closer to the publicly documented logic behind *Reflection*, Koan/Wotja, Endel and open generative frameworks than a conventional “random note generator.” citeturn226274search0turn226274search12turn226274search9turn662780view0

For an embedded implementation, the **hybrid architecture** is the strongest starting point: several independently seeded role generators; a six-state slow macro machine; a compact harmonic-state graph; renewal/Poisson-style event timing with minimum gaps; one global density controller; 6–8 maximum concurrent voices; short samples plus simple resonant synthesis; and a lightweight MIT-licensed reverb implementation derived from a library such as DaisySP or compatible Mutable Instruments STM32 code. DaisySP’s code is MIT, and Mutable’s relevant STM32 code is MIT, while the latter repository’s AVR and hardware portions carry different licenses and must be reviewed separately. citeturn935078search7turn935078search8turn935078search6