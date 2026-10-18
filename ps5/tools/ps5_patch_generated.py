#!/usr/bin/env python3
"""[ps5 port] Make the recompiled code (generated/) right for the PS5.

The PS5's 16 KB pages: like Windows and arm64 macOS, guest addresses from
0xE0000000 sit 0x1000 higher in host memory (rex::memory::detail::
PhysicalHostOffset). The codegen's header leaves the PS5 out of that test.
Run it after every codegen (it's idempotent):

    python3 ps5/tools/ps5_patch_generated.py [generated]
"""
import pathlib
import sys

gen = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "generated")
old = "#if REX_PLATFORM_WIN32 || (REX_PLATFORM_MAC && REX_ARCH_ARM64)\n"
new = "#if REX_PLATFORM_WIN32 || (REX_PLATFORM_MAC && REX_ARCH_ARM64) || REX_PLATFORM_PS5\n"
patched = 0
for header in sorted(gen.glob("*.h")):
    text = header.read_text()
    if old in text:
        header.write_text(text.replace(old, new))
        print(f"{header}: 0xE0000000 host offset on the PS5")
        patched += 1
    elif new in text:
        print(f"{header}: already patched")
        patched += 1
if not patched:
    sys.exit(f"{gen}: no header with the host offset test (has the codegen template changed?)")
