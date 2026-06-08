# Ground-Truth Brief: FAT12 Mount over ATA, SFT/JFT Handle Layer, and File-Handle INT 21h Functions

**Scope:** The next milestone (initech-509.3 + initech-saw + initech-adf remainder) —
mounting the FAT12 data volume via ATA, the SFT/JFT indirection layer (DEC-06), and the
INT 21h file-handle functions that make DIR and TYPE possible.

**Law 1 citations throughout.** All source references are file:line or section IDs.

---

## 1. QEMU Second-Disk Topology and ATA Drive-Select

### 1.1 Current Topology

`harness/emu/qemu.c:371-377` produces a single `-drive format=raw,file=<img>` argument
from `QemuConfig.disk_path`. With no `if=` or `index=` qualifier, QEMU maps this to the
IDE primary master (hd0). That is the boot disk — it carries the MBR / stage2 / kernel
(built by `make image` into `build/initech.img`, Makefile:635-638). The boot disk is NOT
a FAT12 volume that fat12_mount can parse; it is a raw image with MBR at sector 0.

The FAT12 data volume must be a SEPARATE physical disk. Nothing in the current harness
accommodates a second disk.

### 1.2 QEMU -drive Index Semantics (from QEMU 6+ documentation; ATA/ATAPI-6 port map)

QEMU maps the `if=ide,index=N` arguments to ATA positions:

| index | ATA position | Port base | Drive-select byte (bit 4) |
|-------|-------------|-----------|--------------------------|
| 0 | Primary master | 0x1F0..0x1F7 | `0xE0` (bit 4 = 0) |
| 1 | Primary slave  | 0x1F0..0x1F7 | `0xF0` (bit 4 = 1) |
| 2 | Secondary master | 0x170..0x177 | `0xE0` on secondary |
| 3 | Secondary slave  | 0x170..0x177 | `0xF0` on secondary |

The boot disk is already at `index=0` (primary master). The simplest, lowest-risk choice
for the FAT12 data volume is **primary slave** (`index=1`, same port base 0x1F0, bit 4 of
drive-select register 0x1F6 toggled from 0 to 1).

**Recommended QEMU args for the second disk:**

```
-drive file=build/fat_data.img,format=raw,if=ide,index=1
```

This produces:
- Primary master (index=0): `build/initech.img` (boot disk — existing, unchanged)
- Primary slave  (index=1): `build/fat_data.img` (FAT12 data volume)

A secondary-channel variant (`index=2`) would work but requires a second pair of I/O
ports (0x170/0x376); the primary-slave route reuses the already-implemented port base and
changes only the drive-select bit — the minimal change.

### 1.3 ata.c Changes Required

`os/milton/ata.c:43` defines `ATA_DRIVE_MASTER_LBA = 0xE0u` (LBA mode, master, top-4
LBA-bits = 0 in `ata_read_sectors`). `os/milton/ata.c:103` writes:

```c
outb(ATA_PRI_DRIVE, (uint8_t)(ATA_DRIVE_MASTER_LBA | ((lba >> 24) & 0x0Fu)));
```

For the primary slave only the drive-select bit (bit 4) changes:

```c
#define ATA_DRIVE_MASTER_LBA  0xE0u  /* LBA, master (bit 4 = 0) */
#define ATA_DRIVE_SLAVE_LBA   0xF0u  /* LBA, slave  (bit 4 = 1) */
```

The blockdev context (`blockdev_t.ctx`, currently `NULL` for the master) should carry a
small ATA descriptor so the read function knows which port base and which drive bit to
use:

```c
/* Recommended: a tiny context struct embedded by the kernel for each drive. */
typedef struct {
    uint16_t port_base;    /* 0x1F0 primary, 0x170 secondary */
    uint8_t  drive_select; /* ATA_DRIVE_MASTER_LBA or ATA_DRIVE_SLAVE_LBA */
} ata_ctx_t;
```

`ata_read_sectors` would read `port_base` and `drive_select` from `ctx` instead of using
the compile-time constants. `ata_blockdev_init` gains a channel+drive parameter:

```c
void ata_blockdev_init(blockdev_t *dev, ata_ctx_t *ctx, uint16_t port_base, uint8_t drive_sel);
```

The caller in `kmain.c` / `IO.SYS` initialises two `ata_ctx_t` instances and two
`blockdev_t`s: one for the boot/read path (primary master), one for the FAT volume
(primary slave).

**No other ata.c logic changes.** The polling loop in `ata_wait_drq` reads the STATUS
register from `port_base + 7`; the data register is `port_base + 0`. Both shift cleanly
if `port_base` becomes a variable. The device-control register for the primary channel is
at `0x3F6`; for secondary it is `0x376` — if a soft-reset is ever needed, this would also
need to be parameterized, but the PIO READ path does not issue a reset, so this is a
non-issue for this milestone.

### 1.4 Harness Changes Required

**`harness/emu/qemu.h`:** Add `data_disk_path` to `QemuConfig`:

```c
const char *data_disk_path; /* FAT12 data volume (second -drive, primary slave); NULL if not used */
```

**`harness/emu/qemu.c:build_argv`:** After the boot-disk `-drive` push (qemu.c:372-377),
add conditionally:

```c
if (cfg->data_disk_path) {
    static char drvbuf2[QEMU_PATH_MAX + 48];
    snprintf(drvbuf2, sizeof(drvbuf2),
             "file=%s,format=raw,if=ide,index=1", cfg->data_disk_path);
    PUSH("-drive");
    PUSH(drvbuf2);
}
```

**`harness/emu/qemu_main.c`:** Add `--disk2 IMG` option that sets `cfg.data_disk_path`.

**Makefile:** Mint the FAT12 data image (`build/fat_data.img`) alongside
`build/initech.img`. The minting recipe is identical to the existing `$(FAT12_IMG)` rule
at Makefile:253-261 (mformat -f 1440, mcopy the test files). The boot-oracle make targets
that test FAT-over-ATA pass both `--disk` and `--disk2` to the harness.

---

## 2. First-Time ATA Validation on the Emulator

### 2.1 ata.c Has Never Exercised I/O Ports

`os/milton/ata.h:8-10` is explicit: "CANNOT be host-unit-tested — it touches I/O ports
0x1F0..0x1F7." The beads note in initech-adf confirms: "hardware validation DEFERS to M1
boot." This is the first real validation.

### 2.2 Minimal Oracle

The cheapest proof: call `fat12_mount()` on the primary-slave blockdev immediately after
entering protected mode (in `kmain.c`, after IDT/PIC init). `fat12_mount`
(`os/milton/fat12.c:33`) calls `dev->read_sectors(ctx, 0, 1, buf)` which invokes
`ata_read_sectors` for sector 0, then validates:
1. Boot signature at offset 510 == `0xAA55` (`fat12.h:55`).
2. `bytes_per_sector == 512` (`fat12.c:73`).
3. Geometry sanity (`fat12.c:80-95`).

If all pass, `fat12_mount` returns `FAT12_OK`. The kernel then emits a serial marker
`ATA-OK` / `FAT-MOUNT-OK`. The QEMU oracle `--expect "FAT-MOUNT-OK"` makes this a
machine-checkable pass/fail. No additional test code is needed beyond wiring the mount
call.

### 2.3 Failure Modes to Handle Loudly (Rule 2)

**Floating bus (no drive present):** If the primary slave slot is empty, reading the
status register returns `0xFF` (floating bus). The current `ata_wait_drq` loop
(`ata.c:74`) spins until `BSY` clears; if `STATUS == 0xFF`, `BSY` is set (bit 7), so it
spins forever. The fix: after the drive-select write, read the STATUS register once; if it
returns `0xFF` the drive is absent — return an error immediately (fail loud, Rule 2).
Typical code:

```c
/* After outb(drive_reg, drive_sel): read STATUS; 0xFF = no drive (floating bus). */
if (inb(status_reg) == 0xFFu) return ATA_ERR_NO_DRIVE;
```

**400 ns delay:** After writing the drive/head register, the ATA spec requires 400 ns
before reading STATUS again (the device needs time to assert BSY). In QEMU this is
instantaneous, but on real hardware (86Box) it matters. The standard technique is to read
the STATUS register four times and discard the result (each `inb` ~100 ns on an ISA bus):

```c
/* Four STATUS reads = ~400 ns delay on real ISA hardware. QEMU: no-op. */
inb(status_reg); inb(status_reg); inb(status_reg); inb(status_reg);
```

This is also required after writing the device-select; add it before the BSY poll.

**DRQ never set:** If BSY clears but DRQ is not set and ERR/DF is not set, the drive
accepted the command but never started data transfer. This is a genuine hardware
anomaly — return an error. `ata_wait_drq` at `ata.c:81-83` already handles this case.

**ERR bit set:** If `STATUS & (ATA_SR_ERR | ATA_SR_DF)` is non-zero, read the ERROR
register (`port_base + 1`) and emit it on serial for diagnosis. `ata.c:78-80` already
returns -1; add the serial error dump to fail loud.

**LBA28 vs CHS:** QEMU's PIIX4 IDE controller fully supports LBA28 for the sector counts
involved (1.44 MB = 2880 sectors, well within LBA28's 28-bit range). No CHS fallback is
needed. The drive-select register LBA-bit (bit 6, value 0x40) must be set; the current
`0xE0` / `0xF0` constants include it (bit 6 = 1, bit 5 = 1 = always-1, bit 4 = master/slave).

**QEMU PIIX4 quirk (from QEMU source and experience):** QEMU does NOT require the 400 ns
delay, but it does require that the sector count register be written before LBA fields. The
current ata.c:104-108 write order is correct (DRIVE first, then SECCOUNT, then LBA
fields, then COMMAND). No quirks expected for single-sector reads at small LBAs.

**Bochs conformance check (Rule 5):** Bochs has strict ATA emulation and will reject an
out-of-order register write sequence. The current write order in `ata.c:103-108` is
architecturally correct (ATA/ATAPI-6: write device register, sector count, LBA bytes,
then command). Run the ATA validation boot test on Bochs too before declaring it green.

---

## 3. The SFT/JFT Model (ADR-0003 DEC-06)

### 3.1 Conceptual Model

**Ref:** ADR-0003 §5.6 (DEC-06): "A per-process Job File Table of twenty (20) entries
shall map process file handles to entries in a system-wide System File Table, the capacity
of which shall be governed by the `FILES=` directive of `CONFIG.SYS`."
`CONFIG.SYS` baseline (ADR-0003 Appendix D.2): `FILES=20`.

The JFT is the per-process index into the SFT. The SFT is the kernel-wide open-file
registry. A process handle (0..19) is an index into the process's JFT (psp_t.jft[20] at
PSP offset 0x18); each JFT byte is either 0xFF (closed/free) or a 0-based index into the
kernel SFT array.

**Existing state:**
- `spec/dos_structs.h:97`: `uint8_t jft[20]` at offset 0x18 in `psp_t`.
- `os/milton/psp.c:131-135`: `jft[0]=0x00, jft[1]=0x01, jft[2]=0x01, jft[3..19]=0xFF`.
- `os/milton/int21.c:167-173`: AH=40h currently checks `handle == 1 || handle == 2`
  directly (no SFT lookup yet).
- Psp.c comment at line 128: "The real SFT backing is initech-509.3."

### 3.2 Recommended SFT Entry Structure (sft_entry_t)

```c
/* Discriminator: is this entry backed by a device or a file? */
typedef enum {
    SFT_KIND_FREE   = 0,
    SFT_KIND_DEVICE = 1,
    SFT_KIND_FILE   = 2
} sft_kind_t;

/* Device IDs for the predefined character devices. */
typedef enum {
    SFT_DEV_CON = 0, /* CON (keyboard in / screen out) */
    SFT_DEV_AUX = 1, /* AUX (COM1)                     */
    SFT_DEV_PRN = 2  /* PRN (LPT1)                     */
} sft_dev_id_t;

typedef struct sft_entry {
    sft_kind_t  kind;          /* FREE / DEVICE / FILE                       */
    uint16_t    ref_count;     /* number of JFT slots pointing here (for DUP) */
    uint8_t     open_mode;     /* AL from OPEN: 0=read, 1=write, 2=rdwr       */

    /* --- FILE state (valid when kind == SFT_KIND_FILE) --- */
    dir_entry_t dir_entry;     /* copy of the 32-byte FAT dir entry at open time */
    uint32_t    file_offset;   /* current byte offset (moves on READ/WRITE/LSEEK) */
    /* fat12_volume_t *vol; -- pointer to the mounted volume; one global for now */

    /* --- DEVICE state (valid when kind == SFT_KIND_DEVICE) --- */
    sft_dev_id_t dev_id;      /* which character device                      */
} sft_entry_t;
```

**SFT array size:** `FILES=20` (ADR-0003 Appendix D.2) governs `total_sft_entries`.
DOS allocates the SFT for `FILES=N` entries. However, handles 0-4 consume 3 SFT entries
for CON (stdin shares the same SFT slot as stdout; stderr aliases stdout; AUX and PRN are
separate device entries). With `FILES=20`, the SFT holds 20 entries total (slots 0..19).
Slots 0 and 1 are pre-assigned to CON-read and CON-write devices; slot 2 to AUX, slot 3
to PRN. Slots 4..19 are available for file opens. A static array of 20 entries in the
kernel BSS is sufficient:

```c
#define SFT_MAX_ENTRIES 20   /* FILES=20 per CONFIG.SYS baseline (ADR-0003 App D.2) */
static sft_entry_t g_sft[SFT_MAX_ENTRIES]; /* kernel-global; zero-init by C runtime */
```

### 3.3 Predefined Handles 0-4 at Startup

ADR-0003 DEC-06: "Handles 0 through 4 shall be predefined (standard input, standard
output, standard error, standard auxiliary, standard printer)."

The psp.c-established JFT (`psp.c:131-135`) already sets:
- `jft[0] = 0x00` → SFT slot 0 = CON (stdin, device read)
- `jft[1] = 0x01` → SFT slot 1 = CON (stdout, device write)
- `jft[2] = 0x01` → SFT slot 1 = CON (stderr, same SFT slot as stdout — DOS convention)
- `jft[3..19] = 0xFF` (closed)

The psp.c comment at line 128 explicitly defers AUX and PRN: "AUX/PRN are deferred per
509.7, so NOT given SFT slots."

For this milestone, extend the JFT and SFT to cover AUX (handle 3 → SFT slot 2,
`SFT_KIND_DEVICE, SFT_DEV_AUX`) and PRN (handle 4 → SFT slot 3,
`SFT_KIND_DEVICE, SFT_DEV_PRN`), populating `psp.c:133` to write
`jft[3]=0x02, jft[4]=0x03`. These are vestigial for now (no AUX/PRN hardware driver)
but must be in the JFT to satisfy DEC-06 ("implement vestigial structures IN FULL",
CLAUDE.md design stance). AUX/PRN SFT entries carry `SFT_KIND_DEVICE` with the
appropriate `dev_id`; any READ/WRITE to them returns an appropriate error until the device
driver milestone.

The SFT initialization runs during kernel startup (SYSINIT, DEC-01), before the first
process loads. It sets:

```c
g_sft[0] = (sft_entry_t){ .kind=SFT_KIND_DEVICE, .ref_count=2, .dev_id=SFT_DEV_CON };
/* ref_count=2: handles 0 (stdin) and 1 (stdout) both point here; plus stderr=1 makes 3
   logically, but stdin[jft[0]=0] and stdout[jft[1]=1] point to DIFFERENT SFT slots:
   slot 0 for stdin (device read), slot 1 for stdout/stderr (device write). */
g_sft[0] = (sft_entry_t){ .kind=SFT_KIND_DEVICE, .ref_count=1, .dev_id=SFT_DEV_CON, .open_mode=0 }; /* stdin */
g_sft[1] = (sft_entry_t){ .kind=SFT_KIND_DEVICE, .ref_count=2, .dev_id=SFT_DEV_CON, .open_mode=1 }; /* stdout+stderr */
g_sft[2] = (sft_entry_t){ .kind=SFT_KIND_DEVICE, .ref_count=1, .dev_id=SFT_DEV_AUX, .open_mode=2 }; /* AUX */
g_sft[3] = (sft_entry_t){ .kind=SFT_KIND_DEVICE, .ref_count=1, .dev_id=SFT_DEV_PRN, .open_mode=1 }; /* PRN */
/* g_sft[4..19] are zero (SFT_KIND_FREE) at C init. */
```

### 3.4 DUP (45h) and DUP2 (46h)

**DUP (45h):** Duplicate handle `EBX` to the lowest free JFT slot. Algorithm:
1. Validate `EBX` is a valid open handle (JFT[EBX] != 0xFF, JFT[EBX] < SFT_MAX_ENTRIES,
   g_sft[JFT[EBX]].kind != SFT_KIND_FREE).
2. Find the lowest `i` in 0..19 where `JFT[i] == 0xFF`.
3. Set `JFT[i] = JFT[EBX]`; increment `g_sft[JFT[EBX]].ref_count`.
4. Return `EAX = i`, CF clear.
On error (no free JFT slot, invalid EBX): CF=1, AX=0x0004 (too many open files) or
0x0006 (invalid handle). Per DEC-04a.3 (ADR-0003 Amendment §E.1).

**DUP2 (46h):** Force handle `ECX` to alias handle `EBX`. Algorithm:
1. Validate `EBX` (open source handle) and `ECX` (target handle index, 0..19).
2. If `JFT[ECX] != 0xFF` (target is open): decrement `g_sft[JFT[ECX]].ref_count`; if
   ref_count reaches 0, mark SFT slot as FREE.
3. Set `JFT[ECX] = JFT[EBX]`; increment `g_sft[JFT[EBX]].ref_count`.
4. Return CF clear. (The COMMAND.COM shell uses DUP2 to redirect stdout to a file:
   DUP2(EBX=file_handle, ECX=1) makes handle 1 point at the file's SFT slot.)

**Design stance:** Implement DUP and DUP2 in full even though no shell yet uses them
(CLAUDE.md: "implement vestigial structures IN FULL"). DUP2 is the kernel primitive for
I/O redirection; it must be correct before COMMAND.COM arrives.

---

## 4. File-Handle INT 21h Functions

All conventions are governed by DEC-04a.3 (ADR-0003 Amendment §E.1, §3.3):
AH=function, EBX=handle, ECX=count, EDX=flat ptr, EAX=return, CF=error.
The dispatcher is `os/milton/int21.c`; AH=3Ch-3Fh/42h/4Eh/4Fh/45h/46h are already in
`ah_is_listed()` (int21.c:95-110) as "LISTED-but-deferred." This milestone implements
them.

### 4.1 AH=3Dh OPEN

**Registers:** AH=3Dh, EDX=flat ptr to ASCIIZ path, AL=open mode (0=read, 1=write,
2=rdwr). Returns EAX=handle (JFT index) CF clear, or CF=1, AX=error code.

**Implementation sketch:**
1. Validate `EDX` non-null, path non-empty.
2. Call `fat12_find(vol, sector_buf, (const char*)EDX, &dir_entry)`. On
   `FAT12_ERR_NOT_FOUND` → CF=1, AX=0x0002 (file not found).
3. Find a free SFT slot: lowest `i` in 4..19 where `g_sft[i].kind == SFT_KIND_FREE`.
   If none → CF=1, AX=0x0004 (too many open files).
4. Find a free JFT slot: lowest `j` in 0..19 where `JFT[j] == 0xFF`.
   If none → CF=1, AX=0x0004.
5. Populate `g_sft[i]`:
   ```c
   g_sft[i].kind        = SFT_KIND_FILE;
   g_sft[i].ref_count   = 1;
   g_sft[i].open_mode   = AL;
   g_sft[i].dir_entry   = dir_entry;  /* struct copy */
   g_sft[i].file_offset = 0;
   ```
6. Set `JFT[j] = (uint8_t)i`. Return `EAX = j`, CF clear.

**Scope restriction this milestone:** ASCIIZ path must be a plain 8.3 root-directory
name (no drive letter prefix, no subdirectory path, no wildcards). Path traversal and
subdirectory resolution are deferred. Reject a path with `\` or `:` with
CF=1, AX=0x0003 (path not found) / AX=0x000F (invalid drive).

**Ref:** DOS 3.3 Programmer's Reference Manual AH=3Dh; ADR-0003 Appendix A:3Dh OPEN;
DEC-04a.3 (ABI). `fat12_find` declaration: `os/milton/fat12.h:256`.

### 4.2 AH=3Fh READ

**Registers:** AH=3Fh, EBX=handle, ECX=count, EDX=flat ptr to buffer. Returns EAX=bytes
actually read, CF clear; or CF=1, AX=error code.

**Whole-file-read vs. positioned-read decision (THE key architectural choice):**

`fat12_read_file` (`os/milton/fat12.h:279`) reads the ENTIRE file into a caller-provided
buffer and returns `file_size` bytes. It does not support a `(offset, count)` partial
read. For INT 21h AH=3Fh, the caller expects to read `count` bytes from the current file
offset and have the offset advance.

**Recommendation: implement positioned partial-read at the SFT layer, not inside
fat12.c.** Rationale:
- `fat12_read_file` is oracle-green and mutation-proven (initech-adf). Do not change it.
- The SFT entry already holds `file_offset`. A partial read can be served by:
  1. Walk the cluster chain via `fat12_walk_chain` (already implemented, no I/O for the
     walk itself if the FAT is cached).
  2. Seek to the cluster containing `file_offset`; read only the required sectors.

However, this requires sector-granular cluster positioning, which is new code. **For this
milestone only**, a simpler approach is acceptable:

**Milestone-scope approach (whole-file buffering):**
- At OPEN time, read the entire file into a kernel-owned buffer (a static scratch region
  or a fixed "file read buffer" in the memory map). Store a pointer and length alongside
  the SFT entry.
- AH=3Fh READ: copy `count` bytes from `file_offset` into the caller's buffer; advance
  `file_offset` by bytes actually copied (capped at `file_size - file_offset`); return
  `EAX = bytes_copied`.

This is correct for small files (the test files in the FAT fixture are a few hundred bytes
each) and supports TYPE. The limitation is that a file larger than the scratch buffer
cannot be opened — which is acceptable at this milestone given no large-file programs
exist yet. Document the limit explicitly.

**For a real read path (post-milestone):** Implement `fat12_read_partial(vol, fat,
fat_len, entry, offset, count, buf)` that navigates the cluster chain to `offset /
bytes_per_cluster`, reads whole clusters, and copies the requested slice. This is the
correct positioned-read primitive.

**Implementation sketch (milestone-scope):**
```c
static void do_read(int_frame_t *f) {
    uint32_t handle = f->ebx;
    uint32_t count  = f->ecx;
    uint8_t *buf    = (uint8_t *)(uintptr_t)f->edx;
    /* 1. Validate handle -> JFT -> SFT entry (kind == FILE or DEVICE) */
    /* 2. If DEVICE (handle 0, CON): call con_read (keyboard). For this
          milestone, CON read is not implemented; return 0 bytes or err. */
    /* 3. If FILE: compute available = file_size - file_offset. */
    /*    take = min(count, available). */
    /*    memcpy(buf, file_data_ptr + file_offset, take). */
    /*    sft_entry.file_offset += take. */
    /*    f->eax = take; cf_clear(f). */
}
```

**Ref:** DOS 3.3 Programmer's Reference Manual AH=3Fh; `os/milton/fat12.h:258-281`
(`fat12_read_file`); DEC-04a.3 ABI.

### 4.3 AH=3Eh CLOSE

**Registers:** AH=3Eh, EBX=handle. Returns CF clear on success; CF=1, AX=0x0006
(invalid handle) on error.

**Implementation sketch:**
1. Validate handle (JFT[EBX] != 0xFF, SFT slot valid).
2. Decrement `g_sft[JFT[EBX]].ref_count`.
3. If `ref_count == 0`, mark SFT slot `SFT_KIND_FREE` (and free any file data buffer).
4. Set `JFT[EBX] = 0xFF`. CF clear.

Do NOT close predefined handles 0-4 (handle those as a no-op / CF clear, as real DOS
does). Ref: DOS 3.3 Programmer's Reference Manual AH=3Eh.

### 4.4 AH=42h LSEEK

**Registers:** AH=42h, EBX=handle, ECX=offset_high (0 for this milestone), EDX=offset_low
(signed or unsigned per mode), AL=mode (0=from start, 1=from current, 2=from end).
Returns EAX=new absolute offset, CF clear; or CF=1, AX=error.

**Implementation sketch:**
```c
switch (AL) {
    case 0: sft->file_offset = EDX; break;
    case 1: sft->file_offset += (int32_t)EDX; break;
    case 2: sft->file_offset = sft->dir_entry.file_size + (int32_t)EDX; break;
    default: CF=1, AX=0x0001; return;
}
/* Clamp to [0, file_size] -- seeking past end is allowed in DOS; reading
   past end returns 0 bytes. */
f->eax = sft->file_offset; cf_clear(f);
```

Ref: DOS 3.3 Programmer's Reference Manual AH=42h; ECX is officially the high 16 bits of
a 32-bit offset — treat it as zero for this milestone (files fit in 32 bits).

### 4.5 AH=4Eh FINDFIRST / AH=4Fh FINDNEXT

**Purpose:** Directory listing. `COMMAND.COM DIR` uses these to enumerate files.

**Registers (FINDFIRST):** AH=4Eh, EDX=flat ptr to ASCIIZ file spec (wildcard),
ECX=attribute mask. Sets the DTA with the first matching entry.
**Registers (FINDNEXT):** AH=4Fh, no arguments. Fills the DTA with the next match.

**DTA layout (DOS 3.3 Programmer's Reference Manual, AH=4Eh):**

The Disk Transfer Area (for FINDFIRST/FINDNEXT) is 43 bytes at the current DTA address.
In DOS the DTA defaults to PSP:0x80 (psp_t.cmd_tail) but can be moved with AH=1Ah SETDTA.
The 43-byte find-data block:

| Offset | Size | Field |
|--------|------|-------|
| 0x00   | 1    | Drive/search attribute (internal use) |
| 0x01   | 11   | Pattern (internal use: stored 8.3 search template) |
| 0x0C   | 1    | Attribute of found entry |
| 0x0D   | 2    | File time (DOS packed format) |
| 0x0F   | 2    | File date (DOS packed format) |
| 0x11   | 4    | File size (bytes) |
| 0x15   | 13   | ASCII filename+extension (formatted 8.3, NUL-terminated, max 12 chars + NUL) |

Total: 0x15 + 13 = 0x22 = 34 bytes minimum, padded to 43. The DIR program reads fields at
these fixed offsets. Lock this layout as spec-data in `spec/dos_structs.h` or a new
`spec/find_data.h`.

**FINDNEXT state:** The kernel must remember the root-directory enumeration position
between calls. For the root directory (fixed, not a cluster chain), the position is a
simple entry index (0..root_entry_count-1). Store it in a kernel-global find-state struct
(one active search at a time for this milestone — no reentrant concurrent searches):

```c
typedef struct {
    uint8_t  search_attr;        /* attribute mask from FINDFIRST ECX       */
    uint8_t  pattern[11];        /* stored 8.3 pattern (11 bytes, no dot)   */
    uint32_t next_entry_idx;     /* next dir entry index to check (0-based) */
    uint8_t  active;             /* 1 if a search is in progress            */
} find_state_t;
static find_state_t g_find;
```

**Wildcard matching:** The full DOS wildcard engine is complex (the `?` and `*` rules in
8.3 context differ from Unix). For this milestone, implement two cases only:
- `*.*` (match all regular files): `pattern` is all `?`.
- Exact 8.3 match.
Skip volume labels and subdirectories unless the search attribute includes them (ECX bit 3
for VolLabel, bit 4 for Directory). This is sufficient for `DIR`.

**Scope this milestone:** Root directory only, no subdirectory traversal, no path prefix
in the file spec. These are deferred.

**Ref:** DOS 3.3 Programmer's Reference Manual AH=4Eh/4Fh; ADR-0003 Appendix A:4Eh/4Fh.

### 4.6 AH=3Ch CREAT (deferred)

CREAT requires FAT write (`os/milton/blockdev.h:43`: `write_sectors = NULL`). Defer to
initech-509.11. For this milestone, return CF=1, AX=0x0001 (not-yet-impl, per the
existing controlled-scope diagnostic in `int21.c:254`).

---

## 5. DIR and TYPE End-to-End

### 5.1 Kernel-Side Proto-DIR (Step 1)

Before any program runs, the kernel can call `fat12_read_root_dir` directly in `kmain.c`
with a callback that serializes entry names + sizes. This is the "proto-DIR" smoke test:
no INT 21h, no SFT — just the oracle-green FAT reader exercised over the ATA backend for
the first time. Serial output:

```
HELLO.TXT     5
SECOND.TXT   12
CHAIN.TXT    ...
```

The QEMU oracle asserts these strings appear on serial. This is the first validation that
ata.c + fat12.c work together end-to-end on the emulator.

### 5.2 TYPE via INT 21h (Steps 2-4)

A test program (flat `.COM` image, loaded by the kernel loader from the FAT volume via
initech-saw) that does:

```asm
; AH=3Dh OPEN "HELLO.TXT" for read
mov eax, 0x003D0000   ; AH=3Dh, AL=0 (read)
lea edx, [filename]   ; flat ptr to "HELLO.TXT\0"
int 0x21
jc error
mov [handle], eax

; AH=3Fh READ 512 bytes
mov ah, 0x3F
mov ebx, [handle]
mov ecx, 512
lea edx, [buf]
int 0x21
jc error
; EAX = bytes read

; AH=40h WRITE to stdout (handle 1)
mov ah, 0x40
mov ebx, 1
mov ecx, eax          ; bytes read
lea edx, [buf]
int 0x21

; AH=3Eh CLOSE
mov ah, 0x3E
mov ebx, [handle]
int 0x21

; AH=4Ch EXIT
mov eax, 0x004C0000
int 0x21
```

The QEMU oracle asserts the known contents of HELLO.TXT appear on serial (written through
the AH=40h WRITE → CON path, which is already implemented in `int21.c:165-183`).

### 5.3 DIR via INT 21h

A test program calls 4Eh FINDFIRST with `*.*`, then loops 4Fh FINDNEXT, printing the
filename+size fields from the DTA via AH=09h (the `$`-terminated string path) or AH=40h.
The QEMU oracle asserts the known fixture filenames appear.

### 5.4 Test Programs

Compile as flat `.COM` images (ADR-0003 DEC-08; CLAUDE.md callout: "Flat .COM-equivalent
apps for the current release"). Two programs:

1. `TYPE.COM` — opens a hardcoded filename, reads it in a loop, writes to stdout, closes.
   This tests OPEN+READ+WRITE+CLOSE.
2. `DIR.COM` — calls FINDFIRST/FINDNEXT, writes filenames+sizes to stdout, terminates.
   This tests FINDFIRST/FINDNEXT.

Both are assembled with nasm and loaded from the FAT volume by the kernel loader (the
existing loader at `os/milton/loader.c` loads from a static image; extending it to read
from FAT is initech-saw). For the first integration test, the kernel can bake the test
program image and run it without needing full FAT-program-load, validating the INT 21h
layer independently.

---

## 6. Scope Per Step and Recommended Sequencing

### Step 0 (prerequisite, harness): Add `--disk2` to qemu_main.c + qemu.c and mint the FAT data image

- Mint `build/fat_data.img` with the existing fixture files (hello.txt, etc.).
- Add `data_disk_path` to `QemuConfig`; add the `-drive if=ide,index=1` push in
  `build_argv`.
- Verify with `qemu-system-i386 ... -drive file=initech.img,if=ide,index=0 -drive file=fat_data.img,if=ide,index=1` that QEMU launches and the boot disk is unaffected.

### Step 1 (ata.c validation): Boot test — mount FAT12 on primary slave, emit serial marker

- Extend `ata.c` with the `ata_ctx_t` context struct and a parameterised `ata_read_sectors`.
- Add the 0xFF floating-bus check and the 400 ns delay (four STATUS reads).
- In `kmain.c`: initialize a slave `ata_ctx_t + blockdev_t`; call `fat12_mount`; on
  success emit `FAT-MOUNT-OK` on serial; on failure emit the error code.
- Oracle: `make run-fat-mount` → `--disk2 fat_data.img --expect "FAT-MOUNT-OK"`. Verify
  on Bochs too (Rule 5).

### Step 2 (proto-DIR): Call fat12_read_root_dir in kmain.c, serialize filenames to serial

- No SFT, no INT 21h. Proves the full FAT read chain over ATA.
- Oracle: serial contains the known fixture filenames.

### Step 3 (SFT/JFT): Implement g_sft, sft_entry_t, predefined handles 0-4, DUP/DUP2

- Initialize `g_sft[0..3]` at SYSINIT.
- Update `int21.c:do_write` to use JFT → SFT lookup instead of the hardcoded handle
  check (preserving backward compatibility: CON writes still go to console).
- Host-testable: the SFT init and JFT lookup are pure C; write a `test_sft.c` oracle.
- DUP and DUP2: implement fully. Test: DUP(handle=1) → new handle aliases stdout SFT slot;
  DUP2(EBX=file_handle, ECX=1) → handle 1 now points at the file SFT slot.

### Step 4 (OPEN/READ/CLOSE/LSEEK): Implement AH=3Dh/3Fh/3Eh/42h in int21.c

- Use the whole-file-read approach at OPEN time (milestone-scope simplification).
- Host-testable: wire a mock blockdev that returns known file bytes; test the INT 21h
  layer without real ATA.
- Oracle: a baked TYPE.COM image runs under the emulator, reads HELLO.TXT (5 bytes:
  "Hello"), writes to stdout, serial shows "Hello". `make test-type`.

### Step 5 (FINDFIRST/FINDNEXT): Implement AH=4Eh/4Fh

- Lock the 43-byte DTA find-data layout in spec.
- Implement wildcard `*.*` and exact-match only.
- Oracle: a baked DIR.COM image lists the fixture files; serial shows their names.
  `make test-dir`.

**Defer to later milestones:**
- AH=3Ch CREAT (requires FAT write, initech-509.11).
- AH=40h WRITE to file handles (requires FAT write).
- Subdirectory traversal (paths with `\`).
- Complex wildcard matching.
- FAT16 (initech-z01).
- AUX/PRN device I/O implementation (DEC-09 device driver milestone).

---

## 7. Risks

### Risk 1 (HIGH): ata.c first run on the emulator

ata.c has never executed I/O port instructions outside compilation. The floating-bus hang
(reading 0xFF on STATUS with BSY set if no drive is present) is a guaranteed infinite loop
without the 0xFF guard. The 400 ns delay is required for real hardware (86Box) and
harmless on QEMU. The drive-select bit must be correct (0xF0 for slave, not 0xE0) or the
controller will address the master instead and read the boot disk's sector 0 (which will
fail fat12_mount's BPB check — actually a useful debugging signal, since it will fail with
FAT12_ERR_SIGNATURE or FAT12_ERR_GEOMETRY rather than silently succeed). Write the 0xFF
check, the 400 ns delay, and the extended error logging before attempting any further
integration. Validate on Bochs before declaring green (CLAUDE.md Rule 5: tri-emulator
from day one).

### Risk 2 (MEDIUM): Whole-file-read at OPEN time — stack/memory pressure

`fat12_read_file` (`fat12.c:529`) uses a stack-allocated `chain[2880]` (2880 × 2 bytes =
5760 bytes). A second on-stack array for file data at OPEN time could push the kernel
stack close to its limit. The recommended fix: allocate a static kernel file-data buffer
(one slot, large enough for the largest expected file) rather than a second stack
allocation. The memory map (`spec/memory_map.h`) must reserve space for this buffer. The
single-buffer approach is appropriate for the milestone (one file open at a time); it
forces file-open to be single-concurrent, which is fine before a multi-process shell.

### Risk 3 (MEDIUM): INT 21h reentrancy when IRQs are later unmasked

DEC-04a Consequence C-5 (ADR-0003 Amendment §5.2): "When IRQ0 (PIT timer) or IRQ1
(keyboard) are later unmasked, the INT 21h dispatcher must be audited for reentrancy."
The current dispatcher uses no locks (IRQs are all masked per DEC-04a.2). The SFT add
file-handle functions that touch global state (g_sft, g_find). This is safe now (masked
IRQs) but must be audited before unmasking IRQ0/IRQ1. File the forward obligation in a
beads issue. Do not add locks now; the cooperative model (CLAUDE.md hallucination callout:
"Cooperative, not preemptive") means reentrancy is not a live risk in this milestone.

---

## Summary of Concrete Struct Recommendations

```c
/* os/milton/sft.h (new file, artifact) */

#define SFT_MAX_ENTRIES 20          /* FILES=20 (ADR-0003 Appendix D.2) */
#define SFT_HANDLE_CLOSED 0xFFu     /* JFT sentinel: slot unused */

typedef enum { SFT_KIND_FREE=0, SFT_KIND_DEVICE=1, SFT_KIND_FILE=2 } sft_kind_t;
typedef enum { SFT_DEV_CON=0, SFT_DEV_AUX=1, SFT_DEV_PRN=2 } sft_dev_id_t;

typedef struct sft_entry {
    sft_kind_t   kind;
    uint16_t     ref_count;
    uint8_t      open_mode;    /* 0=read, 1=write, 2=rdwr */
    /* FILE fields */
    dir_entry_t  dir_entry;    /* copy of 32-byte FAT dir entry at open time */
    uint32_t     file_offset;  /* current position */
    const uint8_t *file_data;  /* pointer into the static file buffer (milestone) */
    /* DEVICE fields */
    sft_dev_id_t dev_id;
} sft_entry_t;

extern sft_entry_t g_sft[SFT_MAX_ENTRIES];  /* zero-init; populated at SYSINIT */

/* Translate a process handle (JFT index) to an SFT entry.
 * Returns NULL if handle invalid or slot is FREE.
 * psp is the current process PSP. */
sft_entry_t *sft_from_handle(const psp_t *psp, uint8_t handle);

/* Find lowest free JFT slot. Returns 0xFF if full. */
uint8_t jft_alloc(psp_t *psp);

/* Find lowest free SFT slot index. Returns SFT_MAX_ENTRIES if full. */
uint8_t sft_alloc(void);
```

```c
/* spec/find_data.h (new locked spec-data) */
/* Ref: DOS 3.3 Programmer's Reference Manual AH=4Eh DTA layout */

#define FIND_DATA_SIZE 43u

#pragma pack(push,1)
typedef struct find_data {
    uint8_t  drive_attr;    /* 0x00: internal (drive + search attribute)        */
    uint8_t  pattern[11];   /* 0x01: 11-byte 8.3 search template (no dot)       */
    uint8_t  attr;          /* 0x0C: attribute of found entry                   */
    uint16_t ftime;         /* 0x0D: time of last modification (DOS packed)     */
    uint16_t fdate;         /* 0x0F: date of last modification (DOS packed)     */
    uint32_t fsize;         /* 0x11: file size in bytes                         */
    char     fname[13];     /* 0x15: formatted 8.3 name, NUL-terminated         */
} find_data_t;
#pragma pack(pop)

_Static_assert(sizeof(find_data_t) == FIND_DATA_SIZE,
               "find_data_t must be 43 bytes (DOS 3.3 Programmer's Reference Manual AH=4Eh)");
```

---

*Law 1 citations: ADR-0003 §5.6 (DEC-06), §5.7 (DEC-07), Appendix A (function register),
Appendix B.2 (PSP/JFT), Appendix D.2 (FILES=20); ADR-0003 Amendment DEC-04a.3 (ABI
register convention); ATA/ATAPI-6 READ SECTORS PIO protocol (port map, drive-select
register bit 4, status register bits); DOS 3.3 Programmer's Reference Manual (AH=3Dh/3Eh/
3Fh/42h/4Eh/4Fh); os/milton/fat12.h (all API declarations); os/milton/fat12.c (all
implementations); os/milton/ata.c:43,103 (drive-select constant); os/milton/ata.h:27-34;
os/milton/psp.c:131-135 (JFT init); os/milton/int21.c:84-125 (ah_is_listed, listed-but-
deferred set); spec/dos_structs.h:91-107 (psp_t.jft); harness/emu/qemu.c:371-377
(disk_path/drive arg); harness/emu/qemu.h:52-54 (QemuConfig.disk_path); Makefile:253-261
(FAT12_IMG minting recipe); CLAUDE.md Law 1-4, Rule 2, Rule 5, Rule 8.*
