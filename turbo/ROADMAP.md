# Turbo Initech: remaining path to the golden master

The seed is host C; Turbo Initech is Pascal. The seed emits NASM assembly;
the Pascal compiler now emits both native formats as well as assembly.
The compiler source fits the language already: no additional language feature
is established as necessary for self-hosting. GOLDEN-MASTER.md item 7 remains
open; there is no IDE or self-host certificate yet. Native equality is proved
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

Remaining milestones, in order:

1. Self-host: K1 is host seed-built. In the OS K1(S) -> K2, K2(S) -> K3.
   Extract with mtools, compare whole containers byte for byte, print SHA-256.
   K1==K2 is not the certificate. Keep transcripts under build/.
2. DDC: independent host FPC(S) -> D1, in-OS D1(S) -> D2. Compare D2 with the
   seed path fixed point. Include contaminated-path and changed-source controls.
3. Minimal blue IDE/finale: edit, safe save, compile diagnostics, run and return,
   then a boot-to-self-reproduction session. All OS calls use INT 21h.

Capacity and semantics remain intentionally bounded: signed integers, booleans,
chars, ShortStrings, static arrays, named scalar records and untyped byte files.
No pointers/heap, reals, units, sets, objects or general Pascal compatibility
are claimed. Scheduling is cooperative; generated tools must return/yield.
Source is streamed repeatedly; table capacities and stack need measurement as
native state grows. Existing deferred gates are inventoried in tests/SMOKES.md.
