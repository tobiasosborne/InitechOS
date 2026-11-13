# Turbo Initech seed compiler

The host C bootstrap compiler: lexer, recursive-descent parser, AST, semantic
checker and NASM x86 emitter, with separate bare Multiboot and InitechDOS
startup/link targets and a thin INT 21h file runtime. It bootstraps the resident
Pascal compiler; it is not Turbo Initech itself. Dependencies: its own headers
and host C/POSIX headers only. Runtime dependency: InitechDOS's flat INT 21h ABI.

`make` builds initechc and the three runtime objects. `make test` runs six host
entries, with the unchanged 97 lexer and 115 parser/type checks, 15-fixture
reproducibility, file RTL static contracts and independent FPC reference runs.
Default OUT is build/turbo/seed at the repository root. FPC must be installed;
there is no skip option. Test C files alone include factory/testlib/test_assert.h,
which is the existing shared seed-origin harness. Product builds use only -I.
Host objects are shared between the driver and unit programs for build speed.

Behavioural sources remain the source headers, fixture derivations and the
legacy-final recipes recorded in tests/IMPORT.md. No optimiser/ABI tuning from
the old monolithic build is needed. The factory uses shared HOST_CFLAGS;
runtime assembly/link semantics are preserved. This import changes no behaviour.
`make smoke` fails until the product image is available. `make mutants` reports
that patch conversion remains pending; see tests/MUTANTS-TODO.md (17 hooks).
