# Toolbox file verbs (initech-tdnl.91, slice 1)

The INT 81h gate wraps MILTON's existing INT 21h dispatcher with a synthetic
frame. PRD sections 4 and 6.1 place the Toolbox over DOS; ADR-0013 section 3.4
and AC-2 require crash teardown independent of damaged application data.
Reusing DOS preserves the already judged positioned FAT writes, path resolver,
access modes and SFT accounting. There is no second filesystem implementation.

The slot owns a kernel-resident shadow PSP/JFT, initialized without inherited
files. Only its live FILE handles (5..19) are callable. Each DOS call saves,
binds and restores PSP and CWD, resolving filenames from the data-volume root.
Common finish reclaims all handles before the code block, covering Quit,
close-box termination and crash (kill deliberately skips the close hook).
Writes already commit synchronously; fileio_fat.c's close hook is currently a
no-op. A future deferred writer must extend this cleanup to flush explicitly.

File spans accept module and zero-filled BSS, excluding PSP; other gate pointers
retain their image-only checks. Empty spans touch no memory. Paths must have a
NUL within 128 bytes inside the allocation. OPEN modes and read/write access are
checked before DOS. Seek uses signed deltas but requires the resulting position
in 0..INT32_MAX, so successes cannot alias negative gate errors. DOS errors are
encoded as -(256 + DOS error), distinct from existing gate errors.

CREATE also refuses DOS device basenames using the existing DOS OPEN resolver
(case/drive/path/extension normalization), before touching FAT. Otherwise it
could create NUL.WK1 on disk while OPEN always resolves that name as a device.
This is the authored file-only policy, grounded in int21.c dev_open_lookup's
DOS 3.3 Technical Reference Ch. 4 device precedence.

The numbers, shadow ownership, root default, file span policy, error encoding
and seek ceiling are authored, no local reference. Architecture is grounded in
the references above and GUI-remediation-D1-D2-D3-design.md D1.2/D1.5/D1.6,
as corrected by GUI-remediation-ADR-reconciliation.md DEC-AC3-1..4.

Acceptance: host validation/ownership oracle over real DOS with a mock backend
and named mutants; booted Finder launches a C fixture that writes, seeks,
reopens, reads and deletes; mtools judges surviving bytes and deleted names,
fsck.fat judges the disk. Repeated quit/close/crash with open handles must leave
no live owned SFT entries, including after tenant PSP corruption. Kernel growth
is measured before/after; stop if it requires more than 4096 bytes.

## Slice 2: RENAME and the atomic save (initech-mcep)

Audits 2026-10-23 (iword L001/L002, das L003/L008, i123 L001) showed a save
that ran out of disk left a zero-byte file where the last saved version had
been: CREATE is AH=3Ch and truncates before the first WRITE. The fix is one
tenant-runtime primitive, `tbx_file_save` (os/apps/tbx/tbx_save.h), over one
new gate verb, TBX_FILE_RENAME 0x0089 (spec/toolbox_gate.h Sec 10b, AH=56h
with the new name in EDI). The save writes NAME.$$$ in the same directory,
closes it, deletes NAME, then renames NAME.$$$ to NAME; every failure before
the delete removes the temporary and leaves NAME untouched. DOS 3.3 has no
rename-over-existing (Technical Reference p. 6-206), so delete precedes
rename: a reset exactly between the two leaves the document as NAME.$$$ (the
EDLIN precedent, MS-DOS 3.3 User's Reference p. 164). CREATE keeps its
truncate meaning. Acceptance: test-flair-fullsave (+ -mutant, the pre-fix
in-place save on the same full volume) and test-tbx-file F5.
