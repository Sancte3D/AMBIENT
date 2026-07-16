#!/usr/bin/env python3
"""Generate a reproducible signal-health report for the rendered WAV previews."""

from __future__ import annotations

import math
import pathlib
import wave

import numpy as np


ROOT = pathlib.Path(__file__).resolve().parents[1]
WAV_DIR = ROOT / "build" / "audio_wav"
REPORT = ROOT / "docs" / "PREVIEW_SIGNAL_REPORT.md"


def db(value: float) -> float:
    return 20.0 * math.log10(max(value, 1.0e-12))


def spectral_centroid(mono: np.ndarray, sample_rate: int) -> float:
    window_size = 4096
    if mono.size < window_size:
        return 0.0
    starts = np.linspace(0, mono.size - window_size, 48, dtype=np.int64)
    window = np.hanning(window_size)
    frequencies = np.fft.rfftfreq(window_size, 1.0 / sample_rate)
    weighted = 0.0
    weight = 0.0
    for start in starts:
        magnitude = np.abs(np.fft.rfft(mono[start : start + window_size] * window))
        total = float(magnitude.sum())
        if total > 1.0e-9:
            weighted += float((magnitude * frequencies).sum())
            weight += total
    return weighted / weight if weight else 0.0


def analyze(path: pathlib.Path) -> dict[str, float]:
    with wave.open(str(path), "rb") as wav:
        channels = wav.getnchannels()
        sample_rate = wav.getframerate()
        width = wav.getsampwidth()
        frames = wav.getnframes()
        raw = wav.readframes(frames)
    if channels != 2 or width != 2:
        raise ValueError(f"{path.name}: expected 16-bit stereo WAV")
    audio = np.frombuffer(raw, dtype="<i2").astype(np.float64).reshape(-1, 2) / 32768.0
    mono = audio.mean(axis=1)
    peak = float(np.max(np.abs(audio)))
    rms = float(np.sqrt(np.mean(audio * audio)))
    dc = float(np.max(np.abs(np.mean(audio, axis=0))))
    left = audio[:, 0]
    right = audio[:, 1]
    denominator = float(np.sqrt(np.sum(left * left) * np.sum(right * right)))
    correlation = float(np.sum(left * right) / denominator) if denominator > 1.0e-12 else 0.0
    centroid = spectral_centroid(mono, sample_rate)
    return {
        "peak": db(peak),
        "rms": db(rms),
        "crest": db(peak / max(rms, 1.0e-12)),
        "dc": db(dc),
        "correlation": correlation,
        "centroid": centroid,
    }


def main() -> None:
    paths = sorted(WAV_DIR.glob("*.wav"))
    if len(paths) != 10:
        raise SystemExit(f"expected 10 WAV previews in {WAV_DIR}, found {len(paths)}")
    rows: list[str] = []
    all_ok = True
    for path in paths:
        metric = analyze(path)
        ok = (
            -36.0 <= metric["peak"] <= -6.0
            and -52.0 <= metric["rms"] <= -18.0
            and metric["dc"] < -60.0
            and -0.995 < metric["correlation"] < 0.995
            and 80.0 < metric["centroid"] < 9000.0
        )
        all_ok &= ok
        rows.append(
            f"| {path.stem.upper()} | {metric['peak']:.1f} | {metric['rms']:.1f} | "
            f"{metric['crest']:.1f} | {metric['dc']:.1f} | {metric['correlation']:.3f} | "
            f"{metric['centroid']:.0f} | {'PASS' if ok else 'REVIEW'} |"
        )
    report = "\n".join(
        [
            "# Preview signal report",
            "",
            "Generated from the unnormalized 44.1 kHz stereo WAV renders. MP3 listening previews are loudness-normalized separately; firmware retains mix headroom.",
            "",
            "| Model | Peak dBFS | RMS dBFS | Crest dB | DC dBFS | L/R corr. | Centroid Hz | Gate |",
            "|---|---:|---:|---:|---:|---:|---:|---|",
            *rows,
            "",
            "## Interpretation",
            "",
            "- Peak headroom is intentional because this engine will join the product mix and spatial tail.",
            "- Correlation below 0.995 confirms non-identical stereo output; final mono compatibility still requires speaker testing.",
            "- The DC gate is a technical health check, not a replacement for DAC measurements.",
            "- Spectral centroid is descriptive only. Perceived beauty and fatigue require listening on the actual enclosure and speakers.",
            "",
            f"Overall automated gate: **{'PASS' if all_ok else 'REVIEW REQUIRED'}**",
            "",
        ]
    )
    REPORT.write_text(report, encoding="utf-8")
    print(f"wrote {REPORT}")
    if not all_ok:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
