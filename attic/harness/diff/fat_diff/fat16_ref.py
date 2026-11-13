#!/usr/bin/env python3
# harness/diff/fat_diff/fat16_ref.py -- independent FAT16 reference reader.
#
# FACTORY tooling (CLAUDE.md Law 3): the artifact stays C; this python reader is
# the SECOND independent reference for the FAT16 differential oracle (beads
# initech-z01). It is intentionally a THIRD implementation -- NOT mtools, NOT
# our C reader (os/milton/fat12.c). It parses the BPB, decodes the 16-bit FAT,
# walks cluster chains, and reads files, all from first principles per
# docs/research/fat16-ground-truth.md Sec 1-4. Its purpose is to catch a bug
# SHARED between our C reader and mtools' interpretation -- agreement among
# three independent implementations is a far stronger signal than two.
#
# This is a SEPARATE reader from fat12_ref.py (NOT the 12-bit decode reused):
# FAT16 is a flat little-endian uint16_t array, byte_offset = N*2, EOC 0xFFF8+.
#
# Modes (matching fat_dump.c):
#   --list           : one "NAME.EXT <size>" line per REGULAR file, sorted
#                      ascending by name. Timestamps / volume serial normalized
#                      away (never emitted).
#   --cat NAME.EXT   : write the named file's EXACT bytes to stdout.
#
# Fail loud (CLAUDE.md Rule 2): any structural error raises and exits non-zero;
# a missing file in --cat exits non-zero. No silent partial output.
#
# ASCII-clean (Rule 12). Deterministic output (Rule 11): sorted list, no
# timestamps. Pure stdlib (struct/sys) -- no extra runtimes (Law 3).

import struct
import sys

# FAT16 special values (docs/research/fat16-ground-truth.md Sec 3).
EOC_MIN = 0xFFF8     # >= this is end-of-chain (NOT 0xFF8 -- that is FAT12)
BAD     = 0xFFF7
FREE    = 0x0000

# FAT type classification thresholds (Microsoft FAT spec; brief Sec 2).
FAT12_MAX_CLUSTERS = 4085
FAT16_MAX_CLUSTERS = 65525

# Directory sentinels / attributes (Sec 4 -- identical to FAT12).
NAME_FREE     = 0x00
NAME_DELETED  = 0xE5
NAME_E5_ALIAS = 0x05
ATTR_VOLLABEL = 0x08
ATTR_DIRECTORY = 0x10
ATTR_LFN      = 0x0F


class Fat16:
    """Independent FAT16 reader over a raw image byte buffer."""

    def __init__(self, data):
        self.data = data
        # ---- Parse the BPB (little-endian; brief Sec 1; same layout as FAT12). ----
        bps = struct.unpack_from("<H", data, 0x0B)[0]
        if bps != 512:
            raise ValueError("bytes_per_sector != 512 (got %d)" % bps)
        sig = struct.unpack_from("<H", data, 510)[0]
        if sig != 0xAA55:
            raise ValueError("bad boot signature 0x%04X" % sig)
        self.bps  = bps
        self.spc  = data[0x0D]
        self.res  = struct.unpack_from("<H", data, 0x0E)[0]
        self.nfat = data[0x10]
        self.nrde = struct.unpack_from("<H", data, 0x11)[0]
        ts16      = struct.unpack_from("<H", data, 0x13)[0]
        self.spf  = struct.unpack_from("<H", data, 0x16)[0]
        ts32      = struct.unpack_from("<I", data, 0x20)[0]
        if self.spc == 0 or self.nfat == 0 or self.spf == 0 or self.nrde == 0:
            raise ValueError("nonsense BPB geometry")

        # ---- Derived geometry (brief Sec 2 -- identical formulas to FAT12). ----
        self.total_sectors = ts16 if ts16 != 0 else ts32
        self.fat_off  = self.res * self.bps
        self.rdir_off = (self.res + self.nfat * self.spf) * self.bps
        self.rdir_sectors = (self.nrde * 32 + self.bps - 1) // self.bps
        self.first_data_sector = (self.res + self.nfat * self.spf
                                  + self.rdir_sectors)

        # ---- Classify FAT12 vs FAT16 SOLELY by cluster count (brief Sec 2). ----
        data_sectors = self.total_sectors - self.first_data_sector
        self.total_clusters = data_sectors // self.spc
        if not (FAT12_MAX_CLUSTERS <= self.total_clusters < FAT16_MAX_CLUSTERS):
            raise ValueError(
                "not a FAT16 volume: %d clusters (FAT16 is %d..%d)"
                % (self.total_clusters, FAT12_MAX_CLUSTERS,
                   FAT16_MAX_CLUSTERS - 1))

        # Whole FAT in memory.
        fat_bytes = self.spf * self.bps
        self.fat = data[self.fat_off:self.fat_off + fat_bytes]

    def fat16_entry(self, n):
        """Decode the 16-bit FAT16 entry for cluster n (brief Sec 3).

        Flat little-endian uint16_t array: byte_offset = n*2; no nibble packing,
        no 12-bit mask, no even/odd case."""
        if n < 2:
            raise ValueError("cluster %d is reserved" % n)
        bo = n * 2
        if bo + 1 >= len(self.fat):
            raise ValueError("cluster %d out of FAT range" % n)
        return self.fat[bo] | (self.fat[bo + 1] << 8)

    def walk_chain(self, start):
        """Return the list of clusters from start..EOC (brief Sec 3)."""
        chain = []
        cl = start
        # A valid chain visits at most the cluster count; bound the loop so a
        # cyclic chain raises instead of hanging (Rule 2 anti-hang).
        limit = self.total_clusters + 4
        while cl < EOC_MIN:
            if cl == FREE or cl == BAD:
                raise ValueError("free/bad cluster 0x%04X mid-chain" % cl)
            chain.append(cl)
            if len(chain) > limit:
                raise ValueError("cluster chain too long (cyclic?)")
            cl = self.fat16_entry(cl)
        return chain

    def format_83(self, ent):
        """Render the 8.3 name (Sec 4 -- identical to FAT12). 0x05 -> 0xE5 fix."""
        name = bytearray(ent[0:8])
        if name[0] == NAME_E5_ALIAS:
            name[0] = NAME_DELETED
        nm = name.rstrip(b"\x20")
        ext = bytes(ent[8:11]).rstrip(b"\x20")
        s = nm.decode("latin-1")
        if ext:
            s += "." + ext.decode("latin-1")
        return s

    def list_files(self):
        """Yield (name, size) for each REGULAR file, in directory order."""
        out = []
        for i in range(self.nrde):
            off = self.rdir_off + i * 32
            ent = self.data[off:off + 32]
            if len(ent) < 32:
                break
            first = ent[0]
            if first == NAME_FREE:
                break
            if first == NAME_DELETED:
                continue
            attr = ent[11]
            if attr == ATTR_LFN:
                continue
            if attr & ATTR_VOLLABEL:
                continue
            if attr & ATTR_DIRECTORY:
                continue
            size = struct.unpack_from("<I", ent, 28)[0]
            out.append((self.format_83(ent), size))
        return out

    def find(self, target):
        """Return the 32-byte dir entry whose 8.3 name == target, or None."""
        want = target.upper()
        for i in range(self.nrde):
            off = self.rdir_off + i * 32
            ent = self.data[off:off + 32]
            if len(ent) < 32:
                break
            first = ent[0]
            if first == NAME_FREE:
                break
            if first == NAME_DELETED:
                continue
            if ent[11] == ATTR_LFN:
                continue
            if self.format_83(ent).upper() == want:
                return ent
        return None

    def read_file(self, ent):
        """Read exactly file_size bytes (brief Sec 3/4; RISK-5 last cluster)."""
        start = struct.unpack_from("<H", ent, 26)[0]
        size  = struct.unpack_from("<I", ent, 28)[0]
        if size == 0:
            return b""
        bytes_per_cluster = self.spc * self.bps
        out = bytearray()
        for cl in self.walk_chain(start):
            lba = self.first_data_sector + (cl - 2) * self.spc
            o = lba * self.bps
            out += self.data[o:o + bytes_per_cluster]
            if len(out) >= size:
                break
        return bytes(out[:size])


def main(argv):
    if len(argv) < 3:
        sys.stderr.write(
            "usage: %s <image> --list\n"
            "       %s <image> --cat NAME.EXT\n" % (argv[0], argv[0]))
        return 2

    img  = argv[1]
    mode = argv[2]

    try:
        with open(img, "rb") as fp:
            data = fp.read()
    except OSError as exc:
        sys.stderr.write("fat16_ref: cannot open '%s': %s\n" % (img, exc))
        return 1

    try:
        fs = Fat16(data)
    except ValueError as exc:
        sys.stderr.write("fat16_ref: bad volume '%s': %s\n" % (img, exc))
        return 1

    if mode == "--list":
        if len(argv) != 3:
            sys.stderr.write("fat16_ref: --list takes no extra args\n")
            return 2
        try:
            files = fs.list_files()
        except ValueError as exc:
            sys.stderr.write("fat16_ref: list failed: %s\n" % exc)
            return 1
        for name, size in sorted(files, key=lambda r: r[0]):
            sys.stdout.write("%s %d\n" % (name, size))
        return 0

    if mode == "--cat":
        if len(argv) != 4:
            sys.stderr.write("fat16_ref: --cat needs a NAME.EXT\n")
            return 2
        ent = fs.find(argv[3])
        if ent is None:
            sys.stderr.write("fat16_ref: file not found: %s\n" % argv[3])
            return 1
        try:
            content = fs.read_file(ent)
        except ValueError as exc:
            sys.stderr.write("fat16_ref: read failed: %s\n" % exc)
            return 1
        sys.stdout.buffer.write(content)
        return 0

    sys.stderr.write("fat16_ref: unknown mode '%s'\n" % mode)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
