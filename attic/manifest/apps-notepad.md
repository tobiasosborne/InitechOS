# apps-notepad

NOTEPAD.EXE and SCRAPBK.EXE are two builds of one paged DA source (`-DSCRAPBOOK`). Both share TextEdit and itext font metrics; the store format and save/close semantics travel together.

Evidence revision: `c5d18538049986bbbb27018c351405acf814ebbc` (`legacy-final`). Physical LOC includes comments and blank lines; tests and data are separate from production counts. Source ownership recommendations are explicitly inferences; paths, includes, guard counts and dry-run command text are measured.

## 1. Sources

1 production/source-support files; 427 physical lines. Test programs are in section 4.

|Legacy path|Lines|Role / disposition|
|---|---:|---|
|`os/apps/notepad/notepad.c:1`|427|R4.4 C disk desk accessories, tdnl.18.|

### Contracts, fixture/data and ancillary files owned here

Runtime files in the requested `os/`, `seed/`, `spec/`, `fixtures/` trees are assigned; os/README.md and os/apps/.gitkeep remain explicit unassigned placeholders in SUMMARY. `spec/*.mk` input traces are test data to preserve separately from their obsolete build syntax.

|File|Lines (text)|Role|
|---|---:|---|
|`spec/flair_notepad_traces.mk:1`|20|Locked R4.4 traces (initech-tdnl.18); relative PS/2 deltas <=100.|
|`spec/notepad/store.h:1`|34|Locked R4.4 persistence contract, initech-tdnl.18 (Rule 8).|
|`spec/notepad/store.json:1`|16|"schema_version": 1,|

## 2. Headers it needs from elsewhere

Literal include sites, including host-only conditional includes embedded in production TUs. PUBLIC INTERFACE means publish/move the contract or supported reusable library header; INTERNAL LEAK means replace a foreign implementation dependency. This is not an assertion that every conditional include is active in the production build.

|Include site|Spelling → resolved path|Classification|
|---|---|---|
|`os/apps/notepad/notepad.c:16`|`tbx.h` → `os/apps/tbx/tbx.h`|PUBLIC INTERFACE|
|`os/apps/notepad/notepad.c:17`|`tbx_save.h` → `os/apps/tbx/tbx_save.h`|PUBLIC INTERFACE|
|`os/apps/notepad/notepad.c:18`|`textedit.h` → `os/flair/textedit.h`|PUBLIC INTERFACE|
|`os/apps/notepad/notepad.c:19`|`text.h` → `os/flair/text.h`|PUBLIC INTERFACE|
|`os/apps/notepad/notepad.c:20`|`notepad/store.h` → `spec/notepad/store.h`|PUBLIC INTERFACE|

## 3. How it is built today

`-m32 -march=i386` pins the 32-bit 386-compatible ISA used on the emulated 486. Freestanding/no-libc linkage and entry/linker addresses remain required until packaging changes. `-fno-pic -fno-pie` agrees with flat placement and relocation derivation. Kernel `-mregparm=3 -mrtd -freg-struct-return` was introduced to save bytes, but it is now an ABI: keep C/assembly declarations and all linked translation units in agreement until changing both sides. Do not simply delete it from one file. INT 81h arguments remain explicit dword cdecl with `TBX_TRAP_ABI` exceptions. Preserve `-fno-tree-loop-distribute-patterns` where it prevents freestanding code acquiring libc calls, or supply those functions. `-Oz`, per-object `-Os`, LTO/partition choices, `-fno-inline`, per-pass `-fno-*` lists, `-mtune=generic`, `-malign-data=abi`, `--param` peeling limits and 4-byte stack tuning were selected for legacy caps; retire after replacing caps and verifying the coherent ABI. `-fno-strict-aliasing` and `-fno-common` are correctness/toolchain choices, not automatically disposable size levers.

Commands below are captured output of `make -n -B --trace ...`, never executed. `-o build/flair_dac.stamp.mk` suppresses the parse-time included-makefile remake. Empty headroom-enumeration overrides only suppress irrelevant recursive database enumeration. Shell loops preserve exact input/link order; variables like `$objs` are runtime shell variables, not omitted make flags.

### dry-production-apps-notepad.txt
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/tenant/notepad.exe build/tenant/scrapbook.exe
# exit: 0
Makefile:12268: target 'build' does not exist
mkdir -p build
Makefile:76: update target 'build/.makefile.stamp' due to: Makefile os/samir/cmd/flow_verbs.def os/samir/cmd/mutate_verbs.def os/samir/cmd/proc_verbs.def os/samir/cmd/query_verbs.def os/samir/cmd/set_options.def harness/diff/fat_diff/doslook.mk harness/diff/fat_diff/xcopy_tree.mk harness/diff/fat_diff/copy_stamp.mk tools/serial_trace_pack.c os/milton/kmain_cold.h os/flair/chicago_packed.h harness/diff/fat_diff/shell_files.mk
mkdir -p build/ && touch build/.makefile.stamp
Makefile:10671: update target 'build/tenant/notepad.o0.elf' due to: os/apps/notepad/notepad.c os/flair/textedit.c os/apps/itext/te_fonts.c os/apps/tbx/tbx_rt.c os/apps/tbx/tbx.h os/apps/tbx/tbx_save.h spec/toolbox_gate.h spec/event_model.h spec/cp_dialog.h os/apps/tbx/tenant.ld os/flair/textedit.h os/flair/text.h os/flair/scrap.h spec/assets/geneva9.h spec/assets/chicago12.h spec/assets/geneva12.h spec/notepad/store.h spec/notepad/store.json
mkdir -p build/tenant/notepad
set -e; objs=; for s in os/apps/notepad/notepad.c os/flair/textedit.c os/apps/itext/te_fonts.c os/apps/tbx/tbx_rt.c; do o=build/tenant/notepad/$(basename $s .c).o; gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -fno-tree-loop-distribute-patterns -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-common -Ispec -Ios/apps/tbx -Ios/flair -Ios/flair/atkinson -Ispec/assets -ffunction-sections -fdata-sections  -c $s -o $o; objs="$objs $o"; done; ld -m elf_i386 --no-warn-rwx-segments --gc-sections -T os/apps/tbx/tenant.ld --defsym=TBX_BASE=0 -o build/tenant/notepad.o0.elf $objs; ld -m elf_i386 --no-warn-rwx-segments --gc-sections -T os/apps/tbx/tenant.ld --defsym=TBX_BASE=0x01010140 -o build/tenant/notepad.o1.elf $objs
Makefile:9650: update target 'build/mzlink' due to: tools/mzlink.c spec/initechmz_extended.h spec/samir/memory_contract.h
cc -std=c11 -Wall -Wextra -Werror -o build/mzlink tools/mzlink.c
Makefile:10671: update target 'build/tenant/notepad.exe' due to: build/tenant/notepad.o0.elf build/mzlink
objcopy -O binary build/tenant/notepad.o0.elf build/tenant/notepad.o0.bin
objcopy -O binary build/tenant/notepad.o1.elf build/tenant/notepad.o1.bin
ent=$(nm build/tenant/notepad.o0.elf | awk '$3=="tbx_entry"{print $1}'); [ "$ent" = 00000000 ] || { printf '!!! notepad.exe: tbx_entry is at %s, not module byte 0 (DEC-08a.2)\n' "$ent"; exit 1; }
bss=$(nm build/tenant/notepad.o0.elf | awk '$3=="__bss_len"{print $1}'); paras=$(( (0x$bss + 15) / 16 )); build/mzlink build/tenant/notepad.o0.bin build/tenant/notepad.exe --reloc-diff build/tenant/notepad.o1.bin 0x01010140 --entry 0 --minalloc $paras
sz=$(wc -c < build/tenant/notepad.exe); [ "$sz" -le 65536 ] || { printf '!!! notepad.exe is %s bytes: over the V1 tenant file cap TBX_TENANT_IMAGE_MAX (64 KiB)\n' "$sz"; exit 1; }
Makefile:10672: update target 'build/tenant/scrapbook.o0.elf' due to: os/apps/notepad/notepad.c os/flair/textedit.c os/apps/itext/te_fonts.c os/apps/tbx/tbx_rt.c os/apps/tbx/tbx.h os/apps/tbx/tbx_save.h spec/toolbox_gate.h spec/event_model.h spec/cp_dialog.h os/apps/tbx/tenant.ld os/flair/textedit.h os/flair/text.h os/flair/scrap.h spec/assets/geneva9.h spec/assets/chicago12.h spec/assets/geneva12.h spec/notepad/store.h spec/notepad/store.json
mkdir -p build/tenant/scrapbook
set -e; objs=; for s in os/apps/notepad/notepad.c os/flair/textedit.c os/apps/itext/te_fonts.c os/apps/tbx/tbx_rt.c; do o=build/tenant/scrapbook/$(basename $s .c).o; gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -fno-tree-loop-distribute-patterns -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-common -Ispec -Ios/apps/tbx -Ios/flair -Ios/flair/atkinson -Ispec/assets -ffunction-sections -fdata-sections -DSCRAPBOOK  -c $s -o $o; objs="$objs $o"; done; ld -m elf_i386 --no-warn-rwx-segments --gc-sections -T os/apps/tbx/tenant.ld --defsym=TBX_BASE=0 -o build/tenant/scrapbook.o0.elf $objs; ld -m elf_i386 --no-warn-rwx-segments --gc-sections -T os/apps/tbx/tenant.ld --defsym=TBX_BASE=0x01010140 -o build/tenant/scrapbook.o1.elf $objs
Makefile:10672: update target 'build/tenant/scrapbook.exe' due to: build/tenant/scrapbook.o0.elf build/mzlink
objcopy -O binary build/tenant/scrapbook.o0.elf build/tenant/scrapbook.o0.bin
objcopy -O binary build/tenant/scrapbook.o1.elf build/tenant/scrapbook.o1.bin
ent=$(nm build/tenant/scrapbook.o0.elf | awk '$3=="tbx_entry"{print $1}'); [ "$ent" = 00000000 ] || { printf '!!! scrapbook.exe: tbx_entry is at %s, not module byte 0 (DEC-08a.2)\n' "$ent"; exit 1; }
bss=$(nm build/tenant/scrapbook.o0.elf | awk '$3=="__bss_len"{print $1}'); paras=$(( (0x$bss + 15) / 16 )); build/mzlink build/tenant/scrapbook.o0.bin build/tenant/scrapbook.exe --reloc-diff build/tenant/scrapbook.o1.bin 0x01010140 --entry 0 --minalloc $paras
sz=$(wc -c < build/tenant/scrapbook.exe); [ "$sz" -le 65536 ] || { printf '!!! scrapbook.exe is %s bytes: over the V1 tenant file cap TBX_TENANT_IMAGE_MAX (64 KiB)\n' "$sz"; exit 1; }

# stderr

```

The two ELF links at TBX_BASE=0 and 0x01010140 produce two raw images; mzlink derives relocations by differencing, enforces entry 0, and writes BSS paragraphs into e_minalloc. The 65,536-byte tenant file check is legacy cap policy, while entry/relocation/BSS semantics remain necessary until the loader contract changes.

## 4. Host tests that travel with it

Counts by unique host program/main source (not mutant binaries, CLI cases, or gate names): **ORACLE-STRONG 1; UNIT 1; META 0; SIZE/LAYOUT 0.** META is reserved for factory/process/spec-inventory checks; input generators are listed as support sources rather than counted as tests. Some static classes are inferred from inspected test bodies; the basis is stated for each.

Every build variant name is listed; the exact canonical compilation is from its artifact-only dry run. Variant recipes and invocation assertions remain available in [gates-apps-notepad.md](gates-apps-notepad.md). The complete host dry-run archive is [dry-host.json](dry-host.json). Gate lists below use direct binary prerequisites/recipe references, avoiding giant transitive build closures that would misassign whole kernel test inventories.

### harness/proptest/test_notepad_route.c — UNIT

`harness/proptest/test_notepad_route.c:1`; R4.4 host routing oracle: the real tenant + shared TextEdit and real. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_notepad_route`, `build/test_scrapbook_route`.
Run/assert gate(s): `test-notepad-route`, `test-scrapbook-route`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/chicago12.h`, `spec/assets/font_linkage.h`, `spec/assets/geneva12.h`, `spec/assets/geneva9.h`, `spec/event_model.h`, `spec/notepad/store.h`, `spec/toolbox_gate.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/proptest/test_itext_route.c`, `spec/assets/chicago12.h`, `spec/assets/font_linkage.h`, `spec/assets/geneva12.h`, `spec/assets/geneva9.h`, `spec/event_model.h`, `spec/notepad/store.h`, `spec/toolbox_gate.h`.
Canonical `make -n` compile/link (`build/test_notepad_route`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/apps/tbx -Ispec -Ios/flair -Ios/flair/atkinson -Ispec/assets -o build/test_notepad_route harness/proptest/test_notepad_route.c os/flair/textedit.c os/flair/scrap.c
```

### tools/ppm_flair_notepad_check.c — ORACLE-STRONG

`tools/ppm_flair_notepad_check.c:1`; R4.4 independent composed-chrome pixel gate (factory C).. host pixel grader against captured/reference pixels or independent specimen; see exact CLI reference arguments.
Build target(s): `build/ppm_flair_notepad_check`.
Run/assert gate(s): `test-flair-notepad`, `test-flair-notepad-corrupt`, `test-flair-notepad-restart`, `test-flair-scrapbook`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/scrollbar_look_golden.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/scrollbar_look_golden.h`.
Canonical `make -n` compile/link (`build/ppm_flair_notepad_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec -o build/ppm_flair_notepad_check tools/ppm_flair_notepad_check.c -lpng
```

### Recipe-driven host suites and variants

All assigned legacy gate recipes, including compiler/FPC/mtools/script orchestration and mutants, are enumerated in [gates-apps-notepad.md](gates-apps-notepad.md). Host tier here is the declared legacy vector; some entries launch emulators transitively. The directory name proptest is not itself evidence of a property oracle.

## 5. Emulator gates worth keeping as smokes

**12 exclusive primary-owned entries in the 799-entry declared legacy emulator vector.** Also 12 defined test-* targets owned here have a static dependency closure reaching QEMU/Bochs (includes wrappers, mutants and names outside the active vector; not a runtime boot count). Cross-component smokes below are not double-counted. Exact primary attribution is in [gate-attribution.tsv](gate-attribution.tsv).

### test-flair-notepad

#  recipe to execute (from 'harness/emu/notepad.mk', line 18):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_notepad_data.img`, `build/flair_tenants_mut_iword_gate.img`.
Input trace `FLAIR_NOTEPAD_SPEC` at `spec/flair_notepad_traces.mk:8`:
```text
m-100:-100,m-100:-70,m-50:0,l1,l0,w300,m-50:-61,l1,w200,m40:73,l0,m0:-16,w600,ka,kl,kp,kh,ka,w200,kctrl-a,kctrl-c,w200,m100:100,m100:100,m100:63,m95:0,l1,l0,w300,kctrl-v,k2,w200,kctrl-w,w400,m-100:-100,m-100:-100,m-100:-100,m-100:-20,m-35:0,l1,w200,m40:73,l0,m0:-16,w600,m100:100,m100:100,m100:100,m100:14,m40:0,w300
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@grep -qx 'TENANT-SCRAP put n=5 count=1' build/flair_notepad.serial && grep -qx 'TENANT-SCRAP get n=5' build/flair_notepad.serial
@grep -q 's="2 of 8"' build/flair_notepad.serial && grep -q 's="alpha2"' build/flair_notepad.serial
@fsck.fat -n build/flair_notepad_data.img
```

### test-flair-scrapbook

#  recipe to execute (from 'harness/emu/notepad.mk', line 27):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_scrapbook_data.img`, `build/flair_tenants_mut_iword_gate.img`.
Input trace `FLAIR_SCRAPBOOK_SPEC` at `spec/flair_notepad_traces.mk:9`:
```text
m-100:-100,m-100:-70,m-50:0,l1,l0,w300,m-50:-61,l1,w200,m40:73,l0,m0:-16,w600,ka,kl,kp,kh,ka,w200,kctrl-a,kctrl-c,kctrl-w,w400,m-40:-57,l1,w200,m40:89,l0,m0:-16,w600,kctrl-v,w300,kctrl-v,w300,m100:28,m100:0,m100:0,m100:0,m6:0,l1,l0,w300,kctrl-c,kctrl-w,w400,m-100:-100,m-100:-1,m-100:0,m-100:0,m-46:0,l1,w200,m40:89,l0,m0:-16,w600,m100:100,m100:100,m100:98,m100:0,m40:0,w300
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@grep -q 's="1 of 2"' build/flair_scrapbook.serial && grep -q '^DA-LOADED SCRAPBK.DAT' build/flair_scrapbook.serial
@fsck.fat -n build/flair_scrapbook_data.img
```

The remaining legacy emulator entries cover app launch/registration, menu and keyboard/mouse input, file/save/error behavior, period pixels and focused mutant controls. Mutant twins establish sensitivity of the old assertion; convert useful faults to patches instead of maintaining extra resident kernels. The complete names/recipes are linked above.

## 6. Mutation hooks in production source

Measured count is conditional directive sites containing deliberately-wrong macro names (`MUT`, `MUTATE`, `MUTANT`, `BROKEN`, `DROP_WRITE`, `DOTRUNC`), not #else arms or runtime executions. One directive can mention two macros; distinct macro count is also provided. `#ifndef` controls count because the wrong behavior may be its absence. Non-mutation BOOT/profile selectors are excluded.

|File|Guard sites|Distinct macros|Macros and exact sites|
|---|---:|---:|---|
|`os/apps/notepad/notepad.c:1`|6|5|`os/apps/notepad/notepad.c:95` `DA_MUT_ACCEPT_CORRUPT`; `os/apps/notepad/notepad.c:124` `DA_MUT_OVERWRITE_DAMAGED`; `os/apps/notepad/notepad.c:311` `DA_MUT_PASTE_WRONG_PAGE`; `os/apps/notepad/notepad.c:318` `DA_MUT_PASTE_WRONG_PAGE`; `os/apps/notepad/notepad.c:328` `DA_MUT_SKIP_CLOSE_SAVE`; `os/apps/notepad/notepad.c:386` `DA_MUT_TURN_OFFBYONE`|

## 7. Coupling to fix during import

Concrete include/external-symbol evidence follows; line numbers refer to legacy-final. Cross-component public ABI includes in section 2 are expected dependencies, not violations.

- `Makefile:10669` → `os/flair/textedit.c + os/apps/itext/te_fonts.c`: shared TextEdit/font dependency is currently owned by another app; give it library ownership.

### App Contract use

INT 81h traps observed: `TBX_ALERT`, `TBX_CONTENTORIGIN`, `TBX_EXIT`, `TBX_FILE_CLOSE`, `TBX_FILE_CREATE_NEW`, `TBX_FILE_DELETE`, `TBX_FILE_OPEN`, `TBX_FILE_READ`, `TBX_FILE_RENAME`, `TBX_FILE_WRITE`, `TBX_FILLRECT`, `TBX_GETSCROLL`, `TBX_NEWWINDOW`, `TBX_REGISTER`, `TBX_SCRAP_GET`, `TBX_SCRAP_PUT`, `TBX_SETMBAR`, `TBX_SETSCROLL`, `TBX_SETWTITLE`, `TBX_TEXTDRAW`, `TBX_TRACKMOUSE`. Full AX/arity and per-app call-site matrix: [sdk.md](sdk.md).

## 8. Discard

Discard legacy size/headroom/overlay-layout policing and mutant kernel/image variants after preserving their functional assertions as ordinary tests or patches. Keep independent golden captures/readers/property suites; do not discard an oracle merely because its legacy target contains size or overlay.
Retire the 64-KiB EXE cap check and cap-only per-application codegen tuning. FONTS/CTENANT are specimens; keep them as test/example apps instead of treating them as user features.
