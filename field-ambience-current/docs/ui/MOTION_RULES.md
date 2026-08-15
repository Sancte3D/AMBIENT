# AMBIENT — UI motion rules

Binding for anything that moves on the 320 × 170 panel. Implemented in
`firmware-c-next/tools/ui_motion.{h,c}`, enforced by
`firmware-c-next/test/test_ui_motion.c`.

The short version: **the model never waits for the animation.** Everything
below follows from that.

---

## 1. Input reaches the model immediately

An encoder detent changes the value in the same millisecond the edge arrives.
Only the *presentation* eases toward it. An animation can therefore never
introduce latency, because nothing the user asked for is queued behind it — if
the screen is two frames behind, the sound is already where the hand put it.

Concretely on the bench (`tools/design_bench_pico.c`): the encoder and the
button are on GPIO interrupts, not polled. A full frame takes **29 ms** on the
wire at 32 MHz, and a polled loop simply loses every detent that arrives during
a blit — on a fast turn that is roughly every other step. That is the classic
"latency fail", and it is a wiring decision, not an animation decision.

## 2. Transitions are retargeted, never queued

A second detent during a snap does not wait, replay, or accumulate. It moves
the target and the tween re-eases from wherever it currently is. Turning fast
produces one continuous sweep, not a stutter of nine separate 200 ms
animations.

`ui_tween_to()` always restarts `from` at the *current* value, so a retarget is
continuous by construction. The test bounds the per-frame step across four
retargets in 60 frames.

## 3. Time-based, never frame-based

Every tick takes a real `dt` in milliseconds. A slow frame makes the motion
jump further, never run slower, so a stall in the SPI blit costs one frame
rather than stretching the whole gesture. 120 ms of animation must land in the
same place whether it arrived as 12 × 10 ms or 4 × 30 ms — that equality is a
test.

## 4. The curve follows the cause

| Cause | Curve | Why |
|---|---|---|
| direct manipulation — the value the hand is turning | `UI_EASE_OUT` | must leave instantly and settle gently. Any ease-**in** on a hand-driven value reads as lag: the first thing the eye checks is whether the screen moved when the finger did. Cubic-out opens at 3× speed. |
| system transition — level change, entering a group | `UI_EASE_IN_OUT` | nothing is chasing the hand, so it can accelerate out of rest and decelerate into rest. This is what makes a level change read as one object moving instead of two screens swapping. |
| cross-fade | `UI_EASE_LINEAR` | a curve on an opacity ramp reads as a flicker. |

Curves are anchored at both ends, monotonic, and never leave 0..1. An
overshoot on a value ring means the number is briefly wrong.

## 5. Three durations, nothing in between

| | | |
|---|---:|---|
| `UI_DUR_MICRO` | 90 ms | value orb, small corrections. Below ~80 ms an ease is imperceptible and only costs frames; above ~120 ms a hand-driven value feels rubbery. |
| `UI_DUR_MOVE` | 200 ms | the wheel snapping one step. |
| `UI_DUR_LEVEL` | 280 ms | a level change — the most pixels moved, and the only place the eye has to re-orient. |

## 6. Nothing snaps

The only instant placement allowed is the first frame after power-up
(`ui_tween_reset`), where there is no previous state to move from.

Tweens land *exactly* on their target and then report themselves done. An
asymptotic approach that never quite arrives leaves a sub-pixel drift, which on
this panel shows up as a shimmering edge that never settles.

## 7. Shapes may dissolve through each other; words may not

Geometry cross-fades cleanly, because two overlapping forms read as one object
deforming. Two labels at 50 % opacity in nearly the same place are not a
transition — they are two unreadable words stacked on each other. The level
change moves a heading from y=18 to y=12 *and* adds a second line at y=36, so
the overlap is guaranteed rather than occasional.

So text is sequenced, not cross-faded: the outgoing label is gone by 45 % of
the transition and the incoming one does not start until 55 %. There is a
moment with no label at all, and that is correct — during a level change the
label is exactly the thing that is no longer true.

## 8. The number is the model; the ring is the motion

In the value state the digits show the real value immediately, while the arc
and the orb ease toward it. They are deliberately not locked together: easing
the number would mean displaying a value the instrument is not actually at.

---

## Seeing it

```
cc -O2 -std=c11 -Iinclude -Itools -o /tmp/render_wheel \
    tools/render_wheel.c tools/ui_wheel.c tools/ui_draw.c tools/ui_motion.c \
    src/baked_font.c src/baked_font_data.c src/oled_draw.c \
    src/oled_color.c src/font_8x8.c -lm
/tmp/render_wheel /tmp/frames
python3 tools/make_motion_sheet.py /tmp/frames /tmp/frames/motion
```

Produces `MOTION_STRIPS.png` (every 2nd frame at a real 16 ms spacing — frames
bunching toward the right of a row *is* the deceleration) and three GIFs at
16 ms/frame. Both come out of the same tween code the device runs, so they are
device output rather than an illustration of an intention.
