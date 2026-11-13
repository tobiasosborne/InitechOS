# boot

Import MBR, IO.SYS and handoff behavior. Replace the fixed-sector, single-buffer loader with chunked reads before lifting kernel caps. `os/milton/boot_info.h` is physically under kernel today but is the public boot/kernel handoff contract.

Evidence revision: `c5d18538049986bbbb27018c351405acf814ebbc` (`legacy-final`). Physical LOC includes comments and blank lines; tests and data are separate from production counts. Source ownership recommendations are explicitly inferences; paths, includes, guard counts and dry-run command text are measured.

## 1. Sources

3 production/source-support files; 1,807 physical lines. Test programs are in section 4.

|Legacy path|Lines|Role / disposition|
|---|---:|---|
|`os/boot/io_sys.asm:1`|1161|BIOS setup and the original real->protected/flat handoff.|
|`os/boot/mbr.asm:1`|175|InitechOS tracer boot, stage1 / MBR (512 bytes, sector 0).|
|`os/boot/stage2.asm:1`|471|physical two-file bootstrap, initech-kod.|

### Contracts, fixture/data and ancillary files owned here

Runtime files in the requested `os/`, `seed/`, `spec/`, `fixtures/` trees are assigned; os/README.md and os/apps/.gitkeep remain explicit unassigned placeholders in SUMMARY. `spec/*.mk` input traces are test data to preserve separately from their obsolete build syntax.

|File|Lines (text)|Role|
|---|---:|---|
|`os/boot/.gitkeep`|binary|.gitkeep; source/contract input (role inferred from path)|
|`spec/mbr_partition_contract.json:1`|22|"_comment": "Locked by initech-kzfs under ADR-0003 DEC-07a; explicit lane ruling selects content sniffing so shipped ATA superfloppies remain valid. Not an INT21h AH register. IBM DOS 3.30 T|

## 2. Headers it needs from elsewhere

Literal include sites, including host-only conditional includes embedded in production TUs. PUBLIC INTERFACE means publish/move the contract or supported reusable library header; INTERNAL LEAK means replace a foreign implementation dependency. This is not an assertion that every conditional include is active in the production build.

|Include site|Spelling → resolved path|Classification|
|---|---|---|
|`os/boot/io_sys.asm:45`|`dos_messages.inc` → `build/dos_messages.inc`|PUBLIC INTERFACE|

### Generated inputs

|Header|Generator inputs / recipe origin|
|---|---|
|`build/dos_messages.inc`|`build/dos_messages.h | build`; #  recipe to execute (from 'Makefile', line 28357):|
|`build/dos_messages.h`|`spec/dos_messages.json | build`; #  recipe to execute (from 'Makefile', line 28325):|

## 3. How it is built today

`-m32 -march=i386` pins the 32-bit 386-compatible ISA used on the emulated 486. Freestanding/no-libc linkage and entry/linker addresses remain required until packaging changes. `-fno-pic -fno-pie` agrees with flat placement and relocation derivation. Kernel `-mregparm=3 -mrtd -freg-struct-return` was introduced to save bytes, but it is now an ABI: keep C/assembly declarations and all linked translation units in agreement until changing both sides. Do not simply delete it from one file. INT 81h arguments remain explicit dword cdecl with `TBX_TRAP_ABI` exceptions. Preserve `-fno-tree-loop-distribute-patterns` where it prevents freestanding code acquiring libc calls, or supply those functions. `-Oz`, per-object `-Os`, LTO/partition choices, `-fno-inline`, per-pass `-fno-*` lists, `-mtune=generic`, `-malign-data=abi`, `--param` peeling limits and 4-byte stack tuning were selected for legacy caps; retire after replacing caps and verifying the coherent ABI. `-fno-strict-aliasing` and `-fno-common` are correctness/toolchain choices, not automatically disposable size levers.

Commands below are captured output of `make -n -B --trace ...`, never executed. `-o build/flair_dac.stamp.mk` suppresses the parse-time included-makefile remake. Empty headroom-enumeration overrides only suppress irrelevant recursive database enumeration. Shell loops preserve exact input/link order; variables like `$objs` are runtime shell variables, not omitted make flags.

### dry-production-boot.txt
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/mbr.bin build/stage2.bin build/IO.SYS
# exit: 0
Makefile:12268: target 'build' does not exist
mkdir -p build
Makefile:76: update target 'build/.makefile.stamp' due to: Makefile os/samir/cmd/flow_verbs.def os/samir/cmd/mutate_verbs.def os/samir/cmd/proc_verbs.def os/samir/cmd/query_verbs.def os/samir/cmd/set_options.def harness/diff/fat_diff/doslook.mk harness/diff/fat_diff/xcopy_tree.mk harness/diff/fat_diff/copy_stamp.mk tools/serial_trace_pack.c os/milton/kmain_cold.h os/flair/chicago_packed.h harness/diff/fat_diff/shell_files.mk
mkdir -p build/ && touch build/.makefile.stamp
Makefile:8801: update target 'build/mbr.bin' due to: os/boot/mbr.asm
nasm -f bin os/boot/mbr.asm -o build/mbr.bin
Makefile:28325: update target 'build/dos_messages.h' due to: spec/dos_messages.json
printf '>>> codegen: %s -> %s (63 messages, deterministic)\n' "spec/dos_messages.json" "build/dos_messages.h"
python3 -c "import json; \
d=json.load(open('spec/dos_messages.json')); \
m=d['messages'] if isinstance(d,dict) and 'messages' in d else d; \
assert isinstance(m,dict), 'messages is not an object'; \
esc=lambda s: s.replace(chr(92),chr(92)*2).replace(chr(34),chr(92)+chr(34)).replace(chr(13),chr(92)+'r').replace(chr(10),chr(92)+'n'); \
ids=['MSG-DOS-%04d'%i for i in range(1,64)]; \
missing=[k for k in ids if k not in m]; \
assert not missing, 'spec missing message id(s) (DEC-13 controlled scope): %r'%missing; \
[ (_ for _ in ()).throw(AssertionError('non-string/empty text for %s'%k)) for k in ids if not (isinstance(m[k],str) and m[k]) ]; \
[ (_ for _ in ()).throw(AssertionError('non-ASCII text for %s (Rule 12)'%k)) for k in ids if not all(ord(c)<128 for c in m[k]) ]; \
L=[]; \
L.append('/*'); \
L.append(' * dos_messages.h -- InitechDOS Approved Diagnostic Message Catalogue.'); \
L.append(' *'); \
L.append(' * GENERATED -- DO NOT EDIT; edit the spec and rebuild.'); \
L.append(' *'); \
L.append(' * Source:  spec/dos_messages.json'); \
L.append(' * Ref:     ADR-0003 Appendix C / DEC-13 (controlled vocabulary).'); \
L.append(' *          %c denotes a drive-letter substitution; spacing and'); \
L.append(' *          punctuation are part of the controlled vocabulary.'); \
L.append(' */'); \
L.append('#ifndef INITECH_DOS_MESSAGES_H'); \
L.append('#define INITECH_DOS_MESSAGES_H'); \
L.append(''); \
L+=['#define %s \"%s\"'%(k.replace('-','_'),esc(m[k])) for k in ids]; \
L.append(''); \
L.append('#endif /* INITECH_DOS_MESSAGES_H */'); \
open('build/dos_messages.h','w').write(chr(10).join(L)+chr(10))" \
	|| { printf '!!! codegen FAIL: %s invalid (missing id / bad shape / non-ASCII)\n' "spec/dos_messages.json"; exit 1; }
Makefile:28357: update target 'build/dos_messages.inc' due to: build/dos_messages.h
sed -n 's/^#define \(MSG_DOS_[0-9]*\) "\(.*\)"$/%define \1 `\2`/p' build/dos_messages.h > build/dos_messages.inc
Makefile:8813: update target 'build/IO.SYS' due to: os/boot/io_sys.asm Makefile build/dos_messages.inc
nasm -f bin -Ibuild/ -DMK_KERNEL_SECTORS=384 os/boot/io_sys.asm -o build/IO.SYS
test $(wc -c < build/IO.SYS) -le 4096
Makefile:8817: update target 'build/stage2.bin' due to: os/boot/stage2.asm build/IO.SYS Makefile
nasm -f bin -DMK_KERNEL_SECTORS=384 -DIO_SYS_PATH='"build/IO.SYS"' os/boot/stage2.asm -o build/stage2.bin
make: 'build/IO.SYS' is up to date.

# stderr

```

## 4. Host tests that travel with it

Counts by unique host program/main source (not mutant binaries, CLI cases, or gate names): **ORACLE-STRONG 0; UNIT 0; META 0; SIZE/LAYOUT 0.** META is reserved for factory/process/spec-inventory checks; input generators are listed as support sources rather than counted as tests. Some static classes are inferred from inspected test bodies; the basis is stated for each.

Every build variant name is listed; the exact canonical compilation is from its artifact-only dry run. Variant recipes and invocation assertions remain available in [gates-boot.md](gates-boot.md). The complete host dry-run archive is [dry-host.json](dry-host.json). Gate lists below use direct binary prerequisites/recipe references, avoiding giant transitive build closures that would misassign whole kernel test inventories.
No standalone host test program found for this component. This is a coverage gap, not evidence of zero assurance; relevant emulator graders and shared SDK tests are listed elsewhere.

### Recipe-driven host suites and variants

All assigned legacy gate recipes, including compiler/FPC/mtools/script orchestration and mutants, are enumerated in [gates-boot.md](gates-boot.md). Host tier here is the declared legacy vector; some entries launch emulators transitively. The directory name proptest is not itself evidence of a property oracle.

## 5. Emulator gates worth keeping as smokes

**6 exclusive primary-owned entries in the 799-entry declared legacy emulator vector.** Also 6 defined test-* targets owned here have a static dependency closure reaching QEMU/Bochs (includes wrappers, mutants and names outside the active vector; not a runtime boot count). Cross-component smokes below are not double-counted. Exact primary attribution is in [gate-attribution.tsv](gate-attribution.tsv).

### test-tracer-boot

#  recipe to execute (from 'Makefile', line 19195):

Boot/data images named by direct prerequisites or expanded recipe: `build/tracer_secondary.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@printf '  NOTE: pure-seafoam grid SUPERSEDED by ppm_text_check (banner blitted);\n'
@if grep -q 'triple_fault=1' "build/tracer_boot.report"; then \
	printf '!!! test-tracer-boot FAIL: TRIPLE FAULT detected in the real->protected transition\n'; \
	printf '!!! test-tracer-boot FAIL: no serial captured at %s\n' "build/tracer_boot.serial"; \
	if grep -q "^$$m$$" "build/tracer_boot.serial"; then \
	grep -q "^$$m$$" "build/tracer_boot.serial" \
		|| { printf '!!! test-tracer-boot FAIL: required serial marker %s missing\n' "$$m"; exit 1; }; \
@kat=$$(grep -m1 '^KAT 0x' "build/tracer_boot.serial" || true); \
		printf '!!! test-tracer-boot FAIL: KAT start=%s, expected 0x00500000\n' "$$start"; exit 1; \
	case "$$end" in 0x*) ;; *) printf '!!! test-tracer-boot FAIL: malformed KAT end in "%s"\n' "$$kat"; exit 1;; esac; \
		printf '!!! test-tracer-boot FAIL: KAT end=%s reaches KERNEL_CEIL=0x00600000\n' "$$end"; exit 1; \
	printf '!!! test-tracer-boot FAIL: no screendump captured at %s (live guest required)\n' "build/tracer_boot.ppm"; \
```

### test-boot-bochs

#  recipe to execute (from 'Makefile', line 19314):

Boot/data images named by direct prerequisites or expanded recipe: `build/tracer_secondary.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@command -v bochs >/dev/null 2>&1 || { printf '!!! test-boot-bochs FAIL: bochs not found (apt install bochs -- CLAUDE.md documents it as a required base tool). A skipped oracle is worse than a red one (Law 2). Set SKIP_BOCHS=1 to explicitly (and loudly) opt out.\n'; exit 1; }
@build/bochs_harness --disk "build/tracer_secondary.img" --expect SHELL-READY --quit-after SHELL-READY \
@sh harness/emu/bochs_leg_check.sh test-boot-bochs "build/bochsboot.report.txt" "build/bochsboot.serial" vbe101 \
@kat=$$(grep -m1 '^KAT 0x' "build/bochsboot.serial" || true); \
		printf '!!! test-boot-bochs FAIL: KAT start=%s, expected 0x00500000\n' "$$start"; exit 1; \
	case "$$end" in 0x*) ;; *) printf '!!! test-boot-bochs FAIL: malformed KAT end in "%s"\n' "$$kat"; exit 1;; esac; \
		printf '!!! test-boot-bochs FAIL: KAT end=%s reaches KERNEL_CEIL=0x00600000\n' "$$end"; exit 1; \
```

The remaining legacy emulator entries cover MBR/partition/IO.SYS handoff, VBE mode variants, byte/order fixtures and negative loader cases. Mutant twins establish sensitivity of the old assertion; convert useful faults to patches instead of maintaining extra resident kernels. The complete names/recipes are linked above.

## 6. Mutation hooks in production source

Measured count is conditional directive sites containing deliberately-wrong macro names (`MUT`, `MUTATE`, `MUTANT`, `BROKEN`, `DROP_WRITE`, `DOTRUNC`), not #else arms or runtime executions. One directive can mention two macros; distinct macro count is also provided. `#ifndef` controls count because the wrong behavior may be its absence. Non-mutation BOOT/profile selectors are excluded.

|File|Guard sites|Distinct macros|Macros and exact sites|
|---|---:|---:|---|
|`os/boot/io_sys.asm:1`|2|2|`os/boot/io_sys.asm:466` `VBE_MUT_8_FIRST`; `os/boot/io_sys.asm:539` `VBE_MUT_NO_101`|
|`os/boot/mbr.asm:1`|0|0|none|
|`os/boot/stage2.asm:1`|4|3|`os/boot/stage2.asm:228` `BOOT_MUT_ORDER`; `os/boot/stage2.asm:248` `BOOT_MUT_ORDER`; `os/boot/stage2.asm:261` `BOOT_MUT_ATTR_R`; `os/boot/stage2.asm:307` `BOOT_MUT_CHAIN`|

## 7. Coupling to fix during import

Concrete include/external-symbol evidence follows; line numbers refer to legacy-final. Cross-component public ABI includes in section 2 are expected dependencies, not violations.
No production include/declaration leak found by this static scan. Build/golden dependencies still apply; this does not prove absence of aliasing, absolute-address or assembly-level coupling.

## 8. Discard

Discard legacy size/headroom/overlay-layout policing and mutant kernel/image variants after preserving their functional assertions as ordinary tests or patches. Keep independent golden captures/readers/property suites; do not discard an oracle merely because its legacy target contains size or overlay.
Gates/process wrappers to retire: `test-boot-system-budget`.
