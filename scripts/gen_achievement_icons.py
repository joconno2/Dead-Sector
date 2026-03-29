#!/usr/bin/env python3
"""
Dead Sector — Steam achievement icon generator.

Generates 64×64 unlocked + locked icon pairs for all 20 achievements.
Uses the same HDR vector glow aesthetic as the store assets.

Usage:
    python3 scripts/gen_achievement_icons.py

Output: steam_assets/achievements/*.png
"""

import math, os
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from scipy.ndimage import gaussian_filter

ROOT = os.path.dirname(os.path.abspath(__file__))
FONT = os.path.join(ROOT, "../assets/fonts/ShareTechMono-Regular.ttf")
OUT  = os.path.join(ROOT, "../steam_assets/achievements")

S = 64  # icon size

CYAN   = (80, 220, 255)
RED    = (255, 40, 60)
GOLD   = (255, 200, 40)
GREEN  = (60, 255, 120)
PURPLE = (180, 80, 255)
WHITE  = (255, 255, 255)

# Glow levels scaled for 64px icons (smaller sigmas)
GLOW = [(0.8, 1.4), (2, 0.9), (4, 0.4), (8, 0.15)]
GLOW_SOFT = [(1, 0.7), (3, 0.3), (6, 0.1)]


# ---------------------------------------------------------------------------
# HDR primitives
# ---------------------------------------------------------------------------

def blank():
    return np.zeros((S, S, 3), dtype=np.float32)

def add_mask(canvas, mask, color, glow_levels, dim=1.0):
    cr, cg, cb = color[0]/255.0, color[1]/255.0, color[2]/255.0
    canvas[:,:,0] += mask * cr * dim * 1.6
    canvas[:,:,1] += mask * cg * dim * 1.6
    canvas[:,:,2] += mask * cb * dim * 1.6
    for sigma, strength in glow_levels:
        bl = gaussian_filter(mask, sigma=sigma)
        s = strength * dim
        canvas[:,:,0] += bl * cr * s
        canvas[:,:,1] += bl * cg * s
        canvas[:,:,2] += bl * cb * s

def _mask():
    return Image.new("L", (S, S), 0)

def glow_line(c, x0, y0, x1, y1, color=CYAN, width=1, glow=GLOW, dim=1.0):
    m = _mask()
    ImageDraw.Draw(m).line([(int(x0),int(y0)),(int(x1),int(y1))], fill=255, width=width)
    add_mask(c, np.array(m, dtype=np.float32)/255.0, color, glow, dim)

def glow_poly(c, pts, color=CYAN, width=1, glow=GLOW, dim=1.0):
    m = _mask()
    d = ImageDraw.Draw(m)
    n = len(pts)
    for i in range(n):
        a, b = pts[i], pts[(i+1)%n]
        d.line([(int(a[0]),int(a[1])),(int(b[0]),int(b[1]))], fill=255, width=width)
    add_mask(c, np.array(m, dtype=np.float32)/255.0, color, glow, dim)

def glow_circle(c, cx, cy, r, color=CYAN, width=1, glow=GLOW, dim=1.0):
    m = _mask()
    ImageDraw.Draw(m).ellipse([(int(cx-r),int(cy-r)),(int(cx+r),int(cy+r))],
                              outline=255, width=width)
    add_mask(c, np.array(m, dtype=np.float32)/255.0, color, glow, dim)

def glow_dot(c, cx, cy, r, color=CYAN, glow=GLOW, dim=1.0):
    m = _mask()
    ImageDraw.Draw(m).ellipse([(int(cx-r),int(cy-r)),(int(cx+r),int(cy+r))], fill=255)
    add_mask(c, np.array(m, dtype=np.float32)/255.0, color, glow, dim)

def glow_text(c, text, size, cx, cy, color=CYAN, glow=GLOW, dim=1.0):
    font = ImageFont.truetype(FONT, size)
    m = _mask()
    d = ImageDraw.Draw(m)
    bb = d.textbbox((0, 0), text, font=font)
    tw, th = bb[2] - bb[0], bb[3] - bb[1]
    d.text((cx - tw//2, cy - th//2), text, fill=255, font=font)
    add_mask(c, np.array(m, dtype=np.float32)/255.0, color, glow, dim)

def _xform(hull, cx, cy, angle_deg, scale):
    rad = math.radians(angle_deg)
    ca, sa = math.cos(rad), math.sin(rad)
    return [(cx + (x*ca - y*sa)*scale, cy + (x*sa + y*ca)*scale) for x,y in hull]

def to_image(canvas):
    return Image.fromarray(np.clip(canvas * 255, 0, 255).astype(np.uint8), "RGB")

def make_locked(img):
    """Desaturate and darken for locked variant."""
    arr = np.array(img, dtype=np.float32) / 255.0
    grey = 0.299 * arr[:,:,0] + 0.587 * arr[:,:,1] + 0.114 * arr[:,:,2]
    out = np.stack([grey, grey, grey], axis=2) * 0.3
    return Image.fromarray(np.clip(out * 255, 0, 255).astype(np.uint8), "RGB")

# Ship hulls
DELTA   = [(0,-30),(5,-11),(17,5),(8,11),(5,17),(3,25),(0,19),(-3,25),
            (-5,17),(-8,11),(-17,5),(-5,-11)]
HUNTER  = [(0,-18),(9,12),(-9,12)]


# ---------------------------------------------------------------------------
# Icon builders
# ---------------------------------------------------------------------------

def ach_first_kill():
    """Crosshair over small ICE triangle."""
    c = blank()
    # Hunter triangle
    glow_poly(c, _xform(HUNTER, 32, 34, 0, 1.2), RED, width=1, dim=0.8)
    # Crosshair
    glow_line(c, 32, 10, 32, 22, CYAN, dim=0.6)
    glow_line(c, 32, 46, 32, 54, CYAN, dim=0.6)
    glow_line(c, 10, 34, 22, 34, CYAN, dim=0.6)
    glow_line(c, 42, 34, 54, 34, CYAN, dim=0.6)
    glow_circle(c, 32, 34, 14, CYAN, dim=0.4)
    return to_image(c)

def ach_first_boss():
    """Octagon boss silhouette."""
    c = blank()
    sides = 8
    pts = [(32 + math.cos(i * 2*math.pi/sides) * 20,
            32 + math.sin(i * 2*math.pi/sides) * 20) for i in range(sides)]
    glow_poly(c, pts, RED, width=1, dim=0.9)
    glow_dot(c, 32, 32, 4, RED, dim=0.7)
    return to_image(c)

def ach_clear_world1():
    """'I' with green breach glow."""
    c = blank()
    glow_text(c, "I", 36, 32, 30, GREEN, dim=1.0)
    glow_circle(c, 32, 32, 24, GREEN, dim=0.3)
    return to_image(c)

def ach_clear_world2():
    """'II' with blue breach glow."""
    c = blank()
    glow_text(c, "II", 30, 32, 30, CYAN, dim=1.0)
    glow_circle(c, 32, 32, 24, CYAN, dim=0.3)
    return to_image(c)

def ach_final_breach():
    """'III' with gold breach glow."""
    c = blank()
    glow_text(c, "III", 26, 32, 30, GOLD, dim=1.0)
    glow_circle(c, 32, 32, 24, GOLD, dim=0.3)
    return to_image(c)

def ach_golden_hull():
    """Player ship in gold glow."""
    c = blank()
    glow_poly(c, _xform(DELTA, 32, 34, 0, 1.1), GOLD, width=1, glow=GLOW, dim=1.0)
    # Extra gold halo
    glow_circle(c, 32, 34, 22, GOLD, dim=0.25)
    return to_image(c)

def ach_high_trace():
    """Trace bar at 100% — horizontal bar with '100%'."""
    c = blank()
    # Bar outline
    glow_line(c, 10, 26, 54, 26, RED, width=1, dim=0.5)
    glow_line(c, 10, 36, 54, 36, RED, width=1, dim=0.5)
    glow_line(c, 10, 26, 10, 36, RED, width=1, dim=0.5)
    glow_line(c, 54, 26, 54, 36, RED, width=1, dim=0.5)
    # Filled bar
    m = _mask()
    ImageDraw.Draw(m).rectangle([(11, 27), (53, 35)], fill=180)
    add_mask(c, np.array(m, dtype=np.float32)/255.0, RED, GLOW_SOFT, dim=0.7)
    glow_text(c, "100%", 12, 32, 48, RED, GLOW_SOFT, dim=0.8)
    return to_image(c)

def ach_kill_manticore():
    """Manticore octagon with blades."""
    c = blank()
    sides = 8
    pts = [(32 + math.cos(i * 2*math.pi/sides + math.pi/8) * 16,
            32 + math.sin(i * 2*math.pi/sides + math.pi/8) * 16) for i in range(sides)]
    glow_poly(c, pts, RED, width=1, dim=0.9)
    # Spinning blades
    for i in range(4):
        a = i * math.pi / 2
        ix, iy = 32 + math.cos(a)*16, 32 + math.sin(a)*16
        ox, oy = 32 + math.cos(a)*26, 32 + math.sin(a)*26
        glow_line(c, ix, iy, ox, oy, RED, width=1, dim=0.7)
    glow_dot(c, 32, 32, 3, (255, 220, 50), dim=0.8)
    return to_image(c)

def ach_kill_archon():
    """Archon diamond core with orbiting shields."""
    c = blank()
    # Diamond core
    pts = [(32, 14), (46, 32), (32, 50), (18, 32)]
    glow_poly(c, pts, RED, width=1, dim=0.9)
    # 4 shield dots
    for i in range(4):
        a = i * math.pi / 2 + math.pi / 4
        sx, sy = 32 + math.cos(a)*22, 32 + math.sin(a)*22
        glow_dot(c, sx, sy, 2.5, CYAN, dim=0.7)
    return to_image(c)

def ach_kill_vortex():
    """Vortex hexagon with 4 head prongs."""
    c = blank()
    sides = 6
    pts = [(32 + math.cos(i * 2*math.pi/sides) * 14,
            32 + math.sin(i * 2*math.pi/sides) * 14) for i in range(sides)]
    glow_poly(c, pts, RED, width=1, dim=0.9)
    # 4 cardinal heads
    for a in [0, math.pi/2, math.pi, 3*math.pi/2]:
        ix, iy = 32 + math.cos(a)*14, 32 + math.sin(a)*14
        mx, my = 32 + math.cos(a)*22, 32 + math.sin(a)*22
        ox, oy = 32 + math.cos(a)*28, 32 + math.sin(a)*28
        glow_line(c, ix, iy, mx, my, RED, width=1, dim=0.7)
        glow_dot(c, ox, oy, 2, (255, 120, 40), dim=0.8)
    return to_image(c)

def ach_chain_kill():
    """5 small explosion bursts in a chain."""
    c = blank()
    positions = [(14, 20), (24, 40), (34, 18), (44, 42), (52, 24)]
    for i, (px, py) in enumerate(positions):
        dim = 0.6 + 0.1 * i
        for a in range(6):
            ang = a * math.pi / 3
            glow_line(c, px, py, px + math.cos(ang)*6, py + math.sin(ang)*6,
                      CYAN, width=1, dim=dim)
        glow_dot(c, px, py, 1.5, WHITE, dim=dim*0.5)
    return to_image(c)

def ach_ricochet_kill():
    """Bouncing projectile path with angle."""
    c = blank()
    # Projectile path: top-left → bounce point → top-right, hitting enemy
    glow_line(c, 8, 12, 28, 44, CYAN, width=1, dim=0.8)
    glow_line(c, 28, 44, 48, 16, CYAN, width=1, dim=0.9)
    # Bounce point spark
    glow_dot(c, 28, 44, 2, WHITE, dim=0.6)
    # Enemy at end
    glow_poly(c, _xform(HUNTER, 48, 16, 180, 0.7), RED, width=1, dim=0.7)
    # Small burst at impact
    for a in range(4):
        ang = a * math.pi / 2 + math.pi / 4
        glow_line(c, 48, 16, 48+math.cos(ang)*5, 16+math.sin(ang)*5,
                  WHITE, width=1, dim=0.4)
    return to_image(c)

def ach_stealth_kill():
    """Ghostly faded ship with dashed outline."""
    c = blank()
    # Faded ship silhouette
    glow_poly(c, _xform(DELTA, 32, 34, 0, 1.1), CYAN, width=1, dim=0.35)
    # Stealth shimmer — dashed circle
    for i in range(12):
        if i % 2 == 0:
            a0 = i * math.pi / 6
            a1 = (i + 1) * math.pi / 6
            x0, y0 = 32 + math.cos(a0)*24, 34 + math.sin(a0)*24
            x1, y1 = 32 + math.cos(a1)*24, 34 + math.sin(a1)*24
            glow_line(c, x0, y0, x1, y1, PURPLE, width=1, dim=0.5)
    return to_image(c)

def ach_emp_multi():
    """EMP ring expanding outward from center."""
    c = blank()
    glow_circle(c, 32, 32, 8, PURPLE, width=1, dim=0.9)
    glow_circle(c, 32, 32, 16, PURPLE, width=1, dim=0.6)
    glow_circle(c, 32, 32, 24, PURPLE, width=1, dim=0.3)
    glow_dot(c, 32, 32, 3, WHITE, dim=0.5)
    # Small stunned enemies around the edge
    for a in [0.5, 2.1, 3.8, 5.4]:
        ex, ey = 32 + math.cos(a)*20, 32 + math.sin(a)*20
        glow_dot(c, ex, ey, 1.5, RED, dim=0.5)
    return to_image(c)

def ach_speedrun():
    """Lightning bolt — speed symbol."""
    c = blank()
    # Lightning bolt shape
    pts = [(28, 8), (18, 32), (30, 30), (22, 56), (44, 26), (32, 28), (40, 8)]
    m = _mask()
    d = ImageDraw.Draw(m)
    for i in range(len(pts)-1):
        d.line([pts[i], pts[i+1]], fill=255, width=2)
    add_mask(c, np.array(m, dtype=np.float32)/255.0, GOLD, GLOW, dim=1.0)
    return to_image(c)

def ach_all_hulls():
    """5 small ship silhouettes in a row."""
    c = blank()
    # 5 ships spread horizontally
    for i in range(5):
        cx = 10 + i * 11
        glow_poly(c, _xform(DELTA, cx, 32, 0, 0.45), CYAN, width=1,
                  glow=GLOW_SOFT, dim=0.6 + i*0.08)
    return to_image(c)

def ach_all_golden():
    """5 golden ship silhouettes in a V formation."""
    c = blank()
    positions = [(32, 16), (18, 30), (46, 30), (10, 44), (54, 44)]
    for i, (px, py) in enumerate(positions):
        glow_poly(c, _xform(DELTA, px, py, 0, 0.4), GOLD, width=1,
                  glow=GLOW_SOFT, dim=0.8)
    return to_image(c)

def ach_legendary_mod():
    """Glowing diamond — legendary rarity."""
    c = blank()
    # Large diamond
    pts = [(32, 8), (52, 32), (32, 56), (12, 32)]
    glow_poly(c, pts, PURPLE, width=1, dim=1.0)
    # Inner diamond
    pts2 = [(32, 18), (42, 32), (32, 46), (22, 32)]
    glow_poly(c, pts2, PURPLE, width=1, dim=0.5)
    glow_dot(c, 32, 32, 3, WHITE, dim=0.6)
    return to_image(c)

def ach_endless_10():
    """'10' with wave lines."""
    c = blank()
    glow_text(c, "10", 24, 32, 24, CYAN, dim=1.0)
    # Wave lines below
    for i in range(3):
        y = 44 + i * 5
        dim = 0.5 - i * 0.12
        m = _mask()
        d = ImageDraw.Draw(m)
        pts = []
        for x in range(10, 55):
            yy = y + math.sin(x * 0.3 + i) * 2
            pts.append((x, int(yy)))
        d.line(pts, fill=200, width=1)
        add_mask(c, np.array(m, dtype=np.float32)/255.0, CYAN, GLOW_SOFT, dim=dim)
    return to_image(c)

def ach_endless_25():
    """'25' with wave lines."""
    c = blank()
    glow_text(c, "25", 24, 32, 24, GOLD, dim=1.0)
    for i in range(3):
        y = 44 + i * 5
        dim = 0.5 - i * 0.12
        m = _mask()
        d = ImageDraw.Draw(m)
        pts = []
        for x in range(10, 55):
            yy = y + math.sin(x * 0.3 + i) * 2
            pts.append((x, int(yy)))
        d.line(pts, fill=200, width=1)
        add_mask(c, np.array(m, dtype=np.float32)/255.0, GOLD, GLOW_SOFT, dim=dim)
    return to_image(c)

def ach_full_breach():
    """Star/crown — completionist."""
    c = blank()
    # 5-pointed star
    pts = []
    for i in range(10):
        a = i * math.pi / 5 - math.pi / 2
        r = 22 if i % 2 == 0 else 10
        pts.append((32 + math.cos(a)*r, 32 + math.sin(a)*r))
    glow_poly(c, pts, GOLD, width=1, dim=1.0)
    glow_dot(c, 32, 32, 3, WHITE, dim=0.5)
    glow_circle(c, 32, 32, 26, GOLD, dim=0.2)
    return to_image(c)


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

ACHIEVEMENTS = [
    ("ACH_FIRST_KILL",      ach_first_kill),
    ("ACH_FIRST_BOSS",      ach_first_boss),
    ("ACH_CLEAR_WORLD1",    ach_clear_world1),
    ("ACH_CLEAR_WORLD2",    ach_clear_world2),
    ("ACH_FINAL_BREACH",    ach_final_breach),
    ("ACH_GOLDEN_HULL",     ach_golden_hull),
    ("ACH_HIGH_TRACE",      ach_high_trace),
    ("ACH_KILL_MANTICORE",  ach_kill_manticore),
    ("ACH_KILL_ARCHON",     ach_kill_archon),
    ("ACH_KILL_VORTEX",     ach_kill_vortex),
    ("ACH_CHAIN_KILL",      ach_chain_kill),
    ("ACH_RICOCHET_KILL",   ach_ricochet_kill),
    ("ACH_STEALTH_KILL",    ach_stealth_kill),
    ("ACH_EMP_MULTI",       ach_emp_multi),
    ("ACH_SPEEDRUN",        ach_speedrun),
    ("ACH_ALL_HULLS",       ach_all_hulls),
    ("ACH_ALL_GOLDEN",      ach_all_golden),
    ("ACH_LEGENDARY_MOD",   ach_legendary_mod),
    ("ACH_ENDLESS_10",      ach_endless_10),
    ("ACH_ENDLESS_25",      ach_endless_25),
    ("ACH_FULL_BREACH",     ach_full_breach),
]

if __name__ == "__main__":
    os.makedirs(OUT, exist_ok=True)
    for name, builder in ACHIEVEMENTS:
        unlocked = builder()
        locked   = make_locked(unlocked)

        upath = os.path.join(OUT, f"{name}.png")
        lpath = os.path.join(OUT, f"{name}_locked.png")
        unlocked.save(upath)
        locked.save(lpath)
        print(f"  {name}  {unlocked.size[0]}×{unlocked.size[1]}")

    print(f"\n{len(ACHIEVEMENTS)} achievements ({len(ACHIEVEMENTS)*2} icons) → {OUT}/")
