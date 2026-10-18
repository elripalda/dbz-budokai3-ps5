#!/usr/bin/env python3
"""Checks that converting the linked ELF into the PS5 eboot left the game's
writable data alone.

ps5-native-tool writes the PS5 process parameters into the file right after the
RELRO content. When that lands too close to the data segment it spills into the
first bytes of .data (test 1: it zeroed the start of PPCFuncMappings, so no
recompiled function got registered and the game thread had nothing to run).

    check_eboot_data.py <llvm-pie.elf> <eboot.elf>
"""
import struct
import sys


def sections(blob):
    shoff, = struct.unpack_from('<Q', blob, 0x28)
    shentsize, shnum, shstrndx = struct.unpack_from('<HHH', blob, 0x3A)
    def sh(i):
        return struct.unpack_from('<IIQQQQIIQQ', blob, shoff + i * shentsize)
    strtab = sh(shstrndx)
    out = {}
    for i in range(shnum):
        name, typ, flags, addr, off, size = sh(i)[:6]
        end = blob.index(b'\0', strtab[4] + name)
        out[blob[strtab[4] + name:end].decode()] = (typ, addr, off, size)
    return out


def main():
    src = open(sys.argv[1], 'rb').read()
    out = open(sys.argv[2], 'rb').read()
    secs = sections(src)
    bad = []
    # (.dynamic is rewritten elsewhere by the converter; everything after it is the game's)
    for name in ('.data', '.tdata'):
        if name not in secs:
            continue
        typ, addr, off, size = secs[name]
        if typ == 8 or size == 0:  # NOBITS
            continue
        if src[off:off + size] != out[off:off + size]:
            first = next(i for i in range(size) if src[off + i] != out[off + i])
            bad.append(f'{name} changed at +{first:#x} (address {addr + first:#x})')
    if bad:
        print('error: the eboot conversion overwrote game data: ' + '; '.join(bad), file=sys.stderr)
        sys.exit(1)
    print('eboot data check: .data unchanged')


if __name__ == '__main__':
    main()
