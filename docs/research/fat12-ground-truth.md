# FAT12 Ground-Truth Brief — InitechDOS / MILTON Read Support

**Date:** 2026-06-08  
**Author:** Research agent (Law 1: all claims cite a local source or a command run in this session)  
**Scope:** FAT12 read path only, 1.44 MB floppy geometry, for M2 (PRD §11)  
**Sources used:**
- `InitechOS-PRD.md` §6.1, §11 (M2 row)
- `docs/adr/ADR-0003-InitechDOS-Base-OS-Personality.md` §5.7 (DEC-07), Appendix B.1, §1.4 (BPB glossary)
- `spec/dos_structs.h` (locked `dir_entry_t`)
- Empirical: 1.44 MB FAT12 image minted with `mformat` on this machine, bytes read with `python3 struct`

---

## 1. BPB Layout — 1.44 MB FAT12 Floppy

Image minted: `dd if=/dev/zero of=floppy.img bs=512 count=2880` then `mformat -i floppy.img -f 1440 ::`.  
All values below are ACTUAL bytes read from the produced image via `struct.unpack`.  
All multi-byte fields are **little-endian** (Intel byte order throughout FAT).

### Boot sector prefix (offset 0x00)

| Offset | Size | Field | Actual value |
|--------|------|-------|--------------|
| 0x00 | 3 | Jump instruction | `EB 3C 90` (JMP short + NOP) |
| 0x03 | 8 | OEM name | `MTOO4043` (mtools 4.0.43 string) |

### BIOS Parameter Block (BPB, offset 0x0B)

| Offset | Size | Field | Actual value | Notes |
|--------|------|-------|--------------|-------|
| 0x0B | 2 | bytes_per_sector | 512 | Standard sector size |
| 0x0D | 1 | sectors_per_cluster | 1 | Each cluster = 1 sector = 512 bytes |
| 0x0E | 2 | reserved_sectors | 1 | Boot sector only |
| 0x10 | 1 | num_fats | 2 | Two redundant FATs |
| 0x11 | 2 | root_entry_count | 224 | Fixed root dir size |
| 0x13 | 2 | total_sectors_16 | 2880 | Total sectors on volume |
| 0x15 | 1 | media_descriptor | 0xF0 | 1.44 MB floppy canonical value |
| 0x16 | 2 | sectors_per_fat | 9 | Each FAT occupies 9 sectors |
| 0x18 | 2 | sectors_per_track | 18 | CHS geometry |
| 0x1A | 2 | num_heads | 2 | CHS geometry |
| 0x1C | 4 | hidden_sectors | 0 | No partition offset |

### Extended BPB (FAT12/16, offset 0x24)

| Offset | Size | Field | Actual value |
|--------|------|-------|--------------|
| 0x24 | 1 | drive_number | 0x00 |
| 0x25 | 1 | reserved | 0x00 |
| 0x26 | 1 | boot_signature | 0x29 (extended BPB present) |
| 0x27 | 4 | volume_id | 0x1A903600 (random serial) |
| 0x2B | 11 | volume_label | `NO NAME    ` |
| 0x36 | 8 | fs_type_string | `FAT12   ` |

Boot sector signature at 0x1FE: `0xAA55` (valid).

---

## 2. Derived Geometry Formulas

Using actual values from the image above:

```
first_FAT_sector    = reserved_sectors                           =  1
second_FAT_sector   = reserved_sectors + sectors_per_fat         = 10
root_dir_start      = reserved_sectors + num_fats * sectors_per_fat
                    = 1 + 2*9                                    = 19
root_dir_sectors    = ceil(root_entry_count * 32 / bytes_per_sector)
                    = ceil(224 * 32 / 512)                       = 14
first_data_sector   = root_dir_start + root_dir_sectors
                    = 19 + 14                                     = 33
total_data_sectors  = total_sectors_16 - first_data_sector
                    = 2880 - 33                                   = 2847
total_clusters      = total_data_sectors / sectors_per_cluster
                    = 2847 / 1                                    = 2847
```

**Cluster-to-LBA mapping (cluster-2 bias):**

```
LBA(cluster N) = first_data_sector + (N - 2) * sectors_per_cluster
               = 33 + (N - 2) * 1
```

Examples:
- Cluster 2 -> LBA 33 (first data sector, confirmed: contains `Hello from InitechOS test file\n`)
- Cluster 3 -> LBA 34

**FAT type determination by cluster count** (Microsoft FAT specification rule):
- total_clusters < 4085  -> FAT12  (2847 < 4085: confirmed FAT12)
- 4085 <= total_clusters < 65525 -> FAT16
- total_clusters >= 65525 -> FAT32

---

## 3. FAT12 12-Bit Entry Decode Rule

Two 12-bit entries are packed into every 3 consecutive bytes in the FAT region.

**Packing formula:**

For cluster N, compute `byte_offset = (N * 3) / 2` (integer division):

- **N even:** `entry[N] = byte[byte_offset] | ((byte[byte_offset+1] & 0x0F) << 8)`
  - Low byte is fully used; only the low nibble of the next byte contributes.
- **N odd:**  `entry[N] = (byte[byte_offset] >> 4) | (byte[byte_offset+1] << 4)`
  - High nibble of first byte + all 8 bits of second byte, masked to 12 bits.

**Worked example from actual FAT bytes (clusters 4-7, test_chain.txt):**

Raw FAT bytes 0-11: `F0 FF FF FF FF FF 05 60 00 07 F0 FF`

```
Cluster 4 (even): byte_off=6, bytes[6:8] = 05 60
  entry[4] = 0x05 | (0x60 & 0x0F)<<8 = 0x005  -> next cluster is 5

Cluster 5 (odd):  byte_off=7, bytes[7:9] = 60 00
  entry[5] = (0x60 >> 4) | (0x00 << 4) = 0x006  -> next cluster is 6

Cluster 6 (even): byte_off=9, bytes[9:11] = 07 F0
  entry[6] = 0x07 | (0xF0 & 0x0F)<<8 = 0x007  -> next cluster is 7

Cluster 7 (odd):  byte_off=10, bytes[10:12] = F0 FF
  entry[7] = (0xF0 >> 4) | (0xFF << 4) = 0xFFF  -> EOC
```

Cluster chain for test_chain.txt (1620 bytes): 4 -> 5 -> 6 -> 7 -> EOC. Confirmed by reading LBAs 35-38.

**Special entry values (FAT12):**

| Value | Meaning |
|-------|---------|
| 0x000 | Free cluster |
| 0x001 | Reserved (do not use) |
| 0x002-0xFEF | Next cluster in chain |
| 0xFF0-0xFF6 | Reserved |
| 0xFF7 | Bad cluster (do not use) |
| 0xFF8-0xFFF | End of chain (EOC); 0xFFF is the canonical value mformat writes |

Reserved FAT[0] = 0xFF0 (media descriptor 0xF0 padded), FAT[1] = 0xFFF (EOC marker, always).

---

## 4. Directory Entry Read Semantics

The locked `dir_entry_t` in `spec/dos_structs.h` (source: ADR-0003 Appendix B.1) defines the
32-byte structure. Confirmed against actual bytes dumped from the mformat'd image:

```
Offset  Size  Field             Notes
0x00     8    filename          Space-padded (0x20), upper-case ASCII
0x08     3    extension         Space-padded (0x20), upper-case ASCII
0x0B     1    attribute         See attribute bits below
0x0C    10    reserved          Ignore on read
0x16     2    mtime             Two-second resolution; normalize before diffing
0x18     2    mdate             See FAT date encoding; normalize before diffing
0x1A     2    start_cluster     First cluster of file data
0x1C     4    file_size         Exact byte count
```

**8.3 name rules:**
- Stored as 11 bytes (8+3), NOT separated by a dot.
- Space-padded (`0x20`) — do not treat trailing spaces as part of the name.
- Upper-case: mformat stores `TEST_H~1` with tilde-numeric alias for long names.
- `filename[0] == 0xE5` means the entry was deleted; skip it.
- `filename[0] == 0x05` is a special case: the actual first character of the name is `0xE5` (Kanji lead byte / Japanese DOS extension); treat `0x05` -> `0xE5` on display.

**First-byte sentinels** (from `spec/dos_structs.h` `DIR_NAME_FREE` / `DIR_NAME_DELETED`):
- `0x00` — end of directory: stop scanning, no more valid entries follow.
- `0xE5` — deleted entry: skip and continue scanning.

**Attribute byte filtering** (`spec/dos_structs.h` attribute constants):
- `0x08` (`DIR_ATTR_VOLLABEL`): volume label entry — skip for file listing.
- `0x0F` (RO | Hidden | System | VolLabel = `0x01|0x02|0x04|0x08`): Long File Name (LFN)
  entry — VFAT extension. Skip these for read purposes (read the 8.3 alias entry that follows).
  Observed in practice: mtools writes LFN entries even on FAT12. The 8.3 alias (e.g. `TEST_H~1.TXT`)
  is the entry with `attr != 0x0F` that follows the LFN sequence.
- `0x10` (`DIR_ATTR_DIRECTORY`): subdirectory. `start_cluster` points to its cluster chain;
  `file_size` is 0 for directories.

**File read using start_cluster and file_size:**
1. Walk the cluster chain starting at `start_cluster` using FAT entries.
2. Read `sectors_per_cluster` sectors per cluster starting at `LBA(cluster)`.
3. Stop when the FAT entry >= 0xFF8 (EOC) or bytes read >= `file_size`.
4. Truncate the final cluster's data to `file_size` mod `bytes_per_cluster` bytes
   (last cluster is typically partially filled; `file_size` is authoritative).

---

## 5. Differential Oracle Reference Commands

### Directory listing (golden):

```bash
mdir -i disk.img ::
```

Output includes: name (8.3 alias + long name), size in bytes, date/time.
**Normalization needed**: mtools date/time fields use the host clock when copying files in;
use `mdir` output for names and sizes only when diffing. The time/date fields in
directory entries are implementation-specific on write; normalize them to zero before
byte-level comparison, or compare only the name/size/cluster/content fields.

Volume label line varies; strip before diffing.

### File content extraction (golden):

```bash
mcopy -i disk.img ::FILENAME.EXT -
```

Outputs raw file content to stdout. Use this as ground truth for content comparison.
Example: `mcopy -i floppy.img "::TEST_H~1.TXT" -` produces `Hello from InitechOS test file\n`.

### Minimal independent Python3 FAT12 reader (oracle pseudocode):

A second independent reference (not mtools) for the differential suite:

```python
# fat12_read.py -- minimal independent FAT12 oracle
import struct, sys

def read_fat12(img_path, target_83name):
    data = open(img_path, "rb").read()
    # Parse BPB
    bps  = struct.unpack_from("<H", data, 11)[0]   # bytes_per_sector
    spc  = data[13]                                  # sectors_per_cluster
    res  = struct.unpack_from("<H", data, 14)[0]    # reserved_sectors
    nfat = data[16]
    nrde = struct.unpack_from("<H", data, 17)[0]    # root_entry_count
    spf  = struct.unpack_from("<H", data, 22)[0]    # sectors_per_fat
    # Geometry
    fat_off  = res * bps
    rdir_off = (res + nfat * spf) * bps
    rdir_sec = (nrde * 32 + bps - 1) // bps
    data_off = rdir_off + rdir_sec * bps
    # FAT12 entry reader
    fat = data[fat_off : fat_off + nfat * spf * bps]
    def fat12(n):
        bo = (n * 3) // 2
        v = fat[bo] | (fat[bo+1] << 8)
        return (v & 0x0FFF) if n % 2 == 0 else ((v >> 4) & 0x0FFF)
    # Directory scan (root dir only -- no subdirs for this sketch)
    for i in range(nrde):
        e = data[rdir_off + i*32 : rdir_off + (i+1)*32]
        if e[0] == 0x00: break
        if e[0] == 0xE5: continue
        if e[11] == 0x0F: continue  # LFN
        name83 = (e[0:8].rstrip(b'\x20') + b'.' + e[8:11].rstrip(b'\x20')).decode()
        if name83.upper() == target_83name.upper():
            start = struct.unpack_from("<H", e, 26)[0]
            fsize = struct.unpack_from("<I", e, 28)[0]
            # Walk cluster chain
            out = bytearray()
            cl = start
            while cl < 0xFF8:
                lba = (res + nfat*spf + rdir_sec) + (cl - 2) * spc
                out += data[lba*bps : lba*bps + spc*bps]
                cl = fat12(cl)
            return bytes(out[:fsize])
    return None
```

This reader is intentionally minimal; the real oracle harness should add error checking,
subdirectory traversal, and verification that both FATs agree.

### Normalization for diffs:
- Strip volume label entries (attr `0x08`) from listings.
- Normalize mtime/mdate fields to zero before byte-level header comparison.
- Compare file content byte-for-byte (no normalization needed for content).

---

## 6. Recommendation: Lock a `bpb_t` into `spec/dos_structs.h`

**Recommendation: YES, lock it.**

### Finding: SPEC GAP

The BPB is currently only **glossary-mentioned** in ADR-0003 §1.4 ("BIOS Parameter Block: the
structure, resident in the boot sector, describing volume geometry") and is referenced by name in
DEC-07 (§5.7). It is **not locked as a struct** in `spec/dos_structs.h`. This is a spec gap that
must be surfaced before M2 implementation begins (PRD §8 / CLAUDE.md Rule 8: "specs are locked
data, not prose in someone's head").

### Proposed `bpb_t` for `spec/dos_structs.h`:

```c
/* ------------------------------------------------------------------------ *
 * Boot Sector / BIOS Parameter Block (FAT12/16)
 *
 *   Source: Microsoft FAT File System Specification (1.03 / 2000-11-06),
 *           Sections 3 (BPB) and 4 (Extended BPB); empirically confirmed
 *           against mformat 4.0.43 output on a 1.44 MB image (Law 1:
 *           docs/research/fat12-ground-truth.md Section 1).
 *   ADR-0003 DEC-07 (Sec 5.7): "The on-volume layout shall be: boot sector;
 *           first File Allocation Table; second (redundant) File Allocation
 *           Table; fixed-size root directory; data area."
 *   Note: the jump instruction and OEM name at offsets 0x00-0x0A are NOT
 *         part of the BPB proper; they precede it and are included here
 *         for struct completeness. The BPB proper begins at 0x0B.
 *   Extended BPB (offset 0x24+) is present when boot_sig == 0x29.
 *   All multi-byte fields are little-endian (Intel byte order).
 * ------------------------------------------------------------------------ */

#pragma pack(push, 1)
typedef struct bpb {
    /* Boot sector prefix (not BPB proper) */
    uint8_t  jmp[3];              /* 0x00: Jump instruction (EB xx 90 or E9 xx xx) */
    uint8_t  oem_name[8];         /* 0x03: OEM name string (e.g. "MTOO4043")       */

    /* BPB proper -- offset 0x0B */
    uint16_t bytes_per_sector;    /* 0x0B: Bytes per sector (512 for floppy)        */
    uint8_t  sectors_per_cluster; /* 0x0D: Sectors per cluster (1 for 1.44 MB)      */
    uint16_t reserved_sectors;    /* 0x0E: Reserved sector count (1 = boot sector)  */
    uint8_t  num_fats;            /* 0x10: Number of FATs (2 = redundant copy)      */
    uint16_t root_entry_count;    /* 0x11: Root directory entry count (224)         */
    uint16_t total_sectors_16;    /* 0x13: Total sectors (2880 for 1.44 MB; 0=FAT32)*/
    uint8_t  media_descriptor;    /* 0x15: Media type (0xF0 = 1.44 MB floppy)       */
    uint16_t sectors_per_fat;     /* 0x16: Sectors per FAT (9 for 1.44 MB)          */
    uint16_t sectors_per_track;   /* 0x18: Sectors per track (18 for 1.44 MB)       */
    uint16_t num_heads;           /* 0x1A: Number of heads (2 for 1.44 MB)          */
    uint32_t hidden_sectors;      /* 0x1C: Hidden sectors (0 for non-partitioned)   */
    uint32_t total_sectors_32;    /* 0x20: Total sectors if total_sectors_16==0     */

    /* Extended BPB (FAT12/FAT16 only) -- offset 0x24 */
    uint8_t  drive_number;        /* 0x24: BIOS drive number (0x00=floppy)          */
    uint8_t  reserved1;           /* 0x25: Reserved (0x00)                          */
    uint8_t  boot_sig;            /* 0x26: Extended boot signature (0x29 = present) */
    uint32_t volume_id;           /* 0x27: Volume serial number (random)            */
    uint8_t  volume_label[11];    /* 0x2B: Volume label (space-padded)              */
    uint8_t  fs_type[8];          /* 0x36: FS type string ("FAT12   " / "FAT16   ") */
} bpb_t;
#pragma pack(pop)

_Static_assert(sizeof(bpb_t) == 62, "bpb_t must be 62 bytes (BPB prefix + BPB + extended BPB; "
               "see docs/research/fat12-ground-truth.md)");

/* Derived geometry macros (ADR-0003 DEC-07; formula verified empirically). */
#define BPB_FAT1_SECTOR(b)       ((b)->reserved_sectors)
#define BPB_ROOT_DIR_SECTOR(b)   ((b)->reserved_sectors + (b)->num_fats * (b)->sectors_per_fat)
#define BPB_ROOT_DIR_SECTORS(b)  (((b)->root_entry_count * 32u + (b)->bytes_per_sector - 1u) \
                                   / (b)->bytes_per_sector)
#define BPB_FIRST_DATA_SECTOR(b) (BPB_ROOT_DIR_SECTOR(b) + BPB_ROOT_DIR_SECTORS(b))
#define BPB_CLUSTER_LBA(b, n)    (BPB_FIRST_DATA_SECTOR(b) + ((n) - 2u) * (b)->sectors_per_cluster)
```

**Note on `total_sectors_32`:** for FAT12 `total_sectors_16` is always non-zero (2880 for 1.44 MB);
`total_sectors_32` at 0x20 is a FAT32-era field retained here for struct completeness.
The `_Static_assert` on 62 bytes covers offsets 0x00-0x3D; the remainder of the 512-byte
boot sector (0x3E-0x1FD) is reserved code/data and the 0x1FE/0x1FF signature, not part of
the struct.

---

## 7. Open Questions and Risks for the Implementer

### RISK-1: FAT12 odd-cluster split across a FAT sector boundary

Clusters 341, 682, 1365, 2047, ... have their 3-byte group straddling a 512-byte FAT sector
boundary (verified by computation: cluster 341 -> byte_offset 511 straddles bytes 511 and 512).
**If the FAT reader processes one sector at a time and indexes into a single-sector buffer,
it will read a garbage high byte for these entries.**
Mitigation: read the entire FAT into memory (9 sectors * 512 bytes = 4608 bytes for 1.44 MB),
which is small enough to hold in RAM without concern on a 32-bit flat memory model.
The whole-FAT-in-memory approach eliminates the boundary case entirely.

### RISK-2: LFN entries in root directory

mtools writes VFAT Long File Name (LFN) entries (attr `0x0F`) even for a plain FAT12 volume
when the source filename has a long name (observed in practice above). The MILTON reader MUST
skip LFN entries (attr `== 0x0F`) and use only the 8.3 alias entries that follow.
For M2 read-only, ignoring LFN is correct. Do NOT mistake an LFN entry for a file.

### RISK-3: `0x05` first-byte special case

If `filename[0] == 0x05`, the actual first character of the 8.3 name is `0xE5` (used by
Japanese DOS; also appears in some CP/M-originated tools). Treat `0x05` -> real name starts
with `0xE5`, not as a deleted-entry sentinel. (The `0xE5` sentinel is only meaningful when
it appears as the RAW value of `filename[0]`.)

### RISK-4: All FAT fields are little-endian

Every multi-byte field in the BPB and directory entries is little-endian.
The 12-bit FAT entry packing is a nibble operation, not a multi-byte endianness concern,
but the two bytes read for each entry must be assembled as LE: `b[0] | (b[1] << 8)`.

### RISK-5: file_size is authoritative for the last cluster

The last cluster in a chain is typically partially filled. Read all `sectors_per_cluster`
sectors of the last cluster into a buffer, but expose only `file_size % (bytes_per_sector *
sectors_per_cluster)` bytes (or all bytes if it divides evenly). Do not return padding zeros.

### RISK-6: root_entry_count * 32 must be sector-aligned in practice

For the 1.44 MB standard geometry `224 * 32 = 7168 = 14 * 512`, which is exactly sector-aligned.
For non-standard images it may not be; the `ceil()` formula is the correct general case.
Lock to the standard 1.44 MB BPB values for M2; generalize later for FAT16/HDD.

### RISK-7: BPB spec gap must be resolved before M2

The BPB is not currently locked in `spec/dos_structs.h`. Per CLAUDE.md Rule 8, adding the
proposed `bpb_t` is a deliberate act requiring an issue in beads and a worklog note.
File the issue before writing any M2 FAT code. This is the single highest-priority pre-M2
action this brief surfaces.

### RISK-8: FAT12/FAT16 auto-detect

The FAT type is determined solely by `total_clusters` (< 4085 = FAT12; 4085-65524 = FAT16),
NOT by the `fs_type` string in the extended BPB, which is informational only and has been
observed to be unreliable in practice. Always compute cluster count and branch on it.

---

## Empirical Artifact

Scratch image retained at `/tmp/fat12_scratch/floppy.img` for this session.
Commands used in this session (Law 1 transparency):

```bash
dd if=/dev/zero of=floppy.img bs=512 count=2880
mformat -i floppy.img -f 1440 ::
mcopy -i floppy.img test_hello.txt ::       # 31-byte file -> cluster 2
mcopy -i floppy.img test_second.txt ::      # 95-byte file -> cluster 3
mcopy -i floppy.img test_chain.txt ::       # 1620-byte file -> clusters 4->5->6->7->EOC
mdir -i floppy.img ::
python3 struct.unpack (BPB, FAT, directory)
```

All values in this document are from actual byte reads, not from memory.
