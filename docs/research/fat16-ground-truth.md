# FAT16 Ground-Truth Brief -- InitechDOS / MILTON Read Support (initech-z01)

**Date:** 2026-06-15
**Author:** Forward-feature agent (Law 1: every claim cites a local source or a
command run in this session)
**Scope:** FAT16 **READ-ONLY** on a **NON-PARTITIONED** single volume
(hidden_sectors=0), mounted on the slave disk exactly like the existing FAT12
data disk. FAT16 WRITE is deferred (509.11); partitioned FAT16 / MBR /
hidden_sectors offset is kzfs/DEC-07 and EXPLICITLY OUT of z01; initrd/tarfs and
multi-volume are OUT.
**Sources used:**
- `InitechOS-PRD.md` Sec 5, Sec 6.1
- `docs/adr/ADR-0003-...` DEC-07 (Sec 5.7): FAT16 for fixed-disk is mandated
- `docs/research/fat12-ground-truth.md` Sec 1 (BPB), Sec 2 (geometry + type rule)
- `spec/dos_structs.h` (locked `bpb_t`, BPB_* geometry macros)
- Microsoft FAT File System Specification 1.03 (2000-11-06), Sec 3 (BPB), Sec 3.2
  (FAT16 entry: a 16-bit entry, EOC 0xFFF8..0xFFFF, bad 0xFFF7)
- Empirical: a real FAT16 image minted this session with
  `mkfs.fat -F 16 -s 1` on an 8 MB file, bytes read with `python3 struct`.

---

## 0. Why FAT16 is "the same BPB, a different FAT"

The BPB / boot-sector layout is **byte-identical** to FAT12 from the struct
point of view (`spec/dos_structs.h` `bpb_t`): same field offsets, same
little-endian encoding, same derived-geometry formulas (`BPB_FAT1_SECTOR`,
`BPB_ROOT_DIR_SECTOR`, `BPB_ROOT_DIR_SECTORS`, `BPB_FIRST_DATA_SECTOR`,
`BPB_CLUSTER_LBA`). The directory-entry layout (`dir_entry_t`) and the fixed
root-directory region are also identical. **Only two things differ between
FAT12 and FAT16:**

1. **The FAT entry decode.** FAT12 packs two 12-bit entries into 3 bytes
   (nibble-straddling). FAT16 is a flat array of little-endian `uint16_t`:
   `entry[N] = read_le16(fat + N*2)`. No packing, no nibble math, no
   odd/even case.
2. **The special-value thresholds.** They are the FAT12 values widened to 16
   bits: free `0x0000`, reserved `0x0001`, bad `0xFFF7`, EOC `0xFFF8..0xFFFF`.

Type is decided **solely by cluster count** (Microsoft FAT spec; fat12 brief
Sec 2): `< 4085` => FAT12, `4085..65524` => FAT16. The `fs_type` string in the
extended BPB is informational and MUST NOT be trusted.

Consequence for the artifact: the FAT12 reader's higher-level walkers
(`read_file`, `read_partial`, `read_dir`, `resolve_path`, directory scans) are
already *parameterized by* the link-decode primitive `fat12_next_cluster` and
the classify predicates (`fat12_is_eoc/_free/_bad`) + the data-cluster bounds
(`fat12_cluster_in_range` / `fat12_last_cluster`). If those primitives dispatch
on `vol->fat_type`, every higher-level walk is correct for both FAT types with
ZERO duplicated walk logic -- and the FAT12 path stays byte-identical when
`fat_type == FAT_TYPE_FAT12` (the 12-bit branch runs unchanged).

---

## 1. BPB Layout -- a real `mkfs.fat -F 16` image

Image minted this session:

```bash
dd if=/dev/zero of=fat16gt.img bs=512 count=16384 status=none
mkfs.fat -F 16 -s 1 -S 512 -r 512 -R 1 fat16gt.img    # 8 MB, 1 sector/cluster
mcopy -i fat16gt.img hello.txt   ::HELLO.TXT          # 18 bytes
mcopy -i fat16gt.img chain16.txt ::CHAIN16.TXT        # 1620 bytes -> 4 clusters
mcopy -i fat16gt.img big16.bin   ::BIG16.BIN          # 10240 bytes -> 20 clusters
```

All values below are ACTUAL bytes read from the produced image via
`struct.unpack_from` (little-endian throughout).

### Boot sector prefix + BPB proper

| Offset | Size | Field | Actual value | Notes |
|--------|------|-------|--------------|-------|
| 0x00 | 3 | Jump instruction | `EB 3C 90` | JMP short + NOP |
| 0x03 | 8 | OEM name | `mkfs.fat` | tool string |
| 0x0B | 2 | bytes_per_sector | 512 | same as FAT12 |
| 0x0D | 1 | sectors_per_cluster | 1 | one sector per cluster |
| 0x0E | 2 | reserved_sectors | 1 | boot sector only |
| 0x10 | 1 | num_fats | 2 | two redundant FATs |
| 0x11 | 2 | root_entry_count | 512 | fixed root dir (32 sectors) |
| 0x13 | 2 | total_sectors_16 | 16384 | fits in 16 bits, used |
| 0x15 | 1 | media_descriptor | 0xF8 | fixed-disk descriptor (NOT 0xF0) |
| 0x16 | 2 | sectors_per_fat | 64 | each FAT = 64 sectors (16384 entries) |
| 0x18 | 2 | sectors_per_track | 32 | CHS geometry |
| 0x1A | 2 | num_heads | 2 | CHS geometry |
| 0x1C | 4 | hidden_sectors | **0** | NON-PARTITIONED (z01 scope) |
| 0x20 | 4 | total_sectors_32 | 0 | unused (total_sectors_16 != 0) |

### Extended BPB (offset 0x24)

| Offset | Size | Field | Actual value |
|--------|------|-------|--------------|
| 0x24 | 1 | drive_number | 0x80 (fixed disk; floppy was 0x00) |
| 0x26 | 1 | boot_signature | 0x29 |
| 0x27 | 4 | volume_id | 0x5287EDE5 |
| 0x2B | 11 | volume_label | `NO NAME    ` |
| 0x36 | 8 | fs_type_string | `FAT16   ` (informational only) |

Boot signature at 0x1FE: `0xAA55` (valid).

**Media descriptor note:** a fixed disk uses 0xF8 (vs 0xF0 for the 1.44 MB
floppy). `media_descriptor` is NOT used by the mount/geometry path (the FAT12
mount never reads it for geometry; classification is by cluster count), so no
code change is needed for the 0xF8 value. `hidden_sectors == 0` is the
non-partitioned invariant z01 requires; a partitioned image would have a
nonzero hidden_sectors and is OUT of scope (kzfs/DEC-07).

---

## 2. Derived Geometry -- identical formulas, FAT16 cluster count

Using the actual values above (`spec/dos_structs.h` BPB_* macros):

```
first_FAT_sector   = reserved_sectors                          = 1
second_FAT_sector  = reserved_sectors + sectors_per_fat        = 65
root_dir_sector    = reserved_sectors + num_fats*sectors_per_fat
                   = 1 + 2*64                                   = 129
root_dir_sectors   = ceil(root_entry_count*32 / bytes_per_sector)
                   = ceil(512*32 / 512)                         = 32
first_data_sector  = root_dir_sector + root_dir_sectors
                   = 129 + 32                                   = 161
total_data_sectors = total_sectors_16 - first_data_sector
                   = 16384 - 161                                = 16223
total_clusters     = total_data_sectors / sectors_per_cluster
                   = 16223 / 1                                  = 16223
```

**FAT type by cluster count (Microsoft FAT spec rule):**
- total_clusters < 4085             -> FAT12
- **4085 <= total_clusters < 65525  -> FAT16   (16223 -> CONFIRMED FAT16)**
- total_clusters >= 65525           -> FAT32 (out of scope)

**Cluster-to-LBA mapping (cluster-2 bias -- IDENTICAL to FAT12):**

```
LBA(cluster N) = first_data_sector + (N - 2) * sectors_per_cluster
               = 161 + (N - 2) * 1
```
- Cluster 2 -> LBA 161 (first data sector)
- Cluster 3 -> LBA 162

This cluster-2 bias is the same `BPB_CLUSTER_LBA` macro FAT12 uses; omitting it
(M4) reads the wrong sector for every cluster.

---

## 3. FAT16 16-Bit Entry Decode Rule

Each FAT16 entry is a single little-endian `uint16_t`. For cluster N:

```
byte_offset = N * 2          (NOT (N*3)/2 -- that is FAT12)
entry[N]    = fat[byte_offset] | (fat[byte_offset+1] << 8)    (little-endian u16)
```

There is **no even/odd case and no 12-bit mask** -- using `& 0x0FFF` (M2) would
corrupt every entry >= 0x1000 (i.e. every cluster pointer once the volume
exceeds 4095 clusters, which a FAT16 volume always does).

### Worked examples from ACTUAL FAT bytes of the minted image

FAT region begins at sector 1 (byte 512). Reserved entries:

```
FAT[0] = FFF8     (media descriptor 0xF8 widened to the FAT16 reserved slot)
FAT[1] = FFFF     (EOC marker, always)
```

CHAIN16.TXT, start_cluster 3, size 1620 (1620 / 512 = 3.16 -> 4 clusters,
last partial -- the RISK-5 case):

```
cluster 3 (byte_off  6): bytes 04 00 -> entry = 0x0004  -> next cluster 4
cluster 4 (byte_off  8): bytes 05 00 -> entry = 0x0005  -> next cluster 5
cluster 5 (byte_off 10): bytes 06 00 -> entry = 0x0006  -> next cluster 6
cluster 6 (byte_off 12): bytes FF FF -> entry = 0xFFFF  -> EOC (>= 0xFFF8)
```
Chain: 3 -> 4 -> 5 -> 6 -> EOC. Confirmed by walking with the python reader.

BIG16.BIN, start_cluster 7, size 10240 (10240 / 512 = exactly 20 clusters):

```
cluster 7 (byte_off 14): bytes 08 00 -> entry = 0x0008  -> next cluster 8
...
chain: 7 -> 8 -> 9 -> ... -> 26 -> EOC (20 clusters, no partial last cluster)
```

HELLO.TXT, start_cluster 2, size 18 (single cluster):

```
cluster 2 (byte_off 4): bytes FF FF -> entry = 0xFFFF -> EOC (single-cluster file)
```

### Special entry values (FAT16) -- Microsoft FAT spec Sec 3.2

| Value | Meaning |
|-------|---------|
| 0x0000 | Free cluster |
| 0x0001 | Reserved (do not use) |
| 0x0002-0xFFEF | Next cluster in chain |
| 0xFFF0-0xFFF6 | Reserved |
| 0xFFF7 | Bad cluster (do not use) |
| 0xFFF8-0xFFFF | End of chain (EOC); mkfs.fat writes 0xFFFF |

The EOC THRESHOLD is `0xFFF8`, NOT `0xFF8` (M3): a 16-bit chain pointer like
0x0FF8..0x0FFF is a perfectly normal cluster index on FAT16; treating it as EOC
truncates large files mid-chain.

---

## 4. Directory Entry Read Semantics -- IDENTICAL to FAT12

The 32-byte `dir_entry_t` layout, the fixed root-directory region
(`root_dir_sector .. + root_dir_sectors`, `root_entry_count` entries), the
0x00/0xE5 sentinels, the attribute filtering (0x0F LFN, 0x08 vol-label, 0x10
directory), the 0x05->0xE5 first-byte alias, and `file_size`-is-authoritative
(RISK-5: no trailing padding on the last cluster) are ALL identical to FAT12
(fat12 brief Sec 4). No FAT16-specific directory handling is needed; only the
chain-link decode differs.

---

## 5. Differential Oracle Reference Commands

Same three-way differential structure as `test-fat` (FAT12): our reader vs
mtools vs an independent python reader, on a freshly-minted FAT16 fixture image.

### Directory listing (golden):

```bash
mdir -i fat16gt.img ::                 # names + sizes (date/time/serial normalized away)
```

### File content extraction (golden):

```bash
mcopy -i fat16gt.img ::CHAIN16.TXT -   # raw content bytes
```

### Independent python3 FAT16 reader (oracle reference #2):

A SEPARATE `fat16_ref.py` (NOT the 12-bit `fat12_ref.py`, NOT our C) decodes the
16-bit entry from first principles:

```python
def fat16(n):                          # entry N: little-endian u16 at byte N*2
    return struct.unpack_from("<H", fat, n * 2)[0]
EOC_MIN = 0xFFF8
BAD     = 0xFFF7
```

### Normalization for diffs:
- Strip volume-label entries (attr 0x08) from listings.
- Normalize/ignore mtime/mdate + the volume serial (implementation/host-clock
  dependent) -- compare only name / size / cluster / content.
- Compare file content byte-for-byte (no normalization).

---

## 6. Mutation Targets (Rule 6 -- each MUST turn the FAT16 oracle RED)

These prove the FAT16-specific code is load-bearing:

- **M1 off-by-one entry offset** -- `byte_offset = N*2 + 1` (or `N*2 - 1`):
  reads a misaligned word, every chain link is garbage -> RED.
- **M2 treat the FAT16 entry as 12-bit** -- mask `& 0x0FFF`: any cluster
  pointer >= 0x1000 (all of them past cluster 4095, and EOC 0xFFFF -> 0x0FFF
  which is NOT >= 0xFFF8) corrupts -> RED.
- **M3 wrong EOC threshold** -- use `0xFF8` instead of `0xFFF8`: a normal
  cluster pointer in 0x0FF8..0x0FFF is wrongly seen as EOC; large files
  truncate -> RED.
- **M4 cluster-2 LBA bias omitted** -- `LBA = first_data + N*spc` (drop `-2`):
  every cluster maps two sectors too high -> wrong bytes -> RED.
- **M5 fat_type not classified** -- run the FAT12 12-bit decode on the FAT16
  image (force `fat_type = FAT12` at mount): the nibble-packed decode produces
  garbage chains -> RED.

---

## 7. Scope guardrails (operator-decided; do NOT exceed)

- READ-ONLY. FAT16 write is deferred to 509.11. The `fat16_*` functions added
  are read-only; no FAT16 write/allocate path.
- NON-PARTITIONED single volume (hidden_sectors == 0), mounted on the slave
  disk like the FAT12 data disk. Partitioned FAT16 / MBR / hidden_sectors
  offset is kzfs/DEC-07 -- OUT.
- initrd/tarfs -- OUT (separate discovery bead).
- multi-volume / drive letters -- OUT (defer to slvd).
- Within ADR-0003 DEC-07 (FAT16 for fixed-disk) -- NO new ratified ADR needed.
