# Ambient concept and continuity — 2026-09-11

## Verdict

The core idea fits the requested calm ambient instrument. The current device
is not yet a coherent implementation of that idea. A shared scale, softer
oscillators and atmospheric names do not by themselves connect its roles.
The user calls the latest Dusk direction better; that is useful feedback,
not acceptance of the complete instrument.

Our product criterion is a place the player can inhabit and influence, with
room for attention to come and go. This is consistent with Eno's original
ambient intention and his later description of imagined sonic landscapes;
neither requires literal nature simulation or removing every ambiguity.
Primary reference: [Eno, Ambient Music (1978) and On Land liner notes
(1982/1986)](https://static1.squarespace.com/static/5d4dcd89a70bc90001638861/t/5d9ca5a21e81bb3340e041d3/1570547107101/eno-ambient.pdf).
This is the product's chosen direction, not a universal rule for the genre.

## Findings that change the order of work

| Priority | Current evidence | Product consequence / decision |
|---|---|---|
| 1 — integration gap | `engine_set_synth` releases the Ambient sources and bass. `engine_render` replaces the entire Ambient dry/send mix with the selected core over 662 samples (~15 ms). Generation returns early in Character mode. | World + Character is currently an alternative instrument mode, not a continuous landscape with another playable colour. Make role continuity the next architecture unit. |
| 1 — concrete bug, fixed here | The same early return skipped recording physical activity in Character mode. Returning to Ambient could start an automatic note immediately after the player's phrase. | Record presence before the generation guard; preserve the existing ~8 s return pause. No new audio buffer, DSP state or per-sample work. |
| 2 — transitions | Held source ownership is cleared at Character selection. The short crossfade avoids a hard signal step but does not preserve a long musical phrase. Shared master FX can continue ringing. | Define held-note, source-release and FX-tail behaviour separately. A longer crossfade alone does not solve ownership or resource use. |
| 2 — harmonic limits | Generator pitch choices consider held notes, bass/pedal and estimated tails; live Ambient held-note retuning remains unresolved. | Keep the shared pitch context. Review retuning and old-tail collisions before claiming universal harmonious combinations. |
| 3 — effects hierarchy | Dream is the menu default. Its direct path passes through Tape, Chorus and Blur before delay/reverb returns are added (`ambient_effects.c`). | These are not just remote room returns. Audit their combined colour, especially with Mist's own chorus. Compare the dry source, room alone, then each added effect before changing all world presets. |
| 3 — duplicated roles | Mist overlaps the existing sustained bed; Dusk overlaps warm foundation voices; Dew overlaps existing plucks. Tide/Horizon/Mist all occupy sustained territory. | Six existing cores do not require six shipping modes. Retain a core only when its playable role and default sound justify it in context. |

The continuity measurement is explicit: with the same seed, the uninterrupted
Moss Fields dry control has three automatic onsets during 10–20 s. Switching
to unplayed Dusk at 10 s gives zero; the dry output at 12–20 s is exactly
silent. This is evidence of the architecture, not a demand to make an
unplayed solo synth sound. FX tails in the wet example must not be mistaken
for a continuing generated bed.

## What already supports the concept

- Shared modal pitch material and context-sensitive voice leading; no safe
  automatic pitch means a pause, not a forced event.
- Sparse onsets (at least 1.4 s apart), long notes, differently timed loops,
  a composer with quiet states, and player priority.
- A common master and effects room with separate sends; recent repairs to
  irregular wind, unwanted transposition and fundamental cancellation.

These are useful mechanisms, not listening approval. Pitch-class checks do
not measure clashes between partials, detuning, resonances or transformed FX
tails. Strict avoidance can also flatten modal differences. Long-form
variation needs a separate listening decision; a 28 s clip cannot establish it.

## Working product target

World supplies harmonic context, register, acoustic distance and a restrained
environment. Character supplies the playable timbre within that context.
The bed may breathe into silence; continuity means coherent state and
transitions, not an obligatory permanent drone. Space and dynamics should
agree while source identities remain different.

Use the existing bed / playable voice / optional foundation roles. Substitute
a selected core for an existing foreground role before considering another
layer. Natural textures remain optional, with lulls; they need not explain
every world literally. More reverb, a blanket low-pass or more simultaneous
voices is not a substitute for role design.

Next bounded unit: trace and budget a minimal bed + one selected foreground
core through source ownership, generator routing and both dry/send buses.
Specify which old voice stops and how its release survives. Check linker map
and peak block cost, including overlap and the full effect chain. Only then
implement the first coexistence candidate. Host timing cannot establish the
H743 deadline: on-device `peak_load < 0.60`, zero misses, stack/map and actual
outputs remain required. Do not copy the full engine or add a second FX tank.

Then: Ambient held tuning and musical handovers; one source/control family
at a time (Dew attacks, Glimmer Ratio/aliasing remain open); individual FX,
then curated combinations; five-world defaults and physical-output balance.
The existing SOUND_REVIEW_QUEUE.md retains the full inventory.

## This unit: reproduction and limits

`tools/review_world_continuity.py` builds the real C engine path and renders
three fresh-process cases in blocks no larger than 512 samples. The world
macros and Voice follow Moss Fields. Autoplay uses engine harmony; direct
Dusk notes do not exercise Bloom cell chord/hold gestures or hardware input.
Clock time derives from total rendered frames, not truncated block increments.

```sh
python3 tools/review_world_continuity.py --output /tmp/world-continuity
# Reuse a comparison library built by render_musical_review.build_engine:
python3 tools/review_world_continuity.py --engine /path/before.so --output /tmp/world-before
```

`WORLD_CONTINUITY_METRICS.json` records before/after renders. The wet case
plays Dusk at 11–14 s and 16–18 s, returns to Ambient at 20 s. First automatic
return onset moves from 20.000 s to 26.002 s. The dry cases have no manual
notes and remain unchanged by the presence fix.

The delivered **28 s system probe** contains 0–10 s Moss Fields autoplay,
10–20 s Dusk (two notes), and 20–28 s return to Ambient, including the restored
quiet interval. It uses one constant gain across the entire file, only edge
fades, no reference tone or external accompaniment: -25.0 LUFS, -12.4 dBFS
true peak. This exposes current behaviour; it is not a finished product demo.

Regression: the new `test_character_return_pause` fails on the old engine,
then verifies all six Characters suppress automatic onsets until the return
interval expires and resume afterwards. Full `bash test/run_tests.sh` passes:
16,478 device-path checks, 478,860 effects checks, realtime lint clean.
No claim of listening or hardware verification is made.
