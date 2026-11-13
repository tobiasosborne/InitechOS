# Legacy emulator migration

Source: legacy-final gate recipes, checked against the frozen Makefile.
A family combines only emulator-backend and mutant suffix variants. This table
is a migration plan, not a claim that all mapped scenarios already run.

| Category | Families | Legacy gate entries |
|---|---:|---:|
| SESSION | 40 | 80 |
| PROGRAM | 0 | 0 |
| DIAGNOSTIC | 0 | 0 |
| RETIRED | 0 | 0 |

Total: 40 families, 80 original entries.

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
| test-autoexec, test-autoexec-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-batch-break-emu, test-batch-break-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-bsy7-redir, test-bsy7-redir-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-bsy9-redir, test-bsy9-redir-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-chkdsk-emu, test-chkdsk-emu-bochs, test-chkdsk-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-copy-selfcopy, test-copy-selfcopy-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-batch2-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-batch2-body | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-batch2-console | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-batch2-cwd | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-batch2-for | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-batch2-prompt | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-folder, test-dos-folder-bochs, test-dos-folder-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-safety-audit, test-dos-safety-audit-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-safety-copy-ro, test-dos-safety-copy-ro-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-safety-del-failure, test-dos-safety-del-failure-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-safety-del-ro, test-dos-safety-del-ro-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-safety-del-ro-progress-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-safety-k01, test-dos-safety-k01-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-safety-k02, test-dos-safety-k02-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-safety-k03, test-dos-safety-k03-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-safety-k04, test-dos-safety-k04-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dos-safety-k15, test-dos-safety-k15-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-dosutil-emu, test-dosutil-emu-bochs, test-dosutil-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-fdisk-emu, test-fdisk-emu-bochs, test-fdisk-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-find-filter, test-find-filter-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-format-system-boot, test-format-system-boot-bochs | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-formatsys-emu, test-formatsys-emu-bochs, test-formatsys-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-formatsys-syswrite-emu, test-formatsys-syswrite-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-hsct-redir, test-hsct-redir-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-more-filter, test-more-filter-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-redir-full-emu, test-redir-full-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-shell | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-shell-dir-look, test-shell-dir-look-bochs, test-shell-dir-look-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-shell-files-emu, test-shell-files-emu-bochs, test-shell-files-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-shell-permanent, test-shell-permanent-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-sort-filter, test-sort-filter-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-xcopy-cwd-emu, test-xcopy-cwd-emu-bochs, test-xcopy-cwd-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-xcopy-tree-emu, test-xcopy-tree-emu-bochs, test-xcopy-tree-emu-mutant | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
| test-xcopy-tree-emu-cancel | SESSION | Type the normal DOS command sequence on a copied data volume; compare serial and mtools bytes/attributes/FAT before and after. |
