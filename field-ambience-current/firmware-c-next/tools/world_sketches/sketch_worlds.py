#!/usr/bin/env python3
"""Hoerskizzen fuer drei neue World-Kandidaten (Schritt 1 des Worlds-Auftrags).

KEIN Firmware-Klang: reine Host-Skizzen in numpy, um Ort, Klangquelle,
Tonvorrat, Zeitgefuehl und Signatur-Effekt hoerbar zu machen. Nichts davon
laeuft durch engine_render(); Pegel, Rechenzeit und Echtzeitregeln sind hier
nicht geprueft. Deterministisch (fester Seed).

    python3 sketch_worlds.py <ausgabeordner>      # schreibt cave/dunes/jungle .wav
"""
import sys, wave
import numpy as np
SR = 44100; DUR = 30.0; N = int(SR * DUR); D4 = 293.665
rng = np.random.default_rng(1113)

def env_ad(n, attack, decay):
    t = np.arange(n) / SR
    return np.minimum(1.0, t / max(attack, 1e-4)) * np.exp(-np.maximum(0, t - attack) / decay)
def place(buf, sig, t0, pan=0.5, gain=1.0):
    i = int(t0 * SR)
    if i >= N: return
    sig = sig[: N - i] * gain
    buf[i:i + len(sig), 0] += sig * np.cos(pan * np.pi / 2)
    buf[i:i + len(sig), 1] += sig * np.sin(pan * np.pi / 2)
def fft_filter(x, shape):
    X = np.fft.rfft(x, axis=0); f = np.fft.rfftfreq(len(x), 1 / SR)
    g = shape(f); return np.fft.irfft(X * (g[:, None] if x.ndim == 2 else g), n=len(x), axis=0)
def lowpass(x, hz, order=2): return fft_filter(x, lambda f: 1 / np.sqrt(1 + (f / hz) ** (2 * order)))
def highpass(x, hz, order=2): return fft_filter(x, lambda f: 1 / np.sqrt(1 + (hz / np.maximum(f, 1e-3)) ** (2 * order)))
def bandpass(x, lo, hi): return highpass(lowpass(x, hi), lo)
def reverb(x, t60, bright_hz, predelay=0.02, seed=7):
    """Faltung mit abklingendem Rauschen: nur fuer die Skizze, nicht der Geraete-Hall."""
    r = np.random.default_rng(seed); n = int(SR * t60 * 1.3); t = np.arange(n) / SR
    ir = r.standard_normal((n, 2)) * np.exp(-6.908 * t / t60)[:, None]
    ir = lowpass(ir, bright_hz); ir[: int(.004 * SR)] *= np.linspace(0, 1, int(.004 * SR))[:, None]
    ir /= np.sqrt((ir ** 2).sum(0)).mean()
    m = len(x) + n; L = 1 << (m - 1).bit_length(); out = np.zeros_like(x); pd = int(predelay * SR)
    for c in range(2):
        y = np.fft.irfft(np.fft.rfft(x[:, c], L) * np.fft.rfft(ir[:, c], L), L)[: len(x)]
        out[pd:, c] = y[: len(x) - pd]
    return out
def finish(buf, peak_db=-3.0):
    buf = highpass(buf, 40); fade = int(.4 * SR); buf[-fade * 3:] *= np.linspace(1, 0, fade * 3)[:, None]
    buf[:fade // 8] *= np.linspace(0, 1, fade // 8)[:, None]
    return buf * (10 ** (peak_db / 20) / np.abs(buf).max())
def write(path, buf):
    with wave.open(path, 'wb') as w:
        w.setnchannels(2); w.setsampwidth(2); w.setframerate(SR)
        w.writeframes((np.clip(buf, -1, 1) * 32767).astype('<i2').tobytes())

def cave():
    """CAVE (Grotte): gestimmte Tropfen, Moll-Pentatonik in reiner Stimmung, Zufalls-Tropfen,
    sehr langer dunkler Hall, Tropfstein-Echo (Taps aus den Natur-Tropfen)."""
    ratios = [1, 6 / 5, 4 / 3, 3 / 2, 9 / 5]; dry = np.zeros((N, 2)); nat = np.zeros((N, 2))
    t = 0.6; last = 2
    while t < DUR - 4:
        k = int(np.clip(last + rng.integers(-2, 3), 0, 9)); last = k
        hz = D4 * 2 * ratios[k % 5] * (2 ** (k // 5))
        n = int(SR * 1.2); tt = np.arange(n) / SR
        glide = 1 - .12 * np.exp(-tt / .018)                    # Tropfen: Tonhoehe steigt kurz an
        ph = 2 * np.pi * np.cumsum(hz * glide) / SR
        s = (np.sin(ph) + .22 * np.sin(2 * ph) * np.exp(-tt / .08)) * env_ad(n, .003, rng.uniform(.22, .5))
        place(dry, s, t, rng.uniform(.2, .8), rng.uniform(.5, 1.0))
        if rng.random() < .3: t += rng.uniform(.18, .4)        # kleine Tropfengruppen
        else: t += rng.exponential(1.7) + .5
    # Natur: ungestimmte Tropfen und ein tiefes unterirdisches Rinnen
    drips = np.sort(rng.uniform(0, DUR - 1, 26))
    for td in drips:
        n = int(SR * .12); tt = np.arange(n) / SR; f0 = rng.uniform(900, 2600)
        s = np.sin(2 * np.pi * np.cumsum(f0 * (1 + 1.5 * tt / .12)) / SR) * env_ad(n, .001, .02)
        place(nat, s, td, rng.random(), .10)
    stream = lowpass(rng.standard_normal((N, 2)), 220) * (0.6 + 0.4 * np.sin(2 * np.pi * .07 * np.arange(N) / SR))[:, None]
    nat += stream * .5
    # DRONE: Grundton und Quinte, leise Schwebung
    tt = np.arange(N) / SR; dr = np.zeros((N, 2))
    for hz, g in ((D4 / 2, 1), (D4 / 2 * 3 / 2, .6)):
        dr[:, 0] += g * np.sin(2 * np.pi * hz * tt); dr[:, 1] += g * np.sin(2 * np.pi * (hz + .23) * tt)
    dr *= (.75 + .25 * np.sin(2 * np.pi * .05 * tt))[:, None] * np.minimum(1, tt / 5)[:, None]
    # Signatur: Tropfstein-Echo. Vier Taps, Zeiten aus den Abstaenden der Natur-Tropfen, jedes dunkler
    echo = np.zeros((N, 2)); gaps = np.clip(np.diff(drips)[:4] * .45 + .21, .23, 1.9)
    for j, d in enumerate(np.cumsum(gaps)):
        sh = int(d * SR); e = np.zeros((N, 2)); e[sh:] = dry[:-sh]
        e = lowpass(e, 2600 / (1 + j)); p = .15 + .7 * (j % 2)
        echo[:, 0] += e[:, 0] * (1 - p) * .55 ** (j + 1) * 1.6; echo[:, 1] += e[:, 1] * p * .55 ** (j + 1) * 1.6
    music = dry + echo
    mix = music * .9 + reverb(music + .3 * nat, 7.5, 2400, .045) * .55 + nat * .35 + dr * .05
    return finish(mix)

def dunes():
    """DUNES (Duenen): weiches Rohrblatt (Duduk-artig) mit Bordun, septimale Fuenftonreihe,
    langsamer Karawanen-Puls, trockener Raum mit weitem Echo, Sandkorn-Wolke."""
    ratios = [1, 7 / 6, 4 / 3, 3 / 2, 7 / 4]; base = D4 / 2 * 1.5; tt_all = np.arange(N) / SR
    def reed(hz, dur, vib=True):
        n = int(SR * dur); tt = np.arange(n) / SR
        v = 1 + (.006 * np.sin(2 * np.pi * 5.1 * tt) * np.clip((tt - .5) / .8, 0, 1) if vib else 0)
        ph = 2 * np.pi * np.cumsum(hz * v) / SR; s = np.zeros(n)
        for h in range(1, 16):                                   # ungerade Teiltoene betont, weicher Abfall
            a = (1.0 if h % 2 else .35) / h ** 1.35
            if hz * h < 9000: s += a * np.sin(h * ph + .3 * h)
        e = np.minimum(1, tt / .28) * np.minimum(1, (dur - tt) / .7).clip(0)
        breath = bandpass(rng.standard_normal(n), 900, 3200) * .05 * e
        return lowpass(s, 1500) * e * (.85 + .15 * np.sin(2 * np.pi * .4 * tt)) + breath
    dry = np.zeros((N, 2)); pulse = 3.9; steps = [0, 2, 3, 2, 1, 0, 3, 4]
    for i, k in enumerate(steps[:7]):
        place(dry, reed(base * ratios[k], 3.1), .8 + i * pulse, .42 + .16 * (i % 2), .9)
    # Bordun (Dam): gehaltener Grundton, sehr ruhig
    dam = reed(base, DUR - .2, vib=False) * .22
    dr = np.zeros((N, 2)); place(dr, dam, 0, .5, 1.0); dr = lowpass(dr, 700) * np.minimum(1, tt_all / 4)[:, None]
    # Natur: rieselnder Sand in langsamen Boeen
    gust = np.clip(.5 + .5 * np.sin(2 * np.pi * .045 * tt_all + 1) + .3 * np.sin(2 * np.pi * .11 * tt_all), 0, 1.3) ** 2
    sand = bandpass(rng.standard_normal((N, 2)), 2500, 9000) * gust[:, None] * .06
    # Signatur: Sandkorn-Wolke. 25-ms-Koernchen des Rohrblatts, verweht nach hinten gestreut
    cloud = np.zeros((N, 2)); mono = dry.mean(1); g = int(.025 * SR); win = np.hanning(g)
    for _ in range(2600):
        src = rng.integers(0, N - g); dst = src + int(rng.uniform(.15, 2.2) * SR)
        if dst + g >= N or abs(mono[src:src + g]).max() < 1e-3: continue
        p = rng.random(); grain = mono[src:src + g] * win * (.6 + .8 * gust[dst])
        cloud[dst:dst + g, 0] += grain * (1 - p); cloud[dst:dst + g, 1] += grain * p
    cloud = highpass(cloud, 500) * .9
    slap = np.zeros((N, 2)); sh = int(.43 * SR); slap[sh:, 0] = dry[:-sh, 1] * .22; slap[sh:, 1] = dry[:-sh, 0] * .22
    music = dry + lowpass(slap, 1800) + cloud
    mix = music + dr + reverb(music, 1.1, 3000, .01) * .16 + sand
    return finish(mix)

def jungle():
    """JUNGLE (Regenwald): Gong-Paare mit Schwebung, Slendro (5 fast gleiche Stufen),
    zwei verzahnte Muster, naher dichter Raum, Nacht aus Froeschen und Zikaden."""
    step = 2 ** (240 / 1200); dry = np.zeros((N, 2))
    def gong(hz, dur, beat=5.5):
        n = int(SR * dur); tt = np.arange(n) / SR; s = np.zeros(n)
        for r, a, d in ((1, 1, dur * .42), (2.76, .34, dur * .16), (5.4, .12, dur * .07)):
            for det in (-beat / 2, beat / 2):                   # Paar: zwei Staebe, wenige Hz auseinander
                s += a * np.sin(2 * np.pi * (hz * r + det * r ** .5) * tt) * np.exp(-tt / d)
        return s * np.minimum(1, tt / .006)
    A = [0, 2, None, 3, None]; B = [None, 4, 1, None, 2, None, 3]; tick = .62; i = 0; t = .5
    while t < DUR - 4:
        a = A[i % len(A)]; b = B[i % len(B)]
        if a is not None: place(dry, gong(D4 * step ** a, 3.2), t, .3, .55)
        if b is not None: place(dry, gong(D4 * 2 * step ** b, 2.6, 6.5), t + tick / 2, .7, .42)
        if i % 16 == 0: place(dry, gong(D4 / 2, 7.0, 2.2), t, .5, .55)   # grosser Gong, langsame Schwebung
        i += 1; t += tick
    dry = lowpass(dry, 6000)
    tt = np.arange(N) / SR; nat = np.zeros((N, 2))
    # Zikaden: schmales hohes Band, schnell amplitudenmoduliert, kommt und geht
    cic = bandpass(rng.standard_normal((N, 2)), 4300, 5600) * (.5 + .5 * np.sin(2 * np.pi * 38 * tt))[:, None]
    nat += cic * (np.clip(np.sin(2 * np.pi * .06 * tt), 0, 1) ** 2)[:, None] * .05
    # Froesche: kurze gepulste Rufe, zwei Stimmen links und rechts
    for f0, pan, seed in ((520, .15, 1), (690, .85, 2), (410, .6, 3)):
        r = np.random.default_rng(seed); t = r.uniform(.5, 3)
        while t < DUR - 1:
            for k in range(r.integers(2, 6)):
                n = int(SR * .07); x = np.arange(n) / SR
                s = np.sin(2 * np.pi * f0 * (1 + .25 * x / .07) * x) * (np.sin(2 * np.pi * 55 * x) > 0) * np.hanning(n)
                place(nat, s, t + k * .16, pan, .035)
            t += r.uniform(2.5, 7)
    # Tropfen von den Blaettern
    for td in rng.uniform(0, DUR - 1, 60):
        n = int(SR * .03); s = bandpass(rng.standard_normal(n), 1500, 6000) * env_ad(n, .0005, .006)
        place(nat, s, td, rng.random(), .05)
    dr = np.zeros((N, 2))
    for hz in (D4 / 2, D4 / 2 * step ** 3):
        dr[:, 0] += np.sin(2 * np.pi * hz * tt); dr[:, 1] += np.sin(2 * np.pi * (hz + .4) * tt)
    dr *= np.minimum(1, tt / 5)[:, None] * .02
    mix = dry + reverb(dry + .5 * nat, 1.3, 5000, .006) * .30 + nat + dr
    return finish(mix)

if __name__ == '__main__':
    out = sys.argv[1] if len(sys.argv) > 1 else '.'
    for name, fn in (('cave', cave), ('dunes', dunes), ('jungle', jungle)):
        write(f'{out}/sketch_{name}.wav', fn()); print('ok', name)
