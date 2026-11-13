# Pascal mutation seams and patch sweep

Pascal has no conditional mutation macros. Nine comment anchors mark exact
source-edit seams. Seven are modest host-testable changes and are converted
to component-relative patch -p1 files using mk/mutants.mk. Each selected baseline
passes; every mutated compiler builds; all seven fail at test-run. The seed-built
link artifacts also build in each scratch directory. The runner never changes
the working source. Source comments remain explanatory, not executable hooks.

| Original source line | Anchor suffix (TURBO_ prefix after rename) | Patch / catcher |
|---|---|---|
| 72 | LEX_MUT_STRESC | quote-escape.patch: lex_basic byte 39 -> 33 |
| 797 | LEX_MUT_CASEFOLD | casefold.patch: lex_basic canonical identifiers |
| 1671 | TYPE_MUT_DUPOK | duplicate.patch: type_bad_duplicate error |
| 2216 | GEN_MUT_OFFBYONE | deferred: frame parameter +4; gen_tiny in DOS vs FPC |
| 2263 | GEN_MUT_VARPARAM | deferred: store callee slot instead of caller; gen_func_deep in DOS vs FPC |
| 3323 | PARSE_MUT_PRECEDENCE | precedence.patch: parse_rich grammar trace |
| 3760 | TYPE_MUT_ASSIGN | assignment.patch: type_bad_assignment error |
| 4016 | PARSE_MUT_ELSE | dangling-else.patch: parse_rich nearest-if trace |
| 4042 | GEN_MUT_LABELS | labels.patch: gen_tiny assembles baseline, duplicate labels fail NASM under mutation |

Legacy precedence/else mutants used two valid-input trace-shape comparisons
against the unmodified compiler. The imported patches use the independent rich
trace golden instead; they do not derive expected bytes from a second copy of
our implementation. OFFBYONE/VARPARAM formerly passed host changed-assembly
checks but needed separate guest wrong-value controls. They remain deferred,
not advertised as killed. Their exact old edits are in legacy Makefile:18386-18408
and their guest requirements are in SMOKES.md. No unimplemented IDE/selfhost/DDC
fault is invented here. Source anchors receive the product name in the rename
commit along with the patches; seven kills are rerun after that commit's edits.
