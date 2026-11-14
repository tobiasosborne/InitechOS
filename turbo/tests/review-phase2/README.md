# Phase 2 compiler review probes

These are independent Free Pascal differential inputs from the 2026-11-14
quality review. The five minimal defect reproducers now live in
`../execution/` and run in default smoke; the remaining review controls stay
here. No expected output is computed by either compiler under review.

Use the existing product-image execution harness from the repository root:

```sh
nice -n 19 make -j2 image
nice -n 19 make -j2 -C turbo all
nice -n 19 make -C turbo "$PWD/build/turbo/smoke/TIMER.COM"
TURBO_EXEC_SOURCES=turbo/tests/execution/length_live.pas \
TURBO_EXEC_PATHS='seed hosted hosted-flat resident resident-flat' \
TURBO_EXEC_EMULATORS='qemu bochs' \
TURBO_EXEC_OUT=build/turbo/review-replay \
nice -n 19 sh turbo/tests/execution/run.sh
```

The runner compiles the FPC reference with `-Mtp -B -O-`. Each source uses the
same Delphi/B+/H- directives as `compiler/CORPUS.md`: 32-bit Integer, complete
Boolean evaluation, ShortString. Integer overflow probes assign each result
back to Integer before printing, because host FPC widens direct arithmetic
expressions even in this mode. This preserves the operator's 32-bit ruling.

Repaired defects (all P1; inputs in `../execution/`):

- `mod_min`, `div_min`: initech-jyaxt, both compilers raised a divide exception
  and halted the OS for minimum Integer and divisor -1. Modulo has an exactly
  representable result of zero. To replay an unfixed revision, isolate each path; the
  first fault prevents later cases from executing. An external timeout may be
  needed to retire QEMU after HALTED; retain serial output before stopping it.
- `length_live`: initech-gnam4, seed silently overwrote a local when Length
  consumed a concatenation. `length_concat` (still here) is the shorter
  no-local variant; it printed the right lengths but corrupted the frame.
- `record_index_string`, `record_field_string`: initech-hpa1h; see the Phase 2 review report
  for the issue ID. Seed omitted string temporary planning in whole-record copy
  designators and direct record-array field reads, selecting element 0 instead
  of element 1. These indices have no side effects.

Passing probes cover ordinary signed div/mod and stored overflow wrap
(`integer_edges`), empty and maximum-length strings/truncation/high-byte
comparison (`string_edges`, `string_relations`), computed string and scalar-array
indices (`string_index`), scalar/record/string character var arguments and
function returns (`char_return`), 150 recursive calls and var aliases
(`recursion_alias`), and zero-byte/partial/EOF untyped-file operations
(`file_edges`). The file reference specifies record size 1 explicitly, matching
the documented product contract. The file runner also extracts EDGE.BIN and
compares it to FPC's file.

The full report, inline reproducers, stdout, compiler transcripts and emulator
traces are in `phase2/review-turbo.md` and `phase2/review-turbo-evidence/` under
the orchestrator's report directory. The reviewer made no compiler fixes.
