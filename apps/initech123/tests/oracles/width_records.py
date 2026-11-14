"""Independent WK1 width records; no product constants or codec."""
import struct
import sys

def widths(path):
    data = open(path, 'rb').read()
    pos = 0
    result = {}
    while pos < len(data):
        op, size = struct.unpack_from('<HH', data, pos)
        pos += 4
        assert pos + size <= len(data)
        body = data[pos:pos+size]
        pos += size
        if op == 7:  # WINDOW1, global width word
            result['global'] = struct.unpack_from('<H', body, 6)[0]
        if op == 8:  # COLW1
            result[struct.unpack_from('<H', body)[0]] = body[2]
        if op in (13, 14, 15, 16):
            result['cell', pos] = body  # these scenarios must not create cells
        if op == 1:
            assert pos == len(data)
            return result
    raise AssertionError('missing EOF')

want, got = map(widths, sys.argv[1:])
assert want == got, f'width mismatch: real R2.2 {want}, saved {got}'
print('saved width records agree with real R2.2:', got)
