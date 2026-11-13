# apps-i123

Separate pure worksheet/formula/format/range/recalc/WK1/menu core from Face A event/drawing glue; keep Lotus real captures and independent readers. CALC is another consumer of part of this core.

Evidence revision: `c5d18538049986bbbb27018c351405acf814ebbc` (`legacy-final`). Physical LOC includes comments and blank lines; tests and data are separate from production counts. Source ownership recommendations are explicitly inferences; paths, includes, guard counts and dry-run command text are measured.

## 1. Sources

12 production/source-support files; 7,802 physical lines. Test programs are in section 4.

|Legacy path|Lines|Role / disposition|
|---|---:|---|
|`os/i123/app/face_a.c:1`|1412|- 123.EXE, Initech 123's Face A: the classic Lotus|
|`os/i123/core/eval.c:1`|1554|- the formula EVALUATOR and the @function library|
|`os/i123/core/fmt.c:1`|816|- how a cell LOOKS: the 1-2-3 R2.2 display formats, the|
|`os/i123/core/formula.c:1`|806|- the formula COMPILER (typed text -> the .WK1|
|`os/i123/core/i123.h:1`|312|- Initech 123 core: the worksheet model, the .WK1|
|`os/i123/core/menu.c:1`|80|spec/i123/menu_ri_offsets.tsv. Separate resource storage follows the real|
|`os/i123/core/menu.h:1`|17|initech-uqah: R2.2 menu inventory, spec/i123/menu_ri_offsets.tsv. */|
|`os/i123/core/menu_data.h:1`|1119|R2.2 123.RI menu strings; cited offsets: spec/i123/menu_ri_offsets.tsv. */|
|`os/i123/core/range.c:1`|308|(PDF 69..73), /Move pp. 7-1..4 (PDF 189..192), /Range Name and Trans.|
|`os/i123/core/recalc.c:1`|404|- RECALCULATION: natural, columnwise and rowwise|
|`os/i123/core/sheet.c:1`|519|- the Initech 123 worksheet model (THE ARTIFACT).|
|`os/i123/fs/wk1.c:1`|455|- the .WK1 reader and writer (THE ARTIFACT).|

### Contracts, fixture/data and ancillary files owned here

Runtime files in the requested `os/`, `seed/`, `spec/`, `fixtures/` trees are assigned; os/README.md and os/apps/.gitkeep remain explicit unassigned placeholders in SUMMARY. `spec/*.mk` input traces are test data to preserve separately from their obsolete build syntax.

|File|Lines (text)|Role|
|---|---:|---|
|`spec/flair_i123_copy_traces.mk:1`|9|Deliberately added B11 replay. Independent real R2.2 screens/WK1 files:|
|`spec/flair_i123_entry_traces.mk:1`|27|LOCKED B13 / initech-twxf entry trace. Deliberately added, no golden mint.|
|`spec/flair_i123_file_traces.mk:1`|21|initech-tdnl.91 slice 2: deliberate locked keystroke traces.|
|`spec/flair_i123_hit_traces.mk:1`|36|LOCKED B31 / initech-umfi cell hit-testing trace (Rule 8). Deliberately added.|
|`spec/flair_i123_keys_traces.mk:1`|17|LOCKED B11 / initech-69mz keyboard trace (Rule 8). Deliberately added.|
|`spec/flair_i123_menu_traces.mk:1`|23|LOCKED initech-uqah P3 traces. Same launch geometry and QMP injection as|
|`spec/flair_i123_quit_traces.mk:1`|54|- the LOCKED input traces of|
|`spec/flair_i123_resize_traces.mk:1`|39|LOCKED B12 / initech-89rc resized-worksheet trace (Rule 8). Deliberately added.|
|`spec/flair_i123_traces.mk:1`|56|- the LOCKED input traces of test-flair-i123|
|`spec/flair_i123_wide_traces.mk:1`|30|LOCKED B06 / initech-3qtt oversized-column trace (Rule 8). Deliberately added.|
|`spec/i123/era_shortfalls.md:1`|27|Initech 123 P3 shortfalls (initech-uqah)|
|`spec/i123/menu_level_visits.tsv:1`|104|1	/Worksheet/Global|
|`spec/i123/menu_ri_offsets.tsv:1`|381|/Worksheet	0ba1	0ba8|
|`spec/i123/menu_tree.golden:1`|381|/	Worksheet	Global  Insert  Delete  Column  Erase  Titles  Window  Status  Page  Learn|
|`spec/i123/wk1_format.h:1`|207|- the Lotus 1-2-3 Release 2.2 .WK1 record set the|
|`spec/i123/wk1_formula.h:1`|291|- the Lotus 1-2-3 Release 2.2 formula bytecode:|
|`spec/i123_copy_smoke_keys.h:1`|17|Deliberately added CPU-path replay, TBX_SMOKE_I123_COPY only (factory).|

## 2. Headers it needs from elsewhere

Literal include sites, including host-only conditional includes embedded in production TUs. PUBLIC INTERFACE means publish/move the contract or supported reusable library header; INTERNAL LEAK means replace a foreign implementation dependency. This is not an assertion that every conditional include is active in the production build.

|Include site|Spelling → resolved path|Classification|
|---|---|---|
|`os/i123/app/face_a.c:72`|`tbx.h` → `os/apps/tbx/tbx.h`|PUBLIC INTERFACE|
|`os/i123/app/face_a.c:73`|`tbx_save.h` → `os/apps/tbx/tbx_save.h`|PUBLIC INTERFACE|
|`os/i123/app/face_a.c:76`|`i123/wk1_format.h` → `spec/i123/wk1_format.h`|PUBLIC INTERFACE|
|`os/i123/app/face_a.c:77`|`cpanels_record.h` → `spec/cpanels_record.h`|PUBLIC INTERFACE|
|`os/i123/core/eval.c:44`|`i123/wk1_format.h` → `spec/i123/wk1_format.h`|PUBLIC INTERFACE|
|`os/i123/core/eval.c:45`|`i123/wk1_formula.h` → `spec/i123/wk1_formula.h`|PUBLIC INTERFACE|
|`os/i123/core/fmt.c:53`|`i123/wk1_format.h` → `spec/i123/wk1_format.h`|PUBLIC INTERFACE|
|`os/i123/core/formula.c:55`|`i123/wk1_format.h` → `spec/i123/wk1_format.h`|PUBLIC INTERFACE|
|`os/i123/core/formula.c:56`|`i123/wk1_formula.h` → `spec/i123/wk1_formula.h`|PUBLIC INTERFACE|
|`os/i123/core/range.c:10`|`i123/wk1_format.h` → `spec/i123/wk1_format.h`|PUBLIC INTERFACE|
|`os/i123/core/range.c:11`|`i123/wk1_formula.h` → `spec/i123/wk1_formula.h`|PUBLIC INTERFACE|
|`os/i123/core/recalc.c:59`|`i123/wk1_format.h` → `spec/i123/wk1_format.h`|PUBLIC INTERFACE|
|`os/i123/core/recalc.c:60`|`i123/wk1_formula.h` → `spec/i123/wk1_formula.h`|PUBLIC INTERFACE|
|`os/i123/core/sheet.c:23`|`i123/wk1_format.h` → `spec/i123/wk1_format.h`|PUBLIC INTERFACE|
|`os/i123/fs/wk1.c:24`|`i123/wk1_format.h` → `spec/i123/wk1_format.h`|PUBLIC INTERFACE|

## 3. How it is built today

`-m32 -march=i386` pins the 32-bit 386-compatible ISA used on the emulated 486. Freestanding/no-libc linkage and entry/linker addresses remain required until packaging changes. `-fno-pic -fno-pie` agrees with flat placement and relocation derivation. Kernel `-mregparm=3 -mrtd -freg-struct-return` was introduced to save bytes, but it is now an ABI: keep C/assembly declarations and all linked translation units in agreement until changing both sides. Do not simply delete it from one file. INT 81h arguments remain explicit dword cdecl with `TBX_TRAP_ABI` exceptions. Preserve `-fno-tree-loop-distribute-patterns` where it prevents freestanding code acquiring libc calls, or supply those functions. `-Oz`, per-object `-Os`, LTO/partition choices, `-fno-inline`, per-pass `-fno-*` lists, `-mtune=generic`, `-malign-data=abi`, `--param` peeling limits and 4-byte stack tuning were selected for legacy caps; retire after replacing caps and verifying the coherent ABI. `-fno-strict-aliasing` and `-fno-common` are correctness/toolchain choices, not automatically disposable size levers.

Commands below are captured output of `make -n -B --trace ...`, never executed. `-o build/flair_dac.stamp.mk` suppresses the parse-time included-makefile remake. Empty headroom-enumeration overrides only suppress irrelevant recursive database enumeration. Shell loops preserve exact input/link order; variables like `$objs` are runtime shell variables, not omitted make flags.

### dry-production-apps-i123.txt
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/tenant/i123.exe
# exit: 0
Makefile:12268: target 'build' does not exist
mkdir -p build
Makefile:76: update target 'build/.makefile.stamp' due to: Makefile os/samir/cmd/flow_verbs.def os/samir/cmd/mutate_verbs.def os/samir/cmd/proc_verbs.def os/samir/cmd/query_verbs.def os/samir/cmd/set_options.def harness/diff/fat_diff/doslook.mk harness/diff/fat_diff/xcopy_tree.mk harness/diff/fat_diff/copy_stamp.mk tools/serial_trace_pack.c os/milton/kmain_cold.h os/flair/chicago_packed.h harness/diff/fat_diff/shell_files.mk
mkdir -p build/ && touch build/.makefile.stamp
Makefile:10710: update target 'build/tenant/i123.o0.elf' due to: os/i123/app/face_a.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/eval.c os/i123/core/recalc.c os/i123/core/range.c os/i123/core/menu.c os/apps/tbx/tbx_rt.c os/apps/tbx/tbx.h os/apps/tbx/tbx_save.h spec/toolbox_gate.h spec/event_model.h spec/cp_dialog.h os/apps/tbx/tenant.ld os/i123/core/i123.h os/i123/core/menu.h os/i123/core/menu_data.h spec/i123/wk1_format.h spec/i123/wk1_formula.h spec/cpanels_record.h
mkdir -p build/tenant/i123
set -e; objs=; for s in os/i123/app/face_a.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/eval.c os/i123/core/recalc.c os/i123/core/range.c os/i123/core/menu.c os/apps/tbx/tbx_rt.c; do o=build/tenant/i123/$(basename $s .c).o; gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -fno-tree-loop-distribute-patterns -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-common -Ispec -Ios/apps/tbx -Ios/i123/core -DI123_MENU_DISK -flto -ffunction-sections -fdata-sections -Oz -fno-jump-tables -fno-inline -fno-ipa-cp-clone -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fomit-frame-pointer -fno-tree-loop-im -fno-tree-sra -fno-tree-dominator-opts -fno-tree-scev-cprop -fno-move-loop-invariants -fno-shrink-wrap --param=max-completely-peel-times=0 -mregparm=3 -mrtd -freg-struct-return -fno-caller-saves -mtune=generic -fno-if-conversion -fno-split-wide-types -fno-tree-switch-conversion -malign-data=abi -fno-tree-cselim -fno-tree-coalesce-vars -fno-ipa-strict-aliasing -fno-tree-phiprop -c $s -o $o; objs="$objs $o"; done; gcc -m32 -nostdlib -no-pie -flto -Oz -fno-inline -fomit-frame-pointer -fno-tree-loop-im -fno-tree-sra -fno-tree-dominator-opts -fno-tree-scev-cprop -fno-move-loop-invariants -fno-shrink-wrap --param=max-completely-peel-times=0 -mregparm=3 -mrtd -freg-struct-return -fno-caller-saves -mtune=generic -fno-if-conversion -fno-split-wide-types -fno-tree-switch-conversion -malign-data=abi -fno-tree-cselim -fno-tree-coalesce-vars -fno-ipa-strict-aliasing -fno-tree-phiprop -Wl,--gc-sections,--no-warn-rwx-segments,--build-id=none -T os/apps/tbx/tenant.ld -Wl,--defsym=TBX_BASE=0 -o build/tenant/i123.o0.elf $objs; gcc -m32 -nostdlib -no-pie -flto -Oz -fno-inline -fomit-frame-pointer -fno-tree-loop-im -fno-tree-sra -fno-tree-dominator-opts -fno-tree-scev-cprop -fno-move-loop-invariants -fno-shrink-wrap --param=max-completely-peel-times=0 -mregparm=3 -mrtd -freg-struct-return -fno-caller-saves -mtune=generic -fno-if-conversion -fno-split-wide-types -fno-tree-switch-conversion -malign-data=abi -fno-tree-cselim -fno-tree-coalesce-vars -fno-ipa-strict-aliasing -fno-tree-phiprop -Wl,--gc-sections,--no-warn-rwx-segments,--build-id=none -T os/apps/tbx/tenant.ld -Wl,--defsym=TBX_BASE=0x01010140 -o build/tenant/i123.o1.elf $objs
Makefile:9650: update target 'build/mzlink' due to: tools/mzlink.c spec/initechmz_extended.h spec/samir/memory_contract.h
cc -std=c11 -Wall -Wextra -Werror -o build/mzlink tools/mzlink.c
Makefile:10710: update target 'build/tenant/i123.exe' due to: build/tenant/i123.o0.elf build/mzlink
objcopy -O binary build/tenant/i123.o0.elf build/tenant/i123.o0.bin
objcopy -O binary build/tenant/i123.o1.elf build/tenant/i123.o1.bin
ent=$(nm build/tenant/i123.o0.elf | awk '$3=="tbx_entry"{print $1}'); [ "$ent" = 00000000 ] || { printf '!!! i123.exe: tbx_entry is at %s, not module byte 0 (DEC-08a.2)\n' "$ent"; exit 1; }
bss=$(nm build/tenant/i123.o0.elf | awk '$3=="__bss_len"{print $1}'); paras=$(( (0x$bss + 15) / 16 )); build/mzlink build/tenant/i123.o0.bin build/tenant/i123.exe --reloc-diff build/tenant/i123.o1.bin 0x01010140 --entry 0 --minalloc $paras
sz=$(wc -c < build/tenant/i123.exe); [ "$sz" -le 65536 ] || { printf '!!! i123.exe is %s bytes: over the V1 tenant file cap TBX_TENANT_IMAGE_MAX (64 KiB)\n' "$sz"; exit 1; }

# stderr

```

The two ELF links at TBX_BASE=0 and 0x01010140 produce two raw images; mzlink derives relocations by differencing, enforces entry 0, and writes BSS paragraphs into e_minalloc. The 65,536-byte tenant file check is legacy cap policy, while entry/relocation/BSS semantics remain necessary until the loader contract changes.

## 4. Host tests that travel with it

Counts by unique host program/main source (not mutant binaries, CLI cases, or gate names): **ORACLE-STRONG 6; UNIT 10; META 0; SIZE/LAYOUT 0.** META is reserved for factory/process/spec-inventory checks; input generators are listed as support sources rather than counted as tests. Some static classes are inferred from inspected test bodies; the basis is stated for each.

Every build variant name is listed; the exact canonical compilation is from its artifact-only dry run. Variant recipes and invocation assertions remain available in [gates-apps-i123.md](gates-apps-i123.md). The complete host dry-run archive is [dry-host.json](dry-host.json). Gate lists below use direct binary prerequisites/recipe references, avoiding giant transitive build closures that would misassign whole kernel test inventories.

### harness/diff/wk1_diff/check_i123_menu_levels.c — UNIT

`harness/diff/wk1_diff/check_i123_menu_levels.c:1`; Factory C: expected entry screens come only from the RI-checked golden,. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/check_i123_menu_levels`.
Run/assert gate(s): `test-flair-i123-menu`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/i123/menu_level_visits.tsv`, `spec/i123/menu_tree.golden`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/check_i123_menu_levels`):
```sh
cc -std=c11 -Wall -Wextra -Werror -o build/check_i123_menu_levels harness/diff/wk1_diff/check_i123_menu_levels.c
```

### harness/diff/wk1_diff/check_i123_numeric.c — UNIT

`harness/diff/wk1_diff/check_i123_numeric.c:1`; Independent stopped-image WK1 checker. Does not link the worksheet code.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/check_i123_numeric`.
Run/assert gate(s): `test-flair-i123-numeric`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): `LOTUS123_DECOMP` → `../lotus123-decomp` (Makefile:39–43 defaults).
Canonical `make -n` compile/link (`build/check_i123_numeric`):
```sh
cc -std=c11 -Wall -Wextra -Werror -O2 -o build/check_i123_numeric harness/diff/wk1_diff/check_i123_numeric.c
```

### harness/diff/wk1_diff/test_i123_copy_move.c — ORACLE-STRONG

`harness/diff/wk1_diff/test_i123_copy_move.c:1`; Factory C independent coordinate model, R2.2 Reference /Copy pp. 3-2..6. independent period corpus/reference consumer; paths and opt-out/error policy below.
Build target(s): `build/test_i123_copy_move`.
Run/assert gate(s): `test-flair-i123-copy`, `test-flair-i123-copy-cpu`, `test-i123-copy-move-model`, `test-i123-copy-tile`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `fixtures/i123-copy/`, `fixtures/i123-copy/SINGLE.WK1`, `harness/diff/wk1_diff/wk1_formula_ref.py`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`.
External root(s): `LOTUS123_DECOMP` → `../lotus123-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`.
Canonical `make -n` compile/link (`build/test_i123_copy_move`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ios/i123/core -Ispec -o build/test_i123_copy_move harness/diff/wk1_diff/test_i123_copy_move.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/eval.c os/i123/core/recalc.c os/i123/core/range.c os/i123/core/menu.c
```

### harness/diff/wk1_diff/test_i123_formula.c — ORACLE-STRONG

`harness/diff/wk1_diff/test_i123_formula.c:1`; - THE ORACLE for the Initech 123. independent period corpus/reference consumer; paths and opt-out/error policy below.
Build target(s): `build/test_i123_formula`.
Run/assert gate(s): `test-i123-formula`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/wk1_diff/test_i123_formula.c`, `harness/diff/wk1_diff/wk1_formula_ref.py`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`.
External root(s): `LOTUS123_DECOMP` → `../lotus123-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `seed/test_assert.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`.
Canonical `make -n` compile/link (`build/test_i123_formula`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Iseed -Ios/i123/core -Ispec -o build/test_i123_formula harness/diff/wk1_diff/test_i123_formula.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/eval.c os/i123/core/recalc.c os/i123/core/range.c os/i123/core/menu.c
```

### harness/diff/wk1_diff/test_i123_menu_tree.c — ORACLE-STRONG

`harness/diff/wk1_diff/test_i123_menu_tree.c:1`; Factory independent RI reader and artifact dump. No spec constants or. independent period corpus/reference consumer; paths and opt-out/error policy below.
Build target(s): `build/test_i123_menu_tree`, `build/test_i123_menu_tree_mutant`.
Run/assert gate(s): `test-i123-menu-tree`, `test-i123-menu-tree-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/i123/menu_ri_offsets.tsv`, `spec/i123/menu_tree.golden`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_i123_menu_tree`):
```sh
cc -std=c11 -Wall -Wextra -Werror -DI123_MENU_HOST -Ios/i123/core -o build/test_i123_menu_tree harness/diff/wk1_diff/test_i123_menu_tree.c os/i123/core/menu.c
```

### harness/diff/wk1_diff/test_i123_recalc_order.c — ORACLE-STRONG

`harness/diff/wk1_diff/test_i123_recalc_order.c:1`; - THE ORACLE for Initech. independent period corpus/reference consumer; paths and opt-out/error policy below.
Build target(s): `build/test_i123_recalc_order`.
Run/assert gate(s): `test-i123-recalc-order`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/wk1_diff/test_i123_recalc_order.c`, `harness/diff/wk1_diff/wk1_formula_ref.py`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`.
External root(s): `LOTUS123_DECOMP` → `../lotus123-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `seed/test_assert.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`.
Canonical `make -n` compile/link (`build/test_i123_recalc_order`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Iseed -Ios/i123/core -Ispec -o build/test_i123_recalc_order harness/diff/wk1_diff/test_i123_recalc_order.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/eval.c os/i123/core/recalc.c os/i123/core/range.c os/i123/core/menu.c
```

### harness/diff/wk1_diff/test_i123_transfer_baseline.c — ORACLE-STRONG

`harness/diff/wk1_diff/test_i123_transfer_baseline.c:1`; Independent FORMULA body oracle for existing R2.2 mint sheets. These are. independent period corpus/reference consumer; paths and opt-out/error policy below.
Build target(s): `build/test_i123_transfer_baseline`.
Run/assert gate(s): `test-i123-copy-move-baseline`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/wk1_diff/wk1_formula_ref.py`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`.
External root(s): `LOTUS123_DECOMP` → `../lotus123-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`.
Canonical `make -n` compile/link (`build/test_i123_transfer_baseline`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ios/i123/core -Ispec -o build/test_i123_transfer_baseline harness/diff/wk1_diff/test_i123_transfer_baseline.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/eval.c os/i123/core/recalc.c os/i123/core/range.c os/i123/core/menu.c
```

### harness/diff/wk1_diff/test_i123_wk1_roundtrip.c — ORACLE-STRONG

`harness/diff/wk1_diff/test_i123_wk1_roundtrip.c:1`; - THE ORACLE for the. independent period corpus/reference consumer; paths and opt-out/error policy below.
Build target(s): `build/test_i123_wk1_roundtrip`.
Run/assert gate(s): `test-i123-wk1-roundtrip`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `fixtures/i123-width/13_w240_labels.png`, `harness/diff/wk1_diff/test_i123_wk1_roundtrip.c`, `harness/diff/wk1_diff/wk1_canon.py`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`.
External root(s): `LOTUS123_DECOMP` → `../lotus123-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `seed/test_assert.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`.
Canonical `make -n` compile/link (`build/test_i123_wk1_roundtrip`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Iseed -Ios/i123/core -Ispec -o build/test_i123_wk1_roundtrip harness/diff/wk1_diff/test_i123_wk1_roundtrip.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/eval.c os/i123/core/recalc.c os/i123/core/range.c os/i123/core/menu.c
```

### harness/proptest/test_i123_cells.c — UNIT

`harness/proptest/test_i123_cells.c:1`; Initech 123 cell-store capacity oracle, bead initech-3s12 (I123 B02; audit. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_i123_cells`.
Run/assert gate(s): `test-i123-cells`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/cpanels_record.h`, `spec/event_model.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
External root(s): `LOTUS123_DECOMP` → `../lotus123-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/proptest/test_i123_quit.c`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
Canonical `make -n` compile/link (`build/test_i123_cells`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/apps/tbx -Ios/i123/core -Ispec -o build/test_i123_cells harness/proptest/test_i123_cells.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/eval.c os/i123/core/recalc.c os/i123/core/range.c os/i123/core/menu.c
```

### harness/proptest/test_i123_clock.c — UNIT

`harness/proptest/test_i123_clock.c:1`; B08 / initech-6edn: real Face A with injected TBX_CLOCK snapshots.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_i123_clock`.
Run/assert gate(s): `test-i123-clock`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/cpanels_record.h`, `spec/event_model.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/proptest/test_i123_quit.c`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
Canonical `make -n` compile/link (`build/test_i123_clock`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/apps/tbx -Ios/i123/core -Ispec -o build/test_i123_clock harness/proptest/test_i123_clock.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/eval.c os/i123/core/recalc.c os/i123/core/range.c os/i123/core/menu.c
```

### harness/proptest/test_i123_entry.c — UNIT

`harness/proptest/test_i123_entry.c:1`; Initech 123 entry boundary/state oracle, bead initech-twxf (B13).. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_i123_entry`, `build/test_i123_entry_currency_mutant`, `build/test_i123_entry_mutant`.
Run/assert gate(s): `test-i123-entry`, `test-i123-entry-currency-mutant`, `test-i123-entry-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `fixtures/i123-keys/50_bad_value.png`, `fixtures/i123-numeric/NUMENTRY.WK1`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
External root(s): `LOTUS123_DECOMP` → `../lotus123-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/proptest/test_i123_quit.c`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
Canonical `make -n` compile/link (`build/test_i123_entry`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/apps/tbx -Ios/i123/core -Ispec -o build/test_i123_entry harness/proptest/test_i123_entry.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/eval.c os/i123/core/recalc.c os/i123/core/range.c os/i123/core/menu.c
```

### harness/proptest/test_i123_hit.c — UNIT

`harness/proptest/test_i123_hit.c:1`; Initech 123 B31 cell hit-testing oracle, bead initech-umfi (audit L017:. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_i123_hit`.
Run/assert gate(s): `test-i123-hit`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/cpanels_record.h`, `spec/event_model.h`, `spec/flair_i123_traces.mk`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/proptest/test_i123_quit.c`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
Canonical `make -n` compile/link (`build/test_i123_hit`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/apps/tbx -Ios/i123/core -Ispec -o build/test_i123_hit harness/proptest/test_i123_hit.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/eval.c os/i123/core/recalc.c os/i123/core/range.c os/i123/core/menu.c
```

### harness/proptest/test_i123_keys.c — UNIT

`harness/proptest/test_i123_keys.c:1`; Initech 123 B11 keyboard oracle, bead initech-69mz: EDIT (F2), GOTO (F5),. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_i123_keys`.
Run/assert gate(s): `test-i123-keys`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/cpanels_record.h`, `spec/event_model.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
External root(s): `LOTUS123_DECOMP` → `../lotus123-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/proptest/test_i123_quit.c`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
Canonical `make -n` compile/link (`build/test_i123_keys`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/apps/tbx -Ios/i123/core -Ispec -o build/test_i123_keys harness/proptest/test_i123_keys.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/eval.c os/i123/core/recalc.c os/i123/core/range.c os/i123/core/menu.c
```

### harness/proptest/test_i123_quit.c — UNIT

`harness/proptest/test_i123_quit.c:1`; - the host oracle of Initech 123's EXIT. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_i123_cells`, `build/test_i123_clock`, `build/test_i123_entry`, `build/test_i123_entry_currency_mutant`, `build/test_i123_entry_mutant`, `build/test_i123_hit`, `build/test_i123_keys`, `build/test_i123_quit`, `build/test_i123_range_menu`, `build/test_i123_range_menu_mutant`, `build/test_i123_resize`.
Run/assert gate(s): `test-i123-cells`, `test-i123-clock`, `test-i123-entry`, `test-i123-entry-currency-mutant`, `test-i123-entry-mutant`, `test-i123-hit`, `test-i123-keys`, `test-i123-quit`, `test-i123-range-menu`, `test-i123-range-menu-mutant`, `test-i123-resize`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/proptest/test_i123_quit.c`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/flair_i123_traces.mk`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
External root(s): `LOTUS123_DECOMP` → `../lotus123-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/proptest/test_i123_cells.c`, `harness/proptest/test_i123_clock.c`, `harness/proptest/test_i123_entry.c`, `harness/proptest/test_i123_hit.c`, `harness/proptest/test_i123_keys.c`, `harness/proptest/test_i123_range_menu.c`, `harness/proptest/test_i123_resize.c`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
Canonical `make -n` compile/link (`build/test_i123_cells`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/apps/tbx -Ios/i123/core -Ispec -o build/test_i123_cells harness/proptest/test_i123_cells.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/eval.c os/i123/core/recalc.c os/i123/core/range.c os/i123/core/menu.c
```

### harness/proptest/test_i123_range_menu.c — UNIT

`harness/proptest/test_i123_range_menu.c:1`; Host UI integration over the recording Toolbox mock. Expectations are. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_i123_range_menu`, `build/test_i123_range_menu_mutant`.
Run/assert gate(s): `test-i123-range-menu`, `test-i123-range-menu-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `fixtures/i123-copy/`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/proptest/test_i123_quit.c`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
Canonical `make -n` compile/link (`build/test_i123_range_menu`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/apps/tbx -Ios/i123/core -Ispec -o build/test_i123_range_menu harness/proptest/test_i123_range_menu.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/eval.c os/i123/core/recalc.c os/i123/core/range.c os/i123/core/menu.c
```

### harness/proptest/test_i123_resize.c — UNIT

`harness/proptest/test_i123_resize.c:1`; Initech 123 B12 resized-worksheet geometry oracle, bead initech-89rc (audit. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_i123_resize`.
Run/assert gate(s): `test-i123-resize`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `fixtures/i123-width/`, `spec/chrome_metrics.h`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/proptest/test_i123_quit.c`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/i123/wk1_format.h`, `spec/i123/wk1_formula.h`, `spec/toolbox_gate.h`.
Canonical `make -n` compile/link (`build/test_i123_resize`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/apps/tbx -Ios/i123/core -Ispec -o build/test_i123_resize harness/proptest/test_i123_resize.c os/i123/core/sheet.c os/i123/core/fmt.c os/i123/fs/wk1.c os/i123/core/formula.c os/i123/core/eval.c os/i123/core/recalc.c os/i123/core/range.c os/i123/core/menu.c
```

### Recipe-driven host suites and variants

All assigned legacy gate recipes, including compiler/FPC/mtools/script orchestration and mutants, are enumerated in [gates-apps-i123.md](gates-apps-i123.md). Host tier here is the declared legacy vector; some entries launch emulators transitively. The directory name proptest is not itself evidence of a property oracle.

## 5. Emulator gates worth keeping as smokes

**27 exclusive primary-owned entries in the 799-entry declared legacy emulator vector.** Also 28 defined test-* targets owned here have a static dependency closure reaching QEMU/Bochs (includes wrappers, mutants and names outside the active vector; not a runtime boot count). Cross-component smokes below are not double-counted. Exact primary attribution is in [gate-attribution.tsv](gate-attribution.tsv).

### test-flair-i123

#  recipe to execute (from 'Makefile', line 23808):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_tenants.img`.
Input trace `FLAIR_I123_PRE_SPEC` at `spec/flair_i123_traces.mk:41`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:20,l1,l0
```
Input trace `FLAIR_I123_QUIT_SPEC` at `spec/flair_i123_traces.mk:37`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:20,l1,l0,l1,l0,kctrl-q
```
Input trace `FLAIR_I123_TYPE_SPEC` at `spec/flair_i123_traces.mk:32`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:20,l1,l0,l1,l0,kshift-h,ke,kl,kl,ko,kright,k1,k2,k3,kright,k1,kdot,k5,kright,kminus,k7,kret
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@$(call APPL_ALIVE,build/flair_i123_type.serial) || { printf '!!! test-flair-i123 [1/3] FAIL: the desktop did not survive\n'; grep -E '^(PANIC|TENANT-(LOAD|REG|CRASH|EXIT)|HALTED)' build/flair_i123_type.serial; exit 1; }
@$(call I123_TYPE_SERIAL,build/flair_i123_type.serial) || { printf '!!! test-flair-i123 [1/3] FAIL: TYPE -- 123 did not come up, or row 1 / line 1 / the mode / the pointer are not what the typed MINT01 row must give\n'; grep -E '^(TENANT-(LOAD|REGISTER|CRASH|EXIT)|FLAIR-DISPATCH|PANIC)' build/flair_i123_type.serial; grep -E '^TENANT-GATE ax=0x0033 win=1 x=(1|33|249|577) y=(0|64) ' build/flair_i123_type.serial | tail -4; exit 1; }
@printf '>>> test-flair-i123 [1/3] TYPE: %s; row 1 drawn exactly as the real 1-2-3 showed MINT01 ("Hello 123 1.5 -7"), line 1 "D1: -7", READY, pointer on D1\n' "$$(grep -m1 '^TENANT-LOAD name=123' build/flair_i123_type.serial)"
		printf '!!! test-flair-i123 [1/3] FAIL: TYPE -- the screen is not the Face A layout (border reverse %s/9728 px, mode %s/640, pointer on D1 %s/1152, A1 ink %s px, pointer below D1 %s px)\n' "$$beq" "$$meq" "$$peq" "$$ane" "$$qeq"; exit 1; }; \
@$(call APPL_ALIVE,build/flair_i123_pre.serial) && ! grep -q '^TENANT-' build/flair_i123_pre.serial || { printf '!!! test-flair-i123 [3/3] FAIL: the never-launched baseline did not come up clean\n'; exit 1; }
@$(call APPL_ALIVE,build/flair_i123_quit.serial) && grep -qx 'TENANT-MENU menu=129 item=1 src=key' build/flair_i123_quit.serial && grep -qx 'TENANT-EXIT rc=0 via=exit' build/flair_i123_quit.serial || { printf '!!! test-flair-i123 [2/3] FAIL: QUIT -- Ctrl-Q must reach 123 as its File > Quit and end it\n'; grep -E '^(TENANT-(MENU|EVT what=81|EXIT|CRASH)|FLAIR-DISPATCH|PANIC)' build/flair_i123_quit.serial; exit 1; }
@cmp -s build/flair_i123_pre.ppm build/flair_i123_quit.ppm || { printf '!!! test-flair-i123 [2/3] FAIL: QUIT -- the post-quit frame differs from the never-launched baseline (%s bytes)\n' "$$(cmp -l build/flair_i123_pre.ppm build/flair_i123_quit.ppm | wc -l)"; exit 1; }
```

### test-flair-i123-copy

#  recipe to execute (from 'Makefile', line 35641):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_i123_copy_data.img`, `build/flair_tenants_mut_i123_keys_gate.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@! grep -Eq 'KBD-RING-DROP|FLAIR-EVT-DROP' build/flair_i123_copy.serial
build/ppm_text_check --i123-copy build/flair_i123_copy.ppm
```

### test-flair-i123-numeric

#  recipe to execute (from 'harness/diff/wk1_diff/i123_numeric.mk', line 20):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_i123_numeric_data.img`, `build/flair_tenants_recorddbl.img`.
Input trace `I123_NUMERIC_TRACE` at `harness/diff/wk1_diff/i123_numeric.mk:4`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m88:20,l1,l0,l1,l0,kshift-4,k1,k2,kdot,k3,k4,kret,kright,kshift-2,kn,ko,kw,kret,kslash,kf,ks,kn,ko,kw,k1,kret,WSaved now1.wk1,w2000,kf9,kslash,kf,ks,kn,ko,kw,k2,kret,WSaved now2.wk1
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@$(call APPL_ALIVE,build/flair_i123_numeric.serial) && grep -qx 'TENANT-REGISTER ok app=123' build/flair_i123_numeric.serial
@! grep -Eq 'KBD-RING-DROP|FLAIR-EVT-DROP' build/flair_i123_numeric.serial
@grep -q 'consumed=.* OK$$' build/i123-NOW2.ref
```

### test-flair-i123-menu

#  recipe to execute (from 'Makefile', line 33560):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_tenants_mut_i123_menu_gate.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Inline harness input/options:
```text
--keys-ack 'FLAIR-EVT what=4 ' --screendump --screendump-after FLAIR-LIVE-OK)
--keys-ack 'FLAIR-EVT what=4 ')
--keys-ack 'FLAIR-EVT what=4 ')
--keys-ack 'FLAIR-EVT what=4 ')
--keys-ack 'FLAIR-EVT what=4 ')
--keys-ack 'FLAIR-EVT what=4 ')
--keys-ack 'FLAIR-EVT what=4 ')
--keys-ack 'FLAIR-EVT what=4 ')
```
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@! grep -q '^FLAIR-EVT-DROP' build/flair_i123_menu_ops.serial
@test "$$(grep -c '^FLAIR-EVT what=3 ' build/flair_i123_menu_ops.serial)" -eq 50
@$(call I123_MENU_OPS_OK,build/flair_i123_menu_ops.serial) || { printf '!!! test-flair-i123-menu FAIL: copy/move row, menu text or MENU\n'; exit 1; }
build/ppm_text_check --i123-menu build/flair_i123_menu_ops.ppm
 grep -qF 's="Attach  Detach  Invoke  Clear  Quit' $$f && grep -qF 's="No  Yes' $$f && \
 grep -qF 's="Command not implemented' $$f || { printf '!!! test-flair-i123-menu FAIL: unavailable leaf / Quit navigation\n'; exit 1; }
```

### test-flair-i123-resize

#  recipe to execute (from 'Makefile', line 33448):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_tenants_mut_i123_keys_gate.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@f=build/flair_i123_resize.serial; $(call APPL_ALIVE,$$f) && grep -qx 'TENANT-REGISTER ok app=123' $$f || { printf '!!! test-flair-i123-resize FAIL: launch/desktop\n'; exit 1; }
@! grep -Eq 'KBD-RING-DROP|FLAIR-EVT-DROP' build/flair_i123_resize.serial || { printf '!!! test-flair-i123-resize FAIL: input ring overrun\n'; exit 1; }
@$(call I123_RS1_OK,build/flair_i123_resize.serial) || { printf '!!! test-flair-i123-resize FAIL: RS1 shrunk to 352x246, READY not at the content edge (x=281)\n'; exit 1; }
@$(call I123_RS2_OK,build/flair_i123_resize.serial) || { printf '!!! test-flair-i123-resize FAIL: RS2 (L018) Right to E1 at 352x246 -- border B..E, pointer x=249, "E1:", READY\n'; exit 1; }
@$(call I123_RS3_OK,build/flair_i123_resize.serial) || { printf '!!! test-flair-i123-resize FAIL: RS3 grown to 552x346 -- border B..G, pointer x=249, READY x=481\n'; exit 1; }
@$(call I123_RS4_OK,build/flair_i123_resize.serial) || { printf '!!! test-flair-i123-resize FAIL: RS4 shrunk to 280x246 -- border C..E, pointer x=177, READY x=209\n'; exit 1; }
build/ppm_text_check --i123-resize build/flair_i123_resize.ppm
```

### test-flair-i123-file-ui

#  recipe to execute (from 'Makefile', line 32066):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_i123_cancel_data.img`, `build/flair_i123_file_data.img`, `build/flair_i123_replace_data.img`, `build/flair_tenants_mut_i123_file_gate.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@$(call I123_UI_OK,build/flair_i123_file_ui.serial) || { printf '!!! test-flair-i123-file-ui FAIL: menu/prompt/visible refusal\n'; exit 1; }
@$(call APPL_ALIVE,build/flair_i123_file_error.serial) && $(call I123_FILE_ROWS,build/flair_i123_file_error.serial) && grep -q 's="File not found' build/flair_i123_file_error.serial || { printf '!!! test-flair-i123-file-ui FAIL: error visible and old sheet retained\n'; exit 1; }
@$(call APPL_ALIVE,build/flair_i123_file_bad.serial) && $(call I123_FILE_ROWS,build/flair_i123_file_bad.serial) && grep -q 's="Not a valid WK1 file' build/flair_i123_file_bad.serial || { printf '!!! test-flair-i123-file-ui FAIL: malformed Retrieve preserved old sheet after repaint\n'; exit 1; }
@$(call APPL_ALIVE,build/flair_i123_cancel.serial) && grep -q 's="Cancel  Replace  Backup' build/flair_i123_cancel.serial && cmp build/i123-corpus-original.WK1 build/i123-cancel.WK1 || { printf '!!! test-flair-i123-file-ui FAIL: overwrite Cancel\n'; exit 1; }
@$(call APPL_ALIVE,build/flair_i123_replace.serial) && grep -q "LABEL.*c0 r0 label=.*Changed" build/i123-replace.ref || { printf '!!! test-flair-i123-file-ui FAIL: overwrite Replace\n'; exit 1; }
fsck.fat -n build/flair_i123_replace_data.img
```

The remaining legacy emulator entries cover app launch/registration, menu and keyboard/mouse input, file/save/error behavior, period pixels and focused mutant controls. Mutant twins establish sensitivity of the old assertion; convert useful faults to patches instead of maintaining extra resident kernels. The complete names/recipes are linked above.

## 6. Mutation hooks in production source

Measured count is conditional directive sites containing deliberately-wrong macro names (`MUT`, `MUTATE`, `MUTANT`, `BROKEN`, `DROP_WRITE`, `DOTRUNC`), not #else arms or runtime executions. One directive can mention two macros; distinct macro count is also provided. `#ifndef` controls count because the wrong behavior may be its absence. Non-mutation BOOT/profile selectors are excluded.

|File|Guard sites|Distinct macros|Macros and exact sites|
|---|---:|---:|---|
|`os/i123/app/face_a.c:1`|21|20|`os/i123/app/face_a.c:131` `I123_MUT_ENTRY_75`; `os/i123/app/face_a.c:185` `I123_MUT_CLOCK_MISSING`; `os/i123/app/face_a.c:191` `I123_MUT_CLOCK_FROZEN`; `os/i123/app/face_a.c:195` `I123_MUT_CLOCK_MISSING`; `os/i123/app/face_a.c:390` `I123_MUT_WIDE_HIDDEN`; `os/i123/app/face_a.c:436` `I123_MUT_FIXED_GEOMETRY`; `os/i123/app/face_a.c:626` `I123_MUT_GOTO_FOLLOW`; `os/i123/app/face_a.c:638` `I123_MUT_SAVE_FAIL_QUITS`; `os/i123/app/face_a.c:698` `I123_MUT_GET_CLOBBER`; `os/i123/app/face_a.c:727` `I123_MUT_SAVE_EMPTY`; `os/i123/app/face_a.c:938` `I123_MUT_PERIOD_TEXT`; `os/i123/app/face_a.c:995` `I123_MUT_ABSENT_UNPROTECTED`; `os/i123/app/face_a.c:1017` `I123_MUT_CURRENCY_REJECT`; `os/i123/app/face_a.c:1025` `I123_MUT_NO_RECALC`; `os/i123/app/face_a.c:1044` `I123_MUT_NEVER_DIRTY`; `os/i123/app/face_a.c:1078` `I123_MUT_ARROWS_DEAD`; `os/i123/app/face_a.c:1117` `I123_MUT_TAB_ONE_CELL`; `os/i123/app/face_a.c:1193` `I123_MUT_EDIT_ESC_CANCELS`; `os/i123/app/face_a.c:1264` `I123_MUT_HIT_IGNORES_SCROLL`; `os/i123/app/face_a.c:1292` `I123_MUT_PENDING_IGNORED`; `os/i123/app/face_a.c:1321` `I123_MUT_CANCEL_QUITS`|
|`os/i123/core/eval.c:1`|1|1|`os/i123/core/eval.c:1257` `I123_MUT_ROUND_EVEN`|
|`os/i123/core/fmt.c:1`|2|2|`os/i123/core/fmt.c:198` `I123_MUT_GENERAL_ROUNDS`; `os/i123/core/fmt.c:511` `I123_MUT_WIDE_HIDDEN`|
|`os/i123/core/formula.c:1`|3|3|`os/i123/core/formula.c:122` `I123_MUT_ERR_LOAD`; `os/i123/core/formula.c:130` `I123_MUT_ROW_ERR`; `os/i123/core/formula.c:525` `I123_MUT_PREC_FLAT`|
|`os/i123/core/i123.h:1`|0|0|none|
|`os/i123/core/menu.c:1`|2|1|`os/i123/core/menu.c:32` `I123_MUT_MENU_SPELLING`; `os/i123/core/menu.c:74` `I123_MUT_MENU_SPELLING`|
|`os/i123/core/menu.h:1`|0|0|none|
|`os/i123/core/menu_data.h:1`|0|0|none|
|`os/i123/core/range.c:1`|6|6|`os/i123/core/range.c:129` `I123_MUT_MOVE_COL14`; `os/i123/core/range.c:134` `I123_MUT_ERR_WORD`; `os/i123/core/range.c:157` `I123_MUT_COPY_ABSOLUTE`; `os/i123/core/range.c:186` `I123_MUT_COPY_RECT_REFUSED`; `os/i123/core/range.c:210` `I123_MUT_ABSENT_UNPROTECTED`; `os/i123/core/range.c:245` `I123_MUT_MOVE_STALE`|
|`os/i123/core/recalc.c:1`|3|3|`os/i123/core/recalc.c:136` `I123_MUT_CACHE_ROUND`; `os/i123/core/recalc.c:218` `I123_MUT_NO_PRECEDENTS`; `os/i123/core/recalc.c:361` `I123_MUT_CIRC_BLIND`|
|`os/i123/core/sheet.c:1`|2|2|`os/i123/core/sheet.c:252` `I123_MUT_ALLOC_BEFORE_RECLAIM`; `os/i123/core/sheet.c:417` `I123_MUT_DEFAULT_BLANK`|
|`os/i123/fs/wk1.c:1`|1|1|`os/i123/fs/wk1.c:381` `I123_MUT_LABEL_OPCODE`|

## 7. Coupling to fix during import

Concrete include/external-symbol evidence follows; line numbers refer to legacy-final. Cross-component public ABI includes in section 2 are expected dependencies, not violations.

- `Makefile:10618` → `per-image regparm/rtd flags`: keep tbx_trap cdecl/regparm(0) and change all image TUs together.
- `os/i123/app/face_a.c:77` → `spec/cpanels_record.h`: shared settings data is public contract; do not import Control Panels tenant implementation.

### App Contract use

INT 81h traps observed: `TBX_ALERT`, `TBX_BEEP`, `TBX_CLOCK`, `TBX_CONTENTORIGIN`, `TBX_CONTENT_SIZE`, `TBX_DRAWCELLS`, `TBX_EXIT`, `TBX_FILE_CLOSE`, `TBX_FILE_CREATE_NEW`, `TBX_FILE_DELETE`, `TBX_FILE_OPEN`, `TBX_FILE_READ`, `TBX_FILE_RENAME`, `TBX_FILE_SEEK`, `TBX_FILE_WRITE`, `TBX_FILLRECT`, `TBX_NEWWINDOW`, `TBX_REGISTER`, `TBX_SETMBAR`, `TBX_SETWTITLE`. Full AX/arity and per-app call-site matrix: [sdk.md](sdk.md).

## 8. Discard

Discard legacy size/headroom/overlay-layout policing and mutant kernel/image variants after preserving their functional assertions as ordinary tests or patches. Keep independent golden captures/readers/property suites; do not discard an oracle merely because its legacy target contains size or overlay.
Retire the 64-KiB EXE cap check and cap-only per-application codegen tuning. FONTS/CTENANT are specimens; keep them as test/example apps instead of treating them as user features.
