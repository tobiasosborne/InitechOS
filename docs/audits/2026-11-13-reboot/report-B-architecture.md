# InitechOS architecture investigation B

Read-only source investigation, 2026-11-13. Source HEAD: `b571f1e35cc386803ccd763f427c1cabe41866b9`.

## Executive assessment

**Recommendation (engineering judgment): restructure the existing implementation around its real boundaries; do not start a replacement OS.** Most applications already are independent disk programs. The largest architectural problems are the resident desktop/kernel composition, build-specific overlay linking, fixed memory budgets, and the global factory/verification machinery. Rewriting the spreadsheet, word processor, database, FAT implementation, or drawing algorithms would discard valuable behavior without fixing those integration mechanisms. The disk-app mechanism and core/adapter separations provide a migration path today ([Makefile:10624](/home/tobiasosborne/Projects/InitechOS/Makefile:10624), [Makefile:10707](/home/tobiasosborne/Projects/InitechOS/Makefile:10707), [Makefile:10723](/home/tobiasosborne/Projects/InitechOS/Makefile:10723), [os/samir/include/samir/pal.h:94](/home/tobiasosborne/Projects/InitechOS/os/samir/include/samir/pal.h:94)).

Three corrections to the task's starting assumptions matter:

1. **The shipping kernel is already a FAT file.** Stage2 reads `IO.SYS` and `INITDOS.SYS` from the active FAT12 partition. It still stages the *entire* kernel below `PROGRAM_BASE`, so changing from raw sectors to FAT alone cannot remove the bounce limit ([os/boot/stage2.asm:192](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:192), [os/boot/stage2.asm:203](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:203)).
2. **A numbered, specified app ABI already exists.** GUI disk apps use `INT 81h`; native text programs use the DOS-shaped flat `INT 21h` interface. The Toolbox ABI is locked and incrementally extended, but lacks explicit version/capability negotiation and exposes pointer-bearing layouts; it is not an absent ABI ([spec/toolbox_gate.h:122](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h:122), [spec/toolbox_gate.h:156](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h:156), [os/apps/tbx/tbx_rt.c:24](/home/tobiasosborne/Projects/InitechOS/os/apps/tbx/tbx_rt.c:24)).
3. **The observed flagship has 1,204 bytes of policy margin, not 120.** Existing ELF measurements give 168,780 loaded bytes against the 169,984-byte policy cap; 27,828 bytes remain against the 196,608-byte physical bounce policy. These are observations of existing artifacts, not a rebuild of HEAD. A concurrent builder may replace them. The statement “120 bytes” may describe a different revision or a larger variant; I have not verified it for the largest current kernel. The eight size rounds are independently visible in the build policy ([Makefile:781](/home/tobiasosborne/Projects/InitechOS/Makefile:781), [Makefile:802](/home/tobiasosborne/Projects/InitechOS/Makefile:802), [harness/proptest/kernel_size_r7.mk:1](/home/tobiasosborne/Projects/InitechOS/harness/proptest/kernel_size_r7.mk:1)).

## Scope, method, and limits

**Measured:** tracked source inventory, literal include directives and their resolved destinations, make's expanded dependency database (`make -np image`), dry-run commands (`make -n image`), existing ELF/object sections with `size`, existing symbols with `nm`, and manual source samples. No compiler, linker, emulator, test, image writer, or generated-header recipe was executed. `make -n` printed commands only. All investigation scripts and outputs were written beneath the requested scratchpad. `git diff --stat` remained empty; the observed untracked `.beads.gate.lock` was left alone.

`bd prime` was attempted as instructed; its database initialization reported pending migrations against dirty Dolt tables. No tracker repair, issue mutation, commit, synchronization, or push was attempted: the current task explicitly authorizes a read-only investigation.

**Inferences/recommendations** are labeled as such. Presence of a test target is evidence of an available harness, not evidence that it passes today. Physical LOC counts include comments and blank lines. A separate lexical code-LOC count removes comments/blanks, not braces or declarations. Counts cover tracked `.c/.h/.asm/.s/.S/.pas/.def`; generated files, binaries, Markdown, fixture text/data, shell/Python/make orchestration are excluded unless expressly stated.

Evidence artifacts beside this report: [make-n-image.txt](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/make-n-image.txt), [make-np-image.txt](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/make-np-image.txt), [object-sizes.txt](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/object-sizes.txt), [snapshot.txt](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/snapshot.txt), [measurements.md](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/measurements.md), [refined-metrics.md](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/refined-metrics.md), and the measurement scripts. Build-artifact observations are inherently less authoritative than the checked-in source because another process is building here.

## 1. Boot path, address ownership, and the two size caps

### Actual shipping path

1. BIOS enters the MBR at physical `0x7C00`. The MBR reads 16 sectors beginning at LBA 1 using BIOS CHS `INT 13h`, into `0:0x8000`, then jumps there ([os/boot/mbr.asm:35](/home/tobiasosborne/Projects/InitechOS/os/boot/mbr.asm:35), [os/boot/mbr.asm:66](/home/tobiasosborne/Projects/InitechOS/os/boot/mbr.asm:66)).
2. Stage2 occupies `[0x8000,0x9000)`; its legacy embedded `IO.SYS` occupies `[0x9000,0xA000)`. On a partitioned boot disk, stage2 validates the first active FAT12 partition and BPB, caches/compares both FATs, and follows the actual file chains, with bounds/cycle/cross-link/EOC checks. It does not quietly fall back to the raw kernel extent for a broken partitioned disk ([os/boot/stage2.asm:8](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:8), [os/boot/stage2.asm:70](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:70), [os/boot/stage2.asm:160](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:160), [os/boot/stage2.asm:280](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:280)).
3. The required first two root entries are `IO.SYS` and `INITDOS.SYS`. It loads 4,096 bytes of `IO.SYS` at `0x9000` and exactly `384*512` bytes of `INITDOS.SYS` at `0x10000`. The file length is required to equal that fixed padded size. Each sector read uses `INT 13h AH=02h AL=1`, with queried geometry; the destination advances by 32 paragraphs per sector ([os/boot/stage2.asm:192](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:192), [os/boot/stage2.asm:203](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:203), [os/boot/stage2.asm:296](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:296), [os/boot/stage2.asm:338](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:338)).
4. Stage2 jumps to `IO.SYS` with `DH=1` indicating the files are loaded. `IO.SYS` performs VBE/font/RAM setup, A20 enablement, GDT setup and protected-mode entry; its `DH=0` path retains the legacy raw LBA-17 loader ([os/boot/io_sys.asm:1](/home/tobiasosborne/Projects/InitechOS/os/boot/io_sys.asm:1), [os/boot/io_sys.asm:305](/home/tobiasosborne/Projects/InitechOS/os/boot/io_sys.asm:305), [os/boot/io_sys.asm:431](/home/tobiasosborne/Projects/InitechOS/os/boot/io_sys.asm:431)).
5. In flat protected mode, `IO.SYS` copies the entire padded window from `0x10000` to `0x00500000` using `rep movsd`, then far-jumps to `CODE_SEL:KERNEL_BASE`. The kernel is linked at that same physical address ([os/boot/io_sys.asm:1002](/home/tobiasosborne/Projects/InitechOS/os/boot/io_sys.asm:1002), [os/milton/kernel.ld:20](/home/tobiasosborne/Projects/InitechOS/os/milton/kernel.ld:20)).
6. `_start` selects the high kernel stack, zeroes the rest of the runway, and calls `kernel_main`. Its zeroing start also depends on `KERNEL_SECTORS`. A documented earlier mismatch actually erased the kernel's own `.rodata`; this is a concrete example of non-local placement coupling ([os/milton/kstart.asm:47](/home/tobiasosborne/Projects/InitechOS/os/milton/kstart.asm:47), [os/milton/kstart.asm:81](/home/tobiasosborne/Projects/InitechOS/os/milton/kstart.asm:81)).

`make image` copies `flair_tenants.img` to `initech.img` and also produces the separate primary-slave data volume. The boot mint currently creates a FAT12 boot partition at LBA 63 in a 46-cylinder image; the historical `IMG_SECTORS=448` raw-image constant is not the total size of that FAT shipping image. `INITDOS.SYS` is the flagship kernel with an overlay identity/CRC slot patched after overlay linking ([Makefile:12290](/home/tobiasosborne/Projects/InitechOS/Makefile:12290), [Makefile:10916](/home/tobiasosborne/Projects/InitechOS/Makefile:10916), [tools/mk_boot_system.sh:29](/home/tobiasosborne/Projects/InitechOS/tools/mk_boot_system.sh:29)).

### Current map (end addresses exclusive except ESP values)

|Owner|Physical range / value|Source|
|---|---|---|
|Boot handoff/font/scratch|boot_info `0x500`; font stash `0x1000`; VBE `0x7000/0x7200`; E820 `0x7400`|[os/boot/io_sys.asm:75](/home/tobiasosborne/Projects/InitechOS/os/boot/io_sys.asm:75)|
|Stage2 / IO.SYS|`0x8000..0xA000`|[os/boot/stage2.asm:8](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:8)|
|Whole-kernel bounce|`0x10000..0x40000`, 192 KiB|[spec/memory_map.h:166](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:166)|
|Conventional program PSP|`PROGRAM_BASE=0x40000`|[spec/memory_map.h:217](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:217)|
|Conventional program entry/image|`0x40100..0x6F000` maximum, before environment|[spec/memory_map.h:226](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:226), [spec/memory_map.h:297](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:297)|
|Environment|`0x6F000..0x70000`|[spec/memory_map.h:228](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:228)|
|Conventional program stack|`0x70000..0x80000`, ESP `0x7FFFC`|[spec/memory_map.h:267](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:267), [spec/memory_map.h:285](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:285)|
|EXEC staging|`0x80000..0x90000`, 64 KiB|[spec/memory_map.h:444](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:444)|
|Retired low kernel stack|`0x90000..0xA0000`, now unassigned after boot; bootstrap transient ESP remains `0x90000`|[spec/memory_map.h:43](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:43), [os/milton/kstart.asm:86](/home/tobiasosborne/Projects/InitechOS/os/milton/kstart.asm:86)|
|FLAIR heap|`0x100000..0x500000`, 4 MiB|[spec/memory_map.h:526](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:526)|
|Kernel image **including BSS**|`0x500000..0x580000`, 512 KiB placement budget|[spec/memory_map.h:160](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:160), [os/milton/kernel.ld:70](/home/tobiasosborne/Projects/InitechOS/os/milton/kernel.ld:70)|
|COMMAND.COM transient|`0x580000..0x5C0000`, 256 KiB|[spec/memory_map.h:189](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:189)|
|FLAIR.OVL|`0x5C0000..0x5E0000`, 128 KiB|[spec/memory_map.h:206](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:206)|
|Kernel stack separation|`0x5E0000..0x5F0000`, unassigned 64 KiB, not a protected page|[spec/memory_map.h:203](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:203)|
|Kernel stack|`0x5F0000..0x600000`, ESP `0x5FFFFC`|[spec/memory_map.h:164](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:164)|
|Extended text application|`0x600000..0x800000`, one owner; guard `0x7EF000..0x7F0000`; stack `0x7F0000..0x800000`|[spec/memory_map.h:577](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:577), [docs/adr/ADR-0009-AMENDMENT-DEC-03a-InitechMZ-Extended-Memory.md:99](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0009-AMENDMENT-DEC-03a-InitechMZ-Extended-Memory.md:99)|

The current minimum is an **8 MiB machine**: `INITECH_MIN_EXT_KB` derives from `EXT_APP_CEIL`, so it is 7,168 KiB above the 1 MiB line. Earlier 5,120 KiB requirements and the proposed stack at `0x4F0000` in the runway design are historical; the latter overlapped the FLAIR heap and was stopped. Do not design from the old diagram without its amendments ([spec/memory_map.h:34](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:34), [spec/memory_map.h:168](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:168)).

### What creates the caps

**196,608 bytes is a whole-image staging contract, not a 486 limit and not the kernel's high-address memory capacity.** `0x40000 - 0x10000 = 0x30000 = 196608 = 384*512`. The stage2 and IO.SYS assembly guards, Makefile parse-time guard, fixed file-length check, fixed post-PE copy count, and `_start` zeroing boundary jointly enforce it. Increasing only a sector count or linker address is unsound ([Makefile:240](/home/tobiasosborne/Projects/InitechOS/Makefile:240), [Makefile:270](/home/tobiasosborne/Projects/InitechOS/Makefile:270), [Makefile:307](/home/tobiasosborne/Projects/InitechOS/Makefile:307), [os/boot/stage2.asm:25](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:25), [os/boot/stage2.asm:206](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:206), [os/boot/io_sys.asm:1011](/home/tobiasosborne/Projects/InitechOS/os/boot/io_sys.asm:1011), [os/milton/kstart.asm:70](/home/tobiasosborne/Projects/InitechOS/os/milton/kstart.asm:70)).

The high kernel move removed the old *residence* conflict with conventional apps but intentionally retained the entire low bounce. The design describes that minimal-risk choice and explicitly acknowledges that growth beyond 384 sectors needs a bounce-policy decision; its earlier “decoupled ... forever” wording does not remove this load-path dependency ([docs/design/kernel-runway-relocation.md:39](/home/tobiasosborne/Projects/InitechOS/docs/design/kernel-runway-relocation.md:39), [docs/design/kernel-runway-relocation.md:85](/home/tobiasosborne/Projects/InitechOS/docs/design/kernel-runway-relocation.md:85)).

**169,984 bytes is an administrative headroom policy:** `196608 - 4096 - 2*11264`. ADR-0014 reserves a 4 KiB safety floor and two worst-case measured 11 KiB feature slices. It applies to kernel targets whose names contain neither `mut` nor `_record`; that naming filter includes diagnostic kernels as well as end-user configurations. The separate mutant-aware gate requires at least 4 KiB under the hard bounce ceiling across all variants. This policy is stricter than the boot mechanism and is deliberately retained by the extended-app amendment ([docs/adr/ADR-0014-Kernel-Overlays.md:247](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0014-Kernel-Overlays.md:247), [harness/proptest/overlay.mk:214](/home/tobiasosborne/Projects/InitechOS/harness/proptest/overlay.mk:214), [harness/proptest/overlay.mk:230](/home/tobiasosborne/Projects/InitechOS/harness/proptest/overlay.mk:230), [Makefile:418](/home/tobiasosborne/Projects/InitechOS/Makefile:418), [docs/adr/ADR-0009-AMENDMENT-DEC-03a-InitechMZ-Extended-Memory.md:274](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0009-AMENDMENT-DEC-03a-InitechMZ-Extended-Memory.md:274)).

Existing flagship ELF snapshot:

|Quantity|Bytes|
|---|---:|
|`.text`|147,871|
|`.rodata`|20,112|
|`.data`|796|
|Loaded span, including alignment gaps|168,780|
|`.bss`, absent from file payload|93,488|
|Image+BSS span (`_kernel_end=0x540090`)|262,288|
|Policy margin|1,204|
|Bounce margin|27,828|

The `.bin` and installed `INITDOS.SYS` are padded, so measuring their file size simply gives the 196,608-byte window. Loaded payload must be measured from the unpadded ELF span / an unpadded extraction, as the policy gate does ([Makefile:10905](/home/tobiasosborne/Projects/InitechOS/Makefile:10905), [harness/proptest/overlay.mk:219](/home/tobiasosborne/Projects/InitechOS/harness/proptest/overlay.mk:219)).

### Simplest sound removal of the constraint (recommendation)

**Replace whole-image low staging with a bounded reusable read buffer and copy each chunk to its final high destination. Keep the existing FAT file parser.** A practical 486 implementation is:

- Enable and verify A20 before the first high copy. Keep BIOS `INT 13h` reads in real mode into an aligned low buffer (e.g. one sector initially, or at most 32 KiB per batch with track/DMA limits respected).
- After each buffer fill, enter a short flat protected-mode copy helper, copy to `KERNEL_BASE + file_offset`, and return to the BIOS-capable real-mode path. Preserve FAT traversal state, real-mode stack/segments, interrupt discipline and the GDT transition. An unreal-mode copy can also work, but must restore/re-establish extended segment limits after BIOS calls; assuming BIOS preserves them is not a sound simplification.
- Read the actual bounded kernel-file length or a small checked kernel header, rather than insisting that every kernel is a 384-sector padded file. Check destination overflow, extent, entry address, and available RAM **before** high writes. Preserve deterministic BSS initialization through linker-delimited BSS/zero ranges, rather than infer it from a shared sector-count constant.
- Generate the bootstrap constants from one contract. Regrade the real BIOS/Bochs paths, chain corruption and placement errors. The legacy raw-image loader can either adopt the same copy sink or be explicitly retired; silently leaving it with different capacity rules is another trap.

This is a bounded bootloader change. It removes the 192 KiB *load-buffer* constraint permanently as a function of file size. Retire or re-derive the 169,984-byte policy from actual module reservations at the same time; changing the loader while keeping that fixed policy gate would leave feature merges blocked. Lifting only the policy cap would buy room temporarily and would not solve the load path. It does **not** create unlimited physical RAM: the present 512 KiB image+BSS ceiling, shell/overlay/stack windows, and 8 MiB RAM contract remain separate limits. Initially use the already reserved image region. Longer term, give independently loaded kernel/toolbox modules explicit non-overlapping reservations and move/retire the old overlay windows as appropriate. Do not simply grow into `0x580000`, `0x5F0000`, or the SAMIR region ([spec/memory_map.h:141](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:141)).

Reading a FAT file is already implemented and is therefore **not** the missing engineering change. Increasing `PROGRAM_BASE` or spending the now-free old stack window only postpones the limit and changes existing COM origins/PSP contracts. The staged-copy design permits `PROGRAM_BASE`, transient addresses, and stack window to stay fixed initially. **Governance fact:** ADR-0014 records an explicit operator choice of overlays over a bounce redesign; the recommendation requires a deliberate superseding architecture decision, not an unannounced implementation tweak ([docs/adr/ADR-0014-Kernel-Overlays.md:23](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0014-Kernel-Overlays.md:23)).

## 2. What is linked into the flagship

`make image` selects `kernel_flairtenants`, via `flair_tenants.img`. It is the **bounded oracle desktop**, not the unbounded interactive desktop; it boots HELLO/NOTES and halts after its tick budget. `make run-flair-tenants` selects an interactive sibling ([Makefile:12278](/home/tobiasosborne/Projects/InitechOS/Makefile:12278), [Makefile:12285](/home/tobiasosborne/Projects/InitechOS/Makefile:12285), [os/milton/kmain.c:6772](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:6772), [os/milton/kmain.c:6833](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:6833)).

The expanded link command has **60 explicit objects plus two objects inserted through `@build/transient_imports.rsp` = 62 objects**. The response file also holds 166 forced-symbol roots in the observed artifact. Omitting it would miss the overlay stubs and import expectations. Its contents are generated from both sides of the resident/overlay/transient boundary ([Makefile:10902](/home/tobiasosborne/Projects/InitechOS/Makefile:10902), [Makefile:10959](/home/tobiasosborne/Projects/InitechOS/Makefile:10959), [Makefile:10973](/home/tobiasosborne/Projects/InitechOS/Makefile:10973), [Makefile:10984](/home/tobiasosborne/Projects/InitechOS/Makefile:10984)).

The exact object inventory and existing `size` values are in Appendix A. Many objects report zero native text/data because they contain **GCC LTO IR**. That is not evidence that they contribute zero bytes; final attribution after whole-program LTO requires a link map or a separate attribution build. Neither was generated here. Input-object sizes also overstate retained size when section GC removes unused code. For example, test/type/dir blobs are link inputs, but their symbols were absent in the observed final ELF ([os/milton/kernel.ld:27](/home/tobiasosborne/Projects/InitechOS/os/milton/kernel.ld:27), [Makefile:838](/home/tobiasosborne/Projects/InitechOS/Makefile:838)).

**FLAIR + Finder:** yes, the actual manager implementation, desktop compositor, process manager, menu/window/dialog/control/text/scrap/Standard File/list services, disk-tenant gate, Finder desktop/windows/commands/menu, and HELLO/NOTES reference tenant code participate in the resident link. This is not a tiny DOS kernel that launches an independent GUI server. “Whole FLAIR” needs qualification: cold portions are in `FLAIR.OVL`, and `textedit.c` is compiled into the Text/Notepad/Scrapbook app images instead of appearing in the flagship resident object list ([Makefile:10828](/home/tobiasosborne/Projects/InitechOS/Makefile:10828), [Makefile:10649](/home/tobiasosborne/Projects/InitechOS/Makefile:10649), [Makefile:10842](/home/tobiasosborne/Projects/InitechOS/Makefile:10842)).

**COMMAND.COM:** the flagship does **not** link `command.o`, `env.o`, or `batch.o` into resident code. It carries a custom transient file called `COMMAND.COM`, linked at `0x580000` against that kernel's exact symbol addresses. Exit-to-DOS loads it through the private boot-volume mount. Shell-only kernel variants directly link those same interpreter objects instead ([Makefile:10923](/home/tobiasosborne/Projects/InitechOS/Makefile:10923), [Makefile:10943](/home/tobiasosborne/Projects/InitechOS/Makefile:10943), [Makefile:10071](/home/tobiasosborne/Projects/InitechOS/Makefile:10071), [os/milton/transient.h:12](/home/tobiasosborne/Projects/InitechOS/os/milton/transient.h:12)). The filename is misleading if read as an ordinary executable: the file is an overlay with `COMMAND3` magic, and ordinary EXEC refuses it; a secondary command interpreter is routed to a resident interpreter entry ([os/milton/transient.h:33](/home/tobiasosborne/Projects/InitechOS/os/milton/transient.h:33)).

**FLAIR.OVL:** current source objects are `egg_jtc.o`, `about_box_ovl.o`, `kmain_cold_ovl.o`, `finder_actions_ovl.o`, `finder_ops_ovl.o`, `finder_windows_ovl.o`, `power_cold_ovl.o`, and `loader_cold_ovl.o`, plus `overlay_hdr.o`. Thus it includes the easter egg/About box and formatter, cold Finder action/operation/window bodies, shutdown/restart/Exit-to-DOS paths, and cold loader validation. It is kernel code, not an application or a full independent Toolbox. It is preloaded at desktop start, checked against the exact kernel build, never evicted, and called through generated resident stubs; reverse calls import checked resident addresses. Existing overlay ELF sections total 860 header + 17,214 text + 4,096 rodata + 2,588 BSS, with alignment gaps additional ([Makefile:10835](/home/tobiasosborne/Projects/InitechOS/Makefile:10835), [Makefile:10842](/home/tobiasosborne/Projects/InitechOS/Makefile:10842), [Makefile:11023](/home/tobiasosborne/Projects/InitechOS/Makefile:11023), [docs/adr/ADR-0014-Kernel-Overlays.md:64](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0014-Kernel-Overlays.md:64), [docs/adr/ADR-0014-Kernel-Overlays.md:347](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0014-Kernel-Overlays.md:347)).

## 3. What kmain.c really is

**Measured:** 8,207 physical lines; 400,037 bytes; 137 `#if/#ifdef/#ifndef` openings (143 counting `#elif`); 89 distinct uppercase identifiers in condition expressions; 25 `BOOT_*` selectors. The user's approximate counts are substantially right.

**It is mainly a desktop integration/program file, not 8,000 lines of boot initialization and not mainly a test suite.** Useful source regions:

|Region|Role|Evidence|
|---|---|---|
|early file through ~995|shared bindings: DOS callbacks, process/EXEC context, keyboard/idle/TSR, filesystem/volume glue|[os/milton/kmain.c:386](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:386), [os/milton/kmain.c:830](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:830)|
|~996–1535|FLAIR boot frame and composed scene construction|[os/milton/kmain.c:996](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:996)|
|~1537–3150|live input, cursor, tracking, close/zoom/grow/menu/modal helpers|[os/milton/kmain.c:1537](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:1537)|
|~3152–5744|Finder bindings/action glue, tenant operations, boot-volume/transient/overlay management|[os/milton/kmain.c:3152](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:3152), [os/milton/kmain.c:5264](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:5264)|
|5746–5935|explicit headless tenant/app smoke scenario|[os/milton/kmain.c:5746](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:5746)|
|5937–6214|entry/handoff validation and common foundation bring-up, approximately 278 lines|[os/milton/kmain.c:5937](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:5937), [os/milton/kmain.c:6039](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:6039)|
|6215–7267|FLAIR scene/tenant setup, resident references, event pump; bounded and interactive variants|[os/milton/kmain.c:6215](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:6215), [os/milton/kmain.c:6755](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:6755)|
|7269 onward|fault/IRQ/FPU/EXEC/memory/file/date/sysinit demonstrations, shared DOS mounting/setup, shell and diagnostic endings|[os/milton/kmain.c:7269](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:7269), [os/milton/kmain.c:7408](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:7408), [os/milton/kmain.c:7619](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:7619), [os/milton/kmain.c:8083](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:8083)|

An inspection-only conditional evaluator for the ordinary bounded flagship retains approximately **6,434 non-directive physical lines (78%)**, including comments; 5,131 are outside `kernel_main` and 1,303 inside. This is a structural estimate, not compiler output: it does not expand included headers, inline/LTO/GC, or establish runtime reachability. It demonstrates why “extract the tests and kmain becomes tiny” is false. Explicit test-scenario blocks account for roughly a tenth of physical source; alternate production personalities, comments, and interspersed mutation branches account for more. The main saving is to **move the production desktop and OS bindings out**, then give each scenario its own entry TU.

The 25 BOOT selectors are:

`ABSDISK, CONIN, DATETIME, DOSMSG_FAULT, EXEC, EXEC_NESTED, EXEC_OVERLAY, EXITH, FLAIR_LIVE, FLAIR_SHELL, FPU_MF, FPU_TEST, IRQSTORM, KBD_ECHO, MEMTEST, MULTIOPEN, MZEXEC, MZFOREIGN, SECONDARY_SHELL, SELFTEST_FAULT, SHELL, SPURIOUS, SYSINIT, VECT, WRITE` (each prefixed `BOOT_`). They are composable flags, not 25 mutually exclusive operating systems. Broad user-facing personalities are DOS shell, static FLAIR render, live chrome demo, and FLAIR tenant desktop; live variants add bounded/interactive/film/recording choices. Most other selectors create direct subsystem oracle guests so failures can be triggered without a user application ([Makefile:1320](/home/tobiasosborne/Projects/InitechOS/Makefile:1320), [Makefile:1331](/home/tobiasosborne/Projects/InitechOS/Makefile:1331), [Makefile:1346](/home/tobiasosborne/Projects/InitechOS/Makefile:1346), [Makefile:10497](/home/tobiasosborne/Projects/InitechOS/Makefile:10497), [os/milton/kmain.c:7269](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:7269)).

The expanded make database contains **352 explicit kernel `.bin` targets**, of which **80 survive the current non-mut/non-record naming filter**. Those 80 include diagnostics. The rest include mutants and recording variants. This is a measurement of generated targets, not 352 conceptual OS personalities or 352 artifacts built in this investigation ([harness/proptest/overlay.mk:230](/home/tobiasosborne/Projects/InitechOS/harness/proptest/overlay.mk:230), [Makefile:11189](/home/tobiasosborne/Projects/InitechOS/Makefile:11189)).

**Yes: tests and test hooks are baked into the shipped artifact's *source*** through `#ifdef`, including injected key scenarios, deliberately broken branches, diagnostic launch modes, pacing, and bounded exits. Most mutant branches are not compiled into the ordinary artifact. The ordinary `make image`, however, deliberately uses the bounded desktop with 250 ticks and terminal halt; that oracle behavior really is compiled into its artifact ([os/milton/kmain.c:5819](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:5819), [os/milton/kmain.c:6772](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:6772), [os/milton/kmain.c:6833](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:6833), [os/milton/kmain.c:7254](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:7254)).

## 4. Application model and source dependencies

### The three materially different interfaces

**A. GUI disk tenants.** A C application links its own source/model with `os/apps/tbx/tbx_rt.c`; a linker script places `tbx_entry` at module byte zero. The build links twice, at zero and a probe address, derives relocations from the difference, wraps an InitechMZ EXE, and records BSS as `e_minalloc`. Current GUI file cap is 64 KiB. Finder → `tbx_launch` → `loader_load_tenant` allocates in the FLAIR heap; registration adds kernel-owned app/window bookkeeping. Apps receive push callbacks and return to yield; they do not own event loops. Only **one disk-tenant slot** exists, alongside the general list of linked/reference tenants ([Makefile:10538](/home/tobiasosborne/Projects/InitechOS/Makefile:10538), [Makefile:10624](/home/tobiasosborne/Projects/InitechOS/Makefile:10624), [os/apps/tbx/tbx_rt.c:38](/home/tobiasosborne/Projects/InitechOS/os/apps/tbx/tbx_rt.c:38), [os/flair/tbxgate.c:1](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:1), [docs/adr/ADR-0013-AMENDMENT-AC-3-Toolbox-Gate-and-Disk-Tenants.md:66](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0013-AMENDMENT-AC-3-Toolbox-Gate-and-Disk-Tenants.md:66)).

**B. Text/native programs.** Conventional flat 32-bit COMs execute at `PROGRAM_IMAGE`; tagged InitechMZ programs can be relocated. They use DOS-numbered `INT 21h` operations with a project-specific flat calling convention; these are not arbitrary binary-compatible real-mode DOS programs. SAMIR now uses an extended InitechMZ tag and the high single-owner region, with low PSP metadata and a distinct full-width allocation model. The desktop suspends around class-2 text execution ([spec/memory_map.h:217](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h:217), [seed/rt/start_dos.asm:27](/home/tobiasosborne/Projects/InitechOS/seed/rt/start_dos.asm:27), [docs/adr/ADR-0009-AMENDMENT-DEC-03a-InitechMZ-Extended-Memory.md:124](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0009-AMENDMENT-DEC-03a-InitechMZ-Extended-Memory.md:124), [docs/adr/ADR-0009-AMENDMENT-DEC-03a-InitechMZ-Extended-Memory.md:164](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0009-AMENDMENT-DEC-03a-InitechMZ-Extended-Memory.md:164), [docs/adr/ADR-0013-AMENDMENT-AC-3-Toolbox-Gate-and-Disk-Tenants.md:70](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0013-AMENDMENT-AC-3-Toolbox-Gate-and-Disk-Tenants.md:70)).

**C. Resident/overlay code.** Reference tenants and Finder directly call internal C Toolbox APIs; COMMAND.COM and FLAIR.OVL import exact resident symbol addresses. This is a build-specific internal interface, distinct from the disk-app trap ABI ([Makefile:10828](/home/tobiasosborne/Projects/InitechOS/Makefile:10828), [os/milton/transient.h:26](/home/tobiasosborne/Projects/InitechOS/os/milton/transient.h:26), [Makefile:11023](/home/tobiasosborne/Projects/InitechOS/Makefile:11023)).

### Per-application inventory

All GUI rows below use **A**, unless stated. “Links a FLAIR source” means it embeds that library implementation in its own EXE; it does not mean it is resident in the kernel.

|App|Build / installed identity|Loading, OS interface, implementation sharing|
|---|---|---|
|calc|`tenant/calc.exe` → `APPS/CALC.EXE`|GUI tenant; TBX runtime plus direct **i123 model/codec/formula/range** sources, not the Calculator DA engine. Includes `i123.h` and the WK1 contract. This is a real app-to-app source dependency ([harness/frameapps.mk:3](/home/tobiasosborne/Projects/InitechOS/harness/frameapps.mk:3), [os/apps/calc/calc.c:9](/home/tobiasosborne/Projects/InitechOS/os/apps/calc/calc.c:9)).|
|calcda|`tenant/calcda.exe` → `APPS/CALCULAT.EXE`|GUI desk accessory; `calcda.c` + separate calculator engine/view; `INT 81h` including rounded-window request; installed in Apple menu ([Makefile:10687](/home/tobiasosborne/Projects/InitechOS/Makefile:10687), [Makefile:10693](/home/tobiasosborne/Projects/InitechOS/Makefile:10693)).|
|cpanels|`tenant/cpanels.exe` → `APPS/CPANELS.EXE`|GUI tenant + `core.c`; uses TBX calls and shared records. Substantial system-dialog/preferences behavior is implemented in Toolbox/kernel service code through the control-panel extension traps, so the app is partly a controller for resident services ([Makefile:34618](/home/tobiasosborne/Projects/InitechOS/Makefile:34618), [Makefile:34639](/home/tobiasosborne/Projects/InitechOS/Makefile:34639), [os/apps/cpanels/cpanels.c:5](/home/tobiasosborne/Projects/InitechOS/os/apps/cpanels/cpanels.c:5), [spec/toolbox_gate.h:245](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h:245), [os/flair/cp_service.h:8](/home/tobiasosborne/Projects/InitechOS/os/flair/cp_service.h:8)).|
|ctenant|`tenant/ctenant.exe` → `APPS/CTENANT.EXE`|Small C registration/window/event fixture; TBX only. `ctenant/files.c` is a separate `FILETEN` file-ABI fixture, not the ordinary CTENANT image ([Makefile:10644](/home/tobiasosborne/Projects/InitechOS/Makefile:10644), [Makefile:31967](/home/tobiasosborne/Projects/InitechOS/Makefile:31967), [os/apps/ctenant/files.c:1](/home/tobiasosborne/Projects/InitechOS/os/apps/ctenant/files.c:1)).|
|fonts|`tenant/fonts.exe`|TBX font/style specimen; built for its gate-local volume, **not installed in flagship** ([Makefile:10680](/home/tobiasosborne/Projects/InitechOS/Makefile:10680)).|
|itext|`tenant/itext.exe` → `APPS/TEXT.EXE`|TBX for windows/events/drawing/files/scrap; embeds `os/flair/textedit.c` and app font metrics, includes `textedit.h` and `text.h`. This is library-code/header sharing across the apparent app/Toolbox directory boundary ([Makefile:10649](/home/tobiasosborne/Projects/InitechOS/Makefile:10649), [os/apps/itext/itext.c:71](/home/tobiasosborne/Projects/InitechOS/os/apps/itext/itext.c:71)).|
|notepad / scrapbook|`tenant/notepad.exe` and `scrapbook.exe` → `NOTEPAD.EXE`, `SCRAPBK.EXE`|One app source with `SCRAPBOOK` variant; same embedded TextEdit/font sources and TBX services. Not two fully separate engines ([Makefile:10668](/home/tobiasosborne/Projects/InitechOS/Makefile:10668), [Makefile:10780](/home/tobiasosborne/Projects/InitechOS/Makefile:10780)).|
|paint|`tenant/paint.exe` → `APPS/PAINT.EXE`|Small TBX tenant scaffold/blank canvas with menu behavior, not a substantial painting engine; host gate explicitly grades the blank canvas and mostly disabled menu ([harness/frameapps.mk:289](/home/tobiasosborne/Projects/InitechOS/harness/frameapps.mk:289), [os/apps/paint/paint.c:1](/home/tobiasosborne/Projects/InitechOS/os/apps/paint/paint.c:1)).|
|i123|`tenant/i123.exe` → `APPS/123.EXE`, plus `123.MNU`|`app/face_a.c` + pure core + WK1 codec + TBX runtime. File/menu resources and aggressive app-specific flags keep it under the 64 KiB tenant cap. **No i123 source in resident kernel list** ([Makefile:10701](/home/tobiasosborne/Projects/InitechOS/Makefile:10701), [Makefile:10707](/home/tobiasosborne/Projects/InitechOS/Makefile:10707), [Makefile:10581](/home/tobiasosborne/Projects/InitechOS/Makefile:10581)).|
|iword|`tenant/iword.exe` → `APPS/WORD.EXE`|Face A + document/edit/codes/block/search/listfiles core + WP5 codec + TBX runtime. BSS holds document banks/piece table; no resident kernel app link ([Makefile:10716](/home/tobiasosborne/Projects/InitechOS/Makefile:10716)).|
|SAMIR / InitechBase|`SAMIR.EXE` + `SAMIR.OVL`; separate `samir-exe` build|Extended InitechMZ text app, **B**; core/fs/cmd/main/screen + PAL Milton + soft-float runtime + app overlay machinery. Sole OS adapter issues `INT 21h`; not a Toolbox-linked GUI tenant. Current high application region is separate from FLAIR heap and kernel runway. Its own overlay is distinct from FLAIR.OVL ([Makefile:28967](/home/tobiasosborne/Projects/InitechOS/Makefile:28967), [Makefile:28988](/home/tobiasosborne/Projects/InitechOS/Makefile:28988), [os/samir/pal/pal_milton.c:5](/home/tobiasosborne/Projects/InitechOS/os/samir/pal/pal_milton.c:5), [docs/adr/ADR-0009-AMENDMENT-DEC-03a-InitechMZ-Extended-Memory.md:99](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0009-AMENDMENT-DEC-03a-InitechMZ-Extended-Memory.md:99)).|
|TPS / Turbo Initech|`tps.pas` compiled by host seed to assembly/ELF, DOS runtime → `TPS.COM`; host FPC binary also built for oracles|Flat 32-bit COM, **B**, seed runtime/INT21 file RTL. The actual current source is a **single-file bootstrap compiler** reading `TPSIN.PAS` and emitting trace/symbol/codegen protocols, not a completed independent blue IDE. `seed/` is the host C bootstrap compiler, not linked into MILTON ([Makefile:17583](/home/tobiasosborne/Projects/InitechOS/Makefile:17583), [Makefile:17600](/home/tobiasosborne/Projects/InitechOS/Makefile:17600), [seed/rt/start_dos.asm:20](/home/tobiasosborne/Projects/InitechOS/seed/rt/start_dos.asm:20), [os/tps/tps.pas:3](/home/tobiasosborne/Projects/InitechOS/os/tps/tps.pas:3)).|
|SideKick|`TSR.COM` fixture from `tsr_program.asm`, with `WAITKEY.COM` variant|No full SideKick application found. This is a minimal SideKick-shaped residency/IRQ/idle/screen-save proof. Conventional COM, `INT 21h` AH=31h/34h/vector services plus project idle/screen vectors; directly reads keyboard flags at `0x417` ([os/milton/tsr_program.asm:1](/home/tobiasosborne/Projects/InitechOS/os/milton/tsr_program.asm:1), [os/milton/tsr_program.asm:58](/home/tobiasosborne/Projects/InitechOS/os/milton/tsr_program.asm:58), [Makefile:35540](/home/tobiasosborne/Projects/InitechOS/Makefile:35540)).|
|COMMAND.COM|Flagship custom transient; shell variants resident|**C**, exact-symbol imports; not an independently EXEC-loadable COM. Same command/env/batch source is relinked per kernel; secondary interpreter dispatch is special-cased ([os/milton/transient.h:33](/home/tobiasosborne/Projects/InitechOS/os/milton/transient.h:33), [Makefile:10923](/home/tobiasosborne/Projects/InitechOS/Makefile:10923)).|
|COMMAND externals|`CHKDSK, ATTRIB, LABEL, XCOPY, TREE, EDLIN, MORE, FIND, SORT, FDISK, FORMAT, SYS, DISKCOPY, DISKCOMP` as `.COM` files|Conventional native flat programs, **B**, built from C/assembly under `os/milton`; loader/INT21 rather than linking kernel FLAIR managers. The canonical deployment list is centralized in the factory ([Makefile:10736](/home/tobiasosborne/Projects/InitechOS/Makefile:10736)). Their location under the kernel directory obscures their app boundary.|
|HELLO / NOTES / Finder|Linked reference tenant / resident desktop|Direct C App Contract vtables and FLAIR structures, **C**. These are the important exceptions to “completely independent apps” ([Makefile:10828](/home/tobiasosborne/Projects/InitechOS/Makefile:10828), [os/apps/ref_tenant.c:77](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_tenant.c:77), [os/milton/kmain.c:110](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:110)).|

The ordinary data-volume recipe installs the GUI apps and DOS externals, not `SAMIR.EXE` or `TPS.COM`; `TPSREP.COM` is the report fixture, a different program. SAMIR/TPS integration uses separate build/fixture workflows. Do not infer product installation from a target merely existing ([Makefile:10759](/home/tobiasosborne/Projects/InitechOS/Makefile:10759), [Makefile:10775](/home/tobiasosborne/Projects/InitechOS/Makefile:10775), [Makefile:28991](/home/tobiasosborne/Projects/InitechOS/Makefile:28991), [Makefile:17601](/home/tobiasosborne/Projects/InitechOS/Makefile:17601)).

### How frozen is the ABI?

There is a real **locked numbered trap table**: vector `0x81`, selector `0x08`, trap attribute `0x8F`, function numbers such as REGISTER `0x0001`, NEWWINDOW `0x0020`, drawing `0x0030/33`, file services `0x0080+`, explicit arities, negative error results, fixed 68-byte interrupt-frame offset, fixed tenant/event/menu records, and `_Static_assert` layout checks. These establish a binary boundary, not merely convenient C function calls ([spec/toolbox_gate.h:122](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h:122), [spec/toolbox_gate.h:135](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h:135), [spec/toolbox_gate.h:156](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h:156), [spec/toolbox_gate.h:201](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h:201), [os/apps/tbx/tbx_rt.c:24](/home/tobiasosborne/Projects/InitechOS/os/apps/tbx/tbx_rt.c:24)).

However, **“versioned SDK” overstates it**. V1/V2/etc are editorial additions in comments, not an ABI-version negotiation trap/required-version field/capability manifest. The registration tag is `FLTP`, not a version range. Menu records expose a relocated pointer graph with the same i386 layout as internal `MenuBar`; the kernel stores that pointer, and 64-bit host tests skip the i386-layout pinning path. Numbers are much better isolated than some data types and lifetimes ([spec/toolbox_gate.h:150](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h:150), [spec/toolbox_gate.h:377](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h:377), [os/apps/tbx/tbx.h:62](/home/tobiasosborne/Projects/InitechOS/os/apps/tbx/tbx.h:62), [os/flair/tbxgate.c:24](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:24), [os/flair/tbxgate.c:96](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:96)).

Everything here is cooperative ring 0. The gate has DPL=0; there is no hardware process isolation. Fault triage checks address ranges and app contracts; it is not protection against arbitrary stores or a non-returning callback. That is an accepted product constraint, not a reason to rewrite the cores ([spec/toolbox_gate.h:123](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h:123), [docs/adr/ADR-0013-FLAIR-App-Contract.md:141](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0013-FLAIR-App-Contract.md:141)).

### Measured cross-component include graph

Counts below are **literal C `#include` occurrences**, not unique headers, transitive dependencies, or actual preprocessed branches. Origin components are every `os/<component>` plus `seed`; external destinations `spec`, `seed`, and `harness` are retained. All tracked source/header/test files in these origin components were scanned. Same-component edges and standard library headers are omitted. **Every resolved cross-component pair is listed.** `menu.h` in i123 Face A resolves to **its own core header**, not FLAIR's same-named header ([Makefile:10710](/home/tobiasosborne/Projects/InitechOS/Makefile:10710), [os/i123/app/face_a.c:75](/home/tobiasosborne/Projects/InitechOS/os/i123/app/face_a.c:75)).

|From|To|All occurrences|Ordinary non-test occurrences|Representative include site|
|---|---|---:|---:|---|
|apps|flair|12|12|[os/apps/itext/itext.c:71](/home/tobiasosborne/Projects/InitechOS/os/apps/itext/itext.c:71)|
|apps|i123|1|1|[os/apps/calc/calc.c:10](/home/tobiasosborne/Projects/InitechOS/os/apps/calc/calc.c:10)|
|apps|spec|19|19|[os/apps/calc/calc.c:11](/home/tobiasosborne/Projects/InitechOS/os/apps/calc/calc.c:11)|
|flair|milton|7|7|[os/flair/cp_service.h:10](/home/tobiasosborne/Projects/InitechOS/os/flair/cp_service.h:10)|
|flair|spec|84|84|[os/flair/apple_menu.c:8](/home/tobiasosborne/Projects/InitechOS/os/flair/apple_menu.c:8)|
|i123|apps (TBX SDK)|2|2|[os/i123/app/face_a.c:72](/home/tobiasosborne/Projects/InitechOS/os/i123/app/face_a.c:72)|
|i123|spec|13|13|[os/i123/app/face_a.c:76](/home/tobiasosborne/Projects/InitechOS/os/i123/app/face_a.c:76)|
|iword|apps (TBX SDK)|2|2|[os/iword/app/face_a.c:165](/home/tobiasosborne/Projects/InitechOS/os/iword/app/face_a.c:165)|
|iword|spec|14|14|[os/iword/app/face_a.c:172](/home/tobiasosborne/Projects/InitechOS/os/iword/app/face_a.c:172)|
|milton|harness|6|0 (4 test files, 2 host-conditionals)|[os/milton/test_kbd.c:37](/home/tobiasosborne/Projects/InitechOS/os/milton/test_kbd.c:37), [os/milton/command.c:3067](/home/tobiasosborne/Projects/InitechOS/os/milton/command.c:3067)|
|milton|apps|2|2|[os/milton/kmain.c:110](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:110)|
|milton|flair|39|39|[os/milton/boot_screen.h:42](/home/tobiasosborne/Projects/InitechOS/os/milton/boot_screen.h:42)|
|milton|seed|40|0 (test assertion helper)|[os/milton/test_40oq.c:82](/home/tobiasosborne/Projects/InitechOS/os/milton/test_40oq.c:82)|
|milton|spec|62|42|[os/milton/block_device.c:4](/home/tobiasosborne/Projects/InitechOS/os/milton/block_device.c:4)|
|samir|spec|5|5|[os/samir/core/ovl.c:32](/home/tobiasosborne/Projects/InitechOS/os/samir/core/ovl.c:32)|

Thus, **the production OS-component cycle is MILTON ↔ FLAIR**, plus MILTON → resident reference apps → FLAIR. SAMIR has no C-header edge into MILTON or FLAIR: its PAL crosses a numeric runtime interface. i123/iword → apps is largely a misplaced SDK directory, not a dependency on unrelated app logic. The calc → i123 source/library dependency is real ([os/flair/tbxgate.c:84](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:84), [os/flair/tbxfile.h:8](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxfile.h:8), [os/apps/ref_tenant.h:29](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_tenant.h:29)).

Appendix B lists every include site for the above edges. Unresolved generated headers (`dos_messages.h`, packed fonts, `flair_dac.h`) are not guessed into component counts. They create additional **build/generation** dependencies on `spec`/tools, visible in the factory, not missing OS-to-OS include edges. NASM `%include`, Pascal source-level semantic mirroring, direct external declarations, imported ELF symbols, shared resources and build dependencies are different graphs; none are implied absent by this small include graph. The table excludes outgoing edges from the entire factory `harness`/`spec` trees, whose purpose is to depend on products, and includes their *incoming* dependencies from all requested source components.

## 5. Isolation and host-test reality

**Can every app be separately built?** There are individual EXE/COM targets. **Can each currently be built by entering its directory with a local build manifest and only a public SDK? No.** They use the root Makefile, shared spec resources, shared source lists/compiler flags and central volume recipes. **Can many be tested without booting MILTON? Yes, already.** These distinctions matter ([Makefile:10624](/home/tobiasosborne/Projects/InitechOS/Makefile:10624), [Makefile:10759](/home/tobiasosborne/Projects/InitechOS/Makefile:10759)).

|Component/app|Existing host seam / test story|Isolation assessment|
|---|---|---|
|SAMIR|`samir_pal_t` vtable: files, console, terminal, date, scratch and persistent allocation, EXEC/environment and optional services. `pal_host.c` supplies stdio/injected date/fixed arena; `pal_null.c` supplies contract stubs; `pal_milton.c` owns INT21 binding. Extensive DBF/DBT/NDX/interpreter/REPL corpus, properties/mutants, plus guest output/filesystem tests.|Strongest complete app boundary. Can run core/interpreter against a fake PAL today. Host environment and period reference corpus remain test dependencies ([os/samir/include/samir/pal.h:94](/home/tobiasosborne/Projects/InitechOS/os/samir/include/samir/pal.h:94), [os/samir/include/samir/pal.h:107](/home/tobiasosborne/Projects/InitechOS/os/samir/include/samir/pal.h:107), [os/samir/pal/pal_host.c:8](/home/tobiasosborne/Projects/InitechOS/os/samir/pal/pal_host.c:8), [Makefile:3066](/home/tobiasosborne/Projects/InitechOS/Makefile:3066), [Makefile:3486](/home/tobiasosborne/Projects/InitechOS/Makefile:3486)).|
|i123|Core/fs are a pure caller-memory library, codecs over byte buffers; no OS calls or allocator. Host tests link the real core; app routing/key/quit tests compile Face A with fake TBX functions. Differential WK1 reader/captures plus QEMU/Bochs GUI legs.|No SAMIR-shaped PAL needed for pure core. GUI face already has a stub seam; package it as SDK transport + model tests ([os/i123/core/i123.h:15](/home/tobiasosborne/Projects/InitechOS/os/i123/core/i123.h:15), [Makefile:15122](/home/tobiasosborne/Projects/InitechOS/Makefile:15122), [harness/proptest/test_i123_quit.c:53](/home/tobiasosborne/Projects/InitechOS/harness/proptest/test_i123_quit.c:53)).|
|iword|Pure document/piece/edit/layout/codec library, I/O owned by Face A through TBX. Host core/code/codec tests and actual Face A inclusion with mock Toolbox.|Same conclusion; already independently testable in substance ([os/iword/core/iword.h:44](/home/tobiasosborne/Projects/InitechOS/os/iword/core/iword.h:44), [Makefile:15353](/home/tobiasosborne/Projects/InitechOS/Makefile:15353), [harness/proptest/test_iword_face.c:77](/home/tobiasosborne/Projects/InitechOS/harness/proptest/test_iword_face.c:77)).|
|calc|Host harness includes actual app source, supplies fake TBX, links i123 library; independent emulator pixels/filesystem legs.|Host-testable now, but must package the shared worksheet library explicitly ([harness/frameapps.mk:12](/home/tobiasosborne/Projects/InitechOS/harness/frameapps.mk:12), [harness/proptest/test_calc_tenant.c:7](/home/tobiasosborne/Projects/InitechOS/harness/proptest/test_calc_tenant.c:7)).|
|calcda|Separate engine/view tests, 20,000-session reference property and screenshot tests; route harness includes real app.|Host-testable now ([Makefile:33700](/home/tobiasosborne/Projects/InitechOS/Makefile:33700), [harness/proptest/test_calcda_route.c:8](/home/tobiasosborne/Projects/InitechOS/harness/proptest/test_calcda_route.c:8)).|
|itext|Host route suite includes real app source over actual TextEdit and mocked gate/volume. Draw-operation and file expectations; guest file/pixel check independently.|Host-testable now; the seam should be a replaceable SDK transport rather than textual `.c` inclusion ([harness/proptest/test_itext_route.c:20](/home/tobiasosborne/Projects/InitechOS/harness/proptest/test_itext_route.c:20), [harness/proptest/test_itext_route.c:36](/home/tobiasosborne/Projects/InitechOS/harness/proptest/test_itext_route.c:36), [harness/proptest/test_itext_route.c:47](/home/tobiasosborne/Projects/InitechOS/harness/proptest/test_itext_route.c:47)).|
|notepad / scrapbook|Shared paged app route harness includes real source and supplies fake gate; variants, persistence, guest checks.|Host-testable now, shared implementation should be explicit in packaging ([harness/proptest/test_notepad_route.c:11](/home/tobiasosborne/Projects/InitechOS/harness/proptest/test_notepad_route.c:11), [Makefile:10668](/home/tobiasosborne/Projects/InitechOS/Makefile:10668)).|
|paint|Host route harness includes app, verifies menu/blank canvas; emulator screenshot leg.|Host-testable scaffold, not evidence of a finished paint core ([harness/proptest/test_paint_tenant.c:11](/home/tobiasosborne/Projects/InitechOS/harness/proptest/test_paint_tenant.c:11), [harness/frameapps.mk:289](/home/tobiasosborne/Projects/InitechOS/harness/frameapps.mk:289)).|
|cpanels|Host `test_cpanels` exercises core/real dialog machinery; actual preference application touches resident services. Guest dialog/bar/persistence checks.|Partial whole-app isolation: core is separable; resident preferences service needs a clean backend/fake ([harness/proptest/test_cpanels.c:1](/home/tobiasosborne/Projects/InitechOS/harness/proptest/test_cpanels.c:1), [os/flair/cp_service.h:8](/home/tobiasosborne/Projects/InitechOS/os/flair/cp_service.h:8)).|
|ctenant / fonts|Tiny gate/ABI specimen programs with explicit guest traces. Font manager itself has host style/raster tests.|I did not find a dedicated complete app-source host harness for these two. Easy to add fake TBX transport; currently guest specimens, not proof that every app has an isolated host runner ([Makefile:10644](/home/tobiasosborne/Projects/InitechOS/Makefile:10644), [Makefile:10680](/home/tobiasosborne/Projects/InitechOS/Makefile:10680), [harness/proptest/test_font_styles.c:1](/home/tobiasosborne/Projects/InitechOS/harness/proptest/test_font_styles.c:1)).|
|TPS / seed|TPS builds under FPC on host, graded against token/parser/type/codegen goldens; seed emits a COM that is run on MILTON. Seed has C lexer/parser tests and FPC differential corpus, self-hosting/DDC ladder targets.|Real independent host compiler paths already exist; preserve them. Absence of a finished IDE is a product gap, not OS coupling ([Makefile:118](/home/tobiasosborne/Projects/InitechOS/Makefile:118), [Makefile:17715](/home/tobiasosborne/Projects/InitechOS/Makefile:17715), [Makefile:18850](/home/tobiasosborne/Projects/InitechOS/Makefile:18850), [os/tps/tps.pas:16](/home/tobiasosborne/Projects/InitechOS/os/tps/tps.pas:16)).|
|SideKick-shaped TSR|Assembly fixture needs interrupt/idle/keyboard/screen semantics; guest test boots shell, installs TSR, runs foreground programs and injects chord.|Cannot run the actual interrupt binary as an ordinary host function. Can isolate a future popup model, but current useful verification is integration-level ([Makefile:35540](/home/tobiasosborne/Projects/InitechOS/Makefile:35540), [os/milton/tsr_program.asm:25](/home/tobiasosborne/Projects/InitechOS/os/milton/tsr_program.asm:25)).|
|COMMAND / externals|Host shell, batch, DOS message/filesystem/utilities tests and mocked backends; executable COM/INT21 entry paths need guests. Some host conditionals are inside command source.|Substantial host seams, incomplete uniform app SDK. Separate shell/utility library models from DOS entry/assembly wrappers ([os/milton/command.c:3067](/home/tobiasosborne/Projects/InitechOS/os/milton/command.c:3067), [harness/diff/fat_diff/shell_files.mk:1](/home/tobiasosborne/Projects/InitechOS/harness/diff/fat_diff/shell_files.mk:1), [harness/diff/fat_diff/dosutil.mk:1](/home/tobiasosborne/Projects/InitechOS/harness/diff/fat_diff/dosutil.mk:1)).|
|FLAIR itself|Real window/region/event/menu/control/dialog/desktop sources compile on host against a memory bitmap/render model. `test_shell` composes the scene; routing suite links actual tbxgate/process/window/event with stubs for kernel dependencies.|Already a host-executable graphics/toolbox model, not a second drawing implementation. Whole-device boot/interrupt behavior still needs guest tests. 64-bit host does not verify all i386 pointer-bearing ABI layouts ([Makefile:15743](/home/tobiasosborne/Projects/InitechOS/Makefile:15743), [harness/proptest/test_shell.c:63](/home/tobiasosborne/Projects/InitechOS/harness/proptest/test_shell.c:63), [Makefile:15069](/home/tobiasosborne/Projects/InitechOS/Makefile:15069), [os/flair/tbxgate.c:96](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:96)).|
|MILTON core / boot|FAT, MCB, PSP, SFT, loader, INT21 and DOS utilities have host tests via explicit backends/buffers. Boot/A20/GDT/ISR paths have QEMU/Bochs oracle guests.|Retain this split. Host pure libraries should not require a boot image; hardware transitions cannot be replaced by fake-OS tests alone ([os/milton/int21.c:410](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:410), [os/milton/int21.c:412](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:412), [harness/proptest/kernel_size.mk:48](/home/tobiasosborne/Projects/InitechOS/harness/proptest/kernel_size.mk:48)).|

A significant **test-system** cause of slow integration is explicit in the source: `make test` drives serial shell loops and ignores `-j`; `test-par` was added to present one parallel make dependency vector. SAMIR has a scoped per-merge vector and moves Bochs/typesetting to per-session work. So “all merges take forever” cannot be attributed solely to code structure without measuring actual selected gates and timings. No timing experiment was run here ([harness/merge_samir.mk:1](/home/tobiasosborne/Projects/InitechOS/harness/merge_samir.mk:1), [harness/merge_samir.mk:25](/home/tobiasosborne/Projects/InitechOS/harness/merge_samir.mk:25)).

## 6. Code quality sampling: what should survive

I read these **300-line contiguous samples** in addition to interface/build/boot reads: `int21.c:320–619`, `fat12.c:780–1079`, `window.c:500–799`, `event.c:100–399`, `samir/core/eval.c:180–479`, `i123/core/sheet.c:100–399`, and `tps.pas:1000–1299`. TPS is Pascal, not C. This is 2,100 physical lines sampled, not a security/correctness audit of the tree.

|Sample|Positive evidence worth retaining|Costs / deformation|
|---|---|---|
|INT21|Explicit file/EXEC/absolute-disk backends; copying supplied vtables avoids lifetime bugs. MCB allocation checks ownership/alignment/capacity, CWD context has save/restore. These are useful host-testable seams ([os/milton/int21.c:410](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:410), [os/milton/int21.c:417](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:417), [os/milton/int21.c:453](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:453), [os/milton/int21.c:547](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:547)).|6,311-line dispatcher has 103 conditional openings; mutable globals, nested EXEC and scoped callbacks create context coupling. Mutation-only entry at line 380; compressed newer routines contrast with expanded older style ([os/milton/int21.c:371](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:371), [os/milton/int21.c:498](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:498)). ANSI and native allocator implementations are textually included `.c` files ([os/milton/int21.c:42](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:42), [os/milton/int21.c:264](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:264)).|
|FAT|Bounded chain validation through EOC, reserved-marker/range rejection, bounded root traversal, explicit device error checks. Sensible algorithms and caller-owned scratch; not disposable code ([os/milton/fat12.c:799](/home/tobiasosborne/Projects/InitechOS/os/milton/fat12.c:799), [os/milton/fat12.c:835](/home/tobiasosborne/Projects/InitechOS/os/milton/fat12.c:835), [os/milton/fat12.c:974](/home/tobiasosborne/Projects/InitechOS/os/milton/fat12.c:974)).|4,968 lines, 76 conditional openings. A deliberately `noinline` common chain guard is a size lever with its own mutation gate; ordinary correctness code is surrounded by extensive oracle/history commentary ([os/milton/fat12.c:780](/home/tobiasosborne/Projects/InitechOS/os/milton/fat12.c:780), [harness/proptest/kernel_size_r7.mk:41](/home/tobiasosborne/Projects/InitechOS/harness/proptest/kernel_size_r7.mk:41)).|
|Window Manager|Clear exposure invariant, front-to-back ownership distribution with non-aliasing region scratch, explicit initialization and lifecycle state. Excellent candidate to keep as a library ([os/flair/window.c:500](/home/tobiasosborne/Projects/InitechOS/os/flair/window.c:500), [os/flair/window.c:525](/home/tobiasosborne/Projects/InitechOS/os/flair/window.c:525), [os/flair/window.c:566](/home/tobiasosborne/Projects/InitechOS/os/flair/window.c:566)).|Mutation branch replaces algorithm with deliberate overpaint; fixed geometry/style coupling and scratch-pool contracts. These constrain reuse but do not invalidate the algorithm ([os/flair/window.c:532](/home/tobiasosborne/Projects/InitechOS/os/flair/window.c:532), [os/flair/window.c:601](/home/tobiasosborne/Projects/InitechOS/os/flair/window.c:601)).|
|Event Manager|Producer/consumer ownership documented; unsigned occupancy handles wrap; compiler barriers order ordinary slot data vs volatile publication. Proper awareness that volatile alone does not order all memory accesses under optimization ([os/flair/event.c:188](/home/tobiasosborne/Projects/InitechOS/os/flair/event.c:188), [os/flair/event.c:213](/home/tobiasosborne/Projects/InitechOS/os/flair/event.c:213)).|Global keyboard/cursor/preferences state, recording-only double-click initialization, mutation-oriented unused attributes. 732 lines/13 conditionals, still comprehensible ([os/flair/event.c:159](/home/tobiasosborne/Projects/InitechOS/os/flair/event.c:159), [os/flair/event.c:253](/home/tobiasosborne/Projects/InitechOS/os/flair/event.c:253), [os/flair/event.c:359](/home/tobiasosborne/Projects/InitechOS/os/flair/event.c:359)).|
|SAMIR evaluator|Small helpers encode exact string/date/coercion behavior; allocation failure is explicit; boundary between semantics and OS remains clean. Historical oddities such as blank-date comparisons are documented compatibility rules, not arbitrary bad design ([os/samir/core/eval.c:197](/home/tobiasosborne/Projects/InitechOS/os/samir/core/eval.c:197), [os/samir/core/eval.c:247](/home/tobiasosborne/Projects/InitechOS/os/samir/core/eval.c:247), [os/samir/core/eval.c:344](/home/tobiasosborne/Projects/InitechOS/os/samir/core/eval.c:344)).|Mutation guards are still in production source, but only 7 conditional openings in this 897-line file. Do not “simplify” peculiar xBase rules without preserving oracle behavior ([os/samir/core/eval.c:189](/home/tobiasosborne/Projects/InitechOS/os/samir/core/eval.c:189), [os/samir/core/eval.c:401](/home/tobiasosborne/Projects/InitechOS/os/samir/core/eval.c:401)).|
|i123 store|Caller-memory initialization, sparse lookup, explicit pool management, preserving byte-level numeric records, non-destructive refusal before replacing cells. Readable library ([os/i123/core/sheet.c:145](/home/tobiasosborne/Projects/InitechOS/os/i123/core/sheet.c:145), [os/i123/core/sheet.c:218](/home/tobiasosborne/Projects/InitechOS/os/i123/core/sheet.c:218), [os/i123/core/sheet.c:243](/home/tobiasosborne/Projects/InitechOS/os/i123/core/sheet.c:243)).|O(n²) scratch-free compaction is a deliberate bounded-resource tradeoff, not a rewrite trigger. A private codec entry is exported by ad-hoc extern rather than private header. Multiplication in initialization relies on the intended bounded cell-cap contract; this sample is not a proof against arbitrary hostile callers ([os/i123/core/sheet.c:154](/home/tobiasosborne/Projects/InitechOS/os/i123/core/sheet.c:154), [os/i123/core/sheet.c:174](/home/tobiasosborne/Projects/InitechOS/os/i123/core/sheet.c:174), [os/i123/core/sheet.c:232](/home/tobiasosborne/Projects/InitechOS/os/i123/core/sheet.c:232)).|
|TPS|Conventional monotonic lexer and explicit located errors; easily compiled under host FPC. Rolling two-ShortString input window is documented and avoids loading all source in BSS ([os/tps/tps.pas:9](/home/tobiasosborne/Projects/InitechOS/os/tps/tps.pas:9), [os/tps/tps.pas:1020](/home/tobiasosborne/Projects/InitechOS/os/tps/tps.pas:1020), [os/tps/tps.pas:1185](/home/tobiasosborne/Projects/InitechOS/os/tps/tps.pas:1185)).|Large single-file bootstrap; long repeated token-name chains; fixed `TPSIN.PAS` and multi-pass dump protocol constrain CLI usability. These are bootstrap/subset constraints, not evidence for rewriting semantics ([os/tps/tps.pas:1101](/home/tobiasosborne/Projects/InitechOS/os/tps/tps.pas:1101), [os/tps/tps.pas:1233](/home/tobiasosborne/Projects/InitechOS/os/tps/tps.pas:1233)).|

### Concrete system-level constraint deformation

1. **Optimization becomes an image-wide ABI design.** Kernel `-mregparm=3 -mrtd -freg-struct-return` and four-byte stack conventions must agree across every TU, assembly entry and LTO link. Round 7 has a gate that audits every printed compile/link command for struct-return ABI agreement. Size rounds turn a routine implementation change into a whole-image convention verification problem ([Makefile:738](/home/tobiasosborne/Projects/InitechOS/Makefile:738), [Makefile:838](/home/tobiasosborne/Projects/InitechOS/Makefile:838), [harness/proptest/kernel_size_r7.mk:4](/home/tobiasosborne/Projects/InitechOS/harness/proptest/kernel_size_r7.mk:4)).
2. **Long compiler-pass exception lists.** Cold kernel code uses `-Oz`, LTO, frame-pointer omission, changed cross-jump heuristics, no jump tables, aliasing suppression and ABI data alignment. i123 has its own large measured list of disabled optimization passes plus different register/callee-pop ABI purely to fit 64 KiB. These choices can be defensible; continually tuning them to land features is still architecture-induced maintenance cost ([Makefile:802](/home/tobiasosborne/Projects/InitechOS/Makefile:802), [Makefile:809](/home/tobiasosborne/Projects/InitechOS/Makefile:809), [Makefile:10581](/home/tobiasosborne/Projects/InitechOS/Makefile:10581), [Makefile:10618](/home/tobiasosborne/Projects/InitechOS/Makefile:10618)).
3. **Hot/cold source splits are packaging constraints masquerading as modules.** `finder_ops_cold.c` shares a private struct/callback with resident code; resident callback addresses must stay valid and every call crosses generated overlay stubs. Host builds textually include the same cold source. This is not a separate conceptual Finder service ([os/flair/finder_ops_cold.c:1](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_ops_cold.c:1), [os/flair/finder_ops_private.h:1](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_ops_private.h:1), [os/flair/finder_ops.c:191](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_ops.c:191)).
4. **LTO import roots impose a boundary tax.** Imports force resident symbols global/alive/less-specialized; moves that seem to remove N bytes may save much less. ADR-0014 explicitly lists this as a high risk and requires measuring every lane ([docs/adr/ADR-0014-Kernel-Overlays.md:308](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0014-Kernel-Overlays.md:308), [Makefile:10959](/home/tobiasosborne/Projects/InitechOS/Makefile:10959)).
5. **Writable app buffers consume on-disk file space to satisfy pointer validation.** `TBX_IN_IMAGE` forces data into `.data`; the binding explicitly warns that BSS is not accepted for several gate reads. That couples a harmless storage choice to file-size budget and pointer lifetime rules ([os/apps/tbx/tbx.h:31](/home/tobiasosborne/Projects/InitechOS/os/apps/tbx/tbx.h:31), [os/apps/tbx/tbx.h:49](/home/tobiasosborne/Projects/InitechOS/os/apps/tbx/tbx.h:49)). A clean SDK should validate against declared readable/writable regions instead of requiring all externally readable buffers in loaded PROGBITS.
6. **Evidence/history prose dominates some core files.** It explains hard-won regressions, but much belongs in tests/design records rather than every production TU. Combined with dozens of deliberately wrong `#ifdef` paths it raises reading/review cost without implying the default algorithm is poor ([os/milton/int21.c:338](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:338), [os/flair/window.c:515](/home/tobiasosborne/Projects/InitechOS/os/flair/window.c:515)).

**Estimate (inference, not measured quality): about 65–80% of non-test `os/` source is healthy core that could be carried into the new structure with its algorithms/function bodies largely verbatim.** This is a broad sampling-based range, not a LOC-certified salvage ratio. A stricter demand of *byte-identical files* would yield much less: includes, platform wrappers, misplaced SDK types and test hooks need edits even when the algorithms should stay. Roughly 20–35% needs interface/glue/entry/test-packaging work; that does not mean all of it needs semantic rewriting. Most proposed rewriting is concentrated in the small loader transfer path and the integration scaffolding, not the 32K-line database or 30K-line graphics tree.

## 7. Source and test size

### Component inventory

The first two columns are **measured non-test-path source** using the stated extensions and exclusions. Tests are excluded by `test_*` filename or `tests/fixtures/examples` ancestry. Embedded mutation branches remain in the non-test source count. Demo/specimen files not named as tests (e.g. TENANTFX, CTENANT, SideKick-shaped TSR) also remain there; this is an auditable mechanical inventory, not a claim they are production features. Host platform bindings under `os/samir/pal` count as implementation/support, not tests.

The associated test columns count **unique tracked test/fixture source files explicitly present alongside a component's implementation/header in expanded make prerequisites, plus colocated tests**. This makes cross-component test ownership explicit but is a **lower bound** on total verification code: orchestration scripts, indirect prerequisite-only associations and fixtures with non-test names are omitted. Multi-component tests are counted for each component they explicitly exercise, so that column is intentionally non-additive. “0” for boot means no colocated/directly-associated test-source file under this method, not no boot tests. Boot integration is mostly orchestration/guest checks. All file ownership lists and the script are provided for reproduction.

|Component|Non-test physical LOC|Non-test code LOC|Colocated test/fixture physical LOC|Associated test/fixture physical LOC|Associated files|
|---|---:|---:|---:|---:|---:|
|os/boot|1,807|1,276|0|0|0|
|os/milton|55,241|34,042|21,999|36,146|120|
|os/flair|29,601|15,871|0|35,391|98|
|os/apps|4,448|3,181|0|7,155|26|
|os/samir|32,424|18,409|0|31,804|79|
|os/i123|7,802|6,390|0|4,880|18|
|os/iword|6,315|4,710|0|6,679|14|
|os/tps|5,033|4,787|1,871|1,871|37|
|seed|10,167|6,007|2,883|3,415|24|

Non-test-path `os/` totals **142,671 physical / 88,666 code LOC**; colocated test/fixture source totals 23,870 physical lines. Seed is additional. Shared tracked `spec` source/header/definition tables total 15,383 physical lines; central `harness` source/header/assembly totals **109,633 physical lines**. These independent totals avoid double-counting the associated-tests column. Many additional shell/Python/make and data/golden lines are deliberately outside the source metric; **do not interpret the associated column as a full exclusive accounting of all tests**. This is a limitation of the requested per-component attribution in a central multi-component harness, not missing test execution evidence.

Useful measured sub-boundaries:

|Component|Physical LOC breakdown|
|---|---|
|apps|calc 312; calcda 894; cpanels 198; ctenant 208; fonts 101; itext 726; notepad 427; paint 109; TBX SDK/runtime/save headers 628; remaining top-level reference/specimen files 845|
|i123|app 1,412; core 5,935; fs 455|
|iword|app 2,793; core 3,400; fs 122|
|SAMIR|boot 710; core 6,377; cmd 11,683; fs 5,160; include 5,494; PAL 2,371; main 629|
|central harness|dbf_diff 34,788; fat_diff 12,111; wk1_diff 2,730; wp5_diff 1,675; proptest 42,746; emu 6,348; render 1,037; pubs 6,069; other helpers/fixtures 2,129|

The pure model/codec boundary is quite substantial: i123's 6,390 physical core+fs lines and iword's 3,522 are already separated from their GUI faces. Their healthy structure is concrete, not a speculative refactoring aspiration ([os/i123/core/i123.h:15](/home/tobiasosborne/Projects/InitechOS/os/i123/core/i123.h:15), [os/iword/core/iword.h:44](/home/tobiasosborne/Projects/InitechOS/os/iword/core/iword.h:44)).

## 8. Architectural assessment and target

### The five worst non-local couplings, ranked

1. **Resident OS/Toolbox/Finder composition.** MILTON owns the desktop interaction/presentation policy in kmain; FLAIR's gate/file/control-panel services include MILTON internals; reference apps are resident link inputs. A Finder or GUI feature changes the same image as disk/loader/interrupt services. Measured include edges are MILTON→FLAIR 39, FLAIR→MILTON 7, MILTON→apps 2. The problem is not that every disk app is statically linked; the platform and desktop are ([os/milton/kmain.c:68](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:68), [os/milton/kmain.c:3152](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:3152), [os/flair/tbxgate.c:84](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:84), [os/flair/tbxfile.h:8](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxfile.h:8), [Makefile:10828](/home/tobiasosborne/Projects/InitechOS/Makefile:10828)).
2. **Whole-image memory/load policy controls unrelated feature work.** A disk/console/system service or menu change consumes the same 169,984-byte policy budget; 384 sectors is repeated in bootstrap, IO.SYS, entry and make. Apps have their own 64 KiB disk cap and memory-region rules, causing another optimization program. The kernel high move never removed whole-image staging. This is the direct cause of recurrent “size rounds,” distinct from actual 486 capacity ([Makefile:240](/home/tobiasosborne/Projects/InitechOS/Makefile:240), [os/boot/stage2.asm:206](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:206), [os/milton/kstart.asm:47](/home/tobiasosborne/Projects/InitechOS/os/milton/kstart.asm:47), [harness/proptest/overlay.mk:214](/home/tobiasosborne/Projects/InitechOS/harness/proptest/overlay.mk:214), [Makefile:10641](/home/tobiasosborne/Projects/InitechOS/Makefile:10641)).
3. **Exact-build overlay imports bind every variant to new disk payloads.** An address change in a resident symbol can require new FLAIR.OVL and COMMAND.COM, regenerated import/stub/keep tables and patched INITDOS.SYS. The dependency graph even consults the resident object set to determine which symbols overlays need; it is not a stable export library. LTO/ABI changes propagate across the whole image. Overlays solve a constrained boot file, not app modularity ([Makefile:10984](/home/tobiasosborne/Projects/InitechOS/Makefile:10984), [Makefile:11023](/home/tobiasosborne/Projects/InitechOS/Makefile:11023), [os/milton/transient.h:26](/home/tobiasosborne/Projects/InitechOS/os/milton/transient.h:26), [docs/adr/ADR-0014-Kernel-Overlays.md:325](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0014-Kernel-Overlays.md:325)).
4. **Global build invalidation and packaging.** Every target has `.EXTRA_PREREQS` on a Makefile stamp; that stamp depends on *all* tracked `.def` inputs across OS/seed/spec/harness/tools/pubs. Editing a SAMIR verb definition can therefore invalidate unrelated products. The generic app recipe also recompiles the entire app source set inside one ELF target rather than exposing normal per-source targets. The root data-volume recipe packages many unrelated apps at once. These are direct, evidence-backed reasons an “independent” source change does broad work ([Makefile:62](/home/tobiasosborne/Projects/InitechOS/Makefile:62), [Makefile:73](/home/tobiasosborne/Projects/InitechOS/Makefile:73), [Makefile:80](/home/tobiasosborne/Projects/InitechOS/Makefile:80), [Makefile:10624](/home/tobiasosborne/Projects/InitechOS/Makefile:10624), [Makefile:10759](/home/tobiasosborne/Projects/InitechOS/Makefile:10759)).
5. **Tests, personalities and recording policy share the product entry file/build matrix.** Diagnostic guests and deliberate wrong branches are compiled from the same large production TUs; size gates enumerate 352 kernel targets. `make test` serializes gate loops; full GUI/Bochs/corpus checks have broad prerequisites. Existing comments document actual parallel races from nested producers and image rewrites. This combines test coupling with source coupling and repeated integration obligations ([os/milton/kmain.c:5746](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:5746), [os/milton/kmain.c:7269](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:7269), [Makefile:455](/home/tobiasosborne/Projects/InitechOS/Makefile:455), [harness/proptest/overlay.mk:227](/home/tobiasosborne/Projects/InitechOS/harness/proptest/overlay.mk:227), [harness/merge_samir.mk:25](/home/tobiasosborne/Projects/InitechOS/harness/merge_samir.mk:25), [tools/mk_boot_system.sh:23](/home/tobiasosborne/Projects/InitechOS/tools/mk_boot_system.sh:23)).

Additional limitations that should shape the target are the **single disk slot**, shared ENV/PSP/global DOS context, raw pointer-bearing menu graphs and ring-0 cooperative callbacks. They constrain the owner's expectation that arbitrary apps are fully independent at runtime. They do not require abandoning the agreed cooperative 486 product model ([docs/adr/ADR-0013-AMENDMENT-AC-3-Toolbox-Gate-and-Disk-Tenants.md:66](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0013-AMENDMENT-AC-3-Toolbox-Gate-and-Disk-Tenants.md:66), [os/flair/tbxgate.c:24](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:24)).

### Clean target architecture (recommendation)

Use a small set of explicit build/load boundaries:

- **Boot:** hardware discovery + checked FAT file loading + chunked high-memory copy; versioned boot handoff with loaded/BSS extents. One generated memory contract. No image-size dependency on a low buffer's total capacity.
- **MILTON kernel:** hardware/interrupt services, block/filesystem, DOS-shaped native process and memory model, console and stable system services. Host libraries for FAT/loader/INT21 semantics; hardware adapters and assembly entry isolated. Production boot entry calls initialization services and transfers to the selected shell/desktop.
- **FLAIR runtime/toolbox:** surface/region/window/menu/control/dialog/text/event/process/compositor managers as one cohesive library/runtime with explicit platform backend interfaces for files, clock, input, memory, console handoff and display. Initially it can stay linked into one platform image behind these internal seams; a subsequent independently loaded FLAIR module removes the boot-size budget from its growth. A microkernel or paging rewrite is unnecessary.
- **Desktop/Finder:** owns gesture tracking, menu policies, app lifecycle, Finder windows/operations, About/DA/system actions. Move its production event pump and adapters out of `kmain.c`; a built-in privileged desktop entry is an acceptable intermediate boundary. Expose required platform operations through a named service interface, not arbitrary resident ELF imports.
- **SDK:** move `apps/tbx` into `sdk/`; publish the existing numbered traps in a small versioned header/library package. Add major ABI version/required capabilities/record lengths; preserve compatible numbers, specify pointer/buffer ownership and readable/writeable ranges, avoid exposing mutable internal records as the public contract. Supply a real i386 trap transport and a host fake transport behind identical API signatures. Pure TextEdit/font/model libraries should live in explicit reusable packages, not be borrowed opportunistically from the OS implementation directory.
- **Apps:** each directory owns its manifest/source/model/platform binding/host tests/guest smoke. Extract DOS shell and externals from `os/milton`, and i123's reusable worksheet pieces into an explicit library used by calc. Keep SAMIR's PAL, pure i123/iword cores, TextEdit library, calculator core, and TPS/seed as distinct products. The repository root should orchestrate targets, not encode every app's compiler flags/source inventory.
- **Verification:** normal per-source dependency files/flag signatures, component-specific schema inputs, isolated output directories, and focused host gates first. Keep independent period goldens and real emulator/Bochs checks for ABI/driver/boot changes. Run the broad system matrix on integration boundaries or scheduled/full verification, rather than require all kernel variants for every pure app-core change. Preserve semantic mutation testing where it adds assurance; move test entry scenarios to dedicated TUs and consider generated patches/link seams for deliberately wrong behavior, without demanding a wholesale test rewrite.

**The critical sequencing is interface extraction plus evidence preservation.** First remove global invalidation and give apps independent manifests/host runners; extract the desktop from kmain without changing behavior; implement chunked loading and deliberately replace the old size policy; then reduce address-specific overlays/imports and add SDK version negotiation. These are migration stages, not authorization to edit this checkout. Simply moving files into new directories while retaining global stamp/overlay imports/whole-image tests will not fix merge latency.

### Disposition of existing components

“KEEP AS-IS” below means preserve the core algorithm/library behavior and body code; routine new manifests/includes are still allowed. “REWRITE” is narrowly scoped, not a demand to throw away all files in that directory.

|Existing component|Disposition|Reason / boundary|
|---|---|---|
|MBR|**KEEP AS-IS** initially|Small fixed-stage loader; current checked disk contract can survive the kernel transfer change ([os/boot/mbr.asm:66](/home/tobiasosborne/Projects/InitechOS/os/boot/mbr.asm:66)).|
|stage2 FAT parser|**KEEP BUT RE-HOUSE**|Keep bounded FAT validation/boot-file contract; separate file reader from destination copy sink ([os/boot/stage2.asm:160](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:160), [os/boot/stage2.asm:278](/home/tobiasosborne/Projects/InitechOS/os/boot/stage2.asm:278)).|
|IO.SYS VBE/A20/GDT/hardware handoff|**KEEP BUT RE-HOUSE**|Known hardware transition sequence, preserve/regrade; isolate from payload sizing ([os/boot/io_sys.asm:22](/home/tobiasosborne/Projects/InitechOS/os/boot/io_sys.asm:22)).|
|Whole-kernel bounce transfer and size-derived zeroing|**REWRITE**|Replace fixed whole-file bounce/copy/zero-count protocol with checked streaming extents and explicit BSS initialization ([os/boot/io_sys.asm:1002](/home/tobiasosborne/Projects/InitechOS/os/boot/io_sys.asm:1002), [os/milton/kstart.asm:92](/home/tobiasosborne/Projects/InitechOS/os/milton/kstart.asm:92)).|
|MILTON FAT/MCB/PSP/SFT/block-device/loader semantics|**KEEP BUT RE-HOUSE**|Readable, tested native libraries; isolate hardware/globals/entry adapters, preserve file and DOS behavior ([os/milton/fat12.c:799](/home/tobiasosborne/Projects/InitechOS/os/milton/fat12.c:799), [os/milton/int21.c:547](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:547)).|
|INT21 implementation|**KEEP BUT RE-HOUSE**|Retain handlers and validation; split dispatch/process-state/backend responsibilities; remove textual implementation aggregation as part of packaging ([os/milton/int21.c:42](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:42), [os/milton/int21.c:410](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:410)).|
|kmain production glue|**KEEP BUT RE-HOUSE**, selectively **REWRITE orchestration**|Desktop production functions should move mostly intact; replace configuration mega-entry with short explicit boot and desktop entrypoints. It is not mostly disposable tests ([os/milton/kmain.c:5937](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:5937), [os/milton/kmain.c:6755](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:6755)).|
|FLAIR region/surface/window/event/blitter/text managers|**KEEP AS-IS** core; **KEEP BUT RE-HOUSE** runtime/backend|Good independently host-exercised algorithms. Separate platform service edges; do not reimplement region damage ([os/flair/window.c:525](/home/tobiasosborne/Projects/InitechOS/os/flair/window.c:525), [os/flair/event.c:188](/home/tobiasosborne/Projects/InitechOS/os/flair/event.c:188), [Makefile:15743](/home/tobiasosborne/Projects/InitechOS/Makefile:15743)).|
|Finder and FLAIR process/gate/services|**KEEP BUT RE-HOUSE**|Keep event/gesture/teardown semantics, establish desktop/runtime/kernel service boundaries; version external data ABI ([os/flair/tbxgate.c:13](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:13), [os/flair/finder_ops_private.h:1](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_ops_private.h:1)).|
|FLAIR.OVL machinery / cold splits|**KEEP BUT RE-HOUSE temporarily; retire/REWRITE packaging**|Useful migration compatibility; exact-build symbol imports should cease to be the fundamental module contract. Do not keep adding cold splits merely to maintain the old boot cap ([Makefile:11023](/home/tobiasosborne/Projects/InitechOS/Makefile:11023)).|
|TBX SDK/runtime|**KEEP BUT RE-HOUSE**, extend protocol|Existing numbered ABI and stubs are valuable. Add version/capabilities and host transport; revise buffer-region validation ([os/apps/tbx/tbx.h:31](/home/tobiasosborne/Projects/InitechOS/os/apps/tbx/tbx.h:31), [os/apps/tbx/tbx_rt.c:74](/home/tobiasosborne/Projects/InitechOS/os/apps/tbx/tbx_rt.c:74)).|
|calc|**KEEP BUT RE-HOUSE**|Already disk app + fake-TBX tests; formalize worksheet library dependency ([harness/frameapps.mk:3](/home/tobiasosborne/Projects/InitechOS/harness/frameapps.mk:3)).|
|calcda|**KEEP AS-IS** core/view; **KEEP BUT RE-HOUSE** app|Clean local model/view/adapter split and strong host oracle ([Makefile:10693](/home/tobiasosborne/Projects/InitechOS/Makefile:10693), [Makefile:33700](/home/tobiasosborne/Projects/InitechOS/Makefile:33700)).|
|cpanels|**KEEP BUT RE-HOUSE**|Separate preferences/dialog service backend from the tenant controller ([os/apps/cpanels/cpanels.c:5](/home/tobiasosborne/Projects/InitechOS/os/apps/cpanels/cpanels.c:5), [os/flair/cp_service.h:8](/home/tobiasosborne/Projects/InitechOS/os/flair/cp_service.h:8)).|
|ctenant / fonts|**KEEP AS-IS** as SDK/ABI specimens|Useful test/examples; do not treat these as product architecture demanding full rewrite ([Makefile:10644](/home/tobiasosborne/Projects/InitechOS/Makefile:10644), [Makefile:10680](/home/tobiasosborne/Projects/InitechOS/Makefile:10680)).|
|itext|**KEEP BUT RE-HOUSE**|Reuse TextEdit, make it a named shared package and use host SDK binding ([Makefile:10657](/home/tobiasosborne/Projects/InitechOS/Makefile:10657)).|
|notepad / scrapbook|**KEEP BUT RE-HOUSE**|Retain common paged source and storage rules; explicit app variants/manifests ([Makefile:10668](/home/tobiasosborne/Projects/InitechOS/Makefile:10668)).|
|paint|**KEEP AS-IS** scaffold; new core when product scope requires|Current 109-line app is not a complete paint engine; architecture alone does not create missing features ([harness/frameapps.mk:289](/home/tobiasosborne/Projects/InitechOS/harness/frameapps.mk:289)).|
|i123|**KEEP AS-IS** core/fs; **KEEP BUT RE-HOUSE** face/build|Pure model with existing host and independent WK1 evidence; remove artificial executable-size optimization pressure separately ([os/i123/core/i123.h:15](/home/tobiasosborne/Projects/InitechOS/os/i123/core/i123.h:15), [Makefile:15122](/home/tobiasosborne/Projects/InitechOS/Makefile:15122)).|
|iword|**KEEP AS-IS** core/fs; **KEEP BUT RE-HOUSE** face/build|Piece-table/codec logic is already independent and host tested ([os/iword/core/iword.h:44](/home/tobiasosborne/Projects/InitechOS/os/iword/core/iword.h:44), [Makefile:15353](/home/tobiasosborne/Projects/InitechOS/Makefile:15353)).|
|SAMIR|**KEEP AS-IS** engine/PAL contract; **KEEP BUT RE-HOUSE** packaging/adapters|Strongest module boundary; preserve DBF/DBT/NDX/interpreter compatibility and corpus. Simplify historical memory/overlay packing only under its extended-memory contract ([os/samir/include/samir/pal.h:5](/home/tobiasosborne/Projects/InitechOS/os/samir/include/samir/pal.h:5), [Makefile:28967](/home/tobiasosborne/Projects/InitechOS/Makefile:28967)).|
|TPS / Turbo Initech|**KEEP BUT RE-HOUSE / evolve**|Preserve compiler behavior, test protocols and FPC path; split bootstrap phases as language support permits. A future IDE is new app work, not replacement of this compiler ([os/tps/tps.pas:3](/home/tobiasosborne/Projects/InitechOS/os/tps/tps.pas:3), [os/tps/tps.pas:21](/home/tobiasosborne/Projects/InitechOS/os/tps/tps.pas:21)).|
|seed compiler|**KEEP AS-IS** core|Already host-native, separate library/driver/test source lists ([Makefile:93](/home/tobiasosborne/Projects/InitechOS/Makefile:93)).|
|SideKick-shaped TSR|**KEEP AS-IS** integration fixture|Tests real residency/idle/IRQ behavior; not an existing full app to rehouse or rewrite ([os/milton/tsr_program.asm:1](/home/tobiasosborne/Projects/InitechOS/os/milton/tsr_program.asm:1)).|
|COMMAND / DOS externals|**KEEP BUT RE-HOUSE**; **REWRITE exact-address shell packaging** later|Keep DOS command/utility implementations and host tests; explicit user-program SDK, standard loadable shell interface rather than per-kernel symbol-address transient ([os/milton/transient.h:33](/home/tobiasosborne/Projects/InitechOS/os/milton/transient.h:33), [Makefile:10736](/home/tobiasosborne/Projects/InitechOS/Makefile:10736)).|
|Root factory/test build|**REWRITE orchestration incrementally**|Replace global stamp/schema invalidation and giant per-product recipes with composable manifests/per-source deps; retain goldens/gates and a root integration entry ([Makefile:62](/home/tobiasosborne/Projects/InitechOS/Makefile:62), [Makefile:10624](/home/tobiasosborne/Projects/InitechOS/Makefile:10624)).|

**Decision:** a ground-up rewrite is unsupported by this evidence. The existing app cores and host harnesses are the best starting point for the clean target. The restructuring must change the boot/load contract, runtime service boundaries, and build/test dependency graph; these are the mechanisms making otherwise independent app work non-local.

## Appendix A. Exact flagship link inventory and observed object sizes

Native text/data/BSS are decimal bytes reported by `size` on existing build artifacts. `IR` marks objects whose native sections report zero: their final resident contribution is decided by LTO, not zero. `GC` can discard sections of any object. The first 60 entries are the expanded `KERNEL_FLAIRTENANTS_OBJS`; the final two are response-file insertions ([Makefile:10828](/home/tobiasosborne/Projects/InitechOS/Makefile:10828), [Makefile:10902](/home/tobiasosborne/Projects/InitechOS/Makefile:10902), [Makefile:10973](/home/tobiasosborne/Projects/InitechOS/Makefile:10973)).

|Object|Text|Data|BSS|Caveat|
|---|---:|---:|---:|---|
|[build/kstart.o](/home/tobiasosborne/Projects/InitechOS/build/kstart.o)|31|0|0|native / subject to GC|
|[build/console.o](/home/tobiasosborne/Projects/InitechOS/build/console.o)|0|0|0|IR|
|[build/kmain_cold.o](/home/tobiasosborne/Projects/InitechOS/build/kmain_cold.o)|0|0|0|IR|
|[build/surface.o](/home/tobiasosborne/Projects/InitechOS/build/surface.o)|0|0|0|IR|
|[build/idt.o](/home/tobiasosborne/Projects/InitechOS/build/idt.o)|398|0|2,054|native / subject to GC|
|[build/pic.o](/home/tobiasosborne/Projects/InitechOS/build/pic.o)|100|0|0|native / subject to GC|
|[build/panic.o](/home/tobiasosborne/Projects/InitechOS/build/panic.o)|0|0|0|IR|
|[build/int21.o](/home/tobiasosborne/Projects/InitechOS/build/int21.o)|0|0|0|IR|
|[build/devices.o](/home/tobiasosborne/Projects/InitechOS/build/devices.o)|0|0|0|IR|
|[build/mcb.o](/home/tobiasosborne/Projects/InitechOS/build/mcb.o)|1,292|0|0|native / subject to GC|
|[build/psp.o](/home/tobiasosborne/Projects/InitechOS/build/psp.o)|0|0|0|IR|
|[build/sft.o](/home/tobiasosborne/Projects/InitechOS/build/sft.o)|0|0|0|IR|
|[build/config_sys.o](/home/tobiasosborne/Projects/InitechOS/build/config_sys.o)|0|0|0|IR|
|[build/sysinit.o](/home/tobiasosborne/Projects/InitechOS/build/sysinit.o)|0|0|0|IR|
|[build/loader_tenant.o](/home/tobiasosborne/Projects/InitechOS/build/loader_tenant.o)|0|0|0|IR|
|[build/ata.o](/home/tobiasosborne/Projects/InitechOS/build/ata.o)|1,254|0|5|native / subject to GC|
|[build/block_device.o](/home/tobiasosborne/Projects/InitechOS/build/block_device.o)|0|0|0|IR|
|[build/fat12.o](/home/tobiasosborne/Projects/InitechOS/build/fat12.o)|0|0|0|IR|
|[build/fileio_fat.o](/home/tobiasosborne/Projects/InitechOS/build/fileio_fat.o)|0|0|0|IR|
|[build/kbd.o](/home/tobiasosborne/Projects/InitechOS/build/kbd.o)|1,201|0|52|native / subject to GC|
|[build/mouse.o](/home/tobiasosborne/Projects/InitechOS/build/mouse.o)|436|0|12|native / subject to GC|
|[build/pit.o](/home/tobiasosborne/Projects/InitechOS/build/pit.o)|101|0|8|native / subject to GC|
|[build/rtc.o](/home/tobiasosborne/Projects/InitechOS/build/rtc.o)|1,378|0|0|native / subject to GC|
|[build/irq.o](/home/tobiasosborne/Projects/InitechOS/build/irq.o)|178|0|4|native / subject to GC|
|[build/test_prog_blob.o](/home/tobiasosborne/Projects/InitechOS/build/test_prog_blob.o)|53|0|0|native / subject to GC|
|[build/type_prog_blob.o](/home/tobiasosborne/Projects/InitechOS/build/type_prog_blob.o)|193|0|0|native / subject to GC|
|[build/dir_prog_blob.o](/home/tobiasosborne/Projects/InitechOS/build/dir_prog_blob.o)|159|0|0|native / subject to GC|
|[build/isr.o](/home/tobiasosborne/Projects/InitechOS/build/isr.o)|848|0|0|native / subject to GC|
|[build/region.o](/home/tobiasosborne/Projects/InitechOS/build/region.o)|3,801|0|9|native / subject to GC|
|[build/heap.o](/home/tobiasosborne/Projects/InitechOS/build/heap.o)|0|0|0|IR|
|[build/event.o](/home/tobiasosborne/Projects/InitechOS/build/event.o)|1,684|8|21|native / subject to GC|
|[build/cursor.o](/home/tobiasosborne/Projects/InitechOS/build/cursor.o)|0|0|0|IR|
|[build/window_live.o](/home/tobiasosborne/Projects/InitechOS/build/window_live.o)|0|0|0|IR|
|[build/blitter.o](/home/tobiasosborne/Projects/InitechOS/build/blitter.o)|757|0|0|native / subject to GC|
|[build/chrome.o](/home/tobiasosborne/Projects/InitechOS/build/chrome.o)|0|0|0|IR|
|[build/text.o](/home/tobiasosborne/Projects/InitechOS/build/text.o)|11,651|0|0|native / subject to GC|
|[build/menu.o](/home/tobiasosborne/Projects/InitechOS/build/menu.o)|0|0|0|IR|
|[build/control.o](/home/tobiasosborne/Projects/InitechOS/build/control.o)|0|0|0|IR|
|[build/dialog.o](/home/tobiasosborne/Projects/InitechOS/build/dialog.o)|0|0|0|IR|
|[build/desktop.o](/home/tobiasosborne/Projects/InitechOS/build/desktop.o)|0|0|0|IR|
|[build/flair_look.o](/home/tobiasosborne/Projects/InitechOS/build/flair_look.o)|0|0|0|IR|
|[build/flair_shell.o](/home/tobiasosborne/Projects/InitechOS/build/flair_shell.o)|0|0|0|IR|
|[build/kmain_flairtenants.o](/home/tobiasosborne/Projects/InitechOS/build/kmain_flairtenants.o)|0|0|0|IR|
|[build/process.o](/home/tobiasosborne/Projects/InitechOS/build/process.o)|0|0|0|IR|
|[build/ref_tenant.o](/home/tobiasosborne/Projects/InitechOS/build/ref_tenant.o)|0|0|0|IR|
|[build/desktop_db.o](/home/tobiasosborne/Projects/InitechOS/build/desktop_db.o)|0|0|0|IR|
|[build/finder_icon.o](/home/tobiasosborne/Projects/InitechOS/build/finder_icon.o)|0|0|0|IR|
|[build/finder_desktop.o](/home/tobiasosborne/Projects/InitechOS/build/finder_desktop.o)|0|0|0|IR|
|[build/finder_windows.o](/home/tobiasosborne/Projects/InitechOS/build/finder_windows.o)|0|0|0|IR|
|[build/finder_ops.o](/home/tobiasosborne/Projects/InitechOS/build/finder_ops.o)|0|0|0|IR|
|[build/finder_cmd.o](/home/tobiasosborne/Projects/InitechOS/build/finder_cmd.o)|0|0|0|IR|
|[build/finder_menu.o](/home/tobiasosborne/Projects/InitechOS/build/finder_menu.o)|0|0|0|IR|
|[build/tbxgate.o](/home/tobiasosborne/Projects/InitechOS/build/tbxgate.o)|0|0|0|IR|
|[build/tbx_gate.o](/home/tobiasosborne/Projects/InitechOS/build/tbx_gate.o)|105|4|0|native / subject to GC|
|[build/scrap.o](/home/tobiasosborne/Projects/InitechOS/build/scrap.o)|0|0|0|IR|
|[build/stdfile.o](/home/tobiasosborne/Projects/InitechOS/build/stdfile.o)|0|0|0|IR|
|[build/list.o](/home/tobiasosborne/Projects/InitechOS/build/list.o)|0|0|0|IR|
|[build/apple_menu.o](/home/tobiasosborne/Projects/InitechOS/build/apple_menu.o)|0|0|0|IR|
|[build/egg_jtc_model.o](/home/tobiasosborne/Projects/InitechOS/build/egg_jtc_model.o)|0|0|0|IR|
|[build/egg_jtc_data.o](/home/tobiasosborne/Projects/InitechOS/build/egg_jtc_data.o)|0|0|0|IR|
|[build/transient_want.o](/home/tobiasosborne/Projects/InitechOS/build/transient_want.o)|48|0|0|response-file insertion|
|[build/ovl_kernel.o](/home/tobiasosborne/Projects/InitechOS/build/ovl_kernel.o)|1,160|0|0|response-file insertion|

### Overlay object inventory

These are the eight code/data inputs from `OVL_SECTION_OBJS`, plus a generated header object; they are linked against the flagship resident ELF using `--just-symbols`, not linked into its resident payload ([Makefile:10842](/home/tobiasosborne/Projects/InitechOS/Makefile:10842), [Makefile:11047](/home/tobiasosborne/Projects/InitechOS/Makefile:11047)).

|Object|Text|Data|BSS|
|---|---:|---:|---:|
|[build/egg_jtc.o](/home/tobiasosborne/Projects/InitechOS/build/egg_jtc.o)|1,334|0|0|
|[build/about_box_ovl.o](/home/tobiasosborne/Projects/InitechOS/build/about_box_ovl.o)|298|0|96|
|[build/kmain_cold_ovl.o](/home/tobiasosborne/Projects/InitechOS/build/kmain_cold_ovl.o)|262|0|0|
|[build/finder_actions_ovl.o](/home/tobiasosborne/Projects/InitechOS/build/finder_actions_ovl.o)|6,778|0|0|
|[build/finder_ops_ovl.o](/home/tobiasosborne/Projects/InitechOS/build/finder_ops_ovl.o)|3,313|0|0|
|[build/finder_windows_ovl.o](/home/tobiasosborne/Projects/InitechOS/build/finder_windows_ovl.o)|3,745|0|0|
|[build/power_cold_ovl.o](/home/tobiasosborne/Projects/InitechOS/build/power_cold_ovl.o)|3,343|0|2,456|
|[build/loader_cold_ovl.o](/home/tobiasosborne/Projects/InitechOS/build/loader_cold_ovl.o)|2,332|0|0|
|[build/overlay_hdr.o](/home/tobiasosborne/Projects/InitechOS/build/overlay_hdr.o)|860|0|0|

## Appendix B. Complete source include-site evidence

Each line identifies a direct include occurrence counted in section 4. Includes into a component can appear in its headers as well as C sources; this is not a transitive fan-out count. All conditional arms are counted. The two host-only adapter includes and four explicit test-source includes into harness are marked.

### os/apps → os/flair
- [os/apps/itext/itext.c:71](/home/tobiasosborne/Projects/InitechOS/os/apps/itext/itext.c:71) -> [os/flair/textedit.h](/home/tobiasosborne/Projects/InitechOS/os/flair/textedit.h)
- [os/apps/itext/itext.c:72](/home/tobiasosborne/Projects/InitechOS/os/apps/itext/itext.c:72) -> [os/flair/text.h](/home/tobiasosborne/Projects/InitechOS/os/flair/text.h)
- [os/apps/notepad/notepad.c:18](/home/tobiasosborne/Projects/InitechOS/os/apps/notepad/notepad.c:18) -> [os/flair/textedit.h](/home/tobiasosborne/Projects/InitechOS/os/flair/textedit.h)
- [os/apps/notepad/notepad.c:19](/home/tobiasosborne/Projects/InitechOS/os/apps/notepad/notepad.c:19) -> [os/flair/text.h](/home/tobiasosborne/Projects/InitechOS/os/flair/text.h)
- [os/apps/ref_demo_menu.h:5](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_demo_menu.h:5) -> [os/flair/menu.h](/home/tobiasosborne/Projects/InitechOS/os/flair/menu.h)
- [os/apps/ref_tenant.c:77](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_tenant.c:77) -> [os/flair/process.h](/home/tobiasosborne/Projects/InitechOS/os/flair/process.h)
- [os/apps/ref_tenant.c:78](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_tenant.c:78) -> [os/flair/window.h](/home/tobiasosborne/Projects/InitechOS/os/flair/window.h)
- [os/apps/ref_tenant.c:81](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_tenant.c:81) -> [os/flair/blitter.h](/home/tobiasosborne/Projects/InitechOS/os/flair/blitter.h)
- [os/apps/ref_tenant.c:82](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_tenant.c:82) -> [os/flair/surface.h](/home/tobiasosborne/Projects/InitechOS/os/flair/surface.h)
- [os/apps/ref_tenant.c:83](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_tenant.c:83) -> [os/flair/heap.h](/home/tobiasosborne/Projects/InitechOS/os/flair/heap.h)
- [os/apps/ref_tenant.c:84](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_tenant.c:84) -> [os/flair/flair_look.h](/home/tobiasosborne/Projects/InitechOS/os/flair/flair_look.h)
- [os/apps/ref_tenant.h:29](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_tenant.h:29) -> [os/flair/process.h](/home/tobiasosborne/Projects/InitechOS/os/flair/process.h)

### os/apps → os/i123
- [os/apps/calc/calc.c:10](/home/tobiasosborne/Projects/InitechOS/os/apps/calc/calc.c:10) -> [os/i123/core/i123.h](/home/tobiasosborne/Projects/InitechOS/os/i123/core/i123.h)

### os/apps → spec
- [os/apps/calc/calc.c:11](/home/tobiasosborne/Projects/InitechOS/os/apps/calc/calc.c:11) -> [spec/i123/wk1_format.h](/home/tobiasosborne/Projects/InitechOS/spec/i123/wk1_format.h)
- [os/apps/calc/calc.c:12](/home/tobiasosborne/Projects/InitechOS/os/apps/calc/calc.c:12) -> [spec/assets/calc_ledger.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/calc_ledger.h)
- [os/apps/calcda/calc_view.c:8](/home/tobiasosborne/Projects/InitechOS/os/apps/calcda/calc_view.c:8) -> [spec/calcda/calcda_layout.h](/home/tobiasosborne/Projects/InitechOS/spec/calcda/calcda_layout.h)
- [os/apps/calcda/calc_view.c:9](/home/tobiasosborne/Projects/InitechOS/os/apps/calcda/calc_view.c:9) -> [spec/assets/calcda_geneva9.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/calcda_geneva9.h)
- [os/apps/calcda/calcda.c:35](/home/tobiasosborne/Projects/InitechOS/os/apps/calcda/calcda.c:35) -> [spec/calcda/calcda_layout.h](/home/tobiasosborne/Projects/InitechOS/spec/calcda/calcda_layout.h)
- [os/apps/cpanels/cpanels.h:4](/home/tobiasosborne/Projects/InitechOS/os/apps/cpanels/cpanels.h:4) -> [spec/cpanels_record.h](/home/tobiasosborne/Projects/InitechOS/spec/cpanels_record.h)
- [os/apps/cpanels/cpanels.h:5](/home/tobiasosborne/Projects/InitechOS/os/apps/cpanels/cpanels.h:5) -> [spec/cp_dialog.h](/home/tobiasosborne/Projects/InitechOS/spec/cp_dialog.h)
- [os/apps/itext/te_fonts.c:14](/home/tobiasosborne/Projects/InitechOS/os/apps/itext/te_fonts.c:14) -> [spec/assets/chicago12.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/chicago12.h)
- [os/apps/itext/te_fonts.c:15](/home/tobiasosborne/Projects/InitechOS/os/apps/itext/te_fonts.c:15) -> [spec/assets/geneva9.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/geneva9.h)
- [os/apps/itext/te_fonts.c:16](/home/tobiasosborne/Projects/InitechOS/os/apps/itext/te_fonts.c:16) -> [spec/assets/geneva12.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/geneva12.h)
- [os/apps/notepad/notepad.c:20](/home/tobiasosborne/Projects/InitechOS/os/apps/notepad/notepad.c:20) -> [spec/notepad/store.h](/home/tobiasosborne/Projects/InitechOS/spec/notepad/store.h)
- [os/apps/paint/paint.c:27](/home/tobiasosborne/Projects/InitechOS/os/apps/paint/paint.c:27) -> [spec/assets/menu_canon.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/menu_canon.h)
- [os/apps/ref_demo_menu.h:6](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_demo_menu.h:6) -> [spec/flair_tenants_demo.h](/home/tobiasosborne/Projects/InitechOS/spec/flair_tenants_demo.h)
- [os/apps/ref_tenant.c:79](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_tenant.c:79) -> [spec/window_record.h](/home/tobiasosborne/Projects/InitechOS/spec/window_record.h)
- [os/apps/ref_tenant.c:80](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_tenant.c:80) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/apps/ref_tenant.c:85](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_tenant.c:85) -> [spec/event_model.h](/home/tobiasosborne/Projects/InitechOS/spec/event_model.h)
- [os/apps/ref_tenant.c:86](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_tenant.c:86) -> [spec/flair_tenants_demo.h](/home/tobiasosborne/Projects/InitechOS/spec/flair_tenants_demo.h)
- [os/apps/tbx/tbx.h:46](/home/tobiasosborne/Projects/InitechOS/os/apps/tbx/tbx.h:46) -> [spec/toolbox_gate.h](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h)
- [os/apps/tbx/tbx.h:47](/home/tobiasosborne/Projects/InitechOS/os/apps/tbx/tbx.h:47) -> [spec/event_model.h](/home/tobiasosborne/Projects/InitechOS/spec/event_model.h)

### os/flair → os/milton
- [os/flair/cp_service.h:10](/home/tobiasosborne/Projects/InitechOS/os/flair/cp_service.h:10) -> [os/milton/pit.h](/home/tobiasosborne/Projects/InitechOS/os/milton/pit.h)
- [os/flair/film_startup.h:7](/home/tobiasosborne/Projects/InitechOS/os/flair/film_startup.h:7) -> [os/milton/panic_screen.h](/home/tobiasosborne/Projects/InitechOS/os/milton/panic_screen.h)
- [os/flair/film_startup.h:8](/home/tobiasosborne/Projects/InitechOS/os/flair/film_startup.h:8) -> [os/milton/fat12.h](/home/tobiasosborne/Projects/InitechOS/os/milton/fat12.h)
- [os/flair/tbxfile.h:8](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxfile.h:8) -> [os/milton/int21.h](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.h)
- [os/flair/tbxfile.h:9](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxfile.h:9) -> [os/milton/sft.h](/home/tobiasosborne/Projects/InitechOS/os/milton/sft.h)
- [os/flair/tbxgate.c:84](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:84) -> [os/milton/loader.h](/home/tobiasosborne/Projects/InitechOS/os/milton/loader.h)
- [os/flair/tbxgate.c:93](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:93) -> [os/milton/io.h](/home/tobiasosborne/Projects/InitechOS/os/milton/io.h)

### os/flair → spec
- [os/flair/apple_menu.c:8](/home/tobiasosborne/Projects/InitechOS/os/flair/apple_menu.c:8) -> [spec/apple_menu_items.h](/home/tobiasosborne/Projects/InitechOS/spec/apple_menu_items.h)
- [os/flair/atkinson/region.h:31](/home/tobiasosborne/Projects/InitechOS/os/flair/atkinson/region.h:31) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/blitter.c:46](/home/tobiasosborne/Projects/InitechOS/os/flair/blitter.c:46) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/blitter.h:58](/home/tobiasosborne/Projects/InitechOS/os/flair/blitter.h:58) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/chrome.c:15](/home/tobiasosborne/Projects/InitechOS/os/flair/chrome.c:15) -> [spec/chrome_metrics.h](/home/tobiasosborne/Projects/InitechOS/spec/chrome_metrics.h)
- [os/flair/chrome.c:16](/home/tobiasosborne/Projects/InitechOS/os/flair/chrome.c:16) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/chrome.c:19](/home/tobiasosborne/Projects/InitechOS/os/flair/chrome.c:19) -> [spec/window_record.h](/home/tobiasosborne/Projects/InitechOS/spec/window_record.h)
- [os/flair/chrome.h:36](/home/tobiasosborne/Projects/InitechOS/os/flair/chrome.h:36) -> [spec/grafport.h](/home/tobiasosborne/Projects/InitechOS/spec/grafport.h)
- [os/flair/chrome.h:37](/home/tobiasosborne/Projects/InitechOS/os/flair/chrome.h:37) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/chrome.h:39](/home/tobiasosborne/Projects/InitechOS/os/flair/chrome.h:39) -> [spec/window_record.h](/home/tobiasosborne/Projects/InitechOS/spec/window_record.h)
- [os/flair/control.c:56](/home/tobiasosborne/Projects/InitechOS/os/flair/control.c:56) -> [spec/chrome_metrics.h](/home/tobiasosborne/Projects/InitechOS/spec/chrome_metrics.h)
- [os/flair/control.c:57](/home/tobiasosborne/Projects/InitechOS/os/flair/control.c:57) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/control.c:59](/home/tobiasosborne/Projects/InitechOS/os/flair/control.c:59) -> [spec/progress_platinum.h](/home/tobiasosborne/Projects/InitechOS/spec/progress_platinum.h)
- [os/flair/control.h:105](/home/tobiasosborne/Projects/InitechOS/os/flair/control.h:105) -> [spec/grafport.h](/home/tobiasosborne/Projects/InitechOS/spec/grafport.h)
- [os/flair/control.h:106](/home/tobiasosborne/Projects/InitechOS/os/flair/control.h:106) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/control.h:107](/home/tobiasosborne/Projects/InitechOS/os/flair/control.h:107) -> [spec/chrome_metrics.h](/home/tobiasosborne/Projects/InitechOS/spec/chrome_metrics.h)
- [os/flair/cursor.c:22](/home/tobiasosborne/Projects/InitechOS/os/flair/cursor.c:22) -> [spec/assets/cursors.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/cursors.h)
- [os/flair/desktop.c:72](/home/tobiasosborne/Projects/InitechOS/os/flair/desktop.c:72) -> [spec/grafport.h](/home/tobiasosborne/Projects/InitechOS/spec/grafport.h)
- [os/flair/desktop.c:73](/home/tobiasosborne/Projects/InitechOS/os/flair/desktop.c:73) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/desktop.c:104](/home/tobiasosborne/Projects/InitechOS/os/flair/desktop.c:104) -> [spec/cpanels_record.h](/home/tobiasosborne/Projects/InitechOS/spec/cpanels_record.h)
- [os/flair/desktop.h:85](/home/tobiasosborne/Projects/InitechOS/os/flair/desktop.h:85) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/dialog.c:79](/home/tobiasosborne/Projects/InitechOS/os/flair/dialog.c:79) -> [spec/chrome_metrics.h](/home/tobiasosborne/Projects/InitechOS/spec/chrome_metrics.h)
- [os/flair/dialog.c:80](/home/tobiasosborne/Projects/InitechOS/os/flair/dialog.c:80) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/dialog.c:81](/home/tobiasosborne/Projects/InitechOS/os/flair/dialog.c:81) -> [spec/event_model.h](/home/tobiasosborne/Projects/InitechOS/spec/event_model.h)
- [os/flair/dialog.c:83](/home/tobiasosborne/Projects/InitechOS/os/flair/dialog.c:83) -> [spec/assets/alert_icons.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/alert_icons.h)
- [os/flair/dialog.h:75](/home/tobiasosborne/Projects/InitechOS/os/flair/dialog.h:75) -> [spec/grafport.h](/home/tobiasosborne/Projects/InitechOS/spec/grafport.h)
- [os/flair/dialog.h:76](/home/tobiasosborne/Projects/InitechOS/os/flair/dialog.h:76) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/dialog.h:77](/home/tobiasosborne/Projects/InitechOS/os/flair/dialog.h:77) -> [spec/window_record.h](/home/tobiasosborne/Projects/InitechOS/spec/window_record.h)
- [os/flair/dialog.h:79](/home/tobiasosborne/Projects/InitechOS/os/flair/dialog.h:79) -> [spec/chrome_metrics.h](/home/tobiasosborne/Projects/InitechOS/spec/chrome_metrics.h)
- [os/flair/egg_jtc_model.h:10](/home/tobiasosborne/Projects/InitechOS/os/flair/egg_jtc_model.h:10) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/egg_jtc_model.h:11](/home/tobiasosborne/Projects/InitechOS/os/flair/egg_jtc_model.h:11) -> [spec/event_model.h](/home/tobiasosborne/Projects/InitechOS/spec/event_model.h)
- [os/flair/event.h:65](/home/tobiasosborne/Projects/InitechOS/os/flair/event.h:65) -> [spec/event_model.h](/home/tobiasosborne/Projects/InitechOS/spec/event_model.h)
- [os/flair/film_scene.h:9](/home/tobiasosborne/Projects/InitechOS/os/flair/film_scene.h:9) -> [spec/event_model.h](/home/tobiasosborne/Projects/InitechOS/spec/event_model.h)
- [os/flair/film_startup.h:9](/home/tobiasosborne/Projects/InitechOS/os/flair/film_startup.h:9) -> [spec/toolbox_gate.h](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h)
- [os/flair/finder_desktop.c:22](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_desktop.c:22) -> [spec/assets/desk_icons.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/desk_icons.h)
- [os/flair/finder_desktop.c:23](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_desktop.c:23) -> [spec/assets/finder_icons.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/finder_icons.h)
- [os/flair/finder_desktop.c:63](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_desktop.c:63) -> [spec/assets/geneva9.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/geneva9.h)
- [os/flair/finder_desktop.h:107](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_desktop.h:107) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/finder_icon.c:19](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_icon.c:19) -> [spec/assets/finder_small_icons.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/finder_small_icons.h)
- [os/flair/finder_icon.h:54](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_icon.h:54) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/finder_icon.h:55](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_icon.h:55) -> [spec/assets/desk_icons.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/desk_icons.h)
- [os/flair/finder_windows.c:50](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_windows.c:50) -> [spec/assets/finder_list_titles.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/finder_list_titles.h)
- [os/flair/finder_windows.c:51](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_windows.c:51) -> [spec/event_model.h](/home/tobiasosborne/Projects/InitechOS/spec/event_model.h)
- [os/flair/finder_windows.c:52](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_windows.c:52) -> [spec/window_record.h](/home/tobiasosborne/Projects/InitechOS/spec/window_record.h)
- [os/flair/finder_windows.c:53](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_windows.c:53) -> [spec/assets/chicago12.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/chicago12.h)
- [os/flair/finder_windows.h:136](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_windows.h:136) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/finder_windows_cold.c:5](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_windows_cold.c:5) -> [spec/assets/chicago12.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/chicago12.h)
- [os/flair/flair_look.c:45](/home/tobiasosborne/Projects/InitechOS/os/flair/flair_look.c:45) -> [spec/assets/finder_small_icons.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/finder_small_icons.h)
- [os/flair/flair_look.c:46](/home/tobiasosborne/Projects/InitechOS/os/flair/flair_look.c:46) -> [spec/flair_skins.h](/home/tobiasosborne/Projects/InitechOS/spec/flair_skins.h)
- [os/flair/flair_look.c:48](/home/tobiasosborne/Projects/InitechOS/os/flair/flair_look.c:48) -> [spec/assets/color_canon.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/color_canon.h)
- [os/flair/flair_look.h:43](/home/tobiasosborne/Projects/InitechOS/os/flair/flair_look.h:43) -> [spec/grafport.h](/home/tobiasosborne/Projects/InitechOS/spec/grafport.h)
- [os/flair/list.h:41](/home/tobiasosborne/Projects/InitechOS/os/flair/list.h:41) -> [spec/grafport.h](/home/tobiasosborne/Projects/InitechOS/spec/grafport.h)
- [os/flair/list.h:42](/home/tobiasosborne/Projects/InitechOS/os/flair/list.h:42) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/menu.c:61](/home/tobiasosborne/Projects/InitechOS/os/flair/menu.c:61) -> [spec/assets/apple_glyph.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/apple_glyph.h)
- [os/flair/menu.h:54](/home/tobiasosborne/Projects/InitechOS/os/flair/menu.h:54) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/menu.h:55](/home/tobiasosborne/Projects/InitechOS/os/flair/menu.h:55) -> [spec/grafport.h](/home/tobiasosborne/Projects/InitechOS/spec/grafport.h)
- [os/flair/menu.h:70](/home/tobiasosborne/Projects/InitechOS/os/flair/menu.h:70) -> [spec/chrome_metrics.h](/home/tobiasosborne/Projects/InitechOS/spec/chrome_metrics.h)
- [os/flair/process.h:72](/home/tobiasosborne/Projects/InitechOS/os/flair/process.h:72) -> [spec/event_model.h](/home/tobiasosborne/Projects/InitechOS/spec/event_model.h)
- [os/flair/shell.c:68](/home/tobiasosborne/Projects/InitechOS/os/flair/shell.c:68) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/shell.c:69](/home/tobiasosborne/Projects/InitechOS/os/flair/shell.c:69) -> [spec/assets/menu_canon.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/menu_canon.h)
- [os/flair/shell.h:88](/home/tobiasosborne/Projects/InitechOS/os/flair/shell.h:88) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/tbxfile.h:7](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxfile.h:7) -> [spec/toolbox_gate.h](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h)
- [os/flair/tbxgate.c:76](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:76) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/tbxgate.c:77](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:77) -> [spec/window_record.h](/home/tobiasosborne/Projects/InitechOS/spec/window_record.h)
- [os/flair/tbxgate.c:78](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:78) -> [spec/event_model.h](/home/tobiasosborne/Projects/InitechOS/spec/event_model.h)
- [os/flair/tbxgate.c:79](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:79) -> [spec/chrome_metrics.h](/home/tobiasosborne/Projects/InitechOS/spec/chrome_metrics.h)
- [os/flair/tbxgate.c:81](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:81) -> [spec/toolbox_gate.h](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h)
- [os/flair/tbxgate.c:88](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:88) -> [spec/find_data.h](/home/tobiasosborne/Projects/InitechOS/spec/find_data.h)
- [os/flair/tbxgate.c:91](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:91) -> [spec/cp_dialog.h](/home/tobiasosborne/Projects/InitechOS/spec/cp_dialog.h)
- [os/flair/tbxgate.c:92](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:92) -> [spec/cpanels_record.h](/home/tobiasosborne/Projects/InitechOS/spec/cpanels_record.h)
- [os/flair/tbxgate.h:53](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.h:53) -> [spec/toolbox_gate.h](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h)
- [os/flair/tbxmenu.h:47](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxmenu.h:47) -> [spec/event_model.h](/home/tobiasosborne/Projects/InitechOS/spec/event_model.h)
- [os/flair/tbxmenu.h:48](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxmenu.h:48) -> [spec/toolbox_gate.h](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h)
- [os/flair/text.h:88](/home/tobiasosborne/Projects/InitechOS/os/flair/text.h:88) -> [spec/assets/chicago12.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/chicago12.h)
- [os/flair/text.h:89](/home/tobiasosborne/Projects/InitechOS/os/flair/text.h:89) -> [spec/assets/geneva9.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/geneva9.h)
- [os/flair/text.h:90](/home/tobiasosborne/Projects/InitechOS/os/flair/text.h:90) -> [spec/assets/geneva12.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/geneva12.h)
- [os/flair/window.c:48](/home/tobiasosborne/Projects/InitechOS/os/flair/window.c:48) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/window.c:51](/home/tobiasosborne/Projects/InitechOS/os/flair/window.c:51) -> [spec/chrome_metrics.h](/home/tobiasosborne/Projects/InitechOS/spec/chrome_metrics.h)
- [os/flair/window.c:52](/home/tobiasosborne/Projects/InitechOS/os/flair/window.c:52) -> [spec/assets/chicago12.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/chicago12.h)
- [os/flair/window.h:72](/home/tobiasosborne/Projects/InitechOS/os/flair/window.h:72) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/window.h:73](/home/tobiasosborne/Projects/InitechOS/os/flair/window.h:73) -> [spec/window_record.h](/home/tobiasosborne/Projects/InitechOS/spec/window_record.h)
- [os/flair/winscroll.h:81](/home/tobiasosborne/Projects/InitechOS/os/flair/winscroll.h:81) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/flair/winscroll.h:82](/home/tobiasosborne/Projects/InitechOS/os/flair/winscroll.h:82) -> [spec/window_record.h](/home/tobiasosborne/Projects/InitechOS/spec/window_record.h)
- [os/flair/winscroll.h:83](/home/tobiasosborne/Projects/InitechOS/os/flair/winscroll.h:83) -> [spec/chrome_metrics.h](/home/tobiasosborne/Projects/InitechOS/spec/chrome_metrics.h)

### os/i123 → os/apps
- [os/i123/app/face_a.c:72](/home/tobiasosborne/Projects/InitechOS/os/i123/app/face_a.c:72) -> [os/apps/tbx/tbx.h](/home/tobiasosborne/Projects/InitechOS/os/apps/tbx/tbx.h)
- [os/i123/app/face_a.c:73](/home/tobiasosborne/Projects/InitechOS/os/i123/app/face_a.c:73) -> [os/apps/tbx/tbx_save.h](/home/tobiasosborne/Projects/InitechOS/os/apps/tbx/tbx_save.h)

### os/i123 → spec
- [os/i123/app/face_a.c:76](/home/tobiasosborne/Projects/InitechOS/os/i123/app/face_a.c:76) -> [spec/i123/wk1_format.h](/home/tobiasosborne/Projects/InitechOS/spec/i123/wk1_format.h)
- [os/i123/app/face_a.c:77](/home/tobiasosborne/Projects/InitechOS/os/i123/app/face_a.c:77) -> [spec/cpanels_record.h](/home/tobiasosborne/Projects/InitechOS/spec/cpanels_record.h)
- [os/i123/core/eval.c:44](/home/tobiasosborne/Projects/InitechOS/os/i123/core/eval.c:44) -> [spec/i123/wk1_format.h](/home/tobiasosborne/Projects/InitechOS/spec/i123/wk1_format.h)
- [os/i123/core/eval.c:45](/home/tobiasosborne/Projects/InitechOS/os/i123/core/eval.c:45) -> [spec/i123/wk1_formula.h](/home/tobiasosborne/Projects/InitechOS/spec/i123/wk1_formula.h)
- [os/i123/core/fmt.c:53](/home/tobiasosborne/Projects/InitechOS/os/i123/core/fmt.c:53) -> [spec/i123/wk1_format.h](/home/tobiasosborne/Projects/InitechOS/spec/i123/wk1_format.h)
- [os/i123/core/formula.c:55](/home/tobiasosborne/Projects/InitechOS/os/i123/core/formula.c:55) -> [spec/i123/wk1_format.h](/home/tobiasosborne/Projects/InitechOS/spec/i123/wk1_format.h)
- [os/i123/core/formula.c:56](/home/tobiasosborne/Projects/InitechOS/os/i123/core/formula.c:56) -> [spec/i123/wk1_formula.h](/home/tobiasosborne/Projects/InitechOS/spec/i123/wk1_formula.h)
- [os/i123/core/range.c:10](/home/tobiasosborne/Projects/InitechOS/os/i123/core/range.c:10) -> [spec/i123/wk1_format.h](/home/tobiasosborne/Projects/InitechOS/spec/i123/wk1_format.h)
- [os/i123/core/range.c:11](/home/tobiasosborne/Projects/InitechOS/os/i123/core/range.c:11) -> [spec/i123/wk1_formula.h](/home/tobiasosborne/Projects/InitechOS/spec/i123/wk1_formula.h)
- [os/i123/core/recalc.c:59](/home/tobiasosborne/Projects/InitechOS/os/i123/core/recalc.c:59) -> [spec/i123/wk1_format.h](/home/tobiasosborne/Projects/InitechOS/spec/i123/wk1_format.h)
- [os/i123/core/recalc.c:60](/home/tobiasosborne/Projects/InitechOS/os/i123/core/recalc.c:60) -> [spec/i123/wk1_formula.h](/home/tobiasosborne/Projects/InitechOS/spec/i123/wk1_formula.h)
- [os/i123/core/sheet.c:23](/home/tobiasosborne/Projects/InitechOS/os/i123/core/sheet.c:23) -> [spec/i123/wk1_format.h](/home/tobiasosborne/Projects/InitechOS/spec/i123/wk1_format.h)
- [os/i123/fs/wk1.c:24](/home/tobiasosborne/Projects/InitechOS/os/i123/fs/wk1.c:24) -> [spec/i123/wk1_format.h](/home/tobiasosborne/Projects/InitechOS/spec/i123/wk1_format.h)

### os/iword → os/apps
- [os/iword/app/face_a.c:165](/home/tobiasosborne/Projects/InitechOS/os/iword/app/face_a.c:165) -> [os/apps/tbx/tbx.h](/home/tobiasosborne/Projects/InitechOS/os/apps/tbx/tbx.h)
- [os/iword/app/face_a.c:171](/home/tobiasosborne/Projects/InitechOS/os/iword/app/face_a.c:171) -> [os/apps/tbx/tbx_save.h](/home/tobiasosborne/Projects/InitechOS/os/apps/tbx/tbx_save.h)

### os/iword → spec
- [os/iword/app/face_a.c:172](/home/tobiasosborne/Projects/InitechOS/os/iword/app/face_a.c:172) -> [spec/iword/fkeys.h](/home/tobiasosborne/Projects/InitechOS/spec/iword/fkeys.h)
- [os/iword/app/face_a.c:174](/home/tobiasosborne/Projects/InitechOS/os/iword/app/face_a.c:174) -> [spec/iword/wp5_format.h](/home/tobiasosborne/Projects/InitechOS/spec/iword/wp5_format.h)
- [os/iword/app/face_a.c:175](/home/tobiasosborne/Projects/InitechOS/os/iword/app/face_a.c:175) -> [spec/iword/reveal_codes.h](/home/tobiasosborne/Projects/InitechOS/spec/iword/reveal_codes.h)
- [os/iword/app/face_a.c:176](/home/tobiasosborne/Projects/InitechOS/os/iword/app/face_a.c:176) -> [spec/iword/settings.h](/home/tobiasosborne/Projects/InitechOS/spec/iword/settings.h)
- [os/iword/app/face_a.c:177](/home/tobiasosborne/Projects/InitechOS/os/iword/app/face_a.c:177) -> [spec/iword/listfiles.h](/home/tobiasosborne/Projects/InitechOS/spec/iword/listfiles.h)
- [os/iword/app/face_a.c:178](/home/tobiasosborne/Projects/InitechOS/os/iword/app/face_a.c:178) -> [spec/cpanels_record.h](/home/tobiasosborne/Projects/InitechOS/spec/cpanels_record.h)
- [os/iword/core/block.c:42](/home/tobiasosborne/Projects/InitechOS/os/iword/core/block.c:42) -> [spec/iword/wp5_format.h](/home/tobiasosborne/Projects/InitechOS/spec/iword/wp5_format.h)
- [os/iword/core/codes.c:33](/home/tobiasosborne/Projects/InitechOS/os/iword/core/codes.c:33) -> [spec/iword/wp5_format.h](/home/tobiasosborne/Projects/InitechOS/spec/iword/wp5_format.h)
- [os/iword/core/codes.c:34](/home/tobiasosborne/Projects/InitechOS/os/iword/core/codes.c:34) -> [spec/iword/reveal_codes.h](/home/tobiasosborne/Projects/InitechOS/spec/iword/reveal_codes.h)
- [os/iword/core/edit.c:50](/home/tobiasosborne/Projects/InitechOS/os/iword/core/edit.c:50) -> [spec/iword/wp5_format.h](/home/tobiasosborne/Projects/InitechOS/spec/iword/wp5_format.h)
- [os/iword/core/listfiles.c:20](/home/tobiasosborne/Projects/InitechOS/os/iword/core/listfiles.c:20) -> [spec/iword/listfiles.h](/home/tobiasosborne/Projects/InitechOS/spec/iword/listfiles.h)
- [os/iword/core/listfiles.c:21](/home/tobiasosborne/Projects/InitechOS/os/iword/core/listfiles.c:21) -> [spec/toolbox_gate.h](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h)
- [os/iword/core/piece.c:22](/home/tobiasosborne/Projects/InitechOS/os/iword/core/piece.c:22) -> [spec/iword/wp5_format.h](/home/tobiasosborne/Projects/InitechOS/spec/iword/wp5_format.h)
- [os/iword/fs/wp5.c:25](/home/tobiasosborne/Projects/InitechOS/os/iword/fs/wp5.c:25) -> [spec/iword/wp5_format.h](/home/tobiasosborne/Projects/InitechOS/spec/iword/wp5_format.h)

### os/milton → harness
- [os/milton/test_kbd.c:37](/home/tobiasosborne/Projects/InitechOS/os/milton/test_kbd.c:37) -> [harness/proptest/expect_abort.h](/home/tobiasosborne/Projects/InitechOS/harness/proptest/expect_abort.h) [TEST]
- [os/milton/command.c:3067](/home/tobiasosborne/Projects/InitechOS/os/milton/command.c:3067) -> [harness/diff/fat_diff/shell_files_host.h](/home/tobiasosborne/Projects/InitechOS/harness/diff/fat_diff/shell_files_host.h) [HOST CONDITIONAL]
- [os/milton/command.c:3070](/home/tobiasosborne/Projects/InitechOS/os/milton/command.c:3070) -> [harness/diff/fat_diff/batch_break_host.h](/home/tobiasosborne/Projects/InitechOS/harness/diff/fat_diff/batch_break_host.h) [HOST CONDITIONAL]
- [os/milton/test_dos_safety.c:17](/home/tobiasosborne/Projects/InitechOS/os/milton/test_dos_safety.c:17) -> [harness/diff/fat_diff/blockdev_file.h](/home/tobiasosborne/Projects/InitechOS/harness/diff/fat_diff/blockdev_file.h) [TEST]
- [os/milton/test_fileio_subdir.c:38](/home/tobiasosborne/Projects/InitechOS/os/milton/test_fileio_subdir.c:38) -> [harness/diff/fat_diff/blockdev_file.h](/home/tobiasosborne/Projects/InitechOS/harness/diff/fat_diff/blockdev_file.h) [TEST]
- [os/milton/test_int24_wired.c:69](/home/tobiasosborne/Projects/InitechOS/os/milton/test_int24_wired.c:69) -> [harness/diff/fat_diff/blockdev_file.h](/home/tobiasosborne/Projects/InitechOS/harness/diff/fat_diff/blockdev_file.h) [TEST]

### os/milton → os/apps
- [os/milton/kmain.c:110](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:110) -> [os/apps/ref_tenant.h](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_tenant.h)
- [os/milton/kmain.c:182](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:182) -> [os/apps/ref_demo_menu.h](/home/tobiasosborne/Projects/InitechOS/os/apps/ref_demo_menu.h)

### os/milton → os/flair
- [os/milton/boot_screen.h:42](/home/tobiasosborne/Projects/InitechOS/os/milton/boot_screen.h:42) -> [os/flair/asset_lz.h](/home/tobiasosborne/Projects/InitechOS/os/flair/asset_lz.h)
- [os/milton/boot_screen.h:43](/home/tobiasosborne/Projects/InitechOS/os/milton/boot_screen.h:43) -> [os/flair/asset_lz_data.h](/home/tobiasosborne/Projects/InitechOS/os/flair/asset_lz_data.h)
- [os/milton/boot_screen.h:46](/home/tobiasosborne/Projects/InitechOS/os/milton/boot_screen.h:46) -> [os/flair/text.h](/home/tobiasosborne/Projects/InitechOS/os/flair/text.h)
- [os/milton/console.c:32](/home/tobiasosborne/Projects/InitechOS/os/milton/console.c:32) -> [os/flair/surface.h](/home/tobiasosborne/Projects/InitechOS/os/flair/surface.h)
- [os/milton/finder_actions.c:6](/home/tobiasosborne/Projects/InitechOS/os/milton/finder_actions.c:6) -> [os/flair/tbxgate.h](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.h)
- [os/milton/finder_actions.c:8](/home/tobiasosborne/Projects/InitechOS/os/milton/finder_actions.c:8) -> [os/flair/event.h](/home/tobiasosborne/Projects/InitechOS/os/flair/event.h)
- [os/milton/finder_actions.h:7](/home/tobiasosborne/Projects/InitechOS/os/milton/finder_actions.h:7) -> [os/flair/shell.h](/home/tobiasosborne/Projects/InitechOS/os/flair/shell.h)
- [os/milton/finder_actions.h:8](/home/tobiasosborne/Projects/InitechOS/os/milton/finder_actions.h:8) -> [os/flair/finder_ops.h](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_ops.h)
- [os/milton/finder_actions.h:9](/home/tobiasosborne/Projects/InitechOS/os/milton/finder_actions.h:9) -> [os/flair/dialog.h](/home/tobiasosborne/Projects/InitechOS/os/flair/dialog.h)
- [os/milton/finder_actions.h:12](/home/tobiasosborne/Projects/InitechOS/os/milton/finder_actions.h:12) -> [os/flair/event.h](/home/tobiasosborne/Projects/InitechOS/os/flair/event.h)
- [os/milton/kbd.c:18](/home/tobiasosborne/Projects/InitechOS/os/milton/kbd.c:18) -> [os/flair/ring_report.h](/home/tobiasosborne/Projects/InitechOS/os/flair/ring_report.h)
- [os/milton/kmain.c:43](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:43) -> [os/flair/surface.h](/home/tobiasosborne/Projects/InitechOS/os/flair/surface.h)
- [os/milton/kmain.c:65](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:65) -> [os/flair/film_startup.h](/home/tobiasosborne/Projects/InitechOS/os/flair/film_startup.h)
- [os/milton/kmain.c:77](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:77) -> [os/flair/heap.h](/home/tobiasosborne/Projects/InitechOS/os/flair/heap.h)
- [os/milton/kmain.c:78](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:78) -> [os/flair/surface.h](/home/tobiasosborne/Projects/InitechOS/os/flair/surface.h)
- [os/milton/kmain.c:79](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:79) -> [os/flair/cursor.h](/home/tobiasosborne/Projects/InitechOS/os/flair/cursor.h)
- [os/milton/kmain.c:81](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:81) -> [os/flair/atkinson/region.h](/home/tobiasosborne/Projects/InitechOS/os/flair/atkinson/region.h)
- [os/milton/kmain.c:83](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:83) -> [os/flair/shell.h](/home/tobiasosborne/Projects/InitechOS/os/flair/shell.h)
- [os/milton/kmain.c:84](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:84) -> [os/flair/shell_menus.h](/home/tobiasosborne/Projects/InitechOS/os/flair/shell_menus.h)
- [os/milton/kmain.c:85](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:85) -> [os/flair/desktop.h](/home/tobiasosborne/Projects/InitechOS/os/flair/desktop.h)
- [os/milton/kmain.c:86](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:86) -> [os/flair/flair_look.h](/home/tobiasosborne/Projects/InitechOS/os/flair/flair_look.h)
- [os/milton/kmain.c:87](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:87) -> [os/flair/chrome.h](/home/tobiasosborne/Projects/InitechOS/os/flair/chrome.h)
- [os/milton/kmain.c:91](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:91) -> [os/flair/event.h](/home/tobiasosborne/Projects/InitechOS/os/flair/event.h)
- [os/milton/kmain.c:102](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:102) -> [os/flair/process.h](/home/tobiasosborne/Projects/InitechOS/os/flair/process.h)
- [os/milton/kmain.c:104](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:104) -> [os/flair/system_key.h](/home/tobiasosborne/Projects/InitechOS/os/flair/system_key.h)
- [os/milton/kmain.c:105](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:105) -> [os/flair/power_notice.h](/home/tobiasosborne/Projects/InitechOS/os/flair/power_notice.h)
- [os/milton/kmain.c:106](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:106) -> [os/flair/egg_jtc.h](/home/tobiasosborne/Projects/InitechOS/os/flair/egg_jtc.h)
- [os/milton/kmain.c:108](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:108) -> [os/flair/film_scene.h](/home/tobiasosborne/Projects/InitechOS/os/flair/film_scene.h)
- [os/milton/kmain.c:112](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:112) -> [os/flair/finder_desktop.h](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_desktop.h)
- [os/milton/kmain.c:116](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:116) -> [os/flair/finder_cmd.h](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_cmd.h)
- [os/milton/kmain.c:119](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:119) -> [os/flair/finder_windows.h](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_windows.h)
- [os/milton/kmain.c:123](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:123) -> [os/flair/finder_ops.h](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_ops.h)
- [os/milton/kmain.c:126](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:126) -> [os/flair/finder_menu.h](/home/tobiasosborne/Projects/InitechOS/os/flair/finder_menu.h)
- [os/milton/kmain.c:129](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:129) -> [os/flair/tbxgate.h](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.h)
- [os/milton/kmain.c:131](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:131) -> [os/flair/apple_menu.h](/home/tobiasosborne/Projects/InitechOS/os/flair/apple_menu.h)
- [os/milton/kmain.c:135](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:135) -> [os/flair/stdfile.h](/home/tobiasosborne/Projects/InitechOS/os/flair/stdfile.h)
- [os/milton/kmain.c:136](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:136) -> [os/flair/scrap.h](/home/tobiasosborne/Projects/InitechOS/os/flair/scrap.h)
- [os/milton/power_cold.c:20](/home/tobiasosborne/Projects/InitechOS/os/milton/power_cold.c:20) -> [os/flair/tbxgate.h](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.h)
- [os/milton/power_cold.c:22](/home/tobiasosborne/Projects/InitechOS/os/milton/power_cold.c:22) -> [os/flair/power_notice.h](/home/tobiasosborne/Projects/InitechOS/os/flair/power_notice.h)

### os/milton → seed
- [os/milton/test_40oq.c:82](/home/tobiasosborne/Projects/InitechOS/os/milton/test_40oq.c:82) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_ansi.c:37](/home/tobiasosborne/Projects/InitechOS/os/milton/test_ansi.c:37) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_ansi_wire.c:53](/home/tobiasosborne/Projects/InitechOS/os/milton/test_ansi_wire.c:53) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_arena_disjoint.c:56](/home/tobiasosborne/Projects/InitechOS/os/milton/test_arena_disjoint.c:56) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_ata.c:7](/home/tobiasosborne/Projects/InitechOS/os/milton/test_ata.c:7) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_batch.c:32](/home/tobiasosborne/Projects/InitechOS/os/milton/test_batch.c:32) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_batch_exec.c:37](/home/tobiasosborne/Projects/InitechOS/os/milton/test_batch_exec.c:37) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_command.c:34](/home/tobiasosborne/Projects/InitechOS/os/milton/test_command.c:34) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_config_sys.c:34](/home/tobiasosborne/Projects/InitechOS/os/milton/test_config_sys.c:34) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_conin.c:49](/home/tobiasosborne/Projects/InitechOS/os/milton/test_conin.c:49) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_console.c:37](/home/tobiasosborne/Projects/InitechOS/os/milton/test_console.c:37) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_devices.c:31](/home/tobiasosborne/Projects/InitechOS/os/milton/test_devices.c:31) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_devwire.c:45](/home/tobiasosborne/Projects/InitechOS/os/milton/test_devwire.c:45) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_dos_safety.c:18](/home/tobiasosborne/Projects/InitechOS/os/milton/test_dos_safety.c:18) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_env.c:32](/home/tobiasosborne/Projects/InitechOS/os/milton/test_env.c:32) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_exec.c:36](/home/tobiasosborne/Projects/InitechOS/os/milton/test_exec.c:36) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_fileio.c:47](/home/tobiasosborne/Projects/InitechOS/os/milton/test_fileio.c:47) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_fileio_subdir.c:39](/home/tobiasosborne/Projects/InitechOS/os/milton/test_fileio_subdir.c:39) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_flair_heap_ram.c:45](/home/tobiasosborne/Projects/InitechOS/os/milton/test_flair_heap_ram.c:45) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_fpu.c:40](/home/tobiasosborne/Projects/InitechOS/os/milton/test_fpu.c:40) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_idt.c:28](/home/tobiasosborne/Projects/InitechOS/os/milton/test_idt.c:28) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_int21.c:45](/home/tobiasosborne/Projects/InitechOS/os/milton/test_int21.c:45) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_int21_edge.c:52](/home/tobiasosborne/Projects/InitechOS/os/milton/test_int21_edge.c:52) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_int21_wildcard.c:41](/home/tobiasosborne/Projects/InitechOS/os/milton/test_int21_wildcard.c:41) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_int24.c:52](/home/tobiasosborne/Projects/InitechOS/os/milton/test_int24.c:52) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_int24_wired.c:70](/home/tobiasosborne/Projects/InitechOS/os/milton/test_int24_wired.c:70) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_kbd.c:34](/home/tobiasosborne/Projects/InitechOS/os/milton/test_kbd.c:34) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_loader.c:39](/home/tobiasosborne/Projects/InitechOS/os/milton/test_loader.c:39) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_loader_big.c:52](/home/tobiasosborne/Projects/InitechOS/os/milton/test_loader_big.c:52) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_mcb.c:21](/home/tobiasosborne/Projects/InitechOS/os/milton/test_mcb.c:21) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_mcb_int21.c:33](/home/tobiasosborne/Projects/InitechOS/os/milton/test_mcb_int21.c:33) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_mz.c:35](/home/tobiasosborne/Projects/InitechOS/os/milton/test_mz.c:35) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_mzload.c:59](/home/tobiasosborne/Projects/InitechOS/os/milton/test_mzload.c:59) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_overlay.c:49](/home/tobiasosborne/Projects/InitechOS/os/milton/test_overlay.c:49) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_psp.c:36](/home/tobiasosborne/Projects/InitechOS/os/milton/test_psp.c:36) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_redir.c:48](/home/tobiasosborne/Projects/InitechOS/os/milton/test_redir.c:48) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_redir_parse.c:34](/home/tobiasosborne/Projects/InitechOS/os/milton/test_redir_parse.c:34) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_rtc.c:22](/home/tobiasosborne/Projects/InitechOS/os/milton/test_rtc.c:22) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_sft.c:38](/home/tobiasosborne/Projects/InitechOS/os/milton/test_sft.c:38) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]
- [os/milton/test_tenant_load.c:37](/home/tobiasosborne/Projects/InitechOS/os/milton/test_tenant_load.c:37) -> [seed/test_assert.h](/home/tobiasosborne/Projects/InitechOS/seed/test_assert.h) [TEST]

### os/milton → spec
- [os/milton/block_device.c:4](/home/tobiasosborne/Projects/InitechOS/os/milton/block_device.c:4) -> [spec/block_device_contract.h](/home/tobiasosborne/Projects/InitechOS/spec/block_device_contract.h)
- [os/milton/block_device.c:5](/home/tobiasosborne/Projects/InitechOS/os/milton/block_device.c:5) -> [spec/drive_letters.h](/home/tobiasosborne/Projects/InitechOS/spec/drive_letters.h)
- [os/milton/boot_screen.h:34](/home/tobiasosborne/Projects/InitechOS/os/milton/boot_screen.h:34) -> [spec/assets/boot_logo.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/boot_logo.h)
- [os/milton/chkdsk_program.c:7](/home/tobiasosborne/Projects/InitechOS/os/milton/chkdsk_program.c:7) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h)
- [os/milton/command.c:18](/home/tobiasosborne/Projects/InitechOS/os/milton/command.c:18) -> [spec/dos_structs.h](/home/tobiasosborne/Projects/InitechOS/spec/dos_structs.h)
- [os/milton/command.c:1767](/home/tobiasosborne/Projects/InitechOS/os/milton/command.c:1767) -> [spec/find_data.h](/home/tobiasosborne/Projects/InitechOS/spec/find_data.h)
- [os/milton/command.c:1769](/home/tobiasosborne/Projects/InitechOS/os/milton/command.c:1769) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h)
- [os/milton/diskutil_program.c:4](/home/tobiasosborne/Projects/InitechOS/os/milton/diskutil_program.c:4) -> [spec/drive_letters.h](/home/tobiasosborne/Projects/InitechOS/spec/drive_letters.h)
- [os/milton/diskutil_program.c:5](/home/tobiasosborne/Projects/InitechOS/os/milton/diskutil_program.c:5) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h)
- [os/milton/dosutil_target.c:4](/home/tobiasosborne/Projects/InitechOS/os/milton/dosutil_target.c:4) -> [spec/find_data.h](/home/tobiasosborne/Projects/InitechOS/spec/find_data.h)
- [os/milton/edlin_program.c:4](/home/tobiasosborne/Projects/InitechOS/os/milton/edlin_program.c:4) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h)
- [os/milton/fat12.h:25](/home/tobiasosborne/Projects/InitechOS/os/milton/fat12.h:25) -> [spec/dos_structs.h](/home/tobiasosborne/Projects/InitechOS/spec/dos_structs.h)
- [os/milton/fdisk_program.c:7](/home/tobiasosborne/Projects/InitechOS/os/milton/fdisk_program.c:7) -> [spec/drive_letters.h](/home/tobiasosborne/Projects/InitechOS/spec/drive_letters.h)
- [os/milton/fileio_fat.c:49](/home/tobiasosborne/Projects/InitechOS/os/milton/fileio_fat.c:49) -> [spec/dos_structs.h](/home/tobiasosborne/Projects/InitechOS/spec/dos_structs.h)
- [os/milton/finder_actions.c:7](/home/tobiasosborne/Projects/InitechOS/os/milton/finder_actions.c:7) -> [spec/toolbox_gate.h](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h)
- [os/milton/int21.c:24](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:24) -> [spec/dos_structs.h](/home/tobiasosborne/Projects/InitechOS/spec/dos_structs.h)
- [os/milton/int21.c:25](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:25) -> [spec/tsr_contract.h](/home/tobiasosborne/Projects/InitechOS/spec/tsr_contract.h)
- [os/milton/int21.c:26](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:26) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h)
- [os/milton/int21.c:27](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:27) -> [spec/find_data.h](/home/tobiasosborne/Projects/InitechOS/spec/find_data.h)
- [os/milton/kbd.c:19](/home/tobiasosborne/Projects/InitechOS/os/milton/kbd.c:19) -> [spec/tsr_contract.h](/home/tobiasosborne/Projects/InitechOS/spec/tsr_contract.h)
- [os/milton/kmain.c:40](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:40) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h)
- [os/milton/kmain.c:41](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:41) -> [spec/tsr_contract.h](/home/tobiasosborne/Projects/InitechOS/spec/tsr_contract.h)
- [os/milton/kmain.c:80](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:80) -> [spec/region_algebra.h](/home/tobiasosborne/Projects/InitechOS/spec/region_algebra.h)
- [os/milton/kmain.c:88](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:88) -> [spec/assets/menu_canon.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/menu_canon.h)
- [os/milton/kmain.c:89](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:89) -> [spec/assets/palette.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/palette.h)
- [os/milton/kmain.c:111](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:111) -> [spec/flair_tenants_demo.h](/home/tobiasosborne/Projects/InitechOS/spec/flair_tenants_demo.h)
- [os/milton/kmain.c:134](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:134) -> [spec/toolbox_gate.h](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h)
- [os/milton/kmain.c:138](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:138) -> [spec/overlay_format.h](/home/tobiasosborne/Projects/InitechOS/spec/overlay_format.h)
- [os/milton/kmain.c:5830](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:5830) -> [spec/i123_copy_smoke_keys.h](/home/tobiasosborne/Projects/InitechOS/spec/i123_copy_smoke_keys.h)
- [os/milton/loader.c:24](/home/tobiasosborne/Projects/InitechOS/os/milton/loader.c:24) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h)
- [os/milton/loader.c:28](/home/tobiasosborne/Projects/InitechOS/os/milton/loader.c:28) -> [spec/toolbox_gate.h](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h)
- [os/milton/loader_cold.c:6](/home/tobiasosborne/Projects/InitechOS/os/milton/loader_cold.c:6) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h)
- [os/milton/mcb.h:30](/home/tobiasosborne/Projects/InitechOS/os/milton/mcb.h:30) -> [spec/dos_structs.h](/home/tobiasosborne/Projects/InitechOS/spec/dos_structs.h)
- [os/milton/mz.h:30](/home/tobiasosborne/Projects/InitechOS/os/milton/mz.h:30) -> [spec/initechmz_extended.h](/home/tobiasosborne/Projects/InitechOS/spec/initechmz_extended.h)
- [os/milton/native.c:6](/home/tobiasosborne/Projects/InitechOS/os/milton/native.c:6) -> [spec/initechmz_extended.h](/home/tobiasosborne/Projects/InitechOS/spec/initechmz_extended.h)
- [os/milton/power_cold.c:27](/home/tobiasosborne/Projects/InitechOS/os/milton/power_cold.c:27) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h)
- [os/milton/power_cold.c:29](/home/tobiasosborne/Projects/InitechOS/os/milton/power_cold.c:29) -> [spec/assets/color_canon.h](/home/tobiasosborne/Projects/InitechOS/spec/assets/color_canon.h)
- [os/milton/psp.h:32](/home/tobiasosborne/Projects/InitechOS/os/milton/psp.h:32) -> [spec/dos_structs.h](/home/tobiasosborne/Projects/InitechOS/spec/dos_structs.h)
- [os/milton/sysinit.c:25](/home/tobiasosborne/Projects/InitechOS/os/milton/sysinit.c:25) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h)
- [os/milton/sysinit.c:27](/home/tobiasosborne/Projects/InitechOS/os/milton/sysinit.c:27) -> [spec/fpu_contract.h](/home/tobiasosborne/Projects/InitechOS/spec/fpu_contract.h)
- [os/milton/sysinit.c:28](/home/tobiasosborne/Projects/InitechOS/os/milton/sysinit.c:28) -> [spec/tsr_contract.h](/home/tobiasosborne/Projects/InitechOS/spec/tsr_contract.h)
- [os/milton/test_arena_disjoint.c:55](/home/tobiasosborne/Projects/InitechOS/os/milton/test_arena_disjoint.c:55) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h) [TEST]
- [os/milton/test_command.c:33](/home/tobiasosborne/Projects/InitechOS/os/milton/test_command.c:33) -> [spec/dos_structs.h](/home/tobiasosborne/Projects/InitechOS/spec/dos_structs.h) [TEST]
- [os/milton/test_exec.c:34](/home/tobiasosborne/Projects/InitechOS/os/milton/test_exec.c:34) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h) [TEST]
- [os/milton/test_exec.c:35](/home/tobiasosborne/Projects/InitechOS/os/milton/test_exec.c:35) -> [spec/dos_structs.h](/home/tobiasosborne/Projects/InitechOS/spec/dos_structs.h) [TEST]
- [os/milton/test_fileio.c:45](/home/tobiasosborne/Projects/InitechOS/os/milton/test_fileio.c:45) -> [spec/dos_structs.h](/home/tobiasosborne/Projects/InitechOS/spec/dos_structs.h) [TEST]
- [os/milton/test_fileio.c:46](/home/tobiasosborne/Projects/InitechOS/os/milton/test_fileio.c:46) -> [spec/find_data.h](/home/tobiasosborne/Projects/InitechOS/spec/find_data.h) [TEST]
- [os/milton/test_fileio_subdir.c:37](/home/tobiasosborne/Projects/InitechOS/os/milton/test_fileio_subdir.c:37) -> [spec/find_data.h](/home/tobiasosborne/Projects/InitechOS/spec/find_data.h) [TEST]
- [os/milton/test_flair_heap_ram.c:46](/home/tobiasosborne/Projects/InitechOS/os/milton/test_flair_heap_ram.c:46) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h) [TEST]
- [os/milton/test_fpu.c:42](/home/tobiasosborne/Projects/InitechOS/os/milton/test_fpu.c:42) -> [spec/fpu_contract.h](/home/tobiasosborne/Projects/InitechOS/spec/fpu_contract.h) [TEST]
- [os/milton/test_int21_edge.c:50](/home/tobiasosborne/Projects/InitechOS/os/milton/test_int21_edge.c:50) -> [spec/dos_structs.h](/home/tobiasosborne/Projects/InitechOS/spec/dos_structs.h) [TEST]
- [os/milton/test_int21_edge.c:51](/home/tobiasosborne/Projects/InitechOS/os/milton/test_int21_edge.c:51) -> [spec/find_data.h](/home/tobiasosborne/Projects/InitechOS/spec/find_data.h) [TEST]
- [os/milton/test_kbd.c:32](/home/tobiasosborne/Projects/InitechOS/os/milton/test_kbd.c:32) -> [spec/tsr_contract.h](/home/tobiasosborne/Projects/InitechOS/spec/tsr_contract.h) [TEST]
- [os/milton/test_loader.c:38](/home/tobiasosborne/Projects/InitechOS/os/milton/test_loader.c:38) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h) [TEST]
- [os/milton/test_loader_big.c:51](/home/tobiasosborne/Projects/InitechOS/os/milton/test_loader_big.c:51) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h) [TEST]
- [os/milton/test_mzload.c:57](/home/tobiasosborne/Projects/InitechOS/os/milton/test_mzload.c:57) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h) [TEST]
- [os/milton/test_overlay.c:47](/home/tobiasosborne/Projects/InitechOS/os/milton/test_overlay.c:47) -> [spec/dos_structs.h](/home/tobiasosborne/Projects/InitechOS/spec/dos_structs.h) [TEST]
- [os/milton/test_tenant_load.c:34](/home/tobiasosborne/Projects/InitechOS/os/milton/test_tenant_load.c:34) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h) [TEST]
- [os/milton/test_tenant_load.c:36](/home/tobiasosborne/Projects/InitechOS/os/milton/test_tenant_load.c:36) -> [spec/toolbox_gate.h](/home/tobiasosborne/Projects/InitechOS/spec/toolbox_gate.h) [TEST]
- [os/milton/test_tsr.c:38](/home/tobiasosborne/Projects/InitechOS/os/milton/test_tsr.c:38) -> [spec/tsr_contract.h](/home/tobiasosborne/Projects/InitechOS/spec/tsr_contract.h) [TEST]
- [os/milton/test_tsr_io.c:21](/home/tobiasosborne/Projects/InitechOS/os/milton/test_tsr_io.c:21) -> [spec/tsr_contract.h](/home/tobiasosborne/Projects/InitechOS/spec/tsr_contract.h) [TEST]
- [os/milton/xcopy_tree.c:22](/home/tobiasosborne/Projects/InitechOS/os/milton/xcopy_tree.c:22) -> [spec/find_data.h](/home/tobiasosborne/Projects/InitechOS/spec/find_data.h)

### os/samir → spec
- [os/samir/core/ovl.c:32](/home/tobiasosborne/Projects/InitechOS/os/samir/core/ovl.c:32) -> [spec/samir/ovl_format.h](/home/tobiasosborne/Projects/InitechOS/spec/samir/ovl_format.h)
- [os/samir/fs/dbf.c:88](/home/tobiasosborne/Projects/InitechOS/os/samir/fs/dbf.c:88) -> [spec/samir/dbf_format.h](/home/tobiasosborne/Projects/InitechOS/spec/samir/dbf_format.h)
- [os/samir/fs/ndx.c:147](/home/tobiasosborne/Projects/InitechOS/os/samir/fs/ndx.c:147) -> [spec/samir/ndx_format.h](/home/tobiasosborne/Projects/InitechOS/spec/samir/ndx_format.h)
- [os/samir/pal/pal_milton.c:98](/home/tobiasosborne/Projects/InitechOS/os/samir/pal/pal_milton.c:98) -> [spec/samir/memory_contract.h](/home/tobiasosborne/Projects/InitechOS/spec/samir/memory_contract.h)
- [os/samir/pal/pal_milton.c:99](/home/tobiasosborne/Projects/InitechOS/os/samir/pal/pal_milton.c:99) -> [spec/memory_map.h](/home/tobiasosborne/Projects/InitechOS/spec/memory_map.h)

## Appendix C. Reproduction artifacts

- [snapshot.txt](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/snapshot.txt)
- [make-n-image.txt](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/make-n-image.txt)
- [make-np-image.txt](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/make-np-image.txt)
- [object-sizes.txt](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/object-sizes.txt)
- [measurements.md](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/measurements.md)
- [refined-metrics.md](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/refined-metrics.md)
- [extra-metrics.txt](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/extra-metrics.txt)
- [measure.py](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/measure.py)
- [refine.py](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/refine.py)
- [extra.py](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/extra.py)
- [final-measure.py](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/final-measure.py)
