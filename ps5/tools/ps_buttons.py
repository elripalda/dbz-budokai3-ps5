#!/usr/bin/env python3
"""The PlayStation versions of Budokai 3 HD's Xbox button glyphs.

The game draws its button prompts from one strip (data_usi, the options menu's
512x64 texture: LB LT RB RT Y B A X). This draws the same strip with the
PlayStation buttons, at 4x (2048x256, sharp at 1440p and 4K), built the way
the originals are (measured from the strip):

- The face buttons are the same 35x32 balls, lit from the upper left (a bright
  rim on that side, darker to the lower right), in a dark PlayStation grey.
  The symbols take the place of the letters, at the letters' size and weight:
  coloured, with the same black outline and a darker edge on the lower right.
- L1 / R1 and L2 / R2 are the same white blocks as LB / RB and LT / RT: a face
  pushed out 4 pixels to the lower right into a grey side, with a dark outline,
  and the lettering in the same style (white strokes, a heavy black outline,
  a grey edge on the lower right) in a rounded face like the game's.

Everything is drawn in the strip's own coordinates at 16x and scaled down, so
each glyph sits exactly where the game (and NovaPowers' screens) read it.

    python3 ps_buttons.py <out.png> [--preview <file.png> --original <the game's strip.png>]
"""
import argparse
import os

import cv2
import numpy as np
from PIL import Image, ImageDraw, ImageFont
from shapely.affinity import rotate, scale, translate
from shapely.geometry import Point, Polygon, box
from shapely.ops import unary_union

K = 4          # the pack's scale
SS = 4         # supersampling while drawing
U = K * SS     # drawing pixels per strip pixel
FONT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "fonts", "MPLUSRounded1c-Bold.ttf")

INK = (12, 12, 14)                      # outlines
LETTER_TOP, LETTER_BOTTOM, LETTER_EDGE = (246, 246, 246), (228, 228, 230), (168, 168, 172)
SYMBOL_COLORS = {"triangle": (40, 214, 168), "circle": (255, 78, 92),
                 "cross": (100, 160, 255), "square": (246, 128, 216)}

# the measured glyphs, in strip pixels
BALLS = {"Y": ("triangle", 266.5), "B": ("circle", 314.5), "A": ("cross", 363.5), "X": ("square", 410.5)}
BALL_CY, BALL_RX, BALL_RY = 25.0, 17.3, 15.8
EXTRUDE = (4.0, 3.5)                    # the blocks' grey side (to the lower right)
OUTLINE = 1.25


# --------------------------------------------------------------- drawing

class Window:
    """One glyph's area of the strip, drawn at U pixels per strip pixel
    (premultiplied colour, so layers composite and scale down correctly)."""

    def __init__(self, x0, y0, x1, y1):
        self.x0, self.y0, self.x1, self.y1 = x0, y0, x1, y1
        self.w, self.h = (x1 - x0) * U, (y1 - y0) * U
        self.P = np.zeros((self.h, self.w, 3), np.float32)
        self.A = np.zeros((self.h, self.w), np.float32)
        ys, xs = np.mgrid[0:self.h, 0:self.w].astype(np.float32)
        self.X, self.Y = x0 + (xs + 0.5) / U, y0 + (ys + 0.5) / U   # strip coordinates

    def mask(self, geom):
        """A shapely shape's coverage (0..1), anti-aliased."""
        out = np.zeros((self.h, self.w), np.uint8)
        for g in getattr(geom, "geoms", [geom]):
            if g.is_empty or g.geom_type != "Polygon":
                continue
            rings = [g.exterior] + list(g.interiors)
            pts = [np.round((np.asarray(r.coords) - (self.x0, self.y0)) * U * 16).astype(np.int32) for r in rings]
            m = np.zeros_like(out)
            cv2.fillPoly(m, pts, 255, cv2.LINE_AA, shift=4)
            out = np.maximum(out, m)
        return out.astype(np.float32) / 255.0

    def over(self, colour, m):
        c = np.broadcast_to(np.asarray(colour, np.float32), self.P.shape)
        self.P = c * m[..., None] + self.P * (1 - m[..., None])
        self.A = m + self.A * (1 - m)

    def distance_from(self, m):
        """Distance (strip pixels) from the shape m, 0 inside it."""
        return cv2.distanceTransform((m < 0.5).astype(np.uint8), cv2.DIST_L2, 5) / U

    def distance_inside(self, m):
        """Distance (strip pixels) to the edge of m, from inside, 0 outside."""
        return cv2.distanceTransform((m >= 0.5).astype(np.uint8), cv2.DIST_L2, 5) / U

    def grown(self, m, r):
        """m grown by r strip pixels, with a soft edge."""
        return np.clip((r - self.distance_from(m)) * U + 0.5, 0, 1) if r > 0 else m

    def shifted(self, m, dx, dy):
        M = np.float32([[1, 0, dx * U], [0, 1, dy * U]])
        return cv2.warpAffine(m, M, (self.w, self.h), flags=cv2.INTER_LINEAR, borderValue=0)

    def vgrad(self, top, bottom, y0, y1):
        t = np.clip((self.Y - y0) / (y1 - y0), 0, 1)[..., None]
        return np.asarray(top, np.float32) * (1 - t) + np.asarray(bottom, np.float32) * t

    def image(self):
        P = cv2.resize(self.P, (self.w // SS, self.h // SS), interpolation=cv2.INTER_AREA)
        A = cv2.resize(self.A, (self.w // SS, self.h // SS), interpolation=cv2.INTER_AREA)
        rgb = np.where(A[..., None] > 1e-4, P / np.maximum(A[..., None], 1e-4), 0)
        return Image.fromarray(np.dstack([np.clip(rgb, 0, 255), np.clip(A * 255, 0, 255)]).round().astype(np.uint8),
                               "RGBA")


def styled(win, m, top, bottom, edge, outline):
    """A raised mark in the game's lettering style: a black outline around it,
    its fill lighter at the top, and a darker edge along its lower right."""
    win.over(INK, win.grown(m, outline))
    ys = np.nonzero(m.max(1) > 0.5)[0]
    y0, y1 = win.y0 + ys[0] / U, win.y0 + ys[-1] / U
    win.over(win.vgrad(top, bottom, y0, y1), m)
    lip = m * (1 - win.shifted(m, -0.85, -0.85))
    win.over(edge, lip)


# --------------------------------------------------------------- lettering

def text_mask(win, text, centre, cap=20.0, gap=0.35):
    """The text's coverage, its capitals `cap` strip pixels tall, the letters'
    ink `gap` pixels apart, centred on `centre`."""
    font = ImageFont.truetype(FONT, round(cap / 0.73 * U))
    inks = []
    for ch in text:
        im = Image.new("L", (font.size * 2, font.size * 2), 0)
        ImageDraw.Draw(im).text((font.size // 2, font.size // 4), ch, font=font, fill=255)
        a = np.asarray(im)
        ys, xs = np.nonzero(a > 0)
        inks.append(a[ys.min():ys.max() + 1, xs.min():xs.max() + 1])
    width = sum(i.shape[1] for i in inks) + gap * U * (len(inks) - 1)
    height = max(i.shape[0] for i in inks)
    out = np.zeros((win.h, win.w), np.float32)
    x = (centre[0] - win.x0) * U - width / 2
    top = (centre[1] - win.y0) * U - height / 2
    for ink in inks:
        y = int(round(top + height - ink.shape[0]))      # (on a common baseline)
        xi = int(round(x))
        out[y:y + ink.shape[0], xi:xi + ink.shape[1]] = np.maximum(out[y:y + ink.shape[0], xi:xi + ink.shape[1]],
                                                                    ink / 255.0)
        x += ink.shape[1] + gap * U
    return out


# --------------------------------------------------------------- blocks

def block(win, face):
    """The game's white button block: the face, its grey side pushed out to the
    lower right, a dark outline round both."""
    steps = 32
    solid = unary_union([translate(face, EXTRUDE[0] * i / steps, EXTRUDE[1] * i / steps) for i in range(steps + 1)])
    win.over(INK, win.mask(solid.buffer(OUTLINE, 48)))
    mf = win.mask(face)
    d = win.distance_from(mf)
    side = np.interp(d, [0, 0.6, 2.0, 4.6], [176, 122, 76, 42])[..., None] * np.ones(3, np.float32)
    win.over(side, win.mask(solid))
    # the face: white, a little greyer at the bottom, with a thin light-grey rim
    fy0, fy1 = face.bounds[1], face.bounds[3]
    col = win.vgrad((255, 255, 255), (230, 230, 232), fy0 + (fy1 - fy0) * 0.45, fy1)
    rim = np.clip(1.15 - win.distance_inside(mf), 0, 1)[..., None]
    col = col * (1 - rim) + np.asarray((198, 198, 202), np.float32) * rim
    win.over(col, mf)


def bumper_face(x0, x1, y0, y1, round_left):
    """The bumpers' face: one end round, the other with small corners."""
    r = (y1 - y0) / 2
    cy = (y0 + y1) / 2
    corner = 4.0
    if round_left:
        end = Point(x0 + r, cy).buffer(r, 128)
        rest = box(x0 + r, y0, x1, y1).buffer(-corner).buffer(corner, 32)
        bridge = box(x0 + r, y0, x1 - corner, y1)
    else:
        end = Point(x1 - r, cy).buffer(r, 128)
        rest = box(x0, y0, x1 - r, y1).buffer(-corner).buffer(corner, 32)
        bridge = box(x0 + corner, y0, x1 - r, y1)
    return unary_union([end, rest, bridge])


def trigger_face(x0, x1, y0, y1):
    """The triggers' face: a tall tab whose sides curve in a little between its
    top and bottom (a spool, as in the strip), with rounded corners."""
    waist, top_cap, bottom_cap = 1.1, 7.0, 4.5
    ys = np.linspace(y0, y1, 97)

    def inset(y):
        a, b = y0 + top_cap, y1 - bottom_cap
        if y <= a or y >= b:
            return 0.0
        return waist * np.sin(np.pi * (y - a) / (b - a)) ** 0.6

    left = [(x0 + inset(y), y) for y in ys]
    right = [(x1 - inset(y), y) for y in ys[::-1]]
    corner = 3.2
    return Polygon(left + right).buffer(-corner, join_style=1).buffer(corner, 32, join_style=1)


def shoulder(name):
    if name == "L1":
        win, face, centre = Window(0, 4, 60, 46), bumper_face(6.5, 49.5, 9.5, 35.5, True), (28.0, 24.0)
    else:
        win, face, centre = Window(122, 4, 182, 46), bumper_face(127.5, 170.5, 9.5, 35.5, False), (149.0, 24.0)
    block(win, face)
    styled(win, text_mask(win, name, centre, cap=19.6, gap=0.6), LETTER_TOP, LETTER_BOTTOM, LETTER_EDGE, 1.75)
    return win


def trigger(name):
    if name == "L2":
        win, face, centre = Window(70, 0, 112, 48), trigger_face(76.5, 97.5, 4.0, 40.0), (90.0, 24.0)
    else:
        win, face, centre = Window(190, 0, 236, 48), trigger_face(199.5, 220.5, 4.0, 40.0), (212.5, 24.0)
    block(win, face)
    styled(win, text_mask(win, name, centre, cap=20.2, gap=-0.2), LETTER_TOP, LETTER_BOTTOM, LETTER_EDGE, 1.75)
    return win


# --------------------------------------------------------------- face buttons

def symbol_shape(kind, cx, cy):
    """The PlayStation symbol (outline style, as on the PS2 pad), in strip pixels."""
    stroke = 3.3
    if kind == "circle":
        return Point(cx, cy).buffer(9.6, 128).difference(Point(cx, cy).buffer(9.6 - stroke, 128))
    if kind == "square":
        h = 8.1
        outer = box(cx - h, cy - h, cx + h, cy + h).buffer(-0.9).buffer(0.9, 32)
        return outer.difference(box(cx - h + stroke, cy - h + stroke, cx + h - stroke, cy + h - stroke))
    if kind == "triangle":
        r, oy = 12.5, cy + 1.6               # (a triangle's weight sits low: lowered to look centred)
        pts = [(cx, oy - r), (cx + r * 0.8660, oy + r * 0.5), (cx - r * 0.8660, oy + r * 0.5)]
        outer = Polygon(pts).buffer(-1.0, join_style=1).buffer(1.0, 32, join_style=1)
        inner = Polygon(pts).buffer(-(stroke - 0.15), join_style=2)
        return outer.difference(inner)
    bar = box(cx - 10.6, cy - stroke / 2 - 0.05, cx + 10.6, cy + stroke / 2 + 0.05)   # the cross
    return unary_union([rotate(bar, 45, (cx, cy)), rotate(bar, -45, (cx, cy))])


def face_button(name):
    kind, cx = BALLS[name]
    cy = BALL_CY
    win = Window(int(cx - BALL_RX) - 1, 7, int(cx + BALL_RX) + 2, 43)
    ellipse = scale(Point(0, 0).buffer(1, 256), BALL_RX, BALL_RY, origin=(0, 0))
    m = win.mask(translate(ellipse, cx, cy))
    # the ball's light, as on the game's: from the upper left
    u, v = (win.X - cx) / BALL_RX, (win.Y - cy) / BALL_RY
    r = np.sqrt(u * u + v * v)
    nz = np.sqrt(np.clip(1 - r * r, 0, 1))
    L = np.array([-0.62, -0.52, 0.59]); L /= np.linalg.norm(L)
    diffuse = np.clip(u * L[0] + v * L[1] + nz * L[2], 0, 1)
    dark, mid, light = (np.array(c, np.float32) for c in ((10, 10, 12), (76, 78, 88), (196, 200, 214)))
    col = dark + (mid - dark) * (diffuse ** 1.25)[..., None]
    rim = (np.clip((r - 0.6) / 0.38, 0, 1) ** 1.4) * np.clip(-(u * 0.78 + v * 0.62) / np.maximum(r, 1e-3), 0, 1) ** 1.5
    col = col + (light - col) * (rim * 0.9)[..., None]
    H = L + np.array([0, 0, 1.0]); H /= np.linalg.norm(H)
    spec = np.clip(u * H[0] + v * H[1] + nz * H[2], 0, 1) ** 36 * 0.22
    col = np.clip(col + 255 * spec[..., None], 0, 255)
    win.over(col, m)
    # the symbol
    c = np.asarray(SYMBOL_COLORS[kind], np.float32)
    top = c + (255 - c) * 0.32
    styled(win, win.mask(symbol_shape(kind, cx, cy)), top, c, c * 0.62, 1.3)
    return win


# --------------------------------------------------------------- strip

def make_strip():
    out = Image.new("RGBA", (512 * K, 64 * K), (0, 0, 0, 0))
    wins = [shoulder("L1"), shoulder("R1"), trigger("L2"), trigger("R2")] + [face_button(n) for n in BALLS]
    for win in wins:
        out.alpha_composite(win.image(), (win.x0 * K, win.y0 * K))
    return out


def preview(original, strip, path):
    """The original and the new strip, one above the other, on the menus' dark blue."""
    src = Image.open(original).convert("RGBA").resize(strip.size, Image.LANCZOS)
    sheet = Image.new("RGBA", (strip.width, strip.height * 2 + 24), (40, 46, 74, 255))
    sheet.alpha_composite(src, (0, 0))
    sheet.alpha_composite(strip, (0, strip.height + 24))
    sheet.crop((0, 0, 1736, sheet.height)).save(path)


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("out")
    ap.add_argument("--preview", help="a side-by-side with the game's strip (--original)")
    ap.add_argument("--original")
    a = ap.parse_args()
    strip = make_strip()
    strip.save(a.out)
    if a.preview and a.original:
        preview(a.original, strip, a.preview)
