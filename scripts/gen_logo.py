#!/usr/bin/env python3
"""
Dead Sector — title logo generator
Produces: dead_sector_logo.png  (1280 x 480)

Vector glow style matching the in-game VectorRenderer:
  - Additive blending on black
  - Multi-scale Gaussian bloom per line/glyph
  - Cyan avatar colour (80, 220, 255)
"""

import math
import os
import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter

# ---------------------------------------------------------------------------
# Canvas
# ---------------------------------------------------------------------------
W, H = 1280, 480
FONT_PATH = os.path.join(os.path.dirname(__file__),
                         "../assets/fonts/ShareTechMono-Regular.ttf")

# Colours  (0-255)
CYAN    = (80,  220, 255)
YELLOW  = (255, 255, 100)
MAGENTA = (255,  60, 120)
WHITE   = (255, 255, 255)

# Glow blur passes (radius_px, strength multiplier)
GLOW_LEVELS = [(2, 1.2), (5, 0.7), (11, 0.35), (22, 0.15), (44, 0.07)]


# ---------------------------------------------------------------------------
# HDR float canvas helpers (additive RGB)
# ---------------------------------------------------------------------------
def blank_canvas():
    return np.zeros((H, W, 3), dtype=np.float32)


def add_glow_mask(canvas, mask_img, color, glow_levels=GLOW_LEVELS):
    """
    mask_img : PIL 'L' image (white = lit, black = unlit)
    Adds the core + blurred halo into the float canvas additively.
    """
    cr, cg, cb = color[0]/255.0, color[1]/255.0, color[2]/255.0
    core = np.array(mask_img, dtype=np.float32) / 255.0

    canvas[:, :, 0] += core * cr
    canvas[:, :, 1] += core * cg
    canvas[:, :, 2] += core * cb

    for radius, strength in glow_levels:
        blurred_img = mask_img.filter(ImageFilter.GaussianBlur(radius=radius))
        blurred = np.array(blurred_img, dtype=np.float32) / 255.0
        canvas[:, :, 0] += blurred * cr * strength
        canvas[:, :, 1] += blurred * cg * strength
        canvas[:, :, 2] += blurred * cb * strength


def canvas_to_image(canvas):
    """Tone-map float canvas → 8-bit PIL Image (simple clip)."""
    arr = np.clip(canvas * 255, 0, 255).astype(np.uint8)
    return Image.fromarray(arr, "RGB")


# ---------------------------------------------------------------------------
# Glow line drawing
# ---------------------------------------------------------------------------
def draw_glow_line(canvas, x0, y0, x1, y1, color, width=2):
    mask = Image.new("L", (W, H), 0)
    draw = ImageDraw.Draw(mask)
    draw.line([(x0, y0), (x1, y1)], fill=255, width=width)
    add_glow_mask(canvas, mask, color)


def draw_glow_poly(canvas, pts, color, width=2, closed=True):
    n = len(pts)
    edges = n if closed else n - 1
    for i in range(edges):
        a = pts[i]
        b = pts[(i + 1) % n]
        draw_glow_line(canvas, a[0], a[1], b[0], b[1], color, width)


# ---------------------------------------------------------------------------
# Ship geometry  (from src/entities/Avatar.cpp)
# ---------------------------------------------------------------------------
HULL_DELTA = [
    (  0, -30), (  5, -11), ( 17,   5), (  8,  11),
    (  5,  17), (  3,  25), (  0,  19), ( -3,  25),
    ( -5,  17), ( -8,  11), (-17,   5), ( -5, -11),
]

HULL_RAPTOR = [
    (  0,  34), (  3,  24), ( 14,  32), (  8,  14),
    ( 28,  30), ( 22,   2), (  8,  -8), (  5, -22),
    (  0, -32), ( -5, -22), ( -8,  -8), (-22,   2),
    (-28,  30), ( -8,  14), (-14,  32), ( -3,  24),
]

HULL_MANTIS = [
    (  0, -22), (  4, -16), ( 10, -20), ( 32, -34),
    ( 26,  -4), ( 14,   6), (  5,  18), (  9,  28),
    (  3,  36), (  0,  28), ( -3,  36), ( -9,  28),
    ( -5,  18), (-14,   6), (-26,  -4), (-32, -34),
    (-10, -20), ( -4, -16),
]

HULL_BLADE = [
    (  0, -44), (  1, -22), (  3,  -6), ( 22,   6),
    (  5,  16), (  4,  28), (  8,  36), (  2,  34),
    (  0,  30), ( -2,  34), ( -8,  36), ( -4,  28),
    ( -5,  16), (-22,   6), ( -3,  -6), ( -1, -22),
]


def transform_hull(hull, cx, cy, angle_deg, scale=1.0):
    """Rotate + translate hull vertices."""
    rad = math.radians(angle_deg)
    cos_a, sin_a = math.cos(rad), math.sin(rad)
    out = []
    for x, y in hull:
        sx, sy = x * scale, y * scale
        rx = sx * cos_a - sy * sin_a
        ry = sx * sin_a + sy * cos_a
        out.append((cx + rx, cy + ry))
    return out


# ---------------------------------------------------------------------------
# Text mask helper
# ---------------------------------------------------------------------------
def text_mask(text, font_size, letter_spacing=0):
    """Return a grayscale PIL Image containing the rendered text."""
    font = ImageFont.truetype(FONT_PATH, font_size)

    # Measure each character individually so we can apply custom spacing
    chars = list(text)
    widths = []
    tmp_draw = ImageDraw.Draw(Image.new("L", (1, 1)))
    for ch in chars:
        bb = tmp_draw.textbbox((0, 0), ch, font=font)
        widths.append(bb[2] - bb[0])

    total_w = sum(widths) + letter_spacing * (len(chars) - 1)

    # Height from a representative tall string
    bb = tmp_draw.textbbox((0, 0), "DEAD SECTOR|", font=font)
    char_h = bb[3] - bb[1]

    # Render with padding for glow bleed
    pad = 60
    img = Image.new("L", (total_w + pad * 2, char_h + pad * 2), 0)
    draw = ImageDraw.Draw(img)

    x = pad
    for i, ch in enumerate(chars):
        draw.text((x, pad), ch, fill=255, font=font)
        x += widths[i] + letter_spacing

    return img


def paste_text_glow(canvas, text, font_size, cx, cy, color,
                    letter_spacing=0, glow_levels=GLOW_LEVELS):
    """Centre a glowing text mask at (cx, cy) on the canvas."""
    mask = text_mask(text, font_size, letter_spacing)
    mw, mh = mask.size

    # Dest region (may extend outside canvas — PIL handles it)
    dx = cx - mw // 2
    dy = cy - mh // 2

    # Crop / pad so mask fits within canvas exactly
    full = Image.new("L", (W, H), 0)
    full.paste(mask, (dx, dy))

    add_glow_mask(canvas, full, color, glow_levels)


# ---------------------------------------------------------------------------
# Build the logo
# ---------------------------------------------------------------------------
def build_logo():
    canvas = blank_canvas()

    # --- Title "DEAD SECTOR" -------------------------------------------
    title_cx = W // 2
    title_cy = 155
    # Render as one string with an extra space as a separator gap
    paste_text_glow(canvas, "DEAD SECTOR", 130, title_cx, title_cy, CYAN,
                    letter_spacing=8)

    # --- Underline --------------------------------------------------------
    ul_y  = title_cy + 88
    ul_x0 = W // 2 - 520
    ul_x1 = W // 2 + 520
    draw_glow_line(canvas, ul_x0, ul_y, ul_x1, ul_y, CYAN, width=3)

    # Accent ticks at ends
    tick_h = 14
    draw_glow_line(canvas, ul_x0, ul_y - tick_h, ul_x0, ul_y + tick_h, CYAN, width=2)
    draw_glow_line(canvas, ul_x1, ul_y - tick_h, ul_x1, ul_y + tick_h, CYAN, width=2)

    # --- Subtitle ---------------------------------------------------------
    paste_text_glow(canvas, "A CYBERPUNK SHOOTER", 24, W // 2, ul_y + 34,
                    CYAN,
                    letter_spacing=3,
                    glow_levels=[(2, 0.8), (5, 0.4), (11, 0.2)])

    # --- Ships scattered around logo ------------------------------------
    # Left side: Delta — large, angled toward the title, yellow
    pts = transform_hull(HULL_DELTA,  cx=110, cy=160, angle_deg=-40, scale=2.8)
    draw_glow_poly(canvas, pts, YELLOW, width=2)

    # Lower-left: Raptor — smaller, tilted in, cyan
    pts = transform_hull(HULL_RAPTOR, cx=90,  cy=360, angle_deg=20,  scale=2.0)
    draw_glow_poly(canvas, pts, CYAN, width=2)

    # Right side: Blade — large, mirrored angle, magenta
    pts = transform_hull(HULL_BLADE,  cx=1170, cy=155, angle_deg=40, scale=2.6)
    draw_glow_poly(canvas, pts, MAGENTA, width=2)

    # Lower-right: Mantis — tilted inward, yellow
    pts = transform_hull(HULL_MANTIS, cx=1185, cy=355, angle_deg=-15, scale=2.0)
    draw_glow_poly(canvas, pts, YELLOW, width=2)

    # Bottom center: small Delta, pointing straight up
    pts = transform_hull(HULL_DELTA,  cx=W // 2, cy=440, angle_deg=0, scale=1.6)
    draw_glow_poly(canvas, pts, CYAN, width=2)

    # Stray projectile crosses for atmosphere
    for (px, py, col) in [
        (235, 88,  YELLOW),
        (295, 415, CYAN),
        (980, 75,  MAGENTA),
        (1040, 415, YELLOW),
        (640,  462, CYAN),
        (440,  420, MAGENTA),
        (850,  420, CYAN),
    ]:
        draw_glow_line(canvas, px - 4, py, px + 4, py, col, width=2)
        draw_glow_line(canvas, px, py - 4, px, py + 4, col, width=2)

    return canvas_to_image(canvas)


if __name__ == "__main__":
    out_path = os.path.join(os.path.dirname(__file__), "../dead_sector_logo.png")
    print("Rendering logo…")
    img = build_logo()
    img.save(out_path)
    print(f"Saved → {os.path.abspath(out_path)}")
