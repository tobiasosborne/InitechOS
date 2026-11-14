#!/usr/bin/env python3
"""Independent directory metadata check (Microsoft FAT specification)."""
import pathlib
import struct
import sys

def entries(path):
    b = pathlib.Path(path).read_bytes()
    u16 = lambda n: struct.unpack_from('<H', b, n)[0]
    bps, reserved, spf = u16(11), u16(14), u16(22)
    root = reserved + b[16] * spf
    roots = u16(17)
    data = root + (roots * 32 + bps - 1) // bps
    fat = b[reserved*bps:(reserved+spf)*bps]
    result = {}
    def walk(body, prefix):
        for i in range(0, len(body), 32):
            e = body[i:i+32]
            if not e[0]: break
            if e[0] == 229 or e[0] == 46 or e[11] & 8: continue
            name = e[:8].decode().rstrip() + ('.' + e[8:11].decode().rstrip() if e[8:11].strip() else '')
            result[prefix + name] = e
            if e[11] & 16:
                c = struct.unpack_from('<H', e, 26)[0]
                directory = bytearray()
                while c < 4088:
                    off = (data + (c-2)*b[13])*bps
                    directory += b[off:off+b[13]*bps]
                    c = (int.from_bytes(fat[c*3//2:c*3//2+2], 'little') >> (4 if c & 1 else 0)) & 4095
                walk(directory, prefix + name + '/')
    walk(b[root*bps:root*bps+roots*32], '')
    return result

before, after = entries(sys.argv[1]), entries(sys.argv[2])
stage, dest = (s.replace('\\', '/') for s in sys.argv[3:5])
s, old, new = before[stage], before[dest], after[dest]
assert new[:22] == old[:22], 'destination name/attributes/reserved fields changed'
assert new[22:] == s[22:], 'staged timestamp/chain/size not published'
