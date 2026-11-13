# dos

COMMAND is currently a kernel-address-bound transient, not a normal independently relocatable COMMAND.COM. Keep command/batch/env logic and utilities; replace exact resident imports with INT 21h bindings. `transient.*` is packaging to retire, not the desired shell ABI.

Evidence revision: `c5d18538049986bbbb27018c351405acf814ebbc` (`legacy-final`). Physical LOC includes comments and blank lines; tests and data are separate from production counts. Source ownership recommendations are explicitly inferences; paths, includes, guard counts and dry-run command text are measured.

## 1. Sources

40 production/source-support files; 12,072 physical lines. Test programs are in section 4.

|Legacy path|Lines|Role / disposition|
|---|---:|---|
|`os/milton/batch.c:1`|858|InitechDOS COMMAND.COM .BAT interpreter: pure batch-language|
|`os/milton/batch.h:1`|275|InitechDOS COMMAND.COM .BAT interpreter: pure batch-language|
|`os/milton/chkdsk.c:1`|348|CHKDSK engine (artifact, freestanding C).|
|`os/milton/chkdsk.h:1`|22|CHKDSK.COM contract. Ref: PRD 6.1; ADR-0003 App A/B;|
|`os/milton/chkdsk.ld:1`|13|ENTRY(_start)|
|`os/milton/chkdsk_program.c:1`|46|Native flat .COM adapter. Ref: ADR-0003 App A (19h,3Fh,40h,47h,4Ch);|
|`os/milton/chkdsk_start.asm:1`|17|global _start|
|`os/milton/command.c:1`|5145|InitechDOS COMMAND.COM-alike interactive shell (the A:\> REPL).|
|`os/milton/command.h:1`|413|InitechDOS COMMAND.COM-alike interactive shell (the A:\> REPL).|
|`os/milton/diskutil.c:1`|273|Freestanding disk utilities. Law 1: ADR-0003 Appendix A/B.1, DEC-17/18;|
|`os/milton/diskutil.h:1`|16|kind: 0 FORMAT, 1 SYS, 2 DISKCOPY, 3 DISKCOMP. */|
|`os/milton/diskutil_program.c:1`|18|Flat adapter; ADR-0003 Appendix A (07/40/4C), DEC-04a/08/17/18.|
|`os/milton/diskutil_start.asm:1`|17|ADR-0003 DEC-08 flat COM; spec/memory_map.h.|
|`os/milton/dosutil.c:1`|318|freestanding DOS 3.3 utilities (gyvx/f88h).|
|`os/milton/dosutil.h:1`|16|Flat utility platform seam. Ref: ADR-0003 Appendix A (43h,47h,4Eh/4Fh),|
|`os/milton/dosutil.ld:1`|13|ADR-0003 DEC-08 flat COM, spec/memory_map.h. No host metadata. */|
|`os/milton/dosutil_target.c:1`|91|Freestanding flat COM adapter. Ref: ADR-0003 Appendix A, DEC-04a/08/15;|
|`os/milton/edlin.c:1`|327|EDLIN.COM, DOS 3.3 line editor, initech-udr1 (DEBUG is not implemented here).|
|`os/milton/edlin.h:1`|16|edlin.h; source/contract input (role inferred from path)|
|`os/milton/edlin.ld:1`|13|ENTRY(_start)|
|`os/milton/edlin_program.c:1`|30|DOS adapter. Ref: ADR-0003 Appendix A, DEC-04a flat ABI, DEC-08;|
|`os/milton/edlin_start.asm:1`|17|global _start|
|`os/milton/env.c:1`|302|InitechDOS COMMAND.COM master environment store.|
|`os/milton/env.h:1`|102|InitechDOS COMMAND.COM master environment store.|
|`os/milton/fdisk.c:1`|252|FDISK primary-partition editor. Ref: IBM DOS 3.30 TR pp9-6..9;|
|`os/milton/fdisk.h:1`|22|Primary-partition editor: IBM DOS TR pp9-6..9; MS-DOS UG pp287..295.|
|`os/milton/fdisk.ld:1`|12|DEC-08 flat COM and spec/memory_map.h, same loader reserve as EDLIN. */|
|`os/milton/fdisk_program.c:1`|56|Flat COM adapter, ADR-0003 Appendix A / DEC-04a / DEC-08 / DEC-17.|
|`os/milton/fdisk_start.asm:1`|17|DEC-08 / DEC-04a flat COM; spec/memory_map.h PROGRAM_IMAGE.|
|`os/milton/find_program.asm:1`|395|%include "dos_messages.inc" ; DEC-13; MS-DOS 3.3 printed pp.337/341. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/more_program.asm:1`|138|the FAT-sourced flat (.COM-equivalent) MORE filter. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/sort_program.asm:1`|335|%include "dos_messages.inc" ; DEC-13; MS-DOS 3.3 printed pp.337/341. **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/tpsrep_program.asm:1`|1382|TPSREP.COM, the INITECH TPS REPORT GENERATOR **test guest or external entry; inspect deployment before treating as product**|
|`os/milton/transient.h:1`|80|the TRANSIENT portion of COMMAND.COM on the flagship image.|
|`os/milton/transient.ld:1`|41|link script for the TRANSIENT COMMAND.COM (bead|
|`os/milton/transient_hdr.asm:1`|36|the header of the TRANSIENT COMMAND.COM (bead|
|`os/milton/xcopy_tree.c:1`|510|XCOPY and TREE, native flat .COM-equivalent programs (initech-zuvo).|
|`os/milton/xcopy_tree.h:1`|13|XCOPY/TREE flat-program seam. Ref: ADR-0003 Appendix A, DEC-08;|
|`os/milton/xcopy_tree.ld:1`|15|ADR-0003 DEC-08 flat image; spec/memory_map.h PROGRAM_IMAGE. */|
|`os/milton/xcopy_tree_entry.asm:1`|62|Flat .COM-equivalent C entry / register bridge. Ref: ADR-0003 DEC-08,|

### Contracts, fixture/data and ancillary files owned here

Runtime files in the requested `os/`, `seed/`, `spec/`, `fixtures/` trees are assigned; os/README.md and os/apps/.gitkeep remain explicit unassigned placeholders in SUMMARY. `spec/*.mk` input traces are test data to preserve separately from their obsolete build syntax.

|File|Lines (text)|Role|
|---|---:|---|
|`os/milton/test_batch.c:1`|425|host unit oracle for the .BAT parser/expander (batch.c).|
|`os/milton/test_batch_exec.c:1`|338|host unit oracle for the .BAT EXECUTION-logic helpers|
|`os/milton/test_cmdline_fuzz.c:1`|323|deterministic, seeded generative fuzzer for the|
|`os/milton/test_command.c:1`|758|host unit oracle for the PURE COMMAND.COM shell logic|
|`os/milton/test_env.c:1`|362|host unit oracle for the environment store (env.c / env.h).|
|`os/milton/test_redir_parse.c:1`|506|host unit oracle for cmd_redir_parse, the PURE COMMAND.COM|
|`spec/dos_autoexec_bat_baseline.txt:1`|5|PATH C:\DOS;C:\INITECH|
|`spec/dos_banner.txt:1`|2|InitechDOS  Version 3.30|
|`spec/dos_folder_apps.txt:1`|17|dos_folder_apps.txt; source/contract input (role inferred from path)|
|`spec/dos_folder_commands.txt:1`|1|DIR A:\DOS >DOS.LST\nTYPE DOS.LST\nPATH\nCHKDSK >CHK.OUT\nMORE <README.TXT\nMORE <README.TXT >MORE.OUT\nECHO DOS-FOLDER-DONE\n|
|`spec/dos_folder_externals.txt:1`|14|dos_folder_externals.txt; source/contract input (role inferred from path)|
|`spec/dos_folder_traces.mk:1`|15|initech-zc68 / 2kgh, locked before the data-volume change (Rule 1/8).|
|`spec/dos_messages.json:1`|123|"_comment": [|

## 2. Headers it needs from elsewhere

Literal include sites, including host-only conditional includes embedded in production TUs. PUBLIC INTERFACE means publish/move the contract or supported reusable library header; INTERNAL LEAK means replace a foreign implementation dependency. This is not an assertion that every conditional include is active in the production build.

|Include site|Spelling → resolved path|Classification|
|---|---|---|
|`os/milton/chkdsk.c:12`|`dos_messages.h` → `build/dos_messages.h`|PUBLIC INTERFACE|
|`os/milton/chkdsk.c:13`|`dospath.h` → `os/milton/dospath.h`|INTERNAL LEAK|
|`os/milton/chkdsk.h:9`|`blockdev.h` → `os/milton/blockdev.h`|PUBLIC INTERFACE|
|`os/milton/chkdsk.h:10`|`mcb.h` → `os/milton/mcb.h`|INTERNAL LEAK|
|`os/milton/chkdsk_program.c:7`|`memory_map.h` → `spec/memory_map.h`|PUBLIC INTERFACE|
|`os/milton/command.c:18`|`dos_structs.h` → `spec/dos_structs.h`|PUBLIC INTERFACE|
|`os/milton/command.c:19`|`dos_messages.h` → `build/dos_messages.h`|PUBLIC INTERFACE|
|`os/milton/command.c:1767`|`find_data.h` → `spec/find_data.h`|PUBLIC INTERFACE|
|`os/milton/command.c:1768`|`int21.h` → `os/milton/int21.h`|INTERNAL LEAK|
|`os/milton/command.c:1769`|`memory_map.h` → `spec/memory_map.h`|PUBLIC INTERFACE|
|`os/milton/command.c:1771`|`fileio_fat.h` → `os/milton/fileio_fat.h`|INTERNAL LEAK|
|`os/milton/command.c:3067`|`shell_files_host.h` → `harness/diff/fat_diff/shell_files_host.h`|INTERNAL LEAK|
|`os/milton/command.c:3070`|`batch_break_host.h` → `harness/diff/fat_diff/batch_break_host.h`|INTERNAL LEAK|
|`os/milton/diskutil.c:7`|`dos_messages.h` → `build/dos_messages.h`|PUBLIC INTERFACE|
|`os/milton/diskutil_program.c:4`|`drive_letters.h` → `spec/drive_letters.h`|PUBLIC INTERFACE|
|`os/milton/diskutil_program.c:5`|`memory_map.h` → `spec/memory_map.h`|PUBLIC INTERFACE|
|`os/milton/dosutil.c:15`|`dos_messages.h` → `build/dos_messages.h`|PUBLIC INTERFACE|
|`os/milton/dosutil.c:16`|`dospath.h` → `os/milton/dospath.h`|INTERNAL LEAK|
|`os/milton/dosutil_target.c:4`|`find_data.h` → `spec/find_data.h`|PUBLIC INTERFACE|
|`os/milton/edlin.c:9`|`conedit.h` → `os/milton/conedit.h`|INTERNAL LEAK|
|`os/milton/edlin.c:10`|`dos_messages.h` → `build/dos_messages.h`|PUBLIC INTERFACE|
|`os/milton/edlin_program.c:4`|`memory_map.h` → `spec/memory_map.h`|PUBLIC INTERFACE|
|`os/milton/fdisk_program.c:7`|`drive_letters.h` → `spec/drive_letters.h`|PUBLIC INTERFACE|
|`os/milton/fdisk_program.c:8`|`power.h` → `os/milton/power.h`|INTERNAL LEAK|
|`os/milton/find_program.asm:1`|`dos_messages.inc` → `build/dos_messages.inc`|PUBLIC INTERFACE|
|`os/milton/sort_program.asm:1`|`dos_messages.inc` → `build/dos_messages.inc`|PUBLIC INTERFACE|
|`os/milton/transient_hdr.asm:18`|`transient_imports_ext.inc` → `UNRESOLVED`|INTERNAL LEAK|
|`os/milton/transient_hdr.asm:34`|`transient_imports_dd.inc` → `UNRESOLVED`|INTERNAL LEAK|
|`os/milton/xcopy_tree.c:22`|`find_data.h` → `spec/find_data.h`|PUBLIC INTERFACE|
|`os/milton/xcopy_tree.c:23`|`dos_messages.h` → `build/dos_messages.h`|PUBLIC INTERFACE|
|`os/milton/xcopy_tree.c:24`|`dospath.h` → `os/milton/dospath.h`|INTERNAL LEAK|

### Generated inputs

|Header|Generator inputs / recipe origin|
|---|---|
|`build/dos_messages.inc`|`build/dos_messages.h | build`; #  recipe to execute (from 'Makefile', line 28357):|
|`build/dos_messages.h`|`spec/dos_messages.json | build`; #  recipe to execute (from 'Makefile', line 28325):|

### Checked-in generated header provenance

These generation facts supplement section 2; checked-in output is an input during the current build, not necessarily regenerated by make.

|Header|Generator|Generator inputs|
|---|---|---|
|`spec/drive_letters.h:1`|`tools/gen_drive_letters.c:1`|spec/drive_letters.json|

## 3. How it is built today

`-m32 -march=i386` pins the 32-bit 386-compatible ISA used on the emulated 486. Freestanding/no-libc linkage and entry/linker addresses remain required until packaging changes. `-fno-pic -fno-pie` agrees with flat placement and relocation derivation. Kernel `-mregparm=3 -mrtd -freg-struct-return` was introduced to save bytes, but it is now an ABI: keep C/assembly declarations and all linked translation units in agreement until changing both sides. Do not simply delete it from one file. INT 81h arguments remain explicit dword cdecl with `TBX_TRAP_ABI` exceptions. Preserve `-fno-tree-loop-distribute-patterns` where it prevents freestanding code acquiring libc calls, or supply those functions. `-Oz`, per-object `-Os`, LTO/partition choices, `-fno-inline`, per-pass `-fno-*` lists, `-mtune=generic`, `-malign-data=abi`, `--param` peeling limits and 4-byte stack tuning were selected for legacy caps; retire after replacing caps and verifying the coherent ABI. `-fno-strict-aliasing` and `-fno-common` are correctness/toolchain choices, not automatically disposable size levers.

Commands below are captured output of `make -n -B --trace ...`, never executed. `-o build/flair_dac.stamp.mk` suppresses the parse-time included-makefile remake. Empty headroom-enumeration overrides only suppress irrelevant recursive database enumeration. Shell loops preserve exact input/link order; variables like `$objs` are runtime shell variables, not omitted make flags.

### dry-production-dos.txt
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/chkdsk_program.bin build/attrib_program.bin build/label_program.bin build/xcopy_program.bin build/tree_program.bin build/edlin_program.bin build/more_program.bin build/find_program.bin build/sort_program.bin build/fdisk_program.bin build/format_program.bin build/sys_program.bin build/diskcopy_program.bin build/diskcomp_program.bin build/tpsrep_program.bin
# exit: 0
Makefile:12268: target 'build' does not exist
mkdir -p build
Makefile:76: update target 'build/.makefile.stamp' due to: Makefile os/samir/cmd/flow_verbs.def os/samir/cmd/mutate_verbs.def os/samir/cmd/proc_verbs.def os/samir/cmd/query_verbs.def os/samir/cmd/set_options.def harness/diff/fat_diff/doslook.mk harness/diff/fat_diff/xcopy_tree.mk harness/diff/fat_diff/copy_stamp.mk tools/serial_trace_pack.c os/milton/kmain_cold.h os/flair/chicago_packed.h harness/diff/fat_diff/shell_files.mk
mkdir -p build/ && touch build/.makefile.stamp
harness/diff/fat_diff/chkdsk.mk:18: update target 'build/chkdsk_start.o' due to: os/milton/chkdsk_start.asm
nasm -f elf32 os/milton/chkdsk_start.asm -o build/chkdsk_start.o
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
harness/diff/fat_diff/chkdsk.mk:20: update target 'build/chkdsk_core.o' due to: os/milton/chkdsk.c os/milton/chkdsk.h os/milton/dospath.h build/dos_messages.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -Os -Ios/milton -Ispec -Iharness/diff/fat_diff -Ibuild -c os/milton/chkdsk.c -o build/chkdsk_core.o
harness/diff/fat_diff/chkdsk.mk:22: update target 'build/chkdsk_program.o' due to: os/milton/chkdsk_program.c os/milton/chkdsk.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -Os -Ios/milton -Ispec -Iharness/diff/fat_diff -Ibuild -c os/milton/chkdsk_program.c -o build/chkdsk_program.o
Makefile:8979: update target 'build/mcb.o' due to: os/milton/mcb.c os/milton/mcb.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -Ispec -Ios/milton -c os/milton/mcb.c -o build/mcb.o
harness/diff/fat_diff/chkdsk.mk:24: update target 'build/chkdsk.elf' due to: build/chkdsk_start.o build/chkdsk_core.o build/chkdsk_program.o build/mcb.o os/milton/chkdsk.ld
ld -m elf_i386 -T os/milton/chkdsk.ld -o build/chkdsk.elf build/chkdsk_start.o build/chkdsk_core.o build/chkdsk_program.o build/mcb.o
harness/diff/fat_diff/chkdsk.mk:26: update target 'build/chkdsk_program.bin' due to: build/chkdsk.elf
objcopy -O binary build/chkdsk.elf build/chkdsk_program.bin
harness/diff/fat_diff/dosutil.mk:32: update target 'build/attrib_core.o' due to: os/milton/dosutil.c os/milton/dosutil.h os/milton/dospath.h build/dos_messages.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -Ios/milton -Ispec -Ibuild  -c os/milton/dosutil.c -o build/attrib_core.o
harness/diff/fat_diff/dosutil.mk:32: update target 'build/attrib_target.o' due to: os/milton/dosutil_target.c os/milton/dosutil.h os/milton/dospath.h spec/find_data.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -Ios/milton -Ispec -Ibuild -DDOSUTIL_ATTRIB -c os/milton/dosutil_target.c -o build/attrib_target.o
harness/diff/fat_diff/dosutil.mk:32: update target 'build/attrib_program.elf' due to: build/attrib_core.o build/attrib_target.o os/milton/dosutil.ld
ld -m elf_i386 --gc-sections -T os/milton/dosutil.ld -o build/attrib_program.elf build/attrib_target.o build/attrib_core.o
harness/diff/fat_diff/dosutil.mk:32: update target 'build/attrib_program.bin' due to: build/attrib_program.elf
objcopy -O binary build/attrib_program.elf build/attrib_program.bin
harness/diff/fat_diff/dosutil.mk:33: update target 'build/label_core.o' due to: os/milton/dosutil.c os/milton/dosutil.h os/milton/dospath.h build/dos_messages.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -Ios/milton -Ispec -Ibuild  -c os/milton/dosutil.c -o build/label_core.o
harness/diff/fat_diff/dosutil.mk:33: update target 'build/label_target.o' due to: os/milton/dosutil_target.c os/milton/dosutil.h os/milton/dospath.h spec/find_data.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -Ios/milton -Ispec -Ibuild  -c os/milton/dosutil_target.c -o build/label_target.o
harness/diff/fat_diff/dosutil.mk:33: update target 'build/label_program.elf' due to: build/label_core.o build/label_target.o os/milton/dosutil.ld
ld -m elf_i386 --gc-sections -T os/milton/dosutil.ld -o build/label_program.elf build/label_target.o build/label_core.o
harness/diff/fat_diff/dosutil.mk:33: update target 'build/label_program.bin' due to: build/label_program.elf
objcopy -O binary build/label_program.elf build/label_program.bin
harness/diff/fat_diff/xcopy_tree.mk:29: update target 'build/xcopy_program.o' due to: os/milton/xcopy_tree.c os/milton/xcopy_tree.h os/milton/dospath.h build/dos_messages.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -fno-builtin -fno-asynchronous-unwind-tables -fno-unwind-tables -Ios/milton -Ispec -Ibuild  -c os/milton/xcopy_tree.c -o build/xcopy_program.o
harness/diff/fat_diff/xcopy_tree.mk:29: update target 'build/xcopy_entry.o' due to: os/milton/xcopy_tree_entry.asm
nasm -f elf32  os/milton/xcopy_tree_entry.asm -o build/xcopy_entry.o
harness/diff/fat_diff/xcopy_tree.mk:29: update target 'build/xcopy_program.elf' due to: build/xcopy_program.o build/xcopy_entry.o os/milton/xcopy_tree.ld
ld -m elf_i386 --no-warn-rwx-segments -T os/milton/xcopy_tree.ld -o build/xcopy_program.elf build/xcopy_entry.o build/xcopy_program.o
harness/diff/fat_diff/xcopy_tree.mk:29: update target 'build/xcopy_program.bin' due to: build/xcopy_program.elf
objcopy -O binary build/xcopy_program.elf build/xcopy_program.bin
harness/diff/fat_diff/xcopy_tree.mk:30: update target 'build/tree_program.o' due to: os/milton/xcopy_tree.c os/milton/xcopy_tree.h os/milton/dospath.h build/dos_messages.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -fno-builtin -fno-asynchronous-unwind-tables -fno-unwind-tables -Ios/milton -Ispec -Ibuild  -c os/milton/xcopy_tree.c -o build/tree_program.o
harness/diff/fat_diff/xcopy_tree.mk:30: update target 'build/tree_entry.o' due to: os/milton/xcopy_tree_entry.asm
nasm -f elf32 -DTREE_PROGRAM os/milton/xcopy_tree_entry.asm -o build/tree_entry.o
harness/diff/fat_diff/xcopy_tree.mk:30: update target 'build/tree_program.elf' due to: build/tree_program.o build/tree_entry.o os/milton/xcopy_tree.ld
ld -m elf_i386 --no-warn-rwx-segments -T os/milton/xcopy_tree.ld -o build/tree_program.elf build/tree_entry.o build/tree_program.o
harness/diff/fat_diff/xcopy_tree.mk:30: update target 'build/tree_program.bin' due to: build/tree_program.elf
objcopy -O binary build/tree_program.elf build/tree_program.bin
harness/edlin/edlin.mk:16: update target 'build/edlin_start.o' due to: os/milton/edlin_start.asm
nasm -f elf32 os/milton/edlin_start.asm -o build/edlin_start.o
harness/edlin/edlin.mk:20: update target 'build/edlin_core.o' due to: os/milton/edlin.c os/milton/edlin.h os/milton/conedit.h build/dos_messages.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -Ios/milton -Ibuild -c os/milton/edlin.c -o build/edlin_core.o
harness/edlin/edlin.mk:18: update target 'build/edlin_program.o' due to: os/milton/edlin_program.c os/milton/edlin.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -Ios/milton -Ispec -c os/milton/edlin_program.c -o build/edlin_program.o
harness/edlin/edlin.mk:22: update target 'build/edlin.elf' due to: build/edlin_start.o build/edlin_core.o build/edlin_program.o os/milton/edlin.ld
ld -m elf_i386 -T os/milton/edlin.ld -o build/edlin.elf build/edlin_start.o build/edlin_core.o build/edlin_program.o
harness/edlin/edlin.mk:24: update target 'build/edlin_program.bin' due to: build/edlin.elf
objcopy -O binary build/edlin.elf build/edlin_program.bin
Makefile:2145: update target 'build/more_program.bin' due to: os/milton/more_program.asm
nasm -f bin os/milton/more_program.asm -o build/more_program.bin
Makefile:28357: update target 'build/dos_messages.inc' due to: build/dos_messages.h
sed -n 's/^#define \(MSG_DOS_[0-9]*\) "\(.*\)"$/%define \1 `\2`/p' build/dos_messages.h > build/dos_messages.inc
Makefile:2141: update target 'build/find_program.bin' due to: os/milton/find_program.asm build/dos_messages.inc
nasm -Ibuild/ -f bin os/milton/find_program.asm -o build/find_program.bin
Makefile:2137: update target 'build/sort_program.bin' due to: os/milton/sort_program.asm build/dos_messages.inc
nasm -Ibuild/ -f bin os/milton/sort_program.asm -o build/sort_program.bin
harness/diff/fat_diff/fdisk.mk:22: update target 'build/fdisk_start.o' due to: os/milton/fdisk_start.asm
nasm -f elf32 os/milton/fdisk_start.asm -o build/fdisk_start.o
harness/diff/fat_diff/fdisk.mk:24: update target 'build/fdisk_program.o' due to: os/milton/fdisk_program.c os/milton/fdisk.h os/milton/power.h os/milton/diskutil.h spec/drive_letters.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -Ispec -Ios/milton -c os/milton/fdisk_program.c -o build/fdisk_program.o
harness/diff/fat_diff/fdisk.mk:26: update target 'build/fdisk.o' due to: os/milton/fdisk.c os/milton/fdisk.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -Ispec -Ios/milton -c os/milton/fdisk.c -o build/fdisk.o
harness/diff/fat_diff/fdisk.mk:28: update target 'build/fdisk_program.elf' due to: build/fdisk_start.o build/fdisk_program.o build/fdisk.o os/milton/fdisk.ld
ld -m elf_i386 -T os/milton/fdisk.ld -o build/fdisk_program.elf build/fdisk_start.o build/fdisk_program.o build/fdisk.o
harness/diff/fat_diff/fdisk.mk:30: update target 'build/fdisk_program.bin' due to: build/fdisk_program.elf
objcopy -O binary build/fdisk_program.elf build/fdisk_program.bin
harness/diff/fat_diff/formatsys.mk:32: update target 'build/diskutil_start.o' due to: os/milton/diskutil_start.asm
nasm -f elf32 os/milton/diskutil_start.asm -o build/diskutil_start.o
harness/diff/fat_diff/formatsys.mk:43: update target 'build/format_adapter.o' due to: os/milton/diskutil_program.c os/milton/diskutil.h spec/drive_letters.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -DDU_KIND=0 -Ispec -Ios/milton -c os/milton/diskutil_program.c -o build/format_adapter.o
harness/diff/fat_diff/formatsys.mk:34: update target 'build/diskutil.o' due to: os/milton/diskutil.c os/milton/diskutil.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -Ispec -Ios/milton -c os/milton/diskutil.c -o build/diskutil.o
harness/diff/fat_diff/formatsys.mk:43: update target 'build/format_program.elf' due to: build/diskutil_start.o build/format_adapter.o build/diskutil.o os/milton/fdisk.ld
ld -m elf_i386 -T os/milton/fdisk.ld -o build/format_program.elf build/diskutil_start.o build/format_adapter.o build/diskutil.o
harness/diff/fat_diff/formatsys.mk:43: update target 'build/format_program.bin' due to: build/format_program.elf
objcopy -O binary build/format_program.elf build/format_program.bin
harness/diff/fat_diff/formatsys.mk:44: update target 'build/sys_adapter.o' due to: os/milton/diskutil_program.c os/milton/diskutil.h spec/drive_letters.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -DDU_KIND=1 -Ispec -Ios/milton -c os/milton/diskutil_program.c -o build/sys_adapter.o
harness/diff/fat_diff/formatsys.mk:44: update target 'build/sys_program.elf' due to: build/diskutil_start.o build/sys_adapter.o build/diskutil.o os/milton/fdisk.ld
ld -m elf_i386 -T os/milton/fdisk.ld -o build/sys_program.elf build/diskutil_start.o build/sys_adapter.o build/diskutil.o
harness/diff/fat_diff/formatsys.mk:44: update target 'build/sys_program.bin' due to: build/sys_program.elf
objcopy -O binary build/sys_program.elf build/sys_program.bin
harness/diff/fat_diff/formatsys.mk:45: update target 'build/diskcopy_adapter.o' due to: os/milton/diskutil_program.c os/milton/diskutil.h spec/drive_letters.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -DDU_KIND=2 -Ispec -Ios/milton -c os/milton/diskutil_program.c -o build/diskcopy_adapter.o
harness/diff/fat_diff/formatsys.mk:45: update target 'build/diskcopy_program.elf' due to: build/diskutil_start.o build/diskcopy_adapter.o build/diskutil.o os/milton/fdisk.ld
ld -m elf_i386 -T os/milton/fdisk.ld -o build/diskcopy_program.elf build/diskutil_start.o build/diskcopy_adapter.o build/diskutil.o
harness/diff/fat_diff/formatsys.mk:45: update target 'build/diskcopy_program.bin' due to: build/diskcopy_program.elf
objcopy -O binary build/diskcopy_program.elf build/diskcopy_program.bin
harness/diff/fat_diff/formatsys.mk:46: update target 'build/diskcomp_adapter.o' due to: os/milton/diskutil_program.c os/milton/diskutil.h spec/drive_letters.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -Os -DDU_KIND=3 -Ispec -Ios/milton -c os/milton/diskutil_program.c -o build/diskcomp_adapter.o
harness/diff/fat_diff/formatsys.mk:46: update target 'build/diskcomp_program.elf' due to: build/diskutil_start.o build/diskcomp_adapter.o build/diskutil.o os/milton/fdisk.ld
ld -m elf_i386 -T os/milton/fdisk.ld -o build/diskcomp_program.elf build/diskutil_start.o build/diskcomp_adapter.o build/diskutil.o
harness/diff/fat_diff/formatsys.mk:46: update target 'build/diskcomp_program.bin' due to: build/diskcomp_program.elf
objcopy -O binary build/diskcomp_program.elf build/diskcomp_program.bin
harness/emu/fcb_tps.mk:174: update target 'build/tpsrep_program.bin' due to: os/milton/tpsrep_program.asm
nasm -f bin os/milton/tpsrep_program.asm -o build/tpsrep_program.bin

# stderr

```

### dry-dos-transient.txt
```sh
# argv: make -n -B --trace -o build/flair_dac.stamp.mk KERNEL_HEADROOM_BINS= KERNEL_SIZE_OVL_BINS= build/transient_flairtenants.bin build/transient_flairtenants_int.bin
# exit: 0
Makefile:12268: target 'build' does not exist
mkdir -p build
Makefile:76: update target 'build/.makefile.stamp' due to: Makefile os/samir/cmd/flow_verbs.def os/samir/cmd/mutate_verbs.def os/samir/cmd/proc_verbs.def os/samir/cmd/query_verbs.def os/samir/cmd/set_options.def harness/diff/fat_diff/doslook.mk harness/diff/fat_diff/xcopy_tree.mk harness/diff/fat_diff/copy_stamp.mk tools/serial_trace_pack.c os/milton/kmain_cold.h os/flair/chicago_packed.h harness/diff/fat_diff/shell_files.mk
mkdir -p build/ && touch build/.makefile.stamp
Makefile:8857: update target 'build/kstart.o' due to: os/milton/kstart.asm Makefile spec/memory_map.h
nasm -f elf32 -DMK_KERNEL_SECTORS=384 -DMK_KERNEL_STACK_BOT=0x005F0000 -DMK_KERNEL_STACK_TOP=0x005FFFFC os/milton/kstart.asm -o build/kstart.o
Makefile:8949: update target 'build/console.o' due to: os/milton/console.c os/milton/console.h os/milton/boot_info.h os/flair/surface.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/milton -Ios/flair -c os/milton/console.c -o build/console.o
harness/proptest/kernel_size_r4.mk:165: update target 'build/kmain_cold.o' due to: os/milton/kmain_cold.c os/milton/kmain_cold.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/milton -c os/milton/kmain_cold.c -o build/kmain_cold.o
Makefile:8866: update target 'build/surface.o' due to: os/flair/surface.c os/flair/surface.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -c os/flair/surface.c -o build/surface.o
Makefile:8953: update target 'build/idt.o' due to: os/milton/idt.c os/milton/idt.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing -fomit-frame-pointer -Ios/milton -c os/milton/idt.c -o build/idt.o
Makefile:8956: update target 'build/pic.o' due to: os/milton/pic.c os/milton/pic.h os/milton/io.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/milton -c os/milton/pic.c -o build/pic.o
Makefile:8959: update target 'build/panic.o' due to: os/milton/panic.c os/milton/idt.h os/milton/io.h os/milton/console.h os/milton/panic_screen.h os/milton/boot_info.h os/milton/panic.h os/flair/film_startup.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi --param=min-pagesize=0 -Ios/milton -c os/milton/panic.c -o build/panic.o
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
Makefile:8967: update target 'build/int21.o' due to: os/milton/int21.c os/milton/int21.h os/milton/conedit.h os/milton/idt.h os/milton/sft.h os/milton/psp.h os/milton/mcb.h os/milton/ata.h spec/dos_structs.h build/dos_messages.h os/milton/ansi.c os/milton/ansi.h os/milton/native.c spec/initechmz_extended.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -Ibuild -c os/milton/int21.c -o build/int21.o
Makefile:8973: update target 'build/devices.o' due to: os/milton/devices.c os/milton/devices.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/devices.c -o build/devices.o
Makefile:8979: update target 'build/mcb.o' due to: os/milton/mcb.c os/milton/mcb.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -Ispec -Ios/milton -c os/milton/mcb.c -o build/mcb.o
Makefile:8985: update target 'build/psp.o' due to: os/milton/psp.c os/milton/psp.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/psp.c -o build/psp.o
Makefile:8991: update target 'build/sft.o' due to: os/milton/sft.c os/milton/sft.h os/milton/psp.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/sft.c -o build/sft.o
Makefile:8996: update target 'build/config_sys.o' due to: os/milton/config_sys.c os/milton/config_sys.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/config_sys.c -o build/config_sys.o
Makefile:9005: update target 'build/sysinit.o' due to: os/milton/sysinit.c os/milton/sysinit.h os/milton/fpu.h spec/fpu_contract.h os/milton/config_sys.h os/milton/sft.h os/milton/psp.h os/milton/int21.h os/milton/pic.h os/milton/idt.h os/milton/pit.h os/milton/kbd.h os/milton/fat12.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/sysinit.c -o build/sysinit.o
Makefile:10832: update target 'build/loader_tenant.o' due to: os/milton/loader.c os/milton/loader_cold.h os/milton/loader.h os/milton/mz.c spec/initechmz_extended.h spec/memory_map.h os/milton/loader_cold.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DLOADER_COLD_EXTERNAL -Ispec -Ios/milton -c os/milton/loader.c -o build/loader_tenant.o
Makefile:9019: update target 'build/ata.o' due to: os/milton/ata.c os/milton/ata.h os/milton/blockdev.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/milton -c os/milton/ata.c -o build/ata.o
harness/diff/fat_diff/block_device.mk:24: update target 'build/block_device.o' due to: os/milton/block_device.c os/milton/block_device.h spec/block_device_contract.h spec/drive_letters.h os/milton/diskutil.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/block_device.c -o build/block_device.o
Makefile:9025: update target 'build/fat12.o' due to: os/milton/fat12.c os/milton/fat12.h os/milton/blockdev.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/fat12.c -o build/fat12.o
Makefile:9034: update target 'build/fileio_fat.o' due to: os/milton/fileio_fat.c os/milton/fileio_fat.h os/milton/fat12.h os/milton/int21.h os/milton/blockdev.h spec/dos_structs.h spec/memory_map.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/fileio_fat.c -o build/fileio_fat.o
Makefile:9058: update target 'build/kbd.o' due to: os/milton/kbd.c os/milton/kbd.h os/milton/io.h spec/tsr_contract.h os/flair/ring_report.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing -fomit-frame-pointer -DKERNEL_PURE_DECODE_OZ -Ios/milton -c os/milton/kbd.c -o build/kbd.o
Makefile:9064: update target 'build/mouse.o' due to: os/milton/mouse.c os/milton/mouse.h os/milton/mouse_pack.h os/milton/io.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/milton -c os/milton/mouse.c -o build/mouse.o
Makefile:9074: update target 'build/pit.o' due to: os/milton/pit.c os/milton/pit.h os/milton/io.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/milton -c os/milton/pit.c -o build/pit.o
Makefile:9084: update target 'build/rtc.o' due to: os/milton/rtc.c os/milton/rtc.h os/milton/io.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -Ios/milton -c os/milton/rtc.c -o build/rtc.o
Makefile:9090: update target 'build/irq.o' due to: os/milton/irq.c os/milton/irq.h os/milton/io.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/milton -c os/milton/irq.c -o build/irq.o
Makefile:9121: update target 'build/test_program.bin' due to: os/milton/test_program.asm
nasm -f bin os/milton/test_program.asm -o build/test_program.bin
Makefile:9116: update target 'build/bin2c' due to: tools/bin2c.c
cc -std=c11 -Wall -Wextra -Werror -o build/bin2c tools/bin2c.c
Makefile:9126: update target 'build/test_prog_blob.c' due to: build/test_program.bin build/bin2c
build/bin2c build/test_program.bin g_test_prog_image > build/test_prog_blob.c
Makefile:9129: update target 'build/test_prog_blob.o' due to: build/test_prog_blob.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Ios/milton -c build/test_prog_blob.c -o build/test_prog_blob.o
Makefile:9134: update target 'build/type_program.bin' due to: os/milton/type_program.asm
nasm -f bin os/milton/type_program.asm -o build/type_program.bin
Makefile:9137: update target 'build/type_prog_blob.c' due to: build/type_program.bin build/bin2c
build/bin2c build/type_program.bin g_type_prog_image > build/type_prog_blob.c
Makefile:9140: update target 'build/type_prog_blob.o' due to: build/type_prog_blob.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Ios/milton -c build/type_prog_blob.c -o build/type_prog_blob.o
Makefile:9143: update target 'build/dir_program.bin' due to: os/milton/dir_program.asm
nasm -f bin os/milton/dir_program.asm -o build/dir_program.bin
Makefile:9146: update target 'build/dir_prog_blob.c' due to: build/dir_program.bin build/bin2c
build/bin2c build/dir_program.bin g_dir_prog_image > build/dir_prog_blob.c
Makefile:9149: update target 'build/dir_prog_blob.o' due to: build/dir_prog_blob.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Ios/milton -c build/dir_prog_blob.c -o build/dir_prog_blob.o
Makefile:9228: update target 'build/isr.o' due to: os/milton/isr.asm
nasm -f elf32 os/milton/isr.asm -o build/isr.o
Makefile:8887: update target 'build/region.o' due to: os/flair/atkinson/region.c os/flair/atkinson/region.h spec/region_algebra.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -Ispec -Ios/flair/atkinson -Iseed -c os/flair/atkinson/region.c -o build/region.o
Makefile:8890: update target 'build/heap.o' due to: os/flair/heap.c os/flair/heap.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Iseed -c os/flair/heap.c -o build/heap.o
Makefile:8893: update target 'build/event.o' due to: os/flair/event.c os/flair/event.h spec/event_model.h spec/grafport.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -Ios/flair -Ispec -Iseed -c os/flair/event.c -o build/event.o
Makefile:8896: update target 'build/cursor.o' due to: os/flair/cursor.c os/flair/cursor.h os/flair/surface.h spec/assets/cursors.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ispec/assets -c os/flair/cursor.c -o build/cursor.o
Makefile:8907: update target 'build/window_live.o' due to: os/flair/window.c os/flair/window.h os/flair/winscroll.h os/flair/atkinson/region.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/chrome_metrics.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DWINDOW_ENABLE_R1_OPS -Ispec -Ios/flair -Ios/flair/atkinson -Iseed -c os/flair/window.c -o build/window_live.o
Makefile:8910: update target 'build/blitter.o' due to: os/flair/blitter.c os/flair/blitter.h os/flair/atkinson/region.h os/flair/surface.h spec/region_algebra.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -Ispec -Ios/flair -Ios/flair/atkinson -Iseed -c os/flair/blitter.c -o build/blitter.o
Makefile:12043: update target 'build/palette_extract' due to: tools/palette_extract.c
cc -std=c11 -Wall -Wextra -Werror -o build/palette_extract tools/palette_extract.c
Makefile:12063: update target 'spec/assets/palette.h' due to: spec/assets/palette.json build/palette_extract
build/palette_extract --header spec/assets/palette.json > spec/assets/palette.h.tmp.$$ \
	&& { if cmp -s spec/assets/palette.h.tmp.$$ spec/assets/palette.h; then rm -f spec/assets/palette.h.tmp.$$; \
	     else mv -f spec/assets/palette.h.tmp.$$ spec/assets/palette.h && printf ">>> assets: regenerated %s from %s\n" "spec/assets/palette.h" "spec/assets/palette.json"; fi; } \
	|| { rm -f spec/assets/palette.h.tmp.$$; exit 1; }
Makefile:8918: update target 'build/chrome.o' due to: os/flair/chrome.c os/flair/chrome.h os/flair/winscroll.h os/flair/text.h spec/assets/font_linkage.h spec/assets/chicago12.h spec/assets/geneva9.h spec/assets/geneva12.h os/flair/flair_look.h spec/chrome_metrics.h spec/grafport.h spec/imaging.h spec/region_algebra.h spec/assets/palette.h os/flair/rdoc.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -c os/flair/chrome.c -o build/chrome.o
harness/proptest/kernel_size_r4.mk:120: update target 'build/chicago_pack' due to: tools/chicago_pack.c spec/assets/chicago12.h
cc -std=c11 -Wall -Wextra -Werror -Ispec/assets -o build/chicago_pack tools/chicago_pack.c
harness/proptest/kernel_size_r4.mk:122: update target 'build/chicago_packed_data.h' due to: build/chicago_pack spec/assets/chicago12.h
build/chicago_pack >build/chicago_packed_data.h.tmp && mv -f build/chicago_packed_data.h.tmp build/chicago_packed_data.h
harness/proptest/geneva12.mk:49: update target 'build/geneva12_pack' due to: tools/chicago_pack.c spec/assets/geneva12.h spec/assets/font_linkage.h
cc -std=c11 -Wall -Wextra -Werror -DPACK_GENEVA12 -Ispec/assets -o build/geneva12_pack tools/chicago_pack.c
harness/proptest/geneva12.mk:51: update target 'build/geneva12_packed_data.h' due to: build/geneva12_pack spec/assets/geneva12.h
build/geneva12_pack >build/geneva12_packed_data.h.tmp && mv -f build/geneva12_packed_data.h.tmp build/geneva12_packed_data.h
Makefile:8921: update target 'build/text.o' due to: os/flair/text.c os/flair/text.h spec/assets/font_linkage.h spec/assets/chicago12.h spec/assets/geneva9.h spec/assets/geneva12.h os/flair/surface.h os/flair/chicago_packed.h build/chicago_packed_data.h os/flair/geneva12_packed.h build/geneva12_packed_data.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-strict-aliasing -Ios/flair -Ispec/assets -Ispec -Iseed -c os/flair/text.c -o build/text.o
Makefile:8927: update target 'build/menu.o' due to: os/flair/menu.c os/flair/menu.h os/flair/flair_look.h spec/assets/menu_canon.h spec/assets/apple_glyph.h spec/chrome_metrics.h spec/grafport.h spec/imaging.h spec/region_algebra.h spec/assets/palette.h spec/assets/chicago12.h spec/assets/geneva9.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/menu.c -o build/menu.o
Makefile:8930: update target 'build/control.o' due to: os/flair/control.c os/flair/control.h os/flair/flair_look.h spec/chrome_metrics.h spec/grafport.h spec/imaging.h spec/region_algebra.h spec/assets/palette.h spec/assets/chicago12.h spec/progress_platinum.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/control.c -o build/control.o
Makefile:8933: update target 'build/dialog.o' due to: os/flair/dialog.c os/flair/dialog.h spec/assets/alert_icons.h spec/assets/desk_icons.h os/flair/flair_look.h spec/chrome_metrics.h spec/grafport.h spec/event_model.h spec/window_record.h spec/region_algebra.h spec/assets/palette.h spec/assets/chicago12.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/dialog.c -o build/dialog.o
Makefile:8936: update target 'build/desktop.o' due to: os/flair/desktop.c os/flair/desktop.h os/flair/window.h os/flair/winscroll.h os/flair/event.h os/flair/blitter.h os/flair/chrome.h os/flair/surface.h os/flair/heap.h os/flair/atkinson/region.h spec/region_algebra.h spec/window_record.h spec/event_model.h spec/grafport.h spec/imaging.h spec/chrome_metrics.h spec/assets/palette.h os/flair/flair_look.h os/flair/rdoc.h spec/cpanels_record.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -c os/flair/desktop.c -o build/desktop.o
Makefile:8940: update target 'build/flair_look.o' due to: os/flair/flair_look.c os/flair/flair_look.h spec/flair_skins.h spec/assets/color_canon.h os/flair/surface.h spec/grafport.h spec/imaging.h spec/assets/finder_small_icons.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/flair_look.c -o build/flair_look.o
Makefile:8943: update target 'build/flair_shell.o' due to: os/flair/shell.c os/flair/shell.h os/flair/text.h spec/assets/font_linkage.h spec/assets/chicago12.h spec/assets/geneva9.h spec/assets/geneva12.h os/flair/desktop.h os/flair/window.h os/flair/winscroll.h os/flair/menu.h os/flair/dialog.h os/flair/control.h os/flair/chrome.h os/flair/blitter.h os/flair/surface.h os/flair/heap.h os/flair/event.h os/flair/atkinson/region.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/imaging.h spec/chrome_metrics.h spec/assets/menu_canon.h spec/assets/palette.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing -fomit-frame-pointer -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Iharness/render -Iseed -c os/flair/shell.c -o build/flair_shell.o
harness/proptest/kernel_size_r4.mk:86: update target 'build/serial_trace_pack' due to: tools/serial_trace_pack.c
cc -std=c11 -Wall -Wextra -Werror -O2 -o build/serial_trace_pack tools/serial_trace_pack.c
harness/emu/vesa8.mk:22: update target 'build/flair_dac_gen' due to: tools/flair_dac_gen.c
cc -std=c11 -Wall -Wextra -Werror -O2 tools/flair_dac_gen.c -o build/flair_dac_gen
harness/emu/vesa8.mk:24: update target 'build/flair_dac.h' due to: build/flair_dac_gen spec/assets/clut.json spec/assets/color_canon.json
build/flair_dac_gen spec/assets/clut.json spec/assets/color_canon.json > build/flair_dac.h.tmp
mv -f build/flair_dac.h.tmp build/flair_dac.h
Makefile:10814: update target 'build/kmain_flairtenants.o' due to: os/milton/kmain.c os/milton/ansi_cursor.h os/milton/finder_actions.h os/milton/power_cold.h build/serial_trace_pack os/milton/kmain_cold.h os/apps/ref_demo_menu.h os/milton/boot_info.h os/milton/io.h os/milton/console.h os/milton/idt.h os/milton/pic.h os/milton/int21.h os/milton/loader.h os/milton/test_prog.h os/milton/psp.h os/milton/sft.h os/milton/ata.h os/milton/fat12.h os/milton/fileio_fat.h os/milton/desktop_db.h os/milton/blockdev.h os/milton/kbd.h os/milton/mouse.h os/milton/mouse_pack.h os/milton/pit.h os/milton/sysinit.h os/milton/command.h os/flair/heap.h os/flair/surface.h os/flair/shell.h os/flair/desktop.h os/flair/window.h os/flair/winscroll.h os/flair/menu.h os/flair/dialog.h os/flair/control.h os/flair/event.h os/flair/system_key.h os/flair/power_notice.h os/flair/process.h os/flair/finder_desktop.h os/flair/finder_cmd.h os/flair/finder_windows.h os/flair/finder_ops.h os/flair/finder_menu.h os/apps/ref_tenant.h spec/event_model.h spec/memory_map.h spec/dos_structs.h spec/region_algebra.h spec/flair_tenants_demo.h spec/assets/desk_icons.h spec/assets/finder_icons.h spec/assets/menu_canon.h spec/assets/palette.h spec/assets/color_canon.h os/flair/apple_menu.h os/flair/tbxgate.h spec/toolbox_gate.h os/flair/scrap.h os/flair/stdfile.h os/flair/list.h os/flair/text.h os/milton/transient.h build/dos_messages.h os/flair/cursor.h os/milton/boot_screen.h spec/assets/boot_logo.h spec/assets/chicago12.h build/flair_dac.h os/milton/about_box.h os/flair/egg_jtc.h os/flair/egg_jtc_model.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DBOOT_FLAIR_LIVE -DFLAIR_LIVE_TENANTS -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/apps -Ios/milton -E os/milton/kmain.c -o build/kmain_flairtenants.o.i
build/serial_trace_pack build/kmain_flairtenants.o.i build/kmain_flairtenants.o.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DBOOT_FLAIR_LIVE -DFLAIR_LIVE_TENANTS -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/apps -Ios/milton -c build/kmain_flairtenants.o.c -o build/kmain_flairtenants.o
Makefile:10443: update target 'build/process.o' due to: os/flair/process.c os/flair/process.h os/flair/window.h os/flair/winscroll.h os/flair/heap.h os/flair/surface.h os/flair/event.h spec/event_model.h spec/region_algebra.h spec/window_record.h spec/grafport.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/process.c -o build/process.o
Makefile:10449: update target 'build/ref_tenant.o' due to: os/apps/ref_tenant.c os/apps/ref_tenant.h os/flair/process.h os/flair/window.h os/flair/winscroll.h os/flair/blitter.h os/flair/surface.h os/flair/heap.h os/flair/flair_look.h spec/window_record.h spec/region_algebra.h spec/event_model.h spec/chrome_metrics.h spec/flair_tenants_demo.h spec/assets/color_canon.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/apps -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/apps/ref_tenant.c -o build/ref_tenant.o
Makefile:9040: update target 'build/desktop_db.o' due to: os/milton/desktop_db.c os/milton/desktop_db.h os/milton/fat12.h spec/dos_structs.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ispec -Ios/milton -c os/milton/desktop_db.c -o build/desktop_db.o
Makefile:10458: update target 'build/finder_icon.o' due to: os/flair/finder_icon.c os/flair/finder_icon.h os/flair/flair_look.h os/flair/surface.h spec/assets/desk_icons.h spec/region_algebra.h spec/assets/finder_small_icons.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/finder_icon.c -o build/finder_icon.o
Makefile:10462: update target 'build/finder_desktop.o' due to: os/flair/finder_desktop.c os/flair/finder_desktop.h os/flair/finder_icon.h os/flair/window.h os/flair/winscroll.h os/flair/process.h os/flair/heap.h os/flair/blitter.h os/flair/text.h os/flair/surface.h os/flair/flair_look.h spec/assets/desk_icons.h spec/assets/finder_icons.h spec/assets/geneva9.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/event_model.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/finder_desktop.c -o build/finder_desktop.o
Makefile:10470: update target 'build/finder_windows.o' due to: os/flair/finder_windows.c os/flair/finder_windows_cold.c os/flair/finder_windows_private.h os/flair/finder_windows.h os/flair/finder_desktop.h os/flair/finder_cmd.h os/flair/finder_icon.h os/flair/window.h os/flair/winscroll.h os/flair/process.h os/flair/heap.h os/flair/blitter.h os/flair/surface.h os/flair/flair_look.h spec/assets/desk_icons.h spec/assets/finder_icons.h spec/assets/chicago12.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/event_model.h spec/assets/finder_list_titles.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -DFINDER_OVL_RESIDENT -c os/flair/finder_windows.c -o build/finder_windows.o
Makefile:10479: update target 'build/finder_ops.o' due to: os/flair/finder_ops.c os/flair/finder_ops_cold.c os/flair/finder_ops_private.h os/flair/finder_ops.h os/flair/finder_windows.h os/flair/finder_desktop.h os/flair/finder_cmd.h os/flair/window.h os/flair/winscroll.h os/flair/process.h os/flair/heap.h os/flair/surface.h spec/region_algebra.h spec/window_record.h spec/grafport.h spec/event_model.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -DFINDER_OVL_RESIDENT -c os/flair/finder_ops.c -o build/finder_ops.o
Makefile:10486: update target 'build/finder_cmd.o' due to: os/flair/finder_cmd.c os/flair/finder_cmd.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -c os/flair/finder_cmd.c -o build/finder_cmd.o
Makefile:10495: update target 'build/finder_menu.o' due to: os/flair/finder_menu.c os/flair/finder_menu.h os/flair/finder_cmd.h os/flair/menu.h os/flair/text.h os/flair/blitter.h os/flair/surface.h spec/chrome_metrics.h spec/grafport.h spec/region_algebra.h spec/assets/chicago12.h os/flair/finder_windows.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/finder_menu.c -o build/finder_menu.o
Makefile:10424: update target 'build/tbxgate.o' due to: build/serial_trace_pack spec/find_data.h os/flair/cp_service.h spec/cp_dialog.h spec/cpanels_record.h os/flair/scrap.h os/flair/ostype.h os/flair/menu.h os/flair/tbxmenu.h spec/grafport.h os/flair/text.h os/flair/tbxgate.c os/flair/tbxgate.h os/flair/process.h os/flair/window.h os/flair/winscroll.h os/flair/heap.h os/flair/blitter.h os/flair/surface.h os/flair/flair_look.h os/flair/event.h os/milton/loader.h os/milton/psp.h spec/toolbox_gate.h spec/event_model.h spec/region_algebra.h spec/window_record.h spec/chrome_metrics.h spec/assets/chicago12.h spec/assets/geneva12.h os/flair/tbxfile.h os/milton/int21.h os/milton/sft.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ios/milton -Ispec -Ispec/assets -Dtputs=serial_puts -Dtputu=serial_putu -Dtputi=serial_puti -E os/flair/tbxgate.c -o build/tbxgate.o.i
build/serial_trace_pack build/tbxgate.o.i build/tbxgate.o.c tbx
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ios/milton -Ispec -Ispec/assets -c build/tbxgate.o.c -o build/tbxgate.o
Makefile:10426: update target 'build/tbx_gate.o' due to: os/milton/tbx_gate.asm
nasm -f elf32 os/milton/tbx_gate.asm -o build/tbx_gate.o
Makefile:10435: update target 'build/scrap.o' due to: os/flair/scrap.c os/flair/scrap.h os/flair/ostype.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ispec -c os/flair/scrap.c -o build/scrap.o
Makefile:10437: update target 'build/stdfile.o' due to: os/flair/stdfile.c os/flair/stdfile.h os/flair/list.h spec/region_algebra.h spec/grafport.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -c os/flair/stdfile.c -o build/stdfile.o
Makefile:10439: update target 'build/list.o' due to: os/flair/list.c os/flair/list.h spec/region_algebra.h spec/grafport.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -c os/flair/list.c -o build/list.o
Makefile:10822: update target 'build/apple_menu.o' due to: os/flair/apple_menu.c os/flair/apple_menu.h os/flair/menu.h spec/apple_menu_items.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/apple_menu.c -o build/apple_menu.o
harness/proptest/egg_jtc.mk:53: update target 'build/egg_jtc_model.o' due to: os/flair/egg_jtc_model.c os/flair/egg_jtc_model.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -Ibuild -c os/flair/egg_jtc_model.c -o build/egg_jtc_model.o
harness/proptest/egg_jtc.mk:6: update target 'build/egg_jtc_pack' due to: tools/egg_jtc_pack.c os/flair/egg_jtc_model.h
cc -std=c11 -Wall -Wextra -Werror -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -Ibuild -o build/egg_jtc_pack tools/egg_jtc_pack.c
harness/proptest/egg_jtc.mk:8: update target 'build/egg_jtc_data.bin' due to: spec/easter/jump_to_conclusions.json build/egg_jtc_pack
build/egg_jtc_pack spec/easter/jump_to_conclusions.json > build/egg_jtc_data.bin
harness/proptest/egg_jtc.mk:10: update target 'build/egg_jtc_data.c' due to: build/egg_jtc_data.bin build/bin2c
build/bin2c build/egg_jtc_data.bin egg_jtc_data > build/egg_jtc_data.c
harness/proptest/egg_jtc.mk:57: update target 'build/egg_jtc_data.o' due to: build/egg_jtc_data.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -c build/egg_jtc_data.c -o build/egg_jtc_data.o
Makefile:9100: update target 'build/command.o' due to: os/milton/command.c os/milton/command.h os/milton/env.h os/milton/int21.h os/milton/batch.h spec/find_data.h spec/dos_structs.h build/dos_messages.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -DCOMMAND_KERNEL_REPL -Ispec -Ios/milton -Ibuild -c os/milton/command.c -o build/command.o
Makefile:9105: update target 'build/env.o' due to: os/milton/env.c os/milton/env.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Ios/milton -c os/milton/env.c -o build/env.o
Makefile:9111: update target 'build/batch.o' due to: os/milton/batch.c os/milton/batch.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Ios/milton -c os/milton/batch.c -o build/batch.o
harness/proptest/egg_jtc.mk:45: update target 'build/egg_jtc.o' due to: os/flair/egg_jtc.c os/flair/egg_jtc.h os/flair/egg_jtc_model.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -Ibuild -c os/flair/egg_jtc.c -o build/egg_jtc.o
harness/proptest/egg_jtc.mk:48: update target 'build/about_box_ovl.o' due to: os/milton/about_box.c os/milton/about_box.h os/milton/kmain_cold.h os/milton/int21.h os/milton/boot_info.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -Ios/milton -Ispec -c os/milton/about_box.c -o build/about_box_ovl.o
harness/proptest/egg_jtc.mk:50: update target 'build/kmain_cold_ovl.o' due to: os/milton/kmain_cold.c os/milton/kmain_cold.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -Ios/milton -c os/milton/kmain_cold.c -o build/kmain_cold_ovl.o
Makefile:10854: update target 'build/finder_actions_ovl.o' due to: os/milton/finder_actions.c os/milton/finder_actions.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -DFLAIR_LIVE_TENANTS -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/milton -c os/milton/finder_actions.c -o build/finder_actions_ovl.o
Makefile:10856: update target 'build/finder_ops_ovl.o' due to: os/flair/finder_ops_cold.c os/flair/finder_ops_private.h os/flair/finder_ops.h os/flair/finder_windows.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/finder_ops_cold.c -o build/finder_ops_ovl.o
Makefile:10858: update target 'build/finder_windows_ovl.o' due to: os/flair/finder_windows_cold.c os/flair/finder_windows_private.h os/flair/finder_windows.h spec/assets/chicago12.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -Ios/flair -Ios/flair/atkinson -Ispec -Ispec/assets -c os/flair/finder_windows_cold.c -o build/finder_windows_ovl.o
Makefile:10849: update target 'build/power_cold_ovl.o' due to: os/milton/power_cold.c os/milton/power_cold.h os/milton/finder_actions.h os/milton/power.h os/flair/power_notice.h os/milton/transient.h build/dos_messages.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -DFLAIR_LIVE_TENANTS -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/milton -c os/milton/power_cold.c -o build/power_cold_ovl.o
Makefile:10851: update target 'build/loader_cold_ovl.o' due to: os/milton/loader_cold.c os/milton/loader_cold.h os/milton/mz.c os/milton/mz.h spec/initechmz_extended.h spec/memory_map.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Oz -fomit-frame-pointer -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -DLOADER_COLD_EXTERNAL -Ispec -Ios/milton -c os/milton/loader_cold.c -o build/loader_cold_ovl.o
Makefile:10985: update target 'build/transient_imports.rsp' due to: build/command.o build/env.o build/batch.o build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/kmain_flairtenants.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o build/egg_jtc.o build/about_box_ovl.o build/kmain_cold_ovl.o build/finder_actions_ovl.o build/finder_ops_ovl.o build/finder_windows_ovl.o build/power_cold_ovl.o build/loader_cold_ovl.o tools/mk_ovl_tables.sh
export LC_ALL=C; \
for o in build/command.o build/env.o build/batch.o; do gcc-nm -u $o 2>/dev/null | awk '{print $NF}'; done | grep -v ':$' | sort -u > build/transient_imports.rsp.und; \
for o in build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/kmain_flairtenants.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o; do gcc-nm --defined-only $o 2>/dev/null | awk 'NF==3{print $3}'; done | sort -u > build/transient_imports.rsp.kdef; \
for o in build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/kmain_flairtenants.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o; do gcc-nm -u $o 2>/dev/null | awk '{print $NF}'; done | grep -v ':$' | sort -u > build/transient_imports.rsp.kund; \
grep -xF -f build/transient_imports.rsp.kdef build/transient_imports.rsp.und > build/transient_imports.rsp.keep || true; \
n=$(wc -l < build/transient_imports.rsp.keep); [ "$n" -gt 0 ] || { printf '!!! transient imports: no kernel symbol kept -- the derivation is broken\n'; exit 1; }; \
sed 's/^/-Wl,--undefined=/' build/transient_imports.rsp.keep > build/transient_imports.rsp; \
{ sed 's/^/extern /' build/transient_imports.rsp.keep; printf '%%define TRANSIENT_N_IMPORTS %s\n' "$n"; } > build/transient_imports_ext.inc; \
{ sed '$d' build/transient_imports.rsp.keep | sed 's/^/    dd /'; printf '    dd %s + TRANSIENT_LAST_IMPORT_SKEW\n' "$(tail -n 1 build/transient_imports.rsp.keep)"; } > build/transient_imports_dd.inc; \
{ printf 'bits 32\n'; sed 's/^/extern /' build/transient_imports.rsp.keep; \
  printf 'section .rodata progbits alloc noexec nowrite align=4\nglobal transient_import_want\nglobal transient_import_count\ntransient_import_want:\n'; \
  sed 's/^/    dd /' build/transient_imports.rsp.keep; printf 'transient_import_count:\n    dd %s\nsection .note.GNU-stack noalloc noexec nowrite progbits\n' "$n"; } > build/transient_want.asm; \
nasm -f elf32 build/transient_want.asm -o build/transient_want.o || exit 1; \
printf '%s\n' 'build/transient_want.o' >> build/transient_imports.rsp; \
printf '>>> transient imports: %s kernel symbols kept (%s undefined in the transient); header table + kernel want table generated\n' "$n" "$(wc -l < build/transient_imports.rsp.und)"; \
nm --defined-only build/transient_want.o | awk 'NF==3{print $3}' | sort -u - build/transient_imports.rsp.kdef -o build/transient_imports.rsp.kdef; \
sh tools/mk_ovl_tables.sh build nasm build/transient_imports.rsp.kdef build/transient_imports.rsp.kund build/egg_jtc.o build/about_box_ovl.o build/kmain_cold_ovl.o build/finder_actions_ovl.o build/finder_ops_ovl.o build/finder_windows_ovl.o build/power_cold_ovl.o build/loader_cold_ovl.o || exit 1; \
cat build/ovl_undefined.rsp >> build/transient_imports.rsp; \
printf '%s\n' 'build/ovl_kernel.o' >> build/transient_imports.rsp
Makefile:10903: update target 'build/kernel_flairtenants.elf' due to: build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/kmain_flairtenants.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o os/milton/kernel.ld build/transient_imports.rsp
gcc -m32 -march=i386 -nostdlib -no-pie -flto -Oz -flto-partition=one -fno-jump-tables -ffunction-sections -fdata-sections -mregparm=3 -mrtd -freg-struct-return -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Wl,--build-id=none -Xlinker --gc-sections -T os/milton/kernel.ld -o build/kernel_flairtenants.elf build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/kmain_flairtenants.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o @build/transient_imports.rsp
Makefile:10957: update target 'build/transient_hdr.o' due to: os/milton/transient_hdr.asm build/transient_imports.rsp
nasm -f elf32 -Ibuild/ os/milton/transient_hdr.asm -o build/transient_hdr.o
Makefile:11071: update target 'build/transient_flairtenants.elf' due to: build/kernel_flairtenants.elf build/transient_hdr.o build/command.o build/env.o build/batch.o os/milton/transient.ld
end=$(nm build/kernel_flairtenants.elf | awk '$3=="_kernel_end"{print "0x"$1}'); [ -n "$end" ] && [ $(( end )) -le $(( 0x00580000 )) ] || { printf '!!! transient(%s): kernel _kernel_end=%s reaches TRANSIENT_BASE=%s\n' 'flairtenants' "$end" '0x00580000'; exit 1; }
ld -m elf_i386 -T os/milton/transient.ld --just-symbols=build/kernel_flairtenants.elf -o build/transient_flairtenants.elf build/transient_hdr.o build/command.o build/env.o build/batch.o
Makefile:11071: update target 'build/transient_flairtenants.bin' due to: build/transient_flairtenants.elf
objcopy -O binary build/transient_flairtenants.elf build/transient_flairtenants.bin
sz=$(wc -c < build/transient_flairtenants.bin); max=$(( 0x005C0000 - 0x00580000 )); [ "$sz" -le "$max" ] || { printf '!!! transient_flairtenants.bin (%s B) exceeds its load window\n' "$sz"; exit 1; }; n=$(od -An -tu4 -j24 -N4 build/transient_flairtenants.bin | tr -d ' '); want=$(wc -l < build/transient_imports.rsp.keep); [ "$n" = "$want" ] || { printf '!!! transient_flairtenants.bin: header n_imports=%s != %s kernel imports (gcc-nm -u of the transient objects; initech-13ig)\n' "$n" "$want"; exit 1; }; printf '>>> transient COMMAND.COM (%s): %s B in boot root, linked against %s\n' 'flairtenants' "$sz" 'build/kernel_flairtenants.elf'
Makefile:11115: update target 'build/kmain_flairtenants_int.o' due to: os/milton/kmain.c os/milton/ansi_cursor.h os/milton/finder_actions.h os/milton/power_cold.h build/serial_trace_pack os/milton/kmain_cold.h os/apps/ref_demo_menu.h os/milton/boot_info.h os/milton/io.h os/milton/console.h os/milton/idt.h os/milton/pic.h os/milton/int21.h os/milton/loader.h os/milton/test_prog.h os/milton/psp.h os/milton/sft.h os/milton/ata.h os/milton/fat12.h os/milton/fileio_fat.h os/milton/desktop_db.h os/milton/blockdev.h os/milton/kbd.h os/milton/mouse.h os/milton/mouse_pack.h os/milton/pit.h os/milton/sysinit.h os/milton/command.h os/flair/heap.h os/flair/surface.h os/flair/shell.h os/flair/desktop.h os/flair/window.h os/flair/winscroll.h os/flair/menu.h os/flair/dialog.h os/flair/control.h os/flair/event.h os/flair/system_key.h os/flair/power_notice.h os/flair/process.h os/apps/ref_tenant.h spec/event_model.h spec/memory_map.h spec/dos_structs.h spec/region_algebra.h spec/flair_tenants_demo.h spec/assets/menu_canon.h spec/assets/palette.h spec/assets/color_canon.h spec/assets/cursors.h os/flair/tbxgate.h spec/toolbox_gate.h os/flair/scrap.h os/flair/stdfile.h os/flair/list.h os/flair/text.h os/milton/transient.h build/dos_messages.h os/flair/cursor.h os/milton/boot_screen.h spec/assets/boot_logo.h spec/assets/chicago12.h
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DBOOT_FLAIR_LIVE -DFLAIR_LIVE_TENANTS -DFLAIR_LIVE_INTERACTIVE -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/apps -Ios/milton -E os/milton/kmain.c -o build/kmain_flairtenants_int.o.i
build/serial_trace_pack build/kmain_flairtenants_int.o.i build/kmain_flairtenants_int.o.c
gcc -m32 -march=i386 -ffreestanding -nostdlib -fno-stack-protector -fno-pic -Ibuild -fno-pie -std=c11 -Wall -Wextra -Werror -mregparm=3 -mrtd -freg-struct-return -Ibuild -ffunction-sections -fdata-sections -fno-asynchronous-unwind-tables -fno-ident -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Os -fno-strict-aliasing --param=min-pagesize=0 -flto -Oz -fomit-frame-pointer --param=min-crossjump-insns=1 --param=max-crossjump-edges=1000 -fno-jump-tables -fno-strict-aliasing --param=min-pagesize=0 -malign-data=abi -DBOOT_FLAIR_LIVE -DFLAIR_LIVE_TENANTS -DFLAIR_LIVE_INTERACTIVE -Ispec -Ispec/assets -Ios/flair -Ios/flair/atkinson -Ios/apps -Ios/milton -c build/kmain_flairtenants_int.o.c -o build/kmain_flairtenants_int.o
Makefile:11140: update target 'build/kernel_flairtenants_int.elf' due to: build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o build/kmain_flairtenants_int.o os/milton/kernel.ld build/transient_imports.rsp
gcc -m32 -march=i386 -nostdlib -no-pie -flto -Oz -flto-partition=one -fno-jump-tables -ffunction-sections -fdata-sections -mregparm=3 -mrtd -freg-struct-return -mpreferred-stack-boundary=2 -mincoming-stack-boundary=2 -fno-tree-sra -fno-tree-dominator-opts -fno-move-loop-invariants -fno-tree-loop-im -fno-shrink-wrap -fno-tree-tail-merge -fno-partial-inlining --param=max-completely-peel-times=0 -fno-code-hoisting -fno-tree-scev-cprop -fno-stack-clash-protection -fno-if-conversion -fno-guess-branch-probability -fno-forward-propagate -fno-ipa-pure-const --param=max-inline-insns-auto=0 -Wl,--build-id=none -Xlinker --gc-sections -T os/milton/kernel.ld -o build/kernel_flairtenants_int.elf build/kstart.o build/console.o build/kmain_cold.o build/surface.o build/idt.o build/pic.o build/panic.o build/int21.o build/devices.o build/mcb.o build/psp.o build/sft.o build/config_sys.o build/sysinit.o build/loader_tenant.o build/ata.o build/block_device.o build/fat12.o build/fileio_fat.o build/kbd.o build/mouse.o build/pit.o build/rtc.o build/irq.o build/test_prog_blob.o build/type_prog_blob.o build/dir_prog_blob.o build/isr.o build/region.o build/heap.o build/event.o build/cursor.o build/window_live.o build/blitter.o build/chrome.o build/text.o build/menu.o build/control.o build/dialog.o build/desktop.o build/flair_look.o build/flair_shell.o build/process.o build/ref_tenant.o build/desktop_db.o build/finder_icon.o build/finder_desktop.o build/finder_windows.o build/finder_ops.o build/finder_cmd.o build/finder_menu.o build/tbxgate.o build/tbx_gate.o build/scrap.o build/stdfile.o build/list.o build/apple_menu.o build/egg_jtc_model.o build/egg_jtc_data.o build/kmain_flairtenants_int.o @build/transient_imports.rsp
Makefile:11157: update target 'build/transient_flairtenants_int.elf' due to: build/kernel_flairtenants_int.elf build/transient_hdr.o build/command.o build/env.o build/batch.o os/milton/transient.ld
end=$(nm build/kernel_flairtenants_int.elf | awk '$3=="_kernel_end"{print "0x"$1}'); [ -n "$end" ] && [ $(( end )) -le $(( 0x00580000 )) ] || { printf '!!! transient(%s): kernel _kernel_end=%s reaches TRANSIENT_BASE=%s\n' 'flairtenants_int' "$end" '0x00580000'; exit 1; }
ld -m elf_i386 -T os/milton/transient.ld --just-symbols=build/kernel_flairtenants_int.elf -o build/transient_flairtenants_int.elf build/transient_hdr.o build/command.o build/env.o build/batch.o
Makefile:11157: update target 'build/transient_flairtenants_int.bin' due to: build/transient_flairtenants_int.elf
objcopy -O binary build/transient_flairtenants_int.elf build/transient_flairtenants_int.bin
sz=$(wc -c < build/transient_flairtenants_int.bin); max=$(( 0x005C0000 - 0x00580000 )); [ "$sz" -le "$max" ] || { printf '!!! transient_flairtenants_int.bin (%s B) exceeds its load window\n' "$sz"; exit 1; }; n=$(od -An -tu4 -j24 -N4 build/transient_flairtenants_int.bin | tr -d ' '); want=$(wc -l < build/transient_imports.rsp.keep); [ "$n" = "$want" ] || { printf '!!! transient_flairtenants_int.bin: header n_imports=%s != %s kernel imports (gcc-nm -u of the transient objects; initech-13ig)\n' "$n" "$want"; exit 1; }; printf '>>> transient COMMAND.COM (%s): %s B in boot root, linked against %s\n' 'flairtenants_int' "$sz" 'build/kernel_flairtenants_int.elf'

# stderr

```

## 4. Host tests that travel with it

Counts by unique host program/main source (not mutant binaries, CLI cases, or gate names): **ORACLE-STRONG 2; UNIT 20; META 1; SIZE/LAYOUT 0.** META is reserved for factory/process/spec-inventory checks; input generators are listed as support sources rather than counted as tests. Some static classes are inferred from inspected test bodies; the basis is stated for each.

Every build variant name is listed; the exact canonical compilation is from its artifact-only dry run. Variant recipes and invocation assertions remain available in [gates-dos.md](gates-dos.md). The complete host dry-run archive is [dry-host.json](dry-host.json). Gate lists below use direct binary prerequisites/recipe references, avoiding giant transitive build closures that would misassign whole kernel test inventories.

### harness/diff/fat_diff/batch2_host.c — UNIT

`harness/diff/fat_diff/batch2_host.c:1`; Factory oracle: ADR-0003 Appendix A (3Bh/47h/3Fh/4Eh/4Fh).. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/batch2_host`.
Run/assert gate(s): `test-dos-batch2-host`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/batch2.sh`, `spec/dos_structs.h`, `spec/find_data.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`, `harness/diff/fat_diff/blockdev_file.h`, `spec/dos_structs.h`, `spec/find_data.h`.
Canonical `make -n` compile/link (`build/batch2_host`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -no-pie -Ispec -Ios/milton -Iseed -Iharness/diff/fat_diff -Ibuild -o build/batch2_host harness/diff/fat_diff/batch2_host.c os/milton/int21.c os/milton/devices.c os/milton/fileio_fat.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/sft.c os/milton/psp.c os/milton/mcb.c os/milton/irq.c os/milton/batch.c
```

### harness/diff/fat_diff/batch_break.c — UNIT

`harness/diff/fat_diff/batch_break.c:1`; 0qw1 execution-state oracle. MS-DOS 3.3 Reference PAUSE p.149 / PDF234;. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/batch_break`.
Run/assert gate(s): `test-batch-break-host`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/batch_break_host.h`, `harness/diff/fat_diff/batch_break_mint/exec_child.asm`, `harness/diff/fat_diff/batch_break_mint/mint.c`.
Canonical `make -n` compile/link (`build/batch_break`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Wno-unused-function -Ispec -Ios/milton -Ibuild -Iharness/diff/fat_diff harness/diff/fat_diff/batch_break.c -o build/batch_break
```

### harness/diff/fat_diff/batch_break_mint/mint.c — UNIT

`harness/diff/fat_diff/batch_break_mint/mint.c:1`; Independent DOS 3.30 reference capture: HMP + physical VGA text memory.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/batch_break`.
Run/assert gate(s): `test-batch-break-host`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/batch_break.c`, `harness/diff/fat_diff/batch_break_host.h`, `harness/diff/fat_diff/batch_break_mint/exec_child.asm`.
Canonical `make -n` compile/link (`build/batch_break`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Wno-unused-function -Ispec -Ios/milton -Ibuild -Iharness/diff/fat_diff harness/diff/fat_diff/batch_break.c -o build/batch_break
```

### harness/diff/fat_diff/chkdsk_host.c — UNIT

`harness/diff/fat_diff/chkdsk_host.c:1`; Host adapter, not an oracle. Real MCB allocations make memory checks bite.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/chkdsk_host`, `build/chkdsk_host_CROSS`, `build/chkdsk_host_DISCARD`, `build/chkdsk_host_FRAG`, `build/chkdsk_host_INVALID`, `build/chkdsk_host_LABEL`, `build/chkdsk_host_LOST`, `build/chkdsk_host_MEMORY`, `build/chkdsk_host_MIRROR`, `build/chkdsk_host_NOFIX`, `build/chkdsk_host_RECOVER`, `build/chkdsk_host_REPORT`, `build/chkdsk_host_VERBOSE`.
Run/assert gate(s): `test-chkdsk`, `test-chkdsk-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/chkdsk.sh`, `harness/diff/fat_diff/chkdsk_label.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/chkdsk_host`):
```sh
cc -std=c11 -O2 -Wall -Wextra -Werror -Ios/milton -Ispec -Iharness/diff/fat_diff -Ibuild harness/diff/fat_diff/chkdsk_host.c os/milton/chkdsk.c os/milton/mcb.c harness/diff/fat_diff/blockdev_file.c -o build/chkdsk_host
```

### harness/diff/fat_diff/command_file.c — UNIT

`harness/diff/fat_diff/command_file.c:1`; initech-e9sx host oracle / fragmented fixture, authored before the loader.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/command_file`.
Run/assert gate(s): `test-command-file`, `test-command-file-emu`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/command_file.sh`, `spec/mbr_partition_contract.json`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/command_file`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec -Ios/milton -Iharness/diff/fat_diff harness/diff/fat_diff/command_file.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c -o build/command_file
```

### harness/diff/fat_diff/dir_abi.c — META

`harness/diff/fat_diff/dir_abi.c:1`; Locked flat ABI check, IBM DOS 3.30 Technical Reference pp.6-119..121,. checks a register/inventory specification record rather than executing product behavior.
Build target(s): `build/dir_abi`.
Run/assert gate(s): `test-dir-abi`, `test-dir-abi-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/int21h_calling_convention.json`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/dir_abi`):
```sh
cc -std=c11 -Wall -Wextra -Werror harness/diff/fat_diff/dir_abi.c -o build/dir_abi
```

### harness/diff/fat_diff/dir_match.c — ORACLE-STRONG

`harness/diff/fat_diff/dir_match.c:1`; Host half of initech-bsy.1/bcg.13: same matcher as test-80k, independent. DIR matcher compared with independent mdir inventories on identical images (source:1–4).
Build target(s): `build/dir_match`, `build/dir_match_mut`.
Run/assert gate(s): `test-dir-match-emu`, `test-dir-match-emu-mutant`, `test-dir-match-host`, `test-dir-match-host-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/dir_match.sh`, `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/dir_match`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -DINT21_WILDCARD_TESTSEAM -Ispec -Ios/milton -Ibuild -o build/dir_match harness/diff/fat_diff/dir_match.c os/milton/int21.c os/milton/devices.c os/milton/mcb.c os/milton/sft.c os/milton/psp.c os/milton/irq.c os/milton/config_sys.c
```

### harness/diff/fat_diff/dosmsg_scan.c — UNIT

`harness/diff/fat_diff/dosmsg_scan.c:1`; DEC-13 source-purity tooth of test-dosmsg (initech-bcg.13).. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/dosmsg_scan`.
Run/assert gate(s): `test-dosmsg`, `test-dosmsg-source-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/dosmsg.allow`, `harness/diff/fat_diff/dosmsg.allow.baseline`, `harness/diff/fat_diff/dosmsg_mutants.sh`, `spec/dos_messages.json`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/dosmsg_scan`):
```sh
cc -std=c11 -Wall -Wextra -Werror harness/diff/fat_diff/dosmsg_scan.c -o build/dosmsg_scan
```

### harness/diff/fat_diff/dosutil_host.c — UNIT

`harness/diff/fat_diff/dosutil_host.c:1`; Factory adapter: run the shipped utility core over the real FAT backend.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/dosutil_host`, `build/dosutil_host_A`, `build/dosutil_host_BPB`, `build/dosutil_host_CONFIRM`, `build/dosutil_host_DISPLAY`, `build/dosutil_host_INVALID`, `build/dosutil_host_PRESERVE`, `build/dosutil_host_PROMPT`, `build/dosutil_host_R`, `build/dosutil_host_RECURSE`, `build/dosutil_host_ROOM`, `build/dosutil_host_ROOT`, `build/dosutil_host_SYNTAX`, `build/dosutil_host_TRUNCATE`, `build/dosutil_host_WILD`.
Run/assert gate(s): `test-attrib`, `test-attrib-mutant`, `test-label-vol`, `test-label-vol-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/dosutil_diff.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/dosutil_host`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ios/milton -Ispec -Iharness/diff/fat_diff -Ibuild -o build/dosutil_host harness/diff/fat_diff/dosutil_host.c os/milton/dosutil.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c
```

### harness/diff/fat_diff/dosutil_vol.c — UNIT

`harness/diff/fat_diff/dosutil_vol.c:1`; MS-DOS 3.3 User's Guide p.126: VOL is an internal command. */. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/dosutil_vol`, `build/dosutil_vol_mut`.
Run/assert gate(s): `test-label-vol`, `test-label-vol-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/dosutil_diff.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/dosutil_vol`):
```sh
cc -std=c11 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -Wl,--gc-sections -Ios/milton -Ispec -Ibuild -o build/dosutil_vol harness/diff/fat_diff/dosutil_vol.c os/milton/command.c
```

### harness/diff/fat_diff/formatsys_check.c — UNIT

`harness/diff/fat_diff/formatsys_check.c:1`; Independent on-disk oracle; no artifact code linked. IBM TR pp5-4..11;. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/formatsys_check`.
Run/assert gate(s): `test-formatsys`, `test-formatsys-emu`, `test-formatsys-emu-mutant`, `test-formatsys-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/formatsys.sh`, `harness/diff/fat_diff/formatsys_emu.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/formatsys_check`):
```sh
cc -std=c11 -O2 -Wall -Wextra -Werror -Ispec -Ios/milton -Ibuild harness/diff/fat_diff/formatsys_check.c -o build/formatsys_check
```

### harness/diff/fat_diff/formatsys_host.c — UNIT

`harness/diff/fat_diff/formatsys_host.c:1`; Factory adapter only; same freestanding core as the shipped COMs.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/formatsys_host`, `build/formatsys_host_COMMAND`, `build/formatsys_host_COMPARE`, `build/formatsys_host_FAT0`, `build/formatsys_host_LABEL`, `build/formatsys_host_MEDIA`, `build/formatsys_host_MISSING`, `build/formatsys_host_ORDER`, `build/formatsys_host_RESERVE`, `build/formatsys_host_RUNNING`.
Run/assert gate(s): `test-formatsys`, `test-formatsys-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/formatsys.sh`, `spec/drive_letters.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/drive_letters.h`.
Canonical `make -n` compile/link (`build/formatsys_host`):
```sh
cc -std=c11 -O2 -Wall -Wextra -Werror -Ispec -Ios/milton -Ibuild -DBLOCK_DEVICE_HOST harness/diff/fat_diff/formatsys_host.c os/milton/diskutil.c os/milton/block_device.c -o build/formatsys_host
```

### harness/diff/fat_diff/shell_files.c — UNIT

`harness/diff/fat_diff/shell_files.c:1`; Same command.c builtins, real INT21/FAT, independent mtools reference.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/shell_files`.
Run/assert gate(s): `test-del-directory-host`, `test-shell-files-host`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/del_directory.sh`, `harness/diff/fat_diff/shell_files.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`, `harness/diff/fat_diff/shell_files_host.h`.
Canonical `make -n` compile/link (`build/shell_files`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Wno-unused-function -no-pie -ffunction-sections -fdata-sections -Wl,--gc-sections -Ispec -Ios/milton -Ibuild -Iharness/diff/fat_diff harness/diff/fat_diff/shell_files.c os/milton/int21.c os/milton/devices.c os/milton/fileio_fat.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/sft.c os/milton/psp.c os/milton/mcb.c os/milton/irq.c -o build/shell_files
```

### harness/diff/fat_diff/test_dir_disk.c — UNIT

`harness/diff/fat_diff/test_dir_disk.c:1`; Exercise the real FAT backend vs mtools mdir, independently of the shell.. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_dir_disk`, `build/test_dir_disk_mut_free`, `build/test_dir_disk_mut_label`.
Run/assert gate(s): `test-dir-free-space`, `test-dir-free-space-mutant`, `test-dir-volume`, `test-dir-volume-mutant`, `test-shell-dir-look`, `test-shell-dir-look-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/shell_dir_look.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_dir_disk`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec -Ios/milton -Ibuild -Iharness/diff/fat_diff -o build/test_dir_disk harness/diff/fat_diff/test_dir_disk.c harness/diff/fat_diff/blockdev_file.c os/milton/fat12.c os/milton/fileio_fat.c
```

### harness/diff/fat_diff/test_dir_look.c — UNIT

`harness/diff/fat_diff/test_dir_look.c:1`; Independent transcription: MS-DOS_3.3_Users_Guide_198707.pdf,. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_dir_look`, `build/test_dir_look_mut_date`, `build/test_dir_look_mut_joined`, `build/test_dir_look_mut_lower`.
Run/assert gate(s): `test-dir-look`, `test-dir-look-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/dos33_dir.golden`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_dir_look`):
```sh
cc -std=c11 -Wall -Wextra -Werror -Ispec -Ios/milton -Ibuild -Iharness/diff/fat_diff -o build/test_dir_look harness/diff/fat_diff/test_dir_look.c os/milton/command.c os/milton/env.c
```

### harness/diff/fat_diff/test_fdisk.c — UNIT

`harness/diff/fat_diff/test_fdisk.c:1`; File-backed adapter for the actual shipped editor core. Independent MBR. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_fdisk`, `build/test_fdisk_ACTIVE`, `build/test_fdisk_CHS`, `build/test_fdisk_CONFIRM`, `build/test_fdisk_SIGNATURE`.
Run/assert gate(s): `test-fdisk`, `test-fdisk-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/fdisk.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_fdisk`):
```sh
cc -std=c11 -O2 -Wall -Wextra -Werror -Ios/milton harness/diff/fat_diff/test_fdisk.c os/milton/fdisk.c -o build/test_fdisk
```

### harness/diff/fat_diff/test_xcopy_tree.c — UNIT

`harness/diff/fat_diff/test_xcopy_tree.c:1`; Factory oracle bridge: SAME shipped C, REAL INT21/FAT backend. The. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_xcopy_cwd_mut`, `build/test_xcopy_tree`, `build/test_xcopy_tree_mut_A`, `build/test_xcopy_tree_mut_COUNT`, `build/test_xcopy_tree_mut_CTRL_C`, `build/test_xcopy_tree_mut_CYCLIC`, `build/test_xcopy_tree_mut_D`, `build/test_xcopy_tree_mut_DATE_RANGE`, `build/test_xcopy_tree_mut_DEFAULT`, `build/test_xcopy_tree_mut_DESCEND`, `build/test_xcopy_tree_mut_E`, `build/test_xcopy_tree_mut_E_REQUIRES_S`, `build/test_xcopy_tree_mut_HIDDEN`, `build/test_xcopy_tree_mut_LABEL`, `build/test_xcopy_tree_mut_M`, `build/test_xcopy_tree_mut_OMIT`, `build/test_xcopy_tree_mut_P`, `build/test_xcopy_tree_mut_S`, `build/test_xcopy_tree_mut_SELF`, `build/test_xcopy_tree_mut_TARGET`, `build/test_xcopy_tree_mut_TIME`, `build/test_xcopy_tree_mut_TREE_F`, `build/test_xcopy_tree_mut_TREE_LAYOUT`, `build/test_xcopy_tree_mut_V`, `build/test_xcopy_tree_mut_W`, `build/test_xcopy_tree_mut_WILD`.
Run/assert gate(s): `test-dos-stamp-copy-host`, `test-dos-stamp-copy-host-mutant`, `test-drive-operands`, `test-drive-operands-mutant`, `test-xcopy-cwd-host`, `test-xcopy-cwd-host-mutant`, `test-xcopy-tree`, `test-xcopy-tree-emu`, `test-xcopy-tree-emu-cancel`, `test-xcopy-tree-emu-mutant`, `test-xcopy-tree-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `harness/diff/fat_diff/copy_stamp.sh`, `harness/diff/fat_diff/drive_operands.sh`, `harness/diff/fat_diff/xcopy_cwd.sh`, `harness/diff/fat_diff/xcopy_tree.sh`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `harness/diff/fat_diff/blockdev_file.c`.
Canonical `make -n` compile/link (`build/test_xcopy_tree`):
```sh
cc -std=c11 -Wall -Wextra -Werror -no-pie -Ispec -Ios/milton -Ibuild -Iharness/diff/fat_diff -o build/test_xcopy_tree harness/diff/fat_diff/test_xcopy_tree.c os/milton/xcopy_tree.c os/milton/int21.c os/milton/devices.c os/milton/fileio_fat.c os/milton/fat12.c harness/diff/fat_diff/blockdev_file.c os/milton/sft.c os/milton/psp.c os/milton/mcb.c os/milton/irq.c
```

### os/milton/test_batch.c — UNIT

`os/milton/test_batch.c:1`; host unit oracle for the .BAT parser/expander (batch.c).. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_batch`, `build/test_batch_mutant_echooff`, `build/test_batch_mutant_gotonolabel`, `build/test_batch_mutant_nopctpct`.
Run/assert gate(s): `test-batch`, `test-batch-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_batch`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/milton -Iseed \
	-o build/test_batch os/milton/test_batch.c
```

### os/milton/test_batch_exec.c — UNIT

`os/milton/test_batch_exec.c:1`; host unit oracle for the .BAT EXECUTION-logic helpers. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_batch_exec`, `build/test_batch_exec_mutant_fornotokens`, `build/test_batch_exec_mutant_ifignorenot`.
Run/assert gate(s): `test-batch-exec`, `test-batch-exec-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_batch_exec`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/milton -Iseed -o build/test_batch_exec os/milton/test_batch_exec.c
```

### os/milton/test_cmdline_fuzz.c — ORACLE-STRONG

`os/milton/test_cmdline_fuzz.c:1`; deterministic, seeded generative fuzzer for the. property/fuzz invariants over generated inputs.
Build target(s): `build/test_cmdline_fuzz`, `build/test_cmdline_fuzz_mutant_noclamp`.
Run/assert gate(s): `test-cmdline-fuzz`, `test-cmdline-fuzz-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`.
Canonical `make -n` compile/link (`build/test_cmdline_fuzz`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_cmdline_fuzz os/milton/test_cmdline_fuzz.c os/milton/command.c os/milton/psp.c
```

### os/milton/test_command.c — UNIT

`os/milton/test_command.c:1`; host unit oracle for the PURE COMMAND.COM shell logic. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_command`, `build/test_command_mutant_badcmd`, `build/test_command_mutant_com`, `build/test_command_mutant_nocopy`, `build/test_command_mutant_nodate`, `build/test_command_mutant_nodel`, `build/test_command_mutant_nomdrd`, `build/test_command_mutant_nopath`, `build/test_command_mutant_noprompt`, `build/test_command_mutant_noren`, `build/test_command_mutant_nosamefile`, `build/test_command_mutant_noset`, `build/test_command_mutant_notime`, `build/test_command_mutant_noupcase`.
Run/assert gate(s): `test-command`, `test-command-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`, `spec/find_data.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`, `spec/find_data.h`.
Canonical `make -n` compile/link (`build/test_command`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_command os/milton/test_command.c os/milton/command.c os/milton/env.c
```

### os/milton/test_env.c — UNIT

`os/milton/test_env.c:1`; host unit oracle for the environment store (env.c / env.h).. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_env`, `build/test_env_mutant_nodedup`, `build/test_env_mutant_noupcase`.
Run/assert gate(s): `test-env`, `test-env-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): none named; in-source expectations/generated synthetic inputs.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Canonical `make -n` compile/link (`build/test_env`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ios/milton -Iseed \
	-o build/test_env os/milton/test_env.c
```

### os/milton/test_redir_parse.c — UNIT

`os/milton/test_redir_parse.c:1`; host unit oracle for cmd_redir_parse, the PURE COMMAND.COM. ordinary deterministic assertions / fake transport or backend.
Build target(s): `build/test_redir_parse`, `build/test_redir_parse_mutant_keeptarget`, `build/test_redir_parse_mutant_noappend`, `build/test_redir_parse_mutant_nolt`, `build/test_redir_parse_mutant_nopipe`.
Run/assert gate(s): `test-redir-parse`, `test-redir-parse-mutant`.
Corpus/fixture references (static path extraction; full invocation may construct suffixes): `spec/dos_structs.h`, `spec/find_data.h`.
External root(s): none directly named in source/direct gate recipes; inspect support-helper closure below.
Traveling test/helper source/header inputs: `spec/dos_structs.h`, `spec/find_data.h`.
Canonical `make -n` compile/link (`build/test_redir_parse`):
```sh
cc -std=c11 -Wall -Wextra -Werror -D_POSIX_C_SOURCE=200809L -Ispec -Ios/milton -Iseed -Ibuild \
	-o build/test_redir_parse os/milton/test_redir_parse.c os/milton/command.c os/milton/env.c
```

### Recipe-driven host suites and variants

All assigned legacy gate recipes, including compiler/FPC/mtools/script orchestration and mutants, are enumerated in [gates-dos.md](gates-dos.md). Host tier here is the declared legacy vector; some entries launch emulators transitively. The directory name proptest is not itself evidence of a property oracle.

## 5. Emulator gates worth keeping as smokes

**80 exclusive primary-owned entries in the 799-entry declared legacy emulator vector.** Also 82 defined test-* targets owned here have a static dependency closure reaching QEMU/Bochs (includes wrappers, mutants and names outside the active vector; not a runtime boot count). Cross-component smokes below are not double-counted. Exact primary attribution is in [gate-attribution.tsv](gate-attribution.tsv).

### test-shell

#  recipe to execute (from 'Makefile', line 26713):

Boot/data images named by direct prerequisites or expanded recipe: `build/fat_exec.img`, `build/tracer_secondary.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Inline harness input/options:
```text
--keys FIRST and only then screendumps
--keys "d,i,r,ret,t,y,p,e,spc,h,e,l,l,o,dot,t,x,t,ret,g,r,e,e,t,ret,b,a,d,c,m,d,ret,e,x,i,t,ret" \
--keys-after "SHELL-READY" \
--keys. qskz deliberately moves the
```
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
	--quit-after "SHELL-DONE" \
@if grep -q 'triple_fault=1' "build/shell_boot.report"; then \
	printf '!!! test-shell FAIL: TRIPLE FAULT -- the shell boot or a command crashed\n'; \
	printf '!!! test-shell FAIL: no serial captured at %s\n' "build/shell_boot.serial"; exit 1; \
@grep -q '^SHELL-READY$$' "build/shell_boot.serial" \
	|| { printf '!!! test-shell FAIL: SHELL-READY missing -- the REPL was never entered\n'; exit 1; }
@grep -qF 'HELLO    TXT' "build/shell_boot.repl" \
	|| { printf '!!! test-shell FAIL: DIR did not list HELLO.TXT (root-cause the DIR built-in / FINDFIRST, Rule 3)\n'; exit 1; }
@grep -qF 'GREET    COM' "build/shell_boot.repl" \
	|| { printf '!!! test-shell FAIL: DIR did not list GREET.COM\n'; exit 1; }
@grep -qF 'Hello from InitechOS test file' "build/shell_boot.repl" \
	|| { printf '!!! test-shell FAIL: TYPE did not print HELLO.TXT contents "%s"\n' "Hello from InitechOS test file"; exit 1; }
```

### test-autoexec

#  recipe to execute (from 'Makefile', line 30830):

Boot/data images named by direct prerequisites or expanded recipe: `build/autoexec_xw1.img`, `build/tracer_secondary.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Inline harness input/options:
```text
--keys "e,x,i,t,ret" --keys-after "SHELL-READY" \
```
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
@command -v mformat >/dev/null 2>&1 || { printf '!!! test-autoexec FAIL: mtools `mformat` not found (apt install mtools). A skipped oracle is worse than a red one.\n'; exit 1; }
@command -v mcopy   >/dev/null 2>&1 || { printf '!!! test-autoexec FAIL: mtools `mcopy` not found.\n'; exit 1; }
	--quit-after "SHELL-DONE" \
@if grep -q 'triple_fault=1' "build/autoexec_xw1.report"; then \
	printf '!!! test-autoexec FAIL: TRIPLE FAULT -- AUTOEXEC/.BAT execution crashed\n'; exit 1; \
	printf '!!! test-autoexec FAIL: no serial captured at %s\n' "build/autoexec_xw1.serial"; exit 1; \
@grep -q '^SHELL-READY$$' "build/autoexec_xw1.serial" \
	|| { printf '!!! test-autoexec FAIL: SHELL-READY missing -- the REPL was never entered\n'; exit 1; }
	grep -qF "$$m" "build/autoexec_xw1.repl" \
		|| { printf '!!! test-autoexec FAIL: marker %s missing -- AUTOEXEC/.BAT did not execute that step (root-cause batch driver / EXEC / CALL-reload, Rule 3)\n' "$$m"; exit 1; }; \
@grep -q '^SHELL-EXIT$$' "build/autoexec_xw1.serial" \
	|| { printf '!!! test-autoexec FAIL: SHELL-EXIT missing -- the post-AUTOEXEC interactive EXIT did not run\n'; exit 1; }
```

### test-dosutil-emu

#  recipe to execute (from 'harness/diff/fat_diff/dosutil.mk', line 83):

Boot/data images named by direct prerequisites or expanded recipe: `build/tracer_secondary.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
```

### test-xcopy-cwd-emu

#  recipe to execute (from 'harness/diff/fat_diff/shell_files.mk', line 21):

Boot/data images named by direct prerequisites or expanded recipe: `build/tracer_secondary.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
```

### test-fdisk-emu

#  recipe to execute (from 'harness/diff/fat_diff/fdisk.mk', line 35):

Boot/data images named by direct prerequisites or expanded recipe: `build/tracer_secondary.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
```

### test-shell-permanent

#  recipe to execute (from 'Makefile', line 30597):

Boot/data images named by direct prerequisites or expanded recipe: `build/tracer_boot.img`.
Trace file: no separate file named in this recipe; trace is a make *_SPEC literal or inline --keys sequence.
Assertions (representative exact expressions; full rule and all assertions in gate inventory):
```sh
```

The remaining legacy emulator entries cover command parsing, batch expansion/control flow, startup/configuration, redirected I/O, copy/delete/read-only/full-volume recovery, disk/format/install and each external utility. Mutant twins establish sensitivity of the old assertion; convert useful faults to patches instead of maintaining extra resident kernels. The complete names/recipes are linked above.

## 6. Mutation hooks in production source

Measured count is conditional directive sites containing deliberately-wrong macro names (`MUT`, `MUTATE`, `MUTANT`, `BROKEN`, `DROP_WRITE`, `DOTRUNC`), not #else arms or runtime executions. One directive can mention two macros; distinct macro count is also provided. `#ifndef` controls count because the wrong behavior may be its absence. Non-mutation BOOT/profile selectors are excluded.

|File|Guard sites|Distinct macros|Macros and exact sites|
|---|---:|---:|---|
|`os/milton/batch.c:1`|5|5|`os/milton/batch.c:204` `BATCH_MUTATE_NO_PCT_PCT`; `os/milton/batch.c:336` `BATCH_MUTATE_GOTO_NOLABEL`; `os/milton/batch.c:431` `BATCH_MUTATE_ECHO_OFF_MISS`; `os/milton/batch.c:675` `BATCH_MUTATE_IF_IGNORE_NOT`; `os/milton/batch.c:784` `BATCH_MUTATE_FOR_NO_TOKENS`|
|`os/milton/batch.h:1`|0|0|none|
|`os/milton/chkdsk.c:1`|14|12|`os/milton/chkdsk.c:53` `CHKDSK_MUT_NOFIX`; `os/milton/chkdsk.c:74` `CHKDSK_MUT_MIRROR`; `os/milton/chkdsk.c:109` `CHKDSK_MUT_INVALID`; `os/milton/chkdsk.c:132` `CHKDSK_MUT_INVALID`; `os/milton/chkdsk.c:144` `CHKDSK_MUT_CROSS`; `os/milton/chkdsk.c:153` `CHKDSK_MUT_FRAG`; `os/milton/chkdsk.c:187` `CHKDSK_MUT_LABEL`; `os/milton/chkdsk.c:196` `CHKDSK_MUT_VERBOSE`; `os/milton/chkdsk.c:259` `CHKDSK_MUT_LOST`; `os/milton/chkdsk.c:271` `CHKDSK_MUT_NOFIX`; `os/milton/chkdsk.c:274` `CHKDSK_MUT_RECOVER`; `os/milton/chkdsk.c:277` `CHKDSK_MUT_DISCARD`; `os/milton/chkdsk.c:325` `CHKDSK_MUT_REPORT`; `os/milton/chkdsk.c:340` `CHKDSK_MUT_MEMORY`|
|`os/milton/chkdsk.h:1`|0|0|none|
|`os/milton/chkdsk.ld:1`|0|0|none|
|`os/milton/chkdsk_program.c:1`|0|0|none|
|`os/milton/chkdsk_start.asm:1`|0|0|none|
|`os/milton/command.c:1`|86|74|`os/milton/command.c:65` `CMD_MUTATE_PATH_COMMAND`; `os/milton/command.c:68` `DU_MUTATE_VOL`; `os/milton/command.c:74` `CMD_MUTATE_NO_MDRD`; `os/milton/command.c:89` `CMD_MUTATE_NO_SET`; `os/milton/command.c:96` `CMD_MUTATE_NO_PROMPT`; `os/milton/command.c:103` `CMD_MUTATE_NO_COPY`; `os/milton/command.c:110` `CMD_MUTATE_NO_DEL`; `os/milton/command.c:118` `CMD_MUTATE_NO_REN`; `os/milton/command.c:126` `CMD_MUTATE_NO_DATE`; `os/milton/command.c:133` `CMD_MUTATE_NO_TIME`; `os/milton/command.c:151` `CMD_MUTATE_BADCMD_BUILTIN`; `os/milton/command.c:193` `CMD_MUTATE_NO_UPCASE`; `os/milton/command.c:282` `CMD_MUTATE_COM_ALWAYS`; `os/milton/command.c:332` `CMD_MUTATE_DIR_NO_DATE`; `os/milton/command.c:358` `CMD_MUTATE_DIR_JOINED`; `os/milton/command.c:367` `CMD_MUTATE_DIR_NO_DATE`; `os/milton/command.c:390` `CMD_MUTATE_DIR_LOWERCASE`; `os/milton/command.c:492` `CMD_MUTATE_COPY_SWITCH`; `os/milton/command.c:547` `CMD_MUTATE_REN_MAP`; `os/milton/command.c:645` `CMD_MUTATE_NO_SAMEFILE`; `os/milton/command.c:1374` `CMD_MUTATE_NO_PATH`; `os/milton/command.c:1386` `CMD_MUTATE_NO_PATH`; `os/milton/command.c:1402` `CMD_MUTATE_NO_PATH`; `os/milton/command.c:1413` `CMD_MUTATE_NO_PATH`; `os/milton/command.c:1428` `CMD_MUTATE_NO_PATH`; `os/milton/command.c:1540` `CMD_MUTATE_REDIR_NO_APPEND`; `os/milton/command.c:1597` `CMD_MUTATE_REDIR_KEEP_TARGET`; `os/milton/command.c:1647` `CMD_MUTATE_REDIR_NO_LT`; `os/milton/command.c:1704` `CMD_MUTATE_PIPE_NO_SPLIT`; `os/milton/command.c:1928` `CMD_MUTATE_NO_READ_CF`; `os/milton/command.c:2678` `CMD_MUTATE_DIR_ARGS`; `os/milton/command.c:2748` `CMD_MUTATE_DIR_NO_VOLUME`; `os/milton/command.c:2766` `CMD_MUTATE_DIR_BREAK`; `os/milton/command.c:2827` `CMD_MUTATE_TYPE_BREAK`; `os/milton/command.c:2846` `CMD_MUTATE_TYPE_BREAK`; `os/milton/command.c:2883` `CMD_MUTATE_VOLUME_PROMPT`; `os/milton/command.c:2913` `CMD_MUTATE_CD_DRIVE`; `os/milton/command.c:2968` `CMD_MUTATE_RD_NOOP`; `os/milton/command.c:3105` `CMD_MUTATE_NO_SAMEFILE`; `os/milton/command.c:3125` `CMD_MUTATE_COPY_ASCII`; `os/milton/command.c:3150` `CMD_MUTATE_COPY_VERIFY`; `os/milton/command.c:3162` `CMD_MUTATE_COPY_TIME`; `os/milton/command.c:3165` `CMD_MUTATE_COPY_DATE`; `os/milton/command.c:3177` `CMD_MUTATE_COPY_FULL_BADCMD`; `os/milton/command.c:3204` `CMD_MUTATE_COPY_DIR_RAW`; `os/milton/command.c:3220` `CMD_MUTATE_NO_SAMEFILE`; `os/milton/command.c:3258` `CMD_MUTATE_COPY_WILD`; `os/milton/command.c:3279` `CMD_MUTATE_COPY_PLUS`; `os/milton/command.c:3293` `CMD_MUTATE_COPY_WILD`; `os/milton/command.c:3307` `CMD_MUTATE_COPY_EOF`; `os/milton/command.c:3319` `CMD_MUTATE_COPY_TIME`; `os/milton/command.c:3322` `CMD_MUTATE_COPY_DATE`; `os/milton/command.c:3390` `CMD_MUTATE_DEL_DIRECTORY_UNLINK`; `os/milton/command.c:3403` `CMD_MUTATE_DEL_NO_CONFIRM`; `os/milton/command.c:3418` `CMD_MUTATE_DEL_YES_ALWAYS`; `os/milton/command.c:3421` `CMD_MUTATE_DEL_REFUSE_Y`; `os/milton/command.c:3428` `CMD_MUTATE_DEL_DIRECTORY_RECURSE`; `os/milton/command.c:3475` `CMD_MUTATE_DEL_STOP_PROTECTED`; `os/milton/command.c:3500` `CMD_MUTATE_DEL_LOSE_PARENT`; `os/milton/command.c:3512` `CMD_MUTATE_DEL_SILENT_ERROR`; `os/milton/command.c:3518` `CMD_MUTATE_DEL_CAP16`; `os/milton/command.c:3540` `CMD_MUTATE_REN_ENUM`; `os/milton/command.c:3717` `CMD_MUTATE_BATCH_DROP_CHILD_BREAK`; `os/milton/command.c:3829` `CMD_MUTATE_VOLUME_MESSAGE`; `os/milton/command.c:3905` `CMD_MUTATE_PERMANENT_EXIT`; `os/milton/command.c:3953` `CMD_MUTATE_REDIR_IGNORE_FULL`; `os/milton/command.c:3964` `CMD_MUTATE_REDIR_BYPASS`; `os/milton/command.c:4004` `CMD_MUTATE_REDIR_CREATE_BADCMD`; `os/milton/command.c:4028` `CMD_MUTATE_REDIR_BYPASS`; `os/milton/command.c:4045` `CMD_MUTATE_REDIR_NO_RESTORE`; `os/milton/command.c:4248` `CMD_MUTATE_PIPE_CLOBBER`; `os/milton/command.c:4305` `CMD_MUTATE_PIPE_NO_STDIN`; `os/milton/command.c:4344` `CMD_MUTATE_PIPE_IGNORE_FULL`; `os/milton/command.c:4463` `CMD_MUTATE_PROMPT_DATE`; `os/milton/command.c:4690` `CMD_MUTATE_FOR_WILDCARD`; `os/milton/command.c:4728` `CMD_MUTATE_BATCH_BODY`; `os/milton/command.c:4753` `CMD_MUTATE_FOR_PROMPT`; `os/milton/command.c:4769` `CMD_MUTATE_SHIFT_ZERO`; `os/milton/command.c:4829` `CMD_MUTATE_BATCH_ALWAYS_TERMINATE`; `os/milton/command.c:4832` `CMD_MUTATE_BATCH_ALWAYS_RESUME`; `os/milton/command.c:4966` `CMD_MUTATE_COMMAND_C`; `os/milton/command.c:4970` `CMD_MUTATE_COMMAND_ENV`; `os/milton/command.c:5034` `CMD_MUTATE_NO_AUTOEXEC`; `os/milton/command.c:5045` `CMD_MUTATE_STARTUP`; `os/milton/command.c:5069` `CMD_MUTATE_NO_CWD_PROMPT`; `os/milton/command.c:5116` `CMD_MUTATE_PROMPT_DOLLAR`|
|`os/milton/command.h:1`|0|0|none|
|`os/milton/diskutil.c:1`|9|9|`os/milton/diskutil.c:106` `DU_MUT_ORDER`; `os/milton/diskutil.c:152` `DU_MUT_MEDIA`; `os/milton/diskutil.c:160` `DU_MUT_RUNNING`; `os/milton/diskutil.c:169` `DU_MUT_MISSING`; `os/milton/diskutil.c:191` `DU_MUT_LABEL`; `os/milton/diskutil.c:198` `DU_MUT_FAT0`; `os/milton/diskutil.c:204` `DU_MUT_COMMAND`; `os/milton/diskutil.c:213` `DU_MUT_RESERVE`; `os/milton/diskutil.c:257` `DU_MUT_COMPARE`|
|`os/milton/diskutil.h:1`|0|0|none|
|`os/milton/diskutil_program.c:1`|0|0|none|
|`os/milton/diskutil_start.asm:1`|0|0|none|
|`os/milton/dosutil.c:1`|16|16|`os/milton/dosutil.c:36` `DU_MUTATE_DRIVE`; `os/milton/dosutil.c:65` `DU_MUTATE_WILD`; `os/milton/dosutil.c:94` `DU_MUTATE_RECURSE`; `os/milton/dosutil.c:105` `DU_MUTATE_PRESERVE`; `os/milton/dosutil.c:117` `DU_MUTATE_DISPLAY`; `os/milton/dosutil.c:140` `DU_MUTATE_R`; `os/milton/dosutil.c:143` `DU_MUTATE_A`; `os/milton/dosutil.c:151` `DU_MUTATE_SYNTAX`; `os/milton/dosutil.c:236` `DU_MUTATE_ROOT`; `os/milton/dosutil.c:244` `DU_MUTATE_BPB`; `os/milton/dosutil.c:254` `DU_MUTATE_ROOM`; `os/milton/dosutil.c:271` `DU_MUTATE_LABEL_DRIVE`; `os/milton/dosutil.c:283` `DU_MUTATE_PROMPT`; `os/milton/dosutil.c:294` `DU_MUTATE_CONFIRM`; `os/milton/dosutil.c:303` `DU_MUTATE_INVALID`; `os/milton/dosutil.c:308` `DU_MUTATE_TRUNCATE`|
|`os/milton/dosutil.h:1`|0|0|none|
|`os/milton/dosutil.ld:1`|0|0|none|
|`os/milton/dosutil_target.c:1`|0|0|none|
|`os/milton/edlin.c:1`|4|4|`os/milton/edlin.c:38` `EDLIN_MUT_NUMBER`; `os/milton/edlin.c:158` `EDLIN_MUT_BACKUP`; `os/milton/edlin.c:209` `EDLIN_MUT_FIRST`; `os/milton/edlin.c:279` `EDLIN_MUT_QUIT`|
|`os/milton/edlin.h:1`|0|0|none|
|`os/milton/edlin.ld:1`|0|0|none|
|`os/milton/edlin_program.c:1`|0|0|none|
|`os/milton/edlin_start.asm:1`|0|0|none|
|`os/milton/env.c:1`|2|2|`os/milton/env.c:132` `ENV_MUTATE_NO_UPCASE`; `os/milton/env.c:160` `ENV_MUTATE_NO_DEDUP`|
|`os/milton/env.h:1`|0|0|none|
|`os/milton/fdisk.c:1`|6|6|`os/milton/fdisk.c:17` `FDISK_MUT_SIGNATURE`; `os/milton/fdisk.c:57` `FDISK_MUT_ACTIVE`; `os/milton/fdisk.c:79` `FDISK_MUT_CHS`; `os/milton/fdisk.c:88` `FDISK_MUT_CONFIRM`; `os/milton/fdisk.c:129` `FDISK_MUT_LETTER`; `os/milton/fdisk.c:175` `FDISK_MUT_LOADED`|
|`os/milton/fdisk.h:1`|0|0|none|
|`os/milton/fdisk.ld:1`|0|0|none|
|`os/milton/fdisk_program.c:1`|0|0|none|
|`os/milton/fdisk_start.asm:1`|0|0|none|
|`os/milton/find_program.asm:1`|1|1|`os/milton/find_program.asm:208` `FIND_MUTATE_INVERT`|
|`os/milton/more_program.asm:1`|1|1|`os/milton/more_program.asm:78` `MORE_MUTATE_DROP`|
|`os/milton/sort_program.asm:1`|1|1|`os/milton/sort_program.asm:59` `SORT_MUTATE_REVERSE`|
|`os/milton/tpsrep_program.asm:1`|7|6|`os/milton/tpsrep_program.asm:54` `TPS_MUTATE_COUNT6`; `os/milton/tpsrep_program.asm:143` `TPS_MUTATE_LINESOK`; `os/milton/tpsrep_program.asm:259` `TPS_MUTATE_PAGESOK`; `os/milton/tpsrep_program.asm:480` `TPS_MUTATE_UNCHECKED_BACKUP`; `os/milton/tpsrep_program.asm:561` `TPS_MUTATE_NOWRCHK`; `os/milton/tpsrep_program.asm:944` `TPS_MUTATE_NOWRCHK`; `os/milton/tpsrep_program.asm:1251` `TPS_MUTATE_SAY7`|
|`os/milton/transient.h:1`|0|0|none|
|`os/milton/transient.ld:1`|0|0|none|
|`os/milton/transient_hdr.asm:1`|0|0|none|
|`os/milton/xcopy_tree.c:1`|25|25|`os/milton/xcopy_tree.c:66` `DU_MUT_CTRL_C`; `os/milton/xcopy_tree.c:97` `DU_MUT_REL_ROOT`; `os/milton/xcopy_tree.c:107` `DU_MUT_DRIVE`; `os/milton/xcopy_tree.c:162` `DU_MUT_HIDDEN`; `os/milton/xcopy_tree.c:185` `DU_MUT_DATE_RANGE`; `os/milton/xcopy_tree.c:201` `DU_MUT_A`; `os/milton/xcopy_tree.c:204` `DU_MUT_D`; `os/milton/xcopy_tree.c:213` `DU_MUT_SELF`; `os/milton/xcopy_tree.c:217` `DU_MUT_P`; `os/milton/xcopy_tree.c:242` `DU_MUT_TIME`; `os/milton/xcopy_tree.c:250` `DU_MUT_V`; `os/milton/xcopy_tree.c:271` `DU_MUT_M`; `os/milton/xcopy_tree.c:310` `DU_MUT_TREE_LAYOUT`; `os/milton/xcopy_tree.c:402` `DU_MUT_TREE_F`; `os/milton/xcopy_tree.c:417` `DU_MUT_E_REQUIRES_S`; `os/milton/xcopy_tree.c:420` `DU_MUT_DEFAULT`; `os/milton/xcopy_tree.c:423` `DU_MUT_S`; `os/milton/xcopy_tree.c:426` `DU_MUT_E`; `os/milton/xcopy_tree.c:430` `DU_MUT_W`; `os/milton/xcopy_tree.c:436` `DU_MUT_OMIT`; `os/milton/xcopy_tree.c:449` `DU_MUT_WILD`; `os/milton/xcopy_tree.c:456` `DU_MUT_DESCEND`; `os/milton/xcopy_tree.c:472` `DU_MUT_TARGET`; `os/milton/xcopy_tree.c:482` `DU_MUT_CYCLIC`; `os/milton/xcopy_tree.c:499` `DU_MUT_COUNT`|
|`os/milton/xcopy_tree.h:1`|0|0|none|
|`os/milton/xcopy_tree.ld:1`|0|0|none|
|`os/milton/xcopy_tree_entry.asm:1`|0|0|none|

## 7. Coupling to fix during import

Concrete include/external-symbol evidence follows; line numbers refer to legacy-final. Cross-component public ABI includes in section 2 are expected dependencies, not violations.

- `os/milton/chkdsk.c:13` → `os/milton/dospath.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/chkdsk.h:10` → `os/milton/mcb.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/command.c:1768` → `os/milton/int21.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/command.c:1771` → `os/milton/fileio_fat.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/command.c:3067` → `harness/diff/fat_diff/shell_files_host.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/command.c:3070` → `harness/diff/fat_diff/batch_break_host.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/dosutil.c:16` → `os/milton/dospath.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/edlin.c:9` → `os/milton/conedit.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/fdisk_program.c:8` → `os/milton/power.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/xcopy_tree.c:24` → `os/milton/dospath.h`: literal cross-component include; evaluate in its #if context.
- `os/milton/transient.h:77` → `resident kernel symbol address fingerprint`: transient import table ties shell image to exact kernel ELF.
- `os/milton/command.c:3067` → `harness/diff/fat_diff/shell_files_host.h`: host-only backend seam is useful; move into a dedicated adapter.
- `os/milton/command.c:3070` → `harness/diff/fat_diff/batch_break_host.h`: host-only test injection reaches factory header.

## 8. Discard

Discard legacy size/headroom/overlay-layout policing and mutant kernel/image variants after preserving their functional assertions as ordinary tests or patches. Keep independent golden captures/readers/property suites; do not discard an oracle merely because its legacy target contains size or overlay.
Merge `_cold.c`/`_private.h` size splits with their owning implementation, retire FLAIR.OVL resident/import stub synthesis and COMMAND transient fingerprinting as their replacements become available. Keep actual DOS overlay semantics and independently useful loader validation. Remove the 384-sector/192-KiB policing, padding-only mutants, variant-wide byte ceilings and recording/bounded personality switches from production builds.
