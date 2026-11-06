<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- CONTROLLED DOCUMENT -->
# DEC-03a -- Seat A: Extended Memory Is Available to Management

**Issuing Body:** Office of Enterprise Architecture, STAPLER Committee
**Date:** 2026-11-06 | **Bead:** initech-yndl2 | **Status:** ADVISORY; NOT RATIFIED
**Scope:** Period evidence and MILTON consequences; no implementation authority.
**Inspection:** worktree root verified with `git rev-parse --show-toplevel`;
baseline ca0c4f65, branch t3/initechmz-extended-memory. The separate
t3/tsr-exec-relocation tip cb714ad097481447792aa94d9fbf736f31e43875 was read with
`git show`; its changes are distinguished below from this checkout's code.
Sources below were inspected as repository text or acquired web text. Proposed
rules are recommendations, not claims that their implementation already exists.

## Findings

1. **The proposed product is period-plausible.** Fox Software's 1991-05-20 release explicitly offers FoxPro 2.0 Extended
   Mode for 386/486, protected-mode execution, 32-bit instructions and extended-memory operation. This is a direct
   business-database precedent, two years before the proposed product date. [P1]

2. **Phar Lap predates the objection's era boundary.** The contemporary Micro/Systems account records the
   386|DOS-Extender announcement in 1986 and an enhancement announcement in May 1987; BYTE January 1987 already
   describes RUN386 loading protected-mode applications. An exact November 1986 release date is not established here and
   is unnecessary to the conclusion. [P2], [P3]

3. **Rational and Lotus establish ordinary commercial adoption.** The 1990 first-hand DOS/16M programming walkthrough
   identifies Lotus 1-2-3 Release 3.0 as a customer; the 1989 BYTE account identifies its DOS/16M extended-memory
   execution. DOS/16M is a 16-bit protected-mode extender, not flat-32; Rational's DOS/4G is explicitly a 32-bit
   linear-addressing 386/486 product by 1991. [P4], [P5], [P6]

4. **The interfaces are period technology.** DPMI 1.0's own history dates public DPMI 0.9 to May 1990; an acquired 0.9
   copy is printed July 26, 1990. Its glossary dates VCPI's industry-standard status to 1989, following 1987
   development. Neither fact requires InitechDOS to implement those interfaces: the proposal borrows the memory model,
   not binary/API compatibility. [P7], [P8]

5. **Do not manufacture a dBASE precedent.** Ordinary dBASE IV 1.1/1.5 clients remain conventional-memory products: the
   1.5 product Q&A specifies 640K installed, about 450K available, with optional EMS/extended disk caching, including
   overlay caching improved over 1.1. It separately identifies a protected-mode Server Edition. Thus "needs 640K plus
   EMS/extended" is too strong, and caching does not establish high execution. The VMM/DBASE.VMC evidence belongs to IV
   2.0: the preserved Borland support reply discusses its DOS16M/virtual-memory configuration. Do not backdate that to
   1.1/1.5. [P9], [P10]

6. **Paradox is another qualified precedent.** The preserved Borland support Q&A directs Paradox 3.5 users to its
   protected-mode configuration and extended memory. Borland TI333D, question 10, identifies Paradox 4.0's DPMI DOS
   extender. Protected mode is established; these sources do not make either version a flat-32 product or justify
   calling every Paradox extender Phar Lap. [P11], [P12]

7. **Clipper is particularly relevant to the proposed compromise.** CA's August 18, 1993 ExoSpace announcement
   explicitly covers Clipper 5.2, extended memory and protected mode, while retaining automatic code/data overlays.
   Blinker requires a version qualification: the acquired vendor-reseller advertisement establishes a 286 extender in
   Blinker 3.0 by May 1994; an August 1993 contemporary exchange says it was not yet released. Its exact 1993 shipping
   date is unverified and is not used to carry this decision. [P13], [P14]

8. **WordPerfect and CAD reinforce the distinction between storage and mode.** WordPerfect 6.0 DOS (1993) documents
   extended memory and `/r` loading WP.FIL into extra memory; that alone is not proof of flat-32 high execution. AutoCAD
   386 was announced in 1989 with Phar Lap; Phar Lap's July 1990 vendor advertisement lists it among shipping
   applications. Announcement and shipment are deliberately not conflated. [P15], [P16], [P17]

9. **The fiction already buys extended RAM and a 486.** README's requirements say at least 4 MB above 1 MB, 5 MB total;
   hardware.json locks flat-32 and a dedicated 4 MiB FLAIR heap; ADR-0001 DEC-02 ratifies the 486 target. However,
   README understates today's actual minimum: memory_map.h derives INITECH_MIN_EXT_KB=5120 from KERNEL_CEIL=0x600000,
   and kmain enforces it, i.e. 6 MiB total address extent. Reconcile the discrepancy. [L1], [L2], [L3], [L21], [L22]

10. **"Above 1 MB" is not a vacant address.** The full locked map reserves FLAIR [0x100000,0x500000), kernel image/BSS
    [0x500000,0x580000), transient COMMAND [0x580000,0x5C0000), FLAIR.OVL [0x5C0000,0x5E0000), guard
    [0x5E0000,0x5F0000), and kernel stack [0x5F0000,0x600000). The program PSP is 0x40000, image 0x40100, image/arena
    ceiling 0x6F000, environment [0x6F000,0x70000), stack [0x70000,0x80000), staging [0x80000,0x90000).
    PROGRAM_IMAGE_MAX=0x2EF00=192256 B=187.75 KiB, before BSS and heap costs. [L2]

11. **Relocation arithmetic is ready; EXEC placement policy is not.** In this checkout loader_prepare_mz is in loader.c,
    not mz.c, and hardcodes PROGRAM_IMAGE/PROGRAM_ARENA_CEIL. mz.c:mz_apply_relocs already accepts any uint32 load_base.
    loader_prepare_tenant already relocates disk InitechMZ into a FLAIR heap block above 1 MiB. DEC-08a's 2026-10-20
    amendment expressly authorizes that tenant path, which does not bind a tenant DOS arena. [L4], [L5]

12. **The TSR branch is useful groundwork, not a high-loader implementation.** cb714ad adds loader_prepare_mz_at,
    relocates an MZ above retained residents, trims image/BSS ownership to expose heap, and reclaims on termination. Its
    explicit `arena_ceil > PROGRAM_ARENA_CEIL` rejection and retained PROGRAM_IMAGE_MAX/core checks still prohibit the
    proposed high placement. Its test fixture uses 0x50100, still below 1 MiB. [L6]

13. **The allocation ABI is the material obstacle.** mcb.h describes relative paragraphs and uint32 arena bookkeeping,
    but locked mcb_t.size_paras and owner are uint16; mcb_init casts a whole free run into that size field.
    int21.c:arena_seg_base/do_alloc/do_free/do_setblock truncate addresses to 16-bit fake segments; cur_psp_owner also
    truncates PSP>>4. pal_milton_make masks AX and shifts it left four. A block at 0x600010 would consequently become a
    pointer near 0x10. A single chain initialized over more than 65536 paragraphs also overflows its initial MCB. Moving
    only a constant would produce deterministic corruption, not an extended heap. [L7], [L8], [L9]

14. **The console-window premise conflicts with the governing ADR.** PRD 6.6 describes a console window, but ADR-0013
    Sec 3.2/3.7 explicitly rejects the DOS-box-in-a-window path: SAMIR is class 2, full-screen run-to-completion,
    desktop suspended and rebuilt on return. Native FlairProcess tenants have separate HANDLE records and GENERAL data
    allocations, reclaimed by teardown_common; they are event handlers, not threads. A windowed SAMIR front end needs
    its own ruling; high placement alone does not provide it. [L10], [L11]

## Recommendation

Approve DEC-03a in principle for a 1993 InitechBase product retaining III+ data and language compatibility. Adopt a
separate locked application region [0x600000,0x800000), above the entire kernel runway, and an 8 MiB machine
requirement. Gateway and Dell's September 1993 advertisements offer 8 MB 486 business configurations; this is a credible
configuration, not proof that all 1993 offices had it. [P18] Proposed manual copy: "InitechBase 3.30 uses the Initech
Extended Memory Manager. A 486-class PC with 8 MB RAM is required. Keep SAMIR.EXE and SAMIR.OVL together." This is an
Initech runtime description, not a claim to reproduce dBASE III+'s original execution mechanism. [P1], [L1], [L12]

## Proposed DEC-03a text

- **AM-1 -- Supersession.** Replace DEC-03's rejection of high application loading and its conventional-window packaging
  obligation. Retain SAMIR.OVL and its working manager as heritage packaging; retire further Phase-B overlay expansion,
  resident-shrink quotas and per-lane byte negotiations. Phase B already shipped; retiring its programme does not delete
  its working artifact or its corruption/refusal oracles. III+ semantics, soft-float and FLOW_MAX_REGISTRY=1 remain
  independent choices. [L12], [L13]

- **AM-2 -- Locked region.** Add EXT_APP_BASE=0x600000 and EXT_APP_CEIL=0x800000, containment and every cross-window
  disjointness proof to memory_map.h. Derive INITECH_MIN_EXT_KB from the highest required ceiling:
  (0x800000-0x100000)/1024=7168 KiB. Keep KERNEL_CEIL at 0x600000; moving it would also change runway zeroing. Update
  hardware.json, README, PRD and the boot diagnostic under Rule 8, with an implementing bead/worklog. [L1], [L2], [L14]

- **AM-3 -- Format and policy.** Extend DEC-08a with a distinct extended-model tag at 0x1C (proposed 0x5849; existing
  0x4943 stays legacy) and a versioned, length-checked header descriptor declaring BSS/overlay reserve, minimum heap and
  stack sizes as uint32 bytes. An old loader must reject the new tag rather than ignore a flag and silently select its
  old ABI. Content dispatch remains; flat COMs and legacy InitechMZ retain conventional placement; extended-tagged MZ
  uses the new region or refuses, with no silent conventional fallback. Parameterize the complete plan and FAT reader
  before the existing whole-file PROGRAM_IMAGE_MAX check; a large high image must never transit that window. Relocate
  with the actual base, bound entry/fixups and zero all reserved bytes. Reuse the existing transfer path and relocation
  primitive. [L4], [L5], [L6]

- **AM-4 -- Explicit native allocation contract.** Bind an extended arena to the active tagged process, after its module
  plus declared BSS/overlay reserve and below its stack guard. For this model propose AH=48h EBX=uint32 paragraphs,
  success EAX=flat byte pointer; AH=49h EBX=that pointer; AH=4Ah EBX=pointer, ECX=uint32 paragraphs. Retain CF/errors
  07h/08h/09h and report the largest available paragraph count in full EBX on shortage. Use separate uint32 native
  allocation metadata; do not cast a 2 MiB arena into legacy mcb_t. Retain the conventional MCB ABI for existing
  callers. Lock the new per-model register contract and PAL adaptation together; this is an ABI amendment. [L7], [L8], [L9]

- **AM-5 -- PSP, stack and lifetime.** Prefer a small conventional PSP and environment with a native sidecar holding
  high image/heap/stack bounds; its owner id remains representable. Preserve the 256-byte PSP's legacy fields as
  describing its low allocation, never as truncated high pointers. Allocate a high 64 KiB stack [0x7F0000,0x800000),
  with a proposed 4 KiB guard at [0x7EF000,0x7F0000); the native arena ends at 0x7EF000. Have samir_crt0 preserve
  loader-supplied ESP and make the PAL stack guard context-dependent. Save/restore allocator context across nested EXEC;
  replace loader_run_plan's inference that every noncanonical PSP already owns an MCB with an explicit allocation model.
  On every ordinary exit and failed launch reclaim native image, stack and heap, close JFT handles and restore parent
  PSP/DTA/vectors/arena; release the stack after returning to the parent's stack. KEEP remains a conventional TSR
  facility; reject it for extended-tagged apps explicitly. These are proposed semantics, not capabilities of today's
  PSP/loader. [L4], [L8], [L9], [L15]

- **AM-6 -- Location alternatives.** A bounded FLAIR GENERAL tenant allocation is already possible through
  loader_load_tenant; it must come from flair_alloc, carry a real budget and be freed once, without rebinding the master
  heap as a DOS arena. That consumes desktop capacity and requires class-2 ownership integration. A new region carved
  from FLAIR or the kernel guard needs other locked contracts changed; neither is free RAM. The selected new locked
  region above KERNEL_CEIL keeps those ownership domains disjoint. Placing an app merely above `_kernel_end` would
  overwrite shell/overlay/stack reservations. [L2], [L4], [L11]

- **AM-7 -- Scope boundary.** Flat COMs, EDLIN/CHKDSK, COMMAND.COM, conventional TSRs and their resident-preservation
  behavior keep their present contracts. SAMIR.OVL remains live and uses its actual relocated resident base. The kernel
  retains its separate ADR-0014 169984 B loaded-byte policy cap, 384-sector bounce ceiling, FLAIR.OVL and kernel size
  work; this amendment creates no kernel headroom. Added loader/allocator code must fit that policy. [L2], [L6], [L13], [L16]

## Oracle plan

- **Existing affected gates:** Makefile exposes test-mcb{,-mutant,-int21, -int21-mutant,-emu,-emu-mutant},
  test-mz{,-mutant}, test-mzload{,-mutant}, test-mzexec{,-mutant}, test-loader{,-mutant,-big,-big-mutant}, test-exec{,
  -unit,-mutant,-nested,-nested-mutant}, test-memory-map{,-mutant}, and test-hardware-spec{,-mutant}. Preserve their
  conventional expectations; add tagged-model cases. Include the TSR branch's test-tsr-exec and mutants after
  integration, and tenant-load/FLAIR launch/suspend-resume regressions. [L6], [L17]
- **SAMIR size gates:** Makefile includes samir_size.mk; test-samir-size pins a 93776 B resident budget, 4096 B floor,
  minalloc and largest overlay slot. samir.ld separately asserts the conventional PAL budget and 64 KiB reserve. Replace
  the obsolete conventional-resident constraint with independent high footprint/declared-heap/stack bounds, not a
  disabled test. Retain slot/stamp, malformed-file, reproducibility and test-samir-size-emu behavior checks. [L13], [L18]
- **RED first:** run a hand-authored nonzero-fixup MZ larger than 192256 B at a specified high base; marker and checksum
  must require the relocated absolute reference. Allocate/write/free/resize above 1 MiB through real AH=48h/49h/4Ah;
  assert pointer high bits, owner isolation, >1 MiB allocation and exhaustion. Dirty RAM before launch; prove BSS zero,
  stack canaries and no FLAIR/kernel writes. Exercise failed reads/headers and exit via 4Ch/00h/INT20; repeated launches
  must restore free capacity and preserve parent/TSR bytes. [L4], [L7], [L8]
- **Acceptance:** independent address/byte expectations; wrong-base, pointer truncation, overlap and leaked-owner
  mutants must go RED. QEMU AND Bochs run the high image, extended allocation and reclaim sequence at exactly 8 MiB;
  insufficient-RAM gates exercise 7167 versus 7168 KiB reported extended RAM. Retain real III+ transcript/DBF gates,
  kernel-headroom and reproducibility. Record desktop launch/return per Rule 14; 86Box acceptance remains explicit
  pending wiring, never inferred from a QEMU pass. No execution gate was run for this documentation-only research
  memorandum. [L14], [L16], [L17], [L19]

## Risks

- **BIOS/A20:** no new real-mode transition is needed. Current stage2 delegates BIOS setup to IO.SYS; IO.SYS completes
  BIOS work, enables A20 through port 0x92, sets CR0.PE and never returns for runtime services. This confirms the
  proposed path is native protected mode; it does not certify every 486 board's A20 implementation. Test a high/low
  alias pattern on the period leg. [L14]
- **RAM evidence:** IO.SYS E820 sums usable ranges, with E801/AH=88h fallbacks; a sufficient sum alone does not prove
  contiguous RAM at 6-8 MiB. Lock the supported contiguous-PC assumption or verify interval coverage before writes.
  Harness qemu.c:build_argv passes no `-m` (documented QEMU default 128 MiB);
  bochs.c:write_bochsrc specifies `megs: 32`. These defaults hide minimum RAM bugs;
  kmain's comment claiming both default >=128 MiB is stale. [L14], [L20], [P19]
- **Capacity/cooperation:** no paging or protection is introduced. Validate complete footprint and overflow before
  committing a load. Nested conventional EXEC, TSR registry state and overlay pointers must survive arena switches. No
  86Box driver exists in harness/emu; require a recorded 486/8 MiB/period-VGA run when available, including BIOS
  fallback and A20 checks. [L2], [L8], [L19]

## Open questions for the chair

1. Ratify 8 MiB globally as drafted, or retain 6 MiB OS boot and require 8 MiB only when launching extended
   applications? The latter needs a separate load gate. [L2]
2. Accept the low PSP plus tagged full-width allocation ABI, or commission a broader PSP/MCB extension? A high PSP
   cannot preserve today's fake-segment semantics. [L7], [L15]
3. Confirm the ADR-0013 class-2 desktop suspend/return contract, or separately amend it for the PRD's console-window
   aspiration; do not silently conflate them. [L10]
4. Authorize follow-up Rule-8/worklog and tracker reconciliation. `bd show initech-yndl2` reported no matching issue
   here; no bead was created or changed.

[P1]: https://landley.net/history/mirror/collate/foxpro/pres_rel_foxpro20.htm "Fox Software, 1991-05-20, Extended Mode press release"
[P2]: https://bitsavers.computerhistory.org/magazines/Microsystems_Journal/v04n02.pdf "Micro/Systems Journal, February 1988, p.66, contemporary extender account"
[P3]: https://www.worldradiohistory.com/Archive-Byte/80s/Byte-1987-01.pdf "BYTE January 1987, p.29, Phar Lap announcement"
[P4]: https://dokumen.pub/extending-dos-0201550539-9780201550535.html "Extending DOS, first printing April 1990, Ch.4 DOS/16M walkthrough"
[P5]: https://www.worldradiohistory.com/Archive-Byte/80s/Byte-1989-11-IBM.pdf "BYTE IBM Special Edition Fall 1989, p.63, Lotus Release 3"
[P6]: https://jacobfilipp.com/DrDobbs/articles/DDJ/1991/9110/9110t/9110t.htm "October 1991 vendor product announcements: Rational DOS/4G and DOS/16M"
[P7]: https://docs.pcjs.org/specs/dpmi/1991_03_12-DPMI_Spec_v10.pdf "DPMI Committee v1.0, introduction pp.4-5 and glossary VCPI"
[P8]: https://www.phatcode.net/res/262/files/dpmi09.txt "DPMI Committee v0.9, printed July 26 1990"
[P9]: https://www.oocities.org/geoff_wass/dBASE/GaryWhite/dBASE/FAQ/qdb415.htm "Preserved dBASE IV 1.5 product Q&A, Installation and Memory"
[P10]: https://www.oocities.org/geoff_wass/dBASE/GaryWhite/dBASE/FAQ/qwfwg.htm "Preserved Borland support reply, Paul Powers, dBASE IV 2.0 memory configuration"
[P11]: https://groups.google.com/g/comp.databases.paradox/c/0uHEJ1tciQ0 "Preserved Borland Paradox support Q&A, questions 3 and 9; archive provenance weaker than a scanned manual"
[P12]: https://www.jamesmsandbrook.co.nz/DPSC/15333.html "Borland Technical Information Database TI333D, question 10"
[P13]: https://oldsezam.net/Conference/PCPROG.3/clipper?p=2 "CA press release 1993-08-18, message 153; Blinker exchange, message 152"
[P14]: https://www.worldradiohistory.com/Archive-Byte/90s/1994/Byte-1994-05.pdf "Programmer's Paradise advertisement for Blink Inc. Blinker 3.0"
[P15]: https://mendelson.org/wpdos/WPDOS60Reference.pdf "WordPerfect 6.0 DOS Reference, Questions and Answers pp.22-23, Memory/Speed"
[P16]: https://archive.computerhistory.org/resources/access/text/2023/11/102803713-05-005-acc.pdf "The Computer Paper July 1989, Autodesk announcement dated June 6"
[P17]: https://www.worldradiohistory.com/Archive-Byte/90s/1990/Byte-1990-07.pdf "Phar Lap vendor advertisement: AutoCAD 386 among shipping applications"
[P18]: https://device.report/m/6816389a5c36f4af6762ceacea3852a9a25f1b30a0363a3ac5702961524e485a "BYTE September 1993, Gateway opening spread and Dell business-lease advertisements"
[P19]: https://qemu.readthedocs.io/en/master/system/invocation.html "QEMU invocation manual, -m default 128 MiB"
[L1]: ../../../README.md "System requirements"
[L2]: ../../../spec/memory_map.h "Entire header; all windows, arena invariant, INITECH_MIN_EXT_KB"
[L3]: ../../../os/milton/kmain.c "kernel_main extended-RAM boot gate"
[L4]: ../../../os/milton/loader.c "loader_prepare_core, loader_prepare_mz, loader_prepare_tenant, loader_run_plan, load_program_from_fat"
[L5]: ../ADR-0003-AMENDMENT-DEC-08a-MZ-EXE-Flat-Loader.md "DEC-08a.1-.5 and 2026-10-20 placement amendment; os/milton/mz.c:mz_apply_relocs"
[L6]: ../../../os/milton/loader.c "Separate branch evidence: git show cb714ad097481447792aa94d9fbf736f31e43875 -- os/milton/loader.c os/milton/int21.c os/milton/test_mzload.c harness/emu/tsr_exec.mk"
[L7]: ../../../os/milton/mcb.h "Also mcb.c:mcb_init and spec/dos_structs.h:mcb_t/psp_t"
[L8]: ../../../os/milton/int21.c "arena_seg_base, cur_psp_owner, do_alloc/free/setblock/terminate, tsr_keep, int21_mcb_bind_program"
[L9]: ../../../os/samir/pal/pal_milton.c "pal_milton_make AH=48h conversion; stack guards; environment lookup"
[L10]: ../ADR-0013-FLAIR-App-Contract.md "Sec 3.2 class 2, Sec 3.7, O-7; contrast InitechOS-PRD.md Sec 6.6"
[L11]: ../../../os/flair/process.c "FlairProcess_launch/admit, teardown_common, terminate/kill; AC-2 split arenas"
[L12]: ../ADR-0009-SAMIR-Milton-Integration-Numeric-Memory-Packaging.md "DEC-01, DEC-03, DEC-04"
[L13]: ../../design/samir-overlays.md "Phase B shipped; Sec 1 historical 5575 B headroom; os/samir/core/ovl.c uses __samir_base"
[L14]: ../../../os/boot/io_sys.asm "A20 enable, pm_entry, mem_probe; stage2.asm delegates; kstart.asm runway zeroing"
[L15]: ../../../os/milton/psp.c "flat_to_fake_paragraph masks to uint16; spec/dos_structs.h PSP; os/samir/boot/samir_crt0.asm hardcodes ESP"
[L16]: ../ADR-0014-Kernel-Overlays.md "DEC-14-9 loaded-byte cap; Makefile KERNEL_SECTORS and test-kernel-headroom"
[L17]: ../../../Makefile "Named test families; harness/proptest/test_memory_map.c; harness/diff/dbf_diff/test_hardware_spec.c"
[L18]: ../../../harness/diff/dbf_diff/samir_size.mk "test-samir-size family; test_samir_size.c; os/samir/boot/samir.ld ASSERTs"
[L19]: ../../../CLAUDE.md "Laws 1-4, Rules 1/5/6/8/14; 86Box driver outstanding, initech-x0i; PRD Sec 8"
[L20]: ../../../harness/emu/bochs.c "write_bochsrc megs:32; compare harness/emu/qemu.c:build_argv (no -m)"
[L21]: ../../../spec/hardware.json "cpu.value and memory model/windows; executable-format row contains stale MZ-deferred wording"
[L22]: ../ADR-0001-Hardware-Target-386-Origin-486-Floor.md "DEC-02 ratifies the 486-class target"
