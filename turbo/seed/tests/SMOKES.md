# Seed emulator gates

The legacy gate mapping below records the behavioral sources. None runs in
tier 0. The mutation conversion now executes each affected original arith
fixture on the product DOS through factory/seed-guest.sh, with an independent
live FPC reference in Delphi mode (32-bit integer), stdout comparison and
mtools file comparison. All 17 patch faults are killed by make mutants.
Full Bochs seed-corpus parity remains future work; Turbo Initech's own native
compiler smoke already runs on QEMU and Bochs.
The legacy codegen and four shared-FPC legs booted bare Multiboot programs under
QEMU, not inside the DOS image. Port these as external product-image scenarios:
link the same body with rt/start_dos.asm + seed_dos.ld and launch its COM from
COMMAND.COM. Required environment: bootable InitechDOS with COM loader, serial
capture, file readback via mtools, injected commands and a bounded harness that
rejects timeout, triple fault, guest errors and missing/non-singular markers.
Default 486/8 MiB; no test scenario belongs in kernel source.

| Legacy gate | Program/input and expected-output source |
|---|---|
| test-seed-codegen | smoke.pas: `InitechOS seed OK`; 12 examples/arith files excluding fileio: precedence R=14, parens R=20, divmod D=3 M=2, negative N=-5, negative_divmod A=-3 B=-2 C=-3 D=2 E=3 F=-2; bool/control/char/func/array/record/string full TAG lines derived in each fixture header and legacy Makefile:16816-16859. Also carries a host-only branch-free gen_binop source check, imported in repro. |
| test-seed-codegen-mutant | precedence, imul -> add; R=14 must fail |
| test-seed-bool-mutant | bool, setl/setg swapped; full truth table must fail |
| test-seed-control-mutant | control, invert guards; full control TAG line must fail |
| test-seed-char-mutant | char, chr(i+1); full char TAG line must fail |
| test-seed-func-mutant | func, frame +4 and var parameter copied; full func line must fail separately |
| test-seed-array-mutant | array, 2-byte stride and missing lower-bound rebase; full array line must fail separately |
| test-seed-record-mutant | record, field +4 and record stride minus one field; full record line must fail separately |
| test-seed-string-mutant | string, one live temp and no length tiebreak; RNEST ghijkl and EQF FALSE (full string line) must fail separately |
| test-seed-fileio-os | FILEIO.COM from arith/fileio.pas, writable FAT scratch volume; exact FILEIO W1=1 WB=8 R1=1 RB=8 DATA=NORTHSTAR; B8FILE.BIN exactly NORTHSTAR (9 bytes), B8SINK.BIN/B8LOCAL.BIN empty; independent host FPC file_shared reference |
| test-seed-fileio-os-mutant | same DOS image/program with short-write and wrong-handle bodies separately; wrong stdout/file content must be rejected, no FILEIO-ERROR/fault-only false kill |
| test-seed-fpc-diff | examples/fpc/{func,array,record,string}_shared.pas; guest stdout equals independent live host FPC stdout and the exact hand line in tests/run.sh. file_shared was the host-only B8 root in this gate, with actual file guest leg above. |
| test-seed-repro-mutant | SEED_MUT_NONDET: two builds differ in .s/.o/.elf from process PID data; then execute bool with its unchanged full TAG golden to show only bytes changed. Host comparison and live control together make this an emulator gate. |

The files remain in examples/ and rt/. The old data images and harness are not
ported: one booted product image plus injected program/data files is required.
Do not call a successful bare-file link an INT 21h execution test.
