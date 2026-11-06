<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0009 Amendment DEC-03a -- InitechMZ Extended Memory and InitechBase Programme Re-point

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** Architecture Decision Record Amendment (ADR-A)
**Programme:** STAPLER (InitechOS Platform Modernization & Heritage Compatibility Initiative)

## Document Control

| Field | Value |
|---|---|
| Document ID | OEA-ADR-0009-A2 |
| Version | 0.9 |
| Status | **DRAFT v0.9 -- pending operator ratification** |
| Date | 2026-11-06 |
| Classification | Internal Use Only |
| Document Owner | Office of Enterprise Architecture |
| Primary Author | Architecture Committee Chair, design lane |
| Ruling served | Operator, 2026-11-06: "good, do it" -- permit high InitechMZ image/heap; retire the conventional-window negotiation, packaging B and resident-shrink programme; re-grade W4; redirect capacity to Turbo Initech and FLAIR. |
| Authority | The operator RATIFIES. The chair recommends this text; neither the instruction to draft nor this commit constitutes ratification of its detailed contracts. |
| Supersedes | On ratification, ADR-0009 DEC-03's rejection of loading above 1 MB and its mandatory conventional-window MZ/overlay end-state; the dependent SAMIR resident-headroom policy and W4 blanket byte-exact screen acceptance. DEC-04 disjointness is extended, not waived. |
| Related Documents | ADR-0001 DEC-02; ADR-0003 DEC-13 and Amendments DEC-04a, DEC-08a, DEC-19; ADR-0013 Sec 3.2/3.7 and O-7; ADR-0014 DEC-14-9; PRD Sec 5/6.6/7/8/11; INITECHBASE-ship-plan Sec 7. |
| Related Issues | initech-yndl2; initech-5etom; initech-1x6x2.18; initech-9x8cn; initech-j7lld; initech-y29t4; initech-p3nn0; initech-h4bk; initech-e7qc; initech-x0i. Dispositions take effect after ratification. |
| Evidence baseline | Today's main, 797d87c5cfdd4495bd25a02893cfd572b11a7fa4. Both advisory memos copied unchanged into committee/. Source inspection only; no implementation or execution certificate is claimed. |
| Retention | 7 years following decommission, per RECORDS-SCHED-014. |

| Committee seat | Advice | Chair disposition |
|---|---|---|
| A -- period/platform | Approve in principle; separate 6-8 MiB region, explicit allocation ABI, 8 MiB machine. | Adopt, including distinct tag and low PSP; settle its optional global-vs-launch RAM fork globally. |
| B -- SAMIR/oracles/pubs | Approve; MZ-only placement, frozen flags, retarget gates, preserve overlays, class-2 text tenancy, W4 L1/L2/L3. | Adopt; set its proposed ceiling to 256 KiB and define the reserve/header consistency check below. |
| Chair | Recommend the resolutions in Sec 2. | DRAFT; operator sign-off outstanding. |

## 1. Context

DEC-03 was a decision about the execution mechanism of a III+ 1.1-era clone.
The present product is a period-plausible 1993 system on a 486. Its language,
file formats and real-III+ behavior remain the compatibility target; its
runtime need not reproduce III+'s conventional-memory packaging limitation.
FoxPro 2.0 Extended Mode (1991), Rational DOS/4G (1991), and CA's ExoSpace
announcement (1993) establish protected/extended-memory business software
before this product's date. ExoSpace also retained overlays. [P1] [P6] [P13]

Seat A's evidence is qualified deliberately: Phar Lap was contemporary by
1986/1987; Lotus Release 3 used DOS/16M, which is not flat-32; DPMI 0.9 was
public in May 1990 and VCPI was an industry standard by 1989. Ordinary dBASE
IV 1.1/1.5 conventional clients and their extra-memory caches are not high
execution precedents; IV 2.0's VMM must not be backdated. Paradox establishes
protected mode without proving every version flat-32 or Phar Lap. Blinker
3.0's exact 1993 shipment is unverified; WordPerfect's extra-memory storage
does not prove flat-32 execution. AutoCAD 386's 1989 announcement and its
appearance in a July 1990 shipping-products advertisement are distinct
evidence. None of this grants DPMI, VCPI, XMS or genuine DOS-binary
compatibility to InitechDOS. [P2] [P3] [P4] [P5] [P7] [P8] [P9] [P10]
[P11] [P12] [P14] [P15] [P16] [P17]

The current conventional image bound is 192,256 B (187.75 KiB),
`PROGRAM_IMAGE=0x40100` to `ENV_BLOCK=0x6F000`, before reserve and heap.
SAMIR's current 93,776 B resident budget and 4,096 B headroom floor are
derived from that placement. Phase B already delivered SAMIR.EXE/SAMIR.OVL;
its remaining programme is being retired, not its working artifact.
The six overlays remain file, report, status, set, memfile and forms.
New W4 code may call `samir_do`; today's non-reentrant overlay manager
forbids that from an overlay, independently of available RAM. [A] [B] [L18]

"Above 1 MB" is already occupied: FLAIR owns 1-5 MiB and the whole kernel
runway owns 5-6 MiB. Moving only PROGRAM_IMAGE would additionally truncate
AH=48h pointers to fake 16-bit segments and overflow a legacy MCB size for a
2 MiB chain. A placement constant alone is not this amendment. [L2] [L7] [L8]

Both seats agree on the region and isolation of ownership domains. Their
remaining differences are degrees of specification, not opposing votes:
Seat A supplies an ABI and asks whether the RAM floor is global; Seat B
supplies acceptance classes and leaves the ceiling and reserve limits to
the chair. Sec 2 resolves each; no seat recommends a FLAIR-heap carve-out.

**Baseline correction:** the seats cite 167,360 B / 2,624 B kernel margin.
Today's main has merged initech-h4bk (797d87c5), whose commit record reports
168,764 B largest production kernel / **1,220 B remaining** under 169,984 B;
the flagship is 167,712 B. These are recorded measurements, not a fresh
build by this chair. The historical roughly 2.6 KB allowance is already
partly spent. DEC-03a must fit the actual remaining allowance. [M]

## 2. Decision -- proposed for operator ratification

### DEC-03a.1 -- Supersession and the period claim

Permit an explicitly selected InitechMZ application model whose image,
reserve, heap and stack are above 1 MiB. Retire the 187.75 KiB SAMIR window
negotiation, further packaging step B (initech-5etom), resident-shrink rounds
and per-lane byte allowances. Retain shipped SAMIR.OVL and its manager as
period flavor and supported packaging. The honest claim is a 1993 Initech
runtime implementing III+ 1.1 behavior, not III+ 1.1's original runtime.
SAMIR soft-float, FLOW_MAX_REGISTRY=1, target FLOW_MEMVAR_ARENA=6000,
the PAL boundary and locked III+ semantics remain. The 6000-byte pool is
observable in LIST MEMORY; additional physical capacity does not enlarge it.

### DEC-03a.2 -- A locked application region with one owner

Reserve `EXT_APP_BASE=0x00600000`, `EXT_APP_CEIL=0x00800000`, exclusive:
**[0x600000,0x800000), 2 MiB**. The MILTON EXEC loader owns the reservation
and lends it to one live extended-model process. It is never a FLAIR GENERAL
allocation, never a kernel growth allowance, and never a legacy MCB chain.
The locked map shall prove containment and disjointness against every named
conventional window, FLAIR heap, kernel image, transient shell, FLAIR.OVL,
kernel guard and kernel stack. Keep KERNEL_CEIL at 0x600000 and its runway
zeroing bound unchanged. [L2]

| High-region component | Proposed placement / rule |
|---|---|
| Module | Starts at 0x600000; relocation factor is that actual flat base; entry is base + validated module-relative entry offset. |
| BSS / overlay slot / PAL tail | Immediately after the module, within the header-declared reserve; paragraph rounding checked for overflow. |
| Native heap | Begins after the rounded module + reserve, including its allocator bookkeeping; ends at or below 0x7EF000. |
| Stack guard | [0x7EF000,0x7F0000), 4 KiB of checked canaries; no paging protection is claimed. |
| Program stack | [0x7F0000,0x800000), 64 KiB, downward; initial ESP=0x7FFFFC. |

The entire load plan, declared minimum heap and metadata must fit before
launch. This is a fixed window, not all detected free RAM. A second high
EXEC while this owner is live returns DOS 0008h without modifying it.
Conventional children may still run through the existing nested EXEC path;
their low allocations cannot reuse the parent's PSP/environment or residents.

### DEC-03a.3 -- Content dispatch and an extended-model tag

Keep content dispatch (MZ/ZM, never filename extension). Adopt Seat A's
distinct word **0x5849 at header offset 0x1C** for the extended model;
0x4943 retains the legacy InitechMZ allocation/placement contract, including
today's relocation above conventional TSRs. SAMIR.EXE is re-emitted with
0x5849. No name-based SAMIR exception and no fallback to conventional RAM.
An old loader must refuse the new tag, never silently run the new ABI low.
Untagged genuine 16-bit MZ retains the foreign-MZ panic. [L4] [L5] [M]

For a reviewable v1 contract, propose a 24-byte little-endian descriptor at
0x20: six uint32 fields `version=1`, `length=24`, `reserve_bytes`,
`minimum_heap_bytes`, `stack_bytes=65536`, `guard_bytes=4096`.
`reserve_bytes` includes the overlay slot, BSS and PAL tail; SAMIR initially
declares 65536. Minimum heap is a usable allocation guarantee, net of native
metadata. The MZ header must cover this descriptor; its relocation table
must not overlap the base header/descriptor or module. Reject unknown
versions, bad lengths, unsupported stack sizes, overflow, malformed extents
and inconsistent minimums before transfer. e_minalloc remains an additional
minimum-arena pin: SAMIR keeps **2058 paragraphs**; both that requirement and
the descriptor's usable-heap requirement must be enforced. In this model
e_maxalloc=0xFFFF means all available native arena capacity (not a 65535-
paragraph limit); a finite e_maxalloc caps arena bytes and cannot undercut
either minimum. The fixed high ceiling always bounds it; it never moves
the stack. This sentinel rule permits allocations larger than 1 MiB.

Dispatch and header preflight must precede the current whole-file
PROGRAM_IMAGE_MAX check and FAT destination selection. Stream the high
module directly to its validated high destination; do not first read a
large file through 0x40100, the 64 KiB staging area or a new module-sized
kernel buffer. Reuse `mz_apply_relocs` and the single `loader_run_plan`
transfer. Parameterize the plan's model, image, reserve, arena, stack and
ownership; today's `loader_prepare_mz_at` still rejects a ceiling above
PROGRAM_ARENA_CEIL and is only conventional groundwork. [L4] [L6] [M]

Flat .COMs, EDLIN, CHKDSK, COMMAND.COM, conventional TSRs, legacy MZ images
and AC-3 native GUI disk tenants retain their existing contracts. AC-3
FLAIR tenant loading does not acquire this DOS arena; an extended EXEC
image is not silently admitted through the GUI-tenant loader.

### DEC-03a.4 -- Full-width allocation; a low PSP and explicit process model

Adopt Seat A's per-model native ABI and separate uint32 allocation metadata.
Do not widen or reinterpret the locked legacy `mcb_t`. A high block address
must never pass through `(AX & 0xFFFF) << 4`. [L7] [L8] [L9]

| Service, extended model only | Inputs | Result |
|---|---|---|
| AH=48h | EBX = uint32 paragraph count | CF clear, EAX = flat byte pointer to usable data. |
| AH=49h | EBX = exact flat pointer returned by 48h | Free only a live block owned by the current process; CF clear on success. |
| AH=4Ah | EBX = flat block pointer, ECX = uint32 new paragraph count | Resize in place, preserving old bytes; CF clear on success. |
| Failure | All three | CF set, AX=0007h corrupt metadata, 0008h insufficient memory, or 0009h invalid block/owner as applicable; no mutation on a returning refusal. On 48h/4Ah shortage, full EBX reports largest usable size in paragraphs. |

Checked paragraph-to-byte arithmetic, zero/oversize requests, exact block
identity, foreign owners, and double frees are part of the locked contract.
Legacy callers keep BX/CX counts and fake-segment results. Model selection
comes from the EXEC context, not from the address passed to a syscall.

Allocate a small PSP and environment below 1 MiB, disjoint from retained
residents and a live conventional parent, with representable owner and
environment segments. Keep the 256-byte PSP layout: legacy alloc-end fields
describe only its low control allocation, never truncated high pointers.
Entry EBX still identifies that low PSP. A native sidecar holds full-width
image/reserve/heap/guard/stack bounds and explicit allocation model. Preserve
the conventional chain for low control blocks and nested conventional EXEC;
do not bind the high region into it. The low-control allocation must release
unused low capacity for children rather than consume the largest low run.

Lock the generic tag/descriptor in proposed `spec/initechmz_extended.h`,
SAMIR ceiling/reserve/minalloc in proposed `spec/samir/memory_contract.h`,
and allocation ABI in `spec/int21h_calling_convention.json`, alongside the
map in memory_map.h. The emitter, loader/PAL and gates consume the appropriate
shared contract. Deliberately reconcile the current tag in os/milton/mz.h
with that authority together with the PAL adaptation; no duplicated,
independently editable ABI constants. Linker bounds must consume/check the
locked values. Independent oracle vectors still use literal expectations.
The ordinary EDX/ECX user-buffer paths retain full-width flat addresses.
Extend DEC-19 ownership validation to the high owner's image/reserve and
individual native heap blocks; neither PSP+0x100 nor a fake-segment walk
describes them. Refuse guard, stack, free/foreign blocks, kernel and FLAIR
destinations. Overlay relocation arithmetic and the service's errors remain.

### DEC-03a.5 -- Stack, zeroing, teardown and desktop lifetime

`samir_crt0` shall preserve loader-supplied ESP; the PAL guard shall use the
bound process stack/guard bounds. The low PROGRAM_STACK_* constants remain
the conventional model's contract. Before transfer, zero the high reserve,
free heap and stack, then install canaries; metadata is initialized
deterministically. crt0's BSS-zeroing remains as an independent guarantee.
High RAM is arbitrary at cold boot; `kstart` clears only the kernel runway.

Save and restore the allocation model, native owner and conventional arena
context across nested EXEC. Replace the `PSP != PROGRAM_BASE` inference
that a plan already owns a conventional MCB. Failed launches, AH=4Ch,
AH=00h and INT 20h must close child JFT references, reclaim low controls
and all high resources, and restore parent PSP, DTA, vectors, allocator and
stack context exactly once. Reclaim/clear a child's stack only after transfer
back to the parent stack. Extended-model AH=31h/INT 27h KEEP is explicitly
refused with 0001h, leaving the process live and unchanged; conventional
TSR behavior and byte preservation remain. [L8] [L15]

SAMIR remains an ADR-0013 **class-2 full-console text tenant**: suspend the
desktop, run to completion, re-present its preserved offscreen on return.
The offscreen and native tenants stay live in [0x100000,0x500000).
Replace O-7's old "below 1 MiB" reasoning with locked disjointness; retain
its bit-identical desktop oracle. No FLAIR heap reset or tenant relaunch may
substitute for preservation. Re-prove prompt/hotkey/relaunch, failure return,
Exit to InitechDOS (transient at 0x580000), and later prompt launch. Restart
and Shut Down remain cold paths. A windowed SAMIR front-end remains the
separate initech-e7qc decision. [L10] [L11] [B]

### DEC-03a.6 -- One 8 MiB machine contract and a loud refusal

Choose Seat A's **global 8 MiB minimum**, rather than a 6 MiB boot mode
with an 8 MiB application gate. Derive `INITECH_MIN_EXT_KB` from the highest
required ceiling: `(EXT_APP_CEIL - 0x100000) / 1024 = 7168` KiB above 1 MiB.
Do not change FLAIR_HEAP_MIN to accomplish this. Gateway and Dell offered
8 MB 486 business configurations in September 1993; this supports a plausible
purchase, not a claim that every office already owned one. [P18]

The supported machine has contiguous usable PC RAM through 0x800000.
Ratification accepts this explicit hardware assumption for E801/AH=88h
fallbacks; an E820 map with a known hole in any required region must be
refused. A sum of disjoint usable ranges is not interval coverage. Record
that check in the boot/memory implementation, not a hopeful load-time write.

A **5 MB machine** (1 MB + 4096 KiB extended), the old 6 MiB floor and any
report below 7168 KiB must fail before high writes or desktop admission.
Use the existing catalogue text **`Insufficient memory`**, MSG-DOS-0035,
under ADR-0003 **DEC-13 / Appendix C** and `spec/dos_messages.json`.
Keep the canonical PC LOAD LETTER panic presentation and serial required/
reported values, but add no unapproved user-facing paraphrase. Today's
`not enough memory for kernel/FLAIR` suffix is pre-existing boot-message
drift for the implementation lane to reconcile, not the proposed wording.

Update `spec/memory_map.h`, `spec/hardware.json`, README requirements, PRD
reconciliation and the manuals in lockstep, with implementing bead/worklog
and deliberate Rule-8 provenance. Correct both obsolete 4 MB extended
copy and the actual existing 5120 KiB gate. Reconcile hardware.json's stale
MZ-deferred and heap-only RAM-gate descriptions as part of that same act.

In `harness/emu`, make RAM explicit and configurable: QEMU **`-m 8M`**
and Bochs **`megs: 8`** for minimum-profile positive legs; provide negative
5 MiB / just-below-floor cases and retain larger profiles where useful.
Bochs uses a config setting, not a QEMU `-m` flag. Defaults today are no
QEMU -m and Bochs megs:32; neither proves the proposed floor. Audit run,
record and test call sites so a hidden larger default cannot mask a failure.
Use injected reported-memory cases for the exact 7167/7168 KiB boundary.
[L14] [L20] [P19]

### DEC-03a.7 -- The kernel capacity account is separate

Keep ADR-0014's **169,984 B loaded-byte policy cap**, the **384-sector /
196,608 B bounce ceiling**, all kernel headroom gates, FLAIR.OVL and its
runway, and the kernel overlay programme unchanged. The high application
region creates no kernel bytes. The loader/allocator work must fit within
the seats' roughly 2.6 KB margin and, on today's main, the tighter **1,220 B**
remaining margin; measure every production kernel after integration.
If it cannot, land independently gated kernel capacity work under the
existing policy or seek a separate operator ruling. This draft does not
authorize lifting the cap or borrowing the kernel guard. initech-h4bk is
already merged, not an outstanding lane to retire. [L16] [M]

### DEC-03a.8 -- SAMIR integration and every size-gate disposition

Freeze `SAMIR_SIZE_CFLAGS`, including -Oz, ABI flags, LTO and their crt0/PAL
agreement. No further shrink rounds; an -O2 change requires its own bead
and reproducibility/ABI evidence. New code is resident by default. A truly
cold leaf command may still use an existing supported overlay, with all
static/reentry gates; overlays are not forbidden. No nested-swap expansion
is required. Keep current SAMIR.OVL slot, pairing/stamp, CRC and refusal
behavior; high relocation uses the actual `__samir_base`. [B] [L13]

Set the independent SAMIR resident **runaway ceiling to 262144 B (256 KiB)**
of load module, excluding MZ header/relocations. This is not a feature-lane
budget or a replacement 4 KiB headroom quota. Report footprint/headroom as
information, enforce physical containment and declared heap independently.
Raising this ceiling later requires a recorded Rule-8 decision.

The following is the complete retirement ledger for the seats' SAMIR
size/headroom surface; a name containing `size` is not retirement authority.

| Gate / check | Disposition | Reason and replacement obligation |
|---|---|---|
| `test-samir-size`: resident <= BUDGET-4096 | RETARGET | Fail only above 262144 B module; report headroom; independently validate high footprint and heap declaration. |
| `BUDGET != 93776` exit 2 | RETIRE | Pins the retired conventional equation. No arbitrary 93776 B contract survives. |
| 4096 B floor / per-lane byte negotiation / 98175 B historical reclaim quota | RETIRE | Conventional squeeze retired; the last quota was already removed in WL-0182 (initech-9swot). Do not resurrect it. |
| SLOT exactly largest overlay rounded to 16; each fits | KEEP | Format correctness; unchanged slot policy. |
| InitechMZ tag / e_minalloc=2058 pin | KEEP, adapt tag | Require extended 0x5849 + valid v1 descriptor; preserve 2058 and enforce its declared minimum under the new model. |
| `test-samir-size-mutant` PAD floor-1/floor pair | RETARGET | Module ceiling+1 RED, ceiling GREEN; an image above the old 192256 B bound must pass if its full footprint fits. |
| Same target's SLOT mutant | KEEP | An oversized/nonminimal slot still goes RED. |
| `test-samir-size-repro` | KEEP | Two clean SAMIR.EXE and SAMIR.OVL builds remain byte-identical, including pairing. |
| `test-samir-size-emu`, `-emu-mutant` | KEEP | Actual boot/write/canon-y2k behavior aliases; not size policy. |
| `test-samir-messages`, `-mutant`, `-emu`, `-emu-mutant` | KEEP | Catalogue pairing and real on-target errors; keep MSG_PAIR mutants. |
| `test-samir-memwidth`, `-mutant`, `-emu`, `-emu-mutant`; `samir-size-goldens` | KEEP | Real-III+ column widths/captures and MEMVAR_WIDTH mutants. Never retire by spelling. |
| samir.ld reserve ASSERT (currently 64 KiB) | RETARGET | Slot + BSS + PAL tail <= declared reserve; reserve and whole footprint <= high window below guard. Initially reserve stays 65536. Increasing a link bound without increasing the loader declaration is forbidden. |
| samir.ld module+0x10000+0x8090+0x20 <= 0x6F000-0x40100 | RETIRE old equation; RETARGET ASSERT | Replace with module+declared reserve+native metadata+guaranteed PAL heap <= EXT_APP guard base minus image base. No literal low-window arithmetic. |
| samir.ld slot placement/nonempty ASSERTs, NOCROSSREFS/NOCROSSREFS_TO; packer forbid/call-graph | KEEP | Slot/relocation/reentry correctness. |
| `test-samir-ovl`, `-mutant`, `-static`, `-static-mutant`, `-emu`, `-emu-mutant`, `-swap-emu`, `-swap-emu-mutant`, `-guard-emu-mutant`, `-lifetime`, `-output`; `test-samir-mzlink-io` | KEEP all | Shipped overlay manager, stamps/CRC, corruption, reentry, I/O failure and pointer-lifetime safety remain supported. |
| `SAMIR_SIZE_CFLAGS` | KEEP frozen | Size programme ends without casually changing the calling convention. |
| `test-flair-samir-suspend`, `-mutant`, `-bochs` | KEEP, re-prove | Desktop bit-identity must hold with a high image/heap/stack. |
| `test-samir-lifetime-emu`, `-emu-mutant`, `-desktop`, `-desktop-mutant` | KEEP, re-prove | Repeated prompt/hotkey exits and relaunches must reclaim the new model. |
| `test-arena-disjoint`, `-mutant` | KEEP, extend | Add high image/reserve/heap/guard/stack; overlap mutant must fail. |
| `test-memory-map`, `-mutant`; `test-hardware-spec`, `-mutant` | KEEP, strengthen | New region, all window pairs, hardware row, exact RAM floor, descriptor and ceiling contract. |
| Kernel size/headroom families, including `test-kernel-headroom` and `test-kernel-size-r4` | KEEP | Independent ADR-0014 policy, no SAMIR waiver. |
| `test-samir-boot`, `-write`, `-canon-*`, `-dml-*`, `-fns-*`, `-core-emu`, `-report-emu` and their mutants/goldens | KEEP, re-run | Language/file/transcript behavior is independent of load address. |
| `test-merge-samir` membership | KEEP, audit | Retarget existing entries in place; new acceptance gates enter the vector, Bochs still per-session; no alias is silently dropped. |

### DEC-03a.9 -- Publications and the product description

Adopt Seat A's manual copy:

> InitechBase 3.30 uses the Initech Extended Memory Manager. A 486-class PC
> with 8 MB RAM is required. Keep SAMIR.EXE and SAMIR.OVL together.

This names Initech's native runtime, not a claimed DPMI/XMS interface.
Keep the installation overlay explanation/disk prompt, environment memory
sentence and message 43 unless behavior itself changes. Correct requirements
and their Law-1 comments in the InitechDOS User's Guide ch1, FLAIR Getting
Started ch2/ch5, and FLAIR User's Guide appendix B. [B]

CHKDSK is flat .COM: its MCB memory figures stay unchanged. Preserve
`initechdos-reference/screens/chkdsk.txt`, `chkfile.txt` and
`initechdos-users-guide/screens/chkdsk.txt` byte-exact. Re-mint
`initechbase-volume-one/screens/gs-dir.txt`: its free count is DISK space,
which changes with SAMIR.EXE/SAMIR.OVL bytes, not with installed RAM.
Run `test-pubs-screens`; retain `test-pubs-lockstep-src`, `test-pubs-tps`,
`test-pubs-examples` and per-session full publications/repro gates.
No memory-based exception is granted to their other examples.

### DEC-03a.10 -- W4 full-screen acceptance is a declared class

Record the operator-directed departure from blanket real-III+ byte-exact
screen dumps. It applies **only to W4 full-screen applications**.
W3 GET/READ/@ and already minted form screens retain byte-exact grading.
Verb/function/SET/message and file/transcript oracles retain their existing
independent real-dBASE bar. FLAIR chrome acceptance is not re-graded here.

Every W4 app must carry three acceptance layers:

- **L1 -- data effects:** byte-exact output FILES against the same locked
  trace run on real DBASE.EXE, with only existing dbf_normalization.json
  normalization. Read-only/transcript cases are explicit below.
- **L2 -- structure:** locked injected key trace with independent named
  strings, regions, cursor/highlight/status expectations and named mutants.
  Expectations must not be generated from the implementation's own tables.
- **L3 -- Law 4:** short Rule-14 `record-flair` clip in build/clips/, including
  launch and return where relevant, replay/repro-proven for operator review.
  CONFIG.DB uses startup/effect/return; it invents no configuration editor.

The acceptance table adopts Seat B Sec 4, including its retained cheap
screens; it is not permission to omit a file oracle because a clip looks good.

| App / real spec section | L1: data-effect oracle | L2: locked-trace structural checks | Byte-exact screen retained |
|---|---|---|---|
| BROWSE (Sec 5) | .DBF/.DBT/.NDX after edits; identical trace on real DBASE.EXE. | F1 toggles menu; arrows/PgDn move cell/record; record n in status; Ctrl-End saves. | No. |
| EDIT / CHANGE / APPEND (Sec 4) | .DBF and memo .DBT; Esc abandonment leaves files unchanged; Ctrl-W saves. | Field navigation, key-help line; APPEND adds exactly one record. | Existing shared W3 READ screens stay exact. |
| CREATE / MODIFY STRUCTURE (Sec 6) | .DBF header and surviving MODIFY records per dbf.md. | Field-table rows, type cycling; Ctrl-End save / Esc abandon. | Yes: static empty CREATE table, Sec 6.1. |
| MODIFY COMMAND (Sec 9) | .PRG bytes after edit trace, and any backup file shown by real mint. | Text entry, cursor movement, save/abandon, status line. | No. |
| REPORT designer (Sec 8.1) | .FRM bytes; real-made file re-saved unchanged is identical; real REPORT FORM on SAMIR-made .FRM matches SAMIR output (test-samir-report-emu). | Designer sections reachable and box edits. | No. |
| LABEL designer (Sec 8.2) | .LBL bytes; same unchanged re-save and cross-runtime output round trips. | Label dimensions form. | No. |
| QUERY / SCREEN / VIEW (Sec 7) | .QRY, .SCR plus generated .FMT, .VUE; generated .FMT under SAMIR and real III+ yields identical W3 form. | Filter form rows, painter cursor/draw keys. | Yes: generated-.FMT W3 form screen. |
| ASSIST (Sec 1-2) | Issued-command transcript and resulting files equal real III+ for locked menu walk. | Pads ordered Set Up, Create, Update, Position, Retrieve, Organize, Modify, Tools. | No. |
| HELP (Sec 3) | None, read-only; assert no file changes. | Complete index vs HELP_topics.txt; F1 overlay, PgUp/PgDn, ENTER> prompt. | Main menu and a few cheap sampled topic pages with InitechBase-authored text. |
| CONFIG.DB (config-db.md) | DISPLAY STATUS / LIST MEMORY startup transcripts equal real III+. | N/A: no interactive app; locked startup trace checks settings/effects through L1. | Exact transcripts, no screen requirement. |

HELP's authored text is an explicitly labeled InitechBase content golden,
not a claimed verbatim III+ capture; topic completeness/navigation remain
independently grounded in III+ sources. [B] [W]

Implement a **locked `spec/samir/screen_regrade.json`**, one row for each
of the ten table groups, with expanded command coverage, L1 comparator and
golden/normalization, L2 trace/check/mutant names (or justified N/A), L3 clip
and repro gate, retained exact screens and provenance to this decision.
W5 must reject missing/duplicate groups, missing layer evidence, orphan
mutants, and unlisted re-grading. A deliberately removed row or mutant
must turn its inventory gate RED. This is a Rule-8 act with an issue and
worklog; this draft creates no spec file and weakens no executable gate.
W4.0 minting becomes optional except for retained exact screens and any
reference data/file effects still needing a real capture.

### DEC-03a.11 -- Ship plan, dispositions and released capacity

Append the dated Sec 7 to the ship plan, preserving Sec 0-6 as the record.
Adopt Seat B's seven-line replacement path. W3 slices 2-4 remain first;
the high-model implementation and re-proof unblock W4 resident growth.
W2a/b/c do not wait on packaging; their actual UI dependencies remain.

After ratification: close initech-5etom as delivered in part, with Phase A
DEC-19 and delivered child initech-5etom.1 credited, remaining packaging
expansion retired. Close initech-1x6x2.18 won't-do/superseded; leave closed
initech-1x6x2.6 and earlier completed size work closed without reverting it.
Re-grade initech-9x8cn, initech-j7lld and initech-y29t4 per DEC-03a.10.
Retain initech-p3nn0 and cross-link already-delivered initech-h4bk.
Do not close non-size bug/factory siblings by association.

Reassign packaging/shrink capacity to **Turbo Initech M7/M8** (Free Pascal
differential and resident K2 == K3 certificate) and **FLAIR fidelity** (live
desktop/chrome/input and Rule-14 acceptance). The C OS remains built by its
factory cross-toolchain; the Pascal fixpoint is not a C kernel rebuild.

## 3. Consequences

Application growth no longer demands overlay choreography or byte barter.
The overlay mechanism remains maintained, with its period costs and
correctness gates. An 8 MiB machine becomes a product requirement, and
the new allocation model is a deliberate native ABI extension. No paging,
preemption or protection is introduced. The 1.44 MB system diskette remains
a separate capacity constraint; 256 KiB is a module alarm, not disk space.

W4 loses blanket screen-byte identity by explicit operator decision;
file fidelity, independently specified structure and recorded interaction
become its load-bearing acceptance. No implementation may call a weak
"text present" check a substitute for the specified trace and mutants.
The kernel's very limited capacity remains an integration risk.

## 4. Oracle plan -- RED first, independent evidence, named mutants

These are required implementation gates, not results of this documentation
session. Keep the conventional MCB/MZ/loader/EXEC/TSR/overlay families and
their mutants, tenant-load and FLAIR launch regressions. Extend rather
than replace `test-mcb{,-int21,-emu}`, `test-mz{,-mutant}`,
`test-mzload`, `test-mzexec`, `test-loader{,-big}`, `test-exec{,-unit,-nested}`,
`test-tsr-exec` and their actual mutant targets. Maintain the ledger in
DEC-03a.8 and audit `test-merge-samir` membership. [L17] [M]

| Gate family / first RED | Required green evidence | Named mutation that must go RED |
|---|---|---|
| High MZ dispatch/relocation: today's >192256 B load refuses | Hand-authored nonzero-fixup image above old bound, within 256 KiB; literal expected base/entry/marker/checksum; filename swap leaves model unchanged; old tag stays low. | Wrong relocation base; omit fixup; move dispatch after low size check; accept new tag as legacy. |
| Header and footprint: today's code lacks extended declarations | Bad version/length/overlap/fixup/overflow and truncated FAT chains refuse before launch; canaries prove no writes outside owned destination; partial device I/O failure unwinds ownership and never executes. | Skip descriptor-length check; skip footprint preflight; copy into conventional PROGRAM_IMAGE. |
| Full-width allocation: old fake segments truncate | Real AH=48h/49h/4Ah sequence writes, grows, shrinks and frees high blocks; allocate >1 MiB; exact high bits, contents, largest full-width result, exhaustion and foreign-owner/zero/wrap/double-free refusals. | Mask EAX to 16 bits; truncate count to BX; alias native owner; allow foreign free. |
| Reserve/link and module ceiling: old floor rejects growth | Ceiling module GREEN / ceiling+1 RED; >old window GREEN; reserve+heap+metadata exact-fit and +1 boundaries; mismatched link/descriptor reserve refused. | Off-by-one ceiling; omit reserve from footprint; false minalloc or descriptor heap minimum. |
| Dirty-RAM zero/guard and lifetime | Fill high region with different patterns, boot/launch twice; BSS/free-heap/stack determinism and canaries; repeated successful and failed launches restore capacity; 4Ch/00h/INT20, nested conventional EXEC and refused second high EXEC/KEEP preserve parent/TSRs. | Skip zero; reset ESP low; write guard; leak owner; release stack before switch; discard parent allocator context. |
| Locked map/RAM | Every window pair disjoint; 7167 reported KiB RED, 7168 GREEN; known hole RED; 5 MiB boot fails with exact catalogue line before high writes; exactly 8 MiB boots in QEMU AND Bochs. | Place app in FLAIR heap/kernel guard; leave 5120 gate; trust summed E820 ranges despite hole; emit reworded diagnostic. |
| Suspend/return and Exit/Restart/Shut Down | O-7 bit-identical pre/post desktop and stable FLAIR capacity; prompt/hotkey relaunch; exit-to-DOS then high launch; cold restart deterministic; Rule-14 launch/use/return clip. | High write into offscreen; skip present; keep stale high owner; leak JFT; lose queued keys. |
| Overlay retention at high base | All slot/stamp/CRC/static/reentry/lifetime/output/I/O gates stay green; diskless/corrupt/foreign pair refusal unchanged; actual base relocates resident and overlay. | NO_RELOC/NO_STAMP/NO_CRC/REENTER and lifetime mutants still fail their named checks. |
| W4 three layers and locked inventory | Each DEC-03a.10 row has file/transcript golden, trace/structure and clip evidence or stated N/A; retained exact screens still exact; W3 unchanged. | Drop status line; break Tab/cell navigation; scramble ASSIST pad order; corrupt saved file; remove inventory row or mutant. |
| Shipping regression/pubs/kernel/repro | Real-III+ transcript/DBF/DBT/NDX/REPORT/message gates, kernel cap on all production variants, two clean EXE/OVL builds, full vector + per-session Bochs + publications. | Existing behavioral mutants still bite; changed CHKDSK arena figure or stale gs-dir capture fails. |

Before each implementation change, record the missing-behavior RED for its
named reason; after it, run GREEN and its mutants, restore production code,
then re-run appropriate gates. Reference ports use real period captures
and mutation proof per CLAUDE.md's differential TDD form. Expected addresses
and bytes must be literals or independent fixtures, not the same constants/
render tables transformed twice by artifact and oracle.

The 86Box 486/8 MiB/period-VGA authenticity leg must exercise high/low A20
alias patterns, BIOS memory fallbacks and the same high allocation/return
sequence. Its driver remains outstanding (initech-x0i); record it as owed,
not passed by inference from QEMU/Bochs. Cross-emulator disagreement stops
acceptance. Clips are required for every W4 group and the desktop path.

## 5. Migration and implementation authority

1. Operator ratifies the code-affecting choices listed in Sec 7. Until
   then, current code and locked specs remain the executable contract.
2. File implementation beads for high-model loader/allocator/crt0/PAL and
   Rule-8 ABI/map changes; minimum-RAM harness/boot diagnostics and hardware
   contract; size/link gates; W4 inventory/acceptance; requirements/manuals/
   screen re-mint. Each carries its RED-first plan and worklog obligation.
3. Land the model's tag/descriptor, map, allocator, stack, overlay ownership
   and lifetime changes together or in gated stages that cannot emit a new
   SAMIR binary for an old loader. Re-run kernel-cap gates at each stage.
   Stage small independent capacity fixes first if required; no cap waiver.
4. Re-emit SAMIR with extended tag and unchanged ABI compilation flags,
   retarget the ledger, re-prove suspend/lifetime, and land requirements/
   README/hardware/PRD/manual updates in lockstep with the boot floor.
5. Create screen_regrade.json and independent W4 oracles before claiming
   W4 completion; update the three beads' acceptance and W5 certificate.
   The certificate explicitly names this non-byte-exact screen class.
6. Reconcile packaging/shrink bead dispositions and the append-only plan;
   run full local acceptance/pubs and the period leg as recorded. This chair
   session makes no bd writes, changes no implementation files and pushes
   nothing. Follow-up titles are proposals, not newly allocated bead IDs.

## 6. Risks and rejected alternatives

**Risks.** Loader/allocator additions may exceed 1,220 B of recorded kernel
margin. A fake-segment conversion or PSP-derived image check can corrupt a
low address deterministically. Incorrect owner restoration can overwrite a
resident or leak high capacity after a failed launch. Dirty real-hardware
RAM and an E820 sum can hide defects under generous emulator defaults.
The high stack has a canary guard, not enforced protection. Diskette space
still matters. Structural acceptance without independent expectations and
mutants is insufficient. These risks are the reason for the gates above,
not authority to shrink their coverage.

| Rejected alternative | Reason / chair resolution |
|---|---|
| Keep shrinking, widen Phase B, solve nested overlay swaps | Operator retired that programme; W4 command reentry must be resident with today's manager. Existing supported leaf overlays remain optional. |
| Put SAMIR just above _kernel_end or borrow the kernel guard | Overwrites reserved transient/overlay/stack capacity; no such unowned space exists. |
| Carve from FLAIR heap / rebind it as DOS arena | Destroys live offscreen/native tenancy, invalidating O-7. AC-3 GENERAL allocations are a different contract. |
| Move all applications high / route by .EXE extension | Breaks flat binaries, TSRs and CHKDSK publications; no reliable ABI declaration. Use the distinct tag only. |
| Reuse 0x4943 with an optional flag or high PSP/fake segments | Old loaders can misexecute; 16-bit PSP/MCB encodings truncate high pointers. Adopt Seat A's low PSP + native sidecar/ABI. |
| Keep 6 MiB boot, gate high EXEC separately | Seat A offered this fork; chair selects global 8 MiB to maintain one supported contract and avoid a second product profile. |
| Remove all size checks or retain the 4 KiB quota | Seat B's 256 KiB runaway ceiling plus physical/declared-minimum proof catches runaway linking without restoring byte negotiations. |
| Raise the 64 KiB linker reserve to 2 MiB without a loader declaration | Would permit BSS/slot to overlap heap. Resolve Seat B's shorthand by checking actual declared reserve AND complete high-window containment. |
| Revert -Oz / ban every new overlay | Unnecessary ABI churn; freeze flags and default resident, permit proven leaf-only overlay work. |
| Relax W3/FILE oracles or discard size-named real captures | Outside operator scope and contrary to the compatibility product. Only the listed W4 screen class is re-graded. |
| Re-grade desktop pixel identity as W4 structure | O-7 protects live FLAIR state and remains bit-exact. |
| Raise kernel cap in this amendment | Separate account and separate ADR; the loader must fit measured remaining margin. |

## 7. Open for operator ratification

Only code-affecting confirmations are retained; the chair supplies a default
for each rather than leaving an implementation lane to invent policy.

1. Confirm the fixed 6-8 MiB window and **global** 8 MiB floor, with the
   contiguous-PC fallback assumption and known-E820-hole rejection in
   DEC-03a.6. A launch-only floor or broader sparse-map support changes code.
2. Confirm 0x5849/v1 descriptor, low PSP/native allocation ABI, fixed high
   stack/guard and single high owner (including second-high-EXEC and KEEP
   refusals). Reusing the old tag or supporting nested high images changes
   the allocator/format/lifetime design.
3. Confirm 256 KiB runaway ceiling and declared-reserve checks, and the
   exact W4 scope/retained screens/HELP content distinction in DEC-03a.10,
   including its locked inventory. This determines link and ship gates.

The kernel cap is not an open choice here: an implementation that cannot
fit must obtain a sibling decision. Operator ratification of this text does
not certify an implementation or close the owed 86Box evidence.

## 8. References and evidence ledger

- [A -- Seat A, unchanged advisory record](committee/DEC-03a-seat-A.md),
  findings 1-14, AM-1..7 and oracle plan; its P1-P19 citations carried below.
- [B -- Seat B, unchanged advisory record](committee/DEC-03a-seat-B.md),
  F-1..10, gate ledger, publications, acceptance table and seven-line path.
- [M -- main 797d87c5](../../harness/emu/tsr_exec.mk): `git show
  797d87c5cfdd4495bd25a02893cfd572b11a7fa4` (initech-h4bk merge message and
  loader/int21/test changes); authoritative newer baseline than A's parked
  cb714ad evidence. Numbers above are attributed commit measurements.
- [W -- local III+ specifications](/home/tobiasosborne/Projects/dbase3-decomp/specs/full-screen/assist-ui-and-editors.md):
  Sec 0-9, HELP_topics.txt inventory; file-formats/{dbf,frm,lbl,qry,
  scr-and-fmt,vue}.md and environment/config-db.md in the same corpus.
- Law 1/Law 2/Law 4, Rules 1/5/6/8/11/12/14 in CLAUDE.md; PRD Sec
  5/6.6/7/8/11; docs/design/samir-overlays.md; WL-0182, WL-0187,
  WL-0194; harness/merge_samir.mk; pubs screen/requirements sources named
  in B Sec 3. Current live `bd show initech-yndl2` reports no matching
  issue; read-only Seat B export confirms only size children .6 (closed)
  and .18 (open) by title. Reconcile against the operator's live tracker
  after ratification; no status was changed here.

Seat A's period citations (P numbers retained; limitations in Sec 1 apply):
[P1], [P2], [P3], [P4], [P5], [P6], [P7], [P8], [P9], [P10], [P11],
[P12], [P13], [P14], [P15], [P16], [P17], [P18], [P19]. The chair re-opened
P1, P7, P13 and P18; the remaining items retain Seat A's provenance, not a
claim of a new independent acquisition. The copied memo is the local
research record; implementing lanes must acquire any additional primary
material on which their code depends.

[A]: committee/DEC-03a-seat-A.md "Seat A advisory memo"
[B]: committee/DEC-03a-seat-B.md "Seat B advisory memo"
[M]: ../../harness/emu/tsr_exec.mk "Main 797d87c5, TSR relocation regression family; see git show for measurements"
[W]: /home/tobiasosborne/Projects/dbase3-decomp/specs/full-screen/assist-ui-and-editors.md "Local real-dBASE III PLUS 1.1 full-screen specifications"

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
[L1]: ../../README.md "System requirements"
[L2]: ../../spec/memory_map.h "Entire header; all windows, arena invariant, INITECH_MIN_EXT_KB"
[L3]: ../../os/milton/kmain.c "kernel_main extended-RAM boot gate"
[L4]: ../../os/milton/loader.c "loader_prepare_core, loader_prepare_mz, loader_prepare_tenant, loader_run_plan, load_program_from_fat"
[L5]: ADR-0003-AMENDMENT-DEC-08a-MZ-EXE-Flat-Loader.md "DEC-08a.1-.5 and 2026-10-20 placement amendment; os/milton/mz.c:mz_apply_relocs"
[L6]: ../../os/milton/loader.c "Separate branch evidence: git show cb714ad097481447792aa94d9fbf736f31e43875 -- os/milton/loader.c os/milton/int21.c os/milton/test_mzload.c harness/emu/tsr_exec.mk"
[L7]: ../../os/milton/mcb.h "Also mcb.c:mcb_init and spec/dos_structs.h:mcb_t/psp_t"
[L8]: ../../os/milton/int21.c "arena_seg_base, cur_psp_owner, do_alloc/free/setblock/terminate, tsr_keep, int21_mcb_bind_program"
[L9]: ../../os/samir/pal/pal_milton.c "pal_milton_make AH=48h conversion; stack guards; environment lookup"
[L10]: ADR-0013-FLAIR-App-Contract.md "Sec 3.2 class 2, Sec 3.7, O-7; contrast InitechOS-PRD.md Sec 6.6"
[L11]: ../../os/flair/process.c "FlairProcess_launch/admit, teardown_common, terminate/kill; AC-2 split arenas"
[L12]: ADR-0009-SAMIR-Milton-Integration-Numeric-Memory-Packaging.md "DEC-01, DEC-03, DEC-04"
[L13]: ../design/samir-overlays.md "Phase B shipped; Sec 1 historical 5575 B headroom; os/samir/core/ovl.c uses __samir_base"
[L14]: ../../os/boot/io_sys.asm "A20 enable, pm_entry, mem_probe; stage2.asm delegates; kstart.asm runway zeroing"
[L15]: ../../os/milton/psp.c "flat_to_fake_paragraph masks to uint16; spec/dos_structs.h PSP; os/samir/boot/samir_crt0.asm hardcodes ESP"
[L16]: ADR-0014-Kernel-Overlays.md "DEC-14-9 loaded-byte cap; Makefile KERNEL_SECTORS and test-kernel-headroom"
[L17]: ../../Makefile "Named test families; harness/proptest/test_memory_map.c; harness/diff/dbf_diff/test_hardware_spec.c"
[L18]: ../../harness/diff/dbf_diff/samir_size.mk "test-samir-size family; test_samir_size.c; os/samir/boot/samir.ld ASSERTs"
[L19]: ../../CLAUDE.md "Laws 1-4, Rules 1/5/6/8/14; 86Box driver outstanding, initech-x0i; PRD Sec 8"
[L20]: ../../harness/emu/bochs.c "write_bochsrc megs:32; compare harness/emu/qemu.c:build_argv (no -m)"
[L21]: ../../spec/hardware.json "cpu.value and memory model/windows; executable-format row contains stale MZ-deferred wording"
[L22]: ADR-0001-Hardware-Target-386-Origin-486-Floor.md "DEC-02 ratifies the 486-class target"

---

*-- End of Amendment DEC-03a (DRAFT v0.9) --*

<!-- Tedium certified compliant with NFR-7. The window has moved; the capacity account has not. -->
