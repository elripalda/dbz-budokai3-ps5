#!/usr/bin/env python3
"""The PlayStation versions of Budokai 3 HD's Xbox button glyphs.

The game draws its button prompts from one strip (data_usi, the options menu's
512x64 texture: LB LT RB RT Y B A X). This makes the same strip with the
PlayStation buttons in the same places, at 4x (2048x256: sharp at 1440p and
4K), drawn in the game's style: glossy buttons with a dark outline and a soft
shadow, PlayStation's symbols in their colours on dark buttons, L1/R1/L2/R2
on the shoulder and trigger shapes.

    python3 ps_buttons.py <original strip.png> <out.png> [--preview <file.png>]
"""
import argparse
import math

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont

K = 4                      # the pack's scale
SS = 4                     # supersampling while drawing
FONT = "/usr/share/fonts/opentype/inter/Inter-ExtraBold.otf"

# where each glyph sits in the original 512x64 strip (x0, y0, x1, y1), as the
# game and NovaPowers' screens read it
RECTS = {"LB": (4, 8, 56, 42), "LT": (74, 2, 108, 46), "RB": (126, 8, 178, 42), "RT": (194, 2, 233, 46),
         "Y": (249, 9, 284, 41), "B": (297, 9, 332, 41), "A": (346, 9, 381, 41), "X": (393, 9, 428, 41)}

TRIANGLE, CIRCLE, CROSS, SQUARE = (64, 214, 168), (255, 92, 104), (112, 168, 255), (240, 140, 222)
INK = (12, 12, 16, 255)


def canvas(w, h):
    return Image.new("RGBA", (w * SS, h * SS), (0, 0, 0, 0))


def finish(img, w, h):
    return img.resize((w, h), Image.LANCZOS)


def shadow_of(mask, off, blur, alpha):
    sh = Image.new("RGBA", mask.size, (0, 0, 0, 0))
    a = mask.point(lambda v: int(v * alpha))
    sh.putalpha(ImageChops.offset(a, off, off).filter(ImageFilter.GaussianBlur(blur)))
    return sh


def face_button(symbol, w, h):
    """A dark glossy button with the PlayStation symbol, the size of the game's Y/B/A/X."""
    img = canvas(w, h)
    W, H = img.size
    d = min(W, H) * 0.96
    cx, cy = W * 0.47, H * 0.48
    box = (cx - d / 2, cy - d / 2, cx + d / 2, cy + d / 2)
    # shadow and dark outline
    mask = Image.new("L", img.size, 0)
    ImageDraw.Draw(mask).ellipse(box, fill=255)
    img.alpha_composite(shadow_of(mask, int(W * 0.035), W * 0.025, 0.55))
    ImageDraw.Draw(img).ellipse(box, fill=INK)
    # the button: a vertical gradient, lighter at the top, with a gloss arc
    pad = d * 0.075
    inner = (box[0] + pad, box[1] + pad, box[2] - pad, box[3] - pad)
    grad = Image.new("RGBA", img.size, (0, 0, 0, 0))
    gd = ImageDraw.Draw(grad)
    for y in range(int(inner[1]), int(inner[3]) + 1):
        t = (y - inner[1]) / max(1, inner[3] - inner[1])
        c = tuple(int(a + (b - a) * t) for a, b in zip((86, 88, 98), (22, 23, 28)))
        gd.line([(inner[0], y), (inner[2], y)], fill=c + (255,))
    m = Image.new("L", img.size, 0)
    ImageDraw.Draw(m).ellipse(inner, fill=255)
    img.alpha_composite(Image.composite(grad, Image.new("RGBA", img.size, (0, 0, 0, 0)), m))
    gloss = Image.new("L", img.size, 0)
    gw = (inner[2] - inner[0]) * 0.78
    gx0 = cx - gw / 2
    ImageDraw.Draw(gloss).ellipse((gx0, inner[1] + d * 0.03, gx0 + gw, inner[1] + d * 0.42), fill=70)
    gloss = ImageChops.multiply(gloss, m).filter(ImageFilter.GaussianBlur(W * 0.01))
    img.alpha_composite(Image.composite(Image.new("RGBA", img.size, (255, 255, 255, 255)),
                                        Image.new("RGBA", img.size, (0, 0, 0, 0)), gloss))
    # the symbol: its colour, with a thin dark edge so it reads on any background
    sym = Image.new("RGBA", img.size, (0, 0, 0, 0))
    sd = ImageDraw.Draw(sym)
    r = d * 0.27
    stroke = d * 0.115
    col = {"triangle": TRIANGLE, "circle": CIRCLE, "cross": CROSS, "square": SQUARE}[symbol] + (255,)

    def draw(dd, width, colour):
        if symbol == "circle":
            dd.ellipse((cx - r, cy - r, cx + r, cy + r), outline=colour, width=int(width))
        elif symbol == "square":
            s = r * 0.86
            dd.rectangle((cx - s, cy - s, cx + s, cy + s), outline=colour, width=int(width))
        elif symbol == "triangle":
            ty = cy + r * 0.12
            pts = [(cx, ty - r * 1.05), (cx + r * 1.08, ty + r * 0.78), (cx - r * 1.08, ty + r * 0.78)]
            dd.line(pts + [pts[0], pts[1]], fill=colour, width=int(width), joint="curve")
        else:  # cross
            s = r * 0.84
            dd.line([(cx - s, cy - s), (cx + s, cy + s)], fill=colour, width=int(width))
            dd.line([(cx - s, cy + s), (cx + s, cy - s)], fill=colour, width=int(width))

    edge = Image.new("RGBA", img.size, (0, 0, 0, 0))
    draw(ImageDraw.Draw(edge), stroke + d * 0.075, (6, 6, 8, 235))
    img.alpha_composite(edge.filter(ImageFilter.GaussianBlur(W * 0.003)))
    draw(sd, stroke, col)
    img.alpha_composite(sym)
    return finish(img, w * K, h * K)


def label(img, text, box, size):
    """White letters with a dark outline, centred in box (the game's LB/RB lettering)."""
    font = ImageFont.truetype(FONT, int(size))
    d = ImageDraw.Draw(img)
    l, t, r_, b = d.textbbox((0, 0), text, font=font, stroke_width=int(size * 0.15))
    x = (box[0] + box[2]) / 2 - (r_ - l) / 2 - l
    y = (box[1] + box[3]) / 2 - (b - t) / 2 - t
    d.text((x, y), text, font=font, fill=(250, 250, 250, 255), stroke_width=int(size * 0.15), stroke_fill=INK)


def shoulder(text, w, h):
    """L1 / R1: the game's light pill with a dark edge."""
    img = canvas(w, h)
    W, H = img.size
    box = (W * 0.04, H * 0.06, W * 0.95, H * 0.9)
    rad = (box[3] - box[1]) / 2
    mask = Image.new("L", img.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle(box, rad, fill=255)
    img.alpha_composite(shadow_of(mask, int(H * 0.06), H * 0.04, 0.5))
    ImageDraw.Draw(img).rounded_rectangle(box, rad, fill=INK)
    pad = H * 0.07
    inner = (box[0] + pad, box[1] + pad, box[2] - pad, box[3] - pad)
    grad = Image.new("RGBA", img.size, (0, 0, 0, 0))
    gd = ImageDraw.Draw(grad)
    for y in range(int(inner[1]), int(inner[3]) + 1):
        t = (y - inner[1]) / max(1, inner[3] - inner[1])
        c = tuple(int(a + (b - a) * t) for a, b in zip((235, 236, 240), (150, 152, 160)))
        gd.line([(inner[0], y), (inner[2], y)], fill=c + (255,))
    m = Image.new("L", img.size, 0)
    ImageDraw.Draw(m).rounded_rectangle(inner, rad - pad, fill=255)
    img.alpha_composite(Image.composite(grad, Image.new("RGBA", img.size, (0, 0, 0, 0)), m))
    label(img, text, inner, (inner[3] - inner[1]) * 0.84)
    return finish(img, w * K, h * K)


def trigger(text, w, h):
    """L2 / R2: the game's tall trigger shape (wider at the top and bottom)."""
    img = canvas(w, h)
    W, H = img.size
    x0, x1, y0, y1 = W * 0.08, W * 0.9, H * 0.04, H * 0.93
    neck = (x1 - x0) * 0.14
    pts = [(x0, y0), (x1, y0), (x1 - neck, y0 + (y1 - y0) * 0.28), (x1 - neck, y1 - (y1 - y0) * 0.28),
           (x1, y1), (x0, y1), (x0 + neck, y1 - (y1 - y0) * 0.28), (x0 + neck, y0 + (y1 - y0) * 0.28)]
    mask = Image.new("L", img.size, 0)
    ImageDraw.Draw(mask).polygon(pts, fill=255)
    mask = mask.filter(ImageFilter.GaussianBlur(W * 0.02)).point(lambda v: 255 if v > 128 else 0)
    img.alpha_composite(shadow_of(mask, int(H * 0.04), H * 0.03, 0.5))
    edge = Image.new("RGBA", img.size, INK)
    img.alpha_composite(Image.composite(edge, Image.new("RGBA", img.size, (0, 0, 0, 0)), mask))
    inner = mask.filter(ImageFilter.MinFilter(int(W * 0.09) | 1))
    grad = Image.new("RGBA", img.size, (0, 0, 0, 0))
    gd = ImageDraw.Draw(grad)
    for y in range(H):
        t = y / H
        c = tuple(int(a + (b - a) * t) for a, b in zip((240, 241, 245), (150, 152, 160)))
        gd.line([(0, y), (W, y)], fill=c + (255,))
    img.alpha_composite(Image.composite(grad, Image.new("RGBA", img.size, (0, 0, 0, 0)), inner))
    label(img, text, (x0, y0 + (y1 - y0) * 0.2, x1, y1 - (y1 - y0) * 0.2), (x1 - x0) * 0.66)
    return finish(img, w * K, h * K)


GLYPHS = {"LB": lambda w, h: shoulder("L1", w, h), "RB": lambda w, h: shoulder("R1", w, h),
          "LT": lambda w, h: trigger("L2", w, h), "RT": lambda w, h: trigger("R2", w, h),
          "Y": lambda w, h: face_button("triangle", w, h), "B": lambda w, h: face_button("circle", w, h),
          "A": lambda w, h: face_button("cross", w, h), "X": lambda w, h: face_button("square", w, h)}


def make_strip(original):
    src = Image.open(original).convert("RGBA")
    out = Image.new("RGBA", (src.width * K, src.height * K), (0, 0, 0, 0))
    for name, (x0, y0, x1, y1) in RECTS.items():
        g = GLYPHS[name](x1 - x0, y1 - y0)
        out.alpha_composite(g, (x0 * K, y0 * K))
    return out


if __name__ == "__main__":
    ap = argparse.ArgumentParser()
    ap.add_argument("original")
    ap.add_argument("out")
    ap.add_argument("--preview")
    a = ap.parse_args()
    strip = make_strip(a.original)
    strip.save(a.out)
    if a.preview:
        src = Image.open(a.original).convert("RGBA").resize(strip.size, Image.LANCZOS)
        sheet = Image.new("RGBA", (strip.width, strip.height * 2 + 20), (52, 54, 64, 255))
        sheet.alpha_composite(src, (0, 0))
        sheet.alpha_composite(strip, (0, strip.height + 20))
        sheet.crop((0, 0, 1760, sheet.height)).save(a.preview)
