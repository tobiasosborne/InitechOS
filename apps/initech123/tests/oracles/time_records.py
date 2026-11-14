"""Independent WK1 cell records and global format, read from real captures."""
import struct
import sys

def records(path):
    data = open(path, 'rb').read()
    result = {}
    pos = 0
    while pos + 4 <= len(data):
        op, size = struct.unpack_from('<HH', data, pos)
        pos += 4
        assert pos + size <= len(data)
        body = data[pos:pos+size]
        pos += size
        if op == 7:
            result['global'] = body[4]
        if op in (12, 13, 14, 15, 16, 51):
            col, row = struct.unpack_from('<HH', body, 1)
            result[op, col, row] = body
        if op == 1:
            assert pos == len(data)
            return result
    raise AssertionError('missing EOF')

wanted, saved = map(records, sys.argv[1:])
for key in wanted.keys() | saved.keys():
    assert wanted.get(key) == saved.get(key), f'WK1 time format/value mismatch {key}: {wanted.get(key)!r} != {saved.get(key)!r}'
print(f'time format/value records agree with real R2.2 ({len(wanted)-1} cells)')
