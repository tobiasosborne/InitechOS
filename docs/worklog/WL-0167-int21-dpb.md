# WL-0167 -- INT 21h AH=1Fh / AH=32h: the DOS 3.x Drive Parameter Block

Lane: `lane/dpb` (bead initech-dpb1), based on main `4d907b1f`. Closes the
last RECOGNIZED-but-deferred INT 21h function that kept the strict kernel
coverage certificate RED, and restores `test-kernel-coverage`,
`test-kernel-coverage-mutant` and `test-kernel-certificate` to the default
vectors. No full `make test`, no Bochs, no beads/CLAUDE.md/HANDOFF edits.

## Context

WL-0156 (dosvolumes) built the A/C/D volume table and the coverage register
and left the strict certificate RED on purpose: AH=1Fh sits inside the locked
`0Fh-24h FCB ops` row, `do_fcb` returned 0 for it and it fell into the
not-yet-impl arm. The PM parked the two certificate gates as named
preflights until the DPB landed.

## Ground truth (Law 1)

The local IBM DOS 3.30 Technical Reference does not document the DPB: its
function listing names 1FH and 32H only as "Reserved by DOS". The layout was
therefore acquired with provenance into `docs/research/dpb-ground-truth.md`:
MS-DOS 2.0 `DOSSYM.ASM` / `GETSET.ASM` / `MSINIT.ASM` and MS-DOS 4.0
`DPB.INC` / `MISC.ASM` (MIT, microsoft/MS-DOS @2d04cacc), plus Ralf Brown's
Interrupt List r61 `INTERRUP.F` (Table 01395). SHA-256 of every file is
recorded. The 4.0 source's `;AC000;` mark on the `dpb_FAT_size` STOSW, and the
unmarked `dpb_next_free` / `dpb_free_cnt` init, bracket the 3.x layout
(32 bytes, BYTE FAT size at 0Fh); RBIL states it explicitly. All three agree
field-for-field.

## What changed

- Locked scope (Rule 8, deliberate): `spec/int21h_register.json` and ADR-0003
  Appendix A gain the `32h GETDPB` row (Legacy), dated and cited; ADR-0003
  Appendix B.4 and `spec/dos_structs.h` `dpb_t` (32 bytes, static-asserted)
  record the layout; `spec/int21h_calling_convention.json` pins the 1Fh/32h
  flat ABI (EBX = flat DPB address, AL 00h/FFh, CF cleared, EBX untouched on
  FFh). AH=32h was outside the controlled scope before; this admission is
  the one scope change of the lane and is flagged for operator ratification.
- Artifact: `int21.c` `do_getdpb` -- one resident DPB per letter, chain A ->
  C -> D rebuilt on every call from the mounted geometry (the FATREAD media
  check), last link FFFFFFFFh (MSINIT), unit = letter, 12h = 0 (DEC-17 has no
  installable block driver header), 17h = 00h (mounted volumes were read),
  1Ch = 0 (no rover), 1Eh = the requested drive's live free count, FFFFh for
  the rest. `ah_is_listed` admits 32h; 1Fh is AH=32h with DL=0.
  `fileio_fat.c` supplies `fat_dpb_geometry` from the mounted volume's own
  derived fields through a new `int21_set_dpb_source` seam (int21.c still
  never includes fat12.h).
- Unit oracles: `test_int21` replaces its 1Fh not-yet-impl probe with the
  1Fh/32h dispatch contract; `test_40oq` drops the last FCB waiver (now
  empty, check (d) requires it to stay empty) and dispatches 1Fh/32h.

## Oracle (Law 2)

`harness/diff/fat_diff/int21_dpb.sh` grades every one of the 18 DPB fields
against an independent golden: `minfo`/`mdir` (mtools) of the same images,
with the field table and `$SETDPB` arithmetic transcribed from the research
note -- not from `dpb_t` and not from fat12.c. Map: A = raw FAT (FAT16 8 MiB
and FAT12 1.44 MB on the host; FAT12 in QEMU because EXEC loads only from a
whole-FAT volume), C = the boot image's FAT12 primary 1, D = an appended
FAT16 primary 2 (hidden sectors == start). Ten calls: 1Fh and 32h DL=0/1 on
A, B/E/1Bh invalid, C, D, and 1Fh/32h after selecting C. Chain links and
EBX-untouched-on-FFh are checked too.

- `test-int21-dpb` (host driver `int21_dpb_host.c`, real int21 + FAT backend)
- `test-int21-dpb-emu` (QEMU: `DPB.COM` from `int21_dpb_program.asm`, real
  INT 21h over ATA, run from AUTOEXEC)
- `test-int21-dpb-mutant` / `test-int21-dpb-emu-mutant`: the named
  `INT21_MUTATE_DPB_FIRST_DATA` (first data sector forgets the root
  directory) goes RED on both legs.

Hand perturbations of the printed bytes (free count, chain link, FAT size,
media, drive, an AL=00h on an absent drive) each went RED with the field
named. The coverage checker goes RED with the DPB witness rows removed
(`AH=1F has no implemented gate`) and with the 32h register row removed
(`register dropped dispatcher AH=32`).

## Frictions

- The loader binds only drive A and only a whole-FAT (FAT12) volume, so a
  program cannot be EXECed from a FAT16 A; FAT16 is witnessed in QEMU via D.
- Pre-existing, not changed here: AH=1Bh/1Ch report A's media byte for every
  drive (`int21_set_media` is set once at bind). The DPB carries the correct
  per-volume media byte.

## Acceptance

Re-run by the takeover lane (dpb2) from the WIP commit, one QEMU at a time,
env.sh sourced, no full `make test`. Tails (build/dpb-final/*.log):

```
test-int21               test_int21: 179 checks, 0 failures            rc=0
test-40oq                test_40oq: 250 checks, 0 failures             rc=0
test-int21-dpb           GREEN int21-dpb host fat16 A + FAT12 C + FAT16 D: 10 calls, every DPB field == minfo/mdir
                         GREEN int21-dpb host fat12 A + FAT12 C + FAT16 D: 10 calls, every DPB field == minfo/mdir
test-int21-dpb-mutant    RED int21-dpb A first data sector 0Bh got 129 want 161 (1F00)
                         GREEN test-int21-dpb-mutant: FIRST_DATA rejected
test-int21-dpb-emu       GREEN int21-dpb emu: QEMU DPB.COM over ATA FAT12 A + FAT12 C + FAT16 D, 10 calls, every DPB field == minfo/mdir
test-int21-dpb-emu-mutant RED int21-dpb A first data sector 0Bh got 19 want 33 (1F00)
                         GREEN test-int21-dpb-emu-mutant: FIRST_DATA rejected in QEMU
test-kernel-coverage     GREEN kernel-coverage: 76 Appendix-A AHs registered and exercised by named TEST-vector gates
test-kernel-coverage-mutant RED kernel-coverage register dropped dispatcher AH=30: restore the function in spec/int21h_register.json
                         GREEN test-kernel-coverage-mutant: DROP_REGISTER_GETVER rejected
test-kernel-certificate  GREEN test-kernel-certificate: Appendix A + two volumes + FAT16 ATA
test-kernel-headroom     >>> test-kernel-headroom: 246 kernels measured; window 196608 B (384 sectors); floor 4096 B -- green
```

Golden sanity (Rule 4, read by hand): the C and D DPBs carry volume-relative
sector numbers (C: reserved 1, dir 19, first data 33 behind hidden 63; D:
dir 129, first data 161 behind hidden 2943) and the per-volume media byte
(A F0h, C/D F8h); the A chain link is the flat address of C's DPB, D ends
with FFFFFFFFh; the free count is the only field the artifact computes on
the call and it matched mdir's `bytes free` on every volume.

## Pointers

- docs/research/dpb-ground-truth.md -- the layout + call contract, with provenance
- os/milton/int21.c do_getdpb / dpb_fill; os/milton/fileio_fat.c fat_dpb_geometry
- harness/diff/fat_diff/int21_dpb.{mk,sh}, int21_dpb_host.c, int21_dpb_program.asm
- harness/diff/fat_diff/dos_volumes.mk -- the restored TEST_UNIT_GATES / TEST_EMU_GATES lines
- spec/int21h_register.json (32h row), spec/dos_structs.h dpb_t, ADR-0003 Appendix A/B.4
