# Turbo Initech: remaining path to the golden master

The seed is host C; Turbo Initech is Pascal. The seed emits NASM assembly;
the Pascal compiler now emits both native formats as well as assembly.
The compiler source fits the language already: no additional language feature
is established as necessary for self-hosting. GOLDEN-MASTER.md item 7 remains
open; there is no IDE or DDC proof yet. Native equality is proved
for 17 flat and 18 extended images against NASM/binutils/mzlink. Runtime table
regeneration and all five native encoder mutants are green (compiler/NATIVE.md).

The resident compiler now uses extended InitechMZ placement. The independent
factory/mz reader checks the descriptor, sites and memory plan; the factory
links at zero and a second base to derive relocations independently. Both
seed-built and Pascal-generated compiler memory checks package their real
images and enforce the extended window/module limits. QEMU compiles and runs all 17 corpus programs from a separate TEST disk;
Bochs runs three cases. Stdout agrees with the authored FPC-checked goldens,
and file bytes are extracted with mtools. Native containers pass mzcheck.
Compilation is timed with DOS GET TIME, whose resolution is one second.

Self-host now passes through two fresh QEMU boots: K1(S) -> K2, K2(S) -> K3,
163843 bytes equal, SHA-256
`b37f96ad78cc49bc3e4334b0e6c2551228e3a8dd1dcf5f89a5726d128c3088e3`.
Guest compile times were 68s and 83s (1s resolution); both extended containers
pass mzcheck. The corruption comparator control fails at its modified byte.
Logs/images/hashes live in build/turbo/selfhost; scripts reproduce the proof.

Remaining milestones, in order:

1. DDC: independent host FPC(S) -> D1, in-OS D1(S) -> D2. Compare D2 with the
   seed path fixed point. Include contaminated-path and changed-source controls.
2. Minimal blue IDE/finale: edit, safe save, compile diagnostics, run and return,
   then a boot-to-self-reproduction session. All OS calls use INT 21h.

Capacity and semantics remain intentionally bounded: signed integers, booleans,
chars, ShortStrings, static arrays, named scalar records and untyped byte files.
No pointers/heap, reals, units, sets, objects or general Pascal compatibility
are claimed. Scheduling is cooperative; generated tools must return/yield.
Source is streamed repeatedly; table capacities and stack need measurement as
native state grows. Existing deferred gates are inventoried in tests/SMOKES.md.
