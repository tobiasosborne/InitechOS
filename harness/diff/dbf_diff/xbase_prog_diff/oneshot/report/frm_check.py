#!/usr/bin/env python3
"""Independent III+ FRM reader, from specs/file-formats/frm.md Sections 2-7."""
import json
from pathlib import Path
import struct
import sys


def decode(path):
    b = Path(path).read_bytes()
    assert len(b) == 1990, len(b)
    word = lambda offset: struct.unpack_from('<H', b, offset)[0]
    assert word(0) == word(0x7C4) == 2
    lengths = struct.unpack_from('<55H', b, 4)
    pointers = struct.unpack_from('<54H', b, 0x74)
    total = word(2)
    assert sum(lengths) == total <= 0x68C - 0xE0
    count = sum(length != 0 for length in lengths)
    assert count % 2 == 1
    assert all(lengths[:count]) and not any(lengths[count:])

    def text(index):
        length = lengths[index]
        offset = pointers[index - 1] if index else 0
        assert length and offset + length <= total
        raw = b[0xE0 + offset:0xE0 + offset + length]
        assert raw[-1] == 0 and 0 not in raw[:-1]
        return raw[:-1].decode('ascii')

    ncols = word(0x7BE)
    assert 1 <= ncols <= 24
    groups = (count - 1) // 2 - ncols
    assert 0 <= groups <= 2
    columns = []
    for col in range(ncols):
        offset = 0x68C + 12 * col
        expr, head = word(offset + 8), word(offset + 10)
        assert 1 <= expr < count and 1 <= head < count
        assert word(offset + 2) == 1
        assert b[offset + 5] in b'YN'
        columns.append(dict(expression=text(expr), heading=text(head),
                            width=word(offset), decimals=word(offset + 6),
                            total=chr(b[offset + 5])))
    return dict(title=text(0), groups=groups, columns=columns,
                page_width=word(0x7B6), lines=word(0x7B8),
                left_margin=word(0x7BA), right_margin=word(0x7BC),
                flags=b[0x7C0:0x7C4].hex())


if __name__ == '__main__':
    form = decode(sys.argv[1])
    if '--acceptance' in sys.argv:
        assert form['title'].upper() == 'CLIENT BALANCES', form
        assert form['columns'] == [
            dict(expression='name', heading='client', width=10, decimals=0, total='N'),
            dict(expression='bal', heading='balance', width=12, decimals=2, total='Y')], form
        assert (form['page_width'], form['lines'], form['left_margin'],
                form['right_margin'], form['flags']) == (80, 58, 8, 0, '4e4e4e00')
        output = Path(sys.argv[1]).with_suffix('.TXT').read_bytes().upper()
        assert b'CLIENT BALANCES' in output and b'*** TOTAL ***' in output, output
        assert b'PESTON' in output and b'WADDAMS' in output and b'LUMBERGH' in output, output
        assert b'8970.27' in output, output
    print(json.dumps(form, indent=2))
