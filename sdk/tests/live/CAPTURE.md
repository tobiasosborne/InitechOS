# Live gate conformance capture

The 516-byte gate-results.bin contains 129 little-endian signed EAX results,
ordered by tests/live/cases.h (including its handle-capacity loop). It was read
with mtools from SDKRES.BIN, written by SDKCHECK.EXE on the booted, ordinary
product image at source base 09d52a32. The fake does not produce this file.

Reproduce from the repository root with `make -C sdk smoke`. The SDK smoke
build adds -g to the normal target flags solely for the host debugger's symbol
metadata. The executable tenant and fixture files are copied onto a copy of
the ordinary data image. The boot disk and kernel have no test scenario.
smoke-conformance requires the newly read results to match this capture and
then compares every result from the independently compiled recording fake.

The original fake mismatched 92 of the first 113 scalar/lifecycle cases.
The expanded capture also grades module/BSS/stack pointer classes, empty spans
and an unmounted drive. Expected outcomes are observations of the real gate,
including its error precedence, rather than inferred historical conventions.
The validation sources are flair/tbxgate.c (registration, windows, drawing,
menus, dialogs, modal state, tracking, scroll), flair/tbxfile.h (file spans and
access), flair/cp_service.h (services), and kernel/int21.c (DOS backend).

The review follow-up adds cases 61/62 (zero-based): the same valid dialog
event with what=3 and what=0x10003. Both real-gate EAX results are zero:
v_dialog narrows what to uint16_t before checking the EventRecord kind.
Removing those two dwords reproduces the original 508-byte capture exactly.
The post-exit tenant assertions also request EXIT(7), then EXIT(0), before
returning. The live serial records both calls returning zero and the final
TENANT-EXIT status zero, proving replacement of the pending exit status.
These returns cannot be saved through the already-closed file gate; incorrect
returns crash the test tenant and an incorrect final status fails smoke.
The host conformance check separately grades both returns and exit_rc=0.
Before the follow-up fix, the fake returns NOTREG on the second EXIT and
BADARG for dialog what=0x10003 (129 calls, two mismatches).

Capture hashes (SHA-256):

```
2e0568d0fb68e153e6607d501cc5d3cf9d9d0533f9575a5c8c73025c98029471  gate-results.bin
a5a6468ea6f99d44f7a930161fb082652d70ebd1217d1d93b8980b37bea74a96  flair/tbxgate.c
c92b667f81cb9751daf5ef5cb7680e1397b9f582bbae93702871e2e43bcda7f6  flair/tbxfile.h
dee3b06eeb8ea6b01b9db3c298551ddf8ef13f77d7ef1c6ca10651a98599fb1b  flair/cp_service.h
cc7bcf10637632c039808cc418f3db6b9a13398994ed0f95226f1fd710daf5c8  flair/desktop/tbx_gate.asm
26ad97bb51ed7b5ca401d83efd849286a69172c173de59e58354254fb4b78b70  kernel/int21.c
8a62c2fd2698b96ee4bbcdc96cd9ac4f4a88c1f9f4f75d8a3ed5c442e93b2c5c  INITDOS.SYS
```

abi.tsv is a separately authored literal argument table, not a capture or a
runtime-source parse. inventory.sh checks its coverage against traps.tsv.
abi-smoke.sh observes the actual dispatcher's decoded a[], using the normal
QEMU GDB interface and matching system.elf. Register/evbuf/proc addresses are
resolved independently from the disk tenant's ELF and the live loader base.
The upstream EventRecord and persisted flat callback are compared in all six
fields; the tenant checks all other sentinel return values before its final
exit, whose return is observed before the code allocation is freed.

`sh sdk/tests/live/red.sh` applies the TEXTDRAW x/y mutant to an isolated
runtime, verifies that the old structure/arity audit still passes, then
requires the live observer to fail at row 5. The separate fake mutants allow
an illegal go-away value or bypass pointer spans; both must fail conformance.
No mutation is installed in the source tree or normal product image.
