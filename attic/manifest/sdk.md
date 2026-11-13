# sdk

The legacy SDK has no single reusable host fake transport. Existing tests define their own `tbx_*` functions or `tbx_trap`; import those fakes as one SDK test transport. `tenantfx.asm` is an ABI conformance specimen, not a shipping user app.

Evidence revision: `c5d18538049986bbbb27018c351405acf814ebbc` (`legacy-final`). Physical LOC includes comments and blank lines; tests and data are separate from production counts. Source ownership recommendations are explicitly inferences; paths, includes, guard counts and dry-run command text are measured.

## 1. Sources

5 production/source-support files; 1,025 physical lines. Test programs are in section 4.

|Legacy path|Lines|Role / disposition|
|---|---:|---|
|`os/apps/tbx/tbx.h:1`|195|- the C binding to the Initech Toolbox Gate (INT 81h):|
|`os/apps/tbx/tbx_rt.c:1`|275|- the run-time half of the C Toolbox binding: the|
|`os/apps/tbx/tbx_save.h:1`|158|the tenant runtime's atomic document save (bead initech-mcep).|
|`os/apps/tbx/tenant.ld:1`|49|- the link of a disk-launched C application (factory|
|`os/apps/tenantfx.asm:1`|348|TENANTFX.EXE, the disk-shipped FIXTURE TENANT (THE ARTIFACT's|

## 2. Headers it needs from elsewhere

Literal include sites, including host-only conditional includes embedded in production TUs. PUBLIC INTERFACE means publish/move the contract or supported reusable library header; INTERNAL LEAK means replace a foreign implementation dependency. This is not an assertion that every conditional include is active in the production build.

|Include site|Spelling → resolved path|Classification|
|---|---|---|
|`os/apps/tbx/tbx.h:46`|`toolbox_gate.h` → `spec/toolbox_gate.h`|PUBLIC INTERFACE|
|`os/apps/tbx/tbx.h:47`|`event_model.h` → `spec/event_model.h`|PUBLIC INTERFACE|

## 3. How it is built today

`-m32 -march=i386` pins the 32-bit 386-compatible ISA used on the emulated 486. Freestanding/no-libc linkage and entry/linker addresses remain required until packaging changes. `-fno-pic -fno-pie` agrees with flat placement and relocation derivation. Kernel `-mregparm=3 -mrtd -freg-struct-return` was introduced to save bytes, but it is now an ABI: keep C/assembly declarations and all linked translation units in agreement until changing both sides. Do not simply delete it from one file. INT 81h arguments remain explicit dword cdecl with `TBX_TRAP_ABI` exceptions. Preserve `-fno-tree-loop-distribute-patterns` where it prevents freestanding code acquiring libc calls, or supply those functions. `-Oz`, per-object `-Os`, LTO/partition choices, `-fno-inline`, per-pass `-fno-*` lists, `-mtune=generic`, `-malign-data=abi`, `--param` peeling limits and 4-byte stack tuning were selected for legacy caps; retire after replacing caps and verifying the coherent ABI. `-fno-strict-aliasing` and `-fno-common` are correctness/toolchain choices, not automatically disposable size levers.

Commands below are captured output of `make -n -B --trace ...`, never executed. `-o build/flair_dac.stamp.mk` suppresses the parse-time included-makefile remake. Empty headroom-enumeration overrides only suppress irrelevant recursive database enumeration. Shell loops preserve exact input/link order; variables like `$objs` are runtime shell variables, not omitted make flags.

### dry-production-sdk.txt
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/tenant/ctenant.exe build/tenantfx.exe
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
Makefile:10532: update target 'build/tenantfx.o0.bin' due to: os/apps/tenantfx.asm
nasm -f bin -DORG=0  os/apps/tenantfx.asm -o build/tenantfx.o0.bin
Makefile:10532: update target 'build/tenantfx.o1.bin' due to: os/apps/tenantfx.asm
nasm -f bin -DORG=0x01010110  os/apps/tenantfx.asm -o build/tenantfx.o1.bin
Makefile:10532: update target 'build/tenantfx.exe' due to: build/tenantfx.o0.bin build/tenantfx.o1.bin build/mzlink
build/mzlink build/tenantfx.o0.bin build/tenantfx.exe --reloc-diff build/tenantfx.o1.bin 0x01010110 --entry 0

# stderr

```

The two ELF links at TBX_BASE=0 and 0x01010140 produce two raw images; mzlink derives relocations by differencing, enforces entry 0, and writes BSS paragraphs into e_minalloc. The 65,536-byte tenant file check is legacy cap policy, while entry/relocation/BSS semantics remain necessary until the loader contract changes.

## 4. Host tests that travel with it

Counts by unique host program/main source (not mutant binaries, CLI cases, or gate names): **ORACLE-STRONG 0; UNIT 0; META 0; SIZE/LAYOUT 0.** META is reserved for factory/process/spec-inventory checks; input generators are listed as support sources rather than counted as tests. Some static classes are inferred from inspected test bodies; the basis is stated for each.

Every build variant name is listed; the exact canonical compilation is from its artifact-only dry run. Variant recipes and invocation assertions remain available in [gates-sdk.md](gates-sdk.md). The complete host dry-run archive is [dry-host.json](dry-host.json). Gate lists below use direct binary prerequisites/recipe references, avoiding giant transitive build closures that would misassign whole kernel test inventories.
No standalone host test program found for this component. This is a coverage gap, not evidence of zero assurance; relevant emulator graders and shared SDK tests are listed elsewhere.

### Recipe-driven host suites and variants

All assigned legacy gate recipes, including compiler/FPC/mtools/script orchestration and mutants, are enumerated in [gates-sdk.md](gates-sdk.md). Host tier here is the declared legacy vector; some entries launch emulators transitively. The directory name proptest is not itself evidence of a property oracle.

## 5. Emulator gates worth keeping as smokes

**0 exclusive primary-owned entries in the 799-entry declared legacy emulator vector.** Also 0 defined test-* targets owned here have a static dependency closure reaching QEMU/Bochs (includes wrappers, mutants and names outside the active vector; not a runtime boot count). Cross-component smokes below are not double-counted. Exact primary attribution is in [gate-attribution.tsv](gate-attribution.tsv).

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
|`os/apps/tbx/tbx.h:1`|0|0|none|
|`os/apps/tbx/tbx_rt.c:1`|0|0|none|
|`os/apps/tbx/tbx_save.h:1`|4|3|`os/apps/tbx/tbx_save.h:57` `TBX_MUT_SAVE_IN_PLACE`; `os/apps/tbx/tbx_save.h:86` `TBX_MUT_SAVE_TEMP_CREATE`; `os/apps/tbx/tbx_save.h:112` `TBX_MUT_SAVE_ONE_WRITE`; `os/apps/tbx/tbx_save.h:118` `TBX_MUT_SAVE_ONE_WRITE`|
|`os/apps/tbx/tenant.ld:1`|0|0|none|
|`os/apps/tenantfx.asm:1`|0|0|none|

## 7. Coupling to fix during import

Concrete include/external-symbol evidence follows; line numbers refer to legacy-final. Cross-component public ABI includes in section 2 are expected dependencies, not violations.

- `os/apps/tbx/tbx.h:50` → `TBX_IN_IMAGE`: loaded-PROGBITS requirement inflates disk image; expose readable/writable span contract.
- `os/apps/tbx/tbx_rt.c:85` → `cdecl/regparm(0) asm trap shim`: runtime must preserve trap ABI despite i123 whole-image register ABI.

### Existing host transport implementations to consolidate

- `harness/proptest/test_calc_file_copy.c:19`: int32_t tbx_register(void) { return TBX_OK; }
- `harness/proptest/test_calc_tenant.c:27`: int32_t tbx_register(void) { return 0; }
- `harness/proptest/test_calcda_route.c:13`: int32_t tbx_register(void) { return TBX_OK; }
- `harness/proptest/test_i123_quit.c:85`: int32_t tbx_register(void) { return TBX_OK; }
- `harness/proptest/test_i123_quit.c:109`: int32_t tbx_trap(uint32_t c, uint32_t n, const uint32_t *a)
- `harness/proptest/test_itext_route.c:80`: int32_t tbx_register(void) { return TBX_OK; }
- `harness/proptest/test_itext_route.c:103`: int32_t tbx_trap(uint32_t c, uint32_t n, const uint32_t *a) { (void)c; (void)n; (void)a; return TBX_ERR_BADCODE; }
- `harness/proptest/test_iword_face.c:100`: int32_t tbx_register(void) { return TBX_OK; }
- `harness/proptest/test_iword_face.c:124`: int32_t tbx_trap(uint32_t c, uint32_t n, const uint32_t *a) { (void)c; (void)n; (void)a; return TBX_ERR_BADCODE; }
- `harness/proptest/test_notepad_route.c:37`: int32_t tbx_register(void) { return TBX_OK; }
- `harness/proptest/test_notepad_route.c:61`: int32_t tbx_trap(uint32_t c, uint32_t n, const uint32_t *a) { (void)c; (void)n; (void)a; return TBX_ERR_BADCODE; }
- `harness/proptest/test_paint_tenant.c:20`: int32_t tbx_register(void) { return TBX_OK; }

### App Contract: INT 81h function inventory

Every defined function number from toolbox_gate.h plus its cp_dialog.h extension appears below. Arity is dwords read by the gate; reserved/dissolved numbers have no callable arity. Event delivery codes (TBX_EVT_*) are not extra callable traps. Apps listed are static call-site users, including tbx_save.h helper closure. Explicit mutation-guard arms are excluded; other feature branches are unioned (Notepad/Scrapbook). Runtime execution coverage is not claimed.

|AX|Name|Arity / status|Apps with call sites|Definition|
|---|---|---|---|---|
|0x0001|TBX_REGISTER|1|calc, calcda, cpanels, itext, notepad, paint, fonts, ctenant, i123, iword|`spec/toolbox_gate.h:156`|
|0x0002|TBX_KEEPRESIDENT|DISSOLVED → BADCODE|none statically observed|`spec/toolbox_gate.h:157`|
|0x0010|TBX_EVENTCHANNEL|RESERVED callback delivery, not callable|none statically observed|`spec/toolbox_gate.h:159`|
|0x0020|TBX_NEWWINDOW|5|calc, calcda, cpanels, itext, notepad, paint, fonts, ctenant, i123, iword|`spec/toolbox_gate.h:163`|
|0x0021|TBX_DISPOSEWINDOW|RESERVED → BADCODE|none statically observed|`spec/toolbox_gate.h:164`|
|0x0022|TBX_SETWTITLE|2|calc, calcda, cpanels, itext, notepad, paint, fonts, ctenant, i123, iword|`spec/toolbox_gate.h:165`|
|0x0023|TBX_CONTENTORIGIN|1|calcda, notepad, i123|`spec/toolbox_gate.h:166`|
|0x0024|TBX_CONTENT_SIZE|1|i123|`spec/toolbox_gate.h:170`|
|0x0030|TBX_TEXTDRAW|6|calc, itext, notepad, fonts, ctenant|`spec/toolbox_gate.h:201`|
|0x0031|TBX_FILLRECT|6|calc, calcda, cpanels, itext, notepad, paint, fonts, ctenant, i123, iword|`spec/toolbox_gate.h:202`|
|0x0032|TBX_FRAMERECT|RESERVED → BADCODE|none statically observed|`spec/toolbox_gate.h:203`|
|0x0033|TBX_DRAWCELLS|6|ctenant, i123, iword|`spec/toolbox_gate.h:204`|
|0x0050|TBX_SETMBAR|1|calc, calcda, cpanels, itext, notepad, paint, fonts, ctenant, i123, iword|`spec/toolbox_gate.h:206`|
|0x0051|TBX_MENUSELECT|DISSOLVED → BADCODE|none statically observed|`spec/toolbox_gate.h:210`|
|0x0052|TBX_DRAWMENUBAR|0|none statically observed|`spec/toolbox_gate.h:216`|
|0x0070|TBX_EXIT|1|calc, calcda, cpanels, itext, notepad, paint, fonts, ctenant, i123, iword|`spec/toolbox_gate.h:218`|
|0x0080|TBX_FILE_CREATE|1|calc, ctenant|`spec/toolbox_gate.h:222`|
|0x0081|TBX_FILE_OPEN|2|calc, itext, notepad, ctenant, i123, iword|`spec/toolbox_gate.h:223`|
|0x0082|TBX_FILE_READ|3|itext, notepad, ctenant, i123, iword|`spec/toolbox_gate.h:224`|
|0x0083|TBX_FILE_WRITE|3|calc, cpanels, itext, notepad, ctenant, i123, iword|`spec/toolbox_gate.h:225`|
|0x0084|TBX_FILE_SEEK|3|ctenant, i123, iword|`spec/toolbox_gate.h:226`|
|0x0085|TBX_FILE_CLOSE|1|calc, cpanels, itext, notepad, ctenant, i123, iword|`spec/toolbox_gate.h:227`|
|0x0086|TBX_FILE_DELETE|1|calc, cpanels, itext, notepad, ctenant, i123, iword|`spec/toolbox_gate.h:228`|
|0x0087|TBX_FILE_FIND|4|iword|`spec/toolbox_gate.h:230`|
|0x0088|TBX_DISK_FREE|0|iword|`spec/toolbox_gate.h:232`|
|0x0089|TBX_FILE_RENAME|2|cpanels, itext, notepad, i123, iword|`spec/toolbox_gate.h:236`|
|0x008A|TBX_FILE_CREATE_NEW|1|cpanels, itext, notepad, i123, iword|`spec/toolbox_gate.h:237`|
|0x0090|TBX_ALERT|4|calc, itext, notepad, i123|`spec/toolbox_gate.h:241`|
|0x0091|TBX_FILECOPY|3|calc|`spec/toolbox_gate.h:248`|
|0x0092|TBX_DIALOG|6|cpanels|`spec/cp_dialog.h:9`|
|0x0093|TBX_SETTINGS|2|cpanels|`spec/cp_dialog.h:10`|
|0x0094|TBX_CLOCK|2|cpanels, i123, iword|`spec/cp_dialog.h:11`|
|0x0095|TBX_MEMORY|1|cpanels|`spec/cp_dialog.h:12`|
|0x0096|TBX_BEEP|0|cpanels, i123|`spec/cp_dialog.h:13`|
|0x00A0|TBX_SCRAP_PUT|2|itext, notepad|`spec/toolbox_gate.h:755`|
|0x00A1|TBX_SCRAP_GET|2|itext, notepad|`spec/toolbox_gate.h:756`|
|0x00B0|TBX_SFGETFILE|1|itext|`spec/toolbox_gate.h:793`|
|0x00B1|TBX_SFPUTFILE|3|itext|`spec/toolbox_gate.h:794`|
|0x00C0|TBX_TRACKMOUSE|0|itext, notepad|`spec/toolbox_gate.h:820`|
|0x00C1|TBX_SETSCROLL|3|itext, notepad|`spec/toolbox_gate.h:854`|
|0x00C2|TBX_GETSCROLL|1|itext, notepad|`spec/toolbox_gate.h:855`|
|0x00C3|TBX_GLOBALTOLOCAL|3|itext|`spec/toolbox_gate.h:856`|

### Exact per-app trap call sites

#### apps-calc

- `TBX_ALERT`: `os/apps/calc/calc.c:210` `tbx_alert`; `os/apps/calc/calc.c:253` `tbx_alert`; `os/apps/calc/calc.c:268` `tbx_alert`
- `TBX_EXIT`: `os/apps/calc/calc.c:170` `tbx_exit`; `os/apps/calc/calc.c:172` `tbx_exit`; `os/apps/calc/calc.c:266` `tbx_exit`; `os/apps/calc/calc.c:274` `tbx_exit`; `os/apps/calc/calc.c:293` `tbx_exit`
- `TBX_FILECOPY`: `os/apps/calc/calc.c:244` `tbx_file_copy`
- `TBX_FILE_CLOSE`: `os/apps/calc/calc.c:215` `tbx_file_close`; `os/apps/calc/calc.c:240` `tbx_file_close`
- `TBX_FILE_CREATE`: `os/apps/calc/calc.c:237` `tbx_file_create`
- `TBX_FILE_DELETE`: `os/apps/calc/calc.c:240` `tbx_file_delete`; `os/apps/calc/calc.c:292` `tbx_file_delete`
- `TBX_FILE_OPEN`: `os/apps/calc/calc.c:213` `tbx_file_open`
- `TBX_FILE_WRITE`: `os/apps/calc/calc.c:240` `tbx_file_write`
- `TBX_FILLRECT`: `os/apps/calc/calc.c:91` `tbx_fill_rect`; `os/apps/calc/calc.c:94` `tbx_fill_rect`; `os/apps/calc/calc.c:99` `tbx_fill_rect`; `os/apps/calc/calc.c:101` `tbx_fill_rect`; `os/apps/calc/calc.c:135` `tbx_fill_rect`; `os/apps/calc/calc.c:143` `tbx_fill_rect`
- `TBX_NEWWINDOW`: `os/apps/calc/calc.c:169` `tbx_new_window`; `os/apps/calc/calc.c:171` `tbx_new_window`
- `TBX_REGISTER`: `os/apps/calc/calc.c:161` `tbx_register`
- `TBX_SETMBAR`: `os/apps/calc/calc.c:168` `tbx_set_mbar`
- `TBX_SETWTITLE`: `os/apps/calc/calc.c:170` `tbx_set_wtitle`; `os/apps/calc/calc.c:172` `tbx_set_wtitle`
- `TBX_TEXTDRAW`: `os/apps/calc/calc.c:80` `tbx_draw_text`

#### apps-calcda

- `TBX_CONTENTORIGIN`: `os/apps/calcda/calcda.c:105` `tbx_content_origin`
- `TBX_EXIT`: `os/apps/calcda/calcda.c:118` `tbx_exit`
- `TBX_FILLRECT`: `os/apps/calcda/calcda.c:75` `tbx_fill_rect`
- `TBX_NEWWINDOW`: `os/apps/calcda/calcda.c:90` `tbx_new_window`
- `TBX_REGISTER`: `os/apps/calcda/calcda.c:87` `tbx_register`
- `TBX_SETMBAR`: `os/apps/calcda/calcda.c:88` `tbx_set_mbar`
- `TBX_SETWTITLE`: `os/apps/calcda/calcda.c:94` `tbx_set_wtitle`

#### apps-cpanels

- `TBX_BEEP`: `os/apps/cpanels/cpanels.c:29` `direct constant / trap use`
- `TBX_CLOCK`: `os/apps/cpanels/cpanels.c:18` `direct constant / trap use`
- `TBX_DIALOG`: `os/apps/cpanels/cpanels.c:35` `direct constant / trap use`
- `TBX_EXIT`: `os/apps/cpanels/cpanels.c:59` `tbx_exit`; `os/apps/cpanels/cpanels.c:66` `tbx_exit`
- `TBX_FILE_CLOSE`: `os/apps/cpanels/cpanels.c:21` `tbx_file_save`
- `TBX_FILE_CREATE_NEW`: `os/apps/cpanels/cpanels.c:21` `tbx_file_save`
- `TBX_FILE_DELETE`: `os/apps/cpanels/cpanels.c:21` `tbx_file_save`
- `TBX_FILE_RENAME`: `os/apps/cpanels/cpanels.c:21` `tbx_file_save`
- `TBX_FILE_WRITE`: `os/apps/cpanels/cpanels.c:21` `tbx_file_save`
- `TBX_FILLRECT`: `os/apps/cpanels/cpanels.c:44` `tbx_fill_rect`
- `TBX_MEMORY`: `os/apps/cpanels/cpanels.c:53` `direct constant / trap use`
- `TBX_NEWWINDOW`: `os/apps/cpanels/cpanels.c:54` `tbx_new_window`
- `TBX_REGISTER`: `os/apps/cpanels/cpanels.c:50` `tbx_register`
- `TBX_SETMBAR`: `os/apps/cpanels/cpanels.c:54` `tbx_set_mbar`
- `TBX_SETTINGS`: `os/apps/cpanels/cpanels.c:17` `direct constant / trap use`
- `TBX_SETWTITLE`: `os/apps/cpanels/cpanels.c:56` `tbx_set_wtitle`

#### apps-itext

- `TBX_ALERT`: `os/apps/itext/itext.c:338` `tbx_alert`; `os/apps/itext/itext.c:466` `tbx_alert`
- `TBX_EXIT`: `os/apps/itext/itext.c:441` `tbx_exit`; `os/apps/itext/itext.c:623` `tbx_exit`
- `TBX_FILE_CLOSE`: `os/apps/itext/itext.c:346` `tbx_file_close`; `os/apps/itext/itext.c:396` `tbx_file_save`
- `TBX_FILE_CREATE_NEW`: `os/apps/itext/itext.c:396` `tbx_file_save`
- `TBX_FILE_DELETE`: `os/apps/itext/itext.c:396` `tbx_file_save`
- `TBX_FILE_OPEN`: `os/apps/itext/itext.c:343` `tbx_file_open`
- `TBX_FILE_READ`: `os/apps/itext/itext.c:345` `tbx_file_read`
- `TBX_FILE_RENAME`: `os/apps/itext/itext.c:396` `tbx_file_save`
- `TBX_FILE_WRITE`: `os/apps/itext/itext.c:396` `tbx_file_save`
- `TBX_FILLRECT`: `os/apps/itext/itext.c:209` `tbx_fill_rect`; `os/apps/itext/itext.c:220` `tbx_fill_rect`; `os/apps/itext/itext.c:230` `tbx_fill_rect`; `os/apps/itext/itext.c:238` `tbx_fill_rect`; `os/apps/itext/itext.c:245` `tbx_fill_rect`
- `TBX_GETSCROLL`: `os/apps/itext/itext.c:637` `tbx_get_scroll`
- `TBX_GLOBALTOLOCAL`: `os/apps/itext/itext.c:580` `tbx_global_to_local`
- `TBX_NEWWINDOW`: `os/apps/itext/itext.c:621` `tbx_new_window`
- `TBX_REGISTER`: `os/apps/itext/itext.c:615` `tbx_register`
- `TBX_SCRAP_GET`: `os/apps/itext/itext.c:293` `tbx_scrap_get`; `os/apps/itext/itext.c:545` `tbx_scrap_get`
- `TBX_SCRAP_PUT`: `os/apps/itext/itext.c:539` `tbx_scrap_put`
- `TBX_SETMBAR`: `os/apps/itext/itext.c:620` `tbx_set_mbar`
- `TBX_SETSCROLL`: `os/apps/itext/itext.c:268` `tbx_set_scroll`
- `TBX_SETWTITLE`: `os/apps/itext/itext.c:306` `tbx_set_wtitle`
- `TBX_SFGETFILE`: `os/apps/itext/itext.c:434` `tbx_sf_getfile`
- `TBX_SFPUTFILE`: `os/apps/itext/itext.c:417` `tbx_sf_putfile`
- `TBX_TEXTDRAW`: `os/apps/itext/itext.c:185` `tbx_draw_text`
- `TBX_TRACKMOUSE`: `os/apps/itext/itext.c:655` `tbx_track_mouse`

#### apps-notepad

- `TBX_ALERT`: `os/apps/notepad/notepad.c:138` `tbx_alert`; `os/apps/notepad/notepad.c:334` `tbx_alert`
- `TBX_CONTENTORIGIN`: `os/apps/notepad/notepad.c:379` `tbx_content_origin`
- `TBX_EXIT`: `os/apps/notepad/notepad.c:337` `tbx_exit`; `os/apps/notepad/notepad.c:346` `tbx_exit`; `os/apps/notepad/notepad.c:364` `tbx_exit`
- `TBX_FILE_CLOSE`: `os/apps/notepad/notepad.c:123` `tbx_file_close`; `os/apps/notepad/notepad.c:158` `tbx_file_save`
- `TBX_FILE_CREATE_NEW`: `os/apps/notepad/notepad.c:158` `tbx_file_save`
- `TBX_FILE_DELETE`: `os/apps/notepad/notepad.c:158` `tbx_file_save`
- `TBX_FILE_OPEN`: `os/apps/notepad/notepad.c:118` `tbx_file_open`
- `TBX_FILE_READ`: `os/apps/notepad/notepad.c:122` `tbx_file_read`
- `TBX_FILE_RENAME`: `os/apps/notepad/notepad.c:158` `tbx_file_save`
- `TBX_FILE_WRITE`: `os/apps/notepad/notepad.c:158` `tbx_file_save`
- `TBX_FILLRECT`: `os/apps/notepad/notepad.c:213` `tbx_fill_rect`; `os/apps/notepad/notepad.c:225` `tbx_fill_rect`; `os/apps/notepad/notepad.c:232` `tbx_fill_rect`; `os/apps/notepad/notepad.c:237` `tbx_fill_rect`; `os/apps/notepad/notepad.c:238` `tbx_fill_rect`
- `TBX_GETSCROLL`: `os/apps/notepad/notepad.c:353` `tbx_get_scroll`
- `TBX_NEWWINDOW`: `os/apps/notepad/notepad.c:345` `tbx_new_window`
- `TBX_REGISTER`: `os/apps/notepad/notepad.c:341` `tbx_register`
- `TBX_SCRAP_GET`: `os/apps/notepad/notepad.c:177` `tbx_scrap_get`; `os/apps/notepad/notepad.c:296` `tbx_scrap_get`
- `TBX_SCRAP_PUT`: `os/apps/notepad/notepad.c:277` `tbx_scrap_put`; `os/apps/notepad/notepad.c:279` `tbx_scrap_put`
- `TBX_SETMBAR`: `os/apps/notepad/notepad.c:344` `tbx_set_mbar`
- `TBX_SETSCROLL`: `os/apps/notepad/notepad.c:199` `tbx_set_scroll`; `os/apps/notepad/notepad.c:207` `tbx_set_scroll`
- `TBX_SETWTITLE`: `os/apps/notepad/notepad.c:347` `tbx_set_wtitle`
- `TBX_TEXTDRAW`: `os/apps/notepad/notepad.c:190` `tbx_draw_text`; `os/apps/notepad/notepad.c:245` `tbx_draw_text`; `os/apps/notepad/notepad.c:247` `tbx_draw_text`
- `TBX_TRACKMOUSE`: `os/apps/notepad/notepad.c:397` `tbx_track_mouse`

#### apps-paint

- `TBX_EXIT`: `os/apps/paint/paint.c:95` `tbx_exit`; `os/apps/paint/paint.c:108` `tbx_exit`
- `TBX_FILLRECT`: `os/apps/paint/paint.c:103` `tbx_fill_rect`
- `TBX_NEWWINDOW`: `os/apps/paint/paint.c:94` `tbx_new_window`
- `TBX_REGISTER`: `os/apps/paint/paint.c:93` `tbx_register`
- `TBX_SETMBAR`: `os/apps/paint/paint.c:93` `tbx_set_mbar`
- `TBX_SETWTITLE`: `os/apps/paint/paint.c:95` `tbx_set_wtitle`

#### apps-fonts

- `TBX_EXIT`: `os/apps/fonts/fonts.c:96` `tbx_exit`
- `TBX_FILLRECT`: `os/apps/fonts/fonts.c:68` `tbx_fill_rect`
- `TBX_NEWWINDOW`: `os/apps/fonts/fonts.c:83` `tbx_new_window`
- `TBX_REGISTER`: `os/apps/fonts/fonts.c:81` `tbx_register`
- `TBX_SETMBAR`: `os/apps/fonts/fonts.c:82` `tbx_set_mbar`
- `TBX_SETWTITLE`: `os/apps/fonts/fonts.c:85` `tbx_set_wtitle`
- `TBX_TEXTDRAW`: `os/apps/fonts/fonts.c:63` `tbx_draw_text`

#### apps-ctenant

- `TBX_DRAWCELLS`: `os/apps/ctenant/ctenant.c:76` `tbx_draw_cells`; `os/apps/ctenant/ctenant.c:79` `tbx_draw_cells`; `os/apps/ctenant/files.c:25` `tbx_draw_cells`
- `TBX_EXIT`: `os/apps/ctenant/ctenant.c:124` `tbx_exit`; `os/apps/ctenant/files.c:52` `tbx_exit`
- `TBX_FILE_CLOSE`: `os/apps/ctenant/files.c:18` `tbx_file_close`; `os/apps/ctenant/files.c:18` `direct constant / trap use`; `os/apps/ctenant/files.c:33` `direct constant / trap use`; `os/apps/ctenant/files.c:39` `direct constant / trap use`; `os/apps/ctenant/files.c:43` `direct constant / trap use`
- `TBX_FILE_CREATE`: `os/apps/ctenant/files.c:13` `tbx_file_create`; `os/apps/ctenant/files.c:13` `direct constant / trap use`; `os/apps/ctenant/files.c:30` `tbx_file_create`; `os/apps/ctenant/files.c:31` `direct constant / trap use`; `os/apps/ctenant/files.c:41` `direct constant / trap use`
- `TBX_FILE_DELETE`: `os/apps/ctenant/files.c:19` `tbx_file_delete`; `os/apps/ctenant/files.c:19` `direct constant / trap use`; `os/apps/ctenant/files.c:43` `direct constant / trap use`
- `TBX_FILE_OPEN`: `os/apps/ctenant/files.c:14` `tbx_file_open`; `os/apps/ctenant/files.c:14` `direct constant / trap use`; `os/apps/ctenant/files.c:34` `direct constant / trap use`; `os/apps/ctenant/files.c:49` `direct constant / trap use`
- `TBX_FILE_READ`: `os/apps/ctenant/files.c:15` `tbx_file_read`; `os/apps/ctenant/files.c:15` `direct constant / trap use`; `os/apps/ctenant/files.c:36` `direct constant / trap use`
- `TBX_FILE_SEEK`: `os/apps/ctenant/files.c:17` `tbx_file_seek`; `os/apps/ctenant/files.c:17` `direct constant / trap use`; `os/apps/ctenant/files.c:35` `direct constant / trap use`
- `TBX_FILE_WRITE`: `os/apps/ctenant/files.c:16` `tbx_file_write`; `os/apps/ctenant/files.c:16` `direct constant / trap use`; `os/apps/ctenant/files.c:32` `direct constant / trap use`; `os/apps/ctenant/files.c:42` `direct constant / trap use`
- `TBX_FILLRECT`: `os/apps/ctenant/ctenant.c:64` `tbx_fill_rect`
- `TBX_NEWWINDOW`: `os/apps/ctenant/ctenant.c:86` `tbx_new_window`; `os/apps/ctenant/files.c:67` `tbx_new_window`
- `TBX_REGISTER`: `os/apps/ctenant/ctenant.c:84` `tbx_register`; `os/apps/ctenant/files.c:66` `tbx_register`
- `TBX_SETMBAR`: `os/apps/ctenant/ctenant.c:85` `tbx_set_mbar`
- `TBX_SETWTITLE`: `os/apps/ctenant/ctenant.c:88` `tbx_set_wtitle`; `os/apps/ctenant/files.c:68` `tbx_set_wtitle`
- `TBX_TEXTDRAW`: `os/apps/ctenant/ctenant.c:65` `tbx_draw_text`; `os/apps/ctenant/ctenant.c:75` `tbx_draw_text`

#### apps-i123

- `TBX_ALERT`: `os/i123/app/face_a.c:1305` `tbx_alert`
- `TBX_BEEP`: `os/i123/app/face_a.c:800` `direct constant / trap use`; `os/i123/app/face_a.c:1230` `direct constant / trap use`
- `TBX_CLOCK`: `os/i123/app/face_a.c:196` `tbx_get_clock`
- `TBX_CONTENTORIGIN`: `os/i123/app/face_a.c:1260` `tbx_content_origin`
- `TBX_CONTENT_SIZE`: `os/i123/app/face_a.c:437` `tbx_content_size`
- `TBX_DRAWCELLS`: `os/i123/app/face_a.c:303` `tbx_draw_cells`
- `TBX_EXIT`: `os/i123/app/face_a.c:720` `tbx_exit`; `os/i123/app/face_a.c:1303` `tbx_exit`; `os/i123/app/face_a.c:1319` `tbx_exit`; `os/i123/app/face_a.c:1347` `tbx_exit`; `os/i123/app/face_a.c:1366` `tbx_exit`; `os/i123/app/face_a.c:1374` `tbx_exit`
- `TBX_FILE_CLOSE`: `os/i123/app/face_a.c:693` `tbx_file_close`; `os/i123/app/face_a.c:716` `tbx_file_save`; `os/i123/app/face_a.c:736` `tbx_file_close`; `os/i123/app/face_a.c:1364` `tbx_file_close`
- `TBX_FILE_CREATE_NEW`: `os/i123/app/face_a.c:716` `tbx_file_save`
- `TBX_FILE_DELETE`: `os/i123/app/face_a.c:716` `tbx_file_save`
- `TBX_FILE_OPEN`: `os/i123/app/face_a.c:681` `tbx_file_open`; `os/i123/app/face_a.c:734` `tbx_file_open`; `os/i123/app/face_a.c:1360` `tbx_file_open`
- `TBX_FILE_READ`: `os/i123/app/face_a.c:688` `tbx_file_read`; `os/i123/app/face_a.c:1363` `tbx_file_read`
- `TBX_FILE_RENAME`: `os/i123/app/face_a.c:716` `tbx_file_save`
- `TBX_FILE_SEEK`: `os/i123/app/face_a.c:683` `tbx_file_seek`; `os/i123/app/face_a.c:684` `tbx_file_seek`
- `TBX_FILE_WRITE`: `os/i123/app/face_a.c:716` `tbx_file_save`
- `TBX_FILLRECT`: `os/i123/app/face_a.c:586` `tbx_fill_rect`
- `TBX_NEWWINDOW`: `os/i123/app/face_a.c:1372` `tbx_new_window`
- `TBX_REGISTER`: `os/i123/app/face_a.c:1343` `tbx_register`
- `TBX_SETMBAR`: `os/i123/app/face_a.c:1370` `tbx_set_mbar`
- `TBX_SETWTITLE`: `os/i123/app/face_a.c:1377` `tbx_set_wtitle`

#### apps-iword

- `TBX_CLOCK`: `os/iword/app/face_a.c:178` `direct constant / trap use`; `os/iword/app/face_a.c:1438` `tbx_get_clock`
- `TBX_DISK_FREE`: `os/iword/app/face_a.c:1436` `tbx_disk_free`
- `TBX_DRAWCELLS`: `os/iword/app/face_a.c:400` `tbx_draw_cells`
- `TBX_EXIT`: `os/iword/app/face_a.c:1944` `tbx_exit`; `os/iword/app/face_a.c:2011` `tbx_exit`; `os/iword/app/face_a.c:2761` `tbx_exit`
- `TBX_FILE_CLOSE`: `os/iword/app/face_a.c:1154` `tbx_file_close`; `os/iword/app/face_a.c:1174` `tbx_file_save`; `os/iword/app/face_a.c:1270` `tbx_file_save`; `os/iword/app/face_a.c:1287` `tbx_file_close`; `os/iword/app/face_a.c:1319` `tbx_file_close`; `os/iword/app/face_a.c:1557` `tbx_file_copy_save`; `os/iword/app/face_a.c:1654` `tbx_file_close`; `os/iword/app/face_a.c:1679` `tbx_file_close`
- `TBX_FILE_CREATE_NEW`: `os/iword/app/face_a.c:1174` `tbx_file_save`; `os/iword/app/face_a.c:1270` `tbx_file_save`; `os/iword/app/face_a.c:1557` `tbx_file_copy_save`
- `TBX_FILE_DELETE`: `os/iword/app/face_a.c:1174` `tbx_file_save`; `os/iword/app/face_a.c:1270` `tbx_file_save`; `os/iword/app/face_a.c:1543` `tbx_file_delete`; `os/iword/app/face_a.c:1557` `tbx_file_copy_save`; `os/iword/app/face_a.c:1558` `tbx_file_delete`
- `TBX_FILE_FIND`: `os/iword/app/face_a.c:1432` `tbx_file_find`; `os/iword/app/face_a.c:1531` `tbx_file_find`; `os/iword/app/face_a.c:1609` `tbx_file_find`
- `TBX_FILE_OPEN`: `os/iword/app/face_a.c:1147` `tbx_file_open`; `os/iword/app/face_a.c:1285` `tbx_file_open`; `os/iword/app/face_a.c:1308` `tbx_file_open`; `os/iword/app/face_a.c:1557` `tbx_file_copy_save`; `os/iword/app/face_a.c:1652` `tbx_file_open`; `os/iword/app/face_a.c:1673` `tbx_file_open`
- `TBX_FILE_READ`: `os/iword/app/face_a.c:1150` `tbx_file_read`; `os/iword/app/face_a.c:1314` `tbx_file_read`; `os/iword/app/face_a.c:1557` `tbx_file_copy_save`; `os/iword/app/face_a.c:1676` `tbx_file_read`
- `TBX_FILE_RENAME`: `os/iword/app/face_a.c:1174` `tbx_file_save`; `os/iword/app/face_a.c:1270` `tbx_file_save`; `os/iword/app/face_a.c:1550` `tbx_file_rename`; `os/iword/app/face_a.c:1557` `tbx_file_copy_save`
- `TBX_FILE_SEEK`: `os/iword/app/face_a.c:1310` `tbx_file_seek`; `os/iword/app/face_a.c:1311` `tbx_file_seek`; `os/iword/app/face_a.c:1557` `tbx_file_copy_save`
- `TBX_FILE_WRITE`: `os/iword/app/face_a.c:1174` `tbx_file_save`; `os/iword/app/face_a.c:1270` `tbx_file_save`; `os/iword/app/face_a.c:1557` `tbx_file_copy_save`
- `TBX_FILLRECT`: `os/iword/app/face_a.c:1124` `tbx_fill_rect`
- `TBX_NEWWINDOW`: `os/iword/app/face_a.c:2759` `tbx_new_window`
- `TBX_REGISTER`: `os/iword/app/face_a.c:2755` `tbx_register`
- `TBX_SETMBAR`: `os/iword/app/face_a.c:2758` `tbx_set_mbar`
- `TBX_SETWTITLE`: `os/iword/app/face_a.c:2764` `tbx_set_wtitle`

#### apps-samir

No SDK trap calls statically observed. SAMIR uses INT 21h; see its PAL.

### All app-gate bypasses / library links

- HELLO/NOTES: `os/apps/ref_tenant.c:77`–86 includes process/window/blitter/surface/heap; linked resident via `Makefile:10828`. `os/milton/kmain.c:110` includes their vtables and kmain instantiates the reference apps. Convert to disk examples or mark privileged fixtures.
- Finder: resident process/window/menu/heap callbacks in `os/milton/kmain.c:3152`; `os/flair/finder_*.c` are resident link inputs at `Makefile:10828`. It has no tenant `.exe` and no INT 81h-only boundary. Desktop/service boundary must be explicit.
- TEXT.EXE: `Makefile:10657` directly links `os/flair/textedit.c`; `Makefile:10658` depends on textedit/text/scrap headers and font data.
- NOTEPAD.EXE / SCRAPBK.EXE: `Makefile:10669` shares the same TextEdit/font source set; `Makefile:10672` selects Scrapbook with `-DSCRAPBOOK`.
- CALC.EXE: i123 core reuse at `Makefile:10750` and `os/apps/calc/calc.c:10`; it is a library dependency rather than a Toolbox service bypass.
- The remaining C tenants link their own app/core code plus `tbx_rt.c`, as the exact compile loops show. They include spec contracts, not resident FLAIR manager sources. SAMIR and TPS are DOS programs and use INT 21h; this is their intended interface.
- Public ABI contamination: `spec/cp_dialog.h:28` conditionally contains a private DialogRecord/ControlRecord converter; split it from the flat SDK records.

## 8. Discard

Discard legacy size/headroom/overlay-layout policing and mutant kernel/image variants after preserving their functional assertions as ordinary tests or patches. Keep independent golden captures/readers/property suites; do not discard an oracle merely because its legacy target contains size or overlay.
