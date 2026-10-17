# FORMAT, SYS, diskette copy/compare and DEC-18 drive letters

## Context

Solo FORMAT/SYS lane for initech-du6v, initech-bc7p, initech-kusw and the
first half of initech-slvd. Read CLAUDE.md top to bottom, PRD 6.1/8,
bd prime/show, the JSONL issue snapshot, DEC-17, Appendix A, the FDISK
worked example, boot-system mint/loader, DOS-folder and fixture contracts.
The live database did not contain du6v; all four lane issues were read
from .beads/issues.jsonl. No bead changes, Git commands, commits, pushes,
CLAUDE.md edits or HANDOFF.md edits. No subagents, Bochs or full make test.

The PM fixed the drive mapping rather than delegating a design decision:
A=81h raw FAT or primary 1, B absent, C=80h primary 1, D=80h primary 2.
DEC-18 records that ruling; spec/drive_letters.json and the checked generated
header lock AH60h of the physical INT13 service. EBX receives five uint32
fields (disk, partition, base, sectors, flags). One kernel resolver owns
mapping and running-volume identity; utilities do not parse MBRs. General
SELDISK/GETDISK/path switching remains the second half of slvd.

## Artifact

Four flat COM images share the freestanding C diskutil core and a small
NASM entry point. Each is approximately 9 KiB; its 32 KiB copy buffer fits
the existing loader BSS reserve. Only the DEC-18 physical-service extension
changes the kernel. DOS externals are appended in DOS, bringing that exact
roster to fourteen; APPS order, input traces and purge counts are unchanged.

FORMAT writes SPC1 FAT12/FAT16 BPBs, mirrored initialized FATs and an empty
root, /V labels, /S system files and COMMAND.COM. The FAT width follows
cluster count; sizes outside DOS3.3/SPC1 are rejected before writes with
`Format requires more than one sector per cluster`. /1 /4 /8 /T /N have
mformat geometry differentials. /B reserves root slots and clusters for
the two system files with zero contents, and a later SYS fills them.
That reservation representation is an implementation choice for the
manual's space-reservation requirement, not a captured DOS /B byte golden.
A second Format-another /V iteration clears the first label when ENTER
selects no label. Fixed volumes require the manual label check and Y/ENTER.

SYS streams both source and destination FAT chains, preserves unrelated
files and the destination BPB, writes root slots 0/1 with HSR, and copies
boot code. It excludes COMMAND.COM per printed p119. Occupied first slots
and insufficient free space fail before writes. A fragmented destination
with a surviving third root entry is tested with mcopy and fsck.fat.

DISKCOPY and DISKCOMP implement the logical single-A flow, buffering 64
sectors between SOURCE/TARGET or FIRST/SECOND prompts. Host execution swaps
between separately opened image files. Copy begins with a different,
A5-filled target; byte comparison covers every sector, including the last.
DISKCOMP finds a difference in the last sector and reports side 1, track 79
from the diskette BPB, independently of the service's 63x16 translation.
Both reject fixed volumes with the two-line printed p358 message. B uses
`Invalid drive specification`. Their negative paths run in QEMU as well.

## Ground truth

Local scans in /home/tobias/Projects/initech-manuals-ref/dos33 were rendered
with pdftoppm -f N -l N -r 110 -png and visually inspected. Intermediate
page PNGs are in /tmp/formatsys-manual. Supporting OCR was used for finding
pages, not as a substitute for viewing the scans.

MS-DOS_3.3_Users_Guide_198707.pdf:

- Printed p26 / PDF39: single-drive swaps, FORMAT insertion prompt,
  DISKCOPY SOURCE prompt and `Press any key when ready . . .`.
- pp59-63 / PDF144-148: DISKCOMP switches, comparison, repeat and exit codes;
  DISKCOPY floppy-only rule, same-drive operation and repeat prompt.
- pp74-76 / PDF159-161: FORMAT switches, /B reservation, /V, fixed-disk
  label/warning/confirmation, report semantics and valid switch matrix.
- p119 / PDF204: ordered hidden system transfer, fragmented chains allowed,
  and SYS does NOT transfer COMMAND.COM.
- p318 / PDF403: two-line compare-error side/track report, Compare OK.
- pp332-337 / PDF417-422: Format-another, completion/failure, unsupported
  drive, FIRST/SECOND/TARGET and source/destination insertion messages.
- p340 / PDF425: Invalid drive specification catalogue row.
- p348 / PDF433: No room for system on destination disk.
- p358 / PDF443: System transferred and fixed-disk copy/compare refusal.

IBM 80X0945_DOS_3.30_Technical_Reference_Apr87.pdf:

- p1-7 / PDF24: system files must be first two root entries, in order.
- p2-31 / PDF58: BPB fields and media layout.
- pp5-4..8 / PDF111-115: sector layout, FAT selection, geometry media bytes,
  reserved FAT entries, FAT12 packing and chain traversal.
- p5-11 / PDF118: HSR bits, ordinary archive and volume-label attributes.

Digests:

```
MS-DOS UG b19854d8b789484c6d8bc17fbe4e4187a0f00bd2e64887dec6658c57a39798b4
IBM TR    e986bd10b6e90bdbc97dde3d5f8adfd2bd331ddf1ff962b31285375c7cd270b4
```

ADR-0003 Appendix A supplies console/exit calls, DEC-04a the flat ABI,
DEC-08 the COM model, DEC-17 physical I/O and DEC-18 the PM's drive roles.
The boot-code bytes are compared with the VBR minted by the existing
mk_boot_system.sh. Fresh mformat reference images supply BPB/FAT/root
expectations, mcopy recovers every system file, mattrib checks HSR, and
fsck.fat -n judges each formatted/transferred volume. mformat calls several
small, nonstandard SPC1 floppy layouts F0: the oracle explicitly sets the
independent IBM p5-6 geometry media literal in both the reference BPB and
FAT[0]; it does not derive that literal from artifact output.

## Red, green and mutation witnesses

The initial pre-implementation gate failed because diskutil.c was absent:
`No rule to make target 'os/milton/diskutil.c'`. Later RED-first tests caught
missing /B reservations and a stale BPB label on a second /V format.
The emulator gate and mutant recipes existed before the utility core.
All new gates and mutant wrappers are in TEST_UNIT_GATES / TEST_EMU_GATES.
No existing oracle assertion or threshold was weakened.

Exact implementation-mutant witnesses:

| Mutant | Required RED line |
|---|---|
| MEDIA, host and QEMU | RED formatsys BPB media byte |
| FAT0, host and QEMU | RED formatsys FAT[0] media/EOC |
| COMMAND, host and QEMU | RED formatsys /S COMMAND.COM |
| ORDER, host and QEMU | RED formatsys root entry order |
| COMPARE, host | RED formatsys DISKCOMP missing differing sector |
| RUNNING, host | RED formatsys running volume not refused |
| RUNNING, QEMU | RED formatsys-emu running volume not refused |
| RESERVE, host | RED formatsys /B reserve bytes |
| LABEL, host | RED formatsys /V repeat label fsck |
| drive JSON A disk drift | RED drive-letters SPEC A disk drift |
| drive DISK | RED drive-letters A physical identity |
| drive RUNNING | RED drive-letters running pair |

Wrappers require their named line, so a build error or unrelated failure
cannot count as mutation proof. Current gate logs are under
build/formatsys-certificate, with individual mutant logs in build.

The /S boot gate formats scratch D, then promotes its unchanged volume
entry to the existing bootstrap's first-partition slot on a copied fixture.
It observes IOSYS-FOUND, INITDOS-FOUND, KERNEL, SHELL-READY, A:\> and injected
EXIT/SHELL-DONE, with guest_errors=0 and triple_fault=0. It does not certify
boot support for FAT16 or a bare superfloppy without the existing prefix.

## Named incomplete leg and limits

`test-diskcopy-emu` remains RED, deliberately present in TEST_EMU_GATES.
The injected S<path> event calls QMP blockdev-change-medium for the named
disk2 backend after the guest TARGET prompt. QEMU answers:

```
Device 'initech_disk2' is not removable
RED diskcopy-emu: QMP cannot replace the fixed ATA primary-slave medium; host single-drive logic gate remains authoritative
```

No alternate hardware contract, fictitious second floppy or substitute
second-drive copy was used. Full swapped-media DISKCOPY/DISKCOMP emulator
acceptance and the DISKCOMP emulator mutant remain unproved. Host copy and
compare logic, including its differing-sector mutant, are GREEN.

The existing bootstrap remains FAT12-only. FORMAT/SYS retain MBR/stage2
prefix sectors and install the minted VBR; they do not add a bootstrap to
a bare raw disk. FAT16 formatting and /S file transfer have host mtools/
fsck acceptance, but FAT16 boot is not claimed. In-place FORMAT C: /S uses
the shipped contiguous chains and identical data origin; an incompatible
self-source layout fails before writing with the memory-transfer message.
No changes were made to the one-sector-per-cluster FAT-reader contract.

## Changed files

- Makefile; harness/fixture_volumes.mk; harness/emu/qemu.c.
- os/milton/block_device.c and block_device.h.
- os/milton/diskutil.c, diskutil.h, diskutil_program.c, diskutil_start.asm.
- tools/gen_drive_letters.c; spec/drive_letters.json and drive_letters.h.
- spec/dos_folder_externals.txt; harness/diff/fat_diff/dos_folder.sh.
- harness/diff/fat_diff/formatsys.mk, formatsys.sh, formatsys_host.c,
  formatsys_check.c, formatsys_emu.sh, diskcopy_emu.sh, test_drive_letters.c.
- docs/adr/ADR-0003-AMENDMENT-DEC-18-Drive-Letters.md and this worklog.

## Final acceptance certificate

Fresh final logs are in build/formatsys-certificate. Every row below exited
zero. The separate diskcopy emulator leg exited nonzero for the named
fixed-medium QMP error above. All QEMU runs exported INITECH_QMP_STDIO=1.
No full make test and no Bochs were run. Source ASCII checks produced no
matches. Resource checks before heavy runs showed 20-21 GiB available RAM
and 120-121 GiB free on /; kernel builds used two jobs.

| Exact make target | Final nonblank line |
|---|---|
| `test-drive-letters-spec` | `GREEN test-drive-letters-spec: JSON/header equal` |
| `test-drive-letters-spec-mutant` | `GREEN test-drive-letters-spec-mutant: A disk drift rejected` |
| `test-drive-letters` | `GREEN test-drive-letters: A/B/C/D positions, partition bounds/types, running pair and AH60 ABI` |
| `test-drive-letters-mutant` | `GREEN test-drive-letters-mutant: DISK RUNNING rejected` |
| `test-formatsys` | `GREEN test-formatsys: Format another Y repeats; an empty second /V clears the first label` |
| `test-formatsys-mutant` | `GREEN test-formatsys-mutant: MEDIA FAT0 COMMAND ORDER COMPARE RUNNING RESERVE LABEL rejected` |
| `test-formatsys-emu` | `GREEN test-formatsys-emu: injected FORMAT /S /V and SYS, manual text decoded from pixels, images independently checked, running-volume refusal` |
| `test-formatsys-emu-mutant` | `GREEN test-formatsys-emu-mutant: MEDIA FAT0 COMMAND ORDER RUNNING rejected` |
| `test-format-system-boot` | `GREEN test-format-system-boot: FORMAT /S output boots existing MBR/stage2 to A:\> in QEMU` |
| `image` | `>>> image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS + DOS + AUTOEXEC.BAT)` |
| `test-dos-folder-host` | `GREEN test-dos-folder-host: exact DOS/APPS order, AUTOEXEC bytes, artifact FAT reader == mtools, fsck.fat` |
| `test-dos-folder` | `ppm_flair_disk_windows_check: leg dosfolder PASS (9 icon strikes graded off the finder_icons.h ASCII maps, 9 label-band relations, 0 empty cells, 11 chrome assertions, 2 bare-teal controls)` |
| `test-dos-folder-mutant` | `GREEN test-dos-folder-mutant LABEL_TAIL rejected` |
| `test-flair-locked-purge` | `VERDICT: PASS -- test-flair-locked-purge (locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat)` |
| `test-flair-finder-service` | `>>> test-flair-finder-service: green` |
| `test-flair-app-launch` | `>>> test-flair-app-launch: green` |
| `test-kernel-headroom` | `>>> test-kernel-headroom: green` |
| `test-block-device-spec` | `GREEN test-block-device-spec: JSON/header equal` |
| `test-block-device` | `GREEN test-block-device: CHS grid, BIOS statuses/CF, drive map, geometry, raw origin, coherence` |
| `test-block-device-mutant` | `GREEN test-block-device-mutant: HEADS and CARRY rejected` |
| `test-block-device-emu` | `GREEN test-block-device-emu: QEMU INT13 MBR 55AA/type01, scratch LBA32 host readback, every other sector intact` |
| `test-fixture-volumes` | `>>> test-fixture-volumes: green (311 scratch names vs 133 fixture names; 69 inline image writers)` |
| `test-makefile-vars` | `>>> test-makefile-vars: green` |

```
>>> test-kernel-headroom: 207 kernels measured; window 196608 B (384 sectors); floor 4096 B
    flagship kernel_flairtenants.elf                              182904 B loaded, headroom  13704 B
    LARGEST  kernel_flairtenants_db_write_skip.elf                183800 B loaded, headroom  12808 B (binding)
>>> test-kernel-headroom: green
```

Additional unchanged-service regression mutant witnesses:

```
RED block-device CHS heads/sector/cylinder arithmetic
RED block-device carry set on CHS error
```

The requested DOS-folder mutant wrapper also rejects EXTERNAL_IN_APPS,
EXTRA_DOS_FILE, PATH_UNSET, NO_AUTOEXEC, DOS_NOT_DIRECTORY and LABEL_TAIL;
its exact stdout/pixel witnesses remain in the certificate log. The final
shipped data recipe reports fourteen DOS externals. The locked purge gate
still observes purged=16 refused=2; no APPS traces or counts were edited.
