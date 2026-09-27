# Package 1 — String loop tuning

The damping FIR adds phase delay missing from the fractional delay length.
At default damping 0.42, rendered fundamentals measured -7.239 cents at
440 Hz, -14.440 at 880 Hz and -28.727 at 1760 Hz.

The read delay now subtracts the current damping coefficient from SR/f.
Feedback damping and read compensation follow the same 80 ms one-pole
smoother, including ringing voices. New notes start at the selected value.
This is a low-frequency phase approximation, not exact tuning at arbitrary
frequencies. No buffer growth or per-sample transcendental functions: one
extra float per voice (8 bytes total) and a few arithmetic operations.
Physical H743 DWT/deadline qualification remains pending.

Validation: `python tools/check_pluck_tuning.py` (numpy/scipy, host C compiler)
measures actual firmware PCM with a windowed fundamental spectral peak.
35 combinations: 60, 110, 220, 440, 880, 1320, 1760 Hz across damping
0, 0.2, 0.42, 0.6, 0.9. Worst absolute error 0.0545 cents (bound 0.3).
Six additional cases change damping both ways on ringing notes and verify
settled pitch. This does not qualify higher registers, transient click
perception, timbre or hardware timing.
Full `bash test/run_tests.sh`: exit 0, including 16372 synth-device checks
and 478860 effects checks, zero failures; hot-path lint clean.

Listening asset: Ambient_String_Stimmung_AB.wav, 9.5 s, stereo 44.1 kHz.
0–3.75 s before; 4.75–8.5 s after. Each plays 440, 880, 1760 Hz at identical
gain/default damping with a quiet pure target tone to reveal beating.
Dry deterministic rendering, no reverb/mastering. Peak 0.4624 before 16-bit
conversion. Host rendering, not a device recording.

Next: held-note retuning when switching Equal/Just. Wind/noise, remaining
synth character and FX-range review remain separate work items.
