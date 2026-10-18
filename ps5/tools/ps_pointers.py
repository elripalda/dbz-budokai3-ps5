#!/usr/bin/env python3
"""The controller screen's pointer lines, for the DualSense picture.

The lines from each label to its button (5D9FF23ACEA4B1FE, 512x256: one sprite
per line, each drawn at a fixed place by its label) were drawn for the Xbox
pad. Here they're drawn again at 4x to end on the DualSense's buttons:

- L2, L1, R2 and R1 keep their angle and stop on their shoulder button;
- Triangle runs straight into its button;
- Circle, Cross and Square sit higher on the DualSense than their labels, past
  where their sprites reach, so their sprite keeps the line by the label and
  the rest is drawn in the controller picture (355BB8CF9E381EA7), which covers
  that part of the screen. The two overlap along the horizontal stretch.

The other pieces in the texture (unused on this screen) and the label bar are
drawn as they were. Positions come from a capture of the screen (where each
sprite lands and how big). Nothing is copied from the game.

    python3 ps_pointers.py <pack dir>     (after ps_controller.py)
"""
import argparse
import os

import numpy as np
from PIL import Image

K = 4
LIME = np.array([191, 255, 64], np.float32) / 255
ATLAS = "5D9FF23ACEA4B1FE_512x256_RGBA8.png"
PICTURE = "355BB8CF9E381EA7_512x256_RGBA8.png"

WIDTH = 2.6           # line width, in the lines texture's texels
SCREEN_RATIO = 2.066 / 2.1335   # a lines texel, in controller texels (as drawn on screen)

# The lines texture, in its texels (pixel centres at .5). Each piece: the
# horizontal by the label, from its end under the label to the corner, then
# the diagonal to the button.
PIECES = {
    # used on the controller screen
    "L2": [(53.7, 106.4), (34.44, 106.4), (9.91, 191.12)],
    "L1": [(113.7, 103.4), (91.52, 103.4), (73.63, 142.33)],
    "R2": [(450.7, 118.4), (389.09, 118.4), (344.96, 201.59)],
    "R1": [(232.7, 162.0), (168.89, 162.0), (138.25, 202.28)],
    "Triangle": [(470.7, 13.4), (384.65, 13.4)],
    "Circle": [(77.7, 82.4)],          # (to where the picture takes over: see below)
    "Cross": [(186.7, 84.0)],
    "Square": [(323.7, 151.5)],
    # the rest of the texture, as it was
    "spare1": [(114.7, 4.4), (95.2, 4.4), (66.78, 38.5)],
    "spare2": [(235.7, 4.4), (157.77, 4.4), (126.36, 64.5)],
    "spare3": [(55.7, 7.4), (35.37, 7.4), (10.33, 65.5)],
    "spare5": [(352.7, 16.4), (289.03, 16.4), (246.03, 50.5)],
}
BAR = (343.6, 66.6, 458.4, 100.8, 9.5)   # the label bar: x0, y0, x1, y1, corner radius

# Where the game draws the two textures, from a capture of the screen
# (2559x1439): the controller picture's texel (0, 0) and its texel size, and
# the lines texture's texel size and, for the three lines the picture carries
# on, where its texel (0, 0) lands for that line's sprite.
PICTURE_AT = (1036.57, 554.02, 2.1351, 2.1292)
LINES_SCALE = 2.066
SPRITE_AT = {"Circle": (1672.2, 607.61), "Cross": (1456.0, 710.22), "Square": (1162.49, 673.08)}
OVERLAP = 12          # screen pixels where both draw the horizontal

# The rest of the Circle, Cross and Square lines, in the picture's texels: the
# row of the sprite's horizontal, the corner, and the button (centre; the line
# points at it, or at `aim`, and stops at its ring). Square's goes up between
# the right stick and Cross.
RING = 12.5
CONTINUED = {
    "Circle": dict(row=104.88, corner=325.0, button=(299.0, 73.25)),
    "Cross": dict(row=154.62, corner=329.7, button=(276.25, 97.5)),
    "Square": dict(row=202.67, corner=263.0, button=(253.0, 73.25), aim=(261.0, 100.0)),
}
RIGHT_EDGE = 342.7    # how far right the picture's part goes (it ends at 344)


def stroke(alpha, pts, width, k=K):
    """Adds a round-capped polyline (texel coordinates) to `alpha` (k x size)."""
    h, w = alpha.shape
    r = width * k / 2
    P = [(x * k, y * k) for x, y in pts]
    for (x0, y0), (x1, y1) in zip(P, P[1:]):
        bx0, bx1 = int(max(0, min(x0, x1) - r - 2)), int(min(w, max(x0, x1) + r + 3))
        by0, by1 = int(max(0, min(y0, y1) - r - 2)), int(min(h, max(y0, y1) + r + 3))
        ys, xs = np.mgrid[by0:by1, bx0:bx1].astype(np.float32) + 0.5
        dx, dy = x1 - x0, y1 - y0
        ll = dx * dx + dy * dy
        t = np.clip(((xs - x0) * dx + (ys - y0) * dy) / ll, 0, 1) if ll else 0
        d = np.hypot(xs - (x0 + t * dx), ys - (y0 + t * dy))
        alpha[by0:by1, bx0:bx1] = np.maximum(alpha[by0:by1, bx0:bx1], np.clip(r - d + 0.5, 0, 1))


def bar(w, h):
    """The label bar (a soft green slab behind each label), at 4x: alpha 200,
    darker along the top and the right, lighter along the left and the bottom."""
    x0, y0, x1, y1, rad = [v * K for v in BAR]
    ys, xs = np.mgrid[0:h, 0:w].astype(np.float32) + 0.5
    qx = np.maximum(np.maximum(x0 + rad - xs, xs - (x1 - rad)), 0)
    qy = np.maximum(np.maximum(y0 + rad - ys, ys - (y1 - rad)), 0)
    inside = np.hypot(qx, qy) - rad          # <= 0 inside the rounded rect
    a = np.clip(0.5 - inside, 0, 1) * (200 / 255)
    base = np.array([165, 212, 100], np.float32)
    left = np.clip((xs - x0) / K, 0, None)
    right = np.clip((x1 - xs) / K, 0, None)
    top = np.clip((ys - y0) / K, 0, None)
    bottom = np.clip((y1 - ys) / K, 0, None)
    shade = (0.38 + 0.62 * np.clip(top / 6.2, 0, 1) ** 0.85) * (0.24 + 0.76 * np.clip(right / 6.0, 0, 1) ** 0.8)
    col = base * shade[..., None]
    col += np.array([27, 13, 47], np.float32) * (np.clip(1 - left / 5.5, 0, 1) ** 1.3)[..., None]
    col += np.array([12, 6, 21], np.float32) * (np.clip(1 - bottom / 5.0, 0, 1) ** 1.2)[..., None]
    return np.clip(col / 255, 0, 1), a


def sprite_start(name):
    """Where a carried-on line's sprite starts its horizontal (lines texels): a
    little right of the picture's corner, so the two overlap."""
    c = CONTINUED[name]
    x = PICTURE_AT[0] + c["corner"] * PICTURE_AT[2] + OVERLAP
    return (x - 0.5 - SPRITE_AT[name][0]) / LINES_SCALE + 0.5


def atlas():
    w, h = 512 * K, 256 * K
    lines = np.zeros((h, w), np.float32)
    for name, pts in PIECES.items():
        if name in CONTINUED:
            pts = pts + [(sprite_start(name), pts[0][1])]
        stroke(lines, pts, WIDTH)
    bar_rgb, bar_a = bar(w, h)
    rgb = np.where(lines[..., None] > 0, LIME, bar_rgb)
    rgb = np.where((lines[..., None] == 0) & (bar_a[..., None] == 0), LIME, rgb)
    a = np.maximum(lines, bar_a)
    return Image.fromarray((np.dstack([rgb, a]) * 255).round().astype(np.uint8), "RGBA")


def continuation(name, c):
    """The picture's part of a line: from the button's ring to the corner, then
    along the row to the picture's edge (under the sprite's horizontal)."""
    cx, cy = c["button"]
    kx, ky = c["corner"], c["row"]
    ax, ay = c.get("aim", c["button"])
    dx, dy = ax - kx, ay - ky
    n = np.hypot(dx, dy)
    dx, dy = dx / n, dy / n
    # the first point on the ray from the corner that touches the ring
    r = RING + WIDTH * SCREEN_RATIO / 2 - 0.3
    fx, fy = kx - cx, ky - cy
    b = fx * dx + fy * dy
    t = -b - np.sqrt(b * b - (fx * fx + fy * fy - r * r))
    return [(kx + t * dx, ky + t * dy), (kx, ky), (RIGHT_EDGE, ky)]


def draw_on_picture(path):
    pic = np.asarray(Image.open(path).convert("RGBA")).astype(np.float32) / 255
    a = np.zeros(pic.shape[:2], np.float32)
    for name, c in CONTINUED.items():
        stroke(a, continuation(name, c), WIDTH * SCREEN_RATIO)
    out_a = a + pic[..., 3] * (1 - a)
    rgb = (LIME * a[..., None] + pic[..., :3] * pic[..., 3:] * (1 - a[..., None])) / np.maximum(out_a[..., None], 1e-4)
    rgb = np.where(out_a[..., None] > 1e-4, rgb, pic[..., :3])
    Image.fromarray((np.dstack([np.clip(rgb, 0, 1), out_a]) * 255).round().astype(np.uint8), "RGBA").save(path)


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("pack_dir")
    args = ap.parse_args()
    atlas().save(os.path.join(args.pack_dir, ATLAS))
    draw_on_picture(os.path.join(args.pack_dir, PICTURE))
