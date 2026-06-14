#!/usr/bin/env python3
# harness/diff/fat_diff/fat12_ref.py -- independent FAT12 reference reader.
#
# FACTORY tooling (CLAUDE.md Law 3): the artifact stays C; this python reader is
# the SECOND independent reference for the FAT12 differential oracle (beads
# initech-5cu). It is intentionally a THIRD implementation -- NOT mtools, NOT
# our C reader (os/milton/fat12.c). It parses the BPB, decodes the 12-bit FAT,
# walks cluster chains, and reads files, all from first principles per
# docs/research/fat12-ground-truth.md Sec 1-4. Its purpose is to catch a bug
# SHARED between our C reader and mtools' interpretation -- agreement among
# three independent implementations is a far stronger signal than two.
#
# Modes (matching fat_dump.c, plus the positioned-read mode):
#   --list                    : one "NAME.EXT <size>" line per REGULAR file,
#                               sorted ascending by name. Timestamps / volume
#                               serial normalized away (never emitted).
#   --cat NAME.EXT            : write the named file's EXACT bytes to stdout.
#   --cat-range NAME OFF LEN  : write EXACTLY the byte slice [OFF, OFF+LEN) of
#                               the named file to stdout (binary). Past-EOF /
#                               over-long LEN are clamped to file_size, exactly
#                               like fat12_read_partial: a slice at/after EOF
#                               writes zero bytes (clean EOF, exit 0). This is
#                               the INDEPENDENT reference for the positioned-read
#                               primitive (beads initech-lq2): it slices the
#                               WHOLE file it already reads from first principles,
#                               so it shares no code with the C walk.
#
# Fail loud (CLAUDE.md Rule 2): any structural error raises and exits non-zero;
# a missing file in --cat exits non-zero. No silent partial output.
#
# ASCII-clean (Rule 12). Deterministic output (Rule 11): sorted list, no
# timestamps. Pure stdlib (struct/sys) -- no extra runtimes (Law 3).

import struct
import sys

# FAT12 special values (docs/research/fat12-ground-truth.md Sec 3).
EOC_MIN = 0xFF8     # >= this is end-of-chain
BAD     = 0xFF7
FREE    = 0x000

# Directory sentinels / attributes (Sec 4).
NAME_FREE    = 0x00   # end of directory: stop scanning
NAME_DELETED = 0xE5   # deleted entry: skip
NAME_E5_ALIAS = 0x05  # real first char is 0xE5 (RISK-3)
ATTR_VOLLABEL = 0x08
ATTR_DIRECTORY = 0x10
ATTR_LFN      = 0x0F  # RO|Hidden|System|VolLabel -> VFAT long-name slot


class Fat12:
    """Independent FAT12 reader over a raw image byte buffer."""

    def __init__(self, data):
        self.data = data
        # ---- Parse the BPB (little-endian; Sec 1). ----
        bps = struct.unpack_from("<H", data, 0x0B)[0]
        if bps != 512:
            raise ValueError("bytes_per_sector != 512 (got %d)" % bps)
        sig = struct.unpack_from("<H", data, 510)[0]
        if sig != 0xAA55:
            raise ValueError("bad boot signature 0x%04X" % sig)
        self.bps  = bps
        self.spc  = data[0x0D]                              # sectors_per_cluster
        self.res  = struct.unpack_from("<H", data, 0x0E)[0]  # reserved_sectors
        self.nfat = data[0x10]                              # num_fats
        self.nrde = struct.unpack_from("<H", data, 0x11)[0]  # root_entry_count
        self.spf  = struct.unpack_from("<H", data, 0x16)[0]  # sectors_per_fat
        if self.spc == 0 or self.nfat == 0 or self.spf == 0 or self.nrde == 0:
            raise ValueError("nonsense BPB geometry")

        # ---- Derived geometry (Sec 2). ----
        self.fat_off  = self.res * self.bps
        self.rdir_off = (self.res + self.nfat * self.spf) * self.bps
        self.rdir_sectors = (self.nrde * 32 + self.bps - 1) // self.bps
        self.first_data_sector = (self.res + self.nfat * self.spf
                                  + self.rdir_sectors)
        # Whole FAT in memory (RISK-1: avoids the odd-cluster boundary case).
        fat_bytes = self.nfat and self.spf * self.bps
        self.fat = data[self.fat_off:self.fat_off + fat_bytes]

    def fat12_entry(self, n):
        """Decode the 12-bit FAT entry for cluster n (Sec 3)."""
        if n < 2:
            raise ValueError("cluster %d is reserved" % n)
        bo = (n * 3) // 2
        if bo + 1 >= len(self.fat):
            raise ValueError("cluster %d out of FAT range" % n)
        v = self.fat[bo] | (self.fat[bo + 1] << 8)
        return (v & 0x0FFF) if (n % 2 == 0) else ((v >> 4) & 0x0FFF)

    def walk_chain(self, start):
        """Return the list of clusters from start..EOC (Sec 3)."""
        chain = []
        cl = start
        # A valid chain visits at most total cluster count; bound the loop so a
        # cyclic chain raises instead of hanging (Rule 2 anti-hang).
        limit = len(self.fat) * 2 // 3 + 4
        while cl < EOC_MIN:
            if cl == FREE or cl == BAD:
                raise ValueError("free/bad cluster 0x%03X mid-chain" % cl)
            chain.append(cl)
            if len(chain) > limit:
                raise ValueError("cluster chain too long (cyclic?)")
            cl = self.fat12_entry(cl)
        return chain

    def format_83(self, ent):
        """Render the 8.3 name (Sec 4). Applies the 0x05 -> 0xE5 fix."""
        name = bytearray(ent[0:8])
        if name[0] == NAME_E5_ALIAS:
            name[0] = NAME_DELETED  # 0x05 -> 0xE5 real first char (RISK-3)
        nm = name.rstrip(b"\x20")
        ext = bytes(ent[8:11]).rstrip(b"\x20")
        # Latin-1 keeps the 0xE5 byte intact; names are otherwise ASCII.
        s = nm.decode("latin-1")
        if ext:
            s += "." + ext.decode("latin-1")
        return s

    def list_files(self):
        """Yield (name, size) for each REGULAR file, in directory order.

        Skips deleted (0xE5), LFN (attr 0x0F), volume-label (0x08) and
        directory (0x10) entries; STOPS at the 0x00 sentinel.
        """
        out = []
        for i in range(self.nrde):
            off = self.rdir_off + i * 32
            ent = self.data[off:off + 32]
            if len(ent) < 32:
                break
            first = ent[0]
            if first == NAME_FREE:
                break          # end of directory
            if first == NAME_DELETED:
                continue       # deleted
            attr = ent[11]
            if attr == ATTR_LFN:
                continue       # VFAT long-name slot
            if attr & ATTR_VOLLABEL:
                continue       # volume label -- not a file
            if attr & ATTR_DIRECTORY:
                continue       # subdirectory -- not a regular file
            size = struct.unpack_from("<I", ent, 28)[0]
            out.append((self.format_83(ent), size))
        return out

    def find(self, target):
        """Return the 32-byte dir entry whose 8.3 name == target (case-insens.),
        or None. Stops at the 0x00 sentinel."""
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
        """Read exactly file_size bytes of the file described by dir entry."""
        start = struct.unpack_from("<H", ent, 26)[0]
        size  = struct.unpack_from("<I", ent, 28)[0]
        if size == 0:
            return b""  # zero-length: no chain walk (start may be 0)
        bytes_per_cluster = self.spc * self.bps
        out = bytearray()
        for cl in self.walk_chain(start):
            lba = self.first_data_sector + (cl - 2) * self.spc
            o = lba * self.bps
            out += self.data[o:o + bytes_per_cluster]
            if len(out) >= size:
                break
        # file_size is authoritative for the last cluster (RISK-5).
        return bytes(out[:size])

    def list_dir(self, start_cluster):
        """Yield (name, size) for each REGULAR file in a SUBDIRECTORY whose
        cluster chain starts at start_cluster (independent reference for
        fat12_read_dir, beads initech-ti8). Walks the chain cluster-by-cluster
        reading 32-byte entries (reuse walk_chain + read_file's data-area LBA
        math: first_data_sector + (cl-2)*spc); a subdir is NOT bounded by
        root_entry_count -- it ends at the 0x00 sentinel or EOC. Skips
        deleted/LFN/volume-label/directory entries (incl. '.' and '..', which
        carry the directory attribute)."""
        out = []
        bytes_per_cluster = self.spc * self.bps
        for cl in self.walk_chain(start_cluster):
            lba = self.first_data_sector + (cl - 2) * self.spc
            base = lba * self.bps
            for i in range(bytes_per_cluster // 32):
                off = base + i * 32
                ent = self.data[off:off + 32]
                if len(ent) < 32:
                    return out
                first = ent[0]
                if first == NAME_FREE:
                    return out          # end of directory: stop the whole walk
                if first == NAME_DELETED:
                    continue
                attr = ent[11]
                if attr == ATTR_LFN:
                    continue
                if attr & ATTR_VOLLABEL:
                    continue
                if attr & ATTR_DIRECTORY:
                    continue            # subdir / '.' / '..' -- not a file
                size = struct.unpack_from("<I", ent, 28)[0]
                out.append((self.format_83(ent), size))
        return out

    def _find_in_dir(self, is_root, start_cluster, want):
        """Find the 32-byte dir entry whose 8.3 name == want (upper-cased) in a
        directory (root region OR a subdir chain). Returns the entry or None."""
        if is_root:
            for i in range(self.nrde):
                off = self.rdir_off + i * 32
                ent = self.data[off:off + 32]
                if len(ent) < 32:
                    return None
                first = ent[0]
                if first == NAME_FREE:
                    return None
                if first == NAME_DELETED:
                    continue
                if ent[11] == ATTR_LFN:
                    continue
                if self.format_83(ent).upper() == want:
                    return ent
            return None
        bytes_per_cluster = self.spc * self.bps
        for cl in self.walk_chain(start_cluster):
            lba = self.first_data_sector + (cl - 2) * self.spc
            base = lba * self.bps
            for i in range(bytes_per_cluster // 32):
                off = base + i * 32
                ent = self.data[off:off + 32]
                if len(ent) < 32:
                    return None
                first = ent[0]
                if first == NAME_FREE:
                    return None
                if first == NAME_DELETED:
                    continue
                if ent[11] == ATTR_LFN:
                    continue
                if self.format_83(ent).upper() == want:
                    return ent
        return None

    def find_path(self, path):
        """Resolve a backslash-separated path to a FILE entry (independent
        reference for a subdir file lookup, beads initech-zs24). Split the bare
        8.3 leaf off the PARENT directory path, resolve the parent to its chain,
        then find the leaf there. Returns the 32-byte dir entry or None (missing
        parent component raises ValueError, mirroring resolve_dir)."""
        # Strip a leading drive prefix (letter + ':').
        if len(path) >= 2 and path[1] == ":" and path[0].isalpha():
            path = path[2:]
        # The leaf is everything after the last '\'; the parent is before it.
        sep = path.rfind("\\")
        if sep < 0:
            parent = ""           # bare name -> the root
            leaf = path
        else:
            parent = path[:sep]
            leaf = path[sep + 1:]
        is_root, start_cluster = self.resolve_dir(parent)
        return self._find_in_dir(is_root, start_cluster, leaf.upper())

    def resolve_dir(self, path):
        """Resolve a backslash-separated path to a DIRECTORY, returning
        (is_root, start_cluster). Fails loud (raises ValueError) on a missing or
        non-directory mid-path component (independent reference for
        fat12_resolve_path's directory leg, beads initech-ti8)."""
        # Strip a leading drive prefix (letter + ':').
        if len(path) >= 2 and path[1] == ":" and path[0].isalpha():
            path = path[2:]
        is_root = True
        start_cluster = 0
        for comp in path.split("\\"):
            if comp == "" or comp == ".":
                continue
            if comp == "..":
                if is_root:
                    continue
                ent = self._find_in_dir(is_root, start_cluster, "..")
                if ent is None:
                    raise ValueError("no '..' in directory")
                start_cluster = struct.unpack_from("<H", ent, 26)[0]
                is_root = (start_cluster == 0)
                continue
            ent = self._find_in_dir(is_root, start_cluster, comp.upper())
            if ent is None:
                raise ValueError("path component not found: %s" % comp)
            if not (ent[11] & ATTR_DIRECTORY):
                raise ValueError("path component is not a directory: %s" % comp)
            start_cluster = struct.unpack_from("<H", ent, 26)[0]
            is_root = (start_cluster == 0)
        return (is_root, start_cluster)


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
        sys.stderr.write("fat12_ref: cannot open '%s': %s\n" % (img, exc))
        return 1

    try:
        fs = Fat12(data)
    except ValueError as exc:
        sys.stderr.write("fat12_ref: bad volume '%s': %s\n" % (img, exc))
        return 1

    if mode == "--list":
        if len(argv) != 3:
            sys.stderr.write("fat12_ref: --list takes no extra args\n")
            return 2
        try:
            files = fs.list_files()
        except ValueError as exc:
            sys.stderr.write("fat12_ref: list failed: %s\n" % exc)
            return 1
        # Deterministic: sort ascending by name (Rule 11).
        for name, size in sorted(files, key=lambda r: r[0]):
            sys.stdout.write("%s %d\n" % (name, size))
        return 0

    if mode == "--cat":
        if len(argv) != 4:
            sys.stderr.write("fat12_ref: --cat needs a NAME.EXT\n")
            return 2
        ent = fs.find(argv[3])
        if ent is None:
            sys.stderr.write("fat12_ref: file not found: %s\n" % argv[3])
            return 1
        try:
            content = fs.read_file(ent)
        except ValueError as exc:
            sys.stderr.write("fat12_ref: read failed: %s\n" % exc)
            return 1
        sys.stdout.buffer.write(content)
        return 0

    if mode == "--cat-range":
        # --cat-range NAME OFFSET LEN : write the slice [OFFSET, OFFSET+LEN) of
        # the named file. Computed INDEPENDENTLY of the C primitive: read the
        # whole file with this module's own first-principles reader, then slice
        # in pure python. Past-EOF / over-long LEN clamp to file_size exactly as
        # fat12_read_partial does (a slice at/after EOF -> zero bytes, exit 0).
        if len(argv) != 6:
            sys.stderr.write("fat12_ref: --cat-range needs NAME OFFSET LEN\n")
            return 2
        try:
            off = int(argv[4], 0)
            ln  = int(argv[5], 0)
        except ValueError:
            sys.stderr.write("fat12_ref: --cat-range OFFSET/LEN must be integers\n")
            return 2
        if off < 0 or ln < 0:
            sys.stderr.write("fat12_ref: --cat-range OFFSET/LEN must be >= 0\n")
            return 2
        ent = fs.find(argv[3])
        if ent is None:
            sys.stderr.write("fat12_ref: file not found: %s\n" % argv[3])
            return 1
        try:
            content = fs.read_file(ent)
        except ValueError as exc:
            sys.stderr.write("fat12_ref: read failed: %s\n" % exc)
            return 1
        # Clamp the slice to the file -- a positioned read past EOF yields the
        # bytes that exist (possibly none), never an error.
        sl = content[off:off + ln] if off < len(content) else b""
        sys.stdout.buffer.write(sl)
        return 0

    if mode == "--cat-path":
        # --cat-path PATH : write the EXACT bytes of the file at a backslash-
        # separated PATH (e.g. "SUB\\NEW.TXT" or "\\SUB\\NEW.TXT"). The
        # INDEPENDENT reference for a SUBDIRECTORY file write-back (beads
        # initech-zs24): resolve the parent dir from first principles, find the
        # leaf, read file_size bytes. Missing file -> exit 1.
        if len(argv) != 4:
            sys.stderr.write("fat12_ref: --cat-path needs a PATH\n")
            return 2
        try:
            ent = fs.find_path(argv[3])
        except ValueError as exc:
            sys.stderr.write("fat12_ref: --cat-path bad path '%s': %s\n"
                             % (argv[3], exc))
            return 1
        if ent is None:
            sys.stderr.write("fat12_ref: file not found: %s\n" % argv[3])
            return 1
        try:
            content = fs.read_file(ent)
        except ValueError as exc:
            sys.stderr.write("fat12_ref: read failed: %s\n" % exc)
            return 1
        sys.stdout.buffer.write(content)
        return 0

    if mode == "--list-path":
        # --list-path PATH : list the REGULAR files of the directory at PATH
        # (a backslash-separated subdirectory path; "\\" or "" is the root),
        # one "NAME.EXT <size>" line each, sorted. The INDEPENDENT reference for
        # fat12_read_dir / fat12_resolve_path (beads initech-ti8): resolves the
        # path from first principles, then walks the subdir cluster chain. Fail
        # loud (exit 1) on a missing / non-directory mid-path component.
        if len(argv) != 4:
            sys.stderr.write("fat12_ref: --list-path needs a PATH\n")
            return 2
        path = argv[3]
        try:
            is_root, start_cluster = fs.resolve_dir(path)
        except ValueError as exc:
            sys.stderr.write("fat12_ref: --list-path bad path '%s': %s\n"
                             % (path, exc))
            return 1
        try:
            files = fs.list_files() if is_root else fs.list_dir(start_cluster)
        except ValueError as exc:
            sys.stderr.write("fat12_ref: --list-path list failed: %s\n" % exc)
            return 1
        for name, size in sorted(files, key=lambda r: r[0]):
            sys.stdout.write("%s %d\n" % (name, size))
        return 0

    sys.stderr.write("fat12_ref: unknown mode '%s'\n" % mode)
    return 2


if __name__ == "__main__":
    sys.exit(main(sys.argv))
