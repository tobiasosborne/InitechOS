#!/usr/bin/env python3
"""Independent LIST field oracle, Using III+ U5-144 (PDF 256).
Manual-derived cell rule. Raw DBFs are written with struct, never SAMIR's
writer. Decimal is independent of dec_format. Line framing (initech-ioke) is
the real III+ 1.1 capture xbase_prog_diff/fns-mint/numlist/GOLD.OUT (and
fns-mint/listoff): LIST/DISPLAY OFF keep the one-column deletion field, so
each record line opens with one blank before the first cell.
"""
# Real SF-numlist: "LIST OFF BAL" -> "  1234.50" for N(8,2); "DISPLAY OFF" ->
# " PESTON     HONOLULU      1234.50" (one deletion-field blank, then cells).
DELETION_FIELD = ' '

import decimal
import pathlib
import random
import struct
import subprocess
import sys


def numeric_cell(text, width, places):
    value = decimal.Decimal(text)
    quantum = decimal.Decimal(1).scaleb(-places)
    # Mint-001 confirms ties toward positive infinity; Decimal keeps this
    # independent of the engine's binary-double/integer implementation.
    rounded = (value / quantum + decimal.Decimal('0.5')).to_integral_value(
        rounding=decimal.ROUND_FLOOR) * quantum
    if not rounded:
        rounded = abs(rounded)
    cell = format(rounded, f'.{places}f')
    return cell.rjust(width) if len(cell) <= width else '*' * width


def main():
    binary, corpus = sys.argv[1:]
    corpus = pathlib.Path(corpus)
    required = ['mint/work/M3.TXT', 'mint/work/OTEST.DBF', 'mint/work/TOURS.DBF']
    for relative in required:
        if not (corpus / relative).is_file():
            raise SystemExit(f'FAIL: missing corpus {relative}; a skipped oracle is worse than a red one')
    root = pathlib.Path('build/numeric-format') / pathlib.Path(binary).name
    root.mkdir(parents=True, exist_ok=True)
    rng = random.Random(0x1A0)
    checks = 0
    for width in range(1, 21):
        for places in range(min(width - 1, 18) + 1):
            # Use representable stored decimals, below IEEE's integer limit.
            digits = min(12, width - (1 if places else 0))
            scale = 10 ** places
            values = [decimal.Decimal(0), decimal.Decimal(-42), decimal.Decimal(814)]
            values += [decimal.Decimal(rng.randrange(-10 ** digits, 10 ** digits)) / scale for _ in range(16)]
            cells = [numeric_cell(str(v), width, places) for v in values]
            # Stored overflows are tested separately by the decimal gate; don't
            # reinterpret '*' as a valid numeric field for this display oracle.
            cells = [c for c in cells if '*' not in c]
            if not cells:
                continue
            header = bytearray(32)
            struct.pack_into('<B3xIHH', header, 0, 3, len(cells), 65, width + 1)
            descriptor = bytearray(32)
            descriptor[0] = ord('V')
            descriptor[11] = ord('N')
            descriptor[16:18] = bytes([width, places])
            path = root / 'CASE.DBF'
            path.write_bytes(header + descriptor + b'\r' + b''.join(b' ' + c.encode('ascii') for c in cells) + b'\x1a')
            result = subprocess.run([binary, str(path)], capture_output=True, text=True)
            expected = ''.join(DELETION_FIELD + c + '\n' for c in cells)
            # LIST, explicit-field LIST, DISPLAY ALL, and current DISPLAY.
            expected = expected * 3 + DELETION_FIELD + cells[0] + '\n'
            checks += len(cells) * 3 + 1
            if result.returncode or result.stdout != expected:
                print(f'FAIL: N({width},{places}) descriptor differential', file=sys.stderr)
                print(f'expected {expected[:120]!r}; actual {result.stdout[:120]!r}', file=sys.stderr)
                print(f'test-samir-numeric-format: {checks} checks, FAIL')
                return 1
    # One row wider than the engine's 512-byte staging buffer. Independent
    # descriptor and payload construction checks the complete record view.
    cells = [numeric_cell(str(v), 20, 2) for v in range(-16, 16)]
    header = bytearray(32)
    struct.pack_into('<B3xIHH', header, 0, 3, 1, 32 + 32 * len(cells) + 1, 1 + 20 * len(cells))
    fields = bytearray()
    for index in range(len(cells)):
        descriptor = bytearray(32)
        name = f'V{index}'.encode('ascii')
        descriptor[:len(name)] = name
        descriptor[11] = ord('N')
        descriptor[16:18] = bytes([20, 2])
        fields += descriptor
    path = root / 'WIDE.DBF'
    path.write_bytes(header + fields + b'\r ' + ''.join(cells).encode('ascii') + b'\x1a')
    result = subprocess.run([binary, str(path), 'wide'], capture_output=True, text=True)
    expected = (DELETION_FIELD + ' '.join(cells) + '\n') * 3
    checks += len(cells) * 3
    if result.returncode or result.stdout != expected:
        print('FAIL: wide-record descriptor differential', file=sys.stderr)
        print(f'test-samir-numeric-format: {checks} checks, FAIL')
        return 1
    print(f'test-samir-numeric-format: {checks} cells PASS (manual-derived cells, real SF-numlist framing)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
