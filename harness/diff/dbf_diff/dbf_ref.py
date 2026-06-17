#!/usr/bin/env python3
# harness/diff/dbf_diff/dbf_ref.py -- independent dBASE III+ .dbf/.dbt reference reader.
#
# FACTORY tooling (CLAUDE.md Law 3): the artifact stays C; this python reader is
# the INDEPENDENT reference for the SAMIR (InitechBase) differential oracle
# (beads initech-586.5.7).  It is a SECOND implementation that shares NO code
# and NO constants with os/samir/* or spec/samir/*.  Its purpose is to catch any
# bug that is SHARED by our C reader -- agreement between two independent
# derivations is a far stronger signal than one.
#
# Every offset, size, and encoding decision below is derived from FIRST PRINCIPLES
# per the spec docs at:
#   ../dbase3-decomp/specs/file-formats/dbf.md
#   ../dbase3-decomp/specs/file-formats/dbt.md
# NOT from our C headers or any imported module.
#
# Subcommands:
#   --schema  <file.dbf>  : print normalized header + field descriptors.
#                           Volatile bytes (last-update date, reserved fields,
#                           per-descriptor RAM address) are replaced with fixed
#                           sentinels so output is deterministic (Rule 11).
#   --records <file.dbf>  : print each record: delete-flag + decoded field values.
#   --memo    <file.dbf>  : for each record with a non-zero M pointer, print
#                           recno and decoded memo text from the sibling .dbt.
#   --selftest            : assert known-true values from dbf.md "Verification"
#                           against the real goldens.  Exits 0 on success,
#                           non-zero on failure.  Fails loud if goldens absent.
#
# Fail loud (CLAUDE.md Rule 2): structural errors raise / exit non-zero.
# ASCII-clean (Rule 12).  Deterministic output (Rule 11): no wall-clock,
# field iteration is in descriptor order (stable).
# Pure stdlib (struct/sys/os/argparse) -- no extra runtimes (Law 3).

import os
import struct
import sys

# ---------------------------------------------------------------------------
# DBF layout constants (derived from dbf.md sections 2, 4, 5, 6)
# ---------------------------------------------------------------------------
# Section 2: 32-byte file header
_HDR_SIZE        = 32       # dbf.md sec 2: "32-byte file header"
_HDR_OFF_VER     = 0x00     # 1 byte: version/flags
_HDR_OFF_YY      = 0x01     # 1 byte: last-update year (YY, subtract 1900 base)
_HDR_OFF_MM      = 0x02     # 1 byte: last-update month
_HDR_OFF_DD      = 0x03     # 1 byte: last-update day
_HDR_OFF_NREC    = 0x04     # 4 bytes LE u32: number of records
_HDR_OFF_HLEN    = 0x08     # 2 bytes LE u16: header length
_HDR_OFF_RLEN    = 0x0A     # 2 bytes LE u16: record length
# bytes 0x0C-0x1F: reserved/volatile (date, multi-user, LDID)

# Section 4: 32-byte field descriptor
_DESC_SIZE       = 32       # dbf.md sec 4: descriptor stride = 32 bytes
_DESC_OFF_NAME   = 0x00     # 11 bytes: field name, NUL-terminated, NUL-padded
_DESC_OFF_TYPE   = 0x0B     # 1 byte: field type char (C/N/D/L/M)
_DESC_OFF_ADDR   = 0x0C     # 4 bytes: RAM address (meaningless on disk -- ignore)
_DESC_OFF_LEN    = 0x10     # 1 byte: field length in record
_DESC_OFF_DEC    = 0x11     # 1 byte: decimal count (N only)
# bytes 0x12-0x1F: reserved / work-area id etc.

# Section 4: terminator
_FIELD_TERM      = 0x0D     # 0x0D byte ends the descriptor array

# Section 3: version byte values
_VER_NO_MEMO     = 0x03     # dBASE III/III+ table, no memo
_VER_WITH_MEMO   = 0x83     # dBASE III+ table with memo (.dbt sibling)
_MEMO_BIT        = 0x80     # bit 7: has memo file

# Section 6: delete flag
_DEL_LIVE        = 0x20     # space = active record
_DEL_DELETED     = 0x2A     # '*' = deleted (not yet PACKed)

# Section 5: field type codes
_TYPE_C          = ord('C')
_TYPE_N          = ord('N')
_TYPE_D          = ord('D')
_TYPE_L          = ord('L')
_TYPE_M          = ord('M')

# Logical field true/false sets (dbf.md sec 5)
_LOGICAL_TRUE    = set(b'TtYy')
_LOGICAL_FALSE   = set(b'FfNn')
_LOGICAL_UNINIT  = set(b'? ')

# Memo "no pointer" sentinel (dbf.md sec 5 / dbt.md sec 4)
_MEMO_NO_BLOCK   = 0        # block 0 = header; M ptr of 0 or blank -> no memo

# ---------------------------------------------------------------------------
# DBT layout constants (derived from dbt.md sections 2, 3, 5)
# ---------------------------------------------------------------------------
_DBT_BLOCK_SIZE  = 512      # dbt.md sec 2: hard-coded 512 bytes per block in III+
_DBT_OFF_NEXT    = 0x00     # 4 bytes LE u32: next-available block number
# remaining 508 bytes of block 0: reserved/garbage (dbt.md sec 2.1)

_DBT_MEMO_TERM   = b'\x1a\x1a'  # dbt.md sec 5: two consecutive 0x1A bytes end memo


# ---------------------------------------------------------------------------
# Normalization sentinels (used by --schema to make output deterministic)
# ---------------------------------------------------------------------------
_NORM_DATE       = "YYYY-MM-DD"   # replaces the volatile last-update date
_NORM_ADDR       = "0x00000000"   # replaces the per-descriptor RAM pointer


# ---------------------------------------------------------------------------
# Core parser
# ---------------------------------------------------------------------------

class DbfFile:
    """Independent dBASE III+ .dbf reader.

    Parses from first principles per dbf.md/dbt.md.  Does NOT import
    spec/samir/* or os/samir/*.
    """

    def __init__(self, path):
        """Open and parse the .dbf at PATH."""
        try:
            with open(path, "rb") as fp:
                self._data = fp.read()
        except OSError as exc:
            raise ValueError("cannot open '%s': %s" % (path, exc))

        self._path = path
        self._parse_header()
        self._parse_fields()

    # ------------------------------------------------------------------
    # Internal parsing
    # ------------------------------------------------------------------

    def _parse_header(self):
        """Parse the 32-byte file header (dbf.md sec 2)."""
        d = self._data
        if len(d) < _HDR_SIZE:
            raise ValueError("file too short for 32-byte header: %d bytes" % len(d))

        # Offset 0x00: version/flags byte (dbf.md sec 3)
        self.version = d[_HDR_OFF_VER]

        # Offsets 0x01-0x03: last-update date YY,MM,DD (dbf.md sec 2; YY not MM,DD!)
        self.update_yy = d[_HDR_OFF_YY]    # year - 1900
        self.update_mm = d[_HDR_OFF_MM]
        self.update_dd = d[_HDR_OFF_DD]

        # Offset 0x04: record count, 4-byte LE u32 (dbf.md sec 2)
        self.record_count = struct.unpack_from("<I", d, _HDR_OFF_NREC)[0]

        # Offset 0x08: header length, 2-byte LE u16 (dbf.md sec 2)
        self.header_length = struct.unpack_from("<H", d, _HDR_OFF_HLEN)[0]

        # Offset 0x0A: record length, 2-byte LE u16 (dbf.md sec 2)
        self.record_length = struct.unpack_from("<H", d, _HDR_OFF_RLEN)[0]

        # Sanity checks
        if self.header_length < _HDR_SIZE + 1:
            raise ValueError("header_length %d implausibly small" % self.header_length)
        if self.record_length < 1:
            raise ValueError("record_length %d must be >= 1" % self.record_length)

    def _parse_fields(self):
        """Parse the field-descriptor array (dbf.md sec 4).

        Scans 32-byte descriptors from offset 32 until a 0x0D terminator byte.
        This is the AUTHORITATIVE way to find the field count (sec 4 / sec 8).
        We do NOT compute nfields from header_length because both +1 and +2
        terminator conventions exist in real III+ 1.1 files.
        """
        d = self._data
        self.fields = []    # list of (name, type_char, field_len, dec_count)
        off = _HDR_SIZE     # descriptors start immediately after the 32-byte header

        while True:
            if off >= len(d):
                raise ValueError(
                    "ran off end of file scanning field descriptors (no 0x0D term)")
            if d[off] == _FIELD_TERM:
                break   # 0x0D terminator found
            if off + _DESC_SIZE > len(d):
                raise ValueError(
                    "incomplete field descriptor at offset 0x%X" % off)

            # Name: 11 bytes, NUL-terminated, NUL-padded.
            # Trailing garbage after the first NUL is ignored (dbf.md sec 4).
            name_bytes = d[off + _DESC_OFF_NAME : off + _DESC_OFF_NAME + 11]
            nul = name_bytes.find(0)
            if nul < 0:
                nul = 11
            try:
                fname = name_bytes[:nul].decode("ascii")
            except UnicodeDecodeError:
                raise ValueError(
                    "non-ASCII field name at descriptor offset 0x%X" % off)

            # Type: one ASCII char (C/N/D/L/M for III+)
            ftype = d[off + _DESC_OFF_TYPE]
            if ftype not in (_TYPE_C, _TYPE_N, _TYPE_D, _TYPE_L, _TYPE_M):
                raise ValueError(
                    "unknown III+ field type 0x%02X ('%s') in descriptor '%s'"
                    % (ftype, chr(ftype) if 0x20 <= ftype < 0x7F else '?', fname))

            # Field length and decimal count (dbf.md sec 4 offsets 0x10, 0x11)
            flen = d[off + _DESC_OFF_LEN]
            fdec = d[off + _DESC_OFF_DEC]

            self.fields.append((fname, chr(ftype), flen, fdec))
            off += _DESC_SIZE

        # Structural invariant: record_length == 1 + sum(field lengths) (dbf.md sec 8)
        expected_rlen = 1 + sum(fl for _, _, fl, _ in self.fields)
        if self.record_length != expected_rlen:
            raise ValueError(
                "record_length %d != 1+sum(field_lens)=%d"
                % (self.record_length, expected_rlen))

    # ------------------------------------------------------------------
    # Public accessors
    # ------------------------------------------------------------------

    def has_memo(self):
        """True if the version byte has bit 7 set (dbf.md sec 3)."""
        return bool(self.version & _MEMO_BIT)

    def iter_records(self):
        """Yield (recno, del_flag_byte, [(name, type, raw_bytes), ...])
        for each record.  Iterates record_count records starting at header_length
        (dbf.md sec 6).  Uses nrec from the header as authoritative (sec 9);
        the optional 0x1A EOF byte and any ghost data beyond are ignored.
        """
        d = self._data
        base = self.header_length
        for i in range(self.record_count):
            rec_off = base + i * self.record_length
            if rec_off + self.record_length > len(d):
                raise ValueError(
                    "record %d extends past end of file (offset 0x%X)"
                    % (i, rec_off + self.record_length))
            del_flag = d[rec_off]
            field_off = rec_off + 1
            field_vals = []
            for fname, ftype, flen, fdec in self.fields:
                raw = d[field_off : field_off + flen]
                field_vals.append((fname, ftype, flen, fdec, raw))
                field_off += flen
            yield (i, del_flag, field_vals)

    # ------------------------------------------------------------------
    # Field decoders (dbf.md sec 5)
    # ------------------------------------------------------------------

    @staticmethod
    def decode_field(ftype, flen, fdec, raw):
        """Decode raw field bytes to a typed Python value.

        C -> str (rstrip trailing spaces, as left-justified ASCII)
        N -> float or int (or None if blank/asterisk-overflow)
        D -> str 'YYYYMMDD' (or '' if blank)
        L -> True/False/None (uninitialised)
        M -> int block number (0 = no memo)

        dbf.md sec 5: all III+ fields are fixed-width printable ASCII.
        """
        if ftype == 'C':
            # Left-justified, space-padded (dbf.md sec 5 C).
            return raw.decode("latin-1").rstrip(" ")

        if ftype == 'N':
            # Right-justified, space-padded ASCII numeric (dbf.md sec 5 N).
            # Blank = empty/uninitialised; '*'-filled = overflow (documented, not
            # fixture-confirmed -- dbf.md sec 5 Open Q 4).
            s = raw.decode("latin-1").strip()
            if s == "" or all(c == '*' for c in s):
                return None
            try:
                if fdec > 0:
                    return float(s)
                else:
                    return int(s)
            except ValueError:
                return None

        if ftype == 'D':
            # 8-char YYYYMMDD, full century (dbf.md sec 5 D).
            s = raw.decode("latin-1")
            if s.strip() == "":
                return ""
            return s   # already "YYYYMMDD"

        if ftype == 'L':
            # 1 byte: T/t/Y/y=True, F/f/N/n=False, '?'/space=uninit (dbf.md sec 5 L).
            b = raw[0] if raw else ord('?')
            if b in _LOGICAL_TRUE:
                return True
            if b in _LOGICAL_FALSE:
                return False
            return None   # uninitialised

        if ftype == 'M':
            # 10-byte right-justified ASCII decimal block number (dbf.md sec 5 M).
            # 0 or blank = no memo (dbt.md sec 4).
            s = raw.decode("latin-1").strip()
            if s == "" or s == "0":
                return 0
            try:
                return int(s)
            except ValueError:
                return 0

        raise ValueError("unknown field type '%s'" % ftype)


# ---------------------------------------------------------------------------
# .dbt reader
# ---------------------------------------------------------------------------

class DbtFile:
    """Independent dBASE III+ .dbt memo reader.

    Parses from first principles per dbt.md.  Block size is hard-coded 512
    (dbt.md sec 2): it is NOT stored in the file for III+.
    """

    def __init__(self, path):
        try:
            with open(path, "rb") as fp:
                self._data = fp.read()
        except OSError as exc:
            raise ValueError("cannot open .dbt '%s': %s" % (path, exc))

        if len(self._data) < _DBT_BLOCK_SIZE:
            raise ValueError(
                ".dbt file '%s' shorter than one block (%d bytes)" % (path, len(self._data)))

        # Block 0, offset 0: next-available block, 4-byte LE u32 (dbt.md sec 3)
        self.next_free = struct.unpack_from("<I", self._data, _DBT_OFF_NEXT)[0]

    def read_memo(self, block_num):
        """Return the memo text bytes for the memo starting at BLOCK_NUM.

        Reads forward from block_num * 512, across consecutive blocks, until
        the 0x1A 0x1A terminator (dbt.md sec 5).  Raises ValueError if the
        block number is out of range or the terminator is not found.

        There is NO per-block header in III+ (dbt.md sec 2.2): the text starts
        at byte 0 of the starting block.
        """
        if block_num == _MEMO_NO_BLOCK:
            raise ValueError("block_num 0 is the header -- no memo")

        start_off = block_num * _DBT_BLOCK_SIZE
        if start_off >= len(self._data):
            raise ValueError(
                "memo block %d (offset 0x%X) beyond end of .dbt file (%d bytes)"
                % (block_num, start_off, len(self._data)))

        # Read from start_off forward, looking for 0x1A 0x1A (dbt.md sec 5).
        # The terminator can span block boundaries -- read the whole remainder.
        remaining = self._data[start_off:]
        term_pos = remaining.find(_DBT_MEMO_TERM)
        if term_pos < 0:
            raise ValueError(
                "0x1A 0x1A terminator not found for memo block %d" % block_num)

        return remaining[:term_pos]


# ---------------------------------------------------------------------------
# Subcommand: --schema
# ---------------------------------------------------------------------------

def cmd_schema(dbf_path, out):
    """Print normalized header + field descriptors.

    Volatile bytes (last-update date, reserved, descriptor RAM address) are
    replaced with fixed sentinels so the output is deterministic (Rule 11).
    """
    db = DbfFile(dbf_path)

    # Header summary (dbf.md sec 2)
    out.write("version:      0x%02X\n" % db.version)
    out.write("last_update:  %s\n" % _NORM_DATE)  # volatile -> sentinel
    out.write("record_count: %d\n" % db.record_count)
    out.write("field_count:  %d\n" % len(db.fields))
    out.write("header_length: %d\n" % db.header_length)
    out.write("record_length: %d\n" % db.record_length)
    out.write("has_memo:     %s\n" % ("yes" if db.has_memo() else "no"))
    out.write("fields:\n")

    # Per field: name, type, len, dec (RAM address normalized away)
    for fname, ftype, flen, fdec in db.fields:
        out.write("  %-10s  %s  len=%-3d  dec=%d  addr=%s\n"
                  % (fname, ftype, flen, fdec, _NORM_ADDR))


# ---------------------------------------------------------------------------
# Subcommand: --records
# ---------------------------------------------------------------------------

def _format_del_flag(b):
    """Return a human-readable delete-flag label (dbf.md sec 6)."""
    if b == _DEL_LIVE:
        return "active"
    if b == _DEL_DELETED:
        return "deleted"
    return "?0x%02X" % b


def _format_value(ftype, decoded):
    """Format a decoded field value for deterministic text output."""
    if decoded is None:
        return "(null)"
    if ftype == 'N':
        # Format numbers without locale-dependent float repr noise.
        if isinstance(decoded, float):
            # %.6g trims trailing zeros but keeps enough precision.
            return "%.6g" % decoded
        return str(decoded)
    if ftype == 'L':
        return "T" if decoded else "F"
    if ftype == 'M':
        return "block:%d" % decoded
    return str(decoded)


def cmd_records(dbf_path, out):
    """Print decoded records, one per line."""
    db = DbfFile(dbf_path)

    for recno, del_flag, field_vals in db.iter_records():
        parts = ["rec%d" % recno, _format_del_flag(del_flag)]
        for fname, ftype, flen, fdec, raw in field_vals:
            decoded = DbfFile.decode_field(ftype, flen, fdec, raw)
            parts.append("%s=%s" % (fname, _format_value(ftype, decoded)))
        out.write(" ".join(parts) + "\n")


# ---------------------------------------------------------------------------
# Subcommand: --memo
# ---------------------------------------------------------------------------

def _find_dbt_path(dbf_path):
    """Derive the sibling .dbt path from the .dbf path.

    Tries exact-case DBT, then lower-case dbt, then upper-case DBT.
    Returns the first that exists, or None.
    """
    base, _ = os.path.splitext(dbf_path)
    for ext in (".dbt", ".DBT"):
        candidate = base + ext
        if os.path.exists(candidate):
            return candidate
    return None


def cmd_memo(dbf_path, out):
    """For each record with a non-zero M pointer, print recno and memo text."""
    db = DbfFile(dbf_path)

    if not db.has_memo():
        out.write("(no memo: version byte 0x%02X has no memo bit)\n" % db.version)
        return

    # Find M-type field(s)
    m_fields = [(i, fname) for i, (fname, ftype, flen, fdec)
                in enumerate(db.fields) if ftype == 'M']
    if not m_fields:
        out.write("(no M-type fields in schema)\n")
        return

    dbt_path = _find_dbt_path(dbf_path)
    if dbt_path is None:
        raise ValueError(
            "memo bit set in '%s' but no sibling .dbt found" % dbf_path)

    dbt = DbtFile(dbt_path)

    for recno, del_flag, field_vals in db.iter_records():
        for fi, fname in m_fields:
            _, ftype, flen, fdec, raw = field_vals[fi]
            block_num = DbfFile.decode_field(ftype, flen, fdec, raw)
            if block_num == 0:
                continue    # no memo for this record
            try:
                memo_bytes = dbt.read_memo(block_num)
            except ValueError as exc:
                out.write("rec%d %s=ERROR(%s)\n" % (recno, fname, exc))
                continue
            # Decode as latin-1 (CP437 superset for printable ASCII range).
            # Replace soft-wrap 0x8D with a marker so output stays ASCII-clean
            # (dbt.md sec 2.2: 0x8D 0x0A = soft word-wrap, a non-ASCII byte).
            memo_text = memo_bytes.decode("latin-1")
            out.write("rec%d %s=%s\n" % (recno, fname, repr(memo_text)))


# ---------------------------------------------------------------------------
# Selftest
# ---------------------------------------------------------------------------

def _selftest_fail(msg, failures):
    """Append a failure message and print it immediately."""
    sys.stderr.write("FAIL: %s\n" % msg)
    failures.append(msg)


def run_selftest():
    """Assert dbf.md 'Verification' values against the real goldens.

    Fails loud if the goldens directory is absent (CLAUDE.md Rule 2).
    Prints 'dbf_ref selftest: N checks, 0 failures' or exits non-zero.
    """
    # Locate the golden tree relative to this script.
    # Expected layout: this file is at
    #   <repo>/harness/diff/dbf_diff/dbf_ref.py
    # goldens are at
    #   <repo>/../dbase3-decomp/goldens/...
    here = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.normpath(os.path.join(here, "..", "..", ".."))
    golden_base = os.path.normpath(os.path.join(repo_root, "..", "dbase3-decomp", "goldens"))

    if not os.path.isdir(golden_base):
        sys.stderr.write(
            "dbf_ref selftest: ABORT -- goldens directory not found at '%s'\n"
            % golden_base)
        sys.exit(2)

    pristine = os.path.join(golden_base,
                            "dbase-iii-plus-1.1-pristine",
                            "files", "Sample_Programs_and_Utilities")
    disk2 = os.path.join(golden_base,
                         "dbase-iii-plus-1.1", "extracted", "disk2")

    checks = 0
    failures = []

    def check(desc, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            _selftest_fail("%s: got %r want %r" % (desc, got, want), failures)

    # ------------------------------------------------------------------
    # CLIENTS.DBF (dbf.md sec 8 / Verification section)
    # ver=0x03, nrec=49, 7 fields, hlen=257, rlen=106, +1 term, no EOF
    # ------------------------------------------------------------------
    clients_path = os.path.join(pristine, "CLIENTS.DBF")
    db = DbfFile(clients_path)
    check("CLIENTS version",      db.version,       0x03)
    check("CLIENTS record_count", db.record_count,  49)
    check("CLIENTS field_count",  len(db.fields),   7)
    check("CLIENTS header_length",db.header_length, 257)
    check("CLIENTS record_length",db.record_length, 106)
    check("CLIENTS has_memo",     db.has_memo(),    False)

    # Invariant 1 (+1 terminator): header_length == 32 + 32*7 + 1 = 257
    check("CLIENTS hlen invariant +1",
          db.header_length, 32 + 32 * len(db.fields) + 1)

    # Spot-check field names and types (dbf.md sec 4 / Verification)
    # CLIENTS fields: FIRSTNAME C(20), LASTNAME C(20), ADDRESS C(20),
    #                 CITY C(20), STATE C(2), ZIPCODE C(5), PHONE C(19)
    check("CLIENTS field0 name", db.fields[0][0], "FIRSTNAME")
    check("CLIENTS field0 type", db.fields[0][1], "C")
    check("CLIENTS field0 len",  db.fields[0][2], 20)

    # Spot-check CLIENTS rec0: FIRSTNAME='Claire', STATE='CA', ZIPCODE='93034'
    # (dbf.md sec 5 C / Verification)
    recs = list(db.iter_records())
    _, _, fv0 = recs[0]
    # build a dict for easy lookup
    r0 = {fname: DbfFile.decode_field(ftype, flen, fdec, raw)
          for fname, ftype, flen, fdec, raw in fv0}
    check("CLIENTS rec0 FIRSTNAME", r0["FIRSTNAME"], "Claire")
    check("CLIENTS rec0 STATE",     r0["STATE"],     "CA")
    check("CLIENTS rec0 ZIPCODE",   r0["ZIPCODE"],   "93034")

    # ------------------------------------------------------------------
    # TOURS.DBF
    # ver=0x83, nrec=30, 7 fields, hlen=257, rlen=83, +1 term, no EOF
    # ------------------------------------------------------------------
    tours_path = os.path.join(pristine, "TOURS.DBF")
    db = DbfFile(tours_path)
    check("TOURS version",       db.version,       0x83)
    check("TOURS record_count",  db.record_count,  30)
    check("TOURS field_count",   len(db.fields),   7)
    check("TOURS header_length", db.header_length, 257)
    check("TOURS record_length", db.record_length, 83)
    check("TOURS has_memo",      db.has_memo(),    True)
    check("TOURS hlen invariant +1",
          db.header_length, 32 + 32 * len(db.fields) + 1)

    # Spot-check TOURS rec0 (dbf.md sec 6 worked record):
    # TRAVELCODE='AV10', DEPARTURE='19850805', UNITCOST=1378.00, NOTE=block 0
    recs = list(db.iter_records())
    _, _, fv0 = recs[0]
    r0 = {fname: DbfFile.decode_field(ftype, flen, fdec, raw)
          for fname, ftype, flen, fdec, raw in fv0}
    check("TOURS rec0 TRAVELCODE", r0["TRAVELCODE"], "AV10")
    check("TOURS rec0 DEPARTURE",  r0["DEPARTURE"],  "19850805")
    check("TOURS rec0 UNITCOST",   r0["UNITCOST"],   1378.00)
    check("TOURS rec0 NOTE (no memo)", r0["NOTE"],   0)

    # All 30 TOURS records have NOTE=0 (no memo) (dbt.md sec 3 / Verification)
    for recno, _, fv in recs:
        r = {fname: DbfFile.decode_field(ftype, flen, fdec, raw)
             for fname, ftype, flen, fdec, raw in fv}
        if r.get("NOTE") != 0:
            _selftest_fail("TOURS rec%d NOTE should be 0 got %r" % (recno, r["NOTE"]),
                           failures)
        checks += 1

    # ------------------------------------------------------------------
    # TRAVEL.DBF
    # ver=0x83, nrec=49, 11 fields, hlen=385, rlen=137, +1 term, no EOF
    # ------------------------------------------------------------------
    travel_path = os.path.join(pristine, "TRAVEL.DBF")
    db = DbfFile(travel_path)
    check("TRAVEL version",       db.version,       0x83)
    check("TRAVEL record_count",  db.record_count,  49)
    check("TRAVEL field_count",   len(db.fields),   11)
    check("TRAVEL header_length", db.header_length, 385)
    check("TRAVEL record_length", db.record_length, 137)
    check("TRAVEL has_memo",      db.has_memo(),    True)
    check("TRAVEL hlen invariant +1",
          db.header_length, 32 + 32 * len(db.fields) + 1)

    # Spot-check field types: includes D, N, L, M (dbf.md sec 5)
    field_map = {fname: (ftype, flen, fdec) for fname, ftype, flen, fdec in db.fields}
    check("TRAVEL PAID type",  field_map["PAID"][0],  "L")
    check("TRAVEL NOTES type", field_map["NOTES"][0], "M")
    check("TRAVEL NOTES len",  field_map["NOTES"][1], 10)

    # Spot-check rec0 (dbf.md Verification / dbt.md sec 4.1):
    # NOTES -> block 1 -> TRAVEL.DBT memo text starts with '\r\nPaid on'
    recs = list(db.iter_records())
    _, _, fv0 = recs[0]
    r0 = {fname: DbfFile.decode_field(ftype, flen, fdec, raw)
          for fname, ftype, flen, fdec, raw in fv0}
    check("TRAVEL rec0 NOTES block", r0["NOTES"], 1)
    check("TRAVEL rec0 PAID",        r0["PAID"],  True)

    # Verify memo text for rec0 via DbtFile (dbt.md sec 4.1 worked example)
    dbt = DbtFile(os.path.join(pristine, "TRAVEL.DBT"))
    check("TRAVEL.DBT next_free", dbt.next_free, 3)
    memo1 = dbt.read_memo(1)
    # The spec states the block 1 text; check the start (first 20 bytes)
    # dbt.md sec 4.1: "\r\nPaid on 7/15/85    Visa ..."
    check("TRAVEL block1 memo start",
          memo1[:12].decode("latin-1"), "\r\nPaid on 7/")
    # Full text (dbt.md sec 4.1 full listing)
    expected_memo1 = (
        "\r\nPaid on 7/15/85    Visa 9999-111-999-999\r\n\r\n"
        "Name on the Card: Claire M. Buckman\r\n\r\n"
        "Expiration Date: 5/86\r\n\r\n"
        "Approval: JK \r\n\r\n"
    )
    check("TRAVEL block1 memo full",
          memo1.decode("latin-1"), expected_memo1)

    # rec1 -> NOTES block 2 (dbt.md sec 4 / Verification)
    _, _, fv1 = recs[1]
    r1 = {fname: DbfFile.decode_field(ftype, flen, fdec, raw)
          for fname, ftype, flen, fdec, raw in fv1}
    check("TRAVEL rec1 NOTES block", r1["NOTES"], 2)

    # recs 2..48 -> NOTES block 0 (no memo) (dbt.md sec 4 Verification: recs 3-49)
    for recno, _, fv in recs[2:]:
        r = {fname: DbfFile.decode_field(ftype, flen, fdec, raw)
             for fname, ftype, flen, fdec, raw in fv}
        if r.get("NOTES") != 0:
            _selftest_fail(
                "TRAVEL rec%d NOTES should be 0 got %r" % (recno, r["NOTES"]),
                failures)
        checks += 1

    # ------------------------------------------------------------------
    # BANK.DBF (dbf.md sec 8: nrec=0, +2 term, ghost data after EOF)
    # ver=0x03, nrec=0, 5 fields, hlen=194, rlen=24
    # ------------------------------------------------------------------
    bank_path = os.path.join(pristine, "BANK.DBF")
    db = DbfFile(bank_path)
    check("BANK version",       db.version,       0x03)
    check("BANK record_count",  db.record_count,  0)
    check("BANK field_count",   len(db.fields),   5)
    check("BANK header_length", db.header_length, 194)
    check("BANK record_length", db.record_length, 24)
    # +2 terminator: header_length == 32 + 32*5 + 2 = 194
    check("BANK hlen invariant +2",
          db.header_length, 32 + 32 * len(db.fields) + 2)
    # No records -> iter_records yields nothing
    check("BANK iter_records empty", list(db.iter_records()), [])

    # Verify field names/types (dbf.md sec 8 / Verification)
    check("BANK field0 name", db.fields[0][0], "DATE")
    check("BANK field0 type", db.fields[0][1], "D")
    check("BANK field1 name", db.fields[1][0], "AMT")
    check("BANK field1 type", db.fields[1][1], "N")
    check("BANK field4 name", db.fields[4][0], "CLEAR")
    check("BANK field4 type", db.fields[4][1], "L")

    # ------------------------------------------------------------------
    # TAX.DBF (dbf.md sec 8: nrec=9, +2 term, EOF byte present)
    # ver=0x03, nrec=9, 2 fields, hlen=98, rlen=20
    # ------------------------------------------------------------------
    tax_path = os.path.join(pristine, "TAX.DBF")
    db = DbfFile(tax_path)
    check("TAX version",       db.version,       0x03)
    check("TAX record_count",  db.record_count,  9)
    check("TAX field_count",   len(db.fields),   2)
    check("TAX header_length", db.header_length, 98)
    check("TAX record_length", db.record_length, 20)
    check("TAX hlen invariant +2",
          db.header_length, 32 + 32 * len(db.fields) + 2)

    # Verify all 9 TAX records are readable
    tax_recs = list(db.iter_records())
    check("TAX record_count from iter", len(tax_recs), 9)

    # ------------------------------------------------------------------
    # UNIVERSD.DBF (dbf.md sec 8: nrec=1, +1 term, EOF byte present)
    # ver=0x03, nrec=1, 1 field, hlen=65, rlen=35
    # ------------------------------------------------------------------
    universd_path = os.path.join(disk2, "UNIVERSD.DBF")
    db = DbfFile(universd_path)
    check("UNIVERSD version",       db.version,       0x03)
    check("UNIVERSD record_count",  db.record_count,  1)
    check("UNIVERSD field_count",   len(db.fields),   1)
    check("UNIVERSD header_length", db.header_length, 65)
    check("UNIVERSD record_length", db.record_length, 35)
    check("UNIVERSD hlen invariant +1",
          db.header_length, 32 + 32 * len(db.fields) + 1)

    # Field V:C(34) (Verification)
    check("UNIVERSD field0 name", db.fields[0][0], "V")
    check("UNIVERSD field0 type", db.fields[0][1], "C")
    check("UNIVERSD field0 len",  db.fields[0][2], 34)

    # Spot-check rec0 field V
    u_recs = list(db.iter_records())
    _, _, fv0 = u_recs[0]
    r0 = {fname: DbfFile.decode_field(ftype, flen, fdec, raw)
          for fname, ftype, flen, fdec, raw in fv0}
    # Value: 'tijdschriften betapress' (rstripped)
    check("UNIVERSD rec0 V value",
          r0["V"], "tijdschriften betapress")

    # ------------------------------------------------------------------
    # Report
    # ------------------------------------------------------------------
    if failures:
        sys.stderr.write("dbf_ref selftest: %d checks, %d failures\n"
                         % (checks, len(failures)))
        sys.exit(1)
    else:
        sys.stdout.write("dbf_ref selftest: %d checks, 0 failures\n" % checks)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------
# Mode-based dispatch (same pattern as fat12_ref.py) so mode flags like
# --schema, --selftest work naturally without argparse subparser friction.

_USAGE = (
    "usage: %(prog)s --schema  <file.dbf>   print normalized header + field descriptors\n"
    "       %(prog)s --records <file.dbf>   print decoded records\n"
    "       %(prog)s --memo    <file.dbf>   print memo text keyed by recno\n"
    "       %(prog)s --selftest             assert dbf.md values against real goldens\n"
)


def main(argv):
    prog = argv[0]
    if len(argv) < 2:
        sys.stderr.write(_USAGE % {"prog": prog})
        return 2

    mode = argv[1]

    if mode == "--selftest":
        if len(argv) != 2:
            sys.stderr.write("dbf_ref: --selftest takes no arguments\n")
            return 2
        run_selftest()
        return 0

    if mode == "--schema":
        if len(argv) != 3:
            sys.stderr.write("dbf_ref: --schema needs a FILE.DBF argument\n")
            return 2
        try:
            cmd_schema(argv[2], sys.stdout)
        except (ValueError, OSError) as exc:
            sys.stderr.write("dbf_ref: error: %s\n" % exc)
            return 1
        return 0

    if mode == "--records":
        if len(argv) != 3:
            sys.stderr.write("dbf_ref: --records needs a FILE.DBF argument\n")
            return 2
        try:
            cmd_records(argv[2], sys.stdout)
        except (ValueError, OSError) as exc:
            sys.stderr.write("dbf_ref: error: %s\n" % exc)
            return 1
        return 0

    if mode == "--memo":
        if len(argv) != 3:
            sys.stderr.write("dbf_ref: --memo needs a FILE.DBF argument\n")
            return 2
        try:
            cmd_memo(argv[2], sys.stdout)
        except (ValueError, OSError) as exc:
            sys.stderr.write("dbf_ref: error: %s\n" % exc)
            return 1
        return 0

    sys.stderr.write("dbf_ref: unknown mode '%s'\n" % mode)
    sys.stderr.write(_USAGE % {"prog": prog})
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
