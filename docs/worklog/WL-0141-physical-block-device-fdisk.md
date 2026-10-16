# Physical block device service and FDISK

## Context and decision

Solo initech-aai2 then initech-ur7k lane, resumed under the PM's second-launch
ruling. Retained the first launch's uncommitted service, editor and oracles.
No commits, pushes, Git commands, bead updates, CLAUDE.md edits or HANDOFF.md
edits. Read CLAUDE.md top to bottom, PRD 6.1/8, bd show/prime, the hardware
and partition contracts, ADR-0003 Appendix A and the local manual scans.
bd show found neither lane id; the JSONL snapshot contains ur7k and the
closed m0dc precedent, but not aai2.

DEC-17 is the next available numbered ADR-0003 decision. Amendment:
`docs/adr/ADR-0003-AMENDMENT-DEC-17-Physical-Block-Device-Service.md`.
The checked contract is `spec/block_device_contract.json` and its generated
`spec/block_device_contract.h`. Three geometry/alignment keys were deliberately
added to the MBR contract under the PM ruling, with cross-contract equality
checking. INT25/26 remain volume-relative; INT13 is device-relative.

## Files changed

- `Makefile`.
- `os/milton/block_device.c`, `block_device.h`, `block_device_probe.asm`;
  `ata.c`, `ata.h`, `idt.c`, `isr.asm`, `kmain.c`, `test_idt.c`.
- `os/milton/fdisk.c`, `fdisk.h`, `fdisk.ld`, `fdisk_program.c`,
  `fdisk_start.asm`, `fdisk_fixture.asm`.
- `tools/gen_block_device_spec.c`.
- `harness/diff/fat_diff/block_device.mk`, `block_device.sh`,
  `test_block_device.c`, `fdisk.mk`, `fdisk.sh`, `fdisk_emu.sh`,
  `test_fdisk.c`, `test_partition.c`, `locked_trash.sh`,
  `fixtures/fdisk_display.txt`.
- `harness/fixture_volumes.mk`, `spec/flair_file_ops_traces.mk`.
- `spec/block_device_contract.json`, `spec/block_device_contract.h`,
  `spec/mbr_partition_contract.json`.
- The DEC-17 amendment, `docs/research/block-device-ground-truth.md`,
  and this worklog (renamed from WL-0132 to WL-0141; no stale filename references).

FDISK.COM is appended to APPS by the shipped data-volume recipe. Existing
application cells remain fixed, stranded TRASH is entry 0, and the purge
count is 21 with the new writable leaf checked. The flagship layout is
unchanged. New targets are the five pairs `test-block-device-spec`,
`test-block-device`, `test-block-device-emu`, `test-fdisk`, `test-fdisk-emu`
and their `-mutant` targets. They are in TEST_UNIT_GATES / TEST_EMU_GATES.

## Red to green: fixture correction

Reproduced the host failure before editing:

```text
fat_dump: fat12_mount('build/fdisk/ours.img') failed: rc=-14 base_lba=63 hidden_sectors=63
RED fdisk kernel partition mount
```

The kernel's one-sector-per-cluster guard is unchanged; lifting it remains
initech-spc. Every formatted mount leg now requests -c 1 and independently
checks the actual BPB cluster byte. The full 66-cylinder type06 partition
uses a 1024-sector root directory, leaving fewer than 65525 data clusters.
The full 513-cylinder MBR/packed-CHS differential remains; its mount leg
then deletes/recreates a 66-cylinder primary in both identical images.
Every leg still compares kernel file bytes with mcopy and requires fsck.fat
to report clean. No MBR, file-byte or filesystem assertion was removed.

The QEMU custom-size leg then exposed a reference setup error:
`RED fdisk-emu custom cylinder size/default inactive`. The actual editor
was correctly inactive; mpartition had auto-activated its first primary.
The reference now explicitly requests -d, as required for the MS-DOS UG
p289 N/custom-size workflow. The full table comparison remains intact.

## Final certificate

All 39 targets below exited zero in this launch after the fixture fixes.
These are fresh results, not the first launch's acceptance claims.
Full logs: `build/blockdev-final-certificate/<target>.log`.
Each row quotes the final nonblank, non-border line from that target's log.
The long runner was resumed in shorter batches after interruption.
Headroom was completed with two build jobs after checking available RAM;
one interrupted, empty generated ELF was moved aside and rebuilt.

Every QEMU gate exported INITECH_QMP_STDIO=1. No Bochs and no full make test.
Resource checks before heavy runs showed 20-21 GB RAM available and
124-125 GB free on /. The final headroom measurement is:

```text
>>> test-kernel-headroom: 204 kernels measured; window 196608 B (384 sectors); floor 4096 B
    flagship kernel_flairtenants.elf                              182008 B loaded, headroom  14600 B
    LARGEST  kernel_flairtenants_db_write_skip.elf                182776 B loaded, headroom  13832 B (binding)
>>> test-kernel-headroom: green
```

| Exact make target | Final tail |
|---|---|
| `test-block-device-spec` | `GREEN test-block-device-spec: JSON/header equal` |
| `test-block-device-spec-mutant` | `GREEN test-block-device-spec-mutant: JSON heads drift rejected` |
| `test-block-device` | `GREEN test-block-device: CHS grid, BIOS statuses/CF, drive map, geometry, raw origin, coherence` |
| `test-block-device-mutant` | `GREEN test-block-device-mutant: HEADS and CARRY rejected` |
| `test-block-device-emu` | `GREEN test-block-device-emu: QEMU INT13 MBR 55AA/type01, scratch LBA32 host readback, every other sector intact` |
| `test-block-device-emu-mutant` | `GREEN test-block-device-emu-mutant: volume-relative write rejected` |
| `test-fdisk` | `GREEN test-fdisk: create/type/CHS/signature, active entry, default-No and confirmed deletion differential` |
| `test-fdisk-mutant` | `GREEN test-fdisk-mutant: CHS SIGNATURE ACTIVE CONFIRM rejected` |
| `test-fdisk-emu` | `GREEN test-fdisk-emu` |
| `test-fdisk-emu-mutant` | `GREEN test-fdisk-emu-mutant: CHS SIGNATURE ACTIVE CONFIRM LOADED rejected` |
| `test-fat-partition` | `GREEN test-fat-partition` |
| `test-fat-partition-mutant` | `GREEN partition mutant SIGNATURE rejected` |
| `test-fat-partition-emu` | `GREEN test-fat-partition-emu` |
| `test-fat-partition-emu-mutant` | `GREEN partition emu mutant SIGNATURE rejected` |
| `test-partition-spec` | `GREEN test-partition-spec: IBM table layout and DEC-07a origin contract` |
| `test-partition-spec-mutant` | `GREEN test-partition-spec-mutant: wrong table offset rejected` |
| `test-idt` | `>>> test-idt: green` |
| `test-idt-mutant` | `>>> test-idt-mutant: green (mutant correctly RED -- the oracle bites)` |
| `test-spurious` | `VERDICT   : PASS -- stray/unhandled vector resumed cleanly, machine not wedged` |
| `test-panic` | `VERDICT   : PASS -- deliberate #DE caught by the fail-loud panic, no triple-fault` |
| `test-fpu-probe` | `test_fpu: 18 checks, 0 failures` |
| `test-fpu-probe-mutant` | `>>> test-fpu-probe-mutant: green (both mutants correctly RED)` |
| `test-fpu` | `VERDICT   : PASS -- CR0 MP=1 EM=0 TS=0 NE=1, x87 63/125 exact, SW clean, CW == spec (QEMU)` |
| `test-fpu-mutant` | `>>> test-fpu-mutant: green (both mutants RED for the right reason)` |
| `test-fpu-absent` | `VERDICT   : PASS -- booted without an FPU; EM=1; first x87 op -> #NM panic` |
| `test-fpu-absent-mutant` | `>>> test-fpu-absent-mutant: green (absent branch without EM=1 -> no #NM -> RED on [nm])` |
| `test-fpu-mf` | `VERDICT   : PASS -- unmasked #Z delivered as #MF (vector 16), fail-loud panic` |
| `test-fpu-mf-mutant` | `>>> test-fpu-mf-mutant: green (NE=0 -> no #MF -> RED on [mf])` |
| `test-absdisk` | `>>> test-absdisk: green (round-trip + cross-check + non-corruption + error legs)` |
| `test-absdisk-mutant` | `>>> test-absdisk-mutant: all 8 DEC-15 mutants correctly RED (the oracle bites)` |
| `test-absdisk-emu` | `wrote and read back a sector (QEMU only; tri-emulator pending initech-x0i)` |
| `test-kernel-headroom` | `>>> test-kernel-headroom: green` |
| `test-flair-locked-purge` | `VERDICT: PASS -- test-flair-locked-purge (locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat)` |
| `test-flair-finder-service` | `>>> test-flair-finder-service: green` |
| `test-flair-app-launch` | `>>> test-flair-app-launch: green` |
| `test-flair-app-launch-mutant` | `>>> test-flair-app-launch-mutant: green (all six mutants RED for their named reasons)` |
| `test-fixture-volumes` | `>>> test-fixture-volumes: green (275 scratch names vs 124 fixture names; 68 inline image writers)` |
| `test-makefile-vars` | `>>> test-makefile-vars: green` |
| `image` | `>>> image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS)` |

## Mutation witnesses

Lane-specific witnesses from the final logs:

| Mutant | Exact RED line |
|---|---|
| SPEC heads drift | `RED block-device SPEC heads drift` |
| HEADS | `RED block-device CHS heads/sector/cylinder arithmetic` |
| CARRY | `RED block-device carry set on CHS error` |
| RELATIVE | `RED block-device write device LBA 32 (volume-relative mutant)` |
| FDISK CHS host | `RED fdisk CHS/relative-sector/type/signature vs mpartition` |
| FDISK SIGNATURE host | `RED fdisk CHS/relative-sector/type/signature vs mpartition` |
| FDISK ACTIVE host | `RED fdisk active flag on wrong entry` |
| FDISK CONFIRM host | `RED fdisk delete without confirmation` |
| FDISK CHS QEMU | `RED fdisk-emu CHS/signature create MBR vs mpartition` |
| FDISK SIGNATURE QEMU | `RED fdisk-emu CHS/signature create MBR vs mpartition` |
| FDISK ACTIVE QEMU | `RED fdisk-emu active flag on wrong entry` |
| FDISK CONFIRM QEMU | `RED fdisk-emu delete without confirmation` |
| FDISK LOADED QEMU | `RED fdisk-emu loaded volume was not refused` |

Both FDISK mutant wrappers require their named RED line; unrelated failures
cannot count as mutation proof. Partition regression witnesses:

```text
RED partition SPEC contract drift rc=0
RED fdisk CHS/relative-sector/type/signature vs mpartition
RED fdisk CHS/relative-sector/type/signature vs mpartition
RED fdisk active flag on wrong entry
RED fdisk delete without confirmation
RED fdisk-emu CHS/signature create MBR vs mpartition
RED fdisk-emu CHS/signature create MBR vs mpartition
RED fdisk-emu active flag on wrong entry
RED fdisk-emu delete without confirmation
RED fdisk-emu loaded volume was not refused
RED partition valid MBR rc=-20
RED partition EXTENDED must reject rc=0
RED partition SIGNATURE must reject rc=0
RED block-device SPEC heads drift
RED block-device CHS heads/sector/cylinder arithmetic
RED block-device carry set on CHS error
RED block-device write device LBA 32 (volume-relative mutant)
RED partition MOUNT
RED partition EXTENDED accepted invalid MBR
RED partition SIGNATURE accepted invalid MBR
```

Additional regression mutant witnesses (IDT's wrapper also reported its
offset-hi mutant correctly RED; see its final tail above):

```text
    test_fpu_probe_mut_sw_only: RED (correct)
    test_fpu_probe_mut_absent_em: RED (correct)
>>> test-fpu-probe-mutant: green (both mutants correctly RED)
>>> test-absdisk-mutant: green (M1-off-by-one-LBA correctly RED)
>>> test-absdisk-mutant: green (M2-write-noop correctly RED)
>>> test-absdisk-mutant: green (M3-reg-transpose correctly RED)
>>> test-absdisk-mutant: green (M4-no-bounds correctly RED)
>>> test-absdisk-mutant: green (M5-wrong-errclass correctly RED)
>>> test-absdisk-mutant: green (M6-corrupt-neighbor correctly RED)
>>> test-absdisk-mutant: green (M7-stack-wart correctly RED)
>>> test-absdisk-mutant: green (M8-packet-literal correctly RED)
>>> test-flair-app-launch-mutant: NO_REGISTER RED for the named reason -- no TENANT-REGISTER, the golden trace diverges, desktop intact
>>> test-flair-app-launch-mutant: DOUBLE_LAUNCH RED for the named reason -- a second REGISTER installed over the resident tenant, no TENANT-SLOT-BUSY
>>> test-flair-app-launch-mutant: EXIT_LEAK RED for the named reason -- TENANT-HEAPAVAIL drifted 3490784 -> 3486672 (the code block leaks each cycle)
>>> test-flair-app-launch-mutant: CLOSE_STALE_FOCUS RED for the named reason -- the hide-only close left the tenant resident (no TENANT-EXIT) and the relaunch hit TENANT-SLOT-BUSY on the windowless zombie
>>> test-flair-app-launch-mutant: MBAR_NOT_SWAPPED RED for the named reason -- SETMBAR said 0 and the tenant is foreground, but band 2 is still the shell fallback bar
>>> test-flair-app-launch-mutant: MBAR_NOT_SWAPPED RED on the MENU leg too -- the band-2 click dropped FLAIR-MENU-DROP menu=259, not the tenant menu 131
>>> test-flair-app-launch-mutant: MBAR_NO_BOUNDS RED for the named reason -- the MenuBar in the PSP below imageBase was accepted (-> 0, no TENANT-SETMBAR-BAD), and the out-of-image bar REACHED band 2
    | !!! absent-mutant FAIL [cr0]: CR0 is not MP=0 EM=1 TS=0 (got "FPU-CR0 mp=0 em=0 ts=0 ne=1")
    | !!! absent-mutant FAIL [nm]: the first x87 instruction did not trap #NM (no "PANIC vec=07")
    | !!! skip-mutant FAIL [cr0]: CR0 is not MP=1 EM=0 TS=0 NE=1 (got "FPU-CR0 mp=0 em=0 ts=0 ne=0")
    | !!! cw-mutant FAIL [cw]: FNSTCW readback != spec 037F (got "FPU-CW=0000027F")
    | !!! mf-mutant FAIL [mf]: unmasked x87 #Z was not delivered as #MF (no "PANIC vec=10")
```

## Law-1 sources and remaining limitation

- ADR-0003 Appendix A (console/process ABI), DEC-04a (flat registers),
  DEC-08 (flat external), DEC-07a and spec/mbr_partition_contract.json
  (volume origin), PM DEC-17 (physical service, geometry, vector reuse,
  type06 and loaded-volume refusal).
- IBM DOS 3.30 Technical Reference, printed pp9-6..9-9 (PDF393..396):
  table offsets, active flag, packed CHS, relative-sector fields,
  first-track reservation and 55AA signature.
- MS-DOS 3.3 User's Guide, printed pp287..289,291..293,295,353
  (PDF372..374,376..378,380,438): menus, custom size, active selection,
  default-No deletion confirmation, display columns, restart and the
  primary-already-exists message. These and the IBM DOS pages were
  rendered at 110 DPI with pdftoppm and viewed locally in this launch.
- Local mformat(1) documentation: -c may be raised automatically; -r
  sets the root-directory length. mpartition and sfdisk independently
  supply MBR goldens on identical images; mcopy and fsck.fat judge FAT bytes.
- BIOS function/status source: IBM PS/2 and PC BIOS Interface Technical
  Reference, April 1987, printed pp2-58..61,2-64,2-67. The supplied DOS TR
  does not contain that table. The local source digest is in
  docs/research/block-device-ground-truth.md. Remaining evidence limitation:
  the supplemental BIOS PDF could not be downloaded/rendered locally in
  the first launch; this launch does not claim that it was.

All requested gate targets completed. Multi-sector-cluster support remains
outside this lane by the PM ruling. Changes are left uncommitted for review.

Final source ASCII and worklog-rename/reference checks passed. The FDISK.COM
read back from the shipped APPS directory is byte-identical to the built
flat executable.
