# WL-0156 -- Mounted DOS drives, FAT16 ATA, and strict kernel coverage

Lane: initech-slvd part 2, initech-bcg.16, initech-40oq. Second launch keeps
and completes the first launch's working tree, without commits, Git metadata
changes, pushes, bead changes, CLAUDE.md changes, or HANDOFF.md changes.

## Behaviour and sources

DEC-18's fixed map is used for actual mounts: A is BIOS 81h (raw FAT or
primary partition 1); C is BIOS 80h primary partition 1; D is primary 2 when
present; B is absent. Mounted volumes have independent FAT caches/windows
and current directories. INT21 path selection, open SFT identity, find state,
and the shell prompt preserve the selected volume across intervening calls.
AH=0Eh/19h really select/report the default. Absent letters fail as invalid
drives, and cannot silently alias A. LASTDRIVE contributes to AH=0Eh's
reported logical extent, with DOS 3.3's minimum 5.

Ground truth, rendered at 110 dpi and inspected in this launch:

- ADR-0003 Appendix A/B.1, DEC-07a, ratified DEC-16, DEC-18;
  spec/drive_letters.json; spec/int21h_register.json; PRD 6.1/8.
- MS-DOS_3.3_Users_Guide_198707.pdf: Guide pp24-26 (PDF37-39),
  cross-drive copying and prompt/default-drive identity; Reference pp4-7,
  10-11 (PDF89-92,95-96), path/tree/current-directory behaviour;
  pp41-42 (PDF126-127), CD; p50 (PDF135), COPY drive operands;
  pp57-58 (PDF142-143), DIR drive operands; p340 (PDF425),
  exact `Invalid drive specification` message.
- 80X0945_DOS_3.30_Technical_Reference_Apr87.pdf: p6-66 (PDF189),
  SELDISK validity, LASTDRIVE and minimum extent 5; p6-81 (PDF204),
  GETDISK; p6-140 (PDF263), AH40/CX0 truncation at the current pointer;
  p6-188 (PDF311), GETCWD specified drive and returned string;
  p9-9 (PDF396), MBR/partition 55AA at offset 1FEh.
- Independent mtools/dosfstools read the same A/C images: mtype/mcopy
  content, mdir both images, fsck.fat after actual kernel operations.
  FAT16's long TYPE crosses FAT windows and writes output through a C
  handle while reading A over ATA. FAT16 remains read-only.

## Second-launch corrections and RED evidence

The old fuzz model treated len=0 as a no-op. Seed 3 shrank to CREATE,
WRITE offset=1 len=0, failing `dir file_size != model size`. The model now
sets the size to the pointer (TR p6-140), preserving the independent byte
model, full seed sweep, external comparison and shrinker.

That correction exposed a real retained-tail interaction: seed 7 shrank to
CREATE; WRITE at31230; TRUNCATE to31230; WRITE at31337. Re-extension exposed
stale bytes in the retained partial cluster. Both strengthened host and ATA
truncate matrices went RED: `RED truncate: GAPIN bytes != mcopy`.
Positioned writes now zero the newly exposed part of the old tail as well
as fresh clusters, honouring fat12.h's existing zero-fill contract. GAPIN
and GAPOUT cover gaps within and beyond the retained cluster; the named
STALE_GAP mutant disables only this correction.

The new mount wrapper prevalidated the drive map before FAT mount, turning
an invalid signature into generic partition error and losing diagnostics.
The unchanged DEC-07a gate reproduced `RED partition SIGNATURE accepted
invalid MBR`. A rejected A map now obtains the precise FAT diagnostic,
while still refusing to bind any volume DEC-18 rejects. The original
FAT_PART_MUT_SIGNATURE remains in the host and ATA mutant gates.

The old coverage tool reproduced:
`RED kernel-coverage register/Appendix-A mismatch AH=33: reconcile the dated Appendix-A row with spec/int21h_register.json (AH=33: DEC-16, test-er3h)`.
Appendix A now contains the already-authorized BREAK row with a dated
2026-10-17 reconciliation. JSON is the sole machine scope source. The
checker compares the actual dispatcher register bidirectionally, then
requires a named gate in TEST vectors for every registered function.
Dropping AH30 from JSON fails with `register dropped dispatcher AH=30`.
AH33 retains test-er3h. No function or class was removed from scope.

The strict capstone still refuses AH1F, which is included in the locked
0Fh-24h row, explicitly returns 0 in do_fcb, and falls into not-yet-impl.
The older test-40oq accepts a legacy waiver; the new strict certificate
does not turn that into an implemented-function witness. Implementing DPB
or altering scope/waivers is outside this pass. No green strict kernel
certificate is claimed.

## Validation method

The final sweep runs each named target sequentially, continues to the end
on failure, and keeps one final log per target in build/dos-volumes/final/.
Every QEMU inherits INITECH_QMP_STDIO=1. No full make test or Bochs is run.
Source ASCII and shell syntax checks pass. Resource checks before heavy
runs found 18 GiB available RAM and 114 GiB free disk.

The final sweep also caught a DOS-folder trace completion race: the quit
substring appeared in the typed ECHO line before the terminating Return,
so QEMU stopped before the exact output line. The quit marker now includes
the output line's leading newline and terminal carriage return. The typed
command has a prompt/ECHO prefix and cannot match that marker. The commands
and exact-line serial assertion stay unchanged. Both affected targets are
rerun after this trace correction.

## Changed files in the lane working tree

- Makefile.
- os/milton: command.c, fat12.c, fat12.h, fileio_fat.c, fileio_fat.h,
  int21.c, int21.h, kmain.c, sft.h, sysinit.c, test_exec.c,
  test_int21.c, test_int21_edge.c.
- harness/diff/fat_diff: dos_volumes.mk, dos_volumes.sh,
  dos_volumes_host.c, int21_coverage.c, int21_witnesses.tsv, dos_folder.mk,
  fat12_fuzz.c, truncate.c, truncate.sh, truncate_program.asm.
- spec: int21h_register.json, dos_folder_traces.mk,
  flair_protection_traces.mk.
- docs/adr/ADR-0003-InitechDOS-Base-OS-Personality.md and this worklog.

Build-generated palette.h/fkeys.h were regenerated by existing rules.
The existing harness/pubs/int21_inventory.c is reused without modification.

Locked purge retained every byte/attribute and passed all 17-purged/2-refused
assertions, but its screenshot still had the Trash selected after opening it.
The unchanged locked-full oracle requires mint-008's unselected full can.
A dedicated protection trace now clicks bare desktop to deselect Trash and
returns to the original menu path. The normal Empty Trash selected-can trace
and pixel golden are unchanged. Gate, protection mutant, and recorded trace
share this corrected protection trace. Makefile and
spec/flair_protection_traces.mk carry that re-key; no product UI was changed.

## Final result

The sweep ended after 118 executions, including final reruns: 100 unique
named targets, 98 PASS and 2 RED. The only final failures are
`test-kernel-coverage` and `test-kernel-certificate`, both for the same
unimplemented, ungated AH1F. The three initially failing trace runs were
corrected and their final runs passed. All added gates have final runs.
Every requested FAT/block-device/FDISK/FORMAT-SYS/DOS-folder/COMMAND-file
family target in make's current database was included (set difference empty).

All requested shell, FS, EXEC, desktop app-launch, Finder-service,
locked-purge, Exit-to-DOS, TPS-journey, and image checks passed. The final
headroom run measured 239 kernels in the 196608-byte window with the
4096-byte floor unchanged: flagship 178964 loaded / 17644 headroom;
binding kernel_flairtenants_db_write_skip.elf 180212 loaded / 16396 headroom.
The final locked-full pixel check reports 0/1024 cells different from the
independent capture. Protection mutants all remain RED. The fuzzer passes
200 seeds / 8000 operations and its external leg 40 seeds / 1600 operations;
the original no-RMW mutant still shrinks to a three-operation reproducer.

Remaining work: no strict kernel-complete certificate is possible while
AH1F returns 0 in do_fcb (int21.c:4032) and explicitly falls into the
not-yet-impl arm (int21.c:5243). No implementation, scope removal or new
waiver for it was added. No full make test, Bochs, Git operation, commit,
push, or bead update was performed. All changes remain for PM review.

## Final mutation evidence

```text
SELECT: RED dos-volumes SELECT current drive
PATH: RED dos-volumes CWD chdir
CWD: RED dos-volumes PATH open
HANDLE: RED dos-volumes HANDLE read
SEARCH: RED dos-volumes SEARCH retained C volume
CACHE: RED dos-volumes HANDLE read
INVALID: RED dos-volumes INVALID selection unchanged
LIMIT: RED dos-volumes LIMIT LASTDRIVE=Z
LASTDRIVE: RED dos-volumes LIMIT LASTDRIVE=Z
PROMPT: RED dos-volumes PROMPT current C
MESSAGE: RED dos-volumes INVALID B/D message
FAT16: RED dos-volumes FAT16 ATA TYPE bytes
RED kernel-coverage register dropped dispatcher AH=30: restore the function in spec/int21h_register.json
!!! fat12_fuzz: DIVERGENCE on seed 7 (op #39, read_file bytes != model) -- shrinking...
---- SHRUNK REPRODUCER (seed=7) ----
build/kzfs/emu-mut-SIGNATURE.log:RED partition SIGNATURE accepted invalid MBR
build/dos-volumes/stale-gap-emu.log:RED truncate: GAPIN bytes != mcopy
```

## Final per-target tails

These are the final logs for each unique target; the earlier RED proofs
remain separately available in build/dos-volumes/final/.

### image -- PASS

```text
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
cp -f build/flair_tenants.img build/initech.img
>>> image: build/initech.img (flagship alias of build/flair_tenants.img -- FLAIR desktop + HELLO/NOTES co-resident tenants)
>>> image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS + DOS + AUTOEXEC.BAT)
```

### test-40oq -- PASS

```text
  Deferred (open beads; outside this cert's gate):
    initech-kzfs  -- MBR partition-table parse + hidden_sectors LBA
    initech-slvd  -- Multi-volume drive letters A:/B:/C:
test_40oq: 246 checks, 0 failures
>>> test-40oq: green
```

### test-block-device -- PASS

```text
pubs/pubs.mk:384: warning: overriding recipe for target 'test-pubs-examples'
pubs/pubs.mk:379: warning: ignoring old recipe for target 'test-pubs-examples'
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
GREEN test-block-device: CHS grid, BIOS statuses/CF, drive map, geometry, raw origin, coherence
```

### test-block-device-emu -- PASS

```text
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
cc -std=c11 -O2 -Wall -Wextra -Werror -Ispec -Ios/milton -Iharness/diff/fat_diff harness/diff/fat_diff/test_partition.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c -o build/test_partition
Warning: no active (bootable) partition present
GREEN test-block-device-emu: QEMU INT13 MBR 55AA/type01, scratch LBA32 host readback, every other sector intact
```

### test-block-device-emu-mutant -- PASS

```text
objcopy -O binary build/kernel_block_device_relative.elf build/kernel_block_device_relative.bin
>>> kernel placement guard (block-device-relative): lowest=0x500000 >= KERNEL_BASE=0x500000; _kernel_end=0x524a80 < KERNEL_CEIL=0x600000 OK
>>> boot-system image: build/block_device_relative_boot.img (BOOT FAT12@63; root[0]=IO.SYS, root[1]=INITDOS.SYS; HSR)
RED block-device write device LBA 32 (volume-relative mutant)
GREEN test-block-device-emu-mutant: volume-relative write rejected
```

### test-block-device-mutant -- PASS

```text
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
RED block-device CHS heads/sector/cylinder arithmetic
RED block-device carry set on CHS error
GREEN test-block-device-mutant: HEADS and CARRY rejected
```

### test-block-device-spec -- PASS

```text
pubs/pubs.mk:384: warning: overriding recipe for target 'test-pubs-examples'
pubs/pubs.mk:379: warning: ignoring old recipe for target 'test-pubs-examples'
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
GREEN test-block-device-spec: JSON/header equal
```

### test-block-device-spec-mutant -- PASS

```text
pubs/pubs.mk:379: warning: ignoring old recipe for target 'test-pubs-examples'
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
RED block-device SPEC heads drift
GREEN test-block-device-spec-mutant: JSON heads drift rejected
```

### test-command-file -- PASS

```text
GREEN test-command-file interactive: mdir + mattrib archive-only + mtype/cmp + artifact FAT reader + fsck.fat clean
GREEN command-file: root COMMAND.COM attr=20, 42312 bytes == linked interpreter; FAT@63, 46 cylinders
GREEN test-command-file clip: mdir + mattrib archive-only + mtype/cmp + artifact FAT reader + fsck.fat clean
GREEN command-file: root COMMAND.COM attr=20, 42312 bytes == linked interpreter; FAT@63, 46 cylinders
GREEN test-command-file desktop-family: mdir + mattrib archive-only + mtype/cmp + artifact FAT reader + fsck.fat clean
```

### test-command-file-emu -- PASS

```text
GREEN command-file: root COMMAND.COM attr=20, 42312 bytes == linked interpreter; FAT@63, 46 cylinders
GREEN test-command-file fragmented: mdir + mattrib archive-only + mtype/cmp + artifact FAT reader + fsck.fat clean
cp -f build/flair_data.img build/command_file_fragment_data.img
build/qemu_harness --disk "build/command-file/fragment.img" --disk2 "build/command_file_fragment_data.img" --expect FLAIR-FAT-MOUNT-OK --name "command_file_fragment" --out "build" --mouse "m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:-8,m-100:-8,m-100:-8,m-100:-8,m-30:-2,l1,m0:100,m0:33,l0,m100:2,m100:2,m94:1,l1,l0,WSHELL-READY,kd,ki,kr,kret" --keys-after "FLAIR-LIVE-READY" --quit-after "bytes free" --screendump --screendump-after "bytes free" --timeout-ms 40000 2> "build/command_file_fragment.report" || true
GREEN test-command-file-emu: fragmented COMMAND.COM -> transient -> banner -> SHELL-READY -> injected DIR; text console
```

### test-command-file-emu-mutant -- PASS

```text
GREEN test-command-file-emu-mutant COMMAND_VOLUME rejected
RED command-file-emu skip-magic missing exact MSG-DOS-0014
GREEN test-command-file-emu-mutant COMMAND_MAGIC rejected
RED command-file-emu wrong-message missing exact MSG-DOS-0014
GREEN test-command-file-emu-mutant COMMAND_MESSAGE rejected
```

### test-command-file-mutant -- PASS

```text
GREEN test-command-file-mutant BYTES rejected
RED command-file GEOMETRY 46 cylinders; no raw system area
GREEN test-command-file-mutant GEOMETRY rejected
RED command-file DATE SOURCE_DATE_EPOCH 1980-01-01
GREEN test-command-file-mutant DATE rejected
```

### test-command-file-negative -- PASS

```text
cp -f build/flair_tenants.img build/command-file/magic.img
cp -f build/transient_flairtenants.bin build/command-file/magic.bin
TZ=UTC mcopy -o -m -i build/command-file/magic.img@@32256 build/command-file/magic.bin ::COMMAND.COM
GREEN test-command-file-negative missing: MSG-DOS-0014 once + COMMAND.COM not found + HALTED; no interpreter, no triple fault
GREEN test-command-file-negative magic: MSG-DOS-0014 once + no COMMAND3 transient + HALTED; no interpreter, no triple fault
```

### test-dos-folder -- PASS

```text
    icon[7] FIND.COM   cell (268,158) [col 3 row 1]
    FIND.COM   label band (270,192)-(298,205): normal (black=58 white=306)
    icon[8] SORT.COM   cell (64,210) [col 0 row 2]
    SORT.COM   label band (66,244)-(94,257): normal (black=57 white=307)
ppm_flair_disk_windows_check: leg dosfolder PASS (9 icon cells == the mint-008 captures, 9 label-band relations, 0 empty cells, 11 chrome assertions, 2 bare-teal controls)
```

### test-dos-folder-host -- PASS

```text
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/fat_dump harness/diff/fat_diff/fat_dump.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
GREEN test-dos-folder-host: exact DOS/APPS order, AUTOEXEC bytes, artifact FAT reader == mtools, fsck.fat
```

### test-dos-folder-host-mutant -- PASS

```text
GREEN test-dos-folder-host-mutant EXTRA_DOS_FILE rejected
RED dos-folder autoexec
GREEN test-dos-folder-host-mutant PATH_UNSET rejected
RED dos-folder autoexec
GREEN test-dos-folder-host-mutant NO_AUTOEXEC rejected
```

### test-dos-folder-mutant -- PASS

```text
GREEN test-dos-folder-mutant NO_AUTOEXEC rejected
RED dos-folder finder-folder
GREEN test-dos-folder-mutant DOS_NOT_DIRECTORY rejected
ppm_flair_disk_windows_check: FAIL leg zoomedwin -- grid cell 4 (300,66) is not bare content: 2 unexpected pixels, first at (285,102) -- an icon or label is drawn where DESKTOP.DB / TRASH used to be listed (they must be hidden)
GREEN test-dos-folder-mutant LABEL_TAIL rejected
```

### test-dos-volumes -- PASS

```text
pubs/pubs.mk:384: warning: overriding recipe for target 'test-pubs-examples'
pubs/pubs.mk:379: warning: ignoring old recipe for target 'test-pubs-examples'
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
GREEN test-dos-volumes emu: real FAT/ATA, mtype bytes, mdir both images, fsck.fat clean
```

### test-dos-volumes-emu-mutant -- PASS

```text
>>> boot-system image: build/tracer_mut_safety_volume_prompt.img (BOOT FAT12@63; root[0]=IO.SYS, root[1]=INITDOS.SYS; HSR)
>>> boot-system image: build/tracer_mut_safety_volume_message.img (BOOT FAT12@63; root[0]=IO.SYS, root[1]=INITDOS.SYS; HSR)
RED dos-volumes PROMPT current C
RED dos-volumes INVALID B/D message
GREEN test-dos-volumes-emu-mutant: PROMPT MESSAGE rejected
```

### test-dos-volumes-host -- PASS

```text
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -no-pie -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff -Ibuild harness/diff/fat_diff/dos_volumes_host.c os/milton/int21.c os/milton/devices.c os/milton/fileio_fat.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/sft.c os/milton/psp.c os/milton/mcb.c os/milton/irq.c -o build/dos_volumes_host
GREEN dos-volumes host: path forms, SELDISK/GETDISK, per-drive CWD, SFT/COPY and search identity, absent B/D
GREEN test-dos-volumes host: real FAT/ATA, mtype bytes, mdir both images, fsck.fat clean
```

### test-dos-volumes-mutant -- PASS

```text
RED dos-volumes HANDLE read
RED dos-volumes INVALID selection unchanged
RED dos-volumes LIMIT LASTDRIVE=Z
RED dos-volumes LIMIT LASTDRIVE=Z
GREEN test-dos-volumes-mutant: SELECT PATH CWD HANDLE SEARCH CACHE INVALID LIMIT LASTDRIVE rejected
```

### test-er3h -- PASS

```text
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_int21 os/milton/test_int21.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c os/milton/config_sys.c
>>> test-er3h: AH=33h GET/SET CTRL-BREAK (DL normalize, out-of-scope AL, boot default ON) + CONFIG BREAK= flow (DEC-16)
test_int21: 175 checks, 0 failures
>>> test-er3h: green
```

### test-exec -- PASS

```text
>>> test-exec [4/4]: INT 21h AH=4Bh EXEC ran GREET.COM; AH=4Dh returned rc=7
----------------------------------------------------------------------
VERDICT   : PASS -- InitechDOS loaded a flat .COM BY NAME from a FAT12 volume and ran it (saw + AH=4Bh EXEC)
            (QEMU only; tri-emulator agreement pending beads initech-x0i)
======================================================================
```

### test-fat -- PASS

```text
    BIGCHAIN.TXT 700060 bytes : our == mcopy == python
----------------------------------------------------------------------
VERDICT   : PASS -- our FAT12 reader agrees with mtools AND an independent
            python3 reader on names, sizes, and content (triple oracle).
======================================================================
```

### test-fat-corrupt-fuzz -- PASS

```text
InitechOS (STAPLER) -- make test-fat-corrupt-fuzz : FAT12 corruption fuzzer (deterministic seed sweep)
======================================================================
>>> test-fat-corrupt-fuzz: malformed FAT12 inputs over seeds 1..200 (fail-loud + bounded)
fat12_corrupt_fuzz: ALL GREEN -- 200 seed(s) [1..200], 8000 corruptions exercised; every malformed input handled fail-loud + bounded (no hang, no OOB).
>>> test-fat-corrupt-fuzz: green (every malformed FAT12 input failed loud + bounded -- no hang, no OOB)
```

### test-fat-corrupt-fuzz-mutant -- PASS

```text
	-o build/fat12_corrupt_fuzz_mut_bpb harness/diff/fat_diff/fat12_corrupt_fuzz.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
>>> test-fat-corrupt-fuzz-mutant: confirming the fuzzer DETECTS the no-step-guard + accept-bad-BPB mutants (Rule 6)
>>> test-fat-corrupt-fuzz-mutant: green leg 1/2 (fuzzer RED on the no-step-guard mutant)
>>> test-fat-corrupt-fuzz-mutant: green leg 2/2 (fuzzer RED on the accept-bad-BPB mutant)
>>> test-fat-corrupt-fuzz-mutant: green (fuzzer correctly RED on both corruption mutants)
```

### test-fat-fault-rollback -- PASS

```text
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_fault_rollback harness/diff/fat_diff/test_fat12_fault_rollback.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
>>> test-fat-fault-rollback: FAT12 write-fault rollback atomicity -- nothing leaks (beads initech-lpf3)
test_fat12_fault_rollback: 35 checks, 0 failures
>>> test-fat-fault-rollback: green
```

### test-fat-fault-rollback-mutant -- PASS

```text
>>> test-fat-fault-rollback-mutant: confirming the THREE rollback mutants go RED for the RIGHT reason (Rule 6; beads initech-lpf3)
>>> test-fat-fault-rollback-mutant: green (m-writefile-noroll correctly RED -- its rollback leak bites)
>>> test-fat-fault-rollback-mutant: green (m-growdir-noroll correctly RED -- its rollback leak bites)
>>> test-fat-fault-rollback-mutant: green (m-mkdir-noroll correctly RED -- its rollback leak bites)
>>> test-fat-fault-rollback-mutant: green (ALL THREE rollback mutants ran + RED for the right reason)
```

### test-fat-fuzz -- PASS

```text
>>> test-fat-fuzz [A]: model + remount + FAT-sync over seeds 1..200 (fast, every op)
fat12_fuzz: ALL GREEN -- 200 seed(s) [1..200], 8000 ops exercised, model + remount agreement on every op.
>>> test-fat-fuzz [B]: + mtools/python external cross-check over seeds 1..40
fat12_fuzz: ALL GREEN -- 40 seed(s) [1..40], 1600 ops exercised, model + remount + mtools/python agreement on every op.
>>> test-fat-fuzz: green (deterministic seed sweep -- model + remount + FAT-sync + mtools/python all agree)
```

### test-fat-fuzz-gap-mutant -- PASS

```text
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DFAT12_MUTATE_PARTIAL_STALE_GAP -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff -o build/fat12_fuzz_mut_stale_gap harness/diff/fat_diff/fat12_fuzz.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
>>> fat12: minted BLANK build/fat12_fuzz_blank.img (1.44MB FAT12, no files; generative fuzzer template)
!!! fat12_fuzz: DIVERGENCE on seed 7 (op #39, read_file bytes != model) -- shrinking...
---- SHRUNK REPRODUCER (seed=7) ----
GREEN test-fat-fuzz-gap-mutant: STALE_GAP found and shrunk
```

### test-fat-fuzz-mutant -- PASS

```text
    3 op(s):
      [0] CREATE       FZE.TXT 
      [1] WRITE        FZE.TXT  off=3892 len=256 pat=23365
      [2] WRITE        FZE.TXT  off=4401 len=1 pat=57286
--------------------------------------
```

### test-fat-hygiene-emu -- PASS

```text
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff -o build/test_kernel_hygiene harness/diff/fat_diff/test_kernel_hygiene.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/desktop_db.c
test_kernel_hygiene: 20 checks, 0 failures
GREEN test-fat-hygiene-grow: mtools identical-image differential + fsck.fat
GREEN test-fat-hygiene-emu: shell EXEC directory rename + fragmented growth + mtools/fsck readback
```

### test-fat-hygiene-grow -- PASS

```text
pubs/pubs.mk:379: warning: ignoring old recipe for target 'test-pubs-examples'
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
test_kernel_hygiene: 20 checks, 0 failures
GREEN test-fat-hygiene-grow: mtools identical-image differential + fsck.fat
```

### test-fat-hygiene-grow-emu-mutant -- PASS

```text
>>> boot-system image: build/hygiene_grow.img (BOOT FAT12@63; root[0]=IO.SYS, root[1]=INITDOS.SYS; HSR)
test_kernel_hygiene: 20 checks, 0 failures
GREEN test-fat-hygiene-grow: mtools identical-image differential + fsck.fat
RED FAT hygiene emu: missing KHY-GROW-OK
GREEN test-fat-hygiene-grow-emu-mutant: FAT12_MUTATE_GROW_NO_WRAP correctly RED
```

### test-fat-hygiene-grow-mutant -- PASS

```text
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DFAT12_MUTATE_GROW_NO_WRAP -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff -o build/test_kernel_hygiene_grow harness/diff/fat_diff/test_kernel_hygiene.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/desktop_db.c
  FAIL harness/diff/fat_diff/test_kernel_hygiene.c:45: fov8 directory grows using earlier free clusters
GREEN test-fat-hygiene-grow-mutant: FAT12_MUTATE_GROW_NO_WRAP correctly RED
```

### test-fat-hygiene-hidden -- PASS

```text
pubs/pubs.mk:379: warning: ignoring old recipe for target 'test-pubs-examples'
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
test_kernel_hygiene: 7 checks, 0 failures
GREEN test-fat-hygiene-hidden: mtools identical-image differential + fsck.fat
```

### test-fat-hygiene-hidden-mutant -- PASS

```text
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DFAT12_MUTATE_HIDE_DIR_NO_WRITE -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff -o build/test_kernel_hygiene_hidden harness/diff/fat_diff/test_kernel_hygiene.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/desktop_db.c
  FAIL harness/diff/fat_diff/test_kernel_hygiene.c:52: tdnl.30 runtime TRASH attribute is DIRECTORY|HIDDEN
GREEN test-fat-hygiene-hidden-mutant: FAT12_MUTATE_HIDE_DIR_NO_WRITE correctly RED
```

### test-fat-hygiene-rename -- PASS

```text
pubs/pubs.mk:379: warning: ignoring old recipe for target 'test-pubs-examples'
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
test_kernel_hygiene: 8 checks, 0 failures
GREEN test-fat-hygiene-rename: mtools identical-image differential + fsck.fat
```

### test-fat-hygiene-rename-emu-mutant -- PASS

```text
>>> boot-system image: build/hygiene_rename.img (BOOT FAT12@63; root[0]=IO.SYS, root[1]=INITDOS.SYS; HSR)
test_kernel_hygiene: 20 checks, 0 failures
GREEN test-fat-hygiene-grow: mtools identical-image differential + fsck.fat
RED FAT hygiene emu: missing KHY-RENAME-OK
GREEN test-fat-hygiene-rename-emu-mutant: FAT12_MUTATE_RENAME_REJECT_DIR correctly RED
```

### test-fat-hygiene-rename-mutant -- PASS

```text
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DFAT12_MUTATE_RENAME_REJECT_DIR -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff -o build/test_kernel_hygiene_rename harness/diff/fat_diff/test_kernel_hygiene.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/desktop_db.c
  FAIL harness/diff/fat_diff/test_kernel_hygiene.c:30: f0hh same-directory DIRECTORY rename succeeds
GREEN test-fat-hygiene-rename-mutant: FAT12_MUTATE_RENAME_REJECT_DIR correctly RED
```

### test-fat-move -- PASS

```text
>>> test-fat-move: cross-directory dir-entry transplant + Trash suffix (beads initech-tdnl.26)
test_fat12_move: 149 checks, 0 failures
>>> test-fat-move [host]: transplant legs green -- chain/size/attr/timestamps, both FAT copies and the data clusters proven unchanged
>>> test-fat-move [mtools]: entries transplanted, content identical, folder traversable, free space unchanged
>>> test-fat-move: green
```

### test-fat-move-mutant -- PASS

```text
pubs/pubs.mk:379: warning: ignoring old recipe for target 'test-pubs-examples'
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
>>> test-fat-move-mutant: green (frees-chain correctly RED -- its rule bites)
>>> test-fat-move-mutant: green (ALL SIX MOVE mutants ran + RED for the right reason)
```

### test-fat-partial -- PASS

```text
test_fat12_partial: 204 checks, 0 failures
>>> test-fat-partial [A]: green
>>> test-fat-partial [B]: python3 --cat-range == mcopy+dd of the SAME range (reference proven on real FAT12)
>>> test-fat-partial [B]: green (python --cat-range agrees with mcopy+dd on every range)
>>> test-fat-partial: green
```

### test-fat-partial-mutant -- PASS

```text
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DFAT12_MUTATE_PARTIAL_POS -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_partial_mut_pos harness/diff/fat_diff/test_fat12_partial.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
>>> test-fat-partial-mutant: confirming both positioned-read mutants go RED (Rule 6)
>>> test-fat-partial-mutant: green (skip-count mutant correctly RED)
>>> test-fat-partial-mutant: green (within-cluster-offset mutant correctly RED -- the oracle bites)
```

### test-fat-partition -- PASS

```text
GREEN partition host base=63: directory names/sizes + every file byte == mtools; fsck.fat -n clean
GREEN partition validation: active/first FAT, hidden equality, lengths, signatures, extended rejection
Warning: no active (bootable) partition present
GREEN partition host base=63: directory names/sizes + every file byte == mtools; fsck.fat -n clean
GREEN test-fat-partition
```

### test-fat-partition-emu -- PASS

```text
Warning: no active (bootable) partition present
GREEN partition QEMU rejection SIGNATURE: FAT-MOUNT-FAIL rc=-2 base_lba=0 hidden_sectors=0
Warning: no active (bootable) partition present
GREEN partition QEMU rejection HIDDEN: FAT-MOUNT-FAIL rc=-20 base_lba=63 hidden_sectors=0
GREEN test-fat-partition-emu
```

### test-fat-partition-emu-mutant -- PASS

```text
GREEN partition emu mutant IGNORE_BASE rejected
RED partition EXTENDED accepted invalid MBR
GREEN partition emu mutant EXTENDED rejected
RED partition SIGNATURE accepted invalid MBR
GREEN partition emu mutant SIGNATURE rejected
```

### test-fat-partition-mutant -- PASS

```text
GREEN partition mutant IGNORE_BASE rejected
RED partition EXTENDED must reject rc=0
GREEN partition mutant EXTENDED rejected
RED partition SIGNATURE must reject rc=0
GREEN partition mutant SIGNATURE rejected
```

### test-fat-readfile-mutant -- PASS

```text
	-o build/test_fat12_dir_mut_trunc harness/diff/fat_diff/test_fat12_dir.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
>>> test-fat-readfile-mutant: confirming all THREE streaming read mutants go RED (Rule 6)
>>> test-fat-readfile-mutant: green M1/3 (step-bound(-1) mutant correctly RED)
>>> test-fat-readfile-mutant: green M2/3 (follow-EOC mutant correctly RED)
>>> test-fat-readfile-mutant: green M3/3 (drop-truncation mutant correctly RED -- the oracle bites)
```

### test-fat-subdir -- PASS

```text
    BIGDIR: 40 files across a 3-cluster chain (multi-cluster walk confirmed)
----------------------------------------------------------------------
VERDICT   : PASS -- our subdir reader agrees with mtools AND an independent
            python3 reader on the nested tree (incl. the multi-cluster BIGDIR).
======================================================================
```

### test-fat-subdir-mutant -- PASS

```text
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DFAT12_MUTATE_SUBDIR_NOATTR -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_subdir_mut_noattr harness/diff/fat_diff/test_fat12_subdir.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
>>> test-fat-subdir-mutant: confirming both subdir mutants go RED (Rule 6)
>>> test-fat-subdir-mutant: green (single-sector subdir mutant correctly RED -- BIGDIR loses FILE17+)
>>> test-fat-subdir-mutant: green (no-attr mutant correctly RED -- a file wrongly traversed)
```

### test-fat-truncate-emu -- PASS

```text
pubs/pubs.mk:384: warning: overriding recipe for target 'test-pubs-examples'
pubs/pubs.mk:379: warning: ignoring old recipe for target 'test-pubs-examples'
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
GREEN test-fat-truncate-emu: AH40 position/size, tail release, zero, boundary, subdir, regrow == mtools; fsck.fat
```

### test-fat-truncate-emu-mutant -- PASS

```text
/usr/bin/ld: warning: build/kernel_shell_mut_truncate.elf has a LOAD segment with RWX permissions
objcopy -O binary build/kernel_shell_mut_truncate.elf build/kernel_shell_mut_truncate.bin
>>> boot-system image: build/tracer_mut_truncate.img (BOOT FAT12@63; root[0]=IO.SYS, root[1]=INITDOS.SYS; HSR)
RED truncate: released clusters != mtools
GREEN test-fat-truncate-emu-mutant: FAT12_MUTATE_TRUNCATE_NO_FREE booted and caught RED
```

### test-fat-truncate-gap-emu-mutant -- PASS

```text
cc -std=c11 -Wall -Wextra -Werror -no-pie -Ispec -Ios/milton -Ibuild -Iharness/diff/fat_diff harness/diff/fat_diff/truncate.c os/milton/int21.c os/milton/devices.c os/milton/fileio_fat.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/sft.c os/milton/psp.c os/milton/mcb.c os/milton/irq.c -o build/truncate
nasm -f bin harness/diff/fat_diff/truncate_program.asm -o build/truncate_program.bin
>>> boot-system image: build/j1lu_stale_gap.img (BOOT FAT12@63; root[0]=IO.SYS, root[1]=INITDOS.SYS; HSR)
RED truncate: GAPIN bytes != mcopy
GREEN test-fat-truncate-gap-emu-mutant: STALE_GAP rejected after real AH40/ATA
```

### test-fat-truncate-host -- PASS

```text
GREEN truncate operations
GREEN truncate operations
GREEN truncate operations
GREEN test-fat-truncate-host: AH40 position/size, tail release, zero, boundary, subdir, regrow == mtools; fsck.fat
GREEN truncate rollback: FAT1/FAT2/directory write faults restore identical image and live FAT; short/cycle/free/bad chains refused
```

### test-fat-truncate-host-mutant -- PASS

```text
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
cc -std=c11 -Wall -Wextra -Werror -no-pie -DFAT12_MUTATE_TRUNCATE_NO_FREE -Ispec -Ios/milton -Ibuild -Iharness/diff/fat_diff harness/diff/fat_diff/truncate.c os/milton/int21.c os/milton/devices.c os/milton/fileio_fat.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/sft.c os/milton/psp.c os/milton/mcb.c os/milton/irq.c -o build/truncate_mut
RED truncate: released clusters != mtools
GREEN test-fat-truncate-host-mutant: FAT12_MUTATE_TRUNCATE_NO_FREE caught RED
```

### test-fat-write -- PASS

```text
----------------------------------------------------------------------
VERDICT   : PASS -- a volume written ENTIRELY by InitechDOS reads back correctly
            in mtools AND an independent python3 reader (names, sizes, content)
            AND in our own reader (in-memory round-trip).
======================================================================
```

### test-fat-write-mutant -- PASS

```text
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DFAT12_MUTATE_WRONG_EOC -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_write_mut_eoc harness/diff/fat_diff/test_fat12_write.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
>>> test-fat-write-mutant: confirming both WRITE mutants go RED (Rule 6)
>>> test-fat-write-mutant: green (one-FAT-copy mutant correctly RED)
>>> test-fat-write-mutant: green (wrong-EOC mutant correctly RED -- the oracle bites)
```

### test-fat-write-partial -- PASS

```text
>>> test-fat-write-partial [A]: green
>>> test-fat-write-partial [B]: OUR writer writes the files into a fresh blank image
>>> test-fat-write-partial [C]: content -- mcopy == python3 ref, per positioned-written file
>>> test-fat-write-partial [C]: green (mcopy + python3 agree on every positioned-written file)
>>> test-fat-write-partial: green
```

### test-fat-write-partial-mutant -- PASS

```text
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DFAT12_MUTATE_PARTIAL_EXTEND_SHORT -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_write_partial_mut_ext harness/diff/fat_diff/test_fat12_write_partial.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
>>> test-fat-write-partial-mutant: confirming both positioned-write mutants go RED (Rule 6)
>>> test-fat-write-partial-mutant: green (no-RMW mutant correctly RED)
>>> test-fat-write-partial-mutant: green (short-extend mutant correctly RED -- the oracle bites)
```

### test-fat12-bpb -- PASS

```text
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
>>> test-fat12-bpb: mount + BPB parse + geometry vs verified constants
test_fat12_bpb: 28 checks, 0 failures
>>> test-fat12-bpb: green
```

### test-fat12-chain -- PASS

```text
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
>>> test-fat12-chain: FAT12 12-bit decode + cluster-chain walk + anti-hang guard
test_fat12_chain: 23 checks, 0 failures
>>> test-fat12-chain: green
```

### test-fat12-dir -- PASS

```text
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
>>> test-fat12-dir: root-dir enumerate + find + file-read (byte-for-byte golden)
test_fat12_dir: 83 checks, 0 failures
>>> test-fat12-dir: green
```

### test-fat12-mkdir -- PASS

```text
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff \
	-o build/test_fat12_mkdir harness/diff/fat_diff/test_fat12_mkdir.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
>>> test-fat12-mkdir: MKDIR/RMDIR write side vs mmd golden ('.'/'..'/EOC byte-for-byte)
test_fat12_mkdir: 51 checks, 0 failures
>>> test-fat12-mkdir: green
```

### test-fat12-mkdir-mutant -- PASS

```text
>>> test-fat12-mkdir-mutant: confirming all four MKDIR/RMDIR mutants go RED (Rule 6)
>>> test-fat12-mkdir-mutant: green (dotdot-self mutant correctly RED -- the .. rule bites)
>>> test-fat12-mkdir-mutant: green (no-EOC mutant correctly RED -- the EOC link bites)
>>> test-fat12-mkdir-mutant: green (no-empty-check mutant correctly RED -- the empty-check bites)
>>> test-fat12-mkdir-mutant: green (stale-trusted mutant correctly RED -- mkdir into a purged directory cluster is refused, L002)
```

### test-fat12-subdir -- PASS

```text
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
>>> test-fat12-subdir: subdir enumerate + path resolve (byte-for-byte golden)
test_fat12_subdir: 64 checks, 0 failures
>>> test-fat12-subdir: green
```

### test-fat12-write -- PASS

```text
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
>>> test-fat12-write: FAT12 WRITE unit (12-bit encode + alloc + both-FAT sync + round-trip + full-volume fail-loud + unlink)
test_fat12_write: 97 checks, 0 failures
>>> test-fat12-write: green
```

### test-fat16 -- PASS

```text
    HIGHCLUS.BIN 2621440 bytes : our == mcopy == python
----------------------------------------------------------------------
VERDICT   : PASS -- our FAT16 reader agrees with mtools AND an independent
            python3 reader on names, sizes, and content (triple oracle).
======================================================================
```

### test-fat16-emu -- PASS

```text
pubs/pubs.mk:384: warning: overriding recipe for target 'test-pubs-examples'
pubs/pubs.mk:379: warning: ignoring old recipe for target 'test-pubs-examples'
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
GREEN test-dos-volumes fat16: real FAT/ATA, mtype bytes, mdir both images, fsck.fat clean
```

### test-fat16-emu-mutant -- PASS

```text
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
GREEN test-dos-volumes fat16: real FAT/ATA, mtype bytes, mdir both images, fsck.fat clean
>>> boot-system image: build/j1lu_volumes_fat16.img (BOOT FAT12@63; root[0]=IO.SYS, root[1]=INITDOS.SYS; HSR)
RED dos-volumes FAT16 ATA TYPE bytes
GREEN test-fat16-emu-mutant: FAT16 byte corruption rejected
```

### test-fat16-mutant -- PASS

```text
>>> test-fat16-mutant: green (M2 correctly RED -- its FAT16 read primitive bites)
>>> test-fat16-mutant: green (M3 correctly RED -- its FAT16 read primitive bites)
>>> test-fat16-mutant: green (M4 correctly RED -- its FAT16 read primitive bites)
>>> test-fat16-mutant: green (M5 correctly RED -- its FAT16 read primitive bites)
>>> test-fat16-mutant: green (ALL FIVE FAT16 mutants ran + RED for the right reason)
```

### test-fatwrite -- PASS

```text
>>> test-fatwrite [4/4]: mtools read OUT.TXT off the disk -- content == "hello\r\n" (independent confirmation)
----------------------------------------------------------------------
VERDICT   : PASS -- InitechDOS WROTE a file to a FAT12 volume over ATA and read it back
            (QEMU only; tri-emulator agreement pending beads initech-x0i)
======================================================================
```

### test-fcb-api -- PASS

```text
>>> test-fcb-api [2/4]: transcript == real-DOS golden (23 lines, T01..T20)
>>> test-fcb-api [3/4]: mtools FAT differential: FCB*.DAT names/attrs/sizes == real DOS
>>> test-fcb-api [4/4]: FCBKEPT.DAT (sequential + random 10-byte records) byte-identical to real DOS
VERDICT   : PASS -- the FCB API agrees with real DOS (QEMU only; Bochs/86Box legs not run)
======================================================================
```

### test-fdisk -- PASS

```text
Warning: no active (bootable) partition present
GREEN fdisk create 513 cylinders: MBR table+55AA == mpartition, sfdisk dump equal
Warning: no active (bootable) partition present
GREEN fdisk FAT on 513-cylinder disk (66465 sectors, spc=1): kernel file bytes == mcopy; fsck.fat clean
GREEN test-fdisk: create/type/CHS/signature, active entry, default-No and confirmed deletion differential
```

### test-fdisk-emu -- PASS

```text
GREEN fdisk QEMU active: injected partition 2 == sfdisk --activate 2
GREEN fdisk QEMU delete: default-No leaves every disk byte intact
GREEN fdisk QEMU delete: explicit Y/Return == sfdisk --delete 1
GREEN fdisk QEMU loaded-volume refusal: option 5 exits, data volume byte-identical
GREEN test-fdisk-emu
```

### test-fdisk-emu-mutant -- PASS

```text
RED fdisk-emu CHS/signature create MBR vs mpartition
RED fdisk-emu active flag on wrong entry
RED fdisk-emu delete without confirmation
RED fdisk-emu loaded volume was not refused
GREEN test-fdisk-emu-mutant: CHS SIGNATURE ACTIVE CONFIRM LOADED rejected
```

### test-fdisk-mutant -- PASS

```text
RED fdisk CHS/relative-sector/type/signature vs mpartition
RED fdisk CHS/relative-sector/type/signature vs mpartition
RED fdisk active flag on wrong entry
RED fdisk delete without confirmation
GREEN test-fdisk-mutant: CHS SIGNATURE ACTIVE CONFIRM rejected
```

### test-fileio -- PASS

```text
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_fileio os/milton/test_fileio.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c
>>> test-fileio: OPEN/READ/CLOSE/LSEEK + FINDFIRST/NEXT + SETDTA/GETDTA via mock backend (initech-509.5)
test_fileio: 225 checks, 0 failures
>>> test-fileio: green
```

### test-fixture-volumes -- PASS

```text
pubs/pubs.mk:384: warning: overriding recipe for target 'test-pubs-examples'
pubs/pubs.mk:379: warning: ignoring old recipe for target 'test-pubs-examples'
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
>>> test-fixture-volumes: green (355 scratch names vs 152 fixture names; 70 inline image writers)
```

### test-flair-app-launch -- PASS

```text
>>> ppm_flair_app_launch_check [bar-photoshop]: green
>>> test-flair-app-launch [9/10] BADMBAR: both out-of-image MenuBars refused TENANT-SETMBAR-BAD, and again after the window; band 2 is exactly the shell fallback
cp -f build/flair_data.img build/flair_appl_menu_data.img
>>> test-flair-app-launch [10/10] MENU: band 2 dropped the tenant menu 131 from its own resource; release selected nothing; clean exit after
>>> test-flair-app-launch: green
```

### test-flair-exit-to-dos -- PASS

```text
>>> test-flair-exit-to-dos [2/3]: CANCEL -- Escape dismissed it, nothing put away, the Finder still answers (Ctrl-N)
cp -f build/flair_data.img build/flair_exitdos_exit_data.img
build/qemu_harness --disk "build/flair_tenants.img" --disk2 "build/flair_exitdos_exit_data.img" --expect FLAIR-FAT-MOUNT-OK --name "flair_exitdos_exit" --out "build" --mouse "m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:-8,m-100:-8,m-100:-8,m-100:-8,m-30:-2,l1,m0:100,m0:33,l0,m100:2,m100:2,m94:1,l1,l0,WSHELL-READY,kd,ki,kr,kret" --keys-after "FLAIR-LIVE-READY" --quit-after "bytes free" --screendump --screendump-after "bytes free" --timeout-ms 40000 2> "build/flair_exitdos_exit.report" || true
>>> test-flair-exit-to-dos [3/3]: EXIT -- put away + DESKTOP.DB commit -> FLAIR-EXIT-TO-DOS -> transient COMMAND.COM (COMMAND-TRANSIENT-OK bytes=42312) -> banner == spec -> SHELL-READY -> DIR lists A: (INITECH); the screen is the text console (ppm_text_check)
>>> test-flair-exit-to-dos: green (QEMU; Bochs leg owed)
```

### test-flair-finder-service -- PASS

```text
cp -f build/flair_data.img build/flair_finder_service_data.img
>>> test-flair-finder-service [1/2]: root shows n=4 (pixel-graded, cells 4-5 bare); README staged, DB written (80 B); mtools still lists DESKTOP.DB + TRASH
cp -f build/flair_damaged_data.img build/flair_finder_service_damaged_data.img
>>> test-flair-finder-service [2/2]: damaged volume boots; root n=4; NEWFOLD staged in ::/TRASH; stranded APPS\TRASH opens as an ordinary folder and keeps README.TXT (mtools)
>>> test-flair-finder-service: green
```

### test-flair-locked-purge -- PASS

```text
    OK button label                                      [439,490)x[161,176): 71 ink px
    Cancel only in confirmation                          [367,418)x[161,176): 0 ink px
    FULL Trash (TRASH_FULL)                              @(584,404): 0/1024 cells differ from the capture
ppm_flair_trash_check: leg locked-full PASS
VERDICT: PASS -- test-flair-locked-purge (locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat)
```

### test-flair-locked-trash-mutant -- PASS

```text
VERDICT: PASS -- test-flair-locked-trash-mutant locked_stage RED (locked file left source)
FAIL locked-trash: locked notice pixels missing
VERDICT: PASS -- test-flair-locked-trash-mutant locked_notice RED (locked notice pixels missing)
FAIL locked-trash: visible message missing
VERDICT: PASS -- test-flair-locked-trash-mutant locked_purge RED (visible message missing)
```

### test-flair-tps-journey -- PASS

```text
>>> test-flair-tps-journey [lint]: the trace keys the ritual EXACTLY as keys_a.txt (60 keystrokes)
cp -f build/flair_data.img build/flair_tps_journey_data.img
build/qemu_harness --disk "build/flair_tenants.img" --disk2 "build/flair_tps_journey_data.img" --expect FLAIR-FAT-MOUNT-OK --name "flair_tps_journey" --out "build" --mouse "m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:-8,m-100:-8,m-100:-8,m-100:-8,m-30:-2,l1,m0:100,m0:33,l0,m100:2,m100:2,m94:1,l1,l0,WSHELL-READY,kc,kd,kspc,ka,kp,kp,ks,kret,kt,kp,ks,kr,ke,kp,kspc,kt,kp,ks,kdot,kd,kb,kf,kret,kspc,kspc,kspc,kspc,kspc,kspc,kret,kspc,kspc,kspc,kspc,kspc,kspc,kspc,kspc,kret,kspc,kspc,kspc,kspc,kspc,kspc,kspc,kret,k4,k4,k1,k7,kret,k4,k4,k7,k1,kret,k4,k4,k1,k7,kret,k4,k4,k1,k7,kret,k8,k8,kminus,k1,k1,k3,kret,k8,k8,kminus,k1,k1,k4,kret,kx,ky,WTPS REPORT GENERATOR TERMINATED" --keys-after "FLAIR-LIVE-READY" --quit-after "TPS REPORT GENERATOR TERMINATED. RETURN CODE 0." --screendump --screendump-after "TPS REPORT GENERATOR TERMINATED. RETURN CODE 0." --timeout-ms 60000 2> "build/flair_tps_journey.report" || true
>>> test-flair-tps-journey: Exit -> A:\> -> CD APPS -> TPSREP TPS.DBF -> 6/8 presses refused, 7 aligned, number keyed twice, memo 88-114, cover sheet Y -> transcript (88 lines) == real DOS; APPS\TPSREPT.TXT (2432 B) == real DOS
>>> test-flair-tps-journey: green (QEMU; Bochs leg owed)
```

### test-formatsys -- PASS

```text
GREEN test-formatsys: /1 /4 /8 /T /N geometry and media match mformat; every volume SPC1 and fsck.fat clean
GREEN test-formatsys: /B reserves system slots and clusters, SYS fills reservations; fsck.fat clean
GREEN test-formatsys: FORMAT C: /S preserves source bytes in place; original bootstrap prefix retained
GREEN test-formatsys: SYS fills deleted first slots, preserves KEEP.TXT, writes fragmented chains mcopy/fsck.fat accept
GREEN test-formatsys: Format another Y repeats; an empty second /V clears the first label
```

### test-formatsys-emu -- PASS

```text
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
GREEN formatsys independent image layout
GREEN formatsys independent image layout
GREEN test-formatsys-emu: injected FORMAT /S /V and SYS, manual text decoded from pixels, images independently checked, running-volume refusal
```

### test-formatsys-emu-mutant -- PASS

```text
RED formatsys FAT[0] media/EOC
RED formatsys /S COMMAND.COM
RED formatsys root entry order
RED formatsys-emu running volume not refused
GREEN test-formatsys-emu-mutant: MEDIA FAT0 COMMAND ORDER RUNNING rejected
```

### test-formatsys-mutant -- PASS

```text
RED formatsys DISKCOMP missing differing sector
RED formatsys running volume not refused
RED formatsys /B reserve bytes
RED formatsys /V repeat label fsck
GREEN test-formatsys-mutant: MEDIA FAT0 COMMAND ORDER COMPARE RUNNING RESERVE LABEL rejected
```

### test-fs -- PASS

```text
>>> test-fs [4/4]: mount listing stays on serial; console shows the exact secondary prompt
----------------------------------------------------------------------
VERDICT   : PASS -- InitechDOS mounted FAT12 over ATA; root scan on serial, clean DOS console
            (QEMU only; tri-emulator agreement pending beads initech-x0i)
======================================================================
```

### test-int21 -- PASS

```text
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_int21 os/milton/test_int21.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c os/milton/config_sys.c
>>> test-int21: AH-dispatch + console subset (02h/09h/40h/30h/4Ch) + controlled scope
test_int21: 175 checks, 0 failures
>>> test-int21: green
```

### test-int21-edge -- PASS

```text
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_int21_edge os/milton/test_int21_edge.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c
>>> test-int21-edge: INT 21h error paths (bad/closed handle, double-close, CX=0, short read) + 0Eh/19h/47h/59h/62h via mock backend (initech-xrd/1zk)
test_int21_edge: 101 checks, 0 failures
>>> test-int21-edge: green
```

### test-kernel-certificate -- FAIL(2)

```text
AH=21 gate=test-fcb-api
AH=22 gate=test-fcb-api
AH=23 gate=test-fcb-api
AH=24 gate=test-fcb-api
make: *** [harness/diff/fat_diff/dos_volumes.mk:84: test-kernel-coverage] Error 1
```

### test-kernel-coverage -- FAIL(2)

```text
AH=21 gate=test-fcb-api
AH=22 gate=test-fcb-api
AH=23 gate=test-fcb-api
AH=24 gate=test-fcb-api
make: *** [harness/diff/fat_diff/dos_volumes.mk:84: test-kernel-coverage] Error 1
```

### test-kernel-coverage-mutant -- PASS

```text
pubs/pubs.mk:379: warning: ignoring old recipe for target 'test-pubs-examples'
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
RED kernel-coverage register dropped dispatcher AH=30: restore the function in spec/int21h_register.json
GREEN test-kernel-coverage-mutant: DROP_REGISTER_GETVER rejected
```

### test-kernel-headroom -- PASS

```text
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
>>> test-kernel-headroom: 239 kernels measured; window 196608 B (384 sectors); floor 4096 B
    flagship kernel_flairtenants.elf                              178964 B loaded, headroom  17644 B
    LARGEST  kernel_flairtenants_db_write_skip.elf                180212 B loaded, headroom  16396 B (binding)
>>> test-kernel-headroom: green
```

### test-makefile-vars -- PASS

```text
pubs/pubs.mk:379: warning: ignoring old recipe for target 'test-pubs-examples'
pubs/pubs.mk:390: warning: overriding recipe for target 'test-pubs-mutant'
pubs/pubs.mk:381: warning: ignoring old recipe for target 'test-pubs-mutant'
>>> test-makefile-vars: no rule reads a variable before its definition (tdnl.81)
>>> test-makefile-vars: green
```

### test-sft -- PASS

```text
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed \
	-o build/test_sft os/milton/test_sft.c os/milton/sft.c os/milton/psp.c
>>> test-sft: JFT->SFT handle layer + predefined handles 0-4 + DUP/DUP2 (initech-509.3)
test_sft: 56 checks, 0 failures
>>> test-sft: green
```

### test-shell -- PASS

```text
>>> test-shell [7/7]: screendump shows the "A:\>" COMMAND.COM prompt rendered on the seafoam desktop
----------------------------------------------------------------------
VERDICT   : PASS -- booted InitechDOS, got an A:\> prompt, ran DIR/TYPE/a program/EXIT via COMMAND.COM
            (QEMU only; tri-emulator agreement pending beads initech-x0i)
======================================================================
```

### test-spec -- PASS

```text
>>> test-spec [6/6]: INT 25h/26h absolute-disk vectors JSON (DEC-15; SEPARATE from the AH walk)
    INT 25h/26h: 2 trap vectors (0x8F/DPL0/0x08); EBX-buffer swap pinned; AL/AH pairs locked; CX=0xFFFF rejected; scratch-free rule present; EXCLUDED from the AH walk
----------------------------------------------------------------------
VERDICT   : PASS -- ADR-0003 spec-data parses, sizes hold, banner verbatim, INT 25h/26h locked
======================================================================
```

The original no-RMW mutant's full final RED line (the make wrapper prints
only its shrunk reproducer) is also retained:

```text
!!! fat12_fuzz: DIVERGENCE on seed 1 (op #30, read_file bytes != model) -- shrinking...
---- SHRUNK REPRODUCER (seed=1) ----
```
