# apps-cpanels

Control Panels desk accessory; depends on public settings/dialog/time/memory gate extension. Core service implementation remains in Toolbox `cp_service.h`, not this tenant.

Evidence revision: `c5d18538049986bbbb27018c351405acf814ebbc` (`legacy-final`). Physical LOC includes comments and blank lines; tests and data are separate from production counts. Source ownership recommendations are explicitly inferences; paths, includes, guard counts and dry-run command text are measured.

## 1. Sources

3 production/source-support files; 198 physical lines. Test programs are in section 4.

|Legacy path|Lines|Role / disposition|
|---|---:|---|
|`os/apps/cpanels/core.c:1`|112|R4.5, tdnl.19. Panel state/transactions; no hardware or file I/O here.|
|`os/apps/cpanels/cpanels.c:1`|69|one disk tenant, five Dialog Manager clients (tdnl.19).|
|`os/apps/cpanels/cpanels.h:1`|17|R4.5 disk DA. Ref: period-gui-suite.md sec 5, cp_dialog.h, record.json. */|

### Contracts, fixture/data and ancillary files owned here

Runtime files in the requested `os/`, `seed/`, `spec/`, `fixtures/` trees are assigned; os/README.md and os/apps/.gitkeep remain explicit unassigned placeholders in SUMMARY. `spec/*.mk` input traces are test data to preserve separately from their obsolete build syntax.

|File|Lines (text)|Role|
|---|---:|---|
|`spec/cpanels/record.json:1`|12|"bead": "initech-tdnl.19",|
|`spec/cpanels_record.h:1`|35|R4.5 / initech-tdnl.19, Rule 8. Authored fixed disk contract; JSON companion.|
|`spec/flair_cpanels_traces.mk:1`|31|R4.5 locked traces. Start (320,240). Apple Control Panels row4 centre(60,66).|

## 2. Headers it needs from elsewhere

Literal include sites, including host-only conditional includes embedded in production TUs. PUBLIC INTERFACE means publish/move the contract or supported reusable library header; INTERNAL LEAK means replace a foreign implementation dependency. This is not an assertion that every conditional include is active in the production build.

|Include site|Spelling → resolved path|Classification|
|---|---|---|
|`os/apps/cpanels/cpanels.c:5`|`tbx.h` → `os/apps/tbx/tbx.h`|PUBLIC INTERFACE|
|`os/apps/cpanels/cpanels.c:6`|`tbx_save.h` → `os/apps/tbx/tbx_save.h`|PUBLIC INTERFACE|
|`os/apps/cpanels/cpanels.h:4`|`cpanels_record.h` → `spec/cpanels_record.h`|PUBLIC INTERFACE|
|`os/apps/cpanels/cpanels.h:5`|`cp_dialog.h` → `spec/cp_dialog.h`|PUBLIC INTERFACE|

## 3. How it is built today

`-m32 -march=i386` pins the 32-bit 386-compatible ISA used on the emulated 486. Freestanding/no-libc linkage and entry/linker addresses remain required until packaging changes. `-fno-pic -fno-pie` agrees with flat placement and relocation derivation. Kernel `-mregparm=3 -mrtd -freg-struct-return` was introduced to save bytes, but it is now an ABI: keep C/assembly declarations and all linked translation units in agreement until changing both sides. Do not simply delete it from one file. INT 81h arguments remain explicit dword cdecl with `TBX_TRAP_ABI` exceptions. Preserve `-fno-tree-loop-distribute-patterns` where it prevents freestanding code acquiring libc calls, or supply those functions. `-Oz`, per-object `-Os`, LTO/partition choices, `-fno-inline`, per-pass `-fno-*` lists, `-mtune=generic`, `-malign-data=abi`, `--param` peeling limits and 4-byte stack tuning were selected for legacy caps; retire after replacing caps and verifying the coherent ABI. `-fno-strict-aliasing` and `-fno-common` are correctness/toolchain choices, not automatically disposable size levers.

Commands below are captured output of `make -n -B --trace ...`, never executed. `-o build/flair_dac.stamp.mk` suppresses the parse-time included-makefile remake. Empty headroom-enumeration overrides only suppress irrelevant recursive database enumeration. Shell loops preserve exact input/link order; variables like `$objs` are runtime shell variables, not omitted make flags.

### dry-production-apps-cpanels.txt
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/tenant/cpanels.exe
# exit: 0
Makefile:12268: target 'build' does not exist
mkdir -p build
Makefile:76: update target 'build/.makefile.stamp' due to: Makefile os/samir/cmd/flow_verbs.def os/samir/cmd/mutate_verbs.def os/samir/cmd/proc_verbs.def os/samir/cmd/query_verbs.def os/samir/cmd/set_options.def harness/diff/fat_diff/doslook.mk harness/diff/fat_diff/xcopy_tree.mk harness/diff/fat_diff/copy_stamp.mk tools/serial_trace_pack.c os/milton/kmain_cold.h os/flair/chicago_packed.h harness/diff/fat_diff/shell_files.mk
mkdir -p build/ && touch build/.makefile.stamp
Makefile:34639: update target 'build/tenant/cpanels.o0.elf' due to: os/apps/cpanels/cpanels.c os/apps/cpanels/core.c os/apps/tbx/tbx_rt.c os/apps/tbx/tbx.h os/apps/tbx/tbx_save.h spec/toolbox_gate.h spec/event_model.h spec/cp_dialog.h os/apps/tbx/tenant.ld os/apps/cpanels/cpanels.h spec/cpanels_record.h
mkdir -p build/tenant/cpanels
set -e; objs=; for s in os/apps/cpanels/cpanels.c os/apps/cpanels/core.c os/apps/tbx/tbx_rt.c; do o=build/tenant/cpanels/$(basename $s .c).o; gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -fno-tree-loop-distribute-patterns -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-common -Ispec -Ios/apps/tbx -Ios/apps/cpanels  -c $s -o $o; objs="$objs $o"; done; ld -m elf_i386 --no-warn-rwx-segments  -T os/apps/tbx/tenant.ld --defsym=TBX_BASE=0 -o build/tenant/cpanels.o0.elf $objs; ld -m elf_i386 --no-warn-rwx-segments  -T os/apps/tbx/tenant.ld --defsym=TBX_BASE=0x01010140 -o build/tenant/cpanels.o1.elf $objs
Makefile:9650: update target 'build/mzlink' due to: tools/mzlink.c spec/initechmz_extended.h spec/samir/memory_contract.h
cc -std=c11 -Wall -Wextra -Werror -o build/mzlink tools/mzlink.c
Makefile:34639: update target 'build/tenant/cpanels.exe' due to: build/tenant/cpanels.o0.elf build/mzlink
objcopy -O binary build/tenant/cpanels.o0.elf build/tenant/cpanels.o0.bin
objcopy -O binary build/tenant/cpanels.o1.elf build/tenant/cpanels.o1.bin
ent=$(nm build/tenant/cpanels.o0.elf | awk '$3=="tbx_entry"{print $1}'); [ "$ent" = 00000000 ] || { printf '!!! cpanels.exe: tbx_entry is at %s, not module byte 0 (DEC-08a.2)\n' "$ent"; exit 1; }
bss=$(nm build/tenant/cpanels.o0.elf | awk '$3=="__bss_len"{print $1}'); paras=$(( (0x$bss + 15) / 16 )); build/mzlink build/tenant/cpanels.o0.bin build/tenant/cpanels.exe --reloc-diff build/tenant/cpanels.o1.bin 0x01010140 --entry 0 --minalloc $paras
sz=$(wc -c < build/tenant/cpanels.exe); [ "$sz" -le 65536 ] || { printf '!!! cpanels.exe is %s bytes: over the V1 tenant file cap TBX_TENANT_IMAGE_MAX (64 KiB)\n' "$sz"; exit 1; }

# stderr

```

The two ELF links at TBX_BASE=0 and 0x01010140 produce two raw images; mzlink derives relocations by differencing, enforces entry 0, and writes BSS paragraphs into e_minalloc. The 65,536-byte tenant file check is legacy cap policy, while entry/relocation/BSS semantics remain necessary until the loader contract changes.

## 4. Host tests that travel with it

Counts by unique host program/main source (not mutant binaries, CLI cases, or gate names): **ORACLE-STRONG 1; UNIT 2; META 0; SIZE/LAYOUT 0.** META is reserved for factory/process/spec-inventory checks; input generators are listed as support sources rather than counted as tests. Some static classes are inferred from inspected test bodies; the basis is stated for each.

Every build variant name is listed; the exact canonical compilation is from its artifact-only dry run. Variant recipes and invocation assertions remain available in [gates-apps-cpanels.md](gates-apps-cpanels.md). The complete host dry-run archive is [dry-host.json](dry-host.json). Gate lists below use direct binary prerequisites/recipe references, avoiding giant transitive build closures that would misassign whole kernel test inventories.

### harness/proptest/test_cpanels.c — UNIT

`harness/proptest/test_cpanels.c:1`; initech-tdnl.19: independent routing/transaction oracle. Ref: user R4.5. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_cpanels`.
Run/assert gate(s): `test-cpanels`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/alert_icons.h`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`, `spec/assets/desk_icons.h`, `spec/assets/palette.h`, `spec/chicago12_nfnt_golden.h`, `spec/chrome_fidelity_golden.h`, `spec/chrome_metrics.h`, `spec/cp_dialog.h`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/grafport.h`, `spec/region_algebra.h`, `spec/window_record.h`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/render/alert_capture.h`, `harness/render/render.c`, `spec/assets/alert_icons.h`, `spec/assets/chicago12.h`, `spec/assets/color_canon.h`, `spec/assets/desk_icons.h`, `spec/assets/palette.h`, `spec/chicago12_nfnt_golden.h`, `spec/chrome_fidelity_golden.h`, `spec/chrome_metrics.h`, `spec/cp_dialog.h`, `spec/cpanels_record.h`, `spec/event_model.h`, `spec/grafport.h`, `spec/region_algebra.h`, `spec/window_record.h`, `tools/palette_extract.c`.
Canonical `make -n` compile/link (`build/test_cpanels`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -Ios/apps/cpanels -o build/test_cpanels harness/proptest/test_cpanels.c os/apps/cpanels/core.c os/flair/dialog.c os/flair/control.c os/flair/text.c os/flair/event.c os/flair/window.c os/flair/blitter.c os/flair/surface.c os/flair/chrome.c os/flair/heap.c os/flair/atkinson/region.c harness/render/render.c os/flair/flair_look.c -lpng
```

### tools/ppm_flair_cpanels_bars_check.c — UNIT

`tools/ppm_flair_cpanels_bars_check.c:1`; B05 / initech-c8w7: PRD 6.3, shell.h compositor order, DAS audit L027.. structural/behavioral regression against authored geometry or implementation captures; no independent period/reference differential established for this checker.
Build target(s): `build/ppm_flair_cpanels_bars_check`.
Run/assert gate(s): `test-flair-cpanels-bars-hide`, `test-flair-cpanels-bars-hide-exit`, `test-flair-cpanels-bars-mouse-exit`, `test-flair-cpanels-bars-mutant`, `test-flair-cpanels-bars-pattern`, `test-flair-cpanels-bars-pattern-exit`, `test-flair-cpanels-bars-show-exit`, `test-flair-cpanels-bars-sound-exit`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `fixtures/flair-cpanels-bars/cpanels.png`, `fixtures/flair-cpanels-bars/hello.png`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Canonical `make -n` compile/link (`build/ppm_flair_cpanels_bars_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -o build/ppm_flair_cpanels_bars_check tools/ppm_flair_cpanels_bars_check.c -lpng
```

### tools/ppm_flair_cpanels_check.c — ORACLE-STRONG

`tools/ppm_flair_cpanels_check.c:1`; Factory C / tdnl.19. Independent runtime Mac capture, no render header.. host pixel grader against captured/reference pixels or independent specimen; see exact CLI reference arguments.
Build target(s): `build/ppm_flair_cpanels_check`.
Run/assert gate(s): `test-flair-cpanels-bars-hide`, `test-flair-cpanels-bars-hide-exit`, `test-flair-cpanels-bars-mouse-exit`, `test-flair-cpanels-bars-pattern`, `test-flair-cpanels-bars-pattern-exit`, `test-flair-cpanels-bars-show-exit`, `test-flair-cpanels-bars-sound-exit`, `test-flair-cpanels-corrupt`, `test-flair-cpanels-date`, `test-flair-cpanels-date-mutant`, `test-flair-cpanels-memory`, `test-flair-cpanels-memory-mutant`, `test-flair-cpanels-mouse`, `test-flair-cpanels-mouse-mutant`, `test-flair-cpanels-patterns`, `test-flair-cpanels-patterns-mutant`, `test-flair-cpanels-persist`, `test-flair-cpanels-radio-mutant`, `test-flair-cpanels-sound`, `test-flair-cpanels-sound-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `fixtures/flair-cpanels-bars/cpanels.png`, `fixtures/flair-cpanels-bars/hello.png`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `harness/menubar_clock_oracle.c`, `harness/menubar_clock_oracle.h`.
Canonical `make -n` compile/link (`build/ppm_flair_cpanels_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Iharness -o build/ppm_flair_cpanels_check tools/ppm_flair_cpanels_check.c harness/menubar_clock_oracle.c -lpng
```

### Recipe-driven host suites and variants

All assigned legacy gate recipes, including compiler/FPC/mtools/script orchestration and mutants, are enumerated in [gates-apps-cpanels.md](gates-apps-cpanels.md). Host tier here is the declared legacy vector; some entries launch emulators transitively. The directory name proptest is not itself evidence of a property oracle.

## 5. Emulator gates worth keeping as smokes

**25 exclusive primary-owned entries in the 799-entry declared legacy emulator vector.** Also 35 defined test-* targets owned here have a static dependency closure reaching QEMU/Bochs (includes wrappers, mutants and names outside the active vector; not a runtime boot count). Cross-component smokes below are not double-counted. Exact primary attribution is in [gate-attribution.tsv](gate-attribution.tsv).

### test-flair-cpanels-date

#  recipe to execute (from 'Makefile', line 34652):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_tenants_mut_cpanels_gate.img`.
Input trace `FLAIR_CP_DATE_SPEC` at `spec/flair_cpanels_traces.mk:14`:
```text
m-100:-77,m-100:-77,m-100:-77,l1,w300,m40:57,w300,l0,w600,m100:53,m80:53,l1,l0,w200,m100:0,m30:0,l1,l0,w200,m100:60,m100:60,m100:60,m80:60,w400
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@grep -qx 'APPLE-LAUNCH name=CPANELS.EXE dir=APPS' build/cp_date.serial
@grep -qx 'CPANEL title=Date & Time' build/cp_date.serial
@build/ppm_flair_cpanels_check build/cp_date.ppm "../system7-decomp/goldens/captures/s8_alert_modal.png" "../system7-decomp/goldens/resources/NFNT_5478.bin" 
```

### test-flair-cpanels-patterns

#  recipe to execute (from 'Makefile', line 34653):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_tenants_mut_cpanels_gate.img`.
Input trace `FLAIR_CP_PATTERNS_SPEC` at `spec/flair_cpanels_traces.mk:15`:
```text
m-100:-77,m-100:-77,m-100:-77,l1,w300,m40:57,w300,l0,w600,m100:67,m80:67,l1,l0,w200,m-34:12,l1,l0,w200,m100:60,m100:60,m100:60,m80:60,w400
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@grep -qx 'APPLE-LAUNCH name=CPANELS.EXE dir=APPS' build/cp_patterns.serial
@grep -qx 'CPANEL title=Desktop Patterns' build/cp_patterns.serial
@build/ppm_flair_cpanels_check build/cp_patterns.ppm "../system7-decomp/goldens/captures/s8_alert_modal.png" "../system7-decomp/goldens/resources/NFNT_5478.bin" radio
```

### test-flair-cpanels-mouse

#  recipe to execute (from 'Makefile', line 34654):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_tenants_mut_cpanels_gate.img`.
Input trace `FLAIR_CP_MOUSE_SPEC` at `spec/flair_cpanels_traces.mk:16`:
```text
m-100:-77,m-100:-77,m-100:-77,l1,w300,m40:57,w300,l0,w600,m100:81,m80:81,l1,l0,w200,m-5:-8,l1,l0,w200,m100:60,m100:60,m100:60,m80:60,w400
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@grep -qx 'APPLE-LAUNCH name=CPANELS.EXE dir=APPS' build/cp_mouse.serial
@grep -qx 'CPANEL title=Mouse' build/cp_mouse.serial
@build/ppm_flair_cpanels_check build/cp_mouse.ppm "../system7-decomp/goldens/captures/s8_alert_modal.png" "../system7-decomp/goldens/resources/NFNT_5478.bin" radio
```

### test-flair-cpanels-sound

#  recipe to execute (from 'Makefile', line 34655):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_tenants_mut_cpanels_gate.img`.
Input trace `FLAIR_CP_SOUND_SPEC` at `spec/flair_cpanels_traces.mk:17`:
```text
m-100:-77,m-100:-77,m-100:-77,l1,w300,m40:57,w300,l0,w600,m100:95,m80:95,l1,l0,w200,m-5:-60,l1,l0,w200,m100:60,m100:60,m100:60,m80:60,w400
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@grep -qx 'APPLE-LAUNCH name=CPANELS.EXE dir=APPS' build/cp_sound.serial
@grep -qx 'CPANEL title=Sound' build/cp_sound.serial
@build/ppm_flair_cpanels_check build/cp_sound.ppm "../system7-decomp/goldens/captures/s8_alert_modal.png" "../system7-decomp/goldens/resources/NFNT_5478.bin" radio
```

### test-flair-cpanels-memory

#  recipe to execute (from 'Makefile', line 34656):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_tenants_mut_cpanels_gate.img`.
Input trace `FLAIR_CP_MEMORY_SPEC` at `spec/flair_cpanels_traces.mk:18`:
```text
m-100:-77,m-100:-77,m-100:-77,l1,w300,m40:57,w300,l0,w600,m100:109,m80:109,l1,l0,w200,m100:60,m100:60,m100:60,m80:60,w400
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@grep -qx 'APPLE-LAUNCH name=CPANELS.EXE dir=APPS' build/cp_memory.serial
@grep -qx 'CPANEL title=Memory' build/cp_memory.serial
@build/ppm_flair_cpanels_check build/cp_memory.ppm "../system7-decomp/goldens/captures/s8_alert_modal.png" "../system7-decomp/goldens/resources/NFNT_5478.bin" 
```

The remaining legacy emulator entries cover app launch/registration, menu and keyboard/mouse input, file/save/error behavior, period pixels and focused mutant controls. Mutant twins establish sensitivity of the old assertion; convert useful faults to patches instead of maintaining extra resident kernels. The complete names/recipes are linked above.

## 6. Mutation hooks in production source

Measured count is conditional directive sites containing deliberately-wrong macro names (`MUT`, `MUTATE`, `MUTANT`, `BROKEN`, `DROP_WRITE`, `DOTRUNC`), not #else arms or runtime executions. One directive can mention two macros; distinct macro count is also provided. `#ifndef` controls count because the wrong behavior may be its absence. Non-mutation BOOT/profile selectors are excluded.

|File|Guard sites|Distinct macros|Macros and exact sites|
|---|---:|---:|---|
|`os/apps/cpanels/core.c:1`|3|3|`os/apps/cpanels/core.c:73` `CP_MUT_CANCEL_WRITES`; `os/apps/cpanels/core.c:80` `CP_MUT_OK_NOTHING`; `os/apps/cpanels/core.c:83` `CP_MUT_CLOCK_NOT_APPLIED`|
|`os/apps/cpanels/cpanels.c:1`|0|0|none|
|`os/apps/cpanels/cpanels.h:1`|0|0|none|

## 7. Coupling to fix during import

Concrete include/external-symbol evidence follows; line numbers refer to legacy-final. Cross-component public ABI includes in section 2 are expected dependencies, not violations.
No production include/declaration leak found by this static scan. Build/golden dependencies still apply; this does not prove absence of aliasing, absolute-address or assembly-level coupling.

### App Contract use

INT 81h traps observed: `TBX_BEEP`, `TBX_CLOCK`, `TBX_DIALOG`, `TBX_EXIT`, `TBX_FILE_CLOSE`, `TBX_FILE_CREATE_NEW`, `TBX_FILE_DELETE`, `TBX_FILE_RENAME`, `TBX_FILE_WRITE`, `TBX_FILLRECT`, `TBX_MEMORY`, `TBX_NEWWINDOW`, `TBX_REGISTER`, `TBX_SETMBAR`, `TBX_SETTINGS`, `TBX_SETWTITLE`. Full AX/arity and per-app call-site matrix: [sdk.md](sdk.md).

## 8. Discard

Discard legacy size/headroom/overlay-layout policing and mutant kernel/image variants after preserving their functional assertions as ordinary tests or patches. Keep independent golden captures/readers/property suites; do not discard an oracle merely because its legacy target contains size or overlay.
Retire the 64-KiB EXE cap check and cap-only per-application codegen tuning. FONTS/CTENANT are specimens; keep them as test/example apps instead of treating them as user features.
