#!/usr/bin/env python3
"""Independent test-side DBF reader; creates no fixtures or product artifacts.

Only meaningful DBF bytes: version, count, schema and raw record field bytes.
Ignore update date, reserved bytes/RAM addresses and +1/+2 header padding.
Preserve physical short reads explicitly; do not turn them into blank fields.
Source: dbase3-decomp/specs/file-formats/dbf.md sections 2, 4, 6, 8.
"""
import json
import struct
import sys


def read(path):
    with open(path, "rb") as stream:
        data = stream.read()
    if len(data) < 32:
        return {"short_header": data.hex()}
    count, header, width = struct.unpack_from("<IHH", data, 4)
    result = {"version": data[0], "count": count, "width": width, "fields": []}
    offset = 32
    while offset < len(data) and data[offset] != 13:
        field = data[offset:offset + 32]
        if len(field) != 32 or offset >= header:
            result["short_schema"] = data[32:header].hex()
            return result
        result["fields"].append([field[:11].split(b"\0")[0].hex(),
                                  field[11], field[16], field[17]])
        offset += 32
    if not width or header < offset + 1 or header > len(data):
        result["bad_geometry"] = data.hex()
        return result
    # Never allocate/iterate the untrusted record count: at most one short row.
    available = max(0, len(data) - header)
    rows = min(count, available // width)
    result["records"] = [data[header + i * width:header + (i + 1) * width].hex()
                         for i in range(rows)]
    if rows < count:
        result["short_records"] = data[header + rows * width:].hex()
    return result


if __name__ == "__main__":
    if sys.argv[1] == "--dbt":
        with open(sys.argv[2], "rb") as stream:
            raw = stream.read()
        value = {"next_free": raw[:4].hex(), "memo_blocks": raw[512:].hex()}
    else:
        value = read(sys.argv[1])
    print(json.dumps(value, sort_keys=True, separators=(",", ":")))
