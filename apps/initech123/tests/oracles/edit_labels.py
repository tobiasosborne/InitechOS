"""Independent WK1 record reader: compare saved LABEL bytes to real R2.2."""
import struct
import sys


def labels(path):
    data = open(path, 'rb').read()
    pos = 0
    result = {}
    eof = False
    while pos + 4 <= len(data):
        op, size = struct.unpack_from('<HH', data, pos)
        pos += 4
        assert pos + size <= len(data), 'truncated record'
        body = data[pos:pos + size]
        pos += size
        if op == 0x0f:
            col, row = struct.unpack_from('<HH', body, 1)
            assert body[-1] == 0
            result[col, row] = body[5:-1]
        if op == 1:
            eof = True
            break
    assert eof and pos == len(data), 'missing EOF or trailing bytes'
    return result


expected, actual = map(labels, sys.argv[1:])
assert expected, 'empty reference'
failed = 0
for cell in sorted(expected.keys() | actual.keys()):
    if expected.get(cell) != actual.get(cell):
        print('FAIL EDIT', cell, 'reference:', expected.get(cell), 'saved:', actual.get(cell))
        failed += 1
assert not failed, f'{failed} LABEL mismatches'
print(f'EDIT saved bytes: {len(expected)} real R2.2 LABELs agree independently')
