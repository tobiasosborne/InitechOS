# WL-0102 -- DOS control chords and cooked CON recovery (K12)

## Context

Bead initech-j1lu, lane-j1lu, base 1c93dde. The operator required targeted
host/QEMU gates, INITECH_QMP_STDIO=1, no Bochs, no bd or push, and a private
Git metadata copy under build/lane.git. No sibling directory was written.
The two DOS 3.3 manuals were converted with pdftotext -layout into build/.

The audit pins relocate at this base to kbd.h:72/75 (Shift/Caps only),
kbd.c:119/158 (translator/case handling), int21.c:1869 (raw CON adapter),
int21.c:2912 (named-device READ), and int21.c:1217 (existing cooked ^C
checkpoint). An additional cause was the resident built-in taking the default
INT 23h process termination path, which would halt COMMAND.COM and reclaim
the saved redirection handles.

## What changed

- Track left/right Ctrl independently and consume E0 before decoding its
  following byte. Ctrl+A..Z becomes 01h..1Ah regardless of Shift/Caps.
  Extended keypad Enter/slash work; PrintScreen fake Shifts cannot latch case.
- Share the cooked handle reader between stdin and OPEN CON. Collect through
  Enter, cut at Ctrl-Z, preserve short-read remainders, and return a final
  AX=0 after any preceding text. The next command can read the console again.
  AH=0Ah retains its byte layout; AH=06h/07h retain raw no-check contracts.
- Add a PSP-owned returning INT 23h recovery scope. Resident built-ins return
  through their normal CLOSE/DUP2 epilogues; pipes stop and remove temporaries,
  batches unwind, and the shell reprompts with its handles and CWD intact.
  A child PSP still takes the existing terminating abort path. COPY does not
  print a success footer after an abort. Invalid recovery/staging state panics.
- Extend keyboard/CON host oracles and register test-j1lu-con-unit,
  test-j1lu-kbd-mutant, test-j1lu-con-mutant, test-j1lu-control-emu and
  test-j1lu-control-emu-mutant in the Makefile vectors. The QEMU gate exercises
  COPY CON, TYPE CON, piped TYPE CON, redirection and a subdirectory abort.
  mcopy independently checks T.TXT = hello + CRLF and U.TXT = more + CRLF.

## Why

Law 1 sources, with printed page numbers (PDF page numbers in parentheses):

- 80X0945_DOS_3.30_Technical_Reference_Apr87.pdf: 6-14/15 (137/138),
  INT 23h and returning handlers; 6-51 (174), process termination;
  6-52 (175), AH=01h;
  6-57 (180) and 6-59 (182), AH=06h/07h no checks; 6-60 (183),
  AH=08h break check; 6-62 (185), buffered input through Enter;
  6-137/138 (260/261), handle READ and AX=0 EOF; 6-149 (272),
  ASCII device mode checks Ctrl-Z.
- MS-DOS_3.3_Users_Guide_198707.pdf: User's Reference p. 132 (217),
  COPY CON ends with Ctrl-Z then Return; pp. 158-159 (243-244), holding
  Ctrl while pressing a letter and aborting the current command.

DEC-04a's flat register/CF convention and DEC-16's existing CON checkpoint
remain the contract. No locked spec-data changed and no new table was needed.
The broader BREAK ON sweep beyond CON remains the existing DEC-16 C-6 scope.

## Frictions

The initial host RED was test-kbd-unit: 325 checks, 32 failures and
test_devwire: 46 checks, 4 failures. The initial emulator failed at
"prompt did not return before eofok" twice, the second run isolated.
The shell recovery oracle separately went RED: 56 checks, 6 failures
(termination and lost handles), before wiring the recovery handler.

Running test-zs24 beside test-zs24-mutant caused a shared-fixture failure:
"python --list-path SUB does not list GROW01.TXT". The isolated test-zs24
rerun passed all five legs, including GROW12. Emulator gates ran serially;
every new emulator mutant ran twice in isolation. Bochs was not run under
the operator's hard constraint. No aggregate test vector or real-DOS emulator
differential was run or claimed.

## Acceptance

Final new gate evidence (all targets invoked with make; QEMU inherited
INITECH_QMP_STDIO=1):

| Target / variant | Final evidence line |
| --- | --- |
| test-kbd-unit (extended existing gate) | test-kbd-unit: 325 checks, 0 failures |
| test-j1lu-con-unit | test_devwire: 129 checks, 0 failures; >>> test-j1lu-con-unit: green |
| test-j1lu-kbd-mutant / ctrl | green (ctrl correctly RED: test-kbd-unit: 325 checks, 29 failures) |
| test-j1lu-kbd-mutant / e0 | green (e0 correctly RED: test-kbd-unit: 325 checks, 5 failures) |
| test-j1lu-kbd-mutant / release | green (release correctly RED: test-kbd-unit: 325 checks, 2 failures) |
| test-j1lu-con-mutant / eof | green (eof correctly RED: test_devwire: 129 checks, 4 failures) |
| test-j1lu-con-mutant / raw | green (raw correctly RED: test_devwire: 129 checks, 12 failures) |
| test-j1lu-con-mutant / abort | green (abort correctly RED: test_devwire: 95 checks, 6 failures) |
| test-j1lu-control-emu | >>> test-j1lu-control-emu: green (EOF + abort prompts, COPY bytes=mcopy, redirection + pipe cleanup) |
| test-j1lu-control-emu-mutant / ctrl | ctrl attempt 2 correctly RED (booted; prompt did not return before eofok) |
| test-j1lu-control-emu-mutant / eof | eof attempt 2 correctly RED (booted; prompt did not return before eofok) |
| test-j1lu-control-emu-mutant / raw | raw attempt 2 correctly RED (booted; prompt did not return before eofok) |
| test-j1lu-control-emu-mutant / abort | abort attempt 2 correctly RED (booted; prompt did not return before abortok) |
| test-j1lu-control-emu-mutant / restore | restore attempt 2 correctly RED (booted; prompt did not return before restored) |

The final emulator mutation target ends:
`>>> test-j1lu-control-emu-mutant: green (ctrl/eof/raw/abort/restore each RED twice, isolated QEMU reruns)`.

Permitted regressions also passed: test-console, test-kbd-unit-mutant,
test-conin-unit, test-conin-mutant, test-x8fs, test-x8fs-mutant, test-4tw,
test-4tw-mutant, test-devices, test-devices-mutant, test-devwire,
test-devwire-mutant, test-er3h, test-er3h-mutant, test-fileio,
test-kji0-mutant, test-zs24 (isolated rerun), test-zs24-mutant,
test-kbd, test-conin, test-ut6d, test-ut6d-mutant, test-zs24-exec,
test-zs24-exec-mutant, test-spec, test-makefile-vars and test-kernel-repro.
Evidence includes `test_conin: 83 checks, 0 failures`,
`test_int21: 175 checks, 0 failures`, `test_fileio: 225 checks, 0 failures`,
`>>> test-zs24: green`, and the QEMU PASS verdicts / mutant green lines.
Reproducibility:
`>>> test-kernel-repro: green -- kernel.bin reproducible (sha256 6770e876ac846d76e4c3a9904f3ce8f90ef9d72d3e7eab6da1e50e526bd61049)`.
ASCII checks and git diff --check passed.

Loaded headroom was printed from objcopy's unpadded bytes in the same
196608-byte window as the Makefile guard; all measured kernels exceed the
4096-byte floor. The full kernel-headroom vector was not invoked.

| Kernel ELF suffix | Loaded bytes | Headroom bytes |
| --- | ---: | ---: |
| flairtenants | 187592 | 9016 |
| flairtenants_mut_no_raise_on_title (previous binding mutant) | 188264 | 8344 |
| shell_secondary | 101172 | 95436 |
| shell_j1lu_ctrl | 101128 | 95480 |
| shell_j1lu_eof | 101121 | 95487 |
| shell_j1lu_raw | 101153 | 95455 |
| shell_j1lu_abort | 101153 | 95455 |
| shell_j1lu_restore | 101204 | 95404 |
| shell_mut_ut6d | 100892 | 95716 |
| shell_rdnoop_ut6d | 101116 | 95492 |
| shell_mut_zs24reject | 101217 | 95391 |
| shell_mut_zs24rootonly | 101228 | 95380 |

Baseline flagship / binding mutant loaded 187848 / 188552 bytes. This lane
shrinks them by 256 / 288 bytes, within the under-2-KiB growth constraint.
The flagship and mutant kernel placement guards passed.

## Pointers

os/milton/kbd.c, kbd.h, int21.c, int21.h, sft.h, command.c;
os/milton/test_kbd.c, test_conin.c, test_devwire.c;
harness/emu/test_dos_control.sh and dos_control.mk;
docs/worklog/WL-0028-Forward-Tranche-80k-d27i-x8fs-CON-Chain-plus-dBASE-Research.md;
docs/audits/2026-10-04-flair-gui-codex-pass5/{TRIAGE,REPORT}.md K12.

Local evidence: build/j1lu-red-host.log, j1lu-red-devwire.log,
j1lu-red-emu-alone.log, j1lu-red-recovery.log, j1lu-final-host.log,
j1lu-final-mutants-emu.log, j1lu-regression-host.log,
j1lu-zs24-alone.log, j1lu-regression-emu.log, j1lu-spec.log,
j1lu-repro.log and j1lu-headroom-all-built.log. The images and evidence are
build intermediates. The handoff is build/lane.bundle (1c93dde..lane-j1lu),
verified with git bundle verify; no push and no remaining K12 implementation
work. The operator/PM owns bead status and integration.
