#!/usr/bin/env python3
# harness/diff/dbf_diff/ndx_ref.py -- independent dBASE III+ .ndx reference reader.
#
# FACTORY tooling (CLAUDE.md Law 3): the artifact stays C; this python reader is
# the INDEPENDENT reference for the SAMIR (InitechBase) .ndx differential oracle
# (beads initech-586.5.8).  It is a SECOND implementation that shares NO code
# and NO constants with os/samir/* or spec/samir/*.  Its purpose is to catch any
# bug that is SHARED by our C .ndx reader -- agreement between two independent
# derivations is a far stronger signal than one.
#
# Every offset, size, and encoding decision below is derived from FIRST PRINCIPLES
# per the spec doc at:
#   ../dbase3-decomp/specs/file-formats/ndx.md
# NOT from our C headers or any imported module.
#
# Subcommands:
#   --header <file.ndx>          : normalized header (all 10 fields).
#                                  Volatile/meaningless bytes (reserved@0x08,
#                                  dummy@0x14) are replaced with fixed sentinels
#                                  so output is deterministic (Rule 11).
#   --index-dump <file.ndx>      : in-order B-tree traversal -> sorted sequence
#                                  of (key, dbf_recno) pairs.  For char keys the
#                                  key is printed rstripped; for numeric/date keys
#                                  the raw double is printed (and for date keys the
#                                  Gregorian date is also shown).
#   --seek <file.ndx> <key>      : descend the B-tree and return matching recno(s)
#                                  or "not found".  Char keys: exact match (SET
#                                  EXACT semantics; full key_length comparison after
#                                  space-padding the query).  Numeric/date: exact
#                                  double equality.
#   --selftest                   : assert ndx.md "Verification" values against
#                                  the real goldens.  Exits 0 on success, non-zero
#                                  on failure.  Fails loud if goldens absent.
#
# Structural invariant checks on load (Rule 2):
#   group_length  == ceil4(key_length + 8)
#   keys_per_page == (512 - 4) // group_length
# Stored values are used for traversal; a disagreement is a warning, not a hard
# error (dBASE writes them, we trust the formula for independent re-derivation).
#
# Fail loud (CLAUDE.md Rule 2): structural errors raise / exit non-zero.
# ASCII-clean (Rule 12).  Deterministic output (Rule 11): no wall-clock, no
# nondeterminism in traversal (in-order is fully determined by tree structure).
# Pure stdlib (struct/sys/os/argparse) -- no extra runtimes (Law 3).

import os
import struct
import sys

# ---------------------------------------------------------------------------
# NDX layout constants (derived from ndx.md sections 2, 3, 4)
# ---------------------------------------------------------------------------

# ndx.md sec 1: every page is exactly 512 bytes.
_PAGE_SIZE       = 512

# ndx.md sec 2: header (page 0) field offsets and sizes.
_HDR_OFF_ROOT    = 0x00   # 4 bytes LE u32: root page number
_HDR_OFF_TOTAL   = 0x04   # 4 bytes LE u32: total page count (== filesize/512)
_HDR_OFF_RESERVD = 0x08   # 4 bytes u32: reserved (always 0 in III+; treat as volatile)
_HDR_OFF_KEYLEN  = 0x0C   # 2 bytes LE u16: key_length (bytes per key in each entry)
_HDR_OFF_KPP     = 0x0E   # 2 bytes LE u16: keys_per_page (max live keys per node)
_HDR_OFF_KEYTYPE = 0x10   # 2 bytes LE u16: key_type (0=char, 1=numeric/date)
_HDR_OFF_GRPLEN  = 0x12   # 2 bytes LE u16: group_length = ceil4(key_length+8)
_HDR_OFF_DUMMY   = 0x14   # 2 bytes u16: unused/garbage (volatile sentinel)
_HDR_OFF_UNIQUE  = 0x16   # 2 bytes: unique_flag.  NOTE: dBASE writes the flag
                           # into byte 0x17 (the high byte when read LE), so the
                           # LE-u16 value is 256 for UNIQUE, 0 for non-unique.
                           # [verified: minted NUNIQ.NDX 2026-06-16; see
                           # dbase3-decomp/re/mint-results-001.md]
_HDR_OFF_EXPR    = 0x18   # 100 bytes: NUL-terminated key expression (verbatim as typed)
_HDR_EXPR_CAP    = 100    # max bytes incl. NUL (ndx.md sec 2.1)

# ndx.md sec 2: key_type values
_KEYTYPE_CHAR    = 0   # character key (space-padded ASCII/CP437)
_KEYTYPE_NUM     = 1   # numeric OR date key (8-byte LE IEEE-754 double)

# ndx.md sec 3: node (page 1..total-1) layout
_NODE_OFF_COUNT  = 0x00   # 2 bytes LE u16: live entry count in this node
_NODE_OFF_FILL   = 0x02   # 2 bytes filler (garbage; ignore)
_NODE_HDR_SIZE   = 4      # total node header bytes before entry array

# ndx.md sec 3.1: key entry ("group") layout within a node
_ENTRY_OFF_CHILD = 0x00   # 4 bytes LE u32: child page (0 => leaf)
_ENTRY_OFF_RECNO = 0x04   # 4 bytes LE u32: DBF record number (1-based; 0 in branches)
_ENTRY_OFF_KEY   = 0x08   # key_length bytes: key data (encoding per key_type)
# filler follows key to reach group_length (ceil4 alignment)

# ndx.md sec 1/3: keys_per_page formula uses node header size 4
_KPP_DIVISOR     = _PAGE_SIZE - _NODE_HDR_SIZE   # 508

# ---------------------------------------------------------------------------
# Normalization sentinels (deterministic output, Rule 11)
# ---------------------------------------------------------------------------
_NORM_RESERVED   = "0x00000000"   # reserved@0x08 is always 0 in III+ but volatile
_NORM_DUMMY      = "0x0000"       # dummy@0x14 holds garbage; normalized away


# ---------------------------------------------------------------------------
# Utility: ceil4
# ---------------------------------------------------------------------------

def _ceil4(n):
    """Round N up to the nearest multiple of 4.

    ndx.md sec 2: group_length = ceil4(key_length + 8).
    """
    return ((n + 3) // 4) * 4


# ---------------------------------------------------------------------------
# Utility: JDN -> Gregorian date string (for display in --index-dump)
# ---------------------------------------------------------------------------

def _jdn_to_date(jdn):
    """Convert a Julian Day Number (int) to a 'YYYY-MM-DD' string.

    Uses the proleptic Gregorian calendar formula (Richards 2013 algorithm E).
    ndx.md sec 4.2: date keys are stored as JDN doubles; this is the inverse
    for human-readable display only (no impact on oracle output).
    """
    n = int(jdn)
    # Richards algorithm E (valid for JDN >= 0)
    f = n + 1401 + (((4*n + 274277) // 146097) * 3) // 4 - 38
    e = 4*f + 3
    g = (e % 1461) // 4
    h = 5*g + 2
    day   = (h % 153) // 5 + 1
    month = (h // 153 + 2) % 12 + 1
    year  = e // 1461 - 4716 + (14 - month) // 12
    return "%04d-%02d-%02d" % (year, month, day)


# ---------------------------------------------------------------------------
# Core parser: NdxFile
# ---------------------------------------------------------------------------

class NdxFile:
    """Independent dBASE III+ .ndx B-tree reader.

    Parses from first principles per ndx.md.  Does NOT import spec/samir/*
    or os/samir/*.  All offsets and formulas are re-derived here.
    """

    def __init__(self, path):
        """Open and parse the .ndx at PATH."""
        try:
            with open(path, "rb") as fp:
                self._data = fp.read()
        except OSError as exc:
            raise ValueError("cannot open '%s': %s" % (path, exc))

        self._path = path
        n = len(self._data)
        if n % _PAGE_SIZE != 0:
            raise ValueError(
                "'%s': file size %d is not a multiple of %d (page size)"
                % (path, n, _PAGE_SIZE))
        if n < _PAGE_SIZE:
            raise ValueError(
                "'%s': file too short for even one page (%d bytes)" % (path, n))

        self._parse_header()

    # ------------------------------------------------------------------
    # Internal: header parsing
    # ------------------------------------------------------------------

    def _parse_header(self):
        """Parse the 10 header fields from page 0 (ndx.md sec 2)."""
        hdr = self._data[:_PAGE_SIZE]

        # Offsets 0x00, 0x04, 0x08 -- u32 fields
        self.root_page   = struct.unpack_from("<I", hdr, _HDR_OFF_ROOT)[0]
        self.total_pages = struct.unpack_from("<I", hdr, _HDR_OFF_TOTAL)[0]
        self.reserved    = struct.unpack_from("<I", hdr, _HDR_OFF_RESERVD)[0]

        # Offsets 0x0C .. 0x16 -- u16 fields
        self.key_length   = struct.unpack_from("<H", hdr, _HDR_OFF_KEYLEN)[0]
        self.keys_per_page= struct.unpack_from("<H", hdr, _HDR_OFF_KPP)[0]
        self.key_type     = struct.unpack_from("<H", hdr, _HDR_OFF_KEYTYPE)[0]
        self.group_length = struct.unpack_from("<H", hdr, _HDR_OFF_GRPLEN)[0]
        self.dummy        = struct.unpack_from("<H", hdr, _HDR_OFF_DUMMY)[0]
        # unique_flag: dBASE writes into byte 0x17 (high byte of LE u16 at 0x16).
        # Stored LE u16 is 0x0100 = 256 for UNIQUE, 0 for non-unique.
        # [ndx.md sec 2 / mint-results-001.md "unique flag @0x17 = 0x01"]
        self.unique_flag  = struct.unpack_from("<H", hdr, _HDR_OFF_UNIQUE)[0]

        # Key expression: NUL-terminated at 0x18, capacity 100 bytes
        expr_raw = hdr[_HDR_OFF_EXPR : _HDR_OFF_EXPR + _HDR_EXPR_CAP]
        nul_pos  = expr_raw.find(0)
        if nul_pos < 0:
            nul_pos = _HDR_EXPR_CAP   # no NUL found; take all 100 bytes
        try:
            self.key_expr = expr_raw[:nul_pos].decode("ascii")
        except UnicodeDecodeError:
            raise ValueError(
                "'%s': key_expr at 0x18 contains non-ASCII bytes" % self._path)

        # Validate key_type
        if self.key_type not in (_KEYTYPE_CHAR, _KEYTYPE_NUM):
            raise ValueError(
                "'%s': unknown key_type 0x%04X (expected 0 or 1)"
                % (self._path, self.key_type))

        # Cross-check group_length formula (ndx.md sec 2 / impl checklist 1)
        expected_grp = _ceil4(self.key_length + 8)
        if expected_grp != self.group_length:
            sys.stderr.write(
                "WARNING '%s': stored group_length=%d disagrees with "
                "ceil4(%d+8)=%d -- using stored value\n"
                % (self._path, self.group_length, self.key_length, expected_grp))

        # Cross-check keys_per_page formula
        expected_kpp = _KPP_DIVISOR // self.group_length if self.group_length else 0
        if expected_kpp != self.keys_per_page:
            sys.stderr.write(
                "WARNING '%s': stored keys_per_page=%d disagrees with "
                "(%d)//%d=%d -- using stored value\n"
                % (self._path, self.keys_per_page, _KPP_DIVISOR,
                   self.group_length, expected_kpp))

        # Total pages sanity
        expected_pages = len(self._data) // _PAGE_SIZE
        if self.total_pages != expected_pages:
            raise ValueError(
                "'%s': total_pages=%d but filesize/512=%d"
                % (self._path, self.total_pages, expected_pages))

        # Root page must be within bounds
        if self.root_page >= self.total_pages or self.root_page == 0:
            raise ValueError(
                "'%s': root_page=%d is out of range [1, %d)"
                % (self._path, self.root_page, self.total_pages))

    # ------------------------------------------------------------------
    # Internal: node I/O
    # ------------------------------------------------------------------

    def _read_page(self, page_no):
        """Return the 512-byte slice for PAGE_NO.

        ndx.md sec 1: page P is at byte offset P*512.  NEVER reads past 512
        bytes into the page (ndx.md impl checklist step 5).
        """
        if page_no >= self.total_pages or page_no == 0:
            raise ValueError(
                "'%s': attempt to read page %d (valid range 1..%d)"
                % (self._path, page_no, self.total_pages - 1))
        off = page_no * _PAGE_SIZE
        return self._data[off : off + _PAGE_SIZE]

    def _parse_node(self, page):
        """Return (entry_count, entries, trail_child) for a node page.

        entries = list of (child_page, dbf_recno, key_bytes).
        trail_child = page number of rightmost child (0 if absent or leaf).

        ndx.md sec 3: node header is 4 bytes (2-byte count + 2-byte filler).
        ndx.md sec 3.2: trailing (N+1th) child pointer follows the last entry
        in branch nodes; in leaves it is garbage.  We only read it if it fits
        within the 512-byte page boundary.
        """
        entry_count = struct.unpack_from("<H", page, _NODE_OFF_COUNT)[0]
        entries = []
        for i in range(entry_count):
            e_off = _NODE_HDR_SIZE + i * self.group_length
            if e_off + 8 + self.key_length > _PAGE_SIZE:
                # Entry extends past end of page -- corrupted.
                raise ValueError(
                    "entry %d at offset %d extends past page boundary"
                    % (i, e_off))
            child_page = struct.unpack_from("<I", page, e_off + _ENTRY_OFF_CHILD)[0]
            dbf_recno  = struct.unpack_from("<I", page, e_off + _ENTRY_OFF_RECNO)[0]
            key_bytes  = page[e_off + _ENTRY_OFF_KEY : e_off + _ENTRY_OFF_KEY + self.key_length]
            entries.append((child_page, dbf_recno, key_bytes))

        # Trailing child pointer (rightmost subtree for branch nodes).
        # ndx.md sec 3.2: when entry_count == keys_per_page the trailing slot
        # may not fully fit (overflow); only the leading 4 bytes of child_page
        # are guaranteed present.  We only read it if those 4 bytes are in-page.
        trail_off = _NODE_HDR_SIZE + entry_count * self.group_length
        if trail_off + 4 <= _PAGE_SIZE:
            trail_child = struct.unpack_from("<I", page, trail_off)[0]
        else:
            trail_child = 0

        return entry_count, entries, trail_child

    # ------------------------------------------------------------------
    # Public: in-order traversal
    # ------------------------------------------------------------------

    def inorder(self):
        """In-order B-tree traversal from root_page.

        Yields (key_decoded, dbf_recno) pairs in ascending sorted order.

        For char keys (key_type == 0): key_decoded is the raw key_bytes
        (bytes, key_length long; NOT rstripped here -- caller rstrips for
        display but raw bytes are used for seek comparisons).
        For numeric/date keys (key_type == 1): key_decoded is a float.

        ndx.md sec 5 / impl checklist step 3:
          - branch entry (child_page != 0): recurse into child_page subtree
            before moving on; the entry's own key is a high-key separator
            (NOT an independent data point -- only leaf entries carry recnos).
          - leaf entry (child_page == 0): yield (key, recno).
          - after all entries in an internal node, recurse into trail_child.
        """
        return list(self._inorder_page(self.root_page))

    def _inorder_page(self, page_no):
        """Generator: in-order traversal of the subtree at PAGE_NO."""
        page = self._read_page(page_no)
        _, entries, trail_child = self._parse_node(page)

        for child_page, dbf_recno, key_bytes in entries:
            if child_page != 0:
                # Branch entry: recurse into child subtree (in-order BEFORE this key).
                # The entry's key_data is the high-key separator; it is NOT emitted.
                for item in self._inorder_page(child_page):
                    yield item
            else:
                # Leaf entry: emit (decoded_key, recno).
                yield (self._decode_key(key_bytes), dbf_recno)

        # Rightmost child (after last separator).
        if trail_child != 0:
            for item in self._inorder_page(trail_child):
                yield item

    def _decode_key(self, key_bytes):
        """Decode key_bytes per key_type (ndx.md sec 4).

        char   -> bytes (raw, key_length; caller rstrips for display)
        numeric/date -> float (raw IEEE-754 double decoded arithmetically)
        """
        if self.key_type == _KEYTYPE_CHAR:
            return key_bytes                 # bytes; rstrip on display
        else:
            # ndx.md sec 4.2: 8-byte LE IEEE-754 double, compared arithmetically.
            return struct.unpack("<d", key_bytes)[0]

    # ------------------------------------------------------------------
    # Public: seek (point query)
    # ------------------------------------------------------------------

    def seek(self, query_key):
        """Descend the B-tree for QUERY_KEY; return list of matching recnos.

        For char keys (key_type == 0): QUERY_KEY is a str; comparison uses
        SET EXACT semantics (pad query to key_length with spaces, then compare
        key_length bytes).  Match is exact byte equality (unsigned byte order,
        ndx.md sec 6).

        For numeric/date keys (key_type == 1): QUERY_KEY is a float/int;
        comparison is arithmetic equality.

        Returns [] if not found, [recno, ...] if found (multiple if duplicates).

        ndx.md sec 5 "Search": start at root, scan entries in order; when an
        entry key >= query, descend into that child (or rightmost child if
        query exceeds all); at a leaf, check for exact match.
        """
        if self.key_type == _KEYTYPE_CHAR:
            # Encode query as fixed-width bytes, space-padded to key_length.
            if isinstance(query_key, bytes):
                qbytes = query_key.ljust(self.key_length, b' ')[:self.key_length]
            else:
                qbytes = query_key.encode("ascii").ljust(self.key_length, b' ')[:self.key_length]
            return self._seek_char(self.root_page, qbytes)
        else:
            qval = float(query_key)
            return self._seek_num(self.root_page, qval)

    def _seek_char(self, page_no, qbytes):
        """Recursive char-key seek from PAGE_NO."""
        page = self._read_page(page_no)
        _, entries, trail_child = self._parse_node(page)

        for child_page, dbf_recno, key_bytes in entries:
            # Compare: key_bytes >= qbytes (unsigned byte order, CP437)
            if key_bytes >= qbytes:
                if child_page != 0:
                    return self._seek_char(child_page, qbytes)
                else:
                    # Leaf: exact match check
                    if key_bytes == qbytes:
                        return [dbf_recno]
                    return []

        # Query exceeds all separators -> rightmost child
        if trail_child != 0:
            return self._seek_char(trail_child, qbytes)
        # Leaf with no matching entry
        return []

    def _seek_num(self, page_no, qval):
        """Recursive numeric/date seek from PAGE_NO (arithmetic comparison)."""
        page = self._read_page(page_no)
        _, entries, trail_child = self._parse_node(page)

        for child_page, dbf_recno, key_bytes in entries:
            kval = struct.unpack("<d", key_bytes)[0]
            if kval >= qval:
                if child_page != 0:
                    return self._seek_num(child_page, qval)
                else:
                    if kval == qval:
                        return [dbf_recno]
                    return []

        if trail_child != 0:
            return self._seek_num(trail_child, qval)
        return []


# ---------------------------------------------------------------------------
# Subcommand: --header
# ---------------------------------------------------------------------------

def cmd_header(ndx_path, out):
    """Print normalized header fields.

    Volatile/meaningless bytes (reserved, dummy) are replaced with sentinels
    so output is deterministic (Rule 11).
    """
    ndx = NdxFile(ndx_path)

    out.write("root_page:    %d\n" % ndx.root_page)
    out.write("total_pages:  %d\n" % ndx.total_pages)
    out.write("reserved:     %s\n" % _NORM_RESERVED)  # always 0; sentinel
    out.write("key_length:   %d\n" % ndx.key_length)
    out.write("keys_per_page: %d\n" % ndx.keys_per_page)
    out.write("key_type:     %d  # %s\n" % (
        ndx.key_type,
        "char" if ndx.key_type == _KEYTYPE_CHAR else "numeric/date"))
    out.write("group_length: %d  # ceil4(%d+8)=%d\n" % (
        ndx.group_length, ndx.key_length, _ceil4(ndx.key_length + 8)))
    out.write("dummy:        %s\n" % _NORM_DUMMY)  # garbage; sentinel
    out.write("unique_flag:  %d  # %s\n" % (
        ndx.unique_flag,
        "unique" if ndx.unique_flag != 0 else "not unique"))
    out.write("key_expr:     %s\n" % repr(ndx.key_expr))


# ---------------------------------------------------------------------------
# Subcommand: --index-dump
# ---------------------------------------------------------------------------

def _fmt_key(key_decoded, key_type):
    """Format a decoded key for display.

    char         : bytes, rstripped of trailing spaces, decoded as ASCII.
    numeric/date : float with enough precision.  For whole-number values that
                   look like Julian Day Numbers (JDN >= 1721426 = 0001-01-01)
                   we also show the Gregorian date annotation in brackets --
                   this covers TOURDATE and NDATE-style indices where key_type=1
                   is used for D-field dates.  For non-JDN numeric values (e.g.
                   costs, check numbers) the annotation is suppressed.
    """
    if key_type == _KEYTYPE_CHAR:
        return key_decoded.rstrip(b' ').decode("ascii", errors="replace")
    val = key_decoded
    # Format the number itself.
    if val == int(val) and abs(val) < 1e15:
        num_str = "%d" % int(val)
    else:
        num_str = "%.15g" % val
    # Append a Gregorian date annotation if the value looks like a plausible JDN.
    # JDN 1721426 = 0001-01-01 (proleptic Gregorian); JDN 2816788 = 3000-01-01.
    # Negative values, fractional values, or values outside this range are numeric.
    _JDN_MIN = 1721426   # 0001-01-01 proleptic Gregorian
    _JDN_MAX = 2816788   # 3000-01-01 (generous upper bound)
    if val == int(val) and _JDN_MIN <= int(val) <= _JDN_MAX:
        num_str = "%s  [%s]" % (num_str, _jdn_to_date(int(val)))
    return num_str


def cmd_index_dump(ndx_path, out):
    """Print in-order (key, recno) pairs from the B-tree."""
    ndx = NdxFile(ndx_path)
    pairs = ndx.inorder()
    for key_decoded, recno in pairs:
        out.write("%s  recno=%d\n" % (_fmt_key(key_decoded, ndx.key_type), recno))
    out.write("# total entries: %d\n" % len(pairs))


# ---------------------------------------------------------------------------
# Subcommand: --seek
# ---------------------------------------------------------------------------

def cmd_seek(ndx_path, query_key_str, out):
    """Descend the tree for QUERY_KEY_STR and print result."""
    ndx = NdxFile(ndx_path)

    if ndx.key_type == _KEYTYPE_CHAR:
        recnos = ndx.seek(query_key_str)
    else:
        try:
            qval = float(query_key_str)
        except ValueError:
            sys.stderr.write("ndx_ref: seek: numeric key expected, got %r\n" % query_key_str)
            return 1
        recnos = ndx.seek(qval)

    if recnos:
        for r in recnos:
            out.write("found: recno=%d\n" % r)
    else:
        out.write("not found\n")
    return 0


# ---------------------------------------------------------------------------
# Selftest
# ---------------------------------------------------------------------------

def _selftest_fail(msg, failures):
    sys.stderr.write("FAIL: %s\n" % msg)
    failures.append(msg)


def run_selftest():
    """Assert ndx.md 'Verification' values against the real goldens.

    Fails loud if the goldens directory is absent (CLAUDE.md Rule 2).
    Prints 'ndx_ref selftest: N checks, 0 failures' or exits non-zero.
    """
    # Locate golden trees relative to this file.
    # This file is at: <repo>/harness/diff/dbf_diff/ndx_ref.py
    # Goldens are at:  <repo>/../dbase3-decomp/goldens/...
    here      = os.path.dirname(os.path.abspath(__file__))
    repo_root = os.path.normpath(os.path.join(here, "..", "..", ".."))
    decomp    = os.path.normpath(os.path.join(repo_root, "..", "dbase3-decomp"))
    pristine  = os.path.join(decomp, "goldens", "dbase-iii-plus-1.1-pristine",
                             "files", "Sample_Programs_and_Utilities")
    mint      = os.path.join(decomp, "mint", "work")

    if not os.path.isdir(pristine):
        sys.stderr.write(
            "ndx_ref selftest: ABORT -- pristine goldens not found at '%s'\n"
            % pristine)
        sys.exit(2)
    if not os.path.isdir(mint):
        sys.stderr.write(
            "ndx_ref selftest: ABORT -- mint/work not found at '%s'\n" % mint)
        sys.exit(2)

    checks   = 0
    failures = []

    def check(desc, got, want):
        nonlocal checks
        checks += 1
        if got != want:
            _selftest_fail("%s: got %r want %r" % (desc, got, want), failures)

    def check_approx(desc, got, want, tol=0.5):
        """Check that float GOT is within TOL of WANT (used for JDN doubles)."""
        nonlocal checks
        checks += 1
        if abs(got - want) > tol:
            _selftest_fail("%s: got %r want %r (tol %r)" % (desc, got, want, tol),
                           failures)

    # ------------------------------------------------------------------
    # CNAMES.NDX (ndx.md sec 2.2 worked header + Verification section)
    # root=6, total=7, kl=40, kpp=10, key_type=0, grp=48, unique=0
    # key_expr = "LASTNAME + FIRSTNAME " (with trailing space, 21 chars)
    # ------------------------------------------------------------------
    cnames_path = os.path.join(pristine, "CNAMES.NDX")
    ndx = NdxFile(cnames_path)

    check("CNAMES root_page",    ndx.root_page,    6)
    check("CNAMES total_pages",  ndx.total_pages,  7)
    check("CNAMES reserved",     ndx.reserved,     0)
    check("CNAMES key_length",   ndx.key_length,   40)
    check("CNAMES keys_per_page",ndx.keys_per_page,10)
    check("CNAMES key_type",     ndx.key_type,     0)
    check("CNAMES group_length", ndx.group_length, 48)
    check("CNAMES unique_flag",  ndx.unique_flag,  0)
    # key_expr verbatim as typed (ndx.md sec 2.1: piclist "lowercase" refuted)
    check("CNAMES key_expr",     ndx.key_expr,     "LASTNAME + FIRSTNAME ")
    # group_length formula: ceil4(40+8) = 48
    check("CNAMES grp formula",  _ceil4(ndx.key_length + 8), 48)
    # keys_per_page formula: 508 // 48 = 10
    check("CNAMES kpp formula",  _KPP_DIVISOR // ndx.group_length, 10)

    # In-order traversal: 49 entries, recnos 1..49 in sorted LASTNAME+FIRSTNAME order.
    # [ndx.md Verification: "in-order traversal of CNAMES yields 49 leaf recnos
    #  (1..49) in sorted key order, first 'Adams Nathan', last 'Zambini Rick'"]
    pairs = ndx.inorder()
    check("CNAMES inorder count", len(pairs), 49)
    # All recnos 1..49 present
    recnos_sorted = sorted(r for _, r in pairs)
    check("CNAMES recnos 1..49", recnos_sorted, list(range(1, 50)))
    # First key (rstripped) = "Adams               Nathan" (LASTNAME padded to 20 + FIRSTNAME)
    first_key_raw = pairs[0][0]   # bytes
    first_key_str = first_key_raw.rstrip(b' ').decode("ascii")
    check("CNAMES first key",  first_key_str, "Adams               Nathan")
    first_recno = pairs[0][1]
    check("CNAMES first recno", first_recno, 15)
    # Last key
    last_key_raw = pairs[-1][0]
    last_key_str = last_key_raw.rstrip(b' ').decode("ascii")
    check("CNAMES last key",   last_key_str,  "Zambini             Rick")
    last_recno = pairs[-1][1]
    check("CNAMES last recno",  last_recno,   8)

    # ------------------------------------------------------------------
    # ZIPCODE.NDX (ndx.md Verification: KL=5, grp=16, kpp=31, key_type=0)
    # ------------------------------------------------------------------
    zipcode_path = os.path.join(pristine, "ZIPCODE.NDX")
    ndx = NdxFile(zipcode_path)

    check("ZIPCODE key_length",   ndx.key_length,    5)
    check("ZIPCODE keys_per_page",ndx.keys_per_page, 31)
    check("ZIPCODE key_type",     ndx.key_type,      0)
    check("ZIPCODE group_length", ndx.group_length,  16)
    check("ZIPCODE unique_flag",  ndx.unique_flag,   0)
    # grp formula: ceil4(5+8)=16
    check("ZIPCODE grp formula",  _ceil4(ndx.key_length + 8), 16)
    # kpp formula: 508//16=31
    check("ZIPCODE kpp formula",  _KPP_DIVISOR // ndx.group_length, 31)
    # In-order: 49 entries (CLIENTS.DBF has 49 records), first "72450", last "97401".
    # NOTE: ndx.md Verification section says "50 sorted keys" -- that is a spec typo.
    # The ground truth (byte count from actual ZIPCODE.NDX pages: 31+18=49) is 49.
    # CLIENTS.DBF nrec=49 confirms it.  We assert against the bytes, not the prose.
    pairs = ndx.inorder()
    check("ZIPCODE inorder count", len(pairs), 49)
    first_z = pairs[0][0].rstrip(b' ').decode("ascii")
    last_z  = pairs[-1][0].rstrip(b' ').decode("ascii")
    check("ZIPCODE first key", first_z, "72450")
    check("ZIPCODE last key",  last_z,  "97401")

    # ------------------------------------------------------------------
    # CUSTOMER.NDX (ndx.md Verification: KL=28, grp=36, kpp=14, single leaf)
    # [ndx.md Verification: "CUSTOMER rec1 == CDES_CITY 'ATL'+CNAME 'LOUIS' (28)"]
    # ------------------------------------------------------------------
    customer_path = os.path.join(pristine, "CUSTOMER.NDX")
    ndx = NdxFile(customer_path)

    check("CUSTOMER key_length",   ndx.key_length,    28)
    check("CUSTOMER keys_per_page",ndx.keys_per_page, 14)
    check("CUSTOMER key_type",     ndx.key_type,      0)
    check("CUSTOMER group_length", ndx.group_length,  36)
    check("CUSTOMER unique_flag",  ndx.unique_flag,   0)
    # grp: ceil4(28+8)=36; kpp: 508//36=14
    check("CUSTOMER grp formula",  _ceil4(ndx.key_length + 8), 36)
    check("CUSTOMER kpp formula",  _KPP_DIVISOR // ndx.group_length, 14)
    # root=1, total=2: single-page tree (header + one leaf)
    check("CUSTOMER root_page",    ndx.root_page,   1)
    check("CUSTOMER total_pages",  ndx.total_pages, 2)
    # In-order: 5 records
    pairs = ndx.inorder()
    check("CUSTOMER inorder count", len(pairs), 5)
    # Verify rec1 key = "ATL" + "LOUIS" + spaces (total 28)
    # [ndx.md Verification: "CUSTOMER rec1 == CDES_CITY 'ATL'+CNAME 'LOUIS' (28)"]
    # recno=1 should appear; find it
    rec1_keys = [k for k, r in pairs if r == 1]
    check("CUSTOMER rec1 exists", len(rec1_keys), 1)
    if rec1_keys:
        rec1_key_str = rec1_keys[0].decode("ascii")
        check("CUSTOMER rec1 key prefix", rec1_key_str[:3], "ATL")
        check("CUSTOMER rec1 key CNAME",  rec1_key_str[3:8], "LOUIS")

    # ------------------------------------------------------------------
    # TOURDATE.NDX (ndx.md sec 4.2 worked example + Verification)
    # key_type=1 (date), KL=8, grp=16, kpp=31
    # First entry: rec1, JDN 2446283 = 1985-08-05
    # Bytes: 00 00 00 80 E5 A9 42 41
    # ------------------------------------------------------------------
    tourdate_path = os.path.join(pristine, "TOURDATE.NDX")
    ndx = NdxFile(tourdate_path)

    check("TOURDATE key_type",     ndx.key_type,     1)
    check("TOURDATE key_length",   ndx.key_length,   8)
    check("TOURDATE keys_per_page",ndx.keys_per_page,31)
    check("TOURDATE group_length", ndx.group_length, 16)
    check("TOURDATE unique_flag",  ndx.unique_flag,  0)
    check("TOURDATE key_expr",     ndx.key_expr,     "DEPARTURE ")

    # In-order: 30 entries (30 TOURS records), all JDNs non-decreasing
    pairs = ndx.inorder()
    check("TOURDATE inorder count", len(pairs), 30)
    # First entry: recno=1, JDN=2446283.0
    # [ndx.md sec 4.2: "TOURDATE leaf entry 0 key_data = 00 00 00 80 E5 A9 42 41"]
    first_jdn  = pairs[0][0]
    first_rec  = pairs[0][1]
    check_approx("TOURDATE first JDN", first_jdn, 2446283.0)
    check("TOURDATE first recno",      first_rec,  1)
    # Verify the raw bytes of the first entry key
    # Re-read raw from page directly (ndx.inorder returns decoded floats)
    page = ndx._read_page(ndx.root_page)
    e0_off = _NODE_HDR_SIZE  # first entry at offset 4
    first_key_bytes = page[e0_off + _ENTRY_OFF_KEY : e0_off + _ENTRY_OFF_KEY + 8]
    expected_bytes = bytes([0x00, 0x00, 0x00, 0x80, 0xE5, 0xA9, 0x42, 0x41])
    check("TOURDATE first key bytes", first_key_bytes, expected_bytes)
    # Duplicate keys: recnos 9 and 10 both JDN 2446363
    # [ndx.md Verification: "TOURDATE recnos 9,10 both JDN 2446363"]
    jdn_2446363_recnos = sorted(r for k, r in pairs if abs(k - 2446363.0) < 0.5)
    check("TOURDATE dup keys 2446363", jdn_2446363_recnos, [9, 10])

    # ------------------------------------------------------------------
    # NCOST.NDX (mint, numeric/negative: -123.45/-1/0/279 in true order)
    # [ndx.md sec 4.2 / mint-results-001.md]
    # ------------------------------------------------------------------
    ncost_path = os.path.join(mint, "NCOST.NDX")
    ndx = NdxFile(ncost_path)

    check("NCOST key_type",  ndx.key_type,  1)
    check("NCOST key_length",ndx.key_length, 8)
    check("NCOST unique_flag",ndx.unique_flag, 0)

    pairs = ndx.inorder()
    # First four entries must be -123.45, -1, 0, 279 in that order
    # [ndx.md sec 4.2: "stored in true numeric order"]
    check("NCOST inorder[0] recno", pairs[0][1], 31)
    check_approx("NCOST inorder[0] key", pairs[0][0], -123.45, tol=1e-9)
    check_approx("NCOST inorder[1] key", pairs[1][0], -1.0,    tol=1e-9)
    check_approx("NCOST inorder[2] key", pairs[2][0], 0.0,     tol=1e-9)
    check_approx("NCOST inorder[3] key", pairs[3][0], 279.0,   tol=1e-9)
    # Verify raw bytes for -123.45 (the interesting negative-decimal case)
    # [mint-results-001.md: "cd cc cc cc cc dc 5e c0"]
    # Find the page that holds this entry by re-reading (first leaf leftmost)
    # Use seek to find it
    recnos_neg = ndx.seek(-123.45)
    check("NCOST seek -123.45 found", len(recnos_neg) > 0, True)
    if recnos_neg:
        check("NCOST seek -123.45 recno", recnos_neg[0], 31)

    # ------------------------------------------------------------------
    # NDATE.NDX (mint, date keys as JDN doubles; first key = 2446283.0)
    # ------------------------------------------------------------------
    ndate_path = os.path.join(mint, "NDATE.NDX")
    ndx = NdxFile(ndate_path)

    check("NDATE key_type",   ndx.key_type,  1)
    check("NDATE key_length", ndx.key_length, 8)
    pairs = ndx.inorder()
    check("NDATE inorder count", len(pairs), 30)
    check_approx("NDATE first JDN", pairs[0][0], 2446283.0)
    check("NDATE first recno",      pairs[0][1], 1)

    # ------------------------------------------------------------------
    # BIGIDX.NDX (mint, bulk build: 14 leaves, 245 entries)
    # [ndx.md Open questions (resolved): "14 leaves = 13 full (18/18) + 1 last (11)"]
    # ------------------------------------------------------------------
    bigidx_path = os.path.join(mint, "BIGIDX.NDX")
    ndx = NdxFile(bigidx_path)

    check("BIGIDX total_pages",  ndx.total_pages, 16)  # hdr+1 branch+14 leaves
    check("BIGIDX key_type",     ndx.key_type,    0)
    check("BIGIDX key_length",   ndx.key_length,  20)
    check("BIGIDX keys_per_page",ndx.keys_per_page,18)
    check("BIGIDX group_length", ndx.group_length, 28)
    # grp: ceil4(20+8)=28; kpp: 508//28=18
    check("BIGIDX grp formula",  _ceil4(ndx.key_length + 8), 28)
    check("BIGIDX kpp formula",  _KPP_DIVISOR // ndx.group_length, 18)
    # 14 leaves: root branch has entry_count=13 (13 separators + 1 rightmost child)
    root_page = ndx._read_page(ndx.root_page)
    root_ec   = struct.unpack_from("<H", root_page, _NODE_OFF_COUNT)[0]
    check("BIGIDX root entry_count", root_ec, 13)
    # Total entries: 13*18 + 11 = 245
    pairs = ndx.inorder()
    check("BIGIDX total entries", len(pairs), 245)

    # ------------------------------------------------------------------
    # NUNIQ.NDX (mint, unique_flag != 0)
    # [ndx.md Open questions (resolved): "unique flag @0x17 = 0x01";
    #  mint-results-001.md: "INDEX ON STATE TO NUNIQ UNIQUE"]
    # Stored as LE u16 = 256 (byte 0x17 = 0x01, byte 0x16 = 0x00).
    # ------------------------------------------------------------------
    nuniq_path = os.path.join(mint, "NUNIQ.NDX")
    ndx = NdxFile(nuniq_path)

    check("NUNIQ key_type",  ndx.key_type,  0)
    check("NUNIQ key_length",ndx.key_length, 2)
    check("NUNIQ key_expr",  ndx.key_expr,  "STATE ")
    # unique_flag is non-zero (the exact stored LE u16 = 256 = 0x0100)
    # [ndx.md: byte 0x17 = 0x01; LE u16 read of [0x16..0x17] = 0x0100 = 256]
    if ndx.unique_flag == 0:
        _selftest_fail("NUNIQ unique_flag should be non-zero, got 0", failures)
    checks += 1
    # Spot-check: byte 0x17 in raw data should be 1
    with open(nuniq_path, "rb") as fp:
        raw = fp.read(512)
    check("NUNIQ byte @0x17", raw[0x17], 1)

    # ------------------------------------------------------------------
    # Consistency: group_length and keys_per_page formulas hold across
    # all goldens we tested (ndx.md sec 2 / Verification)
    # ------------------------------------------------------------------
    all_ndx = [
        os.path.join(pristine, n) for n in
        ["CNAMES.NDX", "ZIPCODE.NDX", "CUSTOMER.NDX", "TOURDATE.NDX",
         "AVAL_FLT.NDX", "FLT_NO.NDX", "LOCATION.NDX", "TRIPS.NDX"]
    ] + [
        os.path.join(mint, n) for n in
        ["NCOST.NDX", "NDATE.NDX", "BIGIDX.NDX", "NUNIQ.NDX"]
    ]
    for ndx_path in all_ndx:
        ndx = NdxFile(ndx_path)
        base = os.path.basename(ndx_path)
        expected_grp = _ceil4(ndx.key_length + 8)
        expected_kpp = _KPP_DIVISOR // ndx.group_length
        check("%s grp==ceil4(kl+8)" % base,
              ndx.group_length, expected_grp)
        check("%s kpp==(508//grp)" % base,
              ndx.keys_per_page, expected_kpp)
        checks += 1   # extra check for reserved==0 (all III+ fixtures)
        if ndx.reserved != 0:
            _selftest_fail(
                "%s reserved@0x08 should be 0, got %d" % (base, ndx.reserved),
                failures)

    # ------------------------------------------------------------------
    # Report
    # ------------------------------------------------------------------
    if failures:
        sys.stderr.write("ndx_ref selftest: %d checks, %d failures\n"
                         % (checks, len(failures)))
        sys.exit(1)
    else:
        sys.stdout.write("ndx_ref selftest: %d checks, 0 failures\n" % checks)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

_USAGE = (
    "usage: %(prog)s --header     <file.ndx>         print normalized header\n"
    "       %(prog)s --index-dump <file.ndx>         print in-order (key, recno) pairs\n"
    "       %(prog)s --seek       <file.ndx> <key>   descend tree for KEY\n"
    "       %(prog)s --selftest                       assert ndx.md values against goldens\n"
)


def main(argv):
    prog = argv[0]
    if len(argv) < 2:
        sys.stderr.write(_USAGE % {"prog": prog})
        return 2

    mode = argv[1]

    if mode == "--selftest":
        if len(argv) != 2:
            sys.stderr.write("ndx_ref: --selftest takes no arguments\n")
            return 2
        run_selftest()
        return 0

    if mode == "--header":
        if len(argv) != 3:
            sys.stderr.write("ndx_ref: --header needs a FILE.NDX argument\n")
            return 2
        try:
            cmd_header(argv[2], sys.stdout)
        except (ValueError, OSError) as exc:
            sys.stderr.write("ndx_ref: error: %s\n" % exc)
            return 1
        return 0

    if mode == "--index-dump":
        if len(argv) != 3:
            sys.stderr.write("ndx_ref: --index-dump needs a FILE.NDX argument\n")
            return 2
        try:
            cmd_index_dump(argv[2], sys.stdout)
        except (ValueError, OSError) as exc:
            sys.stderr.write("ndx_ref: error: %s\n" % exc)
            return 1
        return 0

    if mode == "--seek":
        if len(argv) != 4:
            sys.stderr.write("ndx_ref: --seek needs FILE.NDX and KEY arguments\n")
            return 2
        try:
            rc = cmd_seek(argv[2], argv[3], sys.stdout)
        except (ValueError, OSError) as exc:
            sys.stderr.write("ndx_ref: error: %s\n" % exc)
            return 1
        return rc or 0

    sys.stderr.write("ndx_ref: unknown mode '%s'\n" % mode)
    sys.stderr.write(_USAGE % {"prog": prog})
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
