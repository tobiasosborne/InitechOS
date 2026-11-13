# Turbo Initech: remaining path to the golden master

The seed is host C; Turbo Initech is Pascal. Both still emit NASM assembly.
The compiler source fits the language already: no additional language feature
is established as necessary for self-hosting. GOLDEN-MASTER.md item 7 remains
open; there is no IDE or native executable emission yet.

The resident compiler now uses extended InitechMZ placement. The independent
factory/mz reader checks the descriptor, sites and memory plan; the factory
links at zero and a second base to derive relocations independently. Both
seed-built and Pascal-generated compiler memory checks package their real
images and enforce the extended window/module limits. QEMU and Bochs run
TURBO.COM from a separate TEST disk and return assembly identical to the host
Pascal compiler. Those runs do not prove the generated code executes.

Remaining milestones, in order:

1. Native emission: adapt fa5cbb08 to produce conventional flat programs and
   extended containers, recording every absolute address. Compare every corpus
   program and turbo.pas with NASM/ld/objcopy plus independently derived mzlink
   relocations. Enforce real placement limits, regenerate any runtime byte table
   against its assembly source in tier 0, and kill encoder/fixup/layout mutants.
2. In-OS corpus: QEMU compiles all 17 programs directly to executables and runs
   them against the FPC-checked, hand-authored stdout goldens. Bochs runs a
   subset. Extract file output independently and report compile times.
3. Self-host: K1 is host seed-built. In the OS K1(S) -> K2, K2(S) -> K3.
   Extract with mtools, compare whole containers byte for byte, print SHA-256.
   K1==K2 is not the certificate. Keep transcripts under build/.
4. DDC: independent host FPC(S) -> D1, in-OS D1(S) -> D2. Compare D2 with the
   seed path fixed point. Include contaminated-path and changed-source controls.
5. Minimal blue IDE/finale: edit, safe save, compile diagnostics, run and return,
   then a boot-to-self-reproduction session. All OS calls use INT 21h.

Capacity and semantics remain intentionally bounded: signed integers, booleans,
chars, ShortStrings, static arrays, named scalar records and untyped byte files.
No pointers/heap, reals, units, sets, objects or general Pascal compatibility
are claimed. Scheduling is cooperative; generated tools must return/yield.
Source is streamed repeatedly; table capacities and stack need measurement as
native state grows. Existing deferred gates are inventoried in tests/SMOKES.md.
