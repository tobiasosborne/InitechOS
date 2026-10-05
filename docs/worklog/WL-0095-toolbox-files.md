# WL-0095 -- Toolbox file verbs (initech-tdnl.91, slice 1)

## Context and design

Disk applications had no document I/O. The design was stated before artifact
implementation in docs/design/toolbox-file-verbs.md: wrap the existing DOS
file dispatcher, retain a kernel-owned shadow JFT, and reclaim it in common
finish before image free. ADR-0013 section 3.4/AC-2 grounds crash independence;
PRD 4/6.1 and D1.2 ground layering. The new ABI and range/root/error policies
are authored, no local reference. spec/toolbox_gate.h and
spec/flair_file_traces.mk are deliberate Rule-8 additions, initech-tdnl.91.

## Changes and acceptance

Seven gate verbs, static module/BSS transfer spans, owned FILE handles,
signed checked seeks, DOS error encoding and C wrappers. Existing drawing,
registration and menu bounds retain their image-only meaning. Shadow PSP
cleanup survives a scribbled image PSP on the crash path.

Host oracle first failed with missing calls (588 checks, 534 failures).
Booted C fixture first failed FILE WRITE FAILED (unimplemented AX=0080).
Final host oracle: 594 checks, zero failures; bounds/owner/leak mutants RED
on F2/F3/F4. Emulator roundtrip plus mcopy/cmp, mdir deletion and fsck.fat
passed; WRITE_NOOP went RED on reread and independent disk bytes. Two launches
with fifteen files open each passed for Quit, close and crash. FILE_LEAK
went RED on second-launch DOS too-many-open (-260), desktop alive.

Existing QEMU families run: app-launch (+ six mutants), tenant-menu (+ three
mutants), ctenant (+ draw-not-presented mutant), all green. Logs in
build/fileverbs-final-emu.log. Bochs app-launch/tenant-menu legs deliberately
not run: operator says socket binds are forbidden in this sandbox.

Headroom: 117 -> 119 kernels; flagship 184776 -> 186952 loaded bytes (+2176),
largest 185448 -> 187656 (+2208); remaining 9656 / 8952 bytes, gate green.
An initial overlapping-build measurement failed with missing isr symbols;
the sequential rerun is green. No size policy or oracle was weakened.

The whole host vector initially stopped at test-dbf-read: its read/write open
of corpus TAX.DBF is denied by the sandbox. A byte-identical local copy of
all dbase3 goldens was made at build/dbase3-local (diff -qr verified zero
changes), and the entire vector rerun with DBASE3_DECOMP pointing there.
Final vector result is recorded below after completion.

## Handoff

No bd or push, per operator. Commits use build/lane-git metadata and are
transported in build/lane.bundle. Data images used by gates are local copies;
reference corpora are untouched. Writes are synchronous today; future deferred
DOS buffering requires explicit flush during handle reaping.

Final host vector: test-unit: ALL GREEN (378 host gates), with
INITECH_QMP_STDIO=1 make test-unit DBASE3_DECOMP=build/dbase3-local.
test-makefile-vars green. ASCII source and git diff --check passed.

Final review correction (folded into slice 1): CREATE could accept nul.txt as
a disk filename even though OPEN resolves its basename as the NUL device.
A new host case went RED on both the CREATE result and backend mutation;
the C tenant also checks the refusal before its roundtrip. The gate now asks
DOS's existing device-name resolver before CREATE, via a small public kernel
helper; no second path parser. FILE_OWNER also removes this file-type guard
and is RED on the new F3 cases. Host total: 596 checks, zero failures.
The spec Sec 10 wording deliberately records this pre-mutation refusal.
Original 46dd682 is amended in the final bundle; its replacement id is in the
final handoff. The complete host vector and emulator families were rerun after
this correction; final sizes/verdicts follow below.

Corrected final measurements: flagship 184776 -> 186792 bytes (+2016);
largest 185448 -> 187496 (+2048), headroom 9816 / 9112. Final headroom gate
measured 120 kernels (including slice 2's lifetime-only gate), green.
Host vector: test-unit: ALL GREEN (378 host gates). Tbx host: 596 checks,
zero failures; all three mutants RED for F2/F3/F4. File/launch/menu/C-tenant
emulator families and mutants all green after the correction. DOS regressions
fatwrite, exit-handles (+ leak mutant), readerr-winh (+ no-CF mutant) also
passed. Final logs: build/fileverbs-all-final-{unit,emu}.log,
build/fileverbs-dos-emu.log, build/fileverbs-final-quality.log. The local dbase
golden copy still compares byte-identical to the corpus after the full vector.
