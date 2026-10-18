#!/usr/bin/env python3
"""The DualSense picture for Budokai 3 HD's controller screen.

The options menu shows an Xbox 360 pad (355BB8CF9E381EA7, 512x256: lime lines
on a dark green body, at 2-344 x 0-236, the rest clear). This puts the
DualSense art (art/local/buttons/dualsense.png, @elripalda's, already in that
style) in the same box at 4x (2048x1024): scaled to fit, centred in the box.

    python3 ps_controller.py <dualsense.png> <out.png>
"""
import argparse

import numpy as np
from PIL import Image

K = 4
W, H = 512, 256
BOX = (2, 0, 344, 236)          # the 360 pad's extent in the texture


def premultiplied_resize(img, size):
    a = np.asarray(img.convert("RGBA")).astype(np.float32) / 255
    pm = np.dstack([a[..., :3] * a[..., 3:], a[..., 3:]])
    chans = [np.asarray(Image.fromarray(pm[..., i]).resize(size, Image.LANCZOS)) for i in range(4)]
    big = np.dstack(chans)
    al = np.clip(big[..., 3:], 0, 1)
    rgb = np.where(al > 1e-3, big[..., :3] / np.maximum(al, 1e-3), 0)
    return Image.fromarray((np.dstack([np.clip(rgb, 0, 1), al]) * 255).round().astype(np.uint8), "RGBA")


def fit(art_path):
    art = Image.open(art_path).convert("RGBA")
    a = np.asarray(art)[..., 3]
    ys, xs = np.nonzero(a > 8)
    art = art.crop((xs.min(), ys.min(), xs.max() + 1, ys.max() + 1))
    bw, bh = (BOX[2] - BOX[0]) * K, (BOX[3] - BOX[1]) * K
    s = min(bw / art.width, bh / art.height)
    size = (round(art.width * s), round(art.height * s))
    pic = premultiplied_resize(art, size)
    out = Image.new("RGBA", (W * K, H * K), (0, 0, 0, 0))
    out.alpha_composite(pic, (BOX[0] * K + (bw - size[0]) // 2, BOX[1] * K + (bh - size[1]) // 2))
    return out


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("art")
    ap.add_argument("out")
    a = ap.parse_args()
    fit(a.art).save(a.out)
