# apps-calcda

Calculator desk accessory: pure core, view and tenant event glue. Its key/click parity and real Macintosh capture checks are the principal oracles.

Evidence revision: `c5d18538049986bbbb27018c351405acf814ebbc` (`legacy-final`). Physical LOC includes comments and blank lines; tests and data are separate from production counts. Source ownership recommendations are explicitly inferences; paths, includes, guard counts and dry-run command text are measured.

## 1. Sources

5 production/source-support files; 894 physical lines. Test programs are in section 4.

|Legacy path|Lines|Role / disposition|
|---|---:|---|
|`os/apps/calcda/calc_core.c:1`|457|- the Calculator engine (THE ARTIFACT; bead|
|`os/apps/calcda/calc_core.h:1`|142|- the Calculator desk accessory's ENGINE: the|
|`os/apps/calcda/calc_view.c:1`|119|- the Calculator's face (THE ARTIFACT; bead|
|`os/apps/calcda/calc_view.h:1`|53|- the Calculator's FACE: the layout's paint and|
|`os/apps/calcda/calcda.c:1`|123|- CALCULAT.EXE, the Calculator desk accessory (THE|

### Contracts, fixture/data and ancillary files owned here

Runtime files in the requested `os/`, `seed/`, `spec/`, `fixtures/` trees are assigned; os/README.md and os/apps/.gitkeep remain explicit unassigned placeholders in SUMMARY. `spec/*.mk` input traces are test data to preserve separately from their obsolete build syntax.

|File|Lines (text)|Role|
|---|---:|---|
|`spec/assets/calcda_geneva9.h:1`|70|- the Calculator readout + key-label strike|
|`spec/calcda/calcda_layout.h:1`|89|- the Calculator desk accessory's LAYOUT (LOCKED|
|`spec/calcda/calcda_vectors.tsv:1`|120|- LOCKED golden vectors of the Calculator desk|
|`spec/flair_calcda_traces.mk:1`|47|- the LOCKED input traces of test-flair-apple-|

## 2. Headers it needs from elsewhere

Literal include sites, including host-only conditional includes embedded in production TUs. PUBLIC INTERFACE means publish/move the contract or supported reusable library header; INTERNAL LEAK means replace a foreign implementation dependency. This is not an assertion that every conditional include is active in the production build.

|Include site|Spelling → resolved path|Classification|
|---|---|---|
|`os/apps/calcda/calc_view.c:8`|`calcda/calcda_layout.h` → `spec/calcda/calcda_layout.h`|PUBLIC INTERFACE|
|`os/apps/calcda/calc_view.c:9`|`calcda_geneva9.h` → `spec/assets/calcda_geneva9.h`|PUBLIC INTERFACE|
|`os/apps/calcda/calcda.c:32`|`tbx.h` → `os/apps/tbx/tbx.h`|PUBLIC INTERFACE|
|`os/apps/calcda/calcda.c:35`|`calcda/calcda_layout.h` → `spec/calcda/calcda_layout.h`|PUBLIC INTERFACE|

## 3. How it is built today

`-m32 -march=i386` pins the 32-bit 386-compatible ISA used on the emulated 486. Freestanding/no-libc linkage and entry/linker addresses remain required until packaging changes. `-fno-pic -fno-pie` agrees with flat placement and relocation derivation. Kernel `-mregparm=3 -mrtd -freg-struct-return` was introduced to save bytes, but it is now an ABI: keep C/assembly declarations and all linked translation units in agreement until changing both sides. Do not simply delete it from one file. INT 81h arguments remain explicit dword cdecl with `TBX_TRAP_ABI` exceptions. Preserve `-fno-tree-loop-distribute-patterns` where it prevents freestanding code acquiring libc calls, or supply those functions. `-Oz`, per-object `-Os`, LTO/partition choices, `-fno-inline`, per-pass `-fno-*` lists, `-mtune=generic`, `-malign-data=abi`, `--param` peeling limits and 4-byte stack tuning were selected for legacy caps; retire after replacing caps and verifying the coherent ABI. `-fno-strict-aliasing` and `-fno-common` are correctness/toolchain choices, not automatically disposable size levers.

Commands below are captured output of `make -n -B --trace ...`, never executed. `-o build/flair_dac.stamp.mk` suppresses the parse-time included-makefile remake. Empty headroom-enumeration overrides only suppress irrelevant recursive database enumeration. Shell loops preserve exact input/link order; variables like `$objs` are runtime shell variables, not omitted make flags.

### dry-production-apps-calcda.txt
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/tenant/calcda.exe
# exit: 0
Makefile:12268: target 'build' does not exist
mkdir -p build
Makefile:76: update target 'build/.makefile.stamp' due to: Makefile os/samir/cmd/flow_verbs.def os/samir/cmd/mutate_verbs.def os/samir/cmd/proc_verbs.def os/samir/cmd/query_verbs.def os/samir/cmd/set_options.def harness/diff/fat_diff/doslook.mk harness/diff/fat_diff/xcopy_tree.mk harness/diff/fat_diff/copy_stamp.mk tools/serial_trace_pack.c os/milton/kmain_cold.h os/flair/chicago_packed.h harness/diff/fat_diff/shell_files.mk
mkdir -p build/ && touch build/.makefile.stamp
Makefile:10695: update target 'build/tenant/calcda.o0.elf' due to: os/apps/calcda/calcda.c os/apps/calcda/calc_core.c os/apps/calcda/calc_view.c os/apps/tbx/tbx_rt.c os/apps/tbx/tbx.h os/apps/tbx/tbx_save.h spec/toolbox_gate.h spec/event_model.h spec/cp_dialog.h os/apps/tbx/tenant.ld os/apps/calcda/calc_core.h os/apps/calcda/calc_view.h spec/calcda/calcda_layout.h spec/assets/calcda_geneva9.h
mkdir -p build/tenant/calcda
set -e; objs=; for s in os/apps/calcda/calcda.c os/apps/calcda/calc_core.c os/apps/calcda/calc_view.c os/apps/tbx/tbx_rt.c; do o=build/tenant/calcda/$(basename $s .c).o; gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -fno-tree-loop-distribute-patterns -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-common -Ispec -Ios/apps/tbx -Ios/apps/calcda -Ispec/assets  -c $s -o $o; objs="$objs $o"; done; ld -m elf_i386 --no-warn-rwx-segments  -T os/apps/tbx/tenant.ld --defsym=TBX_BASE=0 -o build/tenant/calcda.o0.elf $objs; ld -m elf_i386 --no-warn-rwx-segments  -T os/apps/tbx/tenant.ld --defsym=TBX_BASE=0x01010140 -o build/tenant/calcda.o1.elf $objs
Makefile:9650: update target 'build/mzlink' due to: tools/mzlink.c spec/initechmz_extended.h spec/samir/memory_contract.h
cc -std=c11 -Wall -Wextra -Werror -o build/mzlink tools/mzlink.c
Makefile:10695: update target 'build/tenant/calcda.exe' due to: build/tenant/calcda.o0.elf build/mzlink
objcopy -O binary build/tenant/calcda.o0.elf build/tenant/calcda.o0.bin
objcopy -O binary build/tenant/calcda.o1.elf build/tenant/calcda.o1.bin
ent=$(nm build/tenant/calcda.o0.elf | awk '$3=="tbx_entry"{print $1}'); [ "$ent" = 00000000 ] || { printf '!!! calcda.exe: tbx_entry is at %s, not module byte 0 (DEC-08a.2)\n' "$ent"; exit 1; }
bss=$(nm build/tenant/calcda.o0.elf | awk '$3=="__bss_len"{print $1}'); paras=$(( (0x$bss + 15) / 16 )); build/mzlink build/tenant/calcda.o0.bin build/tenant/calcda.exe --reloc-diff build/tenant/calcda.o1.bin 0x01010140 --entry 0 --minalloc $paras
sz=$(wc -c < build/tenant/calcda.exe); [ "$sz" -le 65536 ] || { printf '!!! calcda.exe is %s bytes: over the V1 tenant file cap TBX_TENANT_IMAGE_MAX (64 KiB)\n' "$sz"; exit 1; }

# stderr

```

The two ELF links at TBX_BASE=0 and 0x01010140 produce two raw images; mzlink derives relocations by differencing, enforces entry 0, and writes BSS paragraphs into e_minalloc. The 65,536-byte tenant file check is legacy cap policy, while entry/relocation/BSS semantics remain necessary until the loader contract changes.

## 4. Host tests that travel with it

Counts by unique host program/main source (not mutant binaries, CLI cases, or gate names): **ORACLE-STRONG 2; UNIT 1; META 0; SIZE/LAYOUT 0.** META is reserved for factory/process/spec-inventory checks; input generators are listed as support sources rather than counted as tests. Some static classes are inferred from inspected test bodies; the basis is stated for each.

Every build variant name is listed; the exact canonical compilation is from its artifact-only dry run. Variant recipes and invocation assertions remain available in [gates-apps-calcda.md](gates-apps-calcda.md). The complete host dry-run archive is [dry-host.json](dry-host.json). Gate lists below use direct binary prerequisites/recipe references, avoiding giant transitive build closures that would misassign whole kernel test inventories.

### harness/proptest/test_calcda.c — ORACLE-STRONG

`harness/proptest/test_calcda.c:1`; - the Calculator desk accessory's HOST ORACLE. independent period corpus/reference consumer; paths and opt-out/error policy below.
Build target(s): `build/test_calcda`.
Run/assert gate(s): `test-calcda`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/proptest/test_calcda.c`, `spec/assets/calcda_geneva9.h`, `spec/calcda/calcda_layout.h`, `spec/calcda/calcda_vectors.tsv`.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Traveling test/helper source/header inputs: `spec/assets/calcda_geneva9.h`, `spec/calcda/calcda_layout.h`.
Canonical `make -n` compile/link (`build/test_calcda`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/apps/calcda -Ispec -Ispec/assets -o build/test_calcda harness/proptest/test_calcda.c os/apps/calcda/calc_core.c os/apps/calcda/calc_view.c -lpng -lm
```

### harness/proptest/test_calcda_route.c — UNIT

`harness/proptest/test_calcda_route.c:1`; initech-0xs6: real Calculator tenant over a recording Toolbox mock.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_calcda_close_only`, `build/test_calcda_route`.
Run/assert gate(s): `test-calcda-power-mutant`, `test-calcda-route`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/assets/calcda_geneva9.h`, `spec/calcda/calcda_layout.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/assets/calcda_geneva9.h`, `spec/calcda/calcda_layout.h`.
Canonical `make -n` compile/link (`build/test_calcda_close_only`):
```sh
cc -std=c11 -Wall -Wextra -Werror -DCALCDA_MUT_CLOSE_ONLY -Ios/apps/tbx -Ios/apps/calcda -Ispec -Ispec/assets -o build/test_calcda_close_only harness/proptest/test_calcda_route.c os/apps/calcda/calc_core.c os/apps/calcda/calc_view.c
```

### tools/ppm_flair_calcda_check.c — ORACLE-STRONG

`tools/ppm_flair_calcda_check.c:1`; - the PIXEL GATE of the Calculator desk. host pixel grader against captured/reference pixels or independent specimen; see exact CLI reference arguments.
Build target(s): `build/ppm_flair_calcda_check`.
Run/assert gate(s): `test-flair-apple-menu`, `test-flair-calcda`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): `SYSTEM7_DECOMP` → `../system7-decomp` (Makefile:39–43 defaults).
Canonical `make -n` compile/link (`build/ppm_flair_calcda_check`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -o build/ppm_flair_calcda_check tools/ppm_flair_calcda_check.c -lpng
```

### Recipe-driven host suites and variants

All assigned legacy gate recipes, including compiler/FPC/mtools/script orchestration and mutants, are enumerated in [gates-apps-calcda.md](gates-apps-calcda.md). Host tier here is the declared legacy vector; some entries launch emulators transitively. The directory name proptest is not itself evidence of a property oracle.

## 5. Emulator gates worth keeping as smokes

**5 exclusive primary-owned entries in the 799-entry declared legacy emulator vector.** Also 5 defined test-* targets owned here have a static dependency closure reaching QEMU/Bochs (includes wrappers, mutants and names outside the active vector; not a runtime boot count). Cross-component smokes below are not double-counted. Exact primary attribution is in [gate-attribution.tsv](gate-attribution.tsv).

### test-flair-calcda

#  recipe to execute (from 'Makefile', line 33848):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_tenants_mut_calcda_gate.img`.
Input trace `FLAIR_CALCDA_CLICKS_SPEC` at `spec/flair_calcda_traces.mk:47`:
```text
m-125:-85,m-125:-85,l1,l0,w400,m-50:-61,l1,w300,m40:41,w300,l0,w1000,m90:42,m90:42,m89:41,l1,l0,w300,m22:0,l1,l0,w300,m-44:44,l1,l0,w300,m0:-66,l1,l0,w300,m-79:124,m-78:123,l1,l0,w800
```
Input trace `FLAIR_CALCDA_KEYS_SPEC` at `spec/flair_calcda_traces.mk:42`:
```text
m-125:-85,m-125:-85,l1,l0,w400,m-50:-61,l1,w300,m40:41,w300,l0,w1000,k1,w150,k2,w150,kshift-equal,w150,k3,w150,kequal,w600
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@$(call CALCDA_UP,build/flair_calcda_keys.serial) && [ "$$(grep -c '^TENANT-EVT what=3$$' build/flair_calcda_keys.serial)" = 5 ] \
 || { printf '!!! test-flair-calcda FAIL [1/2] KEYS: Calculator not up, or not all five keys reached it\n'; grep -E '^(APPLE|TENANT-(REG|EVT|EXIT|CRASH))' build/flair_calcda_keys.serial; exit 1; }
@out=$$(build/ppm_flair_calcda_check window build/flair_calcda_keys.ppm "../system7-decomp/goldens/captures/s8_calc_12plus3eq.png"); rc=$$?; printf '%s\n' "$$out" | sed 's/^/    /'; \
 [ $$rc -eq 0 ] || { printf '!!! test-flair-calcda FAIL [1/2] KEYS: the window differs from s8_calc_12plus3eq.png\n'; exit 1; }
@$(call CALCDA_UP,build/flair_calcda_clicks.serial) && [ "$$(grep -c '^TENANT-EVT what=1$$' build/flair_calcda_clicks.serial)" = 4 ] \
 || { printf '!!! test-flair-calcda FAIL [2/2] CLICKS: Calculator not up, or not all four clicks reached it\n'; grep -E '^(APPLE|TENANT-(REG|EVT|EXIT|CRASH))' build/flair_calcda_clicks.serial; exit 1; }
@out=$$(build/ppm_flair_calcda_check window build/flair_calcda_clicks.ppm "../system7-decomp/goldens/captures/s8_calc_inactive.png"); rc=$$?; printf '%s\n' "$$out" | sed 's/^/    /'; \
 [ $$rc -eq 0 ] || { printf '!!! test-flair-calcda FAIL [2/2] CLICKS: the window differs from s8_calc_inactive.png\n'; exit 1; }
```

### test-flair-calcda-power

#  recipe to execute (from 'harness/emu/calcda_power.mk', line 27):

Boot/data images named by direct prerequisites or expanded recipe: `build/calc_power_rs_data.img`, `build/flair_data.img`, `build/flair_tenants_mut_calcda_gate.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@sh harness/emu/calcda_power_check.sh build/calc_power_sd.serial SHUTDOWN
@build/ppm_flair_shutdown_check safe build/calc_power_sd.ppm
@sh harness/emu/calcda_power_check.sh build/calc_power_rs.serial RESTART
@grep -q 'OK=1' build/calc_power_rs.report
```

The remaining legacy emulator entries cover app launch/registration, menu and keyboard/mouse input, file/save/error behavior, period pixels and focused mutant controls. Mutant twins establish sensitivity of the old assertion; convert useful faults to patches instead of maintaining extra resident kernels. The complete names/recipes are linked above.

## 6. Mutation hooks in production source

Measured count is conditional directive sites containing deliberately-wrong macro names (`MUT`, `MUTATE`, `MUTANT`, `BROKEN`, `DROP_WRITE`, `DOTRUNC`), not #else arms or runtime executions. One directive can mention two macros; distinct macro count is also provided. `#ifndef` controls count because the wrong behavior may be its absence. Non-mutation BOOT/profile selectors are excluded.

|File|Guard sites|Distinct macros|Macros and exact sites|
|---|---:|---:|---|
|`os/apps/calcda/calc_core.c:1`|6|4|`os/apps/calcda/calc_core.c:225` `CALCDA_MUT_FIXED_E3`; `os/apps/calcda/calc_core.c:313` `CALCDA_MUT_DIV0_ZERO`; `os/apps/calcda/calc_core.c:338` `CALCDA_MUT_PRECEDENCE`; `os/apps/calcda/calc_core.c:347` `CALCDA_MUT_PRECEDENCE`; `os/apps/calcda/calc_core.c:378` `CALCDA_MUT_PRECEDENCE`; `os/apps/calcda/calc_core.c:404` `CALCDA_MUT_ENTRY_CAP14`|
|`os/apps/calcda/calc_core.h:1`|1|1|`os/apps/calcda/calc_core.h:117` `CALCDA_MUT_PRECEDENCE`|
|`os/apps/calcda/calc_view.c:1`|2|2|`os/apps/calcda/calc_view.c:44` `CALCDA_MUT_READOUT_SHIFT`; `os/apps/calcda/calc_view.c:108` `CALCDA_MUT_KEY_CLICK_DIVERGE`|
|`os/apps/calcda/calc_view.h:1`|0|0|none|
|`os/apps/calcda/calcda.c:1`|1|1|`os/apps/calcda/calcda.c:49` `CALCDA_MUT_CLOSE_ONLY`|

## 7. Coupling to fix during import

Concrete include/external-symbol evidence follows; line numbers refer to legacy-final. Cross-component public ABI includes in section 2 are expected dependencies, not violations.
No production include/declaration leak found by this static scan. Build/golden dependencies still apply; this does not prove absence of aliasing, absolute-address or assembly-level coupling.

### App Contract use

INT 81h traps observed: `TBX_CONTENTORIGIN`, `TBX_EXIT`, `TBX_FILLRECT`, `TBX_NEWWINDOW`, `TBX_REGISTER`, `TBX_SETMBAR`, `TBX_SETWTITLE`. Full AX/arity and per-app call-site matrix: [sdk.md](sdk.md).

## 8. Discard

Discard legacy size/headroom/overlay-layout policing and mutant kernel/image variants after preserving their functional assertions as ordinary tests or patches. Keep independent golden captures/readers/property suites; do not discard an oracle merely because its legacy target contains size or overlay.
Retire the 64-KiB EXE cap check and cap-only per-application codegen tuning. FONTS/CTENANT are specimens; keep them as test/example apps instead of treating them as user features.
