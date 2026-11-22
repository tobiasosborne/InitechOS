"""Check media-dependent DIR and CHKDSK fields against independent FAT12 reads.

DIR layout: DOS 3.30 manual p.29 and dos/command/tests/dos33_dir.golden.
CHKDSK allocation and program memory: dos/chkdsk/tests/chkdsk_fixture.c.
"""
import struct
import sys
from pathlib import Path

from fat12_ref import Fat12


def entries(data):
    for offset in range(0, len(data), 32):
        record = data[offset:offset + 32]
        if record[0] == 0:
            break
        if record[0] != 0xe5 and record[11] != 0x0f and record[0] != ord('.'):
            yield record


def root(fat):
    return fat.data[fat.rdir_off:fat.rdir_off + 32 * fat.nrde]


def space(fat):
    sectors = struct.unpack_from('<H', fat.data, 19)[0]
    clusters = (sectors - fat.first_data_sector) // fat.spc
    free = sum(fat.fat12_entry(i) == 0 for i in range(2, 2 + clusters))
    unit = fat.spc * fat.bps
    return clusters * unit, free * unit


def dir_rows(fat):
    rows = []
    for record in entries(root(fat)):
        attributes = record[11]
        if attributes & 0x0e:  # hidden, system or volume label
            continue
        time, date = struct.unpack_from('<HH', record, 22)
        size = struct.unpack_from('<I', record, 28)[0]
        name = record[:8].decode('ascii') + ' ' + record[8:11].decode('ascii')
        field = '<DIR>' if attributes & 0x10 else str(size)
        hour = time >> 11
        rows.append(f'{name} {field:>8}  {(date >> 5) & 15:2}-{date & 31:02}-'
                    f'{(1980 + (date >> 9)) % 100:02}  {hour % 12 or 12:2}:'
                    f'{(time >> 5) & 63:02}{"p" if hour >= 12 else "a"}')
    return '\n'.join(rows) + f'\n\n{len(rows):9} File(s) {space(fat)[1]:9} bytes free'


def chkdsk_rows(fat, program):
    total, free = space(fat)
    unit = fat.spc * fat.bps
    pending = [root(fat)]
    visited = set()
    counts = dict.fromkeys(('directories', 'hidden files', 'user files'), 0)
    sizes = counts.copy()
    while pending:
        for record in entries(pending.pop()):
            first = struct.unpack_from('<H', record, 26)[0]
            chain = fat.walk_chain(first) if first else []
            if visited.intersection(chain):
                raise ValueError('cross-linked cluster in parity disk')
            visited.update(chain)
            if record[11] & 0x10:
                kind = 'directories'
                blocks = []
                for cluster in chain:
                    start = fat.first_data_sector * fat.bps + (cluster - 2) * unit
                    blocks.append(fat.data[start:start + unit])
                pending.append(b''.join(blocks))
            else:
                kind = 'hidden files' if record[11] & 0x0e else 'user files'
            counts[kind] += 1
            sizes[kind] += len(chain) * unit
    rows = [f'{total:9} bytes total disk space']
    for kind in ('hidden files', 'directories', 'user files'):
        rows.append(f'{sizes[kind]:9} bytes in {counts[kind]} {kind}')
    rows.append(f'{free:9} bytes available on disk')
    length = struct.unpack_from('<I', fat.find_path(program), 28)[0]
    # The COM arena begins after the loaded program and its 64 KiB stack.
    arena = (0x40100 + length + 0x10000 + 15) & ~15
    memory = 0x6f000 - arena - 16
    return '\n'.join(rows) + f'\n\n{memory:9} bytes total memory\n{memory:9} bytes free'


mode, image = sys.argv[1:3]
fat = Fat12(Path(image).read_bytes())
if mode == 'dir':
    print(dir_rows(fat))
elif mode == 'chkdsk':
    print(chkdsk_rows(fat, sys.argv[3]))
else:
    sys.exit('expected dir or chkdsk')
