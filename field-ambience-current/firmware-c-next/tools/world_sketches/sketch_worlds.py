#!/usr/bin/env python3
"""Hoerskizzen fuer drei neue World-Kandidaten (Schritt 1 des Worlds-Auftrags), Fassung 2.

KEIN Firmware-Klang: reine Host-Skizzen in numpy. Nichts davon laeuft durch
engine_render(); Pegel, Rechenzeit und Echtzeitregeln sind hier nicht geprueft.
Deterministisch (feste Seeds).

Fassung 2 folgt den Prinzipien aus der vermessenen Referenzaufnahme
(docs/audio/AMBIENT_REFERENCE_ANALYSIS.md). Jede World hat vier Schichten:

  Bett    viele leicht verstimmte Teiltoene auf dem Tonvorrat, links und rechts
          eigene Stimmen: das innere Wogen (0,5-2 s) und die Breite
  Luft    gefaerbtes Rauschen, links/rechts unabhaengig, kommt und geht (8-120 s)
  Tupfer  seltene, leise, hohe Ereignisse aus der Natur des Ortes
  Toene   die Klangquelle der World, nur wenige je Minute

    python3 sketch_worlds.py <ausgabeordner>      # schreibt cave/dunes/jungle .wav
"""
import sys, wave
import numpy as np
SR = 44100; DUR = 40.0; N = int(SR * DUR); D3 = 146.832; T = np.arange(N) / SR

def env_ad(n, attack, decay):
    t = np.arange(n) / SR
    return np.minimum(1.0, t / max(attack, 1e-4)) * np.exp(-np.maximum(0, t - attack) / decay)
def place(buf, sig, t0, pan=0.5, gain=1.0):
    i = int(t0 * SR)
    if i >= N or i < 0: return
    sig = sig[: N - i] * gain
    buf[i:i + len(sig), 0] += sig * np.cos(pan * np.pi / 2)
    buf[i:i + len(sig), 1] += sig * np.sin(pan * np.pi / 2)
def fft_filter(x, shape):
    X = np.fft.rfft(x, axis=0); f = np.fft.rfftfreq(len(x), 1 / SR)
    g = shape(f); return np.fft.irfft(X * (g[:, None] if x.ndim == 2 else g), n=len(x), axis=0)
def lowpass(x, hz, order=2): return fft_filter(x, lambda f: 1 / np.sqrt(1 + (f / hz) ** (2 * order)))
def highpass(x, hz, order=2): return fft_filter(x, lambda f: 1 / np.sqrt(1 + (hz / np.maximum(f, 1e-3)) ** (2 * order)))
def bandpass(x, lo, hi, order=2): return highpass(lowpass(x, hi, order), lo, order)
def drift(rng, period, depth=1.0, n=N):
    """Langsame Zufallskurve 0..1 mit typischer Periode in Sekunden."""
    k = int(n / SR / period * 2) + 4
    y = np.interp(np.linspace(0, k - 1, n), np.arange(k), rng.random(k))
    y = lowpass(y, 1.5 / period, 1)
    y = (y - y.min()) / (np.ptp(y) + 1e-9)
    return 1 - depth + depth * y
def reverb(x, t60, bright_hz, predelay=0.02, seed=7):
    """Faltung mit abklingendem Rauschen, links und rechts unabhaengig. Nur fuer die Skizze."""
    r = np.random.default_rng(seed); n = int(SR * t60 * 1.3); t = np.arange(n) / SR
    ir = r.standard_normal((n, 2)) * np.exp(-6.908 * t / t60)[:, None]
    ir = lowpass(ir, bright_hz); ir[: int(.004 * SR)] *= np.linspace(0, 1, int(.004 * SR))[:, None]
    ir /= np.sqrt((ir ** 2).sum(0)).mean()
    m = len(x) + n; Lf = 1 << (m - 1).bit_length(); out = np.zeros_like(x); pd = int(predelay * SR)
    for c in range(2):
        y = np.fft.irfft(np.fft.rfft(x[:, c], Lf) * np.fft.rfft(ir[:, c], Lf), Lf)[: len(x)]
        out[pd:, c] = y[: len(x) - pd]
    return out

def bed(rng, tones, harmonics, beat=(0.5, 2.0), voices=2, slow=(12, 40)):
    """Das Bett: je Ton `voices` Oszillatoren, 0,5 bis 2 Hz auseinander. Sie schweben
    gegeneinander: das ist das Wogen. Der rechte Kanal spielt dieselben Frequenzen etwa
    eine Viertelperiode versetzt. So schwillt das Wogen links und rechts gemeinsam, die
    Kanaele selbst sind aber fast unabhaengig: Breite ohne Hall, und in Mono bleibt das
    Wogen erhalten (wichtig fuer zwei dicht stehende Lautsprecher)."""
    out = np.zeros((N, 2))
    for hz, level in tones:
        offs = np.concatenate(([0.0], rng.uniform(beat[0], beat[1], voices - 1) * rng.choice([-1, 1], voices - 1)))
        quarter = rng.uniform(np.pi / 3, 2 * np.pi / 3) * rng.choice([-1, 1])
        amp = drift(rng, rng.uniform(*slow), .75)
        for off in offs:
            ph0 = rng.uniform(0, 2 * np.pi, len(harmonics))
            for h, (a, p0) in enumerate(zip(harmonics, ph0), start=1):
                if not a or (hz + off) * h >= 6000: continue
                w = 2 * np.pi * (hz + off) * h * T + p0
                out[:, 0] += level * a / voices * amp * np.sin(w)
                out[:, 1] += level * a / voices * amp * np.sin(w + quarter * h + rng.uniform(-.25, .25))
    breath = drift(rng, 5, .45) * drift(rng, 23, .35)
    return out * (breath * np.minimum(1, T / 4))[:, None]
def air(rng, lo, hi, period, depth=.8, order=2):
    """Die Luft: Rauschen, links/rechts unabhaengig, mit langsamem Kommen und Gehen."""
    x = bandpass(rng.standard_normal((N, 2)), lo, hi, order)
    common = drift(rng, period, depth)
    for c in range(2): x[:, c] *= common * drift(rng, period / 3, .35)
    return x / np.sqrt((x ** 2).mean())
def finish(buf, peak_db=-3.0):
    buf = highpass(buf, 35); f = int(1.5 * SR); buf[-f:] *= np.linspace(1, 0, f)[:, None]
    buf[: SR // 20] *= np.linspace(0, 1, SR // 20)[:, None]
    return buf * (10 ** (peak_db / 20) / np.abs(buf).max())
def write(path, buf):
    with wave.open(path, 'wb') as w:
        w.setnchannels(2); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes((np.clip(buf, -1, 1) * 32767).astype('<i2').tobytes())

def cave():
    """CAVE: Bett aus tiefen, fast reinen Toenen (Moll in reiner Stimmung), kalte Hoehlenluft
    und ein tiefes Rinnen, ungestimmte Tropfen als Tupfer, wenige gestimmte Tropfen als Toene.
    Sehr langer dunkler Hall, Tropfstein-Echo mit Abstaenden aus den Natur-Tropfen."""
    rng = np.random.default_rng(1113); R = [1, 6 / 5, 4 / 3, 3 / 2, 9 / 5]
    b = bed(rng, [(D3 / 2, .5), (D3, 1.0), (D3 * 3 / 2, .7), (D3 * 2, .45), (D3 * 2 * 6 / 5, .22), (D3 * 3, .12)],
            [1, .18, .05], beat=(0.5, 1.7))
    luft = air(rng, 70, 260, 50, .6) * .30 + air(rng, 2500, 7000, 35, .9) * .012
    tupf = np.zeros((N, 2)); drips = np.sort(rng.uniform(1, DUR - 2, 11))
    for td in drips:
        n = int(SR * .1); x = np.arange(n) / SR; f0 = rng.uniform(1100, 3200)
        place(tupf, np.sin(2 * np.pi * np.cumsum(f0 * (1 + 1.4 * x / .1)) / SR) * env_ad(n, .001, .018), td, rng.random(), .5)
    tones = np.zeros((N, 2)); t = 3.0; last = 3
    while t < DUR - 6:
        k = int(np.clip(last + rng.integers(-2, 3), 0, 9)); last = k
        hz = D3 * 4 * R[k % 5] * (2 ** (k // 5)); n = int(SR * 1.4); x = np.arange(n) / SR
        ph = 2 * np.pi * np.cumsum(hz * (1 - .1 * np.exp(-x / .018))) / SR
        place(tones, (np.sin(ph) + .2 * np.sin(2 * ph) * np.exp(-x / .08)) * env_ad(n, .003, rng.uniform(.25, .5)), t, rng.uniform(.15, .85), rng.uniform(.5, 1))
        t += rng.uniform(.2, .45) if rng.random() < .3 else rng.exponential(4.0) + 2.0
    echo = np.zeros((N, 2)); gaps = np.clip(np.diff(drips)[:4] * .3 + .25, .25, 1.9)
    for j, d in enumerate(np.cumsum(gaps)):
        sh = int(d * SR); e = np.zeros((N, 2)); e[sh:] = tones[:-sh]; e = lowpass(e, 2600 / (1 + j)); p = .1 + .8 * (j % 2)
        echo[:, 0] += e[:, 0] * (1 - p) * .55 ** (j + 1) * 1.6; echo[:, 1] += e[:, 1] * p * .55 ** (j + 1) * 1.6
    front = (tones + echo) * .16 + tupf * .035
    return finish(b * .30 + luft * .05 + front + reverb(front * 1.6 + b * .12, 7.5, 2400, .045) * .55)

def dunes():
    """DUNES: Bett aus warmen, rohrblattartigen Dauertoenen (septimale Reihe), Sand als Luft,
    die in Boeen kommt und geht, einzelne Sandkoerner als Tupfer, drei lange Rohrblatt-Toene.
    Trocken und offen; die Sandkorn-Wolke verweht den Ausklang."""
    rng = np.random.default_rng(2207); R = [1, 7 / 6, 4 / 3, 3 / 2, 7 / 4]; base = D3 * 1.5
    reedh = [1, .32, .5, .12, .22, .05, .1, 0, .05]
    b = lowpass(bed(rng, [(D3 / 2, .35), (D3, 1.0), (D3 * 3 / 2, .8), (D3 * 7 / 4, .28), (D3 * 2, .4), (D3 * 3, .12)],
                    reedh, beat=(0.45, 1.5)), 900)
    gust = drift(rng, 16, .95) ** 2
    sand = air(rng, 2500, 9500, 40, .85) * gust[:, None]
    tupf = np.zeros((N, 2))
    for td in rng.uniform(0, DUR - 1, 150):
        if rng.random() > gust[int(td * SR)] * 1.3: continue
        n = int(SR * .006); place(tupf, highpass(rng.standard_normal(n), 3000) * np.hanning(n), td, rng.random(), rng.uniform(.3, 1))
    def reed(hz, dur):
        n = int(SR * dur); x = np.arange(n) / SR
        v = 1 + .006 * np.sin(2 * np.pi * 5.1 * x) * np.clip((x - .8) / 1.2, 0, 1)
        ph = 2 * np.pi * np.cumsum(hz * v) / SR; s = np.zeros(n)
        for h in range(1, 14):
            if hz * h < 8000: s += ((1.0 if h % 2 else .35) / h ** 1.4) * np.sin(h * ph + .3 * h)
        e = np.minimum(1, x / .6) ** 1.5 * np.clip((dur - x) / 1.4, 0, 1)
        return lowpass(s, 1400) * e + bandpass(np.random.default_rng(int(hz)).standard_normal(n), 900, 3000) * .04 * e
    tones = np.zeros((N, 2))
    for t0, k, pan in ((4.0, 3, .35), (15.5, 1, .65), (27.0, 2, .45)):
        place(tones, reed(base * R[k], 7.0), t0, pan, 1.0)
    cloud = np.zeros((N, 2)); mono = tones.mean(1); g = int(.025 * SR); win = np.hanning(g)
    for _ in range(2600):
        src = rng.integers(0, N - g); dst = src + int(rng.uniform(.2, 3.0) * SR)
        if dst + g >= N or abs(mono[src:src + g]).max() < 1e-3: continue
        p = rng.random(); gr = mono[src:src + g] * win * (.4 + gust[dst])
        cloud[dst:dst + g, 0] += gr * (1 - p); cloud[dst:dst + g, 1] += gr * p
    front = tones * .22 + highpass(cloud, 500) * .16
    return finish(b * .34 + sand * .012 + tupf * .010 + front + reverb(front + b * .1, 1.2, 3000, .01) * .14)

def jungle():
    """JUNGLE: Bett aus weichen Toenen in fuenf gleichen Stufen, feuchte Luft und ein
    Zikadenband, das kommt und geht, Froesche und Blatt-Tropfen als Tupfer, schwebende
    Klangstaebe in Paaren als wenige Toene. Naher, dichter Raum."""
    rng = np.random.default_rng(909); st = 2 ** (240 / 1200)
    b = bed(rng, [(D3, 1.0), (D3 * st ** 3, .75), (D3 * 2, .5), (D3 * 2 * st, .3), (D3 * 2 * st ** 3, .22), (D3 * 4, .1)],
            [1, .25, .1, .04], beat=(0.6, 2.0))
    luft = air(rng, 500, 3200, 30, .6, 1) * .35 + air(rng, 4300, 5600, 22, 1.0, 3) * (.5 + .5 * np.sin(2 * np.pi * 38 * T))[:, None] * .45
    tupf = np.zeros((N, 2))
    for f0, pan, seed in ((520, .1, 1), (690, .9, 2), (410, .6, 3)):
        r = np.random.default_rng(seed); t = r.uniform(1, 6)
        while t < DUR - 1:
            for k in range(r.integers(2, 5)):
                n = int(SR * .07); x = np.arange(n) / SR
                place(tupf, np.sin(2 * np.pi * f0 * (1 + .25 * x / .07) * x) * (np.sin(2 * np.pi * 55 * x) > 0) * np.hanning(n), t + k * .16, pan, .5)
            t += r.uniform(5, 12)
    for td in rng.uniform(0, DUR - 1, 30):
        n = int(SR * .03); place(tupf, bandpass(rng.standard_normal(n), 1500, 6000) * env_ad(n, .0005, .006), td, rng.random(), .8)
    def bar(hz, dur, beat):
        n = int(SR * dur); x = np.arange(n) / SR; s = np.zeros((n, 2))
        for r_, a, d in ((1, 1, dur * .4), (2.76, .3, dur * .15), (5.4, .1, dur * .07)):
            s[:, 0] += a * np.sin(2 * np.pi * (hz * r_ - beat / 2 * r_ ** .5) * x) * np.exp(-x / d)   # Paar: ein Stab links,
            s[:, 1] += a * np.sin(2 * np.pi * (hz * r_ + beat / 2 * r_ ** .5) * x) * np.exp(-x / d)   # einer rechts
        return s * np.minimum(1, x / .008)[:, None]
    tones = np.zeros((N, 2)); A = [0, None, 2, None, None, 3, None]; B = [None, None, 4, None, 1, None, None, None, 2]
    i = 0; t = 2.5
    while t < DUR - 7:
        for pat, octv, bt, g in ((A, 2, 4.5, .8), (B, 4, 6.0, .55)):
            k = pat[i % len(pat)]
            if k is not None:
                s = bar(D3 * octv * st ** k, 5.0, bt); j = int(t * SR); m = min(len(s), N - j); tones[j:j + m] += s[:m] * g
        i += 1; t += 1.35
    front = lowpass(tones, 5000) * .15 + tupf * .022
    return finish(b * .32 + luft * .012 + front + reverb(front + b * .08, 1.4, 5000, .006) * .30)

if __name__ == '__main__':
    out = sys.argv[1] if len(sys.argv) > 1 else '.'
    for name, fn in (('cave', cave), ('dunes', dunes), ('jungle', jungle)):
        write(f'{out}/sketch_{name}.wav', fn()); print('ok', name)
