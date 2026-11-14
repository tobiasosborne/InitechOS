#!/usr/bin/env python3
"""Independent FAT12 disk reader: allocated iff owned by one live entry.

Microsoft FAT specification BPB, FAT12 packing, directory entries. This file
imports no product code; dot entries do not own their directory twice.
"""
import pathlib
import struct
import sys


def check(path):
    disk = path.read_bytes()
    word = lambda off: struct.unpack_from('<H', disk, off)[0]
    bps, spc, reserved = word(11), disk[13], word(14)
    copies, roots, spf = disk[16], word(17), word(22)
    total = word(19) or struct.unpack_from('<I', disk, 32)[0]
    root = reserved + copies * spf
    data = root + (roots * 32 + bps - 1) // bps
    count = (total - data) // spc
    fat = disk[reserved*bps:(reserved+spf)*bps]
    assert all(fat == disk[(reserved+i*spf)*bps:(reserved+(i+1)*spf)*bps]
               for i in range(copies)), 'FAT copies disagree'
    links = [((fat[c*3//2] | fat[c*3//2+1] << 8) >> (4 if c & 1 else 0)) & 4095
             for c in range(count+2)]
    owners = {}

    def chain(start, owner):
        body = bytearray()
        while start:
            assert 2 <= start < count+2, 'cluster outside volume'
            assert start not in owners, 'crosslink or cycle'
            owners[start] = owner
            off = (data + (start-2)*spc)*bps
            body += disk[off:off+spc*bps]
            link = links[start]
            assert link != 0, 'live file references free cluster'
            if link >= 4088:
                break
            start = link
        return body

    def directory(body, parent):
        for off in range(0, len(body), 32):
            entry = body[off:off+32]
            if entry[0] == 0:
                break
            if entry[0] == 229 or entry[11] & 8 or entry[:11] in (b'.          ', b'..         '):
                continue
            name = parent + '/' + entry[:11].decode('ascii')
            start, size = struct.unpack_from('<H', entry, 26)[0], struct.unpack_from('<I', entry, 28)[0]
            content = chain(start, name)
            if entry[11] & 16:
                directory(content, name)
            else:
                assert len(content) == ((size+spc*bps-1)//(spc*bps))*spc*bps, 'size/chain mismatch'
    directory(disk[root*bps:root*bps+roots*32], '')
    for c in range(2, count+2):
        assert (links[c] != 0) == (c in owners), 'lost allocation or free owner'


for image in sys.argv[1:]:
    try:
        check(pathlib.Path(image))
    except (AssertionError, IndexError, struct.error) as error:
        sys.exit(f'FAIL {image}: {error}')
print(f'PASS independent cluster conservation: {len(sys.argv)-1} images')
