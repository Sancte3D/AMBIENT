# Field Ambience Project — Skill

Use this skill when the user mentions "Field Ambience", "shiftko", "v29", `~famStart`, "ambient device", playing cells, or any of the project-specific OSC endpoints (/fam/*).

## What this is

Standalone playable ambient instrument for the user (mr. mouse, German+English speaker).
Hardware: Pi 4 + Pico, 5 silicon cells + 5 modifier buttons, 4 encoders + small OLED, 2× 40mm speakers behind round front-panel grille.

**Architecture**: Browser HTML Panel ←WS:8765→ Python Bridge ←OSC:57120/57121→ SuperCollider engine

## Core Principle (most important)

**Field Ambience is an INSTRUMENT, not an automatic sound generator.**

Default state when `~famStart.()` runs: silent. The user plays cells.

Generative auto-progression and tonic drone are both **opt-in toggles**, default OFF. Don't reverse this without an explicit user request — it was a major redesign decision.

## Sound Constitution (rules, not taste)

**NEVER**: bells, fast random melodies, dissonance, harsh FM, aggressive resonance, fast arps, glitter, melodic chaos, modes that destabilize (Locrian removed), vibes that are harsh (`\sharp` removed), tempos > 90 BPM, brightness offsets > +2700 Hz.

**ALWAYS**: deep, dark, slow, harmonically stable, low-mid warmth, large reverb, scale-locked pitch, gradual modulation. Defaults: BPM=54, drive=0.18, brightness=-0.2.

**Constitutional safety in code**: every macro encoder has a safe range, every parameter clamps to musical values. The user must not be able to make ugly sound regardless of how they turn knobs.

## Files

Active in `/home/claude/field-ambience/v29a/`:
- `field_ambience_v29o.scd` — SC engine (~2470 lines)
- `field_ambience_panel_v29n.html` — HTML control panel (industrial mockup look)
- `field_ambience_bridge.py` — Python WebSocket↔OSC bridge
- `ROADMAP.md` — full status doc

The user uploads these in chats; you should view them when needed but don't read them all up-front.

## Engine layers (v29-o / v29-p)

- 5-voice detuned saw pad with tape-wow pitch drift (±2.5 cent, 0.18 Hz) + Haas stereo (8/14 ms L/R). Spawned only on cell-hold or in generative mode.
- Two-layer bass: SubBass (sine, 30-60 Hz, gentle 0.04 Hz breath at 8% depth) + DeepBass (sine + tri, 80-160 Hz, drive-controlled — Saw 3rd was removed because it sounded nasal/horn-like).
- Two-layer texture: rumble (low brown noise + LPF 220 Hz) + breath (filtered pink noise BPF sweep). Sparkle layer was removed for being too restless.
- JPverb premium reverb (FreeVerb2 fallback when JPverb unavailable).
- Drone helper: single quiet pad voice on tonic for tanpura-like reference.
- **v29-p**: Voices route dry → `~famDryBus`, wet → `~famVerbBus`. `famReverbMaster` mixes both, applies `masterVol` to the COMBINED signal, then LeakDC + Limiter (0.85 / 10ms) before Out 0. Pre-v29-p, dry bypassed masterVol → jack-detect/amp-shutdown couldn't fully silence the engine.

## Macro encoders (display-menu entries)

`KEY · MODE · CHORDS · MOOD · VOICE · OCTAVE · TEMPO · TEXTURE · SPACE · DEPTH · BLOOM`

All values safe-clamped. The encoder hardware is 4 physical knobs (Drive, Brightness, Display-controller, Volume). Other params via Display-controller scrolling through the menu.

Macro mappings:
- TEXTURE 0..1 → atmospheric noise bed amount
- SPACE 0..1 → reverb size + tail + wet (1.5s..8s t60)
- DEPTH 0..1 → SubBass amp + DeepBass amp + Pad release time
- BLOOM 0..1 → Pad attack + filter envelope amount + reverb wash boost

## Modifier buttons

- SHIFT — shifts cells 1-5 to scale degrees 6-10
- HOLD — toggles hold-mode (next cell tap holds the chord)
- DRONE — toggles tonic drone
- GENERATE — toggles auto-progression bed (the legacy ambient mode)
- CLEAR — releases all held chords

## OSC endpoints

Transport: `/fam/start /fam/stop /fam/auto`
Playing: `/fam/cell [degree] /fam/celloff [degree]`
Holds: `/fam/hold [degree, 0|1] /fam/holdoff [degree] /fam/holdclear /fam/holdflag [0|1]`
Mode: `/fam/generative [0|1] /fam/drone [0|1]`
Tonality: `/fam/key [midi] /fam/mode [0..5] /fam/prog [-1..4] /fam/vibe [0..3]`
Tempo: `/fam/tempo [40..90] /fam/mood [0..1]`
Voice: `/fam/voice /fam/midi /fam/padvoice [0..2] /fam/padoctave [0..2]`
Audio: `/fam/vol [0..1] /fam/jackdetect /fam/ampshutdown`
Macros: `/fam/drive [0..1] /fam/brightness [-1..1] /fam/texture [0..1] /fam/space [0..1] /fam/depth [0..1] /fam/bloom [0..1]`
Query: `/fam/query` → `/fam/state` reply with 30 fields

## Bracket-bug warning

The SC file is large (~2470 lines). When doing structural refactors, prefer **small targeted str_replace operations** and check bracket balance after each one. A single misplaced `}` in a deep block can be very hard to find. Use this Python check:

```python
import re
with open(path) as f: code = f.read()
# strip strings + comments cleanly
def strip(text):
    out = []; i = 0; n = len(text); in_s=False; in_lc=False; in_bc=False
    while i < n:
        c = text[i]; nxt = text[i+1] if i+1 < n else ''
        if in_bc:
            if c=='*' and nxt=='/': in_bc=False; i+=2; continue
            i+=1; continue
        if in_lc:
            if c=='\n': in_lc=False; out.append('\n')
            i+=1; continue
        if in_s:
            if c=='\\': i+=2; continue
            if c=='"': in_s=False
            i+=1; continue
        if c=='/' and nxt=='/': in_lc=True; i+=2; continue
        if c=='/' and nxt=='*': in_bc=True; i+=2; continue
        if c=='"': in_s=True; i+=1; continue
        out.append(c); i+=1
    return ''.join(out)
clean = strip(code)
print(f"diff={clean.count('{')-clean.count('}')}")
```

If diff != 0 after an edit, **immediately** identify which str_replace caused it (compare blocks before/after) and undo via reverse str_replace. Don't accumulate edits on a broken file.

## User communication style

- German + English mix ("pruedata" = Pure Data)
- Direct, blunt, gives concrete sound feedback ("klingt wie SpongeBob-Bootshupe")
- Wants critical thinking and honest pushback, not yes-man behavior
- Wants score-format feedback when iterating sound: Deepness/Epicness/Chaos (lower=better) /Harshness (lower=better) /Warmth/Movement/Emotional pull, each 0-10
- Reference recording: State Azure live performance — slow, deep, two-chord beds, modular-driven, harmonic stability
- Likes Sound Constitution approach: encode rules and forbids in code, not "make it sound nice"
- Prefers clean rebuilds over accumulated micro-patches when files get tangled

## Common requests + how to handle

- "der bass ist zu präsent" → reduce SubBass / DeepBass amp ranges in `~famSetDepth` map
- "klingt zu chaotisch" → reduce modulations, simplify layers, make defaults gentler
- "passiert zu viel" → disable side-chain ducks, reduce LFO depths
- "spielt automatisch" → confirm `~famGenerative=false` default; if user is hearing sound on `~famStart.()`, add `s.freeAll` at boot to clear stale synths from previous SC sessions
- "menü nicht klar" → labels max 8-10 chars for OLED, use musical names not technical (Major not IONIAN, Slow Bed not slow_bed)
- "hässlich darf nicht sein" → constitutional clamping: every encoder has a safe range, list which params can produce ugly sound and clamp them

## Task patterns

When the user says "weiter" or "weiter arbeiten" without specifics:
1. Look at ROADMAP.md unchecked items
2. Pick the highest-value low-risk one
3. Implement with small str_replace operations
4. Audit brackets after each
5. Output to `/mnt/user-data/outputs/`

When the user says "test feedback" with sound problem description:
1. Identify which engine layer is responsible
2. Propose specific parameter changes (not vague "make it warmer")
3. Implement
4. Output

When the user reports a bug:
1. Trace the code path from user-action to symptom
2. Identify the actual root cause (don't just patch symptom)
3. Fix with comment explaining the root cause
4. Confirm via bracket balance + targeted grep checks

## Don't

- Don't add new melodic layers without explicit request (Constitution: never melodic chaos)
- Don't add more macro encoders unless user asks (already have 6, more would clutter the menu)
- Don't use emojis or excessive markdown
- Don't suggest sound generation that requires audio samples I can't produce
- Don't reverse the instrument-mode default — generative is opt-in, not default

## Output convention

Always copy modified files to `/mnt/user-data/outputs/` and call `present_files` so the user can download. Keep summaries short and concrete. End with what was changed and what the user should test next.
