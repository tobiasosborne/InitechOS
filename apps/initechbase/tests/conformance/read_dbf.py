#!/usr/bin/env python3
"""Independent test-side DBF reader; creates no fixtures or product artifacts.

Meaningful DBF bytes: version, count, header length, schema (full 11-byte
field names), terminator padding, raw record field bytes, and every byte
after the last declared record (the 0x1A EOF marker and anything past it).
Normalised away, and nothing else: the header update date (bytes 1-3), the
reserved header/descriptor bytes including descriptor RAM addresses, and
trailing NUL padding in the tail -- every FAT readback of a COPY FILE output
in reference/ is padded with NULs to a 512-byte boundary, including copies
of inputs the product never rewrote, so those NULs are a readback artefact,
not file content. Preserve physical short reads explicitly; do not turn
them into blank fields.
Source: dbase3-decomp/specs/file-formats/dbf.md sections 2, 4, 6, 8.
"""
import json
import os
import struct
import sys


def read_bytes(data):
    if len(data) < 32:
        return {"short_header": data.hex()}
    count, header, width = struct.unpack_from("<IHH", data, 4)
    result = {"version": data[0], "count": count, "header": header,
              "width": width, "fields": []}
    offset = 32
    while offset < len(data) and data[offset] != 13:
        field = data[offset:offset + 32]
        if len(field) != 32 or offset >= header:
            result["short_schema"] = data[32:header].hex()
            return result
        result["fields"].append([field[:11].hex(),
                                  field[11], field[16], field[17]])
        offset += 32
    if not width or header < offset + 1 or header > len(data):
        result["bad_geometry"] = data.hex()
        return result
    result["header_pad"] = data[offset + 1:header].hex()
    # Never allocate/iterate the untrusted record count: at most one short row.
    available = max(0, len(data) - header)
    rows = min(count, available // width)
    result["records"] = [data[header + i * width:header + (i + 1) * width].hex()
                         for i in range(rows)]
    if rows < count:
        result["short_records"] = data[header + rows * width:].hex()
    # Bytes after the last declared record: EOF 0x1A, a longer physical tail,
    # or nothing. Only trailing NUL sector padding is normalised (docstring).
    result["tail"] = data[header + count * width:].rstrip(b"\0").hex()
    return result


def read(path):
    with open(path, "rb") as stream:
        return read_bytes(stream.read())


def selftest():
    # Fixtures are byte edits of the genuine III+ C01 readback (+1 header,
    # 0x1A EOF); expected equal/unequal verdicts come from the file format,
    # not from this reader's own output on product files.
    here = os.path.dirname(os.path.abspath(__file__))
    with open(os.path.join(here, "reference", "C01", "BASE.DBF"), "rb") as s:
        base = s.read()
    header = struct.unpack_from("<H", base, 8)[0]
    want = read_bytes(base)
    date_reserved = bytearray(base)
    date_reserved[1:4] = b"\x01\x02\x03"          # header update date
    date_reserved[12] ^= 0xFF                     # reserved header byte
    date_reserved[44] ^= 0xFF                     # descriptor RAM address
    cases = [
        ("date/reserved-only difference compares equal",
         read_bytes(bytes(date_reserved)), True),
        ("512-boundary NUL padding compares equal",
         read_bytes(base + b"\0" * (512 - len(base))), True),
        ("garbage after last record compares unequal",
         read_bytes(base + b"GARBAGE-AFTER-LAST-RECORD"), False),
        ("missing EOF byte compares unequal",
         read_bytes(base[:-1]), False),
        ("wrong header length compares unequal",
         read_bytes(base[:8] + struct.pack("<H", header + 1)
                    + base[10:header] + b"\0" + base[header:]), False),
        ("descriptor name byte after NUL compares unequal",
         read_bytes(base[:36] + b"Z" + base[37:]), False),
    ]
    failures = 0
    for label, got, should_match in cases:
        if (got == want) != should_match:
            failures += 1
            print("FAIL: %s" % label)
    if failures:
        print("read_dbf selftest: %d checks, %d failures"
              % (len(cases), failures))
        return 1
    print("read_dbf selftest: %d checks, 0 failures" % len(cases))
    return 0


if __name__ == "__main__":
    if sys.argv[1] == "--selftest":
        sys.exit(selftest())
    if sys.argv[1] == "--dbt":
        with open(sys.argv[2], "rb") as stream:
            raw = stream.read()
        value = {"next_free": raw[:4].hex(), "memo_blocks": raw[512:].hex()}
    else:
        value = read(sys.argv[1])
    print(json.dumps(value, sort_keys=True, separators=(",", ":")))
