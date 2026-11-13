# Legacy emulator migration

Source: legacy-final gate recipes, checked against the frozen Makefile.
A family combines only emulator-backend and mutant suffix variants. This table
is a migration plan, not a claim that all mapped scenarios already run.

| Category | Families | Legacy gate entries |
|---|---:|---:|
| SESSION | 26 | 42 |
| PROGRAM | 40 | 63 |
| DIAGNOSTIC | 12 | 23 |
| RETIRED | 4 | 9 |

Total: 82 families, 137 original entries.

`make -C factory smoke-programs` now runs MULTI, FCBX, NEST (with
GREET, TERM20 and DEPTH children), SERVICES, WRITE and CONIN on QEMU and
Bochs. The test volume is minted separately and never installed on either
product disk. MULTI proves three live handles, independent positions and a read
beyond 64 KiB. NEST asserts rc=7, one-shot AH=4Dh, INT20 return, four nested
PSPs and restored CWD/parent memory. SERVICES covers allocate/resize/free,
GETVECT/SETVECT and date/time ranges. CONIN uses real keyboard input. WRITE
is independently read back with mtools. FCBX compares all 23 transcript lines,
surviving names/attributes/sizes and record bytes with real MS-DOS 6.22 captures
(the same source assembled REALDOS). Those captures were claimed from the
legacy FCB differential, not generated from InitechDOS.

Diagnostic work is deliberately deferred: link one additional diagnostic object
only into a tier-2 image, with a file selecting the probe at boot. It may expose
a bounded fault-injection control or counters to a disk program. The test drives
the image externally and requires the actual fault/IRQ/panic evidence. No BOOT_*
personalities, test logic in production startup, or fake successful target.

| Family (original gates) | Category | Re-proof mechanism |
|---|---|---|
| test-absdisk-emu | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-ansi-emu, test-ansi-emu-bochs, test-ansi-emu-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-ata-no-drive-emu, test-ata-no-drive-emu-bochs, test-ata-no-drive-emu-mutant | DIAGNOSTIC | One tier-2 diagnostic object, selected by a boot-disk file; inject/observe the fault and assert panic/IRQ trace externally. |
| test-baked-exec-guard, test-baked-exec-guard-bochs, test-baked-exec-guard-mutant | RETIRED | Retired baked image or exact-address command-layout guard; normal COMMAND.COM launch remains in session. |
| test-block-device-emu, test-block-device-emu-bochs, test-block-device-emu-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-boot-novbe-bochs | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-boot-vga13-bochs | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-bsy8-pipe, test-bsy8-pipe-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-command-c-emu, test-command-c-emu-bochs, test-command-c-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-command-file-emu, test-command-file-emu-bochs, test-command-file-emu-mutant | RETIRED | Retired baked image or exact-address command-layout guard; normal COMMAND.COM launch remains in session. |
| test-command-file-negative, test-command-file-negative-bochs | RETIRED | Retired baked image or exact-address command-layout guard; normal COMMAND.COM launch remains in session. |
| test-conin | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-conkeys, test-conkeys-bochs | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-conkeys-emu-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-datetime | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-del-directory-b023 | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-del-directory-emu, test-del-directory-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dir | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dir-args, test-dir-args-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dir-match-emu-bochs | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dir-page | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-break-emu-bochs | DIAGNOSTIC | One tier-2 diagnostic object, selected by a boot-disk file; inject/observe the fault and assert panic/IRQ trace externally. |
| test-dos-not-ready-emu-bochs | DIAGNOSTIC | One tier-2 diagnostic object, selected by a boot-disk file; inject/observe the fault and assert panic/IRQ trace externally. |
| test-dos-stamp | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-stamp-copy | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-stamp-copy-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-stamp-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-startup, test-dos-startup-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-volumes | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-volumes-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-drive-operands, test-drive-operands-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-edlin-emu, test-edlin-emu-bochs, test-edlin-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-exec | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-exec-high, test-exec-high-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-exec-high-dirty | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-exec-high-exits | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-exec-high-io | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-exec-high-lowmem | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-exec-high-ram | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-exec-nested, test-exec-nested-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-exec-overlay, test-exec-overlay-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-exit-handles, test-exit-handles-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-f3template, test-f3template-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-fat-hygiene-emu, test-fat-hygiene-emu-bochs | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-fat-hygiene-grow-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-fat-hygiene-rename-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-fat-partition-emu, test-fat-partition-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-fat-spc-emu | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-fat-truncate-emu, test-fat-truncate-emu-bochs, test-fat-truncate-emu-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-fat-truncate-gap-emu-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-fat16-emu, test-fat16-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-fatwrite | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-fcb-api, test-fcb-api-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-fpu, test-fpu-bochs, test-fpu-mutant | DIAGNOSTIC | One tier-2 diagnostic object, selected by a boot-disk file; inject/observe the fault and assert panic/IRQ trace externally. |
| test-fpu-absent, test-fpu-absent-mutant | DIAGNOSTIC | One tier-2 diagnostic object, selected by a boot-disk file; inject/observe the fault and assert panic/IRQ trace externally. |
| test-fpu-mf, test-fpu-mf-mutant | DIAGNOSTIC | One tier-2 diagnostic object, selected by a boot-disk file; inject/observe the fault and assert panic/IRQ trace externally. |
| test-fs | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-int21-dpb-emu, test-int21-dpb-emu-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-int21-irqstorm, test-int21-irqstorm-mutant | DIAGNOSTIC | One tier-2 diagnostic object, selected by a boot-disk file; inject/observe the fault and assert panic/IRQ trace externally. |
| test-j1lu-control-emu, test-j1lu-control-emu-bochs, test-j1lu-control-emu-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-kbd | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-kernel-certificate | RETIRED | Retired baked image or exact-address command-layout guard; normal COMMAND.COM launch remains in session. |
| test-mcb-emu, test-mcb-emu-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-multiopen | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-mzexec, test-mzexec-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-mzload-entry-emu, test-mzload-entry-emu-bochs, test-mzload-entry-emu-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-panic | DIAGNOSTIC | One tier-2 diagnostic object, selected by a boot-disk file; inject/observe the fault and assert panic/IRQ trace externally. |
| test-panic-reasons, test-panic-reasons-mutant | DIAGNOSTIC | One tier-2 diagnostic object, selected by a boot-disk file; inject/observe the fault and assert panic/IRQ trace externally. |
| test-panic-screen, test-panic-screen-bochs, test-panic-screen-mutant | DIAGNOSTIC | One tier-2 diagnostic object, selected by a boot-disk file; inject/observe the fault and assert panic/IRQ trace externally. |
| test-path-command, test-path-command-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-program | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-readerr-winh, test-readerr-winh-mutant | DIAGNOSTIC | One tier-2 diagnostic object, selected by a boot-disk file; inject/observe the fault and assert panic/IRQ trace externally. |
| test-spurious | DIAGNOSTIC | One tier-2 diagnostic object, selected by a boot-disk file; inject/observe the fault and assert panic/IRQ trace externally. |
| test-sysinit | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-sysinit-oversize | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-trash-hidden-emu, test-trash-hidden-emu-bochs, test-trash-hidden-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-tsr, test-tsr-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-tsr-exec, test-tsr-exec-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-type | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-ut6d, test-ut6d-mutant | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-vect | PROGRAM | Assemble a normal disk program with dos/tests-programs.mk; run it at the prompt and check literal service results plus mtools readback. |
| test-zs24-exec, test-zs24-exec-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
