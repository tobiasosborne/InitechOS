# Deferred resident compiler emulator gates

Seven legacy gates. Rebuild against one booted product image with the seed-built
resident COM, fixed-name input, writable FAT, COMMAND.COM, INT 21h byte-file RTL,
serial capture, mcopy readback, QEMU and bounded completion/fault checks. The old
secondary tracer/data images are evidence about inputs, not a new image design.
A host FPC build alone does not replace the seed-built guest compiler.

| Legacy gate | Programs, image data, expected-output source |
|---|---|
| test-tps-lex-os | seed-built compiler + lex_basic.pas as fixed input; serial LEX bracket exactly fixtures/lex_basic.golden and live FPC-hosted compiler bracket |
| test-tps-parse-os | same compiler + parse_rich.pas; PARSE bracket exactly fixtures/parse_rich.golden and live FPC-hosted bracket |
| test-tps-type-os | same compiler + type_rich.pas; adjacent LEX/PARSE/TYPE brackets byte-equal to live FPC-hosted full triple bracket, with every boundary singular; TYPE independently fixtures/type_rich.golden |
| test-tps-gen-os | compiler + gen_tiny.pas; read generated assembly back from FAT, host NASM/ld produce TINY.COM; launch it and seed-built STINY.COM separately; both stdout equal fixtures/gen_tiny.golden and live FPC program stdout. Host assembly/link is still in this smoke; native emission is a future milestone. |
| test-tps-gen-os-mutant | host-FPC-built OFFBYONE compiler emits tiny assembly; assemble/link mutant MTINY.COM, execute inside DOS; shifted parameter must disagree with gen_tiny.golden (fault/timeout is not the intended wrong-value kill) |
| test-compiler-os | 17 generated corpus COMs named in fixtures/compiler_corpus.bat/CORPUS.md. Legacy generated with the FPC-hosted Pascal compiler, then executed them in DOS; separate gen-os checks the resident compiler's output. Capture singular CORPUS begin/end markers per row and compare each stdout with independently compiled live FPC plus gen_*.golden; require 17/17, differential_pass_rate=100%, and shell exit. |
| test-compiler-os-mutant | OFFBYONE tiny leg above and VARPARAM compiler emitting gen_func_deep -> MVARPAR.COM; same corpus oracle must reject wrong parameter-slot values (FACT5=120 line etc.), not merely detect changed assembly. |

Renamed guest names/markers and historical names are recorded in IMPORT.md.
The generated-execution and semantic-mutation rows are now implemented by
parent tests/execution/run.sh: the full corpus and 11 regressions run through
seed, hosted Pascal native and resident Pascal native paths on QEMU and Bochs.
frame-offset.patch and var-destination.patch are killed by explicit tier-2
execution entries; the completed MUTANTS-TODO migration record is removed.
The lex/parse/type protocol bracket journeys above remain pending. The parent
selfhost and ddc targets now prove K2==K3==D2 with independent negative controls;
they are new proofs, not imports of the historical failing stubs.
