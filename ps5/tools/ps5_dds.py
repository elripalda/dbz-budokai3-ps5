#!/usr/bin/env python3
"""PNG/JPG -> the PS5 home screen's DDS (3840x2160, BC7, as pic0.dds / pic1.dds).

    python3 ps5/tools/ps5_dds.py <picture> <out.dds>

The picture is scaled to cover 3840x2160 (centred). Needs Pillow and etcpak.
"""
import struct
import sys

import etcpak
from PIL import Image

W, H = 3840, 2160


def write_dds(img, out):
    img = img.convert("RGBA")
    scale = max(W / img.width, H / img.height)
    img = img.resize((round(img.width * scale), round(img.height * scale)), Image.LANCZOS)
    x, y = (img.width - W) // 2, (img.height - H) // 2
    img = img.crop((x, y, x + W, y + H))
    data = etcpak.compress_bc7(img.tobytes(), W, H, None)
    header = struct.pack(
        "<4s31I", b"DDS ", 124, 0x81007, H, W, W * H, 0, 0, *([0] * 11),
        32, 0x4, struct.unpack("<I", b"DX10")[0], 0, 0, 0, 0, 0,
        0x1000, 0, 0, 0, 0)
    dx10 = struct.pack("<5I", 98, 3, 0, 1, 0)  # BC7_UNORM, a 2D texture
    with open(out, "wb") as f:
        f.write(header + dx10 + data)


if __name__ == "__main__":
    write_dds(Image.open(sys.argv[1]), sys.argv[2])
