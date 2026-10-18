#!/usr/bin/env python3
"""The PlayStation versions of the game's two other Xbox button icons, at 4x.
Only the buttons change; the game's words stay as they are.

- the START button (options menu, 64x64): the same silver pill with the
  OPTIONS button's three lines instead of the arrow;
- the BACK button above "BACK : Help" (main menu, 256x64): the touch pad,
  which is BACK on the PS5, as a layer the app puts over the game's own
  texture when it starts (its words stay the game's, and no game pixels
  ship with the app).

Each is drawn in its texture's own coordinates (the same boxes as the
originals, measured), with ps_buttons.py's drawing helpers.

    python3 ps_prompts.py <pack dir> <touch_pad.png>
"""
import argparse
import os

import cv2
import numpy as np
from PIL import Image
from shapely.geometry import Polygon, box
from shapely.ops import unary_union

import ps_buttons as pb
from ps_buttons import K, U, Window


def soft_shadow(win, m, dx, dy, blur, alpha):
    sh = cv2.GaussianBlur(win.shifted(m, dx, dy), (0, 0), blur * U)
    win.over((0, 0, 0), np.clip(sh * alpha, 0, 1))


# ------------------------------------------------------------- silver pill

def silver(win, shape, outline=1.7):
    """The game's small silver buttons (START, BACK): a black edge, a white
    face shading to grey at its rim and bottom, a soft shadow."""
    m = win.mask(shape)
    soft_shadow(win, win.grown(m, outline), 0.6, 0.9, 0.7, 0.45)
    win.over(pb.INK, win.grown(m, outline))
    din = win.distance_inside(m)
    fy0, fy1 = shape.bounds[1], shape.bounds[3]
    col = win.vgrad((252, 252, 252), (214, 214, 218), fy0 + (fy1 - fy0) * 0.35, fy1)
    rim = np.clip(1 - din / 3.4, 0, 1) ** 1.25
    # the rim darker at the bottom right, lighter at the top left
    tl = np.clip(((win.X - shape.centroid.x) + (win.Y - shape.centroid.y) * 1.6) / 14 + 0.5, 0, 1)
    rim_col = np.asarray((236, 236, 238), np.float32) * (1 - tl[..., None]) + np.asarray((92, 92, 98), np.float32) * tl[..., None]
    col = col * (1 - rim[..., None]) + rim_col * rim[..., None]
    win.over(col, m)


def engraved(win, shape):
    """An icon pressed into a silver button: grey, darker at its top edge,
    lighter at its bottom one."""
    m = win.mask(shape)
    win.over((96, 96, 100), win.grown(m, 0.5))
    win.over((150, 150, 156), m)
    win.over((198, 198, 204), m * (1 - win.shifted(m, -0.7, -0.7)))   # its lit lower right edge


def start_button(out_dir):
    """E56FC5E5DEFE917B (64x64): the pill at 10-47 x 4-32, now OPTIONS."""
    win = Window(0, 0, 64, 64)
    pill = box(12.0, 6.0, 45.0, 29.5).buffer(-11.6).buffer(11.6, 64)
    silver(win, pill)
    cx, cy = 28.5, 17.6
    bars = unary_union([box(cx - 7.4, cy + dy - 1.25, cx + 7.4, cy + dy + 1.25).buffer(-0.6).buffer(0.6, 16)
                        for dy in (-4.6, 0.0, 4.6)])
    engraved(win, bars)
    win.image().save(os.path.join(out_dir, "E56FC5E5DEFE917B_64x64_RGBA8.png"))
    return win


# ------------------------------------------------------------- BACK : Help

def touch_pad_layer(path):
    """9607A868D438558E (256x64): the touch pad where its button was (82-122 x
    1-24), alone; the app puts it over the game's texture (its words BACK and
    ': Help' stay the game's) when it starts (ps5/src/ps_button_pack.cpp)."""
    win = Window(70, 0, 134, 26)
    # wide, a little wider at the top, as on the pad
    pad = Polygon([(84.0, 3.4), (119.0, 3.4), (116.6, 20.6), (86.4, 20.6)]).buffer(-3.4, join_style=1)
    pad = pad.buffer(3.4, 48, join_style=1)
    silver(win, pad, outline=1.6)
    layer = Image.new("RGBA", (256 * K, 64 * K), (0, 0, 0, 0))
    layer.alpha_composite(win.image(), (win.x0 * K, win.y0 * K))
    layer.save(path)
    return layer


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("pack_dir")
    ap.add_argument("touch_pad_png")
    a = ap.parse_args()
    os.makedirs(a.pack_dir, exist_ok=True)
    start_button(a.pack_dir)
    touch_pad_layer(a.touch_pad_png)
