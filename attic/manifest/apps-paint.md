# apps-paint

PAINT.EXE presently supplies a Photoshop-like menu/canvas specimen. Several visible menu commands are deliberately disabled; the smoke proves disabled selection suppression and Quit, not a completed paint editor.

Evidence revision: `c5d18538049986bbbb27018c351405acf814ebbc` (`legacy-final`). Physical LOC includes comments and blank lines; tests and data are separate from production counts. Source ownership recommendations are explicitly inferences; paths, includes, guard counts and dry-run command text are measured.

## 1. Sources

1 production/source-support files; 109 physical lines. Test programs are in section 4.

|Legacy path|Lines|Role / disposition|
|---|---:|---|
|`os/apps/paint/paint.c:1`|109|- PAINT.EXE, InitechPaint (THE ARTIFACT; bead|

## 2. Headers it needs from elsewhere

Literal include sites, including host-only conditional includes embedded in production TUs. PUBLIC INTERFACE means publish/move the contract or supported reusable library header; INTERNAL LEAK means replace a foreign implementation dependency. This is not an assertion that every conditional include is active in the production build.

|Include site|Spelling → resolved path|Classification|
|---|---|---|
|`os/apps/paint/paint.c:26`|`tbx.h` → `os/apps/tbx/tbx.h`|PUBLIC INTERFACE|
|`os/apps/paint/paint.c:27`|`assets/menu_canon.h` → `spec/assets/menu_canon.h`|PUBLIC INTERFACE|

## 3. How it is built today

`-m32 -march=i386` pins the 32-bit 386-compatible ISA used on the emulated 486. Freestanding/no-libc linkage and entry/linker addresses remain required until packaging changes. `-fno-pic -fno-pie` agrees with flat placement and relocation derivation. Kernel `-mregparm=3 -mrtd -freg-struct-return` was introduced to save bytes, but it is now an ABI: keep C/assembly declarations and all linked translation units in agreement until changing both sides. Do not simply delete it from one file. INT 81h arguments remain explicit dword cdecl with `TBX_TRAP_ABI` exceptions. Preserve `-fno-tree-loop-distribute-patterns` where it prevents freestanding code acquiring libc calls, or supply those functions. `-Oz`, per-object `-Os`, LTO/partition choices, `-fno-inline`, per-pass `-fno-*` lists, `-mtune=generic`, `-malign-data=abi`, `--param` peeling limits and 4-byte stack tuning were selected for legacy caps; retire after replacing caps and verifying the coherent ABI. `-fno-strict-aliasing` and `-fno-common` are correctness/toolchain choices, not automatically disposable size levers.

Commands below are captured output of `make -n -B --trace ...`, never executed. `-o build/flair_dac.stamp.mk` suppresses the parse-time included-makefile remake. Empty headroom-enumeration overrides only suppress irrelevant recursive database enumeration. Shell loops preserve exact input/link order; variables like `$objs` are runtime shell variables, not omitted make flags.

### dry-production-apps-paint.txt
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/tenant/paint.exe
# exit: 0
Makefile:12268: target 'build' does not exist
mkdir -p build
Makefile:76: update target 'build/.makefile.stamp' due to: Makefile os/samir/cmd/flow_verbs.def os/samir/cmd/mutate_verbs.def os/samir/cmd/proc_verbs.def os/samir/cmd/query_verbs.def os/samir/cmd/set_options.def harness/diff/fat_diff/doslook.mk harness/diff/fat_diff/xcopy_tree.mk harness/diff/fat_diff/copy_stamp.mk tools/serial_trace_pack.c os/milton/kmain_cold.h os/flair/chicago_packed.h harness/diff/fat_diff/shell_files.mk
mkdir -p build/ && touch build/.makefile.stamp
harness/frameapps.mk:234: update target 'build/tenant/paint.o0.elf' due to: os/apps/paint/paint.c os/apps/tbx/tbx_rt.c os/apps/tbx/tbx.h os/apps/tbx/tbx_save.h spec/toolbox_gate.h spec/event_model.h spec/cp_dialog.h os/apps/tbx/tenant.ld spec/assets/menu_canon.h
mkdir -p build/tenant/paint
set -e; objs=; for s in os/apps/paint/paint.c os/apps/tbx/tbx_rt.c; do o=build/tenant/paint/$(basename $s .c).o; gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -fno-tree-loop-distribute-patterns -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-common -Ispec -Ios/apps/tbx   -c $s -o $o; objs="$objs $o"; done; ld -m elf_i386 --no-warn-rwx-segments  -T os/apps/tbx/tenant.ld --defsym=TBX_BASE=0 -o build/tenant/paint.o0.elf $objs; ld -m elf_i386 --no-warn-rwx-segments  -T os/apps/tbx/tenant.ld --defsym=TBX_BASE=0x01010140 -o build/tenant/paint.o1.elf $objs
Makefile:9650: update target 'build/mzlink' due to: tools/mzlink.c spec/initechmz_extended.h spec/samir/memory_contract.h
cc -std=c11 -Wall -Wextra -Werror -o build/mzlink tools/mzlink.c
harness/frameapps.mk:234: update target 'build/tenant/paint.exe' due to: build/tenant/paint.o0.elf build/mzlink
objcopy -O binary build/tenant/paint.o0.elf build/tenant/paint.o0.bin
objcopy -O binary build/tenant/paint.o1.elf build/tenant/paint.o1.bin
ent=$(nm build/tenant/paint.o0.elf | awk '$3=="tbx_entry"{print $1}'); [ "$ent" = 00000000 ] || { printf '!!! paint.exe: tbx_entry is at %s, not module byte 0 (DEC-08a.2)\n' "$ent"; exit 1; }
bss=$(nm build/tenant/paint.o0.elf | awk '$3=="__bss_len"{print $1}'); paras=$(( (0x$bss + 15) / 16 )); build/mzlink build/tenant/paint.o0.bin build/tenant/paint.exe --reloc-diff build/tenant/paint.o1.bin 0x01010140 --entry 0 --minalloc $paras
sz=$(wc -c < build/tenant/paint.exe); [ "$sz" -le 65536 ] || { printf '!!! paint.exe is %s bytes: over the V1 tenant file cap TBX_TENANT_IMAGE_MAX (64 KiB)\n' "$sz"; exit 1; }

# stderr

```

The two ELF links at TBX_BASE=0 and 0x01010140 produce two raw images; mzlink derives relocations by differencing, enforces entry 0, and writes BSS paragraphs into e_minalloc. The 65,536-byte tenant file check is legacy cap policy, while entry/relocation/BSS semantics remain necessary until the loader contract changes.

## 4. Host tests that travel with it

Counts by unique host program/main source (not mutant binaries, CLI cases, or gate names): **ORACLE-STRONG 1; UNIT 1; META 0; SIZE/LAYOUT 0.** META is reserved for factory/process/spec-inventory checks; input generators are listed as support sources rather than counted as tests. Some static classes are inferred from inspected test bodies; the basis is stated for each.

Every build variant name is listed; the exact canonical compilation is from its artifact-only dry run. Variant recipes and invocation assertions remain available in [gates-apps-paint.md](gates-apps-paint.md). The complete host dry-run archive is [dry-host.json](dry-host.json). Gate lists below use direct binary prerequisites/recipe references, avoiding giant transitive build closures that would misassign whole kernel test inventories.

### harness/proptest/test_paint_tenant.c — UNIT

`harness/proptest/test_paint_tenant.c:1`; InitechPaint routing, initech-tdnl.20 (bar carrier). Factory C, host: the. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_paint_tenant`.
Run/assert gate(s): `test-paint-tenant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/calc_ledger.h`, `spec/assets/calc_ledger.json`, `spec/assets/menu_canon.h`, `spec/toolbox_gate.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/assets/calc_ledger.h`, `spec/assets/menu_canon.h`.
Canonical `make -n` compile/link (`build/test_paint_tenant`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ios/apps/tbx -Ispec -Ispec/assets -o build/test_paint_tenant harness/proptest/test_paint_tenant.c
```

### tools/ppm_flair_paint_check.c — ORACLE-STRONG

`tools/ppm_flair_paint_check.c:1`; - InitechPaint screendump grader (factory C,. host pixel grader against captured/reference pixels or independent specimen; see exact CLI reference arguments.
Build target(s): `build/ppm_flair_paint_check`.
Run/assert gate(s): `test-flair-paint`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/chicago12_nfnt_golden.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/chicago12_nfnt_golden.h`.
Canonical `make -n` compile/link (`build/ppm_flair_paint_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec -o build/ppm_flair_paint_check tools/ppm_flair_paint_check.c
```

### Recipe-driven host suites and variants

All assigned legacy gate recipes, including compiler/FPC/mtools/script orchestration and mutants, are enumerated in [gates-apps-paint.md](gates-apps-paint.md). Host tier here is the declared legacy vector; some entries launch emulators transitively. The directory name proptest is not itself evidence of a property oracle.

## 5. Emulator gates worth keeping as smokes

**3 exclusive primary-owned entries in the 799-entry declared legacy emulator vector.** Also 3 defined test-* targets owned here have a static dependency closure reaching QEMU/Bochs (includes wrappers, mutants and names outside the active vector; not a runtime boot count). Cross-component smokes below are not double-counted. Exact primary attribution is in [gate-attribution.tsv](gate-attribution.tsv).

### test-flair-paint

#  recipe to execute (from 'harness/frameapps.mk', line 260):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_tenants.img`.
Input trace `FLAIR_PAINT_MENU_SPEC` at `spec/flair_calc_traces.mk:39`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m100:72,m56:0,l1,l0,l1,l0,m-100:-72,m-100:-72,m-59:0,l1,m42:0,m0:35,l0,m-42:-35,l1,m0:83,l0
```
Input trace `FLAIR_PAINT_SPEC` at `spec/flair_calc_traces.mk:38`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m100:72,m56:0,l1,l0,l1,l0,m0:100,m0:100,m0:86
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@$(call APPL_ALIVE,build/flair_paint.serial) && $(call PAINT_UP,build/flair_paint.serial) || { printf '!!! test-flair-paint FAIL: PAINT did not register / install its bar / title its canvas\n'; grep -E '^(TENANT|FLAIR-DISPATCH)' build/flair_paint.serial | head -20; exit 1; }
@build/ppm_flair_app_launch_check bar-photoshop build/flair_paint.ppm || { printf '!!! test-flair-paint FAIL: band 2 is not the canon File Edit Image Layer Select View Window Help bar\n'; exit 1; }
@build/ppm_flair_paint_check canvas build/flair_paint.ppm || { printf '!!! test-flair-paint FAIL: the canvas\n'; exit 1; }
@$(call APPL_ALIVE,build/flair_paint_menu.serial) && $(call PAINT_GRAYED,build/flair_paint_menu.serial) || { printf '!!! test-flair-paint FAIL: the grayed release delivered something, or Quit did not end PAINT\n'; grep -E '^(TENANT-EVT|TENANT-EXIT|FLAIR-MENU)' build/flair_paint_menu.serial; exit 1; }
@build/ppm_flair_paint_check menu build/flair_paint_menu.ppm || { printf '!!! test-flair-paint FAIL: the Edit panel is not all period-grayed\n'; exit 1; }
```

The remaining legacy emulator entries cover app launch/registration, menu and keyboard/mouse input, file/save/error behavior, period pixels and focused mutant controls. Mutant twins establish sensitivity of the old assertion; convert useful faults to patches instead of maintaining extra resident kernels. The complete names/recipes are linked above.

## 6. Mutation hooks in production source

Measured count is conditional directive sites containing deliberately-wrong macro names (`MUT`, `MUTATE`, `MUTANT`, `BROKEN`, `DROP_WRITE`, `DOTRUNC`), not #else arms or runtime executions. One directive can mention two macros; distinct macro count is also provided. `#ifndef` controls count because the wrong behavior may be its absence. Non-mutation BOOT/profile selectors are excluded.

|File|Guard sites|Distinct macros|Macros and exact sites|
|---|---:|---:|---|
|`os/apps/paint/paint.c:1`|2|2|`os/apps/paint/paint.c:31` `PAINT_MUT_ITEMS_ENABLED`; `os/apps/paint/paint.c:65` `PAINT_MUT_BAR_TEXT`|

## 7. Coupling to fix during import

Concrete include/external-symbol evidence follows; line numbers refer to legacy-final. Cross-component public ABI includes in section 2 are expected dependencies, not violations.
No production include/declaration leak found by this static scan. Build/golden dependencies still apply; this does not prove absence of aliasing, absolute-address or assembly-level coupling.

### App Contract use

INT 81h traps observed: `TBX_EXIT`, `TBX_FILLRECT`, `TBX_NEWWINDOW`, `TBX_REGISTER`, `TBX_SETMBAR`, `TBX_SETWTITLE`. Full AX/arity and per-app call-site matrix: [sdk.md](sdk.md).

## 8. Discard

Discard legacy size/headroom/overlay-layout policing and mutant kernel/image variants after preserving their functional assertions as ordinary tests or patches. Keep independent golden captures/readers/property suites; do not discard an oracle merely because its legacy target contains size or overlay.
Retire the 64-KiB EXE cap check and cap-only per-application codegen tuning. FONTS/CTENANT are specimens; keep them as test/example apps instead of treating them as user features.
