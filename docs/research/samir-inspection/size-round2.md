# SAMIR inspection size round 2 (initech-f99h)

The PM's committed inspection candidate (`30ea1148`) loaded 100,385 bytes.
The unchanged ADR-0009 reclaim cap is 98,175 bytes; the round additionally
requires at least 2,600 bytes recovered without changing behavior.

The existing `test-samir-size` was RED before editing:

```
RED SAMIR_SIZE: 100385 B > 98175 B reclaim cap / 102256 B headroom cap
make: *** [harness/diff/dbf_diff/samir_size.mk:8: test-samir-size] Error 1
```

## Byte accounting

Each row is a forced `make -B samir-com`, measured from the preceding row.
The compiler remains GCC 13.3.0; every production/mutant TU and LTO link uses
one `SAMIR_SIZE_CFLAGS` policy. No linker script or budget changed.

| Change | SAMIR.COM bytes | Recovered bytes |
| --- | ---: | ---: |
| Committed candidate | 100385 | 0 |
| Shared built-in arity/type validation, private handler signatures | 99841 | 544 |
| Shared literal state transitions in seven scanners | 99297 | 544 |
| Shared bounded comma-expression copy; inline-comment scanner uses literal helper | 98977 | 320 |
| Shared date picture renderer for DTOC and query output | 98185 | 792 |
| Delay early inlining; permit jump tables; declared-inline ceiling 175 | 97673 | 512 |
| Total recovered | | 2712 |

Final cap margin: 502 bytes. Final ELF `.text` is 90,659 bytes (was 94,714),
`.rodata` 6,985 (was 5,665), and loaded alignment 29 (was 6): 4,055 fewer
text bytes, 1,320 additional data bytes, 23 additional alignment bytes.
BSS remains 46,772 bytes. The secondary kernel remains 196,608 bytes padded;
no kernel sources or policy changed. `-march=i386` and soft-float remain
pinned. Disassembly has no x87, cmov, bswap, xadd, cmpxchg or endbr instructions.

## Behavior preservation

The existing real-III+ DML, function, control, USE, REPORT and program goldens
passed on the committed candidate before editing. They remain the semantic
oracles; no fixture, SHA pin, tolerance, locked spec or command page changed.

The shared literal helper consumes opening and closing delimiters as literal
bytes. Callers retain their original whitespace, nesting, scope and error
rules. The shared expression copier retains the 254-byte bounds, trailing
blank trimming and commas inside strings/function arguments.

The function descriptor table moves identical count/type preconditions to
one dispatch path. C/M acceptance, strict-C exceptions, MAX/MIN's paired
check and RECSIZE/ERROR/MESSAGE's special paths remain distinct. In
particular MESSAGE keeps its allocation-before-arity-check behavior.

The date renderer retains both renderers' original fallback and leading-digit
rules, as well as blank digits, separators and century width. The existing
SETFMT and blank-DTOC mutation branches still affect their original callers.

An additional scratch differential links the committed built-in/query files
under renamed symbols beside the final engine. It compares return/error
codes, values, numeric presentation, string bytes, scratch consumption and
bounded date output. Result: 462,672 calls (51 names, all combinations of
four argument tags, arities -1..5) and 79,872 date pictures (all 256 format
bytes, century 0/1/255, blank/fractional/normal/large dates, capacities 0..12):
zero differences. This supplements the independent III+ captures; it does
not replace them. Sources, command artifacts and log: `build/size-round2/`.

## Verification

All 202 registered SAMIR host roots and their dependencies passed serially;
the two unregistered control roots (`test-samir-ctl` and `-mutant`) also passed
(all six control mutants RED).
The exact root list is `build/size-round2/host-roots.txt`; the invocation uses
`build/size-round2/host-roots.mk` and the absolute DBASE3_DECOMP reference.
This includes DBF/DBT/NDX, lexer/parser/evaluator/coercion, functions,
interpreter, query/REPL, DML, reports, screen/input, safety/lifetime, soft-float,
messages, their registered mutants, and the size/reproducibility gates.
Size mutant: the exact 98,175-byte boundary passes; the 4,096-byte pad fails
at 101,769 bytes. Two forced clean builds are byte-identical:

```
a23fbab91649f6f00d7959b477063188a0261c9345a071737591c50d353cd126  build/SAMIR.COM
```

All 113 QEMU gate targets and dependencies in the full SAMIR emulator vector
passed, including the 20 DML and 13 function cases, all their mutants, core,
control, REPORT, list layout, boot, persisted writes, canonical replays,
input/screen, lifetime/safety, memory widths and messages. The saved pre-round
100,385-byte COM was then replayed against all 20 DML and 13 function guest
goldens: every byte matches the same locked III+ captures before and after.
The baseline SHA256 is bb0907555ad43aa65e763ad79c99d0d17cd2799278911dc1936b9e529c075e31.

The full emulator aggregate returns FAIL solely for Bochs infrastructure:
24 named Bochs targets fail, and their dependent boot mutant cannot run.
An isolated `test-samir-boot-bochs` retry reproduces the same failure before
any guest serial output or framebuffer capture:

```
bochs_run: RFB socket: Operation not permitted
[bochs] launched=1 rfb_unblocked=0 timed_out=0 exit=1 signal=0
[bochs] serial_len=0 marker_found=0 (expect=SHELL-READY)
make: *** [harness/samir_boot_bochs.mk:37: test-samir-boot-bochs] Error 1
```

No SKIP_BOCHS opt-out or oracle change was used. Follow-up for the PM: run
Bochs acceptance on a host that permits the RFB socket. This is unverified
execution, not a guest behavior mismatch. Prior inspection feature limitations
remain as recorded in the round-1 report; no additional feature work was done.

Logs: `build/size-round2/{baseline-size,goldens-before,host-final,control-final,emu-final,emu-before,bochs-alone,parity}.log`.
The ephemeral aggregate selects the registered TEST_UNIT_GATES by the SAMIR,
interpreter, xBase, DBF/DBT/NDX and dBASE prefixes, plus `test-samir` and
`test-dbase`; the emulator aggregate selects every `test-samir-*` member of
TEST_EMU_GATES plus the control and Bochs boot pairs. Both use `make -j1` and
the absolute DBASE3_DECOMP path. Root lists and the make fragment are in
`build/size-round2/{host-roots,emu-roots}.txt` and `host-roots.mk`.
No full `make test`, tracker writes, commits or pushes; the PM owns merging.
