# Turbo Initech seed compiler

The host C bootstrap compiler: lexer, recursive-descent parser, AST, semantic
checker and NASM x86 emitter, with separate bare Multiboot and InitechDOS
startup/link targets and a thin INT 21h file runtime. It bootstraps the resident
Pascal compiler; it is not Turbo Initech itself. Dependencies: its own headers
and host C/POSIX headers only. Runtime dependency: InitechDOS's flat INT 21h ABI.

`make` builds initechc and the three runtime objects. `make test` runs six host
entries, with 112 lexer checks (15 for the `$I` switch) and 115 parser/type
checks, 16-fixture reproducibility, file RTL static contracts and independent FPC reference runs.
Default OUT is build/turbo/seed at the repository root. FPC must be installed;
there is no skip option. Test C files alone include factory/testlib/test_assert.h,
which is the existing shared seed-origin harness. Product builds use only -I.
Host objects are shared between the driver and unit programs for build speed.

Behavioural sources remain the source headers, fixture derivations and the
legacy-final recipes recorded in tests/IMPORT.md. No optimiser/ABI tuning from
the old monolithic build is needed. The factory uses shared HOST_CFLAGS;
runtime assembly/link semantics are preserved. This import changes no behaviour.
`make mutants` kills all 20 patches through the shared runner: entries run
under `../tests/entries.sh`, which writes the host-test result protocol: lexer/parser host checks, reproducibility,
and guest execution with independent Free Pascal stdout/file comparisons. Guest
entries are selected only by tier 2; the default six tier-0 entries never boot
an emulator. Build `make image` first for the guest mutation catchers. The two
file-I/O faults live only in runtime patches; no wrong entry point ships.
Records now ascend uniformly in local/global/var storage; record copies
evaluate each index once. Char reads zero-extend bytes and var-char writes
preserve adjacent bytes, including string-element actuals. For loops delay
counter initialization until both bounds are saved and stop before stepping
past their endpoint. Parent tests/execution compares all corpus/regression
programs to FPC on QEMU and Bochs. Runtime tables still regenerate in tier 0.
`make smoke` remains a parent Turbo Initech/product responsibility.

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: dos
Interface callback: pas_main
<!-- end-interface-declarations -->

The strict runner rejects the old record-field mutant's pointer-fixture
hang as an infrastructure error. Its `record_slot` guest instead writes a
one-field integer record beside an integer sentinel: shifting the field by
four bytes changes the sentinel's printed value, without a pointer walk.
Free Pascal independently supplies the stdout oracle (37,111,222). The
mutation must reach the completion marker and fail the stdout assertion.
