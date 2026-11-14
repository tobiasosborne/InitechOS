#!/usr/bin/env python3
"""Independent reader of the shipped dBASE III file format, never a producer.
Format: apps/initechbase/spec/ibase/dbf.md; literal data contract in PROVENANCE.
No application code is imported.
"""
import pathlib
import struct
import sys

root = pathlib.Path(sys.argv[1])
cases = {
    'CLIENTS': ([('NAME', 'C', 10, 0), ('CITY', 'C', 12, 0), ('BAL', 'N', 8, 2)], [
        ['PESTON', 'HONOLULU', '1234.50'],
        ['WADDAMS', 'AUSTIN', '-42.00'],
        ['LUMBERGH', 'DALLAS', '7777.77']]),
    'PARTS': ([('CODE', 'C', 3, 0), ('QTY', 'N', 5, 0), ('OK', 'L', 1, 0)], [
        ['AAA', '100', 'T'], ['BBB', '200', 'F'], ['CCC', '100', 'T'], ['DDD', '300', 'T']]),
    'INVOICE': ([('INVNO', 'C', 5, 0), ('CUST', 'C', 10, 0), ('AMOUNT', 'N', 10, 2),
                 ('DUEDATE', 'D', 8, 0), ('PAID', 'L', 1, 0)], [
        ['A1001', 'INITECH', '1250.00', '19991215', 'F'],
        ['A1002', 'ACME CORP', '875.50', '19000105', 'F'],
        ['A1003', 'GLOBEX INC', '4400.00', '19991130', 'T'],
        ['A1004', 'SOYLENT CO', '99.99', '19000210', 'F']]),
}
for name, (fields, expected) in cases.items():
    b = (root / (name + '.DBF')).read_bytes()
    nr, hl, rl = struct.unpack_from('<IHH', b, 4)
    assert b[:4] == bytes([3, 93, 7, 19]), (name, 'version/date')
    assert hl == 33 + 32 * len(fields) and rl == 1 + sum(f[2] for f in fields)
    assert b[12:32] == bytes(20) and b[hl - 1] == 13
    assert len(b) == hl + nr * rl + 1 and b[-1] == 26
    actual_fields = []
    for off in range(32, hl - 1, 32):
        d = b[off:off+32]
        actual_fields.append((d[:11].split(b'\0')[0].decode('ascii'), chr(d[11]), d[16], d[17]))
    assert actual_fields == fields, (name, actual_fields)
    rows = []
    for off in range(hl, hl + nr * rl, rl):
        assert b[off] == 32, (name, 'deleted row')
        row, pos = [], off + 1
        for _, _, width, _ in fields:
            row.append(b[pos:pos+width].decode('ascii').strip())
            pos += width
        rows.append(row)
    assert rows == expected, (name, rows)
    print('ok independent DBF reader:', name, len(rows), 'records; fixed date; no memo/index')
