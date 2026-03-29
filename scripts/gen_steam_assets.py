#!/usr/bin/env python3
"""
Dead Sector — Steam asset generator.

Rules:
  • All store + library capsules keep the "DEAD SECTOR" title (no subtitle/taglines).
  • library_hero.png is pure artwork — no text, no logo at all.
  • logo.png is a transparent PNG with just the title glow.

Usage:
    python3 scripts/gen_steam_assets.py

Output (dead-sector/steam_assets/):
    logo.png               1280 ×  380   transparent PNG — game title for Steam overlay
    header_capsule.png      920 ×  430   store header / recommended
    small_capsule.png       462 ×  174   search / lists
    main_capsule.png       1232 ×  706   front-page featured carousel
    vertical_capsule.png    748 ×  896   seasonal sale pages
    page_background.png    1438 ×  810   store page ambient background (Valve templates it)
    library_capsule.png     600 ×  900   library portrait
    library_header.png     3840 × 1240   library banner
    library_hero.png       3840 × 1240   library spotlight — pure art, no text/logo
"""

import math, os
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy.ndimage import gaussian_filter, map_coordinates

ROOT = os.path.dirname(os.path.abspath(__file__))
FONT = os.path.join(ROOT, "../assets/fonts/ShareTechMono-Regular.ttf")
OUT  = os.path.join(ROOT, "../steam_assets")

CYAN = (80, 220, 255)
RED  = (255, 40, 60)

GLOW_FULL = [(1.0, 1.76), (3, 1.28), (7, 0.72), (15, 0.32), (30, 0.13)]
GLOW_SHIP = [(1.0, 0.96), (3, 0.64), (7, 0.30), (15, 0.11)]
GLOW_GRID = [(1.5, 0.56)]


# ---------------------------------------------------------------------------
# HDR canvas
# ---------------------------------------------------------------------------
def blank(w, h):
    return np.zeros((h, w, 3), dtype=np.float32)

def to_image(canvas):
    return Image.fromarray(np.clip(canvas * 255, 0, 255).astype(np.uint8), "RGB")

def add_mask(canvas, mask_np, color, glow_levels, dim=1.0):
    cr, cg, cb = color[0]/255.0, color[1]/255.0, color[2]/255.0
    canvas[:, :, 0] += mask_np * cr * dim * 1.6
    canvas[:, :, 1] += mask_np * cg * dim * 1.6
    canvas[:, :, 2] += mask_np * cb * dim * 1.6
    for sigma, strength in glow_levels:
        bl = gaussian_filter(mask_np, sigma=sigma)
        s  = strength * dim
        canvas[:, :, 0] += bl * cr * s
        canvas[:, :, 1] += bl * cg * s
        canvas[:, :, 2] += bl * cb * s


# ---------------------------------------------------------------------------
# Grid
# ---------------------------------------------------------------------------
def draw_grid(canvas, spacing=80, alpha=22):
    H, W = canvas.shape[:2]
    mask = np.zeros((H, W), dtype=np.float32)
    for x in range(0, W, spacing):
        mask[:, x] = 1.0
    for y in range(0, H, spacing):
        mask[y, :] = 1.0
    add_mask(canvas, mask * (alpha / 255.0), CYAN, GLOW_GRID, dim=1.0)


# ---------------------------------------------------------------------------
# Primitives
# ---------------------------------------------------------------------------
def _mask(canvas):
    H, W = canvas.shape[:2]
    return Image.new("L", (W, H), 0)

def glow_line(canvas, x0, y0, x1, y1, width=2, glow=GLOW_FULL, dim=1.0):
    m = _mask(canvas)
    ImageDraw.Draw(m).line([(int(x0),int(y0)),(int(x1),int(y1))], fill=255, width=width)
    add_mask(canvas, np.array(m, dtype=np.float32)/255.0, CYAN, glow, dim)

def glow_poly(canvas, pts, color=CYAN, width=2, glow=GLOW_FULL, dim=1.0):
    m = _mask(canvas)
    d = ImageDraw.Draw(m)
    n = len(pts)
    for i in range(n):
        a, b = pts[i], pts[(i+1)%n]
        d.line([(int(a[0]),int(a[1])),(int(b[0]),int(b[1]))], fill=255, width=width)
    add_mask(canvas, np.array(m, dtype=np.float32)/255.0, color, glow, dim)


# ---------------------------------------------------------------------------
# Text
# ---------------------------------------------------------------------------
def draw_text(canvas, text, size, cx, cy, letter_spacing=0):
    H, W = canvas.shape[:2]
    font  = ImageFont.truetype(FONT, size)
    chars = list(text)
    probe = ImageDraw.Draw(Image.new("L", (1, 1)))
    char_ws = [probe.textbbox((0,0),ch,font=font)[2]
               - probe.textbbox((0,0),ch,font=font)[0] for ch in chars]
    total_w = sum(char_ws) + letter_spacing * max(0, len(chars)-1)
    bb_h    = probe.textbbox((0,0), "DEAD SECTOR|", font=font)
    char_h  = bb_h[3] - bb_h[1]
    pad     = 90
    glyph   = Image.new("L", (total_w + pad*2, char_h + pad*2), 0)
    gd      = ImageDraw.Draw(glyph)
    x       = pad
    for i, ch in enumerate(chars):
        gd.text((x, pad), ch, fill=255, font=font)
        x += char_ws[i] + letter_spacing
    dx = cx - (total_w//2 + pad)
    dy = cy - (char_h//2  + pad)
    full = Image.new("L", (W, H), 0)
    full.paste(glyph, (dx, dy))
    add_mask(canvas, np.array(full, dtype=np.float32)/255.0, CYAN, GLOW_FULL, dim=1.0)

def title_line(canvas, font_size, cy, letter_spacing, ul_half_w, tick=14):
    """Single-line 'DEAD SECTOR' + underline with end ticks."""
    W = canvas.shape[1]
    draw_text(canvas, "DEAD SECTOR", font_size, W//2, cy, letter_spacing)
    ul_y = cy + int(font_size * 0.70)
    glow_line(canvas, W//2 - ul_half_w, ul_y, W//2 + ul_half_w, ul_y, width=3)
    for sx in [W//2 - ul_half_w, W//2 + ul_half_w]:
        glow_line(canvas, sx, ul_y - tick, sx, ul_y + tick, width=2)

def title_2line(canvas, font_size, cy1, cy2, ul_half_w, tick=11):
    """Two-line DEAD / SECTOR stacked + underline."""
    W = canvas.shape[1]
    draw_text(canvas, "DEAD",   font_size, W//2, cy1, letter_spacing=6)
    draw_text(canvas, "SECTOR", font_size, W//2, cy2, letter_spacing=6)
    ul_y = cy2 + int(font_size * 0.62)
    glow_line(canvas, W//2 - ul_half_w, ul_y, W//2 + ul_half_w, ul_y, width=2)
    for sx in [W//2 - ul_half_w, W//2 + ul_half_w]:
        glow_line(canvas, sx, ul_y - tick, sx, ul_y + tick, width=2)


# ---------------------------------------------------------------------------
# Ships
# ---------------------------------------------------------------------------
DELTA   = [(0,-30),(5,-11),(17,5),(8,11),(5,17),(3,25),(0,19),(-3,25),
            (-5,17),(-8,11),(-17,5),(-5,-11)]
HUNTER  = [(0,-18),(9,12),(-9,12)]
PHANTOM = [(math.cos(i*math.pi/4 - math.pi/4) * (16 if i%2==0 else 6),
            math.sin(i*math.pi/4 - math.pi/4) * (16 if i%2==0 else 6))
           for i in range(8)]

def _xform(hull, cx, cy, angle_deg, scale):
    rad = math.radians(angle_deg)
    ca, sa = math.cos(rad), math.sin(rad)
    return [(cx + (x*ca - y*sa)*scale, cy + (x*sa + y*ca)*scale) for x,y in hull]

def place_ship(canvas, cx, cy, angle_deg, scale, dim=1.0):
    glow_poly(canvas, _xform(DELTA, cx, cy, angle_deg, scale),
              CYAN, width=2, glow=GLOW_SHIP, dim=dim)

def place_enemy(canvas, hull, cx, cy, angle_deg, scale, dim=0.72):
    glow_poly(canvas, _xform(hull, cx, cy, angle_deg, scale),
              RED, width=2, glow=GLOW_SHIP, dim=dim)


# ---------------------------------------------------------------------------
# CRT
# ---------------------------------------------------------------------------
def apply_crt(img, seed=42, barrel=0.10, scanline=0.62,
              chroma_shift=3, grain=0.012, vignette=0.65):
    rng = np.random.RandomState(seed)
    arr = np.array(img, dtype=np.float32) / 255.0
    H, W = arr.shape[:2]
    cx2, cy2 = W/2.0, H/2.0
    yi, xi = np.mgrid[0:H, 0:W]
    xn, yn = (xi-cx2)/cx2, (yi-cy2)/cy2
    r2 = xn**2 + yn**2
    xs = (xn*(1+barrel*r2))*cx2 + cx2
    ys = (yn*(1+barrel*r2))*cy2 + cy2
    warped = np.zeros_like(arr)
    for c in range(3):
        warped[:,:,c] = map_coordinates(arr[:,:,c],[ys,xs],order=1,mode='constant',cval=0)
    arr = warped
    sl = np.ones(H, dtype=np.float32); sl[::2] = scanline
    arr *= sl[:, np.newaxis, np.newaxis]
    ph = np.ones(W, dtype=np.float32); ph[::3] = 0.96
    arr *= ph[np.newaxis, :, np.newaxis]
    arr[:,:,0] = np.roll(arr[:,:,0], -chroma_shift, axis=1)
    arr[:,:,2] = np.roll(arr[:,:,2],  chroma_shift, axis=1)
    arr[:, :chroma_shift, 2] = 0; arr[:, -chroma_shift:, 0] = 0
    xv, yv = np.linspace(-1,1,W), np.linspace(-1,1,H)
    Xv, Yv = np.meshgrid(xv, yv)
    arr *= np.clip(1.0 - vignette*(Xv**2+Yv**2), 0, 1)[:,:,np.newaxis] ** 1.2
    arr += rng.randn(H,W).astype(np.float32)[:,:,np.newaxis] * grain
    arr[:,:,1] *= 1.04; arr[:,:,0] *= 0.93
    return Image.fromarray(np.clip(arr*255,0,255).astype(np.uint8), "RGB")

def apply_crt_small(img, seed=42):
    rng = np.random.RandomState(seed)
    arr = np.array(img, dtype=np.float32) / 255.0
    H, W = arr.shape[:2]
    sl = np.ones(H, dtype=np.float32); sl[::2] = 0.72
    arr *= sl[:, np.newaxis, np.newaxis]
    arr[:,:,0] = np.roll(arr[:,:,0], -1, axis=1)
    arr[:,:,2] = np.roll(arr[:,:,2],  1, axis=1)
    arr[:, :1, 2] = 0; arr[:, -1:, 0] = 0
    xv, yv = np.linspace(-1,1,W), np.linspace(-1,1,H)
    Xv, Yv = np.meshgrid(xv, yv)
    arr *= np.clip(1.0 - 0.55*(Xv**2+Yv**2), 0, 1)[:,:,np.newaxis] ** 1.1
    arr += rng.randn(H,W).astype(np.float32)[:,:,np.newaxis] * 0.008
    arr[:,:,1] *= 1.03; arr[:,:,0] *= 0.94
    return Image.fromarray(np.clip(arr*255,0,255).astype(np.uint8), "RGB")


# ---------------------------------------------------------------------------
# Asset builders
# ---------------------------------------------------------------------------

def build_logo():
    """Transparent PNG — title glow, black bg → alpha."""
    W, H = 1280, 380
    c = blank(W, H)
    draw_text(c, "DEAD SECTOR", 148, W//2, 160, letter_spacing=8)
    ul_y = 248
    glow_line(c, W//2-540, ul_y, W//2+540, ul_y, width=3)
    for sx in [W//2-540, W//2+540]:
        glow_line(c, sx, ul_y-14, sx, ul_y+14, width=2)
    rgb   = np.clip(c * 255, 0, 255).astype(np.uint8)
    alpha = np.max(rgb, axis=2)
    return Image.fromarray(np.dstack([rgb, alpha]), "RGBA")


def build_header_capsule():
    """920 × 430"""
    W, H = 920, 430
    c = blank(W, H)
    draw_grid(c, spacing=80, alpha=22)
    # Ships in lower-left and upper/lower-right, clear of the centred title
    place_ship(c,  cx=185, cy=375, angle_deg=-22, scale=2.5)
    place_enemy(c, HUNTER,  cx=730, cy=88,  angle_deg=162, scale=3.0)
    place_enemy(c, PHANTOM, cx=760, cy=345, angle_deg=220, scale=2.4)
    title_line(c, font_size=92, cy=185, letter_spacing=6, ul_half_w=358, tick=12)
    return apply_crt(to_image(c))


def build_small_capsule():
    """462 × 174"""
    W, H = 462, 174
    c = blank(W, H)
    draw_grid(c, spacing=40, alpha=20)
    place_ship(c,  cx=42,  cy=130, angle_deg=-15, scale=1.5, dim=0.70)
    place_enemy(c, HUNTER, cx=426, cy=44,  angle_deg=165, scale=1.8, dim=0.50)
    draw_text(c, "DEAD SECTOR", 48, W//2, 88, letter_spacing=2)
    return apply_crt_small(to_image(c))


def build_main_capsule():
    """1232 × 706"""
    W, H = 1232, 706
    c = blank(W, H)
    draw_grid(c, spacing=80, alpha=22)
    # Ships above and below the centred title band
    place_ship(c,  cx=240, cy=610, angle_deg=-20, scale=3.8)
    place_enemy(c, HUNTER,  cx=1010, cy=118, angle_deg=162, scale=4.5)
    place_enemy(c, PHANTOM, cx=1045, cy=570, angle_deg=220, scale=3.6)
    title_line(c, font_size=156, cy=320, letter_spacing=10, ul_half_w=530, tick=16)
    return apply_crt(to_image(c))


def build_vertical_capsule():
    """748 × 896"""
    W, H = 748, 896
    c = blank(W, H)
    draw_grid(c, spacing=80, alpha=22)
    # Ship top-centre pointing up, enemies at bottom flanking
    place_ship(c,  cx=374, cy=185, angle_deg=0,   scale=5.5)
    place_enemy(c, HUNTER,  cx=112, cy=758, angle_deg=35,  scale=4.0)
    place_enemy(c, PHANTOM, cx=638, cy=808, angle_deg=310, scale=3.2)
    title_2line(c, font_size=108, cy1=460, cy2=572, ul_half_w=288, tick=11)
    return apply_crt(to_image(c))


def build_page_background():
    """1438 × 810 — Valve overlays their own template; no title needed."""
    W, H = 1438, 810
    c = blank(W, H)
    draw_grid(c, spacing=80, alpha=26)
    place_ship(c,  cx=255,  cy=600, angle_deg=-18, scale=6.5, dim=0.50)
    place_enemy(c, HUNTER,  cx=1185, cy=200, angle_deg=165, scale=5.5, dim=0.40)
    place_enemy(c, PHANTOM, cx=1090, cy=640, angle_deg=230, scale=4.5, dim=0.40)
    return apply_crt(to_image(c), grain=0.010, vignette=0.55)


def build_library_capsule():
    """600 × 900"""
    W, H = 600, 900
    c = blank(W, H)
    draw_grid(c, spacing=80, alpha=22)
    place_ship(c,  cx=300, cy=185, angle_deg=0,   scale=5.0)
    place_enemy(c, HUNTER,  cx=105, cy=760, angle_deg=35,  scale=3.6)
    place_enemy(c, PHANTOM, cx=500, cy=808, angle_deg=310, scale=2.8)
    title_2line(c, font_size=104, cy1=462, cy2=570, ul_half_w=238, tick=10)
    return apply_crt(to_image(c))


def build_library_header():
    """3840 × 1240"""
    W, H = 3840, 1240
    c = blank(W, H)
    draw_grid(c, spacing=160, alpha=22)
    place_ship(c,  cx=680,  cy=1045, angle_deg=-18, scale=8.5)
    place_enemy(c, HUNTER,  cx=3295, cy=268,  angle_deg=200, scale=9.5)
    place_enemy(c, PHANTOM, cx=3145, cy=990,  angle_deg=145, scale=7.5)
    title_line(c, font_size=360, cy=490, letter_spacing=22, ul_half_w=1480, tick=28)
    return apply_crt(to_image(c), chroma_shift=6, grain=0.012)


def build_library_hero():
    """3840 × 1240 — pure atmospheric art, absolutely no text or logo."""
    W, H = 3840, 1240
    c = blank(W, H)
    draw_grid(c, spacing=160, alpha=24)
    # Centred player ship, enemies at far sides — all ships are the content
    place_ship(c,  cx=1920, cy=800,  angle_deg=0,   scale=14.0)
    place_enemy(c, HUNTER,  cx=600,  cy=310,  angle_deg=145, scale=11.0, dim=0.48)
    place_enemy(c, PHANTOM, cx=3285, cy=345,  angle_deg=215, scale=9.0,  dim=0.48)
    place_enemy(c, HUNTER,  cx=440,  cy=1000, angle_deg=50,  scale=8.0,  dim=0.40)
    place_enemy(c, PHANTOM, cx=3490, cy=955,  angle_deg=300, scale=7.0,  dim=0.40)
    return apply_crt(to_image(c), chroma_shift=6, grain=0.012, vignette=0.70)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------
if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    assets = [
        ("logo.png",             build_logo),
        ("header_capsule.png",   build_header_capsule),
        ("small_capsule.png",    build_small_capsule),
        ("main_capsule.png",     build_main_capsule),
        ("vertical_capsule.png", build_vertical_capsule),
        ("page_background.png",  build_page_background),
        ("library_capsule.png",  build_library_capsule),
        ("library_header.png",   build_library_header),
        ("library_hero.png",     build_library_hero),
    ]
    for fname, builder in assets:
        path = os.path.join(OUT, fname)
        print(f"  {fname}…", end=" ", flush=True)
        img = builder()
        img.save(path)
        print(f"{img.size[0]}×{img.size[1]}")

    legacy = os.path.join(ROOT, "../dead_sector_logo.png")
    if os.path.exists(legacy) or os.path.islink(legacy):
        os.remove(legacy)
    os.symlink(os.path.abspath(os.path.join(OUT, "logo.png")), legacy)
    print(f"\nAll assets → {OUT}/")
