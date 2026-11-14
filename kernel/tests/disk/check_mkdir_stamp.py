#!/usr/bin/env python3
"""Independent FAT12 reader of the three MKDIR timestamps (MS FAT layout).

The fixed DOS clock fixture is 1992-03-02 09:30:00. Expected packed words are
literal reference-format values; no kernel definitions or writer are imported.
"""
import pathlib
import struct
import sys

raw = pathlib.Path(sys.argv[1]).read_bytes()
checks = 0

def check(condition, message):
    global checks
    checks += 1
    if not condition:
        print('FAIL MKDIR format: ' + message)
        print(f'INITECH_CHECKS {checks} failures=1')
        sys.exit(1)

word = lambda offset: struct.unpack_from('<H', raw, offset)[0]
bps, spc, reserved = word(11), raw[13], word(14)
root = (reserved + raw[16] * word(22)) * bps
root_bytes = word(17) * 32
data = root + ((root_bytes + bps - 1) // bps) * bps

def cluster(entry):
    number = struct.unpack_from('<H', entry, 26)[0]
    check(number >= 2, 'directory needs its own cluster')
    offset = data + (number - 2) * spc * bps
    return raw[offset:offset + spc * bps]

def find(body, name):
    entries = [body[i:i + 32] for i in range(0, len(body), 32)]
    matches = [entry for entry in entries if entry[:11] == name]
    check(len(matches) == 1, 'expected one named directory entry')
    return matches[0]

body = raw[root:root + root_bytes]
if sys.argv[2] == 'sub':
    body = cluster(find(body, b'SUB        '))
entry = find(body, b'NEWDIR     ')
dots = cluster(entry)
for candidate in (entry, dots[:32], dots[32:64]):
    check(candidate[11] & 16, 'directory attribute')
    check(struct.unpack_from('<HH', candidate, 22) == (0x4bc0, 0x1862), 'runtime timestamp')
check(dots[:11] == b'.          ' and dots[32:43] == b'..         ', 'dot entries')
print('PASS independent FAT reader: parent, dot and dot-dot timestamps')
print(f'INITECH_CHECKS {checks} failures=0')
