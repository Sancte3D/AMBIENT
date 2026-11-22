#!/usr/bin/env python3
"""Hoerproben: Shepard-Risset-Trick fuer AMBIENT (Besitzer-Wunsch 22.11.2026).

KEIN Firmware-Klang: reine Host-Skizze in numpy, deterministisch. Nichts davon laeuft
durch engine_render(); Rechenzeit und Echtzeitregeln sind hier nicht geprueft.

Prinzip: mehrere Kopien desselben Klangs im Oktavabstand bewegen sich gemeinsam.
Ihre Lautstaerke haengt nur von der Tonhoehe ab (Glockenkurve ueber der Oktavlage):
in der Mitte laut, ganz oben und ganz unten unhoerbar. Wer unten verschwindet,
taucht oben unhoerbar wieder auf. Das Ohr hoert endloses Sinken.

  A  shepard_glide   gleitend, absteigend, eine Oktave in 40 s
                     (Hoerprobe schneller als am Geraet geplant: dort 3-5 min je Oktave)
  B  shepard_steps   Stufenleiter auf dem Tonvorrat (Moll-Pentatonik rein auf D),
                     absteigend, alle Oktaven gleichzeitig, ein Schritt alle 4 s.
                     Jeder Ton liegt im Tonvorrat: kein Halbton, kein Tritonus,
                     deshalb darf ein Grundton-Bett darunter liegen.

Vorgaben: Synthesizer-Toene mit harmonischen Teiltoenen und weichem Anschlag,
keine Glockenspektren, kein Rauschen ausser im Hall der Skizze.

    python3 sketch_shepard.py <ausgabeordner>     # schreibt shepard_glide.wav, shepard_steps.wav
"""
import sys, os, wave
import numpy as np

SR = 44100; DUR = 62.0; N = int(SR * DUR); T = np.arange(N) / SR
D3 = 146.832

def fft_filter(x, shape):
    X = np.fft.rfft(x, axis=0); f = np.fft.rfftfreq(len(x), 1 / SR)
    g = shape(f); return np.fft.irfft(X * (g[:, None] if x.ndim == 2 else g), n=len(x), axis=0)
def lowpass(x, hz, order=2): return fft_filter(x, lambda f: 1 / np.sqrt(1 + (f / hz) ** (2 * order)))
def highpass(x, hz, order=2): return fft_filter(x, lambda f: 1 / np.sqrt(1 + (hz / np.maximum(f, 1e-3)) ** (2 * order)))
def reverb(x, t60, bright_hz, predelay=0.03, seed=7):
    r = np.random.default_rng(seed); n = int(SR * t60 * 1.3); t = np.arange(n) / SR
    ir = r.standard_normal((n, 2)) * np.exp(-6.908 * t / t60)[:, None]
    ir = lowpass(ir, bright_hz); ir[: int(.004 * SR)] *= np.linspace(0, 1, int(.004 * SR))[:, None]
    ir /= np.sqrt((ir ** 2).sum(0)).mean()
    m = len(x) + n; Lf = 1 << (m - 1).bit_length(); out = np.zeros_like(x); pd = int(predelay * SR)
    for c in range(2):
        y = np.fft.irfft(np.fft.rfft(x[:, c], Lf) * np.fft.rfft(ir[:, c], Lf), Lf)[: len(x)]
        out[pd:, c] = y[: len(x) - pd]
    return out
def finish(buf, peak_db=-3.0):
    buf = highpass(buf, 40); f = int(2.5 * SR); buf[-f:] *= np.linspace(1, 0, f)[:, None] ** 2
    fi = int(1.5 * SR); buf[:fi] *= np.linspace(0, 1, fi)[:, None]
    return buf * (10 ** (peak_db / 20) / np.abs(buf).max())
def write(path, buf):
    with wave.open(path, 'wb') as w:
        w.setnchannels(2); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes((np.clip(buf, -1, 1) * 32767).astype('<i2').tobytes())

# Glockenkurve ueber der Oktavlage: Mitte bei 330 Hz, Breite 1,1 Oktaven (log2-Gauss).
F_LO = 40.0; OCT = 8; CENTER = np.log2(330 / F_LO); SIGMA = 1.1
def bell(x): return np.exp(-0.5 * ((x - CENTER) / SIGMA) ** 2)
HARM = (1.0, 0.30, 0.10)          # weicher Synth-Ton: Grundton, Oktave, Duodezime

def shepard_glide(sec_per_octave=40.0):
    """Gleitend, absteigend. Jede Stimme k liegt bei x = k - t/T (mod OCT) Oktaven ueber F_LO."""
    out = np.zeros((N, 2)); rng = np.random.default_rng(3)
    pos = -T / sec_per_octave
    for k in range(OCT):
        for det, pan in ((-0.0025, 0.32), (0.0025, 0.68)):      # zwei Kopien +/- 4 Cent: Wogen und Breite
            x = np.mod(k + pos, OCT)
            f = F_LO * 2 ** (x + det)
            amp = bell(x)
            ph = 2 * np.pi * np.cumsum(f) / SR + rng.uniform(0, 2 * np.pi)
            sig = sum(a * np.sin(h * ph) for h, a in enumerate(HARM, start=1))
            sig *= amp / OCT
            out[:, 0] += sig * np.cos(pan * np.pi / 2); out[:, 1] += sig * np.sin(pan * np.pi / 2)
    out = lowpass(out, 2500, 2)
    wet = reverb(out, 4.5, 3000)
    return finish(0.65 * out + 0.55 * wet)

# Moll-Pentatonik in reiner Stimmung auf D: D F G A C
RATIOS = (1.0, 6 / 5, 4 / 3, 3 / 2, 9 / 5)
def tone(hz, dur, attack=0.5, decay=5.0):
    n = int(dur * SR); t = np.arange(n) / SR
    env = np.minimum(1, t / attack) ** 2 * np.exp(-np.maximum(0, t - attack) / decay)
    sig = sum(a * np.sin(2 * np.pi * hz * h * t) for h, a in enumerate((1.0, 0.35, 0.12, 0.05), start=1) if hz * h < 7000)
    return sig * env
def shepard_steps(step_sec=4.0):
    """Stufenleiter: Schritt s laeuft rueckwaerts durch die fuenf Toene; jeder Schritt spielt
    den Ton in allen Oktaven, gewichtet mit der Glockenkurve an seiner Tonhoehe."""
    out = np.zeros((N, 2)); rng = np.random.default_rng(5)
    nsteps = int(DUR / step_sec)
    for s in range(nsteps):
        idx = -s                                    # absteigend
        deg = idx % 5; octs = idx // 5               # Oktavlage wandert mit
        t0 = s * step_sec + rng.uniform(-0.08, 0.08)
        base = np.log2(D3 / 4 * RATIOS[deg] / F_LO) + octs
        for k in range(OCT):
            # Lage der Stimme in Oktaven ueber F_LO, zyklisch: so verschwindet unten, was oben erscheint
            x = np.mod(base + k, OCT)
            hz = F_LO * 2 ** x
            g = bell(x)
            if g < 0.01: continue
            sig = tone(hz, min(9.0, DUR - t0)) * g
            pan = 0.5 + 0.22 * np.sin(1.7 * k + 0.6 * s)
            i = int(t0 * SR); sig = sig[: N - i]
            out[i:i + len(sig), 0] += sig * np.cos(pan * np.pi / 2)
            out[i:i + len(sig), 1] += sig * np.sin(pan * np.pi / 2)
    # leises Bett auf dem Grundton und der Quinte (D2, A2), langsam wogend
    bedsig = np.zeros((N, 2))
    for hz, lvl in ((D3 / 2, 0.10), (D3 * 3 / 4, 0.05)):
        for off, pan in ((-0.6, 0.3), (0.6, 0.7)):
            w = 2 * np.pi * (hz + off) * T
            s_ = lvl * (np.sin(w) + 0.25 * np.sin(2 * w))
            bedsig[:, 0] += s_ * np.cos(pan * np.pi / 2); bedsig[:, 1] += s_ * np.sin(pan * np.pi / 2)
    swell = 0.75 + 0.25 * np.sin(2 * np.pi * T / 9.0)
    out = out + bedsig * swell[:, None] * np.minimum(1, T / 6)[:, None]
    out = lowpass(out, 3000, 2)
    wet = reverb(out, 5.0, 3200, seed=11)
    return finish(0.6 * out + 0.6 * wet)

if __name__ == '__main__':
    d = sys.argv[1] if len(sys.argv) > 1 else '.'
    os.makedirs(d, exist_ok=True)
    for name, fn in (('shepard_glide', shepard_glide), ('shepard_steps', shepard_steps)):
        buf = fn(); write(os.path.join(d, name + '.wav'), buf)
        print(name, 'peak', round(20 * np.log10(np.abs(buf).max()), 1), 'dBFS', 'nonfinite', int((~np.isfinite(buf)).sum()))
