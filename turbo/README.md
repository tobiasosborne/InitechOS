# Turbo Initech

Two compilers with separate roles: `seed/` is the host C cross-compiler;
`compiler/` is the Pascal compiler that runs on InitechDOS. The seed emits
assembly, not a resident compiler by itself. Host NASM, ld and objcopy finish
that bootstrap. The Pascal compiler currently emits assembly too.

`make` builds both, including the resident compiler COM and the host FPC build.
`make test` composes the two tier-0 suites (6 seed entries + 38 compiler entries).
Only `turbo` is registered in the top-level COMPONENTS list. Each child also
has independent all/test/clean/smoke/mutants entrypoints and writes under
`build/turbo/<child>/`. FPC absence fails even with cached artifacts. NASM and
GNU binutils are also required. Shared settings come from mk/config.mk;
factory/testlib/run-entries.sh provides the shared status/summary convention
for shell and Pascal tests that do not fit host-test.mk's C-only link rules.
Factory code added by this import is make and POSIX shell.

Dependencies: seed uses only C host headers and its own headers; the resident
compiler uses its own source and the seed's public build artifacts/runtime.
The resident runtime calls INT 21h for files/exit, with the inherited serial
output helpers for diagnostics. No kernel private header is included. There
is no product change in this import beyond the separately committed rename.

The protocol goldens are hand-derived contracts, not captures minted by
running our implementation under FPC. The 17 execution goldens are also
hand-computed, independently checked by compiling/running each fixture with
FPC. Tier 0 does not execute either compiler's generated x86 code. Determinism,
assembly/link success and layout checks are labelled separately from execution
oracles. See compiler/tests/IMPORT.md and seed/tests/IMPORT.md for every legacy
gate, deferred emulator work and red controls. ROADMAP.md measures the remaining
work against GOLDEN-MASTER.md item 7. `selfhost` and `ddc` fail explicitly until
the real in-OS proofs exist. Smoke also fails explicitly pending the image.
