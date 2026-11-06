#!/usr/bin/env python3
"""Independent III+ LBL reader, from specs/file-formats/lbl.md Sections 2-6."""
import json
from pathlib import Path
import struct
import sys


def decode(path):
    b = Path(path).read_bytes()
    assert len(b) == 1034 and b[0] == b[-1] == 2
    height, width, left, lines, spaces, across = struct.unpack_from('<6H', b, 61)
    assert 1 <= height <= 16 and 1 <= width <= 120 and 1 <= across <= 5
    assert left <= 120 and lines <= 16 and spaces <= 120
    assert left + width * across + spaces * (across - 1) <= 250
    slots = [b[73 + n * 60:133 + n * 60] for n in range(16)]
    assert all(0 not in slot for slot in slots)
    return dict(remarks=b[1:61].decode('ascii').rstrip(' '), height=height,
                width=width, left=left, lines=lines, spaces=spaces, across=across,
                expressions=[slot.decode('ascii').rstrip(' ') for slot in slots])


if __name__ == '__main__':
    form = decode(sys.argv[1])
    if '--acceptance' in sys.argv:
        assert (form['height'], form['width'], form['left'], form['lines'],
                form['spaces'], form['across']) == (5, 35, 0, 1, 2, 1), form
        assert form['remarks'] == '3 1/2 x 15/16 by 1', form
        assert form['expressions'][:3] == ['name', 'city', 'str(bal,8,2)'], form
        assert not any(form['expressions'][3:]), form
        output = Path(sys.argv[1]).with_suffix('.TXT').read_bytes().upper()
        assert b'PESTON' in output and b'WADDAMS' in output and b'LUMBERGH' in output, output
        assert b'HONOLULU' in output and b'AUSTIN' in output and b'DALLAS' in output, output
    print(json.dumps(form, indent=2))
