<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# ADR-0013 Amendment AC-3 -- The Initech Toolbox Gate and disk-launched tenants (parameterized load bases; push-callback delivery; admit before activate)

**Issuing Body:** Initech Systems Corporation -- Office of Enterprise Architecture (OEA)
**Document Class:** ADR Amendment (binding; amends ADR-0013 in place)
**Programme:** STAPLER (InitechOS)
**Status:** **RATIFIED** (Architecture Review Board, App-Contract Committee; implementation reconciliation; 2026-10-20)
**Amends:** ADR-0013 (The FLAIR App Contract) Sec 3.1, Sec 3.2, Sec 3.4, Sec 3.6, Sec 6, Sec 7 O-3/O-5, Sec 8; ADR-0003 Amendment DEC-08a.1/.2 (parameterized placement)
**Supersedes:** the disk-native-tenant deferral in ADR-0013 Sec 6/Sec 8; AC-2's three-block accounting for disk tenants only (the AC-2 trio remains intact)
**Related Issues:** initech-t5xs (this amendment), initech-tdnl.14 (disk launch implementation), initech-2svz (host routing and gate oracle), initech-cnpm (tenant menubar), initech-smp0 (multi-slot follow-up)

---

## 1. Why this amendment

ADR-0013's original co-residency ruling admits statically linked native tenants and
defers disk-loaded native files. The implementation delivered by initech-tdnl.14
crosses that boundary: the Finder launches an InitechMZ image from disk into the
FLAIR heap, and that image registers with the existing App Contract. This is an
extension of the contract, not a second application model.

The controlling reconciliation is
[GUI-remediation-ADR-reconciliation.md Part B and Part C](../design/GUI-remediation-ADR-reconciliation.md).
The Committee hereby ratifies its four AC-3 decisions against the present sources.
PRD Sec 1.4, Sec 6.1, Sec 6.3 and Sec 8 remain the product and oracle authority
([InitechOS-PRD.md](../../InitechOS-PRD.md)). The parked-stack, tenant-called
WaitNextEvent proposal and the shipping launch watchdog were rejected by that
reconciliation; neither is authorized here. The four Laws and numbered Rules
continue to apply ([CLAUDE.md](../../CLAUDE.md)).

## 2. Disk loading and the four-block memory model (Sec 3.2/Sec 3.6 amended)

**DEC-AC3-1 -- Disk-launched FLAIR tenants; parameterized InitechMZ load bases.**
Per-file GUI tenant loading is ratified, lifting the Sec 6/Sec 8 deferral narrowly.
Disk tenants use the tagged InitechMZ flat-32 container; the content test and
foreign-MZ refusal remain those of
[Amendment DEC-08a](ADR-0003-AMENDMENT-DEC-08a-MZ-EXE-Flat-Loader.md).
The shared parser recognizes MZ and ZM; a non-MZ flat COM is refused on the
heap-tenant path. An untagged MZ produces the existing fail-loud foreign-MZ panic
at the kernel loader seam.

DEC-08a.1/.2 are **extended** to an explicit placement pair: the PSP occupies the
code block head (`psp_addr = block_linear`); the module begins 256 bytes later
(`load_base = block_linear + TBX_TENANT_PSP_BYTES`); relocation adds that flat
load base to each uint32 fixup, and entry is `load_base + entry_off`. The canonical
EXEC pair remains `PROGRAM_BASE = 0x00040000`, `PROGRAM_IMAGE = 0x00040100`.
The fixed FLAIR heap is `[0x00100000, 0x00500000)`; an individual tenant base is
chosen by its heap carve, not assigned a new fixed address
([spec/memory_map.h](../../spec/memory_map.h)).

The code block is GENERAL-class storage for PSP, module and zeroed trailing space.
The current carve includes `e_minalloc * 16` bytes beyond the paragraph-rounded
file size, bounded by `TBX_TENANT_BSS_MAX`; the loader rejects a module/BSS that
does not fit. Registration then carves the AC-2 trio. **Carve order is code ->
handle -> RECORDS -> DATA; free order is DATA -> RECORDS -> handle -> code.**
The small handle and larger RECORDS block are HANDLE-class; DATA is GENERAL-class.
Failed preparation returns any code carve; failed admission reclaims its partial
trio in reverse and installs nothing. The code carve is reclaimed on the reap
after entry returns without registration. The first completed cycle consumes
the four bump spans; subsequent equal-sized cycles reuse the free-list slots.
O-3 requires stable available memory across cycles >= 2, not restoration of the
virgin bump cursor.

One resident disk slot is the current scope; a second launch reports
`TENANT-SLOT-BUSY` without disturbing it. This does not restrict the general
process list to one app. Multiple disk slots and per-tenant DOS MCB arenas
remain follow-up work. Disk loading does not acquire the single-EXEC guard or
occupy `PROGRAM_BASE`. The tenant PSP points at the shared `ENV_BLOCK`
(`0x0006F000`), initially inherit-empty; a later EXEC may rewrite that environment.
During class-2 text execution the desktop pump is suspended, so resident GUI
tenants receive no callbacks; their registrations and z-order remain resident.

**Implementation (Law 1):**
[os/milton/mz.c:mz_is_mz](../../os/milton/mz.c),
[os/milton/mz.c:mz_parse_header](../../os/milton/mz.c),
[os/milton/mz.c:mz_apply_relocs](../../os/milton/mz.c);
[os/milton/loader.c:loader_prepare_mz](../../os/milton/loader.c),
[os/milton/loader.c:loader_prepare_tenant](../../os/milton/loader.c),
[os/milton/loader.c:loader_load_tenant](../../os/milton/loader.c),
[os/milton/loader.c:loader_panic_foreign_mz](../../os/milton/loader.c);
[os/flair/tbxgate.c:carve_code](../../os/flair/tbxgate.c),
[os/flair/tbxgate.c:tbx_launch](../../os/flair/tbxgate.c),
[os/flair/tbxgate.c:tbx_reap](../../os/flair/tbxgate.c),
[os/flair/tbxgate.c:finish](../../os/flair/tbxgate.c);
[os/flair/process.c:FlairProcess_admit](../../os/flair/process.c),
[os/flair/process.c:teardown_common](../../os/flair/process.c);
[os/milton/kmain.c:flair_launch_text_tenant](../../os/milton/kmain.c).

**Oracle (Law 2):** [Makefile](../../Makefile) `test-tenant-load` and
`test-tenant-load-mutant` prove noncanonical relocation, PSP address independence,
BSS zeroing, refusals and the retained EXEC base; the mutant is
`LOADER_MUTATE_TENANT_PROGRAM_IMAGE_BASE`.
`test-flair-app-launch` LAUNCH/DOUBLE/CYCLE and its `-mutant` counterpart prove
the disk trace, slot guard and stable reuse (`DOUBLE_LAUNCH`, `EXIT_LEAK`).
The trace is locked in
[spec/flair_app_launch_trace.golden](../../spec/flair_app_launch_trace.golden).

## 3. The locked Toolbox gate (Sec 3.1 transport extended)

**DEC-AC3-2 -- The Initech Toolbox Gate (INT 0x81).**
[spec/toolbox_gate.h](../../spec/toolbox_gate.h) is hereby ratified as the
**locked Rule-8 ABI**, including its frame-relative argument helper, registration
record and flat event layout. Its current deliberate extensions continue to
govern; the table below records the original tdnl.14 surface, not an exhaustive
inventory of all subsequently added calls.

The gate is vector **0x81**, a present 32-bit **trap gate, type/attributes 0x8F**,
**DPL=0**, flat code **selector 0x08**. IF retains the caller's setting. AX is
the low word of EAX; the dispatcher reads cdecl dword argument `i` at
**frame + 68 + 4*i**. The 68-byte `int_frame_t` is statically pinned in
[os/milton/idt.h](../../os/milton/idt.h); there is no ring-change SS:ESP pair.
The return value replaces saved EAX; errors are negative. The dispatcher emits
one `TENANT-GATE ax=...` trace line per call.

| AX | Locked name | Meaning / arguments |
|---|---|---|
| `0x0001` | `TBX_REGISTER` | REGISTER (`recPtr`) |
| `0x0020` | `TBX_NEWWINDOW` | NEWWINDOW (`l,t,r,b,goAway`) |
| `0x0022` | `TBX_SETWTITLE` | SETWTITLE (`win,strPtr`) |
| `0x0030` | `TBX_TEXTDRAW` | TEXTDRAW (`win,x,y,strPtr,fg,bg`) |
| `0x0031` | `TBX_FILLRECT` | FILLRECT (`win,l,t,r,b,color`) |
| `0x0070` | `TBX_EXIT` | EXIT (`rc`), deferred teardown after return |
| `0x0010` | `TBX_EVENTCHANNEL` | Reserved kernel-to-tenant push channel; not a tenant-callable trap |
| `0x0002` | `TBX_KEEPRESIDENT` | Dissolved; registration establishes residency |

Reserved/dissolved codes return `TBX_ERR_BADCODE` (-1) for an executing tenant;
a stray kernel call returns `TBX_ERR_NOTTENANT` (-7). No tenant-called event
wait or separate KEEP_RESIDENT service is introduced. The hardware-reserved
range 0x28-0x37 remains unavailable for software gates; 0x2F is IRQ7, while
the cascade IRQ2 is 0x2A
([DEC-04a Appendix E](ADR-0003-AMENDMENT-DEC-04a-INT21h-Flat-Calling-Convention.md)).

**Implementation (Law 1):**
[os/milton/kmain.c:kernel_main](../../os/milton/kmain.c) installs the gate from the
locked vector/selector/attribute constants;
[os/flair/tbxgate.c:tbx_gate_dispatch](../../os/flair/tbxgate.c) implements the
AX demux, arities, frame+68 reads, trace and EAX result, with verbs
[os/flair/tbxgate.c:v_register](../../os/flair/tbxgate.c),
[os/flair/tbxgate.c:v_newwindow](../../os/flair/tbxgate.c),
[os/flair/tbxgate.c:v_setwtitle](../../os/flair/tbxgate.c),
[os/flair/tbxgate.c:v_textdraw](../../os/flair/tbxgate.c),
[os/flair/tbxgate.c:v_fillrect](../../os/flair/tbxgate.c) and
[os/flair/tbxgate.c:v_exit](../../os/flair/tbxgate.c).

**Oracle (Law 2):** [Makefile](../../Makefile) `test-flair-routing` leg X checks
the total AX map, poisoned frame-relative args, low-word demux, negative errors,
stray calls and exact trace. `test-flair-routing-mutant` supplies
`TBX_MUT_AX_FULL_EAX` and `TBX_MUT_ARG_OFFSET`. `test-flair-app-launch` LAUNCH
exercises the real INT gate against the locked trace.

## 4. Residency, callback delivery and death (Sec 3.4 amended)

**DEC-AC3-3 -- Push-callback delivery; link-synthesized registration; yield-is-return.**
The registration record is **link-synthesized data**, never a record whose
addresses a Pascal tenant must construct. The locked 24-byte `FlairTenantRec`
is six dwords: tag `'FLTP'` (`0x50544C46`), `namePtr`, `imageBase`, `imageLen`,
`evBufPtr`, `eventProc`, at offsets 0/4/8/12/16/20. Pointer values are resolved
by the toolchain and covered by the InitechMZ relocation table; `imageLen` is
a length and is not relocated. Registration validates the whole record, the
NUL-terminated name, the whole event buffer and the callback entry against
`[load_base, load_base + module_len)`, and requires exact base/length agreement.
BSS and PSP addresses do not satisfy this registration contract. Bad records
report `TENANT-REGISTER-BAD` and return `TBX_ERR_BADREC` without admission.

The hand-assembled tdnl.14 fixture emits this record as `rec` in
[os/apps/tenantfx.asm](../../os/apps/tenantfx.asm). The current C binding emits
`g_rec` in [os/apps/tbx/tbx_rt.c](../../os/apps/tbx/tbx_rt.c), with image symbols
resolved by [os/apps/tbx/tenant.ld](../../os/apps/tbx/tenant.ld). These are the
present producers. The reconciliation's proposed `seed_flair.ld` / TPS MZ-wrap
producer is a separate toolchain undertaking, not an implementation asserted
by this Record; the ABI requires no Pascal subset growth
([ADR-0007 DEC-02](ADR-0007-Turbo-Initech-Self-Hosting-Pascal-Compiler.md)).

Admission stamps `FLAIR_APP_MAGIC` and installs a kernel-owned vtable:
`open=NULL`, `event=disk_tenant_event`, `idle=NULL`, `close=disk_tenant_close`.
The single Layer-5 event spine reaches that event adapter, which widens a cooked
`EventRecord` into the six int32 fields at `evBufPtr` and calls `eventProc`.
**Returning is the yield**. Registration establishes residency and entry
returns to the shell pump. Part B's proposed idle delivery as dispatched
`nullEvent`s is not presently wired: the vtable's idle entry is NULL, the
live pump skips timeout/null events, and `flair_app_dispatch` leaves them
unhandled. This Record does not certify an idle-callback service. No
tenant WaitNextEvent loop, parked-stack event resumption or shipping watchdog
is permitted. If entry returns without registering, the reap reports
`TENANT-UNREGISTERED` and frees the code block; a tenant that never returns
still hangs the cooperative desktop (BC-4).

EXIT records a request; after tenant return, reap calls `FlairProcess_terminate`.
A normal tenant-owned close box also terminates, with `HideWindow` reserved for
unowned shell furniture. The later `TBX_GOAWAY_ASK` extension first delivers a
close request so the tenant may guard unsaved work; it does not authorize
hide-only tenant teardown. The close hook is kernel code and records that the
AC-2 trio was torn down; it does not call tenant code.

Crash triage is precise: while a tenant call is active and its code block
exists, **a CPU exception whose EIP lies inside the loaded module** unwinds to
the kernel call trampoline and marks the slot crashed. Reap uses
`FlairProcess_kill` for an admitted app, skipping `close()` and any further
dead-tenant callback, then frees the code block last. A pre-registration entry
fault in that range is also unwound and its code block reclaimed. This latter
case is broader than Part B's draft phrase "registered image range": the
current implementation governs. A fault outside the range, including a kernel
fault inside a Toolbox verb, falls through to the existing panic and halt.
AC-2's death guarantee protects against a corrupt DATA payload; it does not
promise isolation from arbitrary scribbles over RECORDS or the master heap.

**Implementation (Law 1):**
[os/flair/tbxgate.c:v_register](../../os/flair/tbxgate.c),
[os/flair/tbxgate.c:disk_tenant_event](../../os/flair/tbxgate.c),
[os/flair/tbxgate.c:disk_tenant_close](../../os/flair/tbxgate.c),
[os/flair/tbxgate.c:run_tenant](../../os/flair/tbxgate.c),
[os/flair/tbxgate.c:tbx_fault_triage](../../os/flair/tbxgate.c),
[os/flair/tbxgate.c:tbx_reap](../../os/flair/tbxgate.c);
[os/flair/tbxmenu.h:tbx_evbuf_fill](../../os/flair/tbxmenu.h);
[os/flair/process.c:flair_app_dispatch](../../os/flair/process.c),
[os/flair/process.c:FlairProcess_terminate](../../os/flair/process.c),
[os/flair/process.c:FlairProcess_kill](../../os/flair/process.c),
[os/flair/process.c:FlairProcess_close_window](../../os/flair/process.c);
[os/milton/kmain.c:flair_live_do_close](../../os/milton/kmain.c),
[os/milton/kmain.c:flair_live_tenant_service](../../os/milton/kmain.c),
[os/milton/kmain.c:kernel_main](../../os/milton/kmain.c);
[os/milton/panic.c:isr_dispatch_c](../../os/milton/panic.c) and
[os/milton/tbx_gate.asm](../../os/milton/tbx_gate.asm) (`tbx_tenant_call` / `tbx_tenant_abort`).

**Oracle (Law 2):** [Makefile](../../Makefile) `test-flair-routing` legs B/C/D/E/H
grade the real callback, routing, exit, crash and no-registration paths.
`test-flair-app-launch` CLOSE/NOREG/CRASH proves close/relaunch, no-registration
absence and #UD (`TENANT-CRASH vec=6`) survival; its `-mutant` supplies
`NO_REGISTER` and `CLOSE_STALE_FOCUS`. `test-process-teardown` and its `-mutant`
retain the AC-2 corrupt-DATA death proof. Oracle-side budgets do not become
shipping watchdogs.

## 5. Admission precedes foreground affirmation (Sec 3.2 sequencing amended)

**DEC-AC3-4 -- Windowless admission; foreground affirmation after NEWWINDOW.**
REGISTER uses `FlairProcess_admit`, not the open-and-foreground launch helper.
With an existing foreground app, admission inserts the windowless tenant
immediately after that head as BG; the head, key target and band-2 bar remain
undisturbed. **Empty-list qualification:** the current helper marks the first
admitted app FG when there is no previous head. That bookkeeping case does
not create a window or run the foreground switch; the unconditional draft
claim "registration is never foreground" is not the implemented rule.

NEWWINDOW allocates the `WindowRecord`, structural/content/update region trio
and shared clipping scratch from the RECORDS arena, binds `refCon` to the app,
and requests affirmation. After the tenant returns, the pump takes that request
and calls the existing `FlairProcess_activate` sequence: raise the group,
deactivate the previous app, activate the new app, relink/promote and synchronize
active chrome. `flair_live_finish_tenant_switch` then handles chrome, content,
the band-2 bar and presentation. The first window therefore brings a resident
tenant forward through the same switch used by other apps. The current verb
also requests affirmation for later successful NEWWINDOW calls.

SETMBAR before the first window is legal: its validated tenant-resident bar
takes effect when the tenant is affirmed. The current locked ABI additionally
permits replacing the bar after window creation and requesting DRAWMENUBAR;
this Record does not reinstate the superseded pre-window-only restriction.

**Implementation (Law 1):**
[os/flair/process.c:FlairProcess_admit](../../os/flair/process.c),
[os/flair/process.c:FlairProcess_activate](../../os/flair/process.c),
[os/flair/process.c:switch_foreground](../../os/flair/process.c);
[os/flair/tbxgate.c:v_newwindow](../../os/flair/tbxgate.c),
[os/flair/tbxgate.c:v_setmbar](../../os/flair/tbxgate.c),
[os/flair/tbxgate.c:tbx_take_affirm](../../os/flair/tbxgate.c);
[os/milton/kmain.c:flair_live_launch_app](../../os/milton/kmain.c),
[os/milton/kmain.c:flair_live_tenant_service](../../os/milton/kmain.c),
[os/milton/kmain.c:flair_live_finish_tenant_switch](../../os/milton/kmain.c).

**Oracle (Law 2):** [Makefile](../../Makefile) `test-flair-routing` legs A/B prove
windowless admission preserves Finder focus and first-window activation emits
the ordered pair; `test-flair-routing-mutant` makes
`PROC_MUT_ADMIT_IS_FOREGROUND` RED. `test-flair-app-launch` SHOW/RESTORE/MENU
grades the visible window, tenant bar and restored Finder; its
`MBAR_NOT_SWAPPED` mutant catches a skipped band-2 switch.

## 6. Oracle / acceptance and consequences (Law 2)

The DEC-specific gates above are the implementation acceptance obligations;
their definitions and mutant reasons are in [Makefile](../../Makefile).
`test-flair-app-launch-bochs` is the existing CPU-path differential over the
headless smoke kernel, not a substitute for the QEMU visual legs. Rule-14
visual acceptance uses `app_launch` through the existing recording machinery
(`build/clips/app_launch.gif` / `.mp4`); this documentation amendment does not
mint or recertify that recording.

This change-set is a **documentation ratification**. Its local governance gate
is `make -j3 test-adr-citations`, mutation-proven by the existing
`make -j3 test-adr-citations-mutant` target
([harness/adr_citations.mk](../../harness/adr_citations.mk)); it checks the
foundational ADR-0001/ADR-0002/CDR-0001 citation scope. Links and file:function
citations in this Record must also resolve to the present tree; that gate
does not mechanically validate C symbols or the AC-3 prose. Naming an emulator
gate here records its acceptance role, not a claim it was rerun for this
documentation-only change.

The single event spine, cooperative/no-watchdog rule, refCon magic, AC-2
RECORDS/DATA split, finite fail-loud budgets and flat-32/no-v8086 boundary remain
binding. The new cost is one heap-resident code block per disk slot and a
kernel gate/adapter; no protection, preemption or per-tenant DOS heap is implied.

---

*End of ADR-0013 Amendment AC-3. RATIFIED 2026-10-20. The initech-tdnl.14 disk-tenant and Toolbox-gate reconciliation (initech-t5xs). Controlled Document; verify revision before use.*
