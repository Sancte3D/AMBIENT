// =====================================================================
// FIELD AMBIENCE v29-o
// Sound Constitution: deep, dark, slow, harmonically stable, never ugly
// ---------------------------------------------------------------------
// Reference: State Azure performance style.
// Two-chord slow bed (e.g. E7 → A), 8-bar holds, large reverb,
// soft texture layer, dark filter default, warm drive default.
//
// Constitutional rules (from user):
//   NEVER: bright bell tones, fast random melodies, random notes outside
//          the scale, dry attacks above unity, harsh resonances, rapid
//          octave jumps, dissonant pitch jumps, melodic overload.
//   ALWAYS: slow envelopes, long decay, stable root, low-mid warmth,
//           soft filtering, large reverb, scale-locked pitch, few voices,
//           gradual modulation.
//
// Concrete v29-o changes vs v29-n:
//   1. Tempo default 72 → 54 BPM (less nervous)
//   2. Bar length 4 → 8 bars per chord (slower harmonic motion)
//   3. Default progression simplified to I-IV (two-chord pad bed)
//   4. AUTO mode: fixed progression, NOT Markov by default
//   5. Drive default 0.0 → 0.18 (warm body, never clean-digital)
//   6. Brightness default 0.0 → -0.2 (slightly darker)
//   7. Reverb t60 2.8s → 4.5s (larger, more cinematic space)
//   8. Pad filter ADSR back, but ULTRA slow & subtle (not the v29-j chaos)
//   9. NEW: TEXTURE layer — filtered brown noise atmosphere bed
//      (non-melodic, non-tonal, just air & body)
//  10. Safe ranges clamping on every encoder
// =====================================================================

(
// v29-m fix: disable audio input completely. Field Ambience generates
// audio, it doesn't record. This sidesteps macOS sample-rate mismatch
// errors when the system input device (mic, USB headset, etc.) runs at
// a different rate than the output device.
s.options.numInputBusChannels = 0;
s.options.sampleRate = 44100;
// Optional: increase memory for the JPverb / longer reverb tails
s.options.memSize = 65536;

s.waitForBoot({

	// v29-o BUGFIX: Free any synths that survived from a previous session.
	// Without this, re-running the patch (~CMD-Period not used, just re-eval
	// the file) leaves old voices playing forever, creating "automatic sound
	// without user action" since ~famRunning=false but the actual synth
	// nodes are still on the server.
	s.freeAll;
	~famPadVoices = [];
	~famSubBassVoice = nil;
	~famDeepBassVoice = nil;
	~famDroneVoice = nil;          // v29-o instrument-mode tonic drone
	~famTextureSynth = nil;
	~famVerbSynth = nil;
	~famCellTapVoices = ();
	~famCellHolds = ();
	~famHeldSynths = [];

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// LAYER C -- Voice SynthDefs
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// famCore: Stereo-FM voice, HiChord-inspired
	// v29-c: added verbOut + verbSend parameters for reverb bus routing
	// v29-i: famCore is the GLITTER LAYER -- bell-like FM for accents
	// over the warm pad. Keep it bright, short, and mostly in the upper
	// register so it doesn't compete with the pad fundamentals.
	// v29-k: added highpass at 600Hz and shorter decay envelope.
	SynthDef(\famCore, { |out = 0, verbOut = 0, verbSend = 0.18,
	                     freq = 440, amp = 0.25, decay = 3.0,
	                     modRatio = 3.5, modIndex = 2.5, pan = 0,
	                     detuneCents = 8, tremRate = 0.4, tremDepth = 0.08,
	                     delayTime = 0.28, delayFb = 0.35, delayMix = 0.25,
	                     hpCutoff = 600|
		var env, ampEnv, modEnv, mod1, mod2, car1, car2, sig, wet;
		var freqL, freqR;

		freqL = freq * (2 ** (detuneCents.neg / 1200));
		freqR = freq * (2 ** (detuneCents        / 1200));

		ampEnv = EnvGen.ar(
			Env.new([0, 1, 0], [0.015, decay], [2, -4]),
			doneAction: 2
		);
		modEnv = EnvGen.ar(
			Env.new([1, 0], [decay * 0.6], [-3])
		);
		env = SinOsc.kr(tremRate, 0, tremDepth, 1 - tremDepth);

		mod1 = SinOsc.ar(freqL * modRatio) * modEnv * modIndex * freqL;
		mod2 = SinOsc.ar(freqR * modRatio) * modEnv * modIndex * freqR;

		car1 = SinOsc.ar(freqL + mod1) * ampEnv * env;
		car2 = SinOsc.ar(freqR + mod2) * ampEnv * env;

		wet = CombC.ar(
			Mix.ar([car1, car2]) * 0.5,
			0.5, delayTime, delayTime * delayFb * 4
		);

		sig = [car1, car2] + (wet * delayMix);
		// v29-k: highpass to keep glitter out of pad fundamentals
		sig = HPF.ar(sig, hpCutoff);
		sig = sig * amp;
		sig = Balance2.ar(sig[0], sig[1], pan);

		Out.ar(out, sig);
		Out.ar(verbOut, sig * verbSend);
	}).add;

	// famPad: Gate-controlled voice for HOLD/DRONE with crossfade.
	// v29-c: added verb send
	SynthDef(\famPad, { |out = 0, verbOut = 0, verbSend = 0.25,
	                    freq = 440, amp = 0.08, gate = 1,
	                    modRatio = 2.0, modIndex = 1.2, pan = 0,
	                    detuneCents = 10, attack = 1.5, release = 3.5,
	                    lfoRate = 0.23, lfoDepth = 0.12|
		var env, lfo, mod1, mod2, car1, car2, sig;
		var freqL, freqR;

		freqL = freq * (2 ** (detuneCents.neg / 1200));
		freqR = freq * (2 ** (detuneCents        / 1200));

		env = EnvGen.ar(
			Env.asr(attack, 1.0, release, [2, -4]),
			gate, doneAction: 2
		);
		lfo = SinOsc.kr(lfoRate, 0, lfoDepth, 1 - lfoDepth);

		mod1 = SinOsc.ar(freqL * modRatio) * modIndex * freqL;
		mod2 = SinOsc.ar(freqR * modRatio) * modIndex * freqR;

		car1 = SinOsc.ar(freqL + mod1) * env * lfo;
		car2 = SinOsc.ar(freqR + mod2) * env * lfo;

		sig = [car1, car2] * amp;
		sig = Balance2.ar(sig[0], sig[1], pan);

		Out.ar(out, sig);
		Out.ar(verbOut, sig * verbSend);
	}).add;

	// famAir: sporadic high-register shimmer. Sine with slow LFO, high-pass implicit by freq choice.
	// v29-c: triggered rarely (every 10-30s) for breathing glints.
	SynthDef(\famAir, { |out = 0, verbOut = 0, verbSend = 0.45,
	                    freq = 1760, amp = 0.04, decay = 4.5,
	                    pan = 0, detuneCents = 5, lfoRate = 0.15|
		var env, freqL, freqR, car1, car2, sig, wobble;

		freqL = freq * (2 ** (detuneCents.neg / 1200));
		freqR = freq * (2 ** (detuneCents        / 1200));

		// Long soft envelope, slow attack for shimmer-in-from-nowhere feel
		env = EnvGen.ar(
			Env.new([0, 1, 0], [0.8, decay], [2, -3]),
			doneAction: 2
		);

		// Very slow pitch wobble (0.15 Hz)
		wobble = SinOsc.kr(lfoRate, 0, 0.004, 1);

		car1 = SinOsc.ar(freqL * wobble) * env;
		car2 = SinOsc.ar(freqR * wobble) * env;

		sig = [car1, car2] * amp;
		sig = Balance2.ar(sig[0], sig[1], pan);

		Out.ar(out, sig);
		Out.ar(verbOut, sig * verbSend);
	}).add;

	// famBass: gentle root support. Low-pass filtered, soft.
	// v29-c: triggered on chord changes only. Not ego, just foundation.
	SynthDef(\famBass, { |out = 0, verbOut = 0, verbSend = 0.05,
	                     freq = 65, amp = 0.15, gate = 1,
	                     attack = 0.8, release = 2.5,
	                     lpCutoff = 450, lpRes = 0.3|
		var env, osc, sub, sig;

		env = EnvGen.ar(
			Env.asr(attack, 1.0, release, [2, -3]),
			gate, doneAction: 2
		);

		// Triangle + subtle sine sub, LPF'd
		osc = Mix.ar([
			LFTri.ar(freq) * 0.5,
			SinOsc.ar(freq * 0.5) * 0.3
		]);
		sig = RLPF.ar(osc, lpCutoff, lpRes);
		sig = sig * env * amp;
		// Mono out, centered (bass sits center always)
		sig = Pan2.ar(sig, 0);

		Out.ar(out, sig);
		Out.ar(verbOut, sig * verbSend);
	}).add;

	// ===================================================================
	// v29-i: famPadCore -- the warm continuous pad voice
	// ===================================================================
	// One instance per chord tone (5 voices for a 5-note chord).
	// v29-i: Detuned saws + 24dB lowpass + slow filter modulation.
	// v29-j: + ADSR filter envelope + voiceMix parameter for character
	//        (warm/strings/brass)
	// Sustains forever via gated ASR envelope. Released on chord change.
	//
	// voiceMix parameter (0..2):
	//   0.0 = warm   (pure detuned saw, default)
	//   1.0 = strings (saw + narrow pulse, brighter top)
	//   2.0 = brass  (pulse-dominant, harder edge)
	// Values in between blend smoothly between the extremes.
	SynthDef(\famPadCore, { |out = 0, verbOut = 0, verbSend = 0.4,
	                        freq = 220, amp = 0.12, gate = 1,
	                        pan = 0, detuneCents = 8,
	                        attack = 1.5, release = 3.0,
	                        cutoffBase = 1100, cutoffMod = 600,
	                        cutoffRate = 0.08, lpRes = 0.18,
	                        chorusDepth = 0.0015, chorusRate = 0.35,
	                        // v29-j additions:
	                        voiceMix = 0.0,
	                        fenvAttack = 2.5, fenvDecay = 4.0,
	                        fenvSustain = 0.6, fenvAmount = 0.5,
	                        // v29-n: live brightness offset (lag-smoothed for click-free changes)
	                        cutoffOffset = 0,
	                        // v29-o-tuned: tape-wow halved (±2.5 cent instead of ±5)
	                        // because too much modulation at once felt restless.
	                        wowRate = 0.18, wowDepth = 0.0015|
		var freqL, freqR, freqLDrift, freqRDrift, sawL, sawR, sig;
		var ampEnv, fenvControl, cutoffL, cutoffR, chorus;
		var sawWeight, pulseWeight;
		var cutoffOffsetLag;
		var wowL, wowR;

		// v29-o: independent pitch drift per side (random phase ensures no
		// phasing artifacts between L/R). Two-LFO sum for non-periodic feel.
		// At wowDepth=0.003, max drift = 0.3% = ~5 cent. Subtle but tangible.
		wowL = (SinOsc.kr(wowRate, Rand(0, 2pi)) * wowDepth)
		     + (SinOsc.kr(wowRate * 1.37, Rand(0, 2pi)) * wowDepth * 0.5);
		wowR = (SinOsc.kr(wowRate * 0.91, Rand(0, 2pi)) * wowDepth)
		     + (SinOsc.kr(wowRate * 1.43, Rand(0, 2pi)) * wowDepth * 0.5);

		// Three detuned saws per side for thickness, plus tape-wow drift
		freqL = freq * (2 ** (detuneCents.neg / 1200)) * (1 + wowL);
		freqR = freq * (2 ** (detuneCents        / 1200)) * (1 + wowR);

		// Long ASR envelope, sustains at level 1.0 indefinitely
		ampEnv = EnvGen.ar(
			Env.asr(attack, 1.0, release, [\sin, \sin, \lin]),
			gate,
			doneAction: 2
		);

		// v29-j: ADSR filter envelope -- slow swell from low cutoff to full
		// Adds the "brewing" pad-comes-up character on top of LFO motion.
		// Hold stage (sustain) keeps the cutoff offset stable while gate=1.
		fenvControl = EnvGen.kr(
			Env.adsr(fenvAttack, fenvDecay, fenvSustain, 1.0, 1.0, [\sin, \sin, \sin]),
			gate
		);

		// v29-n: smoothed live brightness offset
		cutoffOffsetLag = Lag.kr(cutoffOffset, 0.3);

		// Slow filter modulation (LFO) + ADSR contribution + live brightness
		// Each side gets its own random LFO phase for stereo movement
		cutoffL = cutoffBase
			+ (SinOsc.kr(cutoffRate, Rand(0, 2pi)) * cutoffMod * 0.5)
			+ (fenvControl * cutoffMod * fenvAmount)
			+ cutoffOffsetLag;
		cutoffR = cutoffBase
			+ (SinOsc.kr(cutoffRate * 1.13, Rand(0, 2pi)) * cutoffMod * 0.5)
			+ (fenvControl * cutoffMod * fenvAmount)
			+ cutoffOffsetLag;

		// Tiny chorus-like delay modulation for stereo width
		chorus = SinOsc.kr(chorusRate) * chorusDepth;

		// v29-j: voiceMix crossfades between saw-heavy (warm) and pulse-heavy (brass)
		// At 0.0: 100% saw   | At 1.0: 50% saw + 50% pulse | At 2.0: 100% pulse
		sawWeight   = (1.0 - (voiceMix * 0.5)).clip(0, 1);
		pulseWeight = (voiceMix * 0.5).clip(0, 1);

		// Build oscillator stack per side
		sawL = Mix.ar([
			Saw.ar(freqL) * sawWeight,
			Saw.ar(freqL * 1.003) * sawWeight,
			Saw.ar(freqL * 0.5) * 0.5 * sawWeight,
			Pulse.ar(freqL, 0.45) * pulseWeight,
			Pulse.ar(freqL * 1.003, 0.40) * pulseWeight
		]) * 0.4;
		sawR = Mix.ar([
			Saw.ar(freqR) * sawWeight,
			Saw.ar(freqR * 0.997) * sawWeight,
			Saw.ar(freqR * 0.5) * 0.5 * sawWeight,
			Pulse.ar(freqR, 0.55) * pulseWeight,
			Pulse.ar(freqR * 0.997, 0.50) * pulseWeight
		]) * 0.4;

		// Apply lowpass with modulated cutoff (now per-channel for stereo motion)
		sawL = RLPF.ar(sawL, cutoffL.clip(80, 8000), lpRes);
		sawR = RLPF.ar(sawR, cutoffR.clip(80, 8000), lpRes);

		// v29-o: Haas effect — small fixed delay between L and R creates
		// massive perceived stereo width without affecting tonality.
		// Combined with the existing chorus delay modulation for "movement".
		// Haas range: 5-15ms is the sweet spot. We use 8ms base + chorus drift.
		sawL = DelayC.ar(sawL, 0.030, 0.008 + chorus);
		sawR = DelayC.ar(sawR, 0.030, 0.014 - chorus);

		sig = [sawL, sawR] * ampEnv * amp;
		sig = Balance2.ar(sig[0], sig[1], pan);

		Out.ar(out, sig);
		Out.ar(verbOut, sig * verbSend);
	}).add;

	// ===================================================================
	// v29-i: famSubBass -- low-end body
	// ===================================================================
	// Sine wave at chord root, two octaves below, gentle lowpass.
	// One instance per chord change. Sustains forever via gate.
	SynthDef(\famSubBass, { |out = 0, verbOut = 0, verbSend = 0.05,
	                         freq = 55, amp = 0.18, gate = 1,
	                         attack = 2.0, release = 2.5,
	                         lpCutoff = 200,
	                         // v29-o-tuned: gentler breathing — 0.04Hz (~25s cycle)
	                         // and only 8% depth so it's a presence, not a pulse.
	                         breathRate = 0.04, breathDepth = 0.08|
		var ampEnv, sig, sub, breath;

		ampEnv = EnvGen.ar(
			Env.asr(attack, 1.0, release, [\sin, \sin, \lin]),
			gate,
			doneAction: 2
		);

		// v29-o: breathing -- very slow LFO (0.07Hz = ~14 sec cycle).
		// Range: (1 - depth)..1.0, never silences the bass, just a "breath".
		// At breathDepth 0.18, the bass swells between 82% and 100% of its
		// nominal level. Subtle but makes the sub feel alive vs static.
		breath = SinOsc.kr(breathRate, Rand(0, 2pi)).range(1.0 - breathDepth, 1.0);

		// Sine + slight triangle harmonic for warmth without mud
		sig = SinOsc.ar(freq) + (LFTri.ar(freq * 2) * 0.08);
		sig = LPF.ar(sig, lpCutoff);

		sig = sig * ampEnv * amp * breath;
		// Mono center; small reverb send for "in-the-room" feel
		sig = sig ! 2;

		Out.ar(out, sig);
		Out.ar(verbOut, sig * verbSend);
	}).add;

	// ===================================================================
	// v29-o: famDeepBass — felt-bass body layer
	// Sits BELOW famSubBass on the same chord root.
	// SubBass = 30-60Hz pure sine (the "feel")
	// DeepBass = 80-160Hz body (the "warmth/punch you can hear")
	// Together they give State Azure-style massive low-end without mud.
	// ===================================================================
	SynthDef(\famDeepBass, { |out = 0, verbOut = 0, verbSend = 0.08,
	                          freq = 110, amp = 0.20, gate = 1,
	                          attack = 3.0, release = 4.0,
	                          lpCutoff = 280, lpRes = 0.10,
	                          drive = 0.05|
		var ampEnv, sig, driveLag;

		ampEnv = EnvGen.ar(
			Env.asr(attack, 1.0, release, [\sin, \sin, \sin]),
			gate,
			doneAction: 2
		);

		driveLag = Lag.kr(drive, 0.4);

		// v29-o-tuned: simpler harmonic content. Removed Saw 3rd (was nasal/honky
		// and made the bass sound like a boat-horn). Just sine + tame triangle.
		sig = SinOsc.ar(freq)
		    + (LFTri.ar(freq * 2) * 0.06);

		// Very gentle tube-style saturation (drive default 0.05 = barely audible)
		sig = (sig * (1 + driveLag * 1.2)).tanh / (1 + driveLag * 0.5);

		// Lowpass tightened (was 350, now 280) for less midrange honk
		sig = RLPF.ar(sig, lpCutoff, lpRes);

		// High-pass at 50Hz to avoid mud below the SubBass region
		sig = HPF.ar(sig, 50);

		sig = sig * ampEnv * amp;
		sig = sig ! 2;

		Out.ar(out, sig);
		Out.ar(verbOut, sig * verbSend);
	}).add;


	// Brown noise → gentle band-pass → slow stereo wandering → reverb send
	// No tones. No pitch. Just air, body, and "lo-fi space".
	// Sustained continuously from ~famStart to ~famStop.
	// =====================================================================
	SynthDef(\famTexture, { |out = 0, verbOut = 0, verbSend = 0.55,
	                         amp = 0.0, gate = 1,
	                         attack = 6.0, release = 6.0,
	                         bpCenter = 600, bpQ = 0.4,
	                         lfoRate = 0.04|
		var ampEnv;
		var rumbleL, rumbleR;
		var breathL, breathR;
		var sparkleL, sparkleR;
		var lfoBreath, lfoStereo, lfoRumble, lfoSparkle;
		var sigL, sigR;

		// Long ASR envelope -- sustains forever, fades softly on release
		ampEnv = EnvGen.ar(
			Env.asr(attack, 1.0, release, [\sin, \sin, \sin]),
			gate,
			doneAction: 2
		);

		// v29-o: Three-layer texture mimicking field-recording depth.
		// Each layer has its own LFO timing, creating non-synchronized motion.

		// LAYER 1: low rumble (distant traffic/wind feel)
		// BrownNoise → LPF 220Hz, ~constant amp, slight stereo width
		lfoRumble = SinOsc.kr(0.03, Rand(0, 2pi)).range(0.85, 1.0);
		rumbleL = LPF.ar(BrownNoise.ar(), 220) * 0.35 * lfoRumble;
		rumbleR = LPF.ar(BrownNoise.ar(), 240) * 0.35 * lfoRumble;

		// LAYER 2: mid "breath" (wind/movement, the main texture body)
		// Filtered pink noise, slow band-pass sweep around bpCenter, amp pulses
		lfoBreath = SinOsc.kr(lfoRate, Rand(0, 2pi)).range(0.5, 1.0);
		lfoStereo = SinOsc.kr(lfoRate * 0.61, pi/2).range(-0.25, 0.25);
		breathL = PinkNoise.ar();
		breathR = PinkNoise.ar();
		breathL = BPF.ar(breathL,
			(bpCenter * SinOsc.kr(lfoRate * 1.31, 0).range(0.7, 1.4)).clip(180, 1800),
			bpQ);
		breathR = BPF.ar(breathR,
			(bpCenter * SinOsc.kr(lfoRate * 1.43, pi/3).range(0.7, 1.4)).clip(180, 1800),
			bpQ);
		breathL = breathL * lfoBreath * 0.55 * (1 + lfoStereo);
		breathR = breathR * lfoBreath * 0.55 * (1 - lfoStereo);

		// v29-o-tuned: SPARKLE LAYER REMOVED. The "distant raindrops" felt
		// like something was always-just-about-to-happen, restless. Constitution
		// rule: ambience should be a still surface, not a perky layer.
		// Mix only rumble + breath now.
		sigL = rumbleL + breathL;
		sigR = rumbleR + breathR;

		// Soft global LPF (warmth, no fizz)
		sigL = LPF.ar(sigL, 4500);
		sigR = LPF.ar(sigR, 4500);

		// v29-o: amp lagged so side-chain ducking from /fam/cell etc. is smooth
		// (no clicks). Lag of 0.08s = gentle ramp on amp changes.
		sigL = sigL * ampEnv * Lag.kr(amp, 0.08);
		sigR = sigR * ampEnv * Lag.kr(amp, 0.08);

		Out.ar(out, [sigL, sigR]);
		Out.ar(verbOut, [sigL, sigR] * verbSend);
	}).add;


	// v29-f: SC's Saw.ar is bandlimited (PolyBLEP-style internally).
	// v29-i: now used ONLY as alternative motif voice, not as pad.
	SynthDef(\famSaw, { |out = 0, verbOut = 0, verbSend = 0.22,
	                    freq = 440, amp = 0.18, decay = 3.5,
	                    pan = 0, detuneCents = 12,
	                    lpCutoff = 2400, lpRes = 0.4,
	                    tremRate = 0.35, tremDepth = 0.06|
		var ampEnv, freqL, freqR, sawL, sawR, sig, trem;

		freqL = freq * (2 ** (detuneCents.neg / 1200));
		freqR = freq * (2 ** (detuneCents        / 1200));

		ampEnv = EnvGen.ar(
			Env.new([0, 1, 0], [0.04, decay], [2, -3]),
			doneAction: 2
		);
		trem = SinOsc.kr(tremRate, 0, tremDepth, 1 - tremDepth);

		sawL = RLPF.ar(Saw.ar(freqL), lpCutoff, lpRes);
		sawR = RLPF.ar(Saw.ar(freqR), lpCutoff, lpRes);

		sig = [sawL, sawR] * ampEnv * trem * amp;
		sig = Balance2.ar(sig[0], sig[1], pan);

		Out.ar(out, sig);
		Out.ar(verbOut, sig * verbSend);
	}).add;

	// famSquare: Bandlimited Pulse voice with slow PWM for hollow / flute character.
	SynthDef(\famSquare, { |out = 0, verbOut = 0, verbSend = 0.30,
	                       freq = 440, amp = 0.14, decay = 3.0,
	                       pan = 0, detuneCents = 8,
	                       lpCutoff = 2800, lpRes = 0.3,
	                       pwRate = 0.3, pwDepth = 0.18|
		var ampEnv, freqL, freqR, pwm, pulseL, pulseR, sig;

		freqL = freq * (2 ** (detuneCents.neg / 1200));
		freqR = freq * (2 ** (detuneCents        / 1200));

		ampEnv = EnvGen.ar(
			Env.new([0, 1, 0], [0.025, decay], [2, -3]),
			doneAction: 2
		);
		pwm = SinOsc.kr(pwRate, 0, pwDepth, 0.5);

		pulseL = RLPF.ar(Pulse.ar(freqL, pwm), lpCutoff, lpRes);
		pulseR = RLPF.ar(Pulse.ar(freqR, pwm), lpCutoff, lpRes);

		sig = [pulseL, pulseR] * ampEnv * amp;
		sig = Balance2.ar(sig[0], sig[1], pan);

		Out.ar(out, sig);
		Out.ar(verbOut, sig * verbSend);
	}).add;

	// famReverbMaster: premium reverb on bus.
	// v29-g: Auto-fallback to FreeVerb2 if sc3-plugins (JPverb) not installed.
	// JPverb-specific params (t60, size, high) are still accepted as ARGUMENTS
	// so external code (.set calls, ~famUpdateReverb) doesn't break, but they're
	// translated/ignored for FreeVerb path.
	~famHasJPverb = \JPverb.asClass.notNil;

	if(~famHasJPverb) {
		"JPverb available -- using premium reverb path.".postln;
		SynthDef(\famReverbMaster, { |in = 0, out = 0,
		                              t60 = 4.5, damp = 0.45, size = 0.85,
		                              earlyDiff = 0.7, modDepth = 0.1, modFreq = 2.0,
		                              low = 1.0, mid = 1.0, high = 0.7,
		                              lowcut = 100, highcut = 8000,
		                              wetAmp = 0.0, masterVol = 0.0,
		                              // v29-n: live performance drive (0 = bypass)
		                              drive = 0.0|
			var dry = In.ar(in, 2);
			// v29-h: Lag-smoothed wetAmp and masterVol prevent click/pop on .set
			var wetAmpLag    = Lag.kr(wetAmp, 0.4);
			var masterVolLag = Lag.kr(masterVol, 0.25);
			var driveLag     = Lag.kr(drive, 0.3);
			var driven, wet;

			// v29-n: tanh soft saturation, pre-reverb. At drive=0 the signal is
			// untouched (tanh of small input = small input, near linear). As drive
			// rises, the pre-gain pushes the input into the tanh curve, adding
			// upper harmonics for an analogue / Solar-style edge.
			//   drive 0.0 -> preGain 1.0 (clean, ~bypass)
			//   drive 0.5 -> preGain 2.5 (warm tube glow)
			//   drive 1.0 -> preGain 5.0 (analogue edge)
			// Output is normalized so adding drive doesn't blow up the level.
			driven = [
				tanh(dry[0] * (1 + (driveLag * 4))) / (1 + (driveLag * 0.6)),
				tanh(dry[1] * (1 + (driveLag * 4))) / (1 + (driveLag * 0.6))
			];

			wet = JPverb.ar(
				driven[0], driven[1],
				t60, damp, size,
				earlyDiff, modDepth, modFreq,
				low, mid, high, lowcut, highcut
			);
			Out.ar(out, wet * wetAmpLag * masterVolLag);
		}).add;
	} {
		"JPverb NOT available -- falling back to FreeVerb2. Install sc3-plugins for premium reverb.".postln;
		SynthDef(\famReverbMaster, { |in = 0, out = 0,
		                              t60 = 4.5, damp = 0.45, size = 0.85,
		                              earlyDiff = 0.7, modDepth = 0.1, modFreq = 2.0,
		                              low = 1.0, mid = 1.0, high = 0.7,
		                              lowcut = 100, highcut = 8000,
		                              wetAmp = 0.0, masterVol = 0.0,
		                              drive = 0.0|
			var dry = In.ar(in, 2);
			var wetAmpLag    = Lag.kr(wetAmp, 0.4);
			var masterVolLag = Lag.kr(masterVol, 0.25);
			var driveLag     = Lag.kr(drive, 0.3);
			var room = (size * 0.6 + (t60 / 8.0)).clip(0.1, 0.95);
			var driven = [
				tanh(dry[0] * (1 + (driveLag * 4))) / (1 + (driveLag * 0.6)),
				tanh(dry[1] * (1 + (driveLag * 4))) / (1 + (driveLag * 0.6))
			];
			var wet  = FreeVerb2.ar(driven[0], driven[1], 1.0, room, damp);
			Out.ar(out, wet * wetAmpLag * masterVolLag * 0.7);
		}).add;
	};

	"v29-i SynthDefs added: famCore + famPad + famAir + famBass + famSaw + famSquare + famReverbMaster + famPadCore + famSubBass.".postln;
	s.sync;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// v29-c: Reverb bus + master synth
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	~famVerbBus = Bus.audio(s, 2);
	~famVerbSynth = nil;   // will be spawned in ~famStart

	// v29-k: dedicated Group for short-lived motif/glitter synths so we can
	// freeAll on Stop without touching the sustained pad/subbass voices or
	// the reverb master. Created lazily on first ~famStart.
	~famMotifGroup = nil;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// LAYER A -- Harmonic Brain
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Scale intervals from root (semitones). 7 church modes.
	~famScales = (
		ionian:     [0, 2, 4, 5, 7, 9, 11],   // major
		dorian:     [0, 2, 3, 5, 7, 9, 10],
		phrygian:   [0, 1, 3, 5, 7, 8, 10],
		lydian:     [0, 2, 4, 6, 7, 9, 11],
		mixolydian: [0, 2, 4, 5, 7, 9, 10],
		aeolian:    [0, 2, 3, 5, 7, 8, 10],   // natural minor
		locrian:    [0, 1, 3, 5, 6, 8, 10]
	);

	// Progression bank: degree sequences (1-indexed, 1 = tonic)
	// v29-o: added \slow_bed -- two-chord meditative pad bed (State Azure style)
	~famProgressions = (
		\slow_bed:      [1, 4],          // v29-o: I → IV, very slow harmonic motion
		\I_V_vi_IV:     [1, 5, 6, 4],
		\I_IV_V_I:      [1, 4, 5, 1],
		\i_VI_III_VII:  [1, 6, 3, 7],
		\i_iv_VI_V:     [1, 4, 6, 5]
	);
	// v29-o: \slow_bed is the new default (index 0)
	~famProgKeys = [\slow_bed, \I_V_vi_IV, \I_IV_V_I, \i_VI_III_VII, \i_iv_VI_V];

	// v29-d: Stufen-Funktionen (HiChord-inspired roles for each scale degree)
	// 1=home, 2=lift, 3=contemplative, 4=lift/preparation, 5=tension/dominant,
	// 6=emotional/relative-minor-home, 7=maximum tension/suspense
	~famDegreeRoles = (
		1: \home,
		2: \lift,
		3: \contemplative,
		4: \lift,
		5: \tension,
		6: \emotional,
		7: \suspense
	);

	// v29-o: Constitutional Markov — only safe harmonic transitions.
	// Per current degree, weights for next degree (1-7).
	// CONSTITUTIONAL RULES:
	//   - Degree 7 (vii diminished) weight set to 0.0 — never harsh
	//   - All transitions favor stepwise/fifth movement (smooth voice leading)
	//   - Tonic-bound moves dominate (V→I, IV→I, ii→V→I patterns)
	//   - No leaps to 3 or 7 from anywhere (avoid "AI-random" jumps)
	~famDegreeTransitions = (
		// from 1 (home): -> IV, V, vi (gentle pull away)
		1: [0.10, 0.05, 0.05, 0.30, 0.25, 0.25, 0.0],
		// from 2: -> V (cadential), I (resolution)
		2: [0.25, 0.05, 0.05, 0.10, 0.40, 0.15, 0.0],
		// from 3: -> IV, vi (passing)
		3: [0.15, 0.10, 0.05, 0.30, 0.10, 0.30, 0.0],
		// from 4: -> I, V (plagal/dominant)
		4: [0.35, 0.10, 0.0, 0.05, 0.35, 0.15, 0.0],
		// from 5: -> I (strong cadence), vi (deceptive)
		5: [0.50, 0.05, 0.0, 0.10, 0.05, 0.30, 0.0],
		// from 6: -> IV, ii (subdominant motion)
		6: [0.20, 0.20, 0.05, 0.30, 0.20, 0.05, 0.0],
		// from 7: never reached (7 weight is 0 everywhere)
		7: [0.50, 0.10, 0.0, 0.20, 0.20, 0.0, 0.0]
	);

	// Pick next degree based on current using Markov weights.
	// v29-o: hard-clamp result to [1..6] so degree 7 can NEVER be reached
	// even if a transition table somehow had a 7-weight > 0.
	~famNextDegreeMarkov = { |currentDegree|
		var weights = ~famDegreeTransitions[currentDegree.asInteger]
		              ? ~famDegreeTransitions[1];
		var next = [1, 2, 3, 4, 5, 6, 7].wchoose(weights);
		next.clip(1, 6)   // constitutional safety
	};

	// AUTO Progression: track current degree separately for Markov mode
	~famAutoCurrentDegree = 1;

	// Harmonic Brain state
	~famKey           = 60;              // MIDI note of tonic (C4)
	~famMode          = \ionian;
	~famProg          = \slow_bed;       // v29-o: default to slow 2-chord bed
	// v29-o: slower harmonic motion. State Azure-style bed.
	~famProgAuto      = false;           // v29-o: NO Markov by default; use fixed 2-chord prog
	~famProgStep      = 0;               // index into current progression
	~famBarLength     = 8;               // v29-o: 8 bars per chord (was 2) -- breathing room
	~famBarCount      = 0;               // bars elapsed in current chord
	~famTotalBars     = 0;               // bars since start (for progression rotation)
	~famNextProgSwap  = rrand(32, 48);   // v29-o: even slower progression rotation

	// Get scale notes relative to a root MIDI note
	~famScaleNotes = { |rootMidi, modeKey, octaves = 2|
		var intervals = ~famScales[modeKey] ? ~famScales[\ionian];
		var notes = [];
		octaves.do { |oct|
			intervals.do { |iv|
				notes = notes.add(rootMidi + iv + (oct * 12));
			};
		};
		notes
	};

	// Build a chord on a given degree with EXTENSIONS (not just triad).
	// v29-d: Chord Families. Each family is a list of scale-step-offsets from the root.
	// Step 0 = degree itself, Step 2 = third (in scale terms), Step 7 = octave, etc.
	// Some families use chromatic alterations (e.g. \maj7_sharp11) which step outside
	// the scale by adding semitones. These are flagged via [step, semitoneOffset] pairs.
	~famChordFamilies = (
		\extended:        [0, 2, 4, 6, 8],            // R, 3rd, 5th, 7th, 9th  (default v29-a+)
		\sus2:            [0, 1, 4],                  // R, 2nd, 5th
		\sus4:            [0, 3, 4],                  // R, 4th, 5th
		\add9:            [0, 2, 4, 8],               // R, 3rd, 5th, 9th
		\maj7:            [0, 2, 4, 6],               // R, 3rd, 5th, 7th
		\six_nine:        [0, 2, 4, 5, 8],            // R, 3rd, 5th, 6th, 9th
		\min11:           [0, 2, 4, 6, 8, 10],        // R, 3rd, 5th, 7th, 9th, 11th
		\maj7_sharp11:    [0, 2, 4, 6, [3, 1]]        // R, 3rd, 5th, 7th, 4th+1semi (#11)
	);

	~famChordFamilyKeys = [\extended, \sus2, \sus4, \add9, \maj7, \six_nine, \min11, \maj7_sharp11];

	// Build chord at degree using a chord family.
	// v29-d: family parameter (default \extended preserves v29-c behavior).
	~famChordAtDegree = { |rootMidi, modeKey, degree, family = \extended|
		var intervals = ~famScales[modeKey];
		var d = degree - 1;
		var steps = ~famChordFamilies[family] ? ~famChordFamilies[\extended];
		var pick = { |stepOffset|
			var idx = d + stepOffset;
			var wrap = idx.mod(7);
			var oct = (idx / 7).floor * 12;
			rootMidi + intervals[wrap] + oct
		};
		var notes = steps.collect { |stepEntry|
			if(stepEntry.isArray) {
				// Chromatic alteration: [stepOffset, semitoneOffset]
				pick.(stepEntry[0]) + stepEntry[1]
			} {
				pick.(stepEntry)
			};
		};
		notes
	};

	// Triad-only version (for when we need just root/3rd/5th, e.g. bass picking)
	~famTriadAtDegree = { |rootMidi, modeKey, degree|
		var intervals = ~famScales[modeKey];
		var d = degree - 1;
		var pick = { |stepOffset|
			var idx = d + stepOffset;
			var wrap = idx.mod(7);
			var oct = (idx / 7).floor * 12;
			rootMidi + intervals[wrap] + oct
		};
		[pick.(0), pick.(2), pick.(4)]
	};

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// v29-b: VOICE LEADING
	// Inspired by HiChord's automatic voice leading.
	// When moving from one chord to another, pick the inversion of the
	// target chord that minimizes the total semitone movement of voices.
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Track the last-played chord for voice leading reference.
	~famLastVoicing = nil;   // array of MIDI notes as actually played last time
	~famVoiceCenter = 64;    // target center register for chords (C4 = 60, we aim slightly above)

	// Generate all candidate voicings of a chord:
	// all inversions × 3 octave shifts (−1, 0, +1 from base).
	~famChordCandidates = { |chordNotes|
		var cands = [];
		var size = chordNotes.size;
		// For each starting note (inversion), rebuild chord so that tone is lowest
		size.do { |startIdx|
			var inv = [];
			size.do { |i|
				var noteIdx = (startIdx + i).mod(size);
				var octShift = if((startIdx + i) >= size) { 12 } { 0 };
				inv = inv.add(chordNotes[noteIdx] + octShift);
			};
			// Try 3 octave placements
			[-12, 0, 12].do { |globalOct|
				cands = cands.add(inv.collect { |n| n + globalOct });
			};
		};
		cands
	};

	// Distance between two voicings: sum of pairwise semitone differences.
	// Uses the minimum-sum-pairing heuristic (for equal-size chords, direct match works well).
	~famVoicingDistance = { |v1, v2|
		var minSize = min(v1.size, v2.size);
		var total = 0;
		minSize.do { |i|
			total = total + (v1[i] - v2[i]).abs;
		};
		// Penalize voicings too far from center
		total = total + ((v2.mean - ~famVoiceCenter).abs * 0.3);
		total
	};

	// Main voice-leading function: given target chord (any voicing),
	// return the candidate voicing that is closest to the last played voicing.
	~famVoiceLead = { |targetChord|
		var candidates = ~famChordCandidates.(targetChord);
		var best, bestDist = 99999;
		if(~famLastVoicing.isNil) {
			// No previous voicing: pick the one closest to center
			candidates.do { |cand|
				var d = (cand.mean - ~famVoiceCenter).abs;
				if(d < bestDist) { bestDist = d; best = cand };
			};
		} {
			// Pick the one with smallest voice movement from last voicing
			candidates.do { |cand|
				var d = ~famVoicingDistance.(~famLastVoicing, cand);
				if(d < bestDist) { bestDist = d; best = cand };
			};
		};
		best ? targetChord
	};

	// Active chord family from current vibe
	~famActiveFamily = {
		(~famVibes[~famVibe] ? ~famVibes[\warm])[\family]
	};

	// Convenience: chord at degree using current vibe's family
	~famActiveChordAt = { |degree|
		~famChordAtDegree.(~famKey, ~famMode, degree, ~famActiveFamily.())
	};

	// Compute current chord based on brain state.
	// v29-b: voice-led -- returns the chord in the inversion closest to last play.
	// Also updates ~famLastVoicing so subsequent notes/motifs use this as reference.
	~famCurrentChord = {
		var deg = ~famCurrentDegree.();
		var raw = ~famChordAtDegree.(~famKey, ~famMode, deg, ~famActiveFamily.());
		var voiced = ~famVoiceLead.(raw);
		~famLastVoicing = voiced;
		voiced
	};

	~famCurrentChordRaw = {
		var deg = ~famCurrentDegree.();
		~famChordAtDegree.(~famKey, ~famMode, deg, ~famActiveFamily.())
	};

	// Compute allowed notes for phrases (chord tones + passing tones from scale)
	~famAllowedNotes = {
		var chord = ~famCurrentChord.();
		var scale = ~famScaleNotes.(~famKey, ~famMode, 2);
		chord ++ scale   // duplicates OK, chord tones will be preferred
	};

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// LAYER B -- Phrase Engine: Motifs + Mutations
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Motif = rhythm pattern + tonal shape.
	// v29-a+: Each motif returns a list of (noteIndex, duration, velocity).
	// Chord has 5 tones now: 0=root, 1=3rd, 2=5th, 3=7th, 4=9th.
	// Values >= 5 wrap one octave up (same tones, higher).
	~famMotifs = (
		\broken_up: {
			// Ascending triad, classic
			[[0, 0.5, 0.7], [1, 0.5, 0.65], [2, 0.5, 0.7]]
		},
		\broken_down: {
			[[2, 0.5, 0.7], [1, 0.5, 0.65], [0, 0.5, 0.75]]
		},
		\hopper: {
			[[0, 0.25, 0.7], [2, 0.25, 0.6], [1, 0.5, 0.75]]
		},
		\pulse_single: {
			[[0, 1.0, 0.8]]
		},
		\triplet: {
			[[0, 0.167, 0.7], [1, 0.167, 0.65], [2, 0.167, 0.7]]
		},
		\question: {
			[[1, 0.5, 0.75], [2, 0.5, 0.65]]
		},
		\answer: {
			[[2, 0.5, 0.7], [0, 0.5, 0.8]]
		},
		// v29-a+: Extension-aware motifs that use 9ths and 7ths
		\dreamy: {
			// Uses 9th (idx 4) for that HiChord-like sparkle
			[[4, 0.5, 0.6], [2, 0.5, 0.7], [0, 0.5, 0.75]]
		},
		\shimmering: {
			// Sus-like opening: root, 5th, 9th
			[[0, 0.33, 0.6], [2, 0.33, 0.6], [4, 0.66, 0.75]]
		},
		\cascade: {
			// Long descending arpeggio across all extensions
			[[4, 0.25, 0.5], [3, 0.25, 0.55], [2, 0.25, 0.6], [1, 0.25, 0.65], [0, 0.5, 0.7]]
		},
		\bell: {
			// Single tones with emphasis on chord color (7th, 9th)
			[[0, 0.75, 0.8], [4, 0.5, 0.5], [3, 0.5, 0.55]]
		},
		\rest: {
			[]   // silent motif
		}
	);
	~famMotifKeys = [\broken_up, \broken_down, \hopper, \pulse_single,
	                 \triplet, \question, \answer,
	                 \dreamy, \shimmering, \cascade, \bell, \rest];

	// Mutations: take a motif result and modify it
	~famMutations = (
		\identity:  { |m| m },
		\octave_up: { |m| m.collect { |n| [n[0] + 7, n[1], n[2]] } },   // shift note idx by ~octave
		\octave_down: { |m| m.collect { |n| [n[0] - 7, n[1], n[2]] } },
		\retrograde: { |m| m.reverse },
		\drop_first: { |m| if(m.size > 1) { m[1..] } { m } },
		\softer:    { |m| m.collect { |n| [n[0], n[1], n[2] * 0.7] } },
		\stretch:   { |m| m.collect { |n| [n[0], n[1] * 1.5, n[2]] } }
	);
	~famMutationKeys = [\identity, \octave_up, \octave_down, \retrograde,
	                    \drop_first, \softer, \stretch];

	// Pick a note value from chord tones using motif note index
	// v29-a+: Chord now has 5 tones (root, 3rd, 5th, 7th, 9th)
	// idx 0..4 picks chord tones, idx 5..9 same but one octave up, etc.
	~famPickNote = { |noteIdx, octaveShift = 0|
		var chord = ~famCurrentChord.();
		var chordSize = chord.size;   // 5 with extensions
		var base = noteIdx.mod(chordSize);
		var oct = (noteIdx / chordSize).floor;
		chord[base] + (oct * 12) + (octaveShift * 12)
	};

	// v29-f: Spawn a single note with active voice character + optional MIDI emit.
	// v29-k: motif amps reduced by ~40%, reverb sends raised so glitter
	// floats above the pad instead of competing with it. Decays shortened
	// so notes don't blur into a sustained mass. Spawned in ~famMotifGroup
	// so they can be freed cleanly on Stop.
	~famSpawnNote = { |freq, vel, decay, pan|
		var voice = ~famCoreVoice;
		var modIdx = vel.linlin(0.3, 1.0, 1.5, 3.5);
		var motifDecay = decay * 0.7;
		var target = ~famMotifGroup ? s;
		case
		{ voice == \saw } {
			Synth(\famSaw, [
				\freq, freq, \amp, vel * 0.10, \decay, motifDecay,
				\pan, pan, \verbOut, ~famVerbBus.index, \verbSend, 0.40
			], target);
		}
		{ voice == \square } {
			Synth(\famSquare, [
				\freq, freq, \amp, vel * 0.08, \decay, motifDecay,
				\pan, pan, \verbOut, ~famVerbBus.index, \verbSend, 0.45
			], target);
		}
		{
			// default \fm
			Synth(\famCore, [
				\freq, freq, \amp, vel * 0.14, \decay, motifDecay,
				\modIndex, modIdx, \pan, pan,
				\verbOut, ~famVerbBus.index, \verbSend, 0.40,
				\hpCutoff, 600
			], target);
		};
		// MIDI emission (if enabled and out is open)
		if(~famMidiEnabled and: { ~famMidiOut.notNil }) {
			var midiNote = freq.cpsmidi.round.asInteger.clip(0, 127);
			var midiVel = (vel * 127).asInteger.clip(1, 127);
			~famMidiOut.noteOn(~famMidiChannel, midiNote, midiVel);
			AppClock.sched(motifDecay * 0.8, {
				if(~famMidiOut.notNil) {
					~famMidiOut.noteOff(~famMidiChannel, midiNote, 0);
				};
				nil
			});
		};
	};

	// Play a motif with the current harmonic context
	~famPlayMotif = { |motifKey, mutationKey = \identity, baseOctave = 0, baseAmp = 1.0|
		var motif = ~famMotifs[motifKey].value;
		var mutated = ~famMutations[mutationKey].value(motif);
		var beatDur = 60 / ~famBPM;
		var chord = ~famCurrentChord.();

		if(mutated.size > 0) {
			Task({
				var chordSize = chord.size;
				mutated.do { |noteSpec|
					var noteIdx = noteSpec[0];
					var dur = noteSpec[1];
					var vel = noteSpec[2] * baseAmp;
					var base = noteIdx.mod(chordSize);
					var oct = (noteIdx / chordSize).floor;
					var midi, freq, decay;

					// v29-j: weighted random octave on top of motif's structural octave
					// Probabilities: low (-1) 15%, mid (0) 50%, high (+1) 30%, very high (+2) 5%
					// Root and 5th get more low/mid; 3rd and 7th get more mid/high.
					var octBias = if((base == 0) or: { base == 2 }) {
						[-1, 0, 0, 0, 1].wchoose([0.20, 0.25, 0.25, 0.20, 0.10])
					} {
						[-1, 0, 0, 1, 1, 2].wchoose([0.05, 0.15, 0.30, 0.25, 0.20, 0.05])
					};

					midi = chord[base] + (oct * 12) + (baseOctave * 12) + (octBias * 12);
					freq = midi.midicps;
					decay = rrand(2.5, 4.5);

					~famSpawnNote.(freq, vel, decay, rrand(-0.4, 0.4));

					(dur * beatDur).wait;
				};
			}).play;
		};
	};

	// Play a specific degree as a motif (triggered by note-cells from UI).
	// Degree can be 1..14 (above 7 wraps one octave up).
	// Always plays a motif based on the requested chord, regardless of progression.
	~famPlayDegree = { |degree, motifKey = \broken_up, mutationKey = \identity, octave = 0, amp = 1.0|
		var rawChord = ~famChordAtDegree.(~famKey, ~famMode, degree, ~famActiveFamily.());
		var chord = ~famVoiceLead.(rawChord);
		var motif = ~famMotifs[motifKey].value;
		var mutated = ~famMutations[mutationKey].value(motif);
		var beatDur = 60 / ~famBPM;

		~famLastVoicing = chord;

		if(mutated.size > 0) {
			Task({
				var chordSize = chord.size;
				mutated.do { |noteSpec|
					var noteIdx = noteSpec[0];
					var dur = noteSpec[1];
					var vel = noteSpec[2] * amp;
					var base = noteIdx.mod(chordSize);
					var oct  = (noteIdx / chordSize).floor;
					var midi, freq, decay;

					// v29-j: same weighted octave bias as in autoMotif
					var octBias = if((base == 0) or: { base == 2 }) {
						[-1, 0, 0, 0, 1].wchoose([0.20, 0.25, 0.25, 0.20, 0.10])
					} {
						[-1, 0, 0, 1, 1, 2].wchoose([0.05, 0.15, 0.30, 0.25, 0.20, 0.05])
					};

					midi = chord[base] + (oct * 12) + (octave * 12) + (octBias * 12);
					freq = midi.midicps;
					decay = rrand(2.5, 4.5);

					~famSpawnNote.(freq, vel, decay, rrand(-0.4, 0.4));
					(dur * beatDur).wait;
				};
			}).play;
		};
	};

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// AUTO-PLAY Task
	// Progression advances every BarLength bars.
	// Auto-plays motifs if ~famAutoPlay is true.
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	~famBPM       = 54;       // v29-o: slower default (was 72) -- less nervous
	~famAutoPlay   = false;   // legacy alias kept for compat
	~famGenerative = false;   // v29-o INSTRUMENT MODE: opt-in auto-progression
	~famDrone      = false;   // v29-o: opt-in tonic drone
	~famHold      = false;
	~famMood      = 0.5;     // legacy float, still used as density modifier
	~famRunning   = false;
	~famBassEnabled = true;
	// v29-f: voice character + MIDI output
	~famCoreVoice   = \fm;
	~famMidiEnabled = false;
	~famMidiOut     = nil;
	~famMidiChannel = 0;
	// v29-h: master volume + hardware amp control
	~famMasterVol   = 0.85;     // 0.0 .. 1.0  (linear gain on reverb master output)
	~famAmpEnabled  = true;     // PAM8403 SHDN state mirror (true = active)
	~famJackInserted = false;   // audio-jack detection (set by Pico via /fam/jackdetect)
	~famEngineTask = nil;
	~famAutoMotifTask = nil;

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// v29-d: VIBE SYSTEM
	// 5 curated moods, each bundles chord family + reverb bias + density + air
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// v29-o: \sharp removed (constitutional rule: never harsh).
	// Was: family maj7_sharp11 — could turn dissonant under bright filter.
	// Internal name stays "Vibe" but user-facing label is "MOOD".
	~famVibes = (
		\warm: (
			family:     \add9,
			density:    0.55,
			airWait:    [12, 35],
			reverbBias: (t60: 1.0, damp: 1.0, size: 1.0, high: 0.7)
		),
		\bright: (
			family:     \maj7,
			density:    0.7,
			airWait:    [6, 18],
			reverbBias: (t60: 1.3, damp: 0.6, size: 1.1, high: 1.4)
		),
		\deep: (
			family:     \min11,
			density:    0.4,
			airWait:    [25, 60],
			reverbBias: (t60: 0.85, damp: 1.4, size: 0.9, high: 0.5)
		),
		\floating: (
			family:     \sus2,
			density:    0.35,
			airWait:    [8, 22],
			reverbBias: (t60: 1.6, damp: 0.7, size: 1.25, high: 1.2)
		)
	);
	~famVibeKeys = [\warm, \bright, \deep, \floating];   // v29-o: 4 safe vibes
	~famVibe     = \warm;

	// Modal Reverb base presets (per scale mode)
	~famReverbModePresets = (
		ionian:     (t60: 2.8, damp: 0.4, size: 0.7,  high: 0.9),
		dorian:     (t60: 2.2, damp: 0.5, size: 0.6,  high: 0.7),
		phrygian:   (t60: 1.8, damp: 0.6, size: 0.5,  high: 0.55),
		lydian:     (t60: 4.5, damp: 0.25, size: 0.85, high: 1.1),
		mixolydian: (t60: 3.0, damp: 0.35, size: 0.75, high: 0.95),
		aeolian:    (t60: 2.4, damp: 0.55, size: 0.6,  high: 0.65),
		locrian:    (t60: 1.5, damp: 0.7, size: 0.45, high: 0.5)
	);

	// Compute current reverb parameters: mode preset × vibe bias.
	// Live-tunable -- call this whenever mode or vibe changes.
	~famComputeReverb = {
		var modePreset = ~famReverbModePresets[~famMode] ? ~famReverbModePresets[\ionian];
		var vibe = ~famVibes[~famVibe] ? ~famVibes[\warm];
		var bias = vibe[\reverbBias];
		(
			t60:  modePreset[\t60]  * bias[\t60],
			damp: (modePreset[\damp] * bias[\damp]).clip(0.0, 1.0),
			size: (modePreset[\size] * bias[\size]).clip(0.5, 5.0),
			high: (modePreset[\high] * bias[\high]).clip(0.0, 1.0)
		)
	};

	// Update reverb master synth with computed params (call after mode/vibe change)
	~famUpdateReverb = {
		var p = ~famComputeReverb.();
		if(~famVerbSynth.notNil) {
			~famVerbSynth.set(
				\t60,  p[\t60],
				\damp, p[\damp],
				\size, p[\size],
				\high, p[\high]
			);
			("FAM reverb updated: t60=" ++ p[\t60].round(0.01)
			 ++ " damp=" ++ p[\damp].round(0.01)
			 ++ " size=" ++ p[\size].round(0.01)).postln;
		};
	};

	// v29-h: master volume setter with Lag-smoothed change
	~famSetMasterVol = { |vol|
		~famMasterVol = vol.clip(0.0, 1.0);
		if(~famVerbSynth.notNil) {
			// Apply jack-detect mute: if jack inserted AND amp is shut down, vol stays 0
			var effective = if(~famAmpEnabled) { ~famMasterVol } { 0.0 };
			~famVerbSynth.set(\masterVol, effective);
		};
		("FAM master volume: " ++ (~famMasterVol * 100).round(1) ++ "%").postln;
	};

	~famAdvanceProgression = {
		var prog;
		var currentBarTarget;

		// v29-j: use prime-ratio bar lengths if enabled, falls back to fixed
		currentBarTarget = if(~famUsePrimeBars) {
			~famPrimeBarLengths[~famPrimeBarIdx]
		} {
			~famBarLength
		};

		if(~famHold.not) {
			~famBarCount = ~famBarCount + 1;
			if(~famBarCount >= currentBarTarget) {
				~famBarCount = 0;
				~famTotalBars = ~famTotalBars + currentBarTarget;

				// v29-j: advance to next prime length for next chord
				if(~famUsePrimeBars) {
					~famPrimeBarIdx = (~famPrimeBarIdx + 1).mod(~famPrimeBarLengths.size);
				};

				if(~famProgAuto) {
					// v29-d: Markov-driven autonomous progression
					~famAutoCurrentDegree = ~famNextDegreeMarkov.(~famAutoCurrentDegree);
					("FAM auto-prog: degree=" ++ ~famAutoCurrentDegree
					 ++ " (" ++ ~famDegreeRoles[~famAutoCurrentDegree.asInteger] ++ ")"
					 ++ " (next change in " ++ ~famPrimeBarLengths[~famPrimeBarIdx] ++ " bars)").postln;
				} {
					// Fixed progression: cycle through preset
					prog = ~famProgressions[~famProg];
					~famProgStep = (~famProgStep + 1).mod(prog.size);
					("FAM bar: chord=" ++ prog[~famProgStep]
					 ++ " key=" ++ ~famKey ++ " mode=" ++ ~famMode).postln;
				};

				// v29-i: spawn fresh pad voices for the new chord
				// (only if no cells are being held -- holds override progression)
				if(~famCellHolds.notNil and: { ~famCellHolds.size == 0 }) {
					~famSpawnPad.(~famCurrentChord.());
					~famPushState.();
				};
			};
		};
	};

	// Helper: get current degree (works for both fixed and auto-Markov mode)
	~famCurrentDegree = {
		if(~famProgAuto) {
			~famAutoCurrentDegree
		} {
			var prog = ~famProgressions[~famProg];
			prog[~famProgStep] ? 1
		}
	};

	~famStart = {
		if(~famRunning.not) {
			~famRunning = true;
			// v29-g: reset Markov to tonic on every Start so we don't bleed state
			//         from a previous session.
			~famAutoCurrentDegree = 1;
			~famProgStep = 0;
			~famBarCount = 0;
			~famLastVoicing = nil;

			// v29-k: create dedicated motif group at server head so motifs are
			// processed before the reverb master, and can be freed cleanly.
			~famMotifGroup = Group.new(s, \addToHead);

			// v29-d/h: spawn reverb master with pop-suppression
			//   -> spawn at wetAmp=0, masterVol=0
			//   -> ramp up via .set (Lag.kr smooths it)
			{
				var rp = ~famComputeReverb.();
				~famVerbSynth = Synth.tail(s, \famReverbMaster, [
					\in,        ~famVerbBus.index,
					\out,       0,
					\t60,       rp[\t60],
					\damp,      rp[\damp],
					\size,      rp[\size],
					\earlyDiff, 0.7,
					\modDepth,  0.1,
					\modFreq,   2.0,
					\low,       1.0,
					\mid,       1.0,
					\high,      rp[\high],
					\lowcut,    100,
					\highcut,   8000,
					\wetAmp,    0.0,           // start silent
					\masterVol, 0.0,
					\drive,     ~famDrive       // v29-n: carry over current drive setting
				]);
				// v29-h: schedule fade-in 200ms after spawn so DAC has stabilized
				AppClock.sched(0.2, {
					if(~famVerbSynth.notNil) {
						~famVerbSynth.set(\wetAmp, 0.55, \masterVol, ~famMasterVol);
					};
					nil
				});
			}.value;

			// v29-o INSTRUMENT MODE: do NOT auto-spawn pad on start.
			// Only spawn pad if generative mode is enabled. Otherwise spawn
			// drone if drone-mode is enabled. Otherwise: silence — user plays.
			AppClock.sched(0.3, {
				if(~famRunning) {
					if(~famGenerative) {
						~famSpawnPad.(~famCurrentChord.());
					};
					if(~famDrone and: { ~famGenerative.not }) {
						~famSpawnDrone.();
					};
				};
				nil
			});

			// v29-o: bar-clock only advances chord in generative mode
			~famEngineTask = Task({
				loop {
					var barDur = (60 / ~famBPM) * 4;
					barDur.wait;
					if(~famGenerative) {
						~famAdvanceProgression.();
					};
				};
			}).play;

			// v29-m: Auto-motif task ONLY spawns if glitter is explicitly enabled.
			// By default the pad is alone -- no FM glockenspiel tones.
			if(~famGlitterEnabled) {
				~famAutoMotifTask = Task({
					loop {
						var pauseBeats, motifKey, mutationKey, oct, amp;
						var beatDur = 60 / ~famBPM;

						if(~famAutoPlay) {
							// v29-d: density now comes from Vibe (with mood as global modifier)
							var vibe = ~famVibes[~famVibe] ? ~famVibes[\warm];
							var density = vibe[\density] * (0.5 + ~famMood);
							pauseBeats = rrand(2.0, 6.0) * (1.5 - density);
							(pauseBeats * beatDur).wait;

							if(0.2.coin.not) {
								motifKey = ~famMotifKeys.wchoose(
									[0.08, 0.06, 0.07, 0.05, 0.04, 0.05, 0.05,
									 0.14, 0.12, 0.10, 0.12, 0.12]
								);
								mutationKey = ~famMutationKeys.wchoose(
									[0.40, 0.10, 0.15, 0.05, 0.10, 0.10, 0.10]
								);
								oct = [-1, 0, 0, 0, 1].choose;
								amp = rrand(0.6, 1.0) * (0.5 + (~famMood * 0.5));
								~famPlayMotif.(motifKey, mutationKey, oct, amp);
							};
						} {
							beatDur.wait;
						};
					};
				}).play;
			} {
				~famAutoMotifTask = nil;
				"FAM glitter disabled (no auto-motifs).".postln;
			};

			// v29-d: Air task -- wait range comes from Vibe
			// v29-m: also conditional on glitter enable flag
			if(~famGlitterEnabled) {
				~famAirTask = Task({
					loop {
						var vibe = ~famVibes[~famVibe] ? ~famVibes[\warm];
						var range = vibe[\airWait];
						var wait = rrand(range[0], range[1]);
						wait.wait;
						if(~famRunning) {
							var scaleNotes = ~famScaleNotes.(~famKey, ~famMode, 1);
							var noteMidi = scaleNotes.choose + 24;
							Synth(\famAir, [
								\freq, noteMidi.midicps,
								\verbOut, ~famVerbBus.index,
								\amp, rrand(0.025, 0.05) * (0.5 + (~famMood * 0.5)),
								\decay, rrand(3.5, 6.0),
								\pan, rrand(-0.6, 0.6),
								\verbSend, 0.55
							], ~famMotifGroup ? s);
						};
					};
				}).play;
			} {
				~famAirTask = nil;
			};

			// v29-k: Bass task DEACTIVATED -- famSubBass now handles low-end
			// continuously (sustained, akkord-aware). The old famBass was a
			// one-shot per-bar trigger that doubled up with the new subbass
			// and muddied the low end. Kept the variable around for legacy
			// cleanup paths in ~famStop.
			~famBassSynth = nil;
			~famBassTask = nil;

			// v29-o: spawn texture synth if texture > 0 (lazy spawn happens
			// in ~famSetTexture too, but we spawn here on start to ensure
			// the bed is alive from the moment the engine starts).
			if(~famTexture > 0.001) {
				~famTextureSynth = Synth(\famTexture, [
					\amp,      ~famTexture * 0.12,
					\gate,     1,
					\verbOut,  ~famVerbBus.index,
					\verbSend, 0.55,
					\bpCenter, 600,
					\bpQ,      0.4,
					\lfoRate,  0.04
				], ~famMotifGroup);
			};

			"FAM started. Engine running (v29-o: Pad + Sub + Texture + Reverb).".postln;
			("  BPM=" ++ ~famBPM ++ "  Key=" ++ ~famKey
			 ++ "  Mode=" ++ ~famMode ++ "  Prog=" ++ ~famProg).postln;
		} {
			"FAM already running.".postln;
		};
	};

	~famStop = {
		// v29-g: stop tasks BEFORE setting running=false so any in-flight
		// loop body that already passed the running-check still runs cleanly,
		// then is stopped. Order matters for race-free shutdown.
		if(~famEngineTask.notNil)    { ~famEngineTask.stop;    ~famEngineTask = nil };
		if(~famAutoMotifTask.notNil) { ~famAutoMotifTask.stop; ~famAutoMotifTask = nil };
		if(~famAirTask.notNil)       { ~famAirTask.stop;       ~famAirTask = nil };
		if(~famBassTask.notNil)      { ~famBassTask.stop;      ~famBassTask = nil };
		~famRunning = false;

		// v29-h: pop-suppression -- ramp masterVol to 0 BEFORE freeing synths.
		// The Lag.kr in famReverbMaster smooths this over 250ms, then we wait
		// 350ms for the ramp to complete before .free.
		if(~famVerbSynth.notNil) {
			~famVerbSynth.set(\masterVol, 0.0);
		};

		// v29-i: gentle release of pad voices (their long release envelope
		// gives them ~3s to fade out after gate=0)
		~famReleasePad.();

		// Per-cell holds: .release for graceful fadeout (envelope drives it)
		if(~famCellHolds.notNil) {
			~famCellHolds.keysValuesDo { |k, synths|
				synths.do { |s| if(s.notNil) { s.release } };
			};
			~famCellHolds.clear;   // v29-o: consistent with rest of cleanup
		};
		~famHeldSynths.do { |s| if(s.notNil) { s.release } };
		~famHeldSynths = [];

		// v29-o: release texture synth (long release envelope, ~6s fade)
		if(~famTextureSynth.notNil) {
			~famTextureSynth.release;
			~famTextureSynth = nil;
		};

		// v29-l: also release any sustained cell-tap voices
		if(~famCellTapVoices.notNil) {
			~famCellTapVoices.keysValuesDo { |k, synth|
				if(synth.notNil) { synth.release };
			};
			~famCellTapVoices = ();
		};

		// v29-k: free the motif group BEFORE the reverb fade completes.
		// 100ms is enough to let any in-flight short notes decay slightly.
		// freeAll on the group also kills any motif synths that the
		// AutoMotifTask spawned in inner Tasks (which we couldn't track
		// individually). That was the v29-h..j stop bug.
		AppClock.sched(0.1, {
			if(~famMotifGroup.notNil) {
				~famMotifGroup.freeAll;
			};
			nil
		});

		// v29-g: hard-cut after fade window
		AppClock.sched(0.35, {
			if(~famBassSynth.notNil)  { ~famBassSynth.free;  ~famBassSynth = nil };
			if(~famVerbSynth.notNil)  { ~famVerbSynth.free;  ~famVerbSynth = nil };
			// v29-i: also hard-cut any remaining pad voices after fade
			~famFreePad.();
			// v29-k: free the motif group itself (not just contents)
			if(~famMotifGroup.notNil) {
				~famMotifGroup.free;
				~famMotifGroup = nil;
			};
			nil
		});

		// v29-g: MIDI all-notes-off blanket on all 16 channels
		if(~famMidiOut.notNil) {
			16.do { |ch|
				~famMidiOut.control(ch, 123, 0);   // CC 123 = All Notes Off
				~famMidiOut.control(ch, 120, 0);   // CC 120 = All Sound Off
			};
		};

		"FAM stopped.".postln;
	};

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// OSC Endpoints -- Interface for HTML Panel via Python Bridge
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	// Transport
	OSCdef(\famStart, { |msg| ~famStart.() }, "/fam/start");
	OSCdef(\famStop,  { |msg| ~famStop.()  }, "/fam/stop");
	OSCdef(\famAuto, { |msg|
		~famAutoPlay = (msg[1].asInteger > 0);
		("FAM autoplay: " ++ ~famAutoPlay).postln;
	}, "/fam/auto");

	// v29-o: /fam/generative [0/1] — toggle auto-progression bed mode
	OSCdef(\famGenerative, { |msg|
		var newVal = (msg[1].asInteger > 0);
		var changed = (newVal != ~famGenerative);
		~famGenerative = newVal;
		if(changed and: { ~famRunning }) {
			if(~famGenerative) {
				if(~famDroneVoice.notNil) {
					~famReleaseDrone.();
				};
				if(~famCellHolds.size == 0) {
					~famSpawnPad.(~famCurrentChord.());
				};
			} {
				if(~famCellHolds.size == 0) {
					~famReleasePad.();
					if(~famDrone) {
						~famSpawnDrone.();
					};
				};
			};
		};
		~famPushState.();
		("FAM generative: " ++ ~famGenerative).postln;
	}, "/fam/generative");

	// v29-o: /fam/drone [0/1] — toggle soft tonic drone
	OSCdef(\famDroneToggle, { |msg|
		var newVal = (msg[1].asInteger > 0);
		~famDrone = newVal;
		if(~famRunning) {
			if(~famDrone) {
				if(~famGenerative.not and: { ~famCellHolds.size == 0 }) {
					~famSpawnDrone.();
				};
			} {
				~famReleaseDrone.();
			};
		};
		~famPushState.();
		("FAM drone: " ++ ~famDrone).postln;
	}, "/fam/drone");

	// v29-l: Cell-Tap now ADDS a sustained pad voice to the ambient texture.
	// It picks the chord-root tone of the requested degree and plays it
	// with the same warm pad timbre as the main pad layer.
	// /fam/cell [degree, octave, amp]  -- triggers a sustained voice
	// /fam/celloff [degree]            -- gates it off; voice fades over 3s
	OSCdef(\famCell, { |msg|
		var degree = msg[1].asInteger;
		var octave = (msg[2] ? 0).asInteger;
		var amp    = (msg[3] ? 1.0).asFloat;
		var rawChord, voicedChord, midi;
		var oldSynth, newSynth;
		var vMix, octShift, moodCutoffBase;
		var brightnessHz;     // v29-n

		// v29-o BUGFIX: drop cell-tap if engine isn't running.
		// Otherwise we'd spawn a synth without a verb bus, leaking it.
		if(~famRunning.not) {
			"FAM cell ignored (engine not running).".postln;
			^nil;
		};

		// Resolve the chord at the requested degree
		rawChord = ~famChordAtDegree.(~famKey, ~famMode, degree, ~famActiveFamily.());
		// Voice-lead it near current center, apply pad-octave shift
		octShift = ~famPadOctaveShifts[~famPadOctave] ? 0;
		voicedChord = rawChord.collect { |n|
			var meanCenter = rawChord.mean;
			var shift = ((~famVoiceCenter - meanCenter) / 12).round * 12;
			n + shift + octShift
		};

		// Pick the chord ROOT (lowest tone) as the cell's voice -- one
		// note per cell. Apply the user's octave offset on top.
		midi = voicedChord.minItem + (octave * 12);

		// Use the pad voice character (warm by default)
		vMix = ~famPadVoiceMixes[~famPadVoiceCharacter] ? 0.0;
		moodCutoffBase = 700 + (~famMood * 800);
		// v29-n: carry over the current brightness offset
		brightnessHz = if(~famBrightness >= 0) { ~famBrightness * 1500 } { ~famBrightness * 700 };

		// Replace any previous tap-synth on this same degree (re-tap)
		oldSynth = ~famCellTapVoices[degree.asSymbol];
		if(oldSynth.notNil) { oldSynth.release };

		// Spawn ONE sustained pad voice for this cell tap
		newSynth = Synth(\famPadCore, [
			\freq,         midi.midicps,
			\amp,          0.10 * amp,
			\gate,         1,
			\pan,          (degree.asInteger - 4) * 0.15,    // spread cells across stereo
			\detuneCents,  7 + 4.0.rand,
			\attack,       0.8,                              // faster than main pad
			\release,      3.0,                              // long release after gate=0
			\cutoffBase,   moodCutoffBase,
			\cutoffMod,    500,
			\cutoffRate,   0.06 + 0.04.rand,
			\lpRes,        0.18,
			\voiceMix,     vMix,
			\cutoffOffset, brightnessHz,                     // v29-n
			\fenvAttack,  1.5,
			\fenvDecay,   2.5,
			\fenvSustain, 0.6,
			\fenvAmount,  0.0,    // v29-m: no filter swelling
			\verbOut,     ~famVerbBus.index,
			\verbSend,    0.4     // v29-m: matches main pad
		]);

		~famCellTapVoices[degree.asSymbol] = newSynth;
		// v29-o-tuned: cell-tap ducking disabled. Was distracting — every tap
		// caused a noticeable drop in the texture, made the ambience feel
		// "reactive" instead of contemplative. Cell-hold still ducks (gentler).
		("FAM cell-tap: degree=" ++ degree ++ " midi=" ++ midi
		 ++ " (sustained until /fam/celloff)").postln;
	}, "/fam/cell");

	// v29-l: gate off a cell-tap voice (lets it fade via release envelope)
	OSCdef(\famCellOff, { |msg|
		var degree = msg[1].asInteger;
		var synth = ~famCellTapVoices[degree.asSymbol];
		if(synth.notNil) {
			synth.release;
			// v29-o BUGFIX: use removeAt (shrinks dict) not assign-nil (keeps key)
			~famCellTapVoices.removeAt(degree.asSymbol);
			("FAM cell-off: degree=" ++ degree).postln;
		};
	}, "/fam/celloff");

	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	// v29-e: PER-CELL HOLD
	// Multiple cells can hold simultaneously. Each cell tracks its own
	// synths so it can be released independently. Voice leading is OFF
	// for layered holds since there's no single "previous" voicing.
	// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

	~famCellHolds = ();   // dict: degree -> array of synths

	// v29-l: cell-tap voices, dict: degree -> single sustained pad synth
	~famCellTapVoices = ();

	// Helper: stop all synths for a given degree (sends gate=0 release)
	~famReleaseCellHold = { |degree|
		var synths = ~famCellHolds[degree.asSymbol];
		if(synths.notNil) {
			synths.do { |s| if(s.notNil) { s.release } };
			// v29-o BUGFIX: removeAt actually shrinks the dict (size goes down).
			// Setting to nil left the key in place, so .size stayed > 0,
			// preventing pad-reset checks like (~famCellHolds.size == 0).
			~famCellHolds.removeAt(degree.asSymbol);
			("FAM hold-off: degree=" ++ degree).postln;
		};
	};

	// v29-i: Cell-Hold updates the Pad-Layer to play the held chord.
	// v29-l: hold-accent voices use famPadCore (warm saw) instead of famPad
	// (FM) so they match the ambient texture instead of clashing with it.
	~famSpawnCellHold = { |degree|
		var rawChord = ~famChordAtDegree.(~famKey, ~famMode, degree, ~famActiveFamily.());
		var meanCenter = rawChord.mean;
		var shift = ((~famVoiceCenter - meanCenter) / 12).round * 12;
		var chord = rawChord.collect { |n| n + shift };
		var synths = [];
		var vMix = ~famPadVoiceMixes[~famPadVoiceCharacter] ? 0.0;
		var moodCutoffBase = 700 + (~famMood * 800);
		// v29-n: pick up current brightness so accent voices match the rest
		var brightnessHz = if(~famBrightness >= 0) { ~famBrightness * 1500 } { ~famBrightness * 700 };

		// Spawn small accent pads (the "anschlag") -- short attack, longer release
		// v29-l: now famPadCore (warm saw) instead of famPad (FM bell)
		chord.do { |midi, i|
			var pan = (i - (chord.size / 2)) / (chord.size * 1.5);
			var synth = Synth(\famPadCore, [
				\freq,         midi.midicps,
				\amp,          0.06,
				\gate,         1,
				\pan,          pan,
				\detuneCents,  7 + 4.0.rand,
				\attack,       0.4,                  // accent feel: faster attack
				\release,      3.0,
				\cutoffBase,   moodCutoffBase,
				\cutoffMod,    400,
				\cutoffRate,   0.07 + 0.05.rand,
				\lpRes,        0.18,
				\voiceMix,     vMix,
				\cutoffOffset, brightnessHz,         // v29-n
				\fenvAttack,   1.5,                   // v29-o: slightly slower
				\fenvDecay,    3.0,
				\fenvSustain,  0.7,
				\fenvAmount,   0.15,                  // v29-o: small accent bloom
				\verbOut,      ~famVerbBus.index,
				\verbSend,     0.5                    // v29-o: matches main pad
			]);
			synths = synths.add(synth);
		};
		~famCellHolds[degree.asSymbol] = synths;

		// v29-i: also re-spawn the main pad layer on this chord so the hold
		// is the dominant sustained sound, not just an accent.
		// If multiple cells are held, this picks the most recently held one.
		~famSpawnPad.(chord);

		// VL reference only when single hold (matches v29-g behavior)
		if(~famCellHolds.size == 1) {
			~famLastVoicing = chord;
		};
		// v29-o-tuned: gentler hold-duck (was 0.3, 0.4 — too dramatic).
		// Just 25% reduction over 1.2s — barely noticeable but gives the
		// hold-chord a tiny bit of room without pumping the ambience.
		~famDuckTexture.(0.75, 1.2);
		("FAM hold-on: degree=" ++ degree ++ " chord=" ++ chord
		 ++ " (active holds: " ++ (~famCellHolds.keys.asArray) ++ ")").postln;
	};

	// /fam/hold [degree, on]  -- on=1 starts hold, on=0 releases
	// v29-o INSTRUMENT MODE helper: after a hold is released, decide what
	// the pad should do. Generative mode → respawn on progression chord.
	// Instrument mode (default) → release pad to silence (or drone if active).
	~famPostHoldUpdate = {
		if(~famRunning) {
			if(~famGenerative) {
				~famSpawnPad.(~famCurrentChord.());
			} {
				~famReleasePad.();
				if(~famDrone) {
					~famSpawnDrone.();
				};
			};
			~famPushState.();
		};
	};

	OSCdef(\famCellHold, { |msg|
		var degree = msg[1].asInteger;
		var on = (msg[2] ? 1).asInteger > 0;
		var key = degree.asSymbol;

		if(~famRunning.not) {
			"FAM hold ignored (engine not running).".postln;
			^nil;
		};

		if(on) {
			if(~famCellHolds[key].notNil) {
				~famReleaseCellHold.(degree);
			};
			~famSpawnCellHold.(degree);
		} {
			~famReleaseCellHold.(degree);
			if(~famCellHolds.size == 0) {
				~famPostHoldUpdate.();
			};
		};
	}, "/fam/hold");

	OSCdef(\famCellHoldOff, { |msg|
		var degree = msg[1] ? 0;
		if(degree.asInteger == 0) {
			~famCellHolds.keysValuesDo { |k, synths|
				synths.do { |s| if(s.notNil) { s.release } };
			};
			~famCellHolds.clear;
			~famPostHoldUpdate.();
			"FAM hold-off: ALL".postln;
		} {
			~famReleaseCellHold.(degree.asInteger);
			if(~famCellHolds.size == 0) {
				~famPostHoldUpdate.();
			};
		};
	}, "/fam/holdoff");

	OSCdef(\famCellHoldClear, { |msg|
		~famCellHolds.keysValuesDo { |k, synths|
			synths.do { |s| if(s.notNil) { s.release } };
		};
		~famCellHolds.clear;

		if(~famCellTapVoices.notNil) {
			~famCellTapVoices.keysValuesDo { |k, synth|
				if(synth.notNil) { synth.release };
			};
			~famCellTapVoices.clear;
		};

		~famPostHoldUpdate.();
		"FAM hold cleared.".postln;
	}, "/fam/holdclear");

	// Legacy compat: ~famHeldSynths is no longer used but referenced in ~famStop.
	// Keep an empty array so the existing cleanup code doesn't break.
	~famHeldSynths = [];

	// ===================================================================
	// v29-i: SUSTAINED PAD LAYER
	// ===================================================================
	~famPadVoices    = [];     // currently sustaining famPadCore synths
	~famSubBassVoice = nil;    // currently sustaining famSubBass synth
	~famDeepBassVoice = nil;   // v29-o: felt-bass body layer (above SubBass)
	~famPadEnabled   = true;   // master switch for pad layer

	// v29-j: pad voice character (independent of motif voice)
	//   \warm    -> voiceMix 0.0 (default, pure detuned saw)
	//   \strings -> voiceMix 0.7 (saw + a bit of pulse, brighter)
	//   \brass   -> voiceMix 1.6 (pulse-dominant, harder edge)
	~famPadVoiceCharacter = \warm;

	~famPadVoiceMixes = (
		\warm:    0.0,
		\strings: 0.6,    // v29-k: was 0.7, slightly less aggressive
		\brass:   1.2     // v29-k: was 1.6 (60% pulse, keeps saw body)
	);

	// v29-j: chord octave register for placement
	//   \low  -> shift all tones down by 12
	//   \mid  -> default (~famVoiceCenter)
	//   \high -> shift all tones up by 12
	~famPadOctave = \mid;
	~famPadOctaveShifts = ( \low: -12, \mid: 0, \high: 12 );

	// v29-j: prime-ratio bar lengths so progression timing doesn't loop
	// at simple multiples. Cycling through these gives subtly varied
	// chord-change timing -- 3 bars, 5 bars, 4 bars, 7 bars, etc.
	// v29-m: DEFAULT OFF -- chord changes too often felt unsettled.
	// Toggle on with ~famUsePrimeBars = true if you want variation.
	~famUsePrimeBars = false;
	~famPrimeBarLengths = [4, 6, 5, 8, 6, 4];   // v29-m: longer, more stable
	~famPrimeBarIdx = 0;

	// v29-m: master switch for ALL glitter/bell layers (auto-motifs + air task)
	// Default OFF. The pad alone is the sound. Set to true to bring back the
	// FM glitter tones over the pad.
	~famGlitterEnabled = false;

	// v29-n: live performance controls
	// v29-o: defaults shifted from "neutral" to "warm/dark/textured" per
	// the Sound Constitution. Encoders still go to 0/0 if user wants clean.
	~famDrive      = 0.18;    // 0.0..1.0 -- default warm body, never clean-digital
	~famBrightness = -0.2;    // -1.0..+1.0 -- default slightly dark
	~famTexture    = 0.0;     // 0.0..1.0 -- non-melodic atmospheric bed
	~famTextureSynth = nil;   // sustained texture synth handle

	// v29-n: setters that update live synths (no respawn needed)
	~famSetDrive = { |val|
		// v29-o: tanh saturation in famReverbMaster is already self-limiting,
		// but we clip the input range too. Constitutional rule: never harsh.
		// Drive 0..1 maps internally to tanh pre-gain 1..5 (in the SynthDef).
		// Hard cap at 1.0 means max pre-gain is 5x — beyond this would alias.
		// Also routes to famDeepBass for warmth: more drive = warmer bass body.
		~famDrive = val.clip(0.0, 1.0);
		if(~famVerbSynth.notNil) {
			~famVerbSynth.set(\drive, ~famDrive);
		};
		// v29-o: bass gets a softer drive curve (max 0.4) so it never distorts
		if(~famDeepBassVoice.notNil) {
			~famDeepBassVoice.set(\drive, (0.15 + ~famDrive * 0.25).clip(0.0, 0.4));
		};
		("FAM drive: " ++ (~famDrive * 100).round(1) ++ "%").postln;
	};

	~famSetBrightness = { |val|
		var hzOffset, safeOffset;
		~famBrightness = val.clip(-1.0, 1.0);
		// Map -1..+1 to -700..+1500 Hz (asymmetric)
		hzOffset = if(~famBrightness >= 0) {
			~famBrightness * 1500
		} {
			~famBrightness * 700
		};
		// v29-o: ULTRA-IMPORTANT safe-range clamp.
		// Constitutional rule: "no high frequencies that could turn ugly."
		// Pad cutoffBase typically sits around 700-1500 Hz, so:
		//   max effective cutoff = 1500 + 1500 = 3000 Hz (safe ceiling at ~4200)
		//   min effective cutoff = 700  -  700 =    0 Hz (very dark; OK musically)
		// Even at extreme settings, NO resonance peaks above 3kHz.
		safeOffset = hzOffset.clip(-1000, 2700);   // hard ceiling
		// Apply to all currently-sustaining pad voices
		~famPadVoices.do { |s| if(s.notNil) { s.set(\cutoffOffset, safeOffset) } };
		~famCellTapVoices.do { |s| if(s.notNil) { s.set(\cutoffOffset, safeOffset) } };
		~famCellHolds.do { |synths|
			synths.do { |s| if(s.notNil) { s.set(\cutoffOffset, safeOffset) } };
		};
		("FAM brightness: " ++ ~famBrightness.round(0.01)
		 ++ " (offset: " ++ safeOffset.round(1) ++ " Hz, clamped)").postln;
	};

	// v29-o: TEXTURE — non-melodic atmospheric bed (brown noise + filter)
	// 0.0 = silent, 1.0 = audible but never overpowering (capped at 0.12 amp)
	// Spawns the synth on first non-zero call, keeps it sustaining.
	~famSetTexture = { |val|
		var safeVal, targetAmp;
		~famTexture = val.clip(0.0, 1.0);
		// Map 0..1 to 0..0.12 amp (texture should always sit BEHIND the pad)
		targetAmp = ~famTexture * 0.12;
		if(~famRunning and: { ~famTextureSynth.isNil and: { ~famTexture > 0.001 } }) {
			// Spawn texture synth lazily if not yet alive
			~famTextureSynth = Synth(\famTexture, [
				\amp,      targetAmp,
				\gate,     1,
				\verbOut,  ~famVerbBus.index,
				\verbSend, 0.55,
				\bpCenter, 600,
				\bpQ,      0.4,
				\lfoRate,  0.04
			], ~famMotifGroup ? s);
		} {
			if(~famTextureSynth.notNil) {
				~famTextureSynth.set(\amp, targetAmp);
			};
		};
		("FAM texture: " ++ (~famTexture * 100).round(1) ++ "%").postln;
	};

	// v29-o: SPACE — macro control combining reverb tail length + size + wet
	// 0.0 = small intimate space (t60 1.5s, size 0.5, wet 0.35)
	// 0.5 = current default cinematic (t60 4.5s, size 0.85, wet 0.55)
	// 1.0 = vast cathedral (t60 8.0s, size 1.0, wet 0.7)
	// Constitutional safety: even at 1.0, no resonance peaks, no harsh tail.
	~famSpace = 0.5;   // default = current v29-o character
	~famSetSpace = { |val|
		var safeVal, t60val, sizeval, wetval;
		~famSpace = val.clip(0.0, 1.0);
		// Map 0..1 to safe parameter ranges
		t60val  = ~famSpace.linlin(0.0, 1.0, 1.5, 8.0);
		sizeval = ~famSpace.linlin(0.0, 1.0, 0.5, 1.0);
		wetval  = ~famSpace.linlin(0.0, 1.0, 0.35, 0.70);
		if(~famVerbSynth.notNil) {
			~famVerbSynth.set(
				\t60,    t60val,
				\size,   sizeval,
				\wetAmp, wetval
			);
		};
		("FAM space: " ++ (~famSpace * 100).round(1) ++ "%"
		 ++ "  t60=" ++ t60val.round(0.1)
		 ++ "s  size=" ++ sizeval.round(0.01)
		 ++ "  wet=" ++ wetval.round(0.01)).postln;
	};

	// v29-o: DEPTH — macro control for low-end weight & sustain.
	// 0.0 = light (sub barely there, deepbass quiet, pad short release)
	// 0.5 = current default character
	// 1.0 = massive (sub & deepbass loud, very long pad release)
	// Constitutional safety: bass amps capped to safe range (<0.30 each).
	~famDepth = 0.5;
	~famSetDepth = { |val|
		var subAmp, deepAmp, padReleaseScale;
		~famDepth = val.clip(0.0, 1.0);
		// Map 0..1 to bass amps. Default=0.5 maps to current values.
		subAmp  = ~famDepth.linlin(0.0, 1.0, 0.06, 0.20);   // v29-o-tuned: less dominant
		deepAmp = ~famDepth.linlin(0.0, 1.0, 0.04, 0.16);   // v29-o-tuned: less dominant
		// Apply live to existing bass voices
		if(~famSubBassVoice.notNil)  { ~famSubBassVoice.set(\amp, subAmp) };
		if(~famDeepBassVoice.notNil) { ~famDeepBassVoice.set(\amp, deepAmp) };
		// Note: pad-release cannot be changed live (it's set on synth spawn).
		// New pad voices at next chord change will pick up depth-adjusted release
		// via ~famDepthPadRelease if we choose to consume it in ~famSpawnPad.
		~famDepthPadRelease = ~famDepth.linlin(0.0, 1.0, 2.0, 5.5);
		("FAM depth: " ++ (~famDepth * 100).round(1) ++ "%"
		 ++ "  sub=" ++ subAmp.round(0.01)
		 ++ "  deep=" ++ deepAmp.round(0.01)
		 ++ "  release=" ++ ~famDepthPadRelease.round(0.1) ++ "s").postln;
	};
	~famDepthPadRelease = 3.0;   // initial value, used by ~famSpawnPad

	// v29-o: BLOOM — macro control for pad attack-swell & reverb-wash drama.
	// 0.0 = no swell, instant pad presence (intimate)
	// 0.5 = current default subtle bloom
	// 1.0 = huge orchestral swell with extra reverb wash
	// Constitutional safety: even at 1.0, attack stays <8s, fenvAmount <0.45.
	~famBloom = 0.5;
	~famSetBloom = { |val|
		~famBloom = val.clip(0.0, 1.0);
		// Map values that ~famSpawnPad will pick up for next pad spawn.
		// Live-modifying current pad's fenv is not possible (already triggered).
		~famBloomPadAttack    = ~famBloom.linlin(0.0, 1.0, 1.5, 7.0);
		~famBloomFenvAmount   = ~famBloom.linlin(0.0, 1.0, 0.0, 0.30);
		~famBloomReverbBoost  = ~famBloom.linlin(0.0, 1.0, -0.10, 0.20);
		("FAM bloom: " ++ (~famBloom * 100).round(1) ++ "%"
		 ++ "  attack=" ++ ~famBloomPadAttack.round(0.1)
		 ++ "s  fenv=" ++ ~famBloomFenvAmount.round(0.01)
		 ++ "  verbBoost=" ++ ~famBloomReverbBoost.round(0.01)).postln;
	};
	~famBloomPadAttack    = 4.0;
	~famBloomFenvAmount   = 0.15;
	~famBloomReverbBoost  = 0.0;

	// v29-o: side-chain ducking for the texture layer. The famTexture SynthDef
	// has a Lag.kr on amp, so setting amp to a low value snaps it via lag
	// (smooth fade-down over ~80ms). After recoverTime, we set it back to full
	// (smooth fade-up over ~80ms). No clicks, no abrupt drops.
	~famDuckTexture = { |duckAmount = 0.5, recoverTime = 0.8|
		var fullAmp = ~famTexture * 0.12;
		var duckedAmp = fullAmp * duckAmount;
		if(~famTextureSynth.notNil and: { ~famTexture > 0.001 }) {
			~famTextureSynth.set(\amp, duckedAmp);
			AppClock.sched(recoverTime, {
				if(~famTextureSynth.notNil) {
					~famTextureSynth.set(\amp, fullAmp);
				};
				nil
			});
		};
	};

	// v29-o INSTRUMENT MODE: SpawnDrone — soft tonic drone for harmonic
	// reference, like a tanpura. One quiet voice on the tonic, sustained.
	~famSpawnDrone = {
		var rootMidi = ~famKey ? 60;
		var freq = rootMidi.midicps;
		if(~famDroneVoice.notNil) {
			~famDroneVoice.release;
			~famDroneVoice = nil;
		};
		~famDroneVoice = Synth(\famPadCore, [
			\freq,         freq,
			\amp,          0.05,
			\gate,         1,
			\attack,       6.0,
			\release,      4.0,
			\pan,          0,
			\detuneCents,  4,
			\cutoffBase,   650,
			\cutoffMod,    150,
			\cutoffRate,   0.03,
			\lpRes,        0.15,
			\voiceMix,     0,
			\cutoffOffset, 0,
			\fenvAttack,   8.0,
			\fenvDecay,    6.0,
			\fenvSustain,  0.6,
			\fenvAmount,   0.1,
			\verbOut,      ~famVerbBus.index,
			\verbSend,     0.45
		]);
		("FAM drone: ON (tonic " ++ rootMidi ++ ")").postln;
	};

	~famReleaseDrone = {
		if(~famDroneVoice.notNil) {
			~famDroneVoice.release;
			~famDroneVoice = nil;
			"FAM drone: OFF".postln;
		};
	};

	// Spawn a fresh set of pad voices for the current chord.
	~famSpawnPad = { |chord|
		var oldVoices = ~famPadVoices;
		var oldSubBass = ~famSubBassVoice;
		var oldDeepBass = ~famDeepBassVoice;     // v29-o
		var voiced;
		var vMix, octShift;
		var moodCutoffBase;
		var brightnessHz;     // v29-n

		if(~famPadEnabled.not) { ^nil };

		// Voice-lead the chord -- place near voice center for warmth
		// v29-j: apply octave shift on top of voice leading
		octShift = ~famPadOctaveShifts[~famPadOctave] ? 0;
		voiced = chord.collect { |n|
			var meanCenter = chord.mean;
			var shift = ((~famVoiceCenter - meanCenter) / 12).round * 12;
			n + shift + octShift
		};

		// v29-j: pad voice character mix
		vMix = ~famPadVoiceMixes[~famPadVoiceCharacter] ? 0.0;

		// v29-j: mood drives cutoff base -- low mood (0.0) = darker (~700Hz),
		// high mood (1.0) = brighter (~1500Hz). LFO and ADSR add motion on top.
		moodCutoffBase = 700 + (~famMood * 800);

		// v29-n: pick up current brightness so new voices match the player's setting
		brightnessHz = if(~famBrightness >= 0) { ~famBrightness * 1500 } { ~famBrightness * 700 };

		// Spawn pad voices for each chord tone
		~famPadVoices = voiced.collect { |midi, i|
			var pan = (i - (voiced.size / 2 - 0.5)) * 0.4;
			Synth(\famPadCore, [
				\freq,         midi.midicps,
				\amp,          0.10,
				\gate,         1,
				\pan,          pan.clip(-0.85, 0.85),
				\detuneCents,  7 + 4.0.rand,
				\attack,       1.5 + 0.5.rand,
				\release,      ~famDepthPadRelease ? 3.0,    // v29-o: depth-controlled release
				\cutoffBase,   moodCutoffBase + 200.rand,
				\cutoffMod,    500,
				\cutoffRate,   0.05 + 0.05.rand,
				\lpRes,        0.18,
				\voiceMix,     vMix,
				\cutoffOffset, brightnessHz,    // v29-n
				// v29-o: ULTRA-slow filter bloom. Now driven by ~famBloom macro.
				// At Bloom=0.5 (default), attack=4-5s, fenvAmount=0.25 (subtle).
				// At Bloom=1.0, attack=7s, fenvAmount=0.45 (orchestral swell).
				\fenvAttack,   (~famBloomPadAttack ? 4.0) + 1.0.rand,
				\fenvDecay,    5.0,
				\fenvSustain,  0.7,
				\fenvAmount,   ~famBloomFenvAmount ? 0.25,
				\verbOut,      ~famVerbBus.index,
				\verbSend,     (0.5 + (~famBloomReverbBoost ? 0.0)).clip(0.2, 0.85)
			]);
		};

		// v29-o: SubBass on chord root, TWO octaves below lowest voiced note
		// Pure sine, very low (30-60 Hz). The "feel" — fundamental body.
		// Long attack (3s) so the bass blooms in, never plops.
		// Amp now driven by ~famDepth macro (default 0.5 → amp=0.22).
		~famSubBassVoice = Synth(\famSubBass, [
			\freq,     (voiced.minItem - 24).midicps,
			\amp,      ((~famDepth ? 0.5).linlin(0.0, 1.0, 0.06, 0.20)),
			\gate,     1,
			\attack,   3.0,
			\release,  4.0,
			\lpCutoff, 90,                                // v29-o: was 200 -- pure sub, no mids
			\verbOut,  ~famVerbBus.index,
			\verbSend, 0.03                               // v29-o: less verb on sub (cleaner)
		]);

		// v29-o: DeepBass on chord root, ONE octave below lowest voiced note.
		// Sine + subtle saw harmonics, gentle saturation. The "warmth/body".
		// Sits between SubBass (sub region) and Pad (mid region) in the spectrum.
		~famDeepBassVoice = Synth(\famDeepBass, [
			\freq,     (voiced.minItem - 12).midicps,    // octave below pad
			\amp,      ((~famDepth ? 0.5).linlin(0.0, 1.0, 0.04, 0.16)),
			\gate,     1,
			\attack,   2.5,
			\release,  3.5,
			\lpCutoff, 350,                              // body, not mud
			\lpRes,    0.15,
			\drive,    0.15,                             // tame saturation
			\verbOut,  ~famVerbBus.index,
			\verbSend, 0.08
		]);

		// Release the old voices -- they crossfade with the new ones
		oldVoices.do { |s| if(s.notNil) { s.release } };
		if(oldSubBass.notNil) { oldSubBass.release };
		if(oldDeepBass.notNil) { oldDeepBass.release };
	};

	// Release all currently-sustaining pad voices (graceful fadeout)
	~famReleasePad = {
		~famPadVoices.do { |s| if(s.notNil) { s.release } };
		~famPadVoices = [];
		if(~famSubBassVoice.notNil) {
			~famSubBassVoice.release;
			~famSubBassVoice = nil;
		};
		if(~famDeepBassVoice.notNil) {       // v29-o
			~famDeepBassVoice.release;
			~famDeepBassVoice = nil;
		};
	};

	// Hard-cut all pad voices (used at full Stop after fade)
	~famFreePad = {
		~famPadVoices.do { |s| if(s.notNil) { s.free } };
		~famPadVoices = [];
		if(~famSubBassVoice.notNil) {
			~famSubBassVoice.free;
			~famSubBassVoice = nil;
		};
		if(~famDeepBassVoice.notNil) {
			~famDeepBassVoice.free;
			~famDeepBassVoice = nil;
		};
		// v29-o: drone hard-free at full stop
		if(~famDroneVoice.notNil) {
			~famDroneVoice.free;
			~famDroneVoice = nil;
		};
	};

	// Harmonic Brain controls
	OSCdef(\famKey, { |msg|
		~famKey = msg[1].asInteger.clip(36, 84);
		// v29-o: respawn drone on new tonic if active
		if(~famRunning and: { ~famDrone } and: { ~famDroneVoice.notNil }) {
			~famSpawnDrone.();
		};
		("FAM key: " ++ ~famKey).postln;
		~famPushState.();
	}, "/fam/key");

	OSCdef(\famMode, { |msg|
		var idx = msg[1].asInteger.clip(0, 5);     // v29-o: was 0..6, now 0..5 (no locrian)
		// v29-o: \locrian removed from user choices (constitutional rule:
		// never dissonant). Still defined in ~famScales for legacy.
		var modes = [\ionian, \dorian, \phrygian, \lydian,
		             \mixolydian, \aeolian];
		~famMode = modes[idx];
		~famUpdateReverb.();
		("FAM mode: " ++ ~famMode).postln;
		~famPushState.();
	}, "/fam/mode");

	OSCdef(\famVibe, { |msg|
		var idx = msg[1].asInteger.clip(0, 3);   // v29-o: 4 safe vibes (was 5, sharp dropped)
		~famVibe = ~famVibeKeys[idx];
		~famUpdateReverb.();
		("FAM vibe: " ++ ~famVibe ++ "  family="
		 ++ (~famVibes[~famVibe][\family])).postln;
		~famPushState.();
	}, "/fam/vibe");

	OSCdef(\famVoice, { |msg|
		var idx = msg[1].asInteger.clip(0, 2);
		var voices = [\fm, \saw, \square];
		~famCoreVoice = voices[idx];
		("FAM voice: " ++ ~famCoreVoice).postln;
		~famPushState.();
	}, "/fam/voice");

	OSCdef(\famMidi, { |msg|
		var on = (msg[1] ? 0).asInteger > 0;
		var ch = (msg[2] ? 0).asInteger.clip(0, 15);
		~famMidiChannel = ch;
		if(on) {
			if(~famMidiOut.isNil) {
				MIDIClient.init(0, 1);
				if(MIDIClient.destinations.size > 0) {
					~famMidiOut = MIDIOut(0);
					~famMidiOut.latency = 0;
					~famMidiEnabled = true;
					("FAM midi: ON  port=" ++ MIDIClient.destinations[0].name
					 ++ "  channel=" ++ ch).postln;
				} {
					"FAM midi: NO destinations available".postln;
					~famMidiEnabled = false;
				};
			} {
				~famMidiEnabled = true;
				("FAM midi: re-enabled  channel=" ++ ch).postln;
			};
		} {
			~famMidiEnabled = false;
			"FAM midi: OFF".postln;
		};
		~famPushState.();
	}, "/fam/midi");

	OSCdef(\famProg, { |msg|
		var idx = msg[1].asInteger;
		if(idx < 0) {
			~famProgAuto = true;
			~famAutoCurrentDegree = 1;
			("FAM progression: AUTO (Markov)").postln;
		} {
			~famProgAuto = false;
			~famProg = ~famProgKeys[idx.clip(0, 4)];   // v29-o: 5 progressions now
			~famProgStep = 0;
			("FAM progression: " ++ ~famProg).postln;
		};
		~famPushState.();
	}, "/fam/prog");

	OSCdef(\famTempo, { |msg|
		// v29-o: BPM range tightened to 40..90 (Constitution: never fast).
		// Above 90 BPM the slow-bed concept breaks down -- arps and modulation
		// rates become nervous. Below 40 is too sluggish for chord motion.
		~famBPM = msg[1].asFloat.clip(40, 90);
		("FAM BPM: " ++ ~famBPM).postln;
		~famPushState.();
	}, "/fam/tempo");

	// v29-j: mood multi-target. Sets ~famMood and live-tweaks pad cutoff,
	// reverb size, and a small tempo nudge -- all at once.
	OSCdef(\famMood, { |msg|
		~famMood = msg[1].asFloat.clip(0.0, 1.0);

		// 1) Pad cutoff: live-set on all sustaining pad voices
		if(~famPadVoices.notNil) {
			var moodCutoffBase = 700 + (~famMood * 800);
			~famPadVoices.do { |s|
				if(s.notNil) { s.set(\cutoffBase, moodCutoffBase) };
			};
		};

		// 2) Reverb size: subtle nudge on master reverb
		if(~famVerbSynth.notNil) {
			var moodSize = 0.55 + (~famMood * 0.30);   // 0.55..0.85
			~famVerbSynth.set(\size, moodSize);
		};

		~famPushState.();
	}, "/fam/mood");

	OSCdef(\famHoldFlag, { |msg|
		~famHold = (msg[1].asInteger > 0);
		("FAM hold: " ++ ~famHold).postln;
	}, "/fam/holdflag");

	// v29-h: master volume  /fam/vol [0.0 .. 1.0]
	OSCdef(\famVol, { |msg|
		~famSetMasterVol.(msg[1].asFloat);
		~famPushState.();
	}, "/fam/vol");

	// v29-h: audio-jack detect from Pico  /fam/jackdetect [0|1]
	// When jack is inserted (1), built-in amp goes silent automatically.
	OSCdef(\famJackDetect, { |msg|
		var inserted = (msg[1].asInteger > 0);
		~famJackInserted = inserted;
		~famAmpEnabled = inserted.not;
		// Re-apply masterVol with new amp state
		if(~famVerbSynth.notNil) {
			var effective = if(~famAmpEnabled) { ~famMasterVol } { 0.0 };
			~famVerbSynth.set(\masterVol, effective);
		};
		("FAM jack: " ++ if(inserted) { "INSERTED (amp off)" } { "REMOVED (amp on)" }).postln;
		~famPushState.();
	}, "/fam/jackdetect");

	// v29-h: explicit amp shutdown  /fam/ampshutdown [0|1]
	// Independent of jack-detect; for manual mute or panic-stop.
	OSCdef(\famAmpShutdown, { |msg|
		var shutdown = (msg[1].asInteger > 0);
		~famAmpEnabled = shutdown.not;
		if(~famVerbSynth.notNil) {
			var effective = if(~famAmpEnabled) { ~famMasterVol } { 0.0 };
			~famVerbSynth.set(\masterVol, effective);
		};
		("FAM amp: " ++ if(~famAmpEnabled) { "ON" } { "SHUTDOWN" }).postln;
		~famPushState.();
	}, "/fam/ampshutdown");

	// v29-j: pad voice character  /fam/padvoice [0|1|2]
	// 0 = warm (default saw), 1 = strings, 2 = brass
	OSCdef(\famPadVoice, { |msg|
		var idx = msg[1].asInteger.clip(0, 2);
		~famPadVoiceCharacter = [\warm, \strings, \brass][idx];
		// Live-update existing pad voices via .set
		if(~famPadVoices.notNil) {
			var vMix = ~famPadVoiceMixes[~famPadVoiceCharacter] ? 0.0;
			~famPadVoices.do { |s| if(s.notNil) { s.set(\voiceMix, vMix) } };
		};
		("FAM pad voice: " ++ ~famPadVoiceCharacter).postln;
		~famPushState.();
	}, "/fam/padvoice");

	// v29-j: pad octave  /fam/padoctave [0|1|2]
	// 0 = low, 1 = mid (default), 2 = high
	// Takes effect on next chord change (or immediately re-spawns pad if running).
	OSCdef(\famPadOctave, { |msg|
		var idx = msg[1].asInteger.clip(0, 2);
		~famPadOctave = [\low, \mid, \high][idx];
		// Re-spawn pad immediately so the user hears the change
		if(~famRunning and: { ~famPadVoices.size > 0 }) {
			~famSpawnPad.(~famCurrentChord.());
		};
		("FAM pad octave: " ++ ~famPadOctave).postln;
		~famPushState.();
	}, "/fam/padoctave");

	// v29-n: live performance controls
	// /fam/drive [0.0 .. 1.0]
	OSCdef(\famDrive, { |msg|
		~famSetDrive.(msg[1].asFloat);
		~famPushState.();
	}, "/fam/drive");

	// /fam/brightness [-1.0 .. 1.0]
	OSCdef(\famBrightness, { |msg|
		~famSetBrightness.(msg[1].asFloat);
		~famPushState.();
	}, "/fam/brightness");

	// v29-o: /fam/texture [0.0 .. 1.0] -- atmospheric bed amount
	OSCdef(\famTexture, { |msg|
		~famSetTexture.(msg[1].asFloat);
		~famPushState.();
	}, "/fam/texture");

	// v29-o: /fam/space [0.0 .. 1.0] -- macro reverb (size + tail + wet)
	OSCdef(\famSpace, { |msg|
		~famSetSpace.(msg[1].asFloat);
		~famPushState.();
	}, "/fam/space");

	// v29-o: /fam/depth [0.0 .. 1.0] -- macro low-end (sub + deep + release)
	OSCdef(\famDepth, { |msg|
		~famSetDepth.(msg[1].asFloat);
		~famPushState.();
	}, "/fam/depth");

	// v29-o: /fam/bloom [0.0 .. 1.0] -- macro pad swell (attack + fenv + verb)
	OSCdef(\famBloom, { |msg|
		~famSetBloom.(msg[1].asFloat);
		~famPushState.();
	}, "/fam/bloom");

	// v29-g: ~famPushState -- send a full state snapshot to the bridge so the
	// Panel can stay in sync. Called automatically after every state-changing
	// endpoint, plus on demand via /fam/query.
	~famPushState = {
		var deg = ~famCurrentDegree.();
		var chord = ~famCurrentChordRaw.();
		var voiceIdx = [\fm, \saw, \square].indexOf(~famCoreVoice) ? 0;
		var vibeIdx  = ~famVibeKeys.indexOf(~famVibe) ? 0;
		var modeIdx  = [\ionian, \dorian, \phrygian, \lydian,
		                \mixolydian, \aeolian, \locrian].indexOf(~famMode) ? 0;
		var progIdx  = if(~famProgAuto) { -1 } { ~famProgKeys.indexOf(~famProg) ? 0 };
		NetAddr("127.0.0.1", 57121).sendMsg("/fam/state",
			~famRunning.asInteger,
			~famAutoPlay.asInteger,
			~famKey,
			modeIdx,
			progIdx,
			deg.asInteger,
			~famBPM,
			~famMood,
			~famHold.asInteger,
			vibeIdx,
			voiceIdx,
			~famMidiEnabled.asInteger,
			chord[0] ? 0, chord[1] ? 0, chord[2] ? 0,
			chord[3] ? 0, chord[4] ? 0,
			// v29-h additions
			~famMasterVol,
			~famAmpEnabled.asInteger,
			~famJackInserted.asInteger,
			// v29-j additions
			[\warm, \strings, \brass].indexOf(~famPadVoiceCharacter) ? 0,
			[\low, \mid, \high].indexOf(~famPadOctave) ? 1,
			// v29-n additions
			~famDrive,
			~famBrightness,
			// v29-o additions
			~famTexture,
			~famSpace,
			~famDepth,
			~famBloom,
			~famGenerative.asInteger,
			~famDrone.asInteger
		);
	};

	// State-query endpoint
	OSCdef(\famQuery, { |msg| ~famPushState.() }, "/fam/query");

	"".postln;
	"================================================================".postln;
	" FIELD AMBIENCE v29-o — playable instrument".postln;
	"".postln;
	"   This is an INSTRUMENT, not an automatic sound generator.".postln;
	"   The device makes no sound on its own. Cells are your voice.".postln;
	"".postln;
	"   ~famStart.()                  power on (silent until you play)".postln;
	"   ~famStop.()                   power off".postln;
	"".postln;
	" Playing it:".postln;
	"   /fam/cell [degree]            tap a chord (stops on release)".postln;
	"   /fam/hold [degree, 1]         hold a chord (sustains)".postln;
	"   /fam/holdoff [degree]         release a held chord".postln;
	"   /fam/holdclear                release everything".postln;
	"".postln;
	" Optional helpers (default OFF):".postln;
	"   /fam/drone [0/1]              soft tonic drone (tanpura-like)".postln;
	"   /fam/generative [0/1]         auto-progression bed (the old behavior)".postln;
	"".postln;
	" Sound character:".postln;
	"   /fam/key /fam/mode /fam/vibe  set tonality".postln;
	"   /fam/drive /fam/brightness    live performance shaping".postln;
	"   /fam/texture /fam/space       atmosphere + reverb".postln;
	"   /fam/depth /fam/bloom         macro low-end + macro pad swell".postln;
	"".postln;
	("   Reverb backend: " ++ if(~famHasJPverb) { "JPverb" } { "FreeVerb2 (fallback)" }).postln;
	"================================================================".postln;
});
)
