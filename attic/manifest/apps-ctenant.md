# apps-ctenant

Minimal C tenant and file-binding specimen; useful as the first SDK consumer and INT 81h smoke.

Evidence revision: `c5d18538049986bbbb27018c351405acf814ebbc` (`legacy-final`). Physical LOC includes comments and blank lines; tests and data are separate from production counts. Source ownership recommendations are explicitly inferences; paths, includes, guard counts and dry-run command text are measured.

## 1. Sources

2 production/source-support files; 208 physical lines. Test programs are in section 4.

|Legacy path|Lines|Role / disposition|
|---|---:|---|
|`os/apps/ctenant/ctenant.c:1`|130|- CTENANT.EXE, the smallest disk-launched|
|`os/apps/ctenant/files.c:1`|78|initech-tdnl.91: emulator fixture for the file ABI, authored, no local|

### Contracts, fixture/data and ancillary files owned here

Runtime files in the requested `os/`, `seed/`, `spec/`, `fixtures/` trees are assigned; os/README.md and os/apps/.gitkeep remain explicit unassigned placeholders in SUMMARY. `spec/*.mk` input traces are test data to preserve separately from their obsolete build syntax.

|File|Lines (text)|Role|
|---|---:|---|
|`spec/flair_ctenant_traces.mk:1`|71|- the LOCKED input trace of test-flair-ctenant|

## 2. Headers it needs from elsewhere

Literal include sites, including host-only conditional includes embedded in production TUs. PUBLIC INTERFACE means publish/move the contract or supported reusable library header; INTERNAL LEAK means replace a foreign implementation dependency. This is not an assertion that every conditional include is active in the production build.

|Include site|Spelling → resolved path|Classification|
|---|---|---|
|`os/apps/ctenant/ctenant.c:33`|`tbx.h` → `os/apps/tbx/tbx.h`|PUBLIC INTERFACE|
|`os/apps/ctenant/files.c:4`|`tbx.h` → `os/apps/tbx/tbx.h`|PUBLIC INTERFACE|

## 3. How it is built today

`-m32 -march=i386` pins the 32-bit 386-compatible ISA used on the emulated 486. Freestanding/no-libc linkage and entry/linker addresses remain required until packaging changes. `-fno-pic -fno-pie` agrees with flat placement and relocation derivation. Kernel `-mregparm=3 -mrtd -freg-struct-return` was introduced to save bytes, but it is now an ABI: keep C/assembly declarations and all linked translation units in agreement until changing both sides. Do not simply delete it from one file. INT 81h arguments remain explicit dword cdecl with `TBX_TRAP_ABI` exceptions. Preserve `-fno-tree-loop-distribute-patterns` where it prevents freestanding code acquiring libc calls, or supply those functions. `-Oz`, per-object `-Os`, LTO/partition choices, `-fno-inline`, per-pass `-fno-*` lists, `-mtune=generic`, `-malign-data=abi`, `--param` peeling limits and 4-byte stack tuning were selected for legacy caps; retire after replacing caps and verifying the coherent ABI. `-fno-strict-aliasing` and `-fno-common` are correctness/toolchain choices, not automatically disposable size levers.

Commands below are captured output of `make -n -B --trace ...`, never executed. `-o build/flair_dac.stamp.mk` suppresses the parse-time included-makefile remake. Empty headroom-enumeration overrides only suppress irrelevant recursive database enumeration. Shell loops preserve exact input/link order; variables like `$objs` are runtime shell variables, not omitted make flags.

### dry-production-apps-ctenant.txt
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/tenant/ctenant.exe
# exit: 0
Makefile:12268: target 'build' does not exist
mkdir -p build
Makefile:76: update target 'build/.makefile.stamp' due to: Makefile os/samir/cmd/flow_verbs.def os/samir/cmd/mutate_verbs.def os/samir/cmd/proc_verbs.def os/samir/cmd/query_verbs.def os/samir/cmd/set_options.def harness/diff/fat_diff/doslook.mk harness/diff/fat_diff/xcopy_tree.mk harness/diff/fat_diff/copy_stamp.mk tools/serial_trace_pack.c os/milton/kmain_cold.h os/flair/chicago_packed.h harness/diff/fat_diff/shell_files.mk
mkdir -p build/ && touch build/.makefile.stamp
Makefile:10646: update target 'build/tenant/ctenant.o0.elf' due to: os/apps/ctenant/ctenant.c os/apps/tbx/tbx_rt.c os/apps/tbx/tbx.h os/apps/tbx/tbx_save.h spec/toolbox_gate.h spec/event_model.h spec/cp_dialog.h os/apps/tbx/tenant.ld
mkdir -p build/tenant/ctenant
set -e; objs=; for s in os/apps/ctenant/ctenant.c os/apps/tbx/tbx_rt.c; do o=build/tenant/ctenant/$(basename $s .c).o; gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -fno-tree-loop-distribute-patterns -fno-asynchronous-unwind-tables -fno-unwind-tables -fno-common -Ispec -Ios/apps/tbx   -c $s -o $o; objs="$objs $o"; done; ld -m elf_i386 --no-warn-rwx-segments  -T os/apps/tbx/tenant.ld --defsym=TBX_BASE=0 -o build/tenant/ctenant.o0.elf $objs; ld -m elf_i386 --no-warn-rwx-segments  -T os/apps/tbx/tenant.ld --defsym=TBX_BASE=0x01010140 -o build/tenant/ctenant.o1.elf $objs
Makefile:9650: update target 'build/mzlink' due to: tools/mzlink.c spec/initechmz_extended.h spec/samir/memory_contract.h
cc -std=c11 -Wall -Wextra -Werror -o build/mzlink tools/mzlink.c
Makefile:10646: update target 'build/tenant/ctenant.exe' due to: build/tenant/ctenant.o0.elf build/mzlink
objcopy -O binary build/tenant/ctenant.o0.elf build/tenant/ctenant.o0.bin
objcopy -O binary build/tenant/ctenant.o1.elf build/tenant/ctenant.o1.bin
ent=$(nm build/tenant/ctenant.o0.elf | awk '$3=="tbx_entry"{print $1}'); [ "$ent" = 00000000 ] || { printf '!!! ctenant.exe: tbx_entry is at %s, not module byte 0 (DEC-08a.2)\n' "$ent"; exit 1; }
bss=$(nm build/tenant/ctenant.o0.elf | awk '$3=="__bss_len"{print $1}'); paras=$(( (0x$bss + 15) / 16 )); build/mzlink build/tenant/ctenant.o0.bin build/tenant/ctenant.exe --reloc-diff build/tenant/ctenant.o1.bin 0x01010140 --entry 0 --minalloc $paras
sz=$(wc -c < build/tenant/ctenant.exe); [ "$sz" -le 65536 ] || { printf '!!! ctenant.exe is %s bytes: over the V1 tenant file cap TBX_TENANT_IMAGE_MAX (64 KiB)\n' "$sz"; exit 1; }

# stderr

```

The two ELF links at TBX_BASE=0 and 0x01010140 produce two raw images; mzlink derives relocations by differencing, enforces entry 0, and writes BSS paragraphs into e_minalloc. The 65,536-byte tenant file check is legacy cap policy, while entry/relocation/BSS semantics remain necessary until the loader contract changes.

## 4. Host tests that travel with it

Counts by unique host program/main source (not mutant binaries, CLI cases, or gate names): **ORACLE-STRONG 0; UNIT 0; META 0; SIZE/LAYOUT 0.** META is reserved for factory/process/spec-inventory checks; input generators are listed as support sources rather than counted as tests. Some static classes are inferred from inspected test bodies; the basis is stated for each.

Every build variant name is listed; the exact canonical compilation is from its artifact-only dry run. Variant recipes and invocation assertions remain available in [gates-apps-ctenant.md](gates-apps-ctenant.md). The complete host dry-run archive is [dry-host.json](dry-host.json). Gate lists below use direct binary prerequisites/recipe references, avoiding giant transitive build closures that would misassign whole kernel test inventories.
No standalone host test program found for this component. This is a coverage gap, not evidence of zero assurance; relevant emulator graders and shared SDK tests are listed elsewhere.

### Recipe-driven host suites and variants

All assigned legacy gate recipes, including compiler/FPC/mtools/script orchestration and mutants, are enumerated in [gates-apps-ctenant.md](gates-apps-ctenant.md). Host tier here is the declared legacy vector; some entries launch emulators transitively. The directory name proptest is not itself evidence of a property oracle.

## 5. Emulator gates worth keeping as smokes

**2 exclusive primary-owned entries in the 799-entry declared legacy emulator vector.** Also 2 defined test-* targets owned here have a static dependency closure reaching QEMU/Bochs (includes wrappers, mutants and names outside the active vector; not a runtime boot count). Cross-component smokes below are not double-counted. Exact primary attribution is in [gate-attribution.tsv](gate-attribution.tsv).

### test-flair-ctenant

#  recipe to execute (from 'Makefile', line 23649):

Boot/data images named by direct prerequisites or expanded recipe: `build/flair_data.img`, `build/flair_tenants.img`, `build/flair_tenants_mut_hold.img`.
Input trace `FLAIR_CTENANT_DRAG_SPEC` at `spec/flair_ctenant_traces.mk:69`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:20,l1,l0,l1,l0,kq,m107:89,m0:89,l1,l0,m50:-120,l1,m25:25,m25:25,l0,m100:100,m100:100,m70:30
```
Input trace `FLAIR_CTENANT_REVEAL_SPEC` at `spec/flair_ctenant_traces.mk:71`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:20,l1,l0,l1,l0,kq,m107:89,m0:89,l1,l0,m50:-120,l1,m25:25,m25:25,l0,m-100:-100,m-30:-40,l1,m0:50,m0:50,l0,m100:20,m100:20,m10:0,l1,l0,m100:100,m90:100,m0:30
```
Input trace `FLAIR_CTENANT_SPEC` at `spec/flair_ctenant_traces.mk:30`:
```text
m100:-88,m100:-88,m80:0,l1,l0,l1,l0,m-100:10,m-100:10,m-100:10,m-100:8,m-77:0,l1,l0,l1,l0,m20:20,l1,l0,l1,l0,kq,m107:89,m0:89,l1,l0,m100:40,m100:40,m100:40,m70:40
```
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@$(call APPL_ALIVE,build/flair_ctenant.serial) || { printf '!!! test-flair-ctenant [1/4] FAIL: the desktop did not survive\n'; grep -E '^(PANIC|TENANT-|HALTED)' build/flair_ctenant.serial; exit 1; }
@$(call CTEN_SERIAL,build/flair_ctenant.serial) || { printf '!!! test-flair-ctenant [1/4] FAIL: CTENANT.EXE did not load, register, come forward and draw exactly its keyDown/mouseDown runs\n'; grep -E '^(TENANT-|FLAIR-DISPATCH|PANIC)' build/flair_ctenant.serial; exit 1; }
@printf '>>> test-flair-ctenant [1/4] C PATH: %s; the keyDown/mouseDown draws are the TENANT-GATE lines ctenant.c issues\n' "$$(grep -m1 '^TENANT-LOAD name=CTENANT' build/flair_ctenant.serial)"
	[ -n "$$k" ] && [ "$$k" -ge 15 ] && [ "$$c" -ge 400 ] && [ "$$m" -ge 400 ] || { printf '!!! test-flair-ctenant [2/4] FAIL: PRESENT -- the key/click draws are not on the screen (ink: TEXTDRAW %s, cells %s, click %s; want >=15, >=400, >=400)\n' "$$k" "$$c" "$$m"; exit 1; }; \
@f=build/flair_ctenant_drag; w=300,180; $(call APPL_ALIVE,$$f.serial) && $(call CTEN_SERIAL,$$f.serial) && $(call CTEN_DRAG_MOVED,$$f.serial) || { printf '!!! test-flair-ctenant [3/4] FAIL: DRAG -- the tenant did not run, draw its readouts and get its title dragged to (230,220)\n'; grep -E '^(TENANT-|FLAIR-DISPATCH|FLAIR-DRAG|PANIC)' $$f.serial; exit 1; }; \
	s=$$($(call CTEN_REPAINT_SERIAL,$$f.serial,$$w)) || { printf '!!! test-flair-ctenant [3/4] FAIL: DRAG serial -- an updateEvt after the input did not repaint all three readouts (%s)\n' "$$s"; grep -E '^(TENANT-|FLAIR-DRAG)' $$f.serial; exit 1; }; \
	p=$$($(call CTEN_REPAINT_PIXELS,$$f.ppm)) || { printf '!!! test-flair-ctenant [3/4] FAIL: DRAG pixels -- the readouts are not on the moved window (%s; want >=15, >=400, >=400)\n' "$$p"; exit 1; }; \
	printf '>>> test-flair-ctenant [3/4] DRAG: %s; %s; on screen at the moved window: %s\n' "$$(grep -m1 '^FLAIR-DRAG win' $$f.serial)" "$$s" "$$p"
@f=build/flair_ctenant_reveal; w=430,230; $(call APPL_ALIVE,$$f.serial) && $(call CTEN_SERIAL,$$f.serial) && $(call CTEN_DRAG_MOVED,$$f.serial) && $(call CTEN_APPS_OVER,$$f.serial) || { printf '!!! test-flair-ctenant [4/4] FAIL: REVEAL -- the tenant did not run, get dragged, and get covered by the APPS window at (40,180)\n'; grep -E '^(TENANT-|FLAIR-DISPATCH|FLAIR-DRAG|PANIC)' $$f.serial; exit 1; }; \
	s=$$($(call CTEN_REPAINT_SERIAL,$$f.serial,$$w)) || { printf '!!! test-flair-ctenant [4/4] FAIL: REVEAL serial -- an updateEvt after the input did not repaint all three readouts (%s)\n' "$$s"; grep -E '^(TENANT-|FLAIR-DRAG|FLAIR-DISPATCH)' $$f.serial; exit 1; }; \
	p=$$($(call CTEN_REPAINT_PIXELS,$$f.ppm)) || { printf '!!! test-flair-ctenant [4/4] FAIL: REVEAL pixels -- the readouts are not on the revealed window (%s; want >=15, >=400, >=400)\n' "$$p"; exit 1; }; \
```

The remaining legacy emulator entries cover app launch/registration, menu and keyboard/mouse input, file/save/error behavior, period pixels and focused mutant controls. Mutant twins establish sensitivity of the old assertion; convert useful faults to patches instead of maintaining extra resident kernels. The complete names/recipes are linked above.

## 6. Mutation hooks in production source

Measured count is conditional directive sites containing deliberately-wrong macro names (`MUT`, `MUTATE`, `MUTANT`, `BROKEN`, `DROP_WRITE`, `DOTRUNC`), not #else arms or runtime executions. One directive can mention two macros; distinct macro count is also provided. `#ifndef` controls count because the wrong behavior may be its absence. Non-mutation BOOT/profile selectors are excluded.

|File|Guard sites|Distinct macros|Macros and exact sites|
|---|---:|---:|---|
|`os/apps/ctenant/ctenant.c:1`|1|1|`os/apps/ctenant/ctenant.c:96` `CTENANT_MUT_NO_RETAIN`|
|`os/apps/ctenant/files.c:1`|0|0|none|

## 7. Coupling to fix during import

Concrete include/external-symbol evidence follows; line numbers refer to legacy-final. Cross-component public ABI includes in section 2 are expected dependencies, not violations.
No production include/declaration leak found by this static scan. Build/golden dependencies still apply; this does not prove absence of aliasing, absolute-address or assembly-level coupling.

### App Contract use

INT 81h traps observed: `TBX_DRAWCELLS`, `TBX_EXIT`, `TBX_FILE_CLOSE`, `TBX_FILE_CREATE`, `TBX_FILE_DELETE`, `TBX_FILE_OPEN`, `TBX_FILE_READ`, `TBX_FILE_SEEK`, `TBX_FILE_WRITE`, `TBX_FILLRECT`, `TBX_NEWWINDOW`, `TBX_REGISTER`, `TBX_SETMBAR`, `TBX_SETWTITLE`, `TBX_TEXTDRAW`. Full AX/arity and per-app call-site matrix: [sdk.md](sdk.md).

## 8. Discard

Discard legacy size/headroom/overlay-layout policing and mutant kernel/image variants after preserving their functional assertions as ordinary tests or patches. Keep independent golden captures/readers/property suites; do not discard an oracle merely because its legacy target contains size or overlay.
Retire the 64-KiB EXE cap check and cap-only per-application codegen tuning. FONTS/CTENANT are specimens; keep them as test/example apps instead of treating them as user features.
