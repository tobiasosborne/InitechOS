#!/usr/bin/env python3
"""Independent LIST field oracle, Using III+ U5-144 (PDF 256).
Manual-derived, no captured LIST golden in the local corpus. Raw DBFs are
written with struct, never SAMIR's writer. Decimal is independent of dec_format.
"""
import decimal
import pathlib
import random
import struct
import subprocess
import sys


def numeric_cell(text, width, places):
    value = decimal.Decimal(text)
    quantum = decimal.Decimal(1).scaleb(-places)
    rounded = value.quantize(quantum, rounding=decimal.ROUND_HALF_UP)
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
    for width in range(2, 21):
        for places in range(min(width - 1, 15) + 1):
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
            expected = ''.join(c + '\n' for c in cells)
            # LIST, explicit-field LIST, DISPLAY ALL, and current DISPLAY.
            expected = expected * 3 + cells[0] + '\n'
            checks += len(cells) * 3 + 1
            if result.returncode or result.stdout != expected:
                print(f'FAIL: N({width},{places}) descriptor differential', file=sys.stderr)
                print(f'expected {expected[:120]!r}; actual {result.stdout[:120]!r}', file=sys.stderr)
                print(f'test-samir-numeric-format: {checks} checks, FAIL')
                return 1
    print(f'test-samir-numeric-format: {checks} cells PASS (manual-derived, no LIST golden)')
    return 0


if __name__ == '__main__':
    sys.exit(main())
