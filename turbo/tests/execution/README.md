# Generated-program differential execution

`make -C turbo smoke` builds the product image and runs `run.sh`. Every
`compiler/fixtures/gen*.pas` corpus row and every regression in this directory
runs on QEMU and Bochs through three compiler paths: direct C seed assembly
packaged by the independent NASM/link/mzlink route, the FPC-hosted Pascal
compiler's native executable, and native compilation by resident TURBO.COM.
Each complete stdout block is compared directly to an independent FPC run;
the original corpus also retains its authored-golden comparison. Files written
by each generated program are extracted with mtools and compared to FPC files.
Missing completion, compilation errors and CPU faults fail the run.

The seven original probes reproduce the six findings in `review-turbo.md`:
`record_var` (initech-08m74), `char_var` (initech-ichdv), `record_array`
(initech-6154u), `for_max`/`for_min` (initech-rnn2n), `for_bound`
(initech-4bjxw), and `record_index` (initech-uaz6c). The remaining four probes
compose those paths with forwarding/aliasing, live sentinels, negative array
bounds, whole-record copies through var formals, destination-index effects,
empty/recursive loops, and expression evaluation order. Their behavioral
source is `fpc -Mtp -B -O-`, run afresh, independent of either product compiler.
Five Phase 2 probes reproduce initech-jyaxt (`div_min`, `mod_min`),
initech-gnam4 (`length_live`), and initech-hpa1h (`record_field_string`,
`record_index_string`). The arithmetic probes store the quotient in Integer
where needed to avoid host expression widening; minimum Integer divided by
-1 wraps and its remainder is zero. Length must preserve a live local, and
both record access forms must select element 1. These use the corpus's
Delphi/B+/H- pins, with identical source given to FPC and both compilers.

Three follow-up probes cover initech-y8xxc: `nested_length` and
`record_char_concat` reproduce missed embedded string planning in computed
leftmost characters. `concat_left_contexts` also keeps enclosing accumulators
live in right-nested concatenations and on both sides of string comparisons,
checking a local sentinel and the prefix independently against FPC.

Only the original endpoint references change the Integer declaration to LongInt to
match the product's documented 32-bit width. The existing corpus retains its
Delphi/B+/H- directives; the original regressions use TP mode without a mode override.
This work does not resolve the separately tracked TP integer-width decision.

Hosted and resident Pascal outputs run in both extended and flat form;
the seed retains its extended assembly/link/mzlink leg. All 36 registered
programs fit the conventional arena, so a missing flat output fails rather
than skipping execution. Flat outputs must be nonempty raw COM files, not
renamed MZ containers. Each form has its own working directory and independent
stdout/file comparison. Default smoke performs 360 executions across
both emulators, preserving all prior 330 and adding 18 extended and 12 flat
computed-left-character comparisons.

`TURBO_EXEC_SOURCES`, `TURBO_EXEC_PATHS`
(`seed hosted hosted-flat resident resident-flat`),
`TURBO_EXEC_EMULATORS` (`qemu bochs`), `TURBO_HOSTED`, and `TURBO_EXEC_OUT`
select focused red-first or tier-2 mutation runs. Defaults always run the full
matrix; tier 0 never invokes the runner. `TURBO_EXEC_TIMEOUT_MS` bounds a
focused nontermination probe; the normal driver deadlines are unchanged.
Each batch stays below COMMAND.COM's 4096-byte input limit. Separate TEST
disks and all reference sources, stdout, compiler transcripts, serial logs,
CPU logs and extracted files remain under `build/turbo/smoke/execution`.

`hosted-flat` and `resident-flat` select native conventional-arena output.
The flat-runtime-digit patch changes decimal printing only in the flat
runtime. Its tier-2 `execution-flat` catcher runs both hosted forms on both
emulators: extended output stays correct while flat stdout differs from FPC.

Compiler patches `frame-offset` and `var-destination` select explicit tier-2
entries `execution-frame` and `execution-var`, which execute `gen_tiny` and
`gen_func_deep` on both emulators. They prove that wrong translations which
still assemble and encode consistently fail the independent execution oracle.
