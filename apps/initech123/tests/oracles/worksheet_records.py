"""Independent WK1 cell/name/protection/settings reader for command captures.

Record numbers/layout: pinned lotus123-decomp/specs/file-formats/wk1-format.md
and tools/wk1_ref.py, not the product codec. Native viewport dimensions and
cursor location intentionally do not grade worksheet content.
"""
import struct
import sys


def records(path):
    data = open(path, 'rb').read()
    result = {}
    pos = 0
    while pos + 4 <= len(data):
        op, size = struct.unpack_from('<HH', data, pos)
        pos += 4
        assert pos + size <= len(data), 'truncated record'
        body = data[pos:pos+size]
        pos += size
        key = None
        if op in (12, 13, 14, 15, 16, 51):
            col, row = struct.unpack_from('<HH', body, 1)
            key = ('cell', op, col, row)
        elif op == 0x47:
            key = ('name', body[:16])
        elif op in (2, 3, 0x24, 0x29, 0x2f):
            key = ('setting', op)
        if key is not None:
            assert key not in result, f'duplicate {key}'
            result[key] = body
        if op == 1:
            assert size == 0 and pos == len(data), 'invalid EOF/trailing bytes'
            return result
    raise AssertionError('missing EOF')


expected, actual = map(records, sys.argv[1:3])
if len(sys.argv) == 4 and sys.argv[3] in ("--native-fill-prefix", "--native-import-prefix"):
    expected = {key: value for key, value in expected.items()
                if key[0] != "cell" or (key[2] == 0 and key[3] < 2048)}
errors = 0
for key in sorted(expected.keys() | actual.keys()):
    if expected.get(key) != actual.get(key):
        print('FAIL WK1', key, 'real:', expected.get(key), 'saved:', actual.get(key))
        errors += 1
assert not errors, f'{errors} worksheet record mismatches'
print(f'worksheet contents/settings match real R2.2 ({len(expected)} records)')
