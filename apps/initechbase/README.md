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
build/apps/initechbase/target using the shared TARGET_CC/TARGET_CFLAGS, plus the
packaging tools, generated linker contract and MZ arguments. Target
additions preserve soft-float ABI and the shipped overlay/variable-store profile:
-msoft-float -mno-80387, the overlay define, FLOW_MAX_REGISTRY=1 and
FLOW_MEMVAR_ARENA=6000. Function/data sections are needed to put compiler-generated
overlay jump tables in their overlay sections. The default C ABI replaces the
old register/callee-pop convention coherently across all objects; the assembly
entry calls only a no-argument C function and INT 21h uses explicit registers.

`make package` attempts the real resident/overlay ELF, pack and MZ wrapper.
It fails at the reserve assertion described under Known blockers. All 23 target
objects and the packaging tools build; no valid ELF or EXE/OVL pair is claimed.
The linker map is retained under target/link.map on failure. No size-squeezing
flags or reserve increases are introduced. Overlay literal/header/relocation
checks run in tier 0; `make pair-test EXE=/absolute/file.EXE` runs all 50
checks against a supplied pair. Grading the new pair waits for packaging.

The application overlay is separate from the kernel's graphical overlay.
It was introduced for space limits; real dBASE also uses overlays, but the
language semantics do not need them. Today's code still needs the file because
six entry tables, relocation/stamp checks, load-on-demand calls and lifetime
validation select its slot. Removing it entails resident dispatch, removing
slot lifetime assumptions and startup file validation, a new linker/packaging
path and host/boot regression comparisons. It is not part of this import.

## Dependencies

Core, commands and codecs depend on nothing outside this component and compile
with only `-Iinclude`. The DOS platform layer depends on kernel/include for the
loader's memory contract (memory_map.h and initechmz_extended.h). The host
linker-contract generator uses those same public headers. No build reads attic.
Packaging uses the shared factory/mz tool, built by that component's Makefile;
its relocation I/O tests also live and run there. App policy remains in
include/ibase/memory_contract.h: the generator supplies explicit --extended,
--reserve, --minimum-heap and --minalloc arguments to the generic mzlink.
There are no SDK or Toolbox dependencies; this is an INT 21h DOS application.

## Known blockers

The current plain-flags link needs 65616 bytes after the 107744-byte resident
module: a 13968-byte overlay slot, 16 bytes of BSS alignment, 47680 bytes of BSS,
and a 3952-byte PAL heap region. These are intentional runtime allocations,
not misplaced file sections. IBASE_LOADER_RESERVE is 65536u in
include/ibase/memory_contract.h, leaving an 80-byte shortfall. The host generator
tools/ibase_memory_ld.c exports it as IBASE_DECLARED_RESERVE. boot/ibase.ld
enforces IBASE_REQUIRED_RESERVE <= __ibase_link_reserve at build time; package
prints the required and declared byte counts and fails before producing a pair.

This reserve protects the overlay/BSS/PAL tail from the native AH=48h heap
above it at runtime. It is not the unrelated 64 KiB LOAD_STAGING_MAX buffer or
PROGRAM_BSS_RESERVE in kernel/include/memory_map.h, nor the desktop tenant's
TBX_TENANT_IMAGE_MAX in kernel/loader.c. Ground truth: kernel/mz.c reads the
extended descriptor's reserve_bytes; kernel/loader_validation.c:loader_high_plan
places arena_base/reserve_end at ALIGN(EXT_APP_BASE + module length + declared
reserve, 16), then checks the guard and minimum heap. It cannot infer the app's
BSS extent. With 65536 declared here, the last 80 bytes would overlap that heap.

No kernel-side change is needed to lift this blocker: the extended loader
already accepts a larger descriptor reserve within the high window. A future
app-owned change must set IBASE_LOADER_RESERVE to at least 65616, regenerate
both linker and MZ contracts, and validate the resulting pair and booted DOS
execution. No constant is raised in this integration and kernel/ is untouched.
Full pair checks, deferred boot smokes and the mutation conversion remain owned
by InitechBase; see tests/IMPORT.md, tests/SMOKES.md and tests/MUTANTS-TODO.md.

Local extensions to mk/host-test.mk register shell suites after C suites,
wrap each run with fresh scratch and independent postchecks, and cache product
objects by profile (normal, DML arena, lifetime guard). Shared fragments supply command tracking, dependencies and test bounds. `make smoke` fails with a pointer to tests/SMOKES.md until the product
boots. The large existing mutation-hook set is inventoried in
`tests/MUTANTS-TODO.md`; patch conversion remains a follow-up.

Shared build conventions: command stamps rebuild on tool/flag changes; C
translation units use -MMD dependencies. TEST_BUDGET_S is 30 seconds for the
complete tier-0 selection; measured execution fits the default, excluding builds.
Standalone tests verify their consumed files in goldens/CORPORA.lock; the
top-level test run verifies the full lock once before parallel execution.
All three host object profiles and target objects retain their flags.
Product mutation hooks and the blocked INITBASE.EXE/OVL all outputs keep the
import audit disabled. Shell suites retain the shared timeout/count runner.
