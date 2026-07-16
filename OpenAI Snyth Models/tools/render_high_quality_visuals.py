#!/usr/bin/env python3
"""Render original cinematic display loops from clean-room optical recipes.

The renderer does not ingest reference media.  It builds every frame from
deterministic geometry, particles, light masks, and colour transforms.  The
2x working canvas is reduced to the product display's native 320 x 170 pixels.
"""

from __future__ import annotations

import argparse
import colorsys
import dataclasses
import math
import pathlib
import random
from collections.abc import Callable, Sequence

import numpy as np
from PIL import Image, ImageChops, ImageDraw, ImageEnhance, ImageFilter, ImageFont


WIDTH = 320
HEIGHT = 170
SCALE = 2
WORK_SIZE = (WIDTH * SCALE, HEIGHT * SCALE)
TAU = math.tau


def sp(value: float) -> int:
    return int(round(value * SCALE))


def points(values: Sequence[tuple[float, float]]) -> list[tuple[int, int]]:
    return [(sp(x), sp(y)) for x, y in values]


def hsv(hue: float, saturation: float = 1.0,
        value: float = 1.0) -> tuple[int, int, int]:
    r, g, b = colorsys.hsv_to_rgb(hue % 1.0, saturation, value)
    return (round(r * 255), round(g * 255), round(b * 255))


def dim(colour: tuple[int, int, int], amount: float) -> tuple[int, int, int]:
    return tuple(round(channel * amount) for channel in colour)


def mix(a: tuple[int, int, int], b: tuple[int, int, int],
        amount: float) -> tuple[int, int, int]:
    return tuple(round(x + (y - x) * amount) for x, y in zip(a, b))


_background_cache: dict[str, Image.Image] = {}


def background(name: str) -> Image.Image:
    cached = _background_cache.get(name)
    if cached is not None:
        return cached.copy()
    y, x = np.mgrid[0:WORK_SIZE[1], 0:WORK_SIZE[0]].astype(np.float32)
    nx = (x / (WORK_SIZE[0] - 1) - 0.5) * 2.0
    ny = (y / (WORK_SIZE[1] - 1) - 0.5) * 2.0
    radius = np.sqrt(nx * nx + ny * ny)
    edge = np.clip(1.0 - radius * 0.62, 0.0, 1.0)
    image = np.zeros((WORK_SIZE[1], WORK_SIZE[0], 3), dtype=np.float32)
    if name == "violet":
        center = np.exp(-((nx / 0.84) ** 2 + (ny / 0.92) ** 2) * 1.8)
        image[..., 0] = 8.0 + center * 18.0
        image[..., 1] = 1.0 + center * 2.0
        image[..., 2] = 18.0 + center * 35.0
    elif name == "ocean":
        center = np.exp(-((nx / 0.95) ** 2 + (ny / 0.72) ** 2) * 2.2)
        image[..., 0] = 0.5
        image[..., 1] = 3.0 + center * 7.0
        image[..., 2] = 7.0 + center * 16.0
    elif name == "ember":
        center = np.exp(-((nx / 0.82) ** 2 + (ny / 0.65) ** 2) * 2.8)
        image[..., 0] = 4.0 + center * 10.0
        image[..., 1] = center * 1.5
        image[..., 2] = 2.0 + center * 4.0
    elif name == "forest":
        center = np.exp(-((nx / 0.95) ** 2 + (ny / 0.48) ** 2) * 2.0)
        image[..., 0] = center * 1.5
        image[..., 1] = 2.0 + center * 9.0
        image[..., 2] = 3.0 + center * 6.0
    else:
        image[..., 0] = 0.5
        image[..., 1] = 0.8
        image[..., 2] = 1.4
    image *= edge[..., None]
    result = Image.fromarray(np.uint8(np.clip(image, 0, 255)), "RGB")
    _background_cache[name] = result
    return result.copy()


def line(draw: ImageDraw.ImageDraw, path: Sequence[tuple[float, float]],
         fill: tuple[int, int, int], width: float = 1.0) -> None:
    draw.line(points(path), fill=fill, width=max(1, sp(width)), joint="curve")


def luminous_line(draw: ImageDraw.ImageDraw,
                  path: Sequence[tuple[float, float]],
                  colour: tuple[int, int, int], width: float = 1.0,
                  core: tuple[int, int, int] | None = None) -> None:
    line(draw, path, dim(colour, 0.22), width * 5.8)
    line(draw, path, dim(colour, 0.58), width * 2.8)
    line(draw, path, core or mix(colour, (255, 255, 255), 0.42), width)


def ellipse(draw: ImageDraw.ImageDraw, box: tuple[float, float, float, float],
            **kwargs: object) -> None:
    draw.ellipse(tuple(sp(value) for value in box), **kwargs)


def rectangle(draw: ImageDraw.ImageDraw,
              box: tuple[float, float, float, float], **kwargs: object) -> None:
    draw.rectangle(tuple(sp(value) for value in box), **kwargs)


def polygon(draw: ImageDraw.ImageDraw,
            vertices: Sequence[tuple[float, float]], **kwargs: object) -> None:
    draw.polygon(points(vertices), **kwargs)


def stable_noise(seed: int, count: int) -> list[tuple[float, float, float]]:
    rng = random.Random(seed)
    return [(rng.random(), rng.random(), rng.random()) for _ in range(count)]


def finish(scene: Image.Image, *, bloom: float = 1.0,
           exposure: float = 1.0, grain_seed: int = 0) -> Image.Image:
    near = scene.filter(ImageFilter.GaussianBlur(sp(1.7)))
    far = scene.filter(ImageFilter.GaussianBlur(sp(7.5)))
    near = ImageEnhance.Brightness(near).enhance(0.62 * bloom)
    far = ImageEnhance.Brightness(far).enhance(0.34 * bloom)
    result = ImageChops.screen(scene, near)
    result = ImageChops.screen(result, far)
    result = result.resize((WIDTH, HEIGHT), Image.Resampling.LANCZOS)

    array = np.asarray(result, dtype=np.float32) / 255.0
    # Preserve clipped white light.  The previous photographic shoulder curve
    # mapped an already-white pixel back to grey, which is exactly the wrong
    # behaviour for emissive OLED-style art.
    array = np.power(np.clip(array * exposure, 0.0, 1.0), 0.91)
    yy, xx = np.mgrid[0:HEIGHT, 0:WIDTH].astype(np.float32)
    nx = (xx / (WIDTH - 1) - 0.5) * 2.0
    ny = (yy / (HEIGHT - 1) - 0.5) * 2.0
    vignette = np.clip(1.04 - 0.23 * (nx * nx + ny * ny), 0.58, 1.0)
    array *= vignette[..., None]
    # A tiny deterministic sensor texture prevents sterile gradients without
    # producing the compression-heavy dancing noise of per-frame film grain.
    rng = np.random.default_rng(grain_seed)
    grain = rng.normal(0.0, 0.0024, (HEIGHT, WIDTH, 1)).astype(np.float32)
    array = np.clip(array + grain * np.clip(array * 3.0, 0.0, 1.0), 0.0, 1.0)
    return Image.fromarray(np.uint8(array * 255.0), "RGB")


def render_radiant_threshold(phase: float, frame: int) -> Image.Image:
    scene = background("violet")
    rays = Image.new("RGB", WORK_SIZE, (0, 0, 0))
    draw = ImageDraw.Draw(rays)
    cx, cy = 160.0, 76.0
    pulse = 0.78 + 0.22 * math.sin(phase + 0.35) ** 2
    rng = random.Random(41817)
    for index in range(54):
        base_angle = TAU * index / 54.0
        motion_seed = rng.random() * TAU
        angle = base_angle + rng.uniform(-0.035, 0.035)
        angle += 0.042 * math.sin(phase * 3.0 + motion_seed)
        flare = math.sin(phase * (3 + index % 3) + motion_seed) ** 2
        flare *= flare
        length = rng.uniform(178.0, 285.0) * pulse * (0.48 + flare * 0.68)
        start = rng.uniform(17.0, 43.0)
        spread = rng.uniform(0.003, 0.018) * (0.72 + flare * 0.72)
        colour = hsv(0.94 + 0.11 * math.sin(index * 1.37), 0.96, 1.0)
        if index % 5 == 0:
            colour = (255, 25, 54)
        inner_a = (cx + math.cos(angle - spread) * start,
                   cy + math.sin(angle - spread) * start)
        inner_b = (cx + math.cos(angle + spread) * start,
                   cy + math.sin(angle + spread) * start)
        outer_a = (cx + math.cos(angle - spread * 2.4) * length,
                   cy + math.sin(angle - spread * 2.4) * length)
        outer_b = (cx + math.cos(angle + spread * 2.4) * length,
                   cy + math.sin(angle + spread * 2.4) * length)
        polygon(draw, [inner_a, outer_a, outer_b, inner_b],
                fill=dim(colour, rng.uniform(0.24, 0.72) * (0.28 + flare * 0.72)))
        if index % 3 == 0:
            line(draw, [(cx, cy),
                        (cx + math.cos(angle) * length,
                         cy + math.sin(angle) * length)],
                 mix(colour, (255, 255, 255), 0.58), rng.uniform(0.6, 1.6))
    rays = rays.filter(ImageFilter.GaussianBlur(sp(0.55)))
    scene = ImageChops.screen(scene, rays)

    aperture = Image.new("RGB", WORK_SIZE, (0, 0, 0))
    adraw = ImageDraw.Draw(aperture)
    breathe = 1.0 + 0.035 * math.sin(phase)
    profile = [
        (0.0, 7.0), (12.0, 11.0), (27.0, 10.0), (41.0, 7.0),
        (55.0, 13.0), (69.0, 25.0), (89.0, 34.0), (115.0, 44.0),
        (147.0, 61.0), (170.0, 70.0),
    ]
    left = [(cx - half * breathe, y) for y, half in profile]
    right = [(cx + half * breathe, y) for y, half in reversed(profile)]
    polygon(adraw, left + right, fill=(255, 255, 255))
    # A razor-bright slit makes this an aperture/monolith, not a traced figure.
    polygon(adraw, [(cx - 2, 5), (cx + 2, 5), (cx + 4, 137),
                    (cx, 165), (cx - 4, 137)], fill=(255, 255, 255))
    scene = ImageChops.screen(scene, aperture)
    return finish(scene, bloom=1.32, exposure=1.26, grain_seed=103)


def _spectral_needles(phase: float, dense: bool) -> Image.Image:
    scene = background("ocean")
    glow_layer = Image.new("RGB", WORK_SIZE, (0, 0, 0))
    core_layer = Image.new("RGB", WORK_SIZE, (0, 0, 0))
    glow_draw = ImageDraw.Draw(glow_layer)
    core_draw = ImageDraw.Draw(core_layer)
    rng = random.Random(7721 if dense else 6129)
    count = 116 if dense else 58
    center_y = 84.0
    for index in range(count):
        x = (index + 0.5) * WIDTH / count + rng.uniform(-2.2, 2.2)
        seed_phase = rng.random() * TAU
        x += math.sin(phase * (2 + index % 3) + seed_phase) * (0.7 if dense else 0.45)
        envelope = 0.42 + 0.58 * abs(math.sin(x * 0.043 + seed_phase))
        breathe = 0.38 + 0.62 * math.sin(phase * (2 + index % 4) + seed_phase) ** 2
        base = rng.uniform(10.0, 54.0 if dense else 36.0)
        if rng.random() < (0.17 if dense else 0.10):
            base *= rng.uniform(1.7, 2.5)
        up = min(82.0, base * envelope * breathe + (18.0 if dense else 5.0))
        down = min(82.0, base * (1.12 - envelope * 0.35) *
                   (0.42 + 0.58 * math.cos(phase * (3 + index % 2) + seed_phase) ** 2))
        mid = center_y + 4.2 * math.sin(phase * (2 + index % 3) + seed_phase)
        # Local hue families, not a left-to-right rainbow ramp.  This keeps the
        # field crystalline and irregular rather than reading as an equalizer.
        hue = (rng.random() * 0.86 + x / WIDTH * 0.16 + 0.43) % 1.0
        colour = hsv(hue, 0.88, 1.0)
        wide = rng.uniform(1.7, 4.6 if dense else 3.2)
        # Tapered bodies create optical needles.  A uniform stroked line looks
        # like UI geometry even after blur.
        polygon(glow_draw, [(x, mid - up - wide * 1.8),
                            (x - wide, mid - up * 0.18),
                            (x - wide * 0.72, mid + down * 0.14),
                            (x, mid + down + wide * 1.8),
                            (x + wide * 0.72, mid + down * 0.14),
                            (x + wide, mid - up * 0.18)],
                fill=dim(colour, 0.68))
        line(glow_draw, [(x, mid - up), (x, mid + down)],
             mix(colour, (255, 255, 255), 0.24), max(0.7, wide * 0.42))
        core_colour = mix(colour, (255, 255, 255), 0.76)
        line(core_draw, [(x, mid - up * 0.90), (x, mid + down * 0.90)],
             core_colour, max(0.35, wide * 0.16))
        if index % 9 == 0:
            tip = rng.uniform(0.7, 1.5)
            ellipse(core_draw, (x - tip, mid - up - tip,
                                x + tip, mid - up + tip),
                    fill=(245, 252, 255))

    # The bright, irregular middle seam is the visual source of the choir.
    band: list[tuple[float, float]] = []
    for x in range(0, WIDTH + 2, 2):
        y = center_y + math.sin(x * 0.16 + phase * 2.0) * 2.6
        y += math.sin(x * 0.047 - phase) * 1.8
        band.append((float(x), y))
    luminous_line(glow_draw, band, (120, 220, 255), 1.5,
                  (245, 252, 255))
    glow_layer = glow_layer.filter(
        ImageFilter.GaussianBlur(sp(1.8 if dense else 2.5))
    )
    scene = ImageChops.screen(scene, glow_layer)
    scene = ImageChops.screen(scene, core_layer)
    return finish(scene, bloom=1.16 if dense else 1.0,
                  exposure=1.03 if dense else 0.98,
                  grain_seed=211 if dense else 223)


def render_spectral_horizon(phase: float, frame: int) -> Image.Image:
    return _spectral_needles(phase, False)


def render_prism_rain(phase: float, frame: int) -> Image.Image:
    return _spectral_needles(phase, True)


def render_biolume_weave(phase: float, frame: int) -> Image.Image:
    scene = background("forest")
    body = Image.new("RGB", WORK_SIZE, (0, 0, 0))
    draw = ImageDraw.Draw(body)
    count = 32
    x0 = 31.0
    spacing = 8.15
    for index in range(count):
        t = index / (count - 1)
        taper = math.sin(math.pi * min(1.0, max(0.0, t))) ** 0.34
        x = x0 + index * spacing + 3.2 * math.sin(phase + index * 0.19)
        y = 75.0 + 5.0 * math.sin(phase * 1.0 + index * 0.31)
        y += 2.0 * math.sin(phase * 2.0 - index * 0.17)
        rx = (5.4 + 2.6 * taper) * (0.95 + 0.05 * math.sin(phase + index))
        ry = 7.0 + 5.0 * taper
        colour = mix((7, 72, 42), (74, 255, 155), 0.28 + 0.58 * taper)
        ellipse(draw, (x - rx, y - ry, x + rx, y + ry),
                fill=dim(colour, 0.82))
        ellipse(draw, (x - rx * 0.72, y - ry * 0.76,
                       x + rx * 0.72, y + ry * 0.12),
                fill=mix(colour, (190, 255, 215), 0.42))
        # Original signal fibres: they propagate like roots, not animal legs.
        fibres = 1 + (index % 3 == 0)
        for fibre in range(fibres):
            length = 8.0 + ((index * 7 + fibre * 11) % 15)
            bend = 4.0 * math.sin(phase * 2.0 + index * 0.73 + fibre)
            path = [(x + (fibre - 0.5) * 3.0, y + ry * 0.45),
                    (x + bend * 0.45, y + ry + length * 0.48),
                    (x + bend, y + ry + length)]
            luminous_line(draw, path, (25, 195, 111), 0.7,
                          (135, 255, 197))
        if index % 4 == 0:
            ellipse(draw, (x - 1.0, y - ry * 0.8 - 1.0,
                           x + 1.0, y - ry * 0.8 + 1.0),
                    fill=(190, 255, 220))
    # Fine dorsal filament and tapered signal tail.
    ridge = []
    for index in range(count):
        x = x0 + index * spacing + 3.2 * math.sin(phase + index * 0.19)
        y = 66.0 + 4.0 * math.sin(phase + index * 0.31)
        ridge.append((x, y))
    luminous_line(draw, ridge, (40, 220, 132), 0.8, (170, 255, 210))
    tail_start = ridge[-1]
    tail = [tail_start,
            (tail_start[0] + 13, tail_start[1] - 4 * math.sin(phase)),
            (tail_start[0] + 26, tail_start[1] + 5 * math.cos(phase)),
            (tail_start[0] + 35, tail_start[1] - 2)]
    luminous_line(draw, tail, (30, 190, 112), 0.75, (115, 255, 183))
    scene = ImageChops.screen(scene, body)
    return finish(scene, bloom=0.72, exposure=0.92, grain_seed=307)


def render_electric_script(phase: float, frame: int) -> Image.Image:
    scene = background("ember")
    layer = Image.new("RGB", WORK_SIZE, (0, 0, 0))
    draw = ImageDraw.Draw(layer)
    route = [(-34, 91), (4, 89), (18, 75), (42, 77), (58, 59),
             (83, 63), (100, 45), (123, 53), (143, 39), (166, 47),
             (183, 68), (207, 62), (225, 80), (249, 76), (267, 95),
             (291, 91), (332, 105)]
    # Three independent runes travel on the authored route with phosphor trails.
    stepped_phase = math.floor((phase / TAU * 12.0) % 12.0) / 12.0
    for packet in range(3):
        head = (stepped_phase + packet / 3.0) % 1.0
        reveal = head * (len(route) - 1)
        head_index = int(reveal)
        for trail in range(4):
            segment = head_index - trail
            if segment < 0 or segment >= len(route) - 1:
                continue
            amount = 1.0 - trail / 4.0
            p0 = route[segment]
            p1 = route[segment + 1]
            luminous_line(draw, [p0, p1], dim((25, 77, 255), 0.45 + amount * 0.55),
                          2.0 + amount, mix((60, 130, 255), (255, 255, 255), 0.42))
        if 0 <= head_index < len(route) - 1:
            local = reveal - head_index
            ax, ay = route[head_index]
            bx, by = route[head_index + 1]
            hx = ax + (bx - ax) * local
            hy = ay + (by - ay) * local
            # An original angular packet, deliberately not a hand or creature.
            glyph = [(hx - 10, hy), (hx - 3, hy - 7), (hx + 2, hy - 3),
                     (hx + 10, hy - 10), (hx + 13, hy - 5),
                     (hx + 5, hy + 4), (hx - 3, hy + 3)]
            luminous_line(draw, glyph, (30, 95, 255), 2.2, (155, 205, 255))

    # Warm tuning beacon: a neutral geometric anchor, not a copied letterform.
    beacon_x = 170.0
    beacon_y = 103.0
    pulse = 0.75 + 0.25 * math.sin(phase * 2.0) ** 2
    rectangle(draw, (beacon_x - 11, beacon_y - 18,
                     beacon_x + 11, beacon_y + 18),
              fill=dim((255, 54, 5), 0.38 * pulse))
    rectangle(draw, (beacon_x - 7, beacon_y - 15,
                     beacon_x - 2, beacon_y + 14), fill=(255, 112, 10))
    rectangle(draw, (beacon_x + 3, beacon_y - 15,
                     beacon_x + 8, beacon_y + 14), fill=(255, 112, 10))
    rectangle(draw, (beacon_x - 2, beacon_y - 15,
                     beacon_x + 4, beacon_y - 10), fill=(255, 172, 28))
    scene = ImageChops.screen(scene, layer)
    return finish(scene, bloom=0.94, exposure=1.03, grain_seed=401)


_apparition_particles = stable_noise(94117, 1900)
_apparition_stars = stable_noise(22271, 145)


def render_particle_apparition(phase: float, frame: int) -> Image.Image:
    scene = background("ocean")
    layer = Image.new("RGB", WORK_SIZE, (0, 0, 0))
    draw = ImageDraw.Draw(layer)
    for x_seed, y_seed, brightness in _apparition_stars:
        x = x_seed * WIDTH
        y = y_seed * HEIGHT
        twinkle = 0.48 + 0.52 * math.sin(phase * (1 + int(brightness * 2)) +
                                                brightness * 19.0) ** 2
        radius = 0.25 + brightness * 1.0
        colour = dim((190, 218, 255), 0.25 + 0.75 * twinkle)
        ellipse(draw, (x - radius, y - radius, x + radius, y + radius),
                fill=colour)

    gather = 0.72 + 0.28 * math.sin(phase * 0.5 - math.pi / 2.0) ** 2
    for index, (u, v, weight) in enumerate(_apparition_particles):
        theta = (u * 2.0 - 1.0) * math.pi
        wing = math.sin(u * math.pi) ** 0.58
        target_x = 160.0 + (u * 2.0 - 1.0) * 103.0
        target_y = 83.0 - 42.0 * wing * math.sin(theta * 0.62 + phase)
        target_y += 20.0 * math.sin(theta * 1.9) * (0.2 + v * 0.8)
        normal_x = math.cos(theta * 1.4 + phase) * (v - 0.5) * 38.0
        normal_y = math.sin(theta * 1.7 - phase * 2.0) * (v - 0.5) * 28.0
        diffuse_x = 32.0 + ((u * 977.0 + v * 431.0) % 1.0) * 256.0
        diffuse_y = 20.0 + ((v * 811.0 + u * 197.0) % 1.0) * 130.0
        x = diffuse_x * (1.0 - gather) + (target_x + normal_x) * gather
        y = diffuse_y * (1.0 - gather) + (target_y + normal_y) * gather
        x += 7.0 * math.sin(phase + index * 0.071) * (1.0 - weight)
        y += 4.0 * math.cos(phase * 2.0 + index * 0.053)
        intensity = 0.24 + weight * 0.76
        colour = mix((55, 155, 195), (255, 255, 242), weight ** 0.55)
        radius = 0.35 + 0.85 * weight
        ellipse(draw, (x - radius, y - radius, x + radius, y + radius),
                fill=dim(colour, intensity))
    nucleus_x = 160.0 + 12.0 * math.sin(phase)
    nucleus_y = 79.0 + 8.0 * math.cos(phase * 2.0)
    ellipse(draw, (nucleus_x - 5, nucleus_y - 5,
                   nucleus_x + 5, nucleus_y + 5), fill=(235, 255, 255))
    scene = ImageChops.screen(scene, layer)
    return finish(scene, bloom=1.08, exposure=1.04, grain_seed=503)


_voxel_particles = stable_noise(51193, 510)


def render_voxel_bloom(phase: float, frame: int) -> Image.Image:
    scene = background("black")
    layer = Image.new("RGB", WORK_SIZE, (0, 0, 0))
    draw = ImageDraw.Draw(layer)
    for index, (angle_seed, radius_seed, weight) in enumerate(_voxel_particles):
        angle = angle_seed * TAU
        shell = 0.72 + radius_seed * 0.54
        wobble = 1.0 + 0.075 * math.sin(phase * 2.0 + index * 0.67)
        rx = 101.0 * shell * wobble
        ry = 63.0 * shell * (1.0 + 0.08 * math.cos(phase + index * 0.51))
        x = 160.0 + math.cos(angle) * rx
        y = 84.0 + math.sin(angle) * ry
        x += 8.0 * math.sin(phase + radius_seed * 17.0) * (shell - 0.7)
        y += 5.0 * math.cos(phase * 2.0 + angle_seed * 13.0)
        size = 0.7 + weight * 3.8
        colour = hsv(angle_seed + 0.83 + 0.04 * math.sin(phase),
                     0.88, 0.55 + weight * 0.45)
        if weight > 0.58:
            rectangle(draw, (x - size, y - size, x + size, y + size),
                      outline=mix(colour, (255, 255, 255), 0.18),
                      width=max(1, sp(0.65 + weight * 0.8)))
        else:
            rectangle(draw, (x - size * 0.55, y - size * 0.55,
                             x + size * 0.55, y + size * 0.55),
                      fill=dim(colour, 0.52 + weight * 0.48))
    # Preserve the decisive black cavity.
    ellipse(draw, (87, 37, 233, 131), fill=(0, 0, 0))
    scene = ImageChops.screen(scene, layer)
    return finish(scene, bloom=0.78, exposure=1.08, grain_seed=607)


def render_noise_chrysalis(phase: float, frame: int) -> Image.Image:
    scene = background("black")
    layer = Image.new("RGB", WORK_SIZE, (0, 0, 0))
    draw = ImageDraw.Draw(layer)
    cx, cy = 160.0, 84.0
    # Dense cross-section traces fill the volume.  Concentric outlines alone
    # create a hollow donut; the desired object is a turbulent body with only
    # its two lateral caustics behaving like a shell.
    trace_count = 92
    for trace in range(trace_count):
        q = trace / (trace_count - 1) * 2.0 - 1.0
        radius_x = 82.0 * math.sqrt(max(0.0, 1.0 - q * q))
        base_y = cy + q * 56.0
        base_y += math.sin(phase * (2 + trace % 3) + trace * 0.71) * 3.2
        path: list[tuple[float, float]] = []
        for step in range(81):
            p = step / 80.0
            x = cx - radius_x + p * radius_x * 2.0
            edge = math.sin(p * math.pi) ** 0.42
            turbulence = math.sin(p * TAU * 8.0 + trace * 0.73 + phase * 4.0)
            turbulence += 0.54 * math.sin(p * TAU * 17.0 - trace * 0.31 - phase * 3.0)
            y = base_y + turbulence * (2.1 + 2.6 * edge)
            x += math.sin(p * TAU * 3.0 + trace * 0.41) * 2.2
            path.append((x, y))
        shade = 0.10 + 0.23 * ((trace * 17) % 11) / 10.0
        line(draw, path, dim((205, 220, 228), shade), 0.38 + (trace % 3) * 0.10)
    for shell in range(16):
        rx = 74.0 + 9.0 * math.sin(shell * 1.37 + phase)
        ry = 49.0 + 6.0 * math.cos(shell * 1.91 - phase)
        shell_path = []
        for step in range(81):
            angle = step / 80.0 * TAU
            noise = math.sin(angle * 11.0 + shell * 0.63 + phase * 3.0) * 3.2
            shell_path.append((cx + math.cos(angle) * (rx + noise),
                               cy + math.sin(angle) * (ry + noise * 0.5)))
        line(draw, shell_path, dim((215, 226, 232), 0.13), 0.45)
    # Side caustics provide the same shell logic without tracing a source orb.
    for side in (-1, 1):
        x = cx + side * 82.0
        for offset in range(-7, 8):
            length = 8.0 + (7 - abs(offset)) * 1.8
            colour = dim((235, 250, 255), 0.28 + (7 - abs(offset)) * 0.07)
            luminous_line(draw, [(x, cy + offset * 1.2),
                                 (x + side * length, cy + offset * 0.7)],
                          colour, 0.55, (245, 255, 255))
    scene = ImageChops.screen(scene, layer)
    return finish(scene, bloom=0.54, exposure=0.9, grain_seed=701)


_rain_particles = stable_noise(61981, 860)
_floor_nodes = stable_noise(17011, 150)


def render_wire_rain(phase: float, frame: int) -> Image.Image:
    scene = background("black")
    layer = Image.new("RGB", WORK_SIZE, (0, 0, 0))
    draw = ImageDraw.Draw(layer)
    horizon = 108.0
    vanishing = (160.0, horizon)
    # Perspective floor with non-uniform depth spacing.
    for index in range(1, 10):
        depth = index / 9.0
        y = horizon + (HEIGHT - horizon) * depth ** 1.85
        line(draw, [(0, y), (WIDTH, y)], dim((180, 195, 205), 0.24 + depth * 0.24), 0.55)
    for x in range(-80, 401, 24):
        line(draw, [vanishing, (float(x), HEIGHT)], dim((175, 192, 205), 0.34), 0.55)
    for x_seed, y_seed, weight in _floor_nodes:
        depth = y_seed ** 0.54
        y = horizon + depth * (HEIGHT - horizon)
        spread = (y - horizon) / (HEIGHT - horizon)
        x = 160.0 + (x_seed * 2.0 - 1.0) * 200.0 * spread
        radius = 0.35 + depth * 1.35
        ellipse(draw, (x - radius, y - radius, x + radius, y + radius),
                fill=dim((230, 240, 245), 0.42 + weight * 0.48))

    # Integer wrap rates guarantee a true seamless rain loop.
    for index, (x_seed, y_seed, weight) in enumerate(_rain_particles):
        wraps = 1 + index % 3
        y_norm = (y_seed + phase / TAU * wraps) % 1.0
        y = 3.0 + y_norm * (horizon + 20.0)
        spread = 0.26 + 0.74 * y_norm
        x = 160.0 + (x_seed * 2.0 - 1.0) * 190.0 * spread
        x += 5.0 * math.sin(phase + index * 0.17) * y_norm
        length = 0.8 + weight * 4.5 + y_norm * 3.0
        shade = 0.18 + 0.62 * weight
        line(draw, [(x, y - length), (x + (x - 160.0) * 0.018, y)],
             dim((225, 237, 244), shade), 0.45 + weight * 0.55)
    # A restrained mesh canopy, redrawn from procedural neighbours each frame.
    canopy = [(30 + item[0] * 260, 20 + item[1] * 85) for item in _floor_nodes[:54]]
    for index, point in enumerate(canopy):
        for jump in (1, 7):
            other = canopy[(index + jump) % len(canopy)]
            if abs(point[0] - other[0]) < 64 and abs(point[1] - other[1]) < 34:
                line(draw, [point, other], dim((175, 190, 202), 0.12), 0.38)
    scene = ImageChops.screen(scene, layer)
    return finish(scene, bloom=0.44, exposure=0.92, grain_seed=809)


def render_focus_rail_hq(phase: float, frame: int) -> Image.Image:
    scene = background("black")
    layer = Image.new("RGB", WORK_SIZE, (0, 0, 0))
    draw = ImageDraw.Draw(layer)
    font = ImageFont.load_default(size=sp(11))
    labels = ["MIST", "GLASS", "FIELD", "TIDE", "VOID"]
    # Four slow focus moves with long rests, matching instrument calm rather
    # than continuously scrolling like a generic software menu.
    # Cosine travel visits all five rows and returns to the first with matching
    # velocity, so the exported loop has no menu-selection jump at its seam.
    position = (1.0 - math.cos(phase)) * 2.0
    focus_y = 30.0 + position * 27.0
    for row, label in enumerate(labels):
        y = 30.0 + row * 27.0
        distance = abs(y - focus_y)
        active = max(0.0, 1.0 - distance / 27.0)
        shade = 0.18 + active * 0.82
        colour = dim((232, 244, 255), shade)
        ellipse(draw, (21, y - 8, 37, y + 8), outline=colour,
                width=max(1, sp(0.6 + active * 0.8)))
        draw.text((sp(25), sp(y - 6)), str(row + 1), font=font,
                  fill=colour, anchor="la")
        rectangle(draw, (48, y - 9, 78, y + 9), outline=colour,
                  width=max(1, sp(0.6 + active)))
        rectangle(draw, (87, y - 9, 154 + row * 9, y + 9),
                  outline=colour, width=max(1, sp(0.6 + active)))
        draw.text((sp(94), sp(y - 6)), label, font=font, fill=colour)
    line(draw, [(0, focus_y + 13.5), (WIDTH, focus_y + 13.5)],
         dim((220, 240, 255), 0.28), 0.5)
    line(draw, [(0, focus_y - 13.5), (WIDTH, focus_y - 13.5)],
         dim((220, 240, 255), 0.28), 0.5)
    # Original sound-state glyphs on the right.
    for index in range(5):
        x = 204.0 + index * 23.0
        y = focus_y + math.sin(phase + index) * 2.0
        radius = 4.0 + (index % 3) * 2.0
        pts = []
        for step in range(9):
            angle = TAU * step / 8.0
            r = radius * (1.0 + 0.28 * math.sin(angle * 3.0 + phase + index))
            pts.append((x + math.cos(angle) * r, y + math.sin(angle) * r))
        luminous_line(draw, pts, (102, 195, 255), 0.7, (225, 250, 255))
    scene = ImageChops.screen(scene, layer)
    return finish(scene, bloom=0.30, exposure=0.9, grain_seed=907)


@dataclasses.dataclass(frozen=True)
class Effect:
    slug: str
    name: str
    delay_ms: int
    render: Callable[[float, int], Image.Image]
    still_fraction: float = 0.5


EFFECTS = [
    Effect("radiant-threshold", "RADIANT THRESHOLD", 42, render_radiant_threshold),
    Effect("spectral-horizon", "SPECTRAL HORIZON", 50, render_spectral_horizon),
    Effect("prism-rain", "PRISM RAIN", 42, render_prism_rain),
    Effect("biolume-weave", "BIOLUME WEAVE", 62, render_biolume_weave),
    Effect("electric-script", "ELECTRIC SCRIPT", 80, render_electric_script),
    Effect("particle-apparition", "PARTICLE APPARITION", 42,
           render_particle_apparition, 0.94),
    Effect("voxel-bloom", "VOXEL BLOOM", 50, render_voxel_bloom),
    Effect("noise-chrysalis", "NOISE CHRYSALIS", 42, render_noise_chrysalis),
    Effect("wire-rain", "WIRE RAIN", 42, render_wire_rain),
    Effect("focus-rail-hq", "FOCUS RAIL HQ", 80, render_focus_rail_hq),
]


def global_palette(frames: Sequence[Image.Image]) -> Image.Image:
    columns = 8
    tile = (80, 43)
    rows = math.ceil(len(frames) / columns)
    atlas = Image.new("RGB", (columns * tile[0], rows * tile[1] + 64), (0, 0, 0))
    for index, frame in enumerate(frames):
        sample = frame.resize(tile, Image.Resampling.LANCZOS)
        atlas.paste(sample, ((index % columns) * tile[0],
                             (index // columns) * tile[1]))
    return atlas.quantize(colors=255, method=Image.Quantize.MEDIANCUT)


def save_animation(effect: Effect, frames: Sequence[Image.Image],
                   output_dir: pathlib.Path) -> None:
    palette = global_palette(frames)
    indexed = [frame.quantize(palette=palette, dither=Image.Dither.NONE)
               for frame in frames]
    gif_path = output_dir / f"{effect.slug}.gif"
    indexed[0].save(
        gif_path,
        save_all=True,
        append_images=indexed[1:],
        duration=effect.delay_ms,
        loop=0,
        disposal=2,
        optimize=False,
        comment=b"Sancte3D original cinematic display loop",
    )
    still_index = min(len(frames) - 1,
                      max(0, round((len(frames) - 1) * effect.still_fraction)))
    frames[still_index].save(output_dir / f"{effect.slug}.png", optimize=True)

    columns = 8
    tile_w, tile_h, label_h, gap = 160, 85, 18, 5
    rows = math.ceil(len(frames) / columns)
    sheet = Image.new("RGB", (columns * tile_w + (columns + 1) * gap,
                              rows * (tile_h + label_h) + (rows + 1) * gap),
                      (2, 3, 6))
    draw = ImageDraw.Draw(sheet)
    font = ImageFont.load_default(size=11)
    for index, frame in enumerate(frames):
        x = gap + (index % columns) * (tile_w + gap)
        y = gap + (index // columns) * (tile_h + label_h + gap)
        sheet.paste(frame.resize((tile_w, tile_h), Image.Resampling.LANCZOS), (x, y))
        draw.text((x + 2, y + tile_h + 3), f"FRAME {index:02d}",
                  fill=(190, 200, 216), font=font)
    timeline_dir = output_dir / "frame-sheets"
    timeline_dir.mkdir(parents=True, exist_ok=True)
    sheet.save(timeline_dir / f"{effect.slug}-all-frames.png", optimize=True)


def make_contact_sheet(effects: Sequence[Effect], output_dir: pathlib.Path) -> None:
    columns = 5
    tile_w, tile_h, label_h, gap = 320, 170, 28, 10
    rows = math.ceil(len(effects) / columns)
    sheet = Image.new("RGB", (columns * tile_w + (columns + 1) * gap,
                              rows * (tile_h + label_h) + (rows + 1) * gap),
                      (2, 3, 6))
    draw = ImageDraw.Draw(sheet)
    font = ImageFont.load_default(size=15)
    for index, effect in enumerate(effects):
        with Image.open(output_dir / f"{effect.slug}.png") as source:
            still = source.convert("RGB").resize((tile_w, tile_h), Image.Resampling.LANCZOS)
        x = gap + (index % columns) * (tile_w + gap)
        y = gap + (index // columns) * (tile_h + label_h + gap)
        sheet.paste(still, (x, y))
        draw.text((x + 4, y + tile_h + 5), effect.name,
                  fill=(226, 232, 244), font=font)
    sheet.save(output_dir / "high-quality-contact-sheet.png", optimize=True)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=pathlib.Path,
                        default=pathlib.Path("ui/previews/high-quality"))
    parser.add_argument("--frames", type=int, default=48)
    parser.add_argument("--effect", action="append", choices=[effect.slug for effect in EFFECTS])
    args = parser.parse_args()
    if args.frames < 12 or args.frames > 120:
        raise SystemExit("--frames must be between 12 and 120")
    selected = [effect for effect in EFFECTS
                if not args.effect or effect.slug in args.effect]
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for effect in selected:
        frames = []
        for frame in range(args.frames):
            phase = TAU * frame / args.frames
            frames.append(effect.render(phase, frame))
        save_animation(effect, frames, args.output_dir)
        print(f"rendered {effect.slug}: {args.frames} frames")
    make_contact_sheet(selected, args.output_dir)


if __name__ == "__main__":
    main()
