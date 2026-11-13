# InitechBase

InitechBase is the dBASE III PLUS 1.1 work-alike: expression evaluation,
work areas, command programs, dot prompt, DBF tables, DBT memos and NDX indexes.
This import preserves the existing feature depth; it adds no language behaviour.

`core/` owns values, lexer, parser, evaluator, built-ins, runtime and text screen;
`cmd/` owns command dispatch and state; `fs/` owns the three file codecs.
`include/` exports their contracts. `pal/` supplies host, null and InitechDOS
bindings; `boot/` supplies startup, soft-float helpers and the linker script.
`spec/` preserves coercion, normalization, message and memory/screen evidence.
The verb/option `.def` files remain beside the command dispatchers.

All OS calls pass through the PAL vtable. It provides file handles, seek/read/
write/close, allocation and ownership/reset, console input/output and painting,
clock/environment/drive queries and child execution. The DOS binding issues
INT 21h with flat pointers and CF/error returns; neither the interpreter nor the
codecs include OS implementation headers. Host tests inject PAL clocks, input,
output and failures. The host binding is never linked into the target.

Run `make test` for tier 0, `make test THOROUGH=1` to add the live real-dBASE
DOSBox differential, or `make pending CASE=set-catalog` for a named unfinished
capture. Pending cases retain their real expected output and may fail; they are
not silently graded as passing. Set DBASE3_DECOMP to dbase3-decomp. Missing corpus
or Python 3 is an error; the normal runner disables historical opt-outs.
Existing independent Python readers are imported unchanged except for paths;
new factory wiring is make and POSIX shell. Every suite has a separate log and
fresh scratch tree under build/apps/initechbase/host. Case files replace the old
per-command/function/SET make target matrices. Shared host objects avoid repeated
compilation; `-j32` safely parallelizes compilation. Execution is sequential.

The oracles are captured real III+ transcripts and binary files, independent
Python DBF/DBT/NDX readers, the manual-derived numeric descriptor model, host
hardware double for soft-float, and authored safety/property assertions. The
provenance ledger is spec/ibase/README.md; test
coverage, measured counts and every legacy gate's disposition are in
`tests/IMPORT.md`. Real captures and their manifests stay together, including
pending evidence and emulator screen captures. No expected real-dBASE text is
rewritten. Two numeric-list captures were copied byte-for-byte from the existing
research directory because those files are outside the permitted move scope;
their original hashes remain in tests/list_off_real.sha256.

`make` builds every freestanding i386 translation unit and startup object under
build/apps/initechbase/target using the shared TARGET_CC/TARGET_CFLAGS. Target
additions preserve soft-float ABI and the shipped overlay/variable-store profile:
-msoft-float -mno-80387, the overlay define, FLOW_MAX_REGISTRY=1 and
FLOW_MEMVAR_ARENA=6000. Function/data sections are needed to put compiler-generated
overlay jump tables in their overlay sections. The default C ABI replaces the
old register/callee-pop convention coherently across all objects; the assembly
entry calls only a no-argument C function and INT 21h uses explicit registers.

`make package` attempts the real resident/overlay ELF, pack and MZ wrapper.
With default flags it correctly refuses the unchanged loader reserve: slot +
BSS + PAL tail is 65616 bytes, 80 above the 65536-byte declaration. All target
objects compile. No missing implementation prevents linking; resolving this
needs a loader reserve decision or approved size tuning. The import does not
increase a shared contract or restore the old size profile. The linker map is
retained under target/link.map on failure. Overlay literal/header/relocation
checks run in tier 0; `make pair-test EXE=/absolute/file.EXE` runs all 50
checks against a supplied pair. Grading the new pair waits for packaging.

The application overlay is separate from the kernel's graphical overlay.
It was introduced for space limits; real dBASE also uses overlays, but the
language semantics do not need them. Today's code still needs the file because
six entry tables, relocation/stamp checks, load-on-demand calls and lifetime
validation select its slot. Removing it entails resident dispatch, removing
slot lifetime assumptions and startup file validation, a new linker/packaging
path and host/boot regression comparisons. It is not part of this import.

## Temporary dependencies

Core, commands and codecs compile with only `-Iinclude`. The DOS PAL and factory
packaging tools additionally borrow attic/spec/memory_map.h and
attic/spec/initechmz_extended.h through a marked temporary include path. Those
contracts belong to the kernel/loader importer; no attic implementation is built.
The MZ tool claimed into factory/mz is shared; its build is currently owned here.
There are no SDK or Toolbox dependencies.

Local extensions to mk/host-test.mk register shell suites after C suites,
wrap each run with fresh scratch and independent postchecks, and cache product
objects by profile (normal, DML arena, lifetime guard). No shared make fragment
is edited. `make smoke` fails with a pointer to tests/SMOKES.md until the product
boots. The large existing mutation-hook set is inventoried in
`tests/MUTANTS-TODO.md`; patch conversion remains a follow-up.
