<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->
<!-- Minted 2026-08-23 by the ox-alpha 1M-context design seat (openrouter stealth/ox-alpha,
     tool-less, 590KB inlined corpus), orchestrator-reviewed. Design gates D1 (R3.7
     initech-tdnl.14) + D2 (R5.1 initech-tdnl.21) + D3 pixel-gap audit (sjvq/81ft/tdnl.6/tdnl.7).
     STATUS: DRAFT pending ratification; the section-0 corrections are honest corpus limits
     (the ADR texts were absent from the corpus -- reconcile before ratifying).
     ORCHESTRATOR NOTE on D3 row 63 / D3-1: the palette value-domain is ALREADY RATIFIED --
     ADR-0004-AMENDMENT-DEC-10 OQ-3 binding rule: canon values enter in the SAMPLED screen
     domain, goldens are sampled-domain, so DAC and grading are consistent by construction
     of the ruling, not by accident. The report's nominal-DAC concern is thereby resolved;
     the provenance audit it asks for was done at DEC-10 ratification (the gamma proof). -->

# D1+D2+D3 Design Report

**Seat:** ox-alpha (read-only design seat, GUI-remediation epic `initech-tdnl`)
**Scope:** R3.7 (D1 — disk-launched FLAIR tenants), R5.1/R5.2 (D2 — TPS Pascal Toolbox binding), D3 — Platinum pixel-gap audit supporting R2 (`sjvq`/`81ft`/`tdnl.6`/`tdnl.7`).

---

## 0. Corrections to the brief — read first

The orchestrator needs these before anything else. Where the corpus contradicts or under-delivers versus the brief's assumptions, they are listed here and reflected inline.

1. **The ADR texts are absent from the corpus.** The corpus contains the markers `docs/adr/0013*`, `0007*`, `0003*` with **zero bytes of content**. All ADR-derived claims below rest on secondary citations inside artifact headers (which quote ADR-0013 Sec 2/3.x extensively, DEC-08/DEC-08a via `loader.c`/`mz.c`, ADR-0007 via `start_dos.asm`/`TPS-M7-subset-plan.md`) — not on the ADRs themselves. Every DECISION below marked *(ADR-text-pending)* must be reconciled against the actual ADR before ratification.
2. **No line numbers survive inlining.** The corpus files carry no line numbers, so per the style clause I cite by symbol, function, and spec-section anchor (e.g. `os/flair/process.c` :: `teardown_common`; `sys8/window-chrome.md` §2.2). Line-exact citation is a mechanical follow-up for the implementing seat, not a design gap.
3. **Files named by the brief but NOT in the corpus:** `os/milton/loader.h`, `spec/memory_map.h` (values recoverable from loader/kmain comments: `PROGRAM_BASE`, `PROGRAM_IMAGE = 0x40100`, `PROGRAM_STACK_TOP = 0x7FFFC`, `ENV_BLOCK = PROGRAM_ARENA_CEIL = 0x6F000`, FLAIR heap `[0x100000, 0x500000)`), `spec/chrome_metrics.h` (all `FLAIR_CHROME_*` constants), `os/flair/flair_look.h` (the `FLAIR_PART_*` → pixel policy table), `spec/window_record.h`, `spec/event_model.h`, `seed/parser.c` / `seed/typecheck.c`, `os/flair/textedit.h` (confirmed absent), the MCB/arena implementation behind `int21_mcb_bind_program` (`os/milton/int21.c`), and `os/tps/tps.pas` beyond the declarations head. Consequences are flagged at each point of use.
4. **`kmain.c` is truncated mid-pump.** The tenant launch site, `ten_plist`, `flair_app_dispatch` call site, and `FlairProcessList` wiring are present; the tail of the pump (band-2 swap loop, tick budget termination) is cut. Nothing below depends on the missing tail.
5. **`NewWindow` carries no title parameter** (`os/flair/window.h` :: `NewWindow(wm, w, bounds, content, wKind, wVariant, goAway)`). Titles live outside the WindowRecord as far as this corpus shows (`fd_win_titles[]` in kmain; `flair_draw_document_window(port, skin, frame, title, hilited)` takes the title as a parameter). This matters for D2 (a Pascal `SetWTitle` needs somewhere to *store* the title). See D2-3.
6. **The brief's premise "PROGRAM_BASE is single-program today" is confirmed and sharper than stated:** `loader.c` :: `g_load_active` enforces single-level EXEC, and `loader_run_plan`'s IN-PLACE SAFETY PROOF states the parent-is-kernel/single-program-region invariant explicitly. The AH=48h arena is bound per-EXEC to one window `[arena_base, PROGRAM_ARENA_CEIL)` (`loader.c` :: `loader_prepare_core`). There is no MCB chain over multiple program regions in evidence.

---

# PART D1 — Disk-launched FLAIR tenants (R3.7)

## D1.1 The binary format: InitechMZ flat-32 `.EXE`, not `.COM`

**Ruling (D1-1): disk-launched GUI tenants ship as InitechMZ flat-32 `.EXE` images; the flat `.COM` remains the format for class-2 text tenants (SAMIR) unchanged.**

Reasoning, grounded:

- Both formats already ship and dispatch by content, not extension: `loader.c` :: `load_program_from_fat` probes `mz_is_mz(image, got)` and routes to `load_program_mz_in_place` vs `load_program_in_place` (DEC-08a.4). Adding an MZ tenant costs no new loader machinery.
- The decisive argument is **relocation**. A `.COM` is position-dependent: its code is correct only at the address it was linked for (today `PROGRAM_IMAGE = 0x40100`). A GUI tenant must be loaded somewhere other than the single-program region (see D1.6), and possibly at varying addresses across boots. InitechMZ exists precisely for this: `mz_apply_relocs(image, len, load_base, reloc_table, count)` adds an arbitrary flat `load_base` to each fixup dword (`os/milton/mz.c` :: `mz_apply_relocs`; DEC-08a.1). A second tenant at a different base is a solved problem; a second `.COM` is not.
- The InitechMZ tag gate (`e_res[0] == MZ_INITECH_TAG`, `mz.c` :: `mz_parse_header`; foreign-MZ panic at `loader.c` :: `loader_panic_foreign_mz`, DEC-08a.5) gives us, for free, the fail-loud property that a stray genuine 16-bit `.EXE` dropped on the volume panics instead of misexecuting — exactly the honesty Rule 2 wants on a user-visible launch path.
- Period authenticity is preserved: real DOS 3.3-era GUI-adjacent systems (Windows 3.x) shipped relocatable NE/MZ containers, not raw `.COM`s, for exactly this reason. The chimera stays honest: `.COM` for console tools, tagged relocatable container for Toolbox apps.

## D1.2 How a loaded program acquires the FLAIR API: the gate

A disk tenant cannot link kernel symbols. Three candidate mechanisms were scored. Scores are 1 (poor) – 5 (excellent).

| Criterion | (a) INT-trap dispatcher (multiplexed by AX) | (b) Fixed-address kernel jump-table page | (c) Pointer block passed at entry |
|---|---|---|---|
| Period authenticity | 5 — INT 2Fh multiplexer is *the* DOS TSR/driver convention (printspool, network redirector, Windows 386); Mac precedent is the A-trap word, which is the same idea (function id in a register, trap into the system). | 4 — Mac A5/trap-table precedent is real, but a *fixed linear page exported by a DOS personality* has no DOS precedent; it smells like a 2026 design wearing a Mac costume. | 2 — closest DOS analogue is the PSP pointer in EBX at entry (`loader.c` asm: `mov %5, %%ebx`), but a whole API-via-one-pointer is neither DOS nor Mac. |
| ABI stability | 5 — AX codes are a locked, sparse namespace; adding functions never breaks old binaries. Gate implementation is invisible. | 4 — stable once the page address is locked, but every entry slot is forever reserved; page placement is a Rule-8 memory-map edit. | 2 — struct layout is baked into every tenant binary; any kernel-side reorder silently corrupts calls (the exact hazard `loader_context_t`'s `_Static_assert` offsets exist to prevent — but tenants can't be recompiled in lockstep with the kernel). |
| Oracle-ability | 5 — one IDT gate; the dispatcher can emit a serial line per call (`TENANT-GATE ax=…`), giving a mechanical call trace for free (Law 2). | 3 — calls are indirect jumps; tracing requires instrumenting every stub. | 2 — no chokepoint at all. |
| Memory-map cost | 5 — one vector, zero bytes of map. (Vector choice constrained, see below.) | 2 — a locked 4 KiB page in `spec/memory_map.h`, a deliberate Rule-8 act, plus a guard in the kernel-end window that kmain already fights (`busted kernel_shell's _kernel_end < PROGRAM_BASE window by 8340 B` per kmain comment). | 4 — near-zero cost. |
| Serves D2 (Pascal RTL calls the same gate) | 5 — a Pascal stub is literally `mov ax, N; int GATE`; identical for seed and TPS codegen. The trap id *is* the ABI. | 3 — Pascal would need `call far [abs addr]` or the pointer stashed; both uglier in a subset with no pointers-in-source. | 2 — the pointer block must be located and cached by RTL startup asm; fragile. |
| **Total** | **30** | **16** | **12** |

**Ruling (D1-2): mechanism (a) — a single INT-trap gate, the "Initech Toolbox Gate," multiplexed by AX.**

- **Vector:** `INT 81h`. `INT 80h` is taken (kmain self-test: `idt_install_trap(0x80u, isr_selftest)`); the PIC remap puts slave IRQs at `0x30+` (mouse = `0x34` per kmain), so `0x2F` is unsafe (master cascade region) and `0x81` is clear. Installing the gate joins `sysinit_early`'s install list. The vector number goes into the locked hardware/memory-map spec (Rule 8 deliberate act).
- **Calling convention (the locked gate ABI):**
  - `AX` = trap function code (sparse, grouped: `0x00xx` lifecycle, `0x10xx` events, `0x20xx` windows, `0x30xx` QuickDraw, `0x40xx` controls, `0x50xx` menus, `0x60xx` scrap, `0x70xx` misc).
  - All scalar/pointer arguments are passed **on the stack, cdecl** (the stub pushes nothing extra beyond the normal Pascal/cdecl frame). Because the gate fires as an interrupt on the *same flat stack*, the kernel-side dispatcher reads arguments from the interrupted `ESP` in the trap frame — no register-marshalling ambiguity, and Pascal `var` parameters (which are pointers under the hood) pass transparently without any `@` in Pascal source.
  - Result in `EAX` (Boolean in `AL`), errors signalled as negative `EAX`, mirroring the B8 RTL error convention (`fileio.asm` :: `rtl_file_fail`: diagnostic + loud exit; here: diagnostic + negative return, *not* exit — a tenant must survive a failed scrap put).
  - The kernel dispatcher emits one serial line per gated call in debug builds (`TENANT-GATE ax=0x…. …`) — this is the D1/D2 oracle backbone.

## D1.3 Entry, registration, and residency

Sequence for double-click → running tenant:

1. **Desktop double-click** resolves the icon to an 8.3 filename on the mounted FAT volume (R3.2/R3.3 provide the icon→name mapping; the desktop DB already lives on the data volume per R3.1).
2. **Load:** a new loader entry `loader_load_tenant(name83)` (sibling of `load_program_from_fat`) locates and reads the file — but **not** into `PROGRAM_IMAGE`. It carves a `FLAIR_CLASS_GENERAL` block from the master FLAIR heap (the same heap `ctx.master` exposes in kmain's tenant arm), rounds the file length up to a paragraph, and reads the MZ file there. It then runs the DEC-08a prologue *parameterized by base*: parse (`mz_parse_header`), relocate with `mz_apply_relocs(module, len, load_base = block_addr, …)`, move the load module down over the header *within the block*. Foreign MZ → the existing panic. Malformed MZ → fail-loud status, no tenant, serial `TENANT-LOAD-FAIL`.
   - A minimal PSP is built at the head of the block via `psp_build` (it already takes an explicit address, `loader.c` :: `psp_build((psp_t *)(uintptr_t)plan.psp_addr, &plan.params)`), giving the tenant's B8-style file I/O a JFT with the standard handles. Env: inherit-empty. *(Verify psp_build tolerates an arbitrary base — its parameterization suggests yes; flagged, ADR-text-pending.)*
3. **Entry:** the loader switches stacks and jumps to `PROGRAM_IMAGE-analogue` = `block_base + module_offset + entry_off` with `EBX = block PSP`, exactly the existing transfer shape (`loader_run_plan`'s asm), except the "return-to-loader" contract is replaced (D1.4).
4. **Registration:** the tenant's first act is one gate call, `AX=0x0001 FLAIR_REGISTER(recPtr)`, where `recPtr` points at a tenant-image-resident record:

```
FlairTenantRec:
  +0  uint32  tag 'FLTP' (0x50544C46)
  +4  uint32  namePtr     (relocated const char*)
  +8  uint32  imageBase
  +12 uint32  imageLen
  +16 uint32  wneBufPtr   (relocated; tenant's FlairEvent buffer)
```

   The kernel validates tag, that `recPtr`, `namePtr`, `wneBufPtr` all lie inside `[imageBase, imageBase+imageLen)` (fail-loud `TENANT-REGISTER-BAD` otherwise — a scribbled or lying record must never reach the process list, Rule 2), then performs the **App Contract launch steps verbatim** (`process.c` :: `FlairProcess_launch` steps a–g): carve the `FlairApp` HANDLE handle, the RECORDS block (WindowRecords + region pools, AC-2 split-arena), and the DATA block from the master heap; stamp `FLAIR_APP_MAGIC`; set `name`; and install a **kernel-owned vtable**:

```
app->procs = &disk_tenant_procs;   /* open=NULL (entry already ran),
                                      event=disk_tenant_event,
                                      idle=NULL, close=disk_tenant_close */
```

   `disk_tenant_event` is the resume trampoline of D1.4. The tenant then calls `AX=0x0002 FLAIR_KEEP_RESIDENT`, which parks it permanently (D1.4). The tenant's *code image is never freed while registered*; its heap partitions are ordinary AC-2 child blocks, so `FlairProcess_kill`'s scribble-survival property holds unchanged.
5. **Foreground:** registration ends with `SelectWindow` + list-head link exactly as `_launch` step (e)/(f) — the disk tenant comes up foreground, the previous foreground demotes, band-2 menubar swaps via the existing `flair_live_finish_tenant_switch` path in kmain (which already handles "list->head changed" generically).

## D1.4 Cooperative yield: park/resume WaitNextEvent

The tenant pumps events with a genuine `WaitNextEvent` analogue, `AX=0x0010 FLAIR_WNE(mask, sleepTicks) → Boolean`, without preemption and **without a second dispatch spine**:

- **Park.** `KEEP_RESIDENT` and every `WNE` that finds no deliverable event do the same thing: the gate handler (running, post-interrupt, on the tenant's stack) records into the tenant's `FlairApp` a `disk_ctx { savedESP, resumeOK }` — `savedESP` pointing at the still-intact interrupt frame on the tenant stack — then restores the kernel's saved pump `ESP` (captured once at pump-loop top, the same trick `loader_context_t.saved_esp` uses) and jumps to the pump resume label. The tenant is *dormant*, holding no CPU; this is cooperative scheduling by stack swap, the System-6 Switcher / MultiFinder model realized with the loader's proven transfer primitive.
- **Resume.** When the existing Layer-5 spine (`flair_app_dispatch`) routes a cooked event to this tenant, `app->procs->event` = `disk_tenant_event` fires: it copies the `EventRecord` into the tenant's registered `wneBufPtr`, sets `EAX = 1`, loads `savedESP`, and `iretd`s — landing the tenant on the instruction after its `int 81h`, inside its stub epilogue, which moves the result into Pascal terms and returns. The tenant handles the event and loops to `WNE` again. If the routed event was destined for a *different* tenant, the parked tenant simply is not resumed; the pump keeps running. A `nullEvent` after `sleepTicks` resumes with `EAX = 0` (driven off `flair_tick_count`, the same budget discipline the pump uses).
- **Single spine preserved (BC-2):** routing, activation synthesis, click-to-activate, update routing (`flair_route_updates`) all remain in `process.c`, reached identically for compiled-in and disk tenants. The gate adds transport, not policy.
- Compiled-in tenants (HELLO/NOTES) are untouched: their direct `procs->event` delivery continues byte-identically; `disk_tenant_event` only exists on disk-tenant `FlairApp`s.
- **Idle slices:** `AX=0x0011 FLAIR_IDLE(sleepTicks)` parks with a guaranteed `nullEvent` wake — the caret-blink path — resumable identically.

**Why not push-only callbacks** (tenant registers an `event` function pointer and just sleeps forever): it would work and is simpler, but it forfeits the period-authentic app-authored `WNE` loop that D2's Pascal apps want to *read like* Inside Macintosh, and it would make `idle`/modal timing kernel-driven. The stack-swap cost is bounded and reuses the most battle-tested asm in the tree. *(If the committee judges the resume asm too risky for R3.7's window, the documented fallback is push-callbacks with the same gate codes minus `WNE` — but the ruling stands as written.)*

## D1.5 Exit, crash, and the initech-8fhu close-semantics trap

- **Clean exit:** `AX=0x0070 FLAIR_EXIT(rc)` sets `state = FLAIR_APP_DYING` and permanently swaps to the kernel stack, then runs **`FlairProcess_terminate`** — `close()` hook, refCon-matched `DisposeWindow` sweep accruing exposure damage, successor promotion, LIFO frees (`process.c` :: `teardown_common`), then frees the code-image block last. Serial: `TENANT-EXIT rc=<rc>`.
- **Crash:** a CPU fault taken while the parked/current stack pointer lies inside a registered tenant's image range (checked by the panic/fault path against each resident disk tenant's `[imageBase, imageBase+imageLen)`) routes to **`FlairProcess_kill`** — never `close()`, never re-entering tenant code — plus serial `TENANT-CRASH name=… vec=…`, then desktop recomposite. AC-2's records-arena separation is precisely what makes this survivable; the design leans on it deliberately.
- **Close box:** `inGoAway` must resolve the owning tenant via the refCon demux and call `FLAIR_EXIT` semantics — **not** the shell-window `HideWindow` path that `flair_live_do_close` uses today. That `HideWindow`-based close is exactly the shape of the `initech-8fhu` defect (closed foreground app left stale in the process list, keeping key focus): the window vanished from the z-order while `list->head` still pointed at the tenant, so `keyDown` kept routing to it. The design rule: **any close of a *tenant-owned* window terminates the tenant through `terminate`/`kill`; `HideWindow` is reserved for shell furniture.** Teardown's unlink-and-promote (`teardown_common` step 5, promoting `succ` and raising its group) is what actually restores keyboard focus; the mutant/oracle in D1.7 pins it.
- **Menu-band ownership after death:** the foreground swap path already repaints band-2 from `list->head->menubar`; termination promotes the survivor, so the existing `flair_live_finish_tenant_switch` covers it. A disk tenant's `MenuBar` structures live in its image and die with it — the survivor's menubar pointer is what gets drawn, never the corpse's.

## D1.6 Memory: honest tiering

- **Today:** one program region `[PROGRAM_IMAGE, PROGRAM_STACK_BOT)`, single-level EXEC (`g_load_active`), per-EXEC disjoint AH=48h arena ending at `ENV_BLOCK` (`loader_prepare_core`). The FLAIR heap `[0x100000, 0x500000)` (4 MiB extended RAM, gated at boot by `flair_heap_ram_ok`) holds the scene, tenants' AC-2 partitions, the offscreen, and the resource/scrap services.
- **Ruling (D1-3), V1: one resident disk GUI tenant at a time, code image carved from the FLAIR heap.** Enforcement: a kernel-resident `g_disk_tenant_slot` (NULL = free); `FLAIR_REGISTER` with the slot occupied fails loud (`TENANT-SLOT-BUSY`), and the desktop's double-click path reports the same. This is the legitimate-V1 the brief anticipates, and it is *more* than a stopgap: placing the image in the FLAIR heap (rather than squatting `PROGRAM_BASE`) keeps the DOS program region free for concurrent class-2 text-tenant EXEC (the SAMIR hotkey suspends the desktop and runs a `.COM` through `PROGRAM_IMAGE` today; a GUI tenant parked in the FLAIR heap does not collide with that, whereas one parked at `PROGRAM_BASE` would).
- **Budget arithmetic:** HELLO/NOTES-scale tenants need ~16 KiB RECORDS + small DATA (AC-2 sizing notes in `process.h`). Code images for Calculator/Note-Pad-class Pascal apps are tens of KiB. Against a 4 MiB heap already carrying a 640×480×8 offscreen (300 KiB) and the scene, several MiB of tenant headroom exists. Fail-loud on exhaustion via the existing `_launch` reverse-carve reclaim.
- **V2 (post-V1, same design, no ABI change):** the slot becomes a short array of slots; each load picks a fresh heap block; `mz_apply_relocs(load_base = block)` already varies per instance. What genuinely does *not* generalize yet is the **DOS heap**: `int21_mcb_bind_program` binds one global arena per EXEC, so two simultaneously-resident tenants doing AH=48h would contend for one window. V2's honest scope statement: N GUI tenants with kernel-carved arenas, DOS-heap-per-tenant deferred. This limitation is stated here so nobody discovers it at R4.
- Reproducibility (Rule 11): load bases derive deterministically from heap allocation order at boot; the launch trace oracle pins them.

## D1.7 Oracle set

All graded on the booted 386 (QEMU dev loop; Bochs legs at phase close per Rule 5), serial-marker driven, mutation-proven per Rule 6:

- **Serial launch trace (positive):** `TENANT-LOAD name=CALC.EXE len=<n> base=0x…` → `TENANT-REGISTER ok handle=…` → `TENANT-EVT what=…` lines from the gate dispatcher → `FLAIR-DISPATCH app=<name>` on the switch path → `TENANT-EXIT rc=0`. Locked as a golden byte sequence; the `record-flair` **app_launch clip** (R6 list) drives it end-to-end: double-click inject → window appears → click inside → clean exit.
- **Disk-shipped fixture tenant:** `TENANTFIX.EXE` — minimal InitechMZ (hand-assembled or seed-built) that registers, opens one window, draws one text run, exits on first `mouseDown`. Doubles as the D2 smoke-tenant skeleton.
- **Mutants:**
  - `NO_REGISTER` — tenant entry skips the `FLAIR_REGISTER` call. Expected red: no window, `TENANT-RUNAWAY` fail-loud marker (the loader arms a watchdog: a tenant that touches neither the gate nor exits within a tick budget is killed via `kill` and reported), desktop intact.
  - `DOUBLE_LAUNCH` — second `FLAIR_REGISTER` while the slot is occupied (or a second double-click). Expected red: `TENANT-SLOT-BUSY`, first tenant undisturbed, no partial install (mirrors `O-4`'s never-partial-install assertion).
  - `EXIT_LEAK` — `FLAIR_EXIT` skips the teardown frees. Expected red: master-heap `avail` drifts downward across launch/exit cycles — the direct analogue of the `TEARDOWN_MUT_LEAK_*` guards and the O-3 avail-stable invariant, now measured on-metal via a `TENANT-HEAPAVAIL n=` serial line.
  - `CLOSE_STALE_FOCUS` (pins the 8fhu lesson): close box hides the window without terminating. Expected red: post-close keystroke still reaches the dead tenant's buffer (`TENANT-EVT` after `TENANT-EXIT` absent) — must be impossible in the clean build because teardown unlinked the list head.

### DECISIONS — Part D1

- **D1-1:** Disk GUI tenants are **InitechMZ flat-32 `.EXE`** (tagged, dword-relocated, dispatched by content per DEC-08a.4); flat `.COM` remains the text-tenant format. *(ADR-text-pending: confirm DEC-08a wording accommodates heap-based load bases.)*
- **D1-2:** The FLAIR API gate is a **single INT trap, vector 0x81, multiplexed by AX**, cdecl-on-trapped-stack arguments, result/error in EAX, per-call serial trace. Chosen over a fixed jump-table page (memory-map cost, no DOS precedent) and an entry pointer block (ABI fragility).
- **D1-3:** V1 memory model: **one resident disk GUI tenant**, code image carved from the master FLAIR heap (keeping `PROGRAM_BASE` free for text-tenant EXEC), AC-2 child arenas as today; multi-slot V2 is an array-of-slots change with DOS-heap-per-tenant explicitly deferred.
- **D1-4:** Cooperative yield is **park/resume over `FLAIR_WNE`**: tenant stack swapped out on park, resumed by `disk_tenant_event` installed as the App Contract `procs->event` — preserving the BC-2 single dispatch spine and giving Pascal apps an authentic WaitNextEvent loop.
- **D1-5:** Tenant exit is `FLAIR_EXIT → FlairProcess_terminate`; crash → `FlairProcess_kill` (never re-run dead code); close box on a tenant window **terminates the tenant** (the anti-8fhu rule) rather than `HideWindow`.
- **D1-6:** Oracle set: locked serial launch trace + `app_launch` clip + `TENANTFIX.EXE` fixture; mutants `NO_REGISTER`, `DOUBLE_LAUNCH`, `EXIT_LEAK`, `CLOSE_STALE_FOCUS`.

---

# PART D2 — The TPS Pascal Toolbox binding (R5.1/R5.2)

## D2.1 Minimum Toolbox surface (Calculator / Note Pad bar)

| # | Need | Trap (AX) | Existing C API it wraps | Notes |
|---|------|-----------|--------------------------|-------|
| 1 | Register + stay resident | 0x0001 / 0x0002 | `FlairProcess_launch` steps (D1.3) | shared with D1 |
| 2 | Open window | 0x0020 `FLAIR_NEWWINDOW(l,t,r,b,goAway) → h` | `NewWindow(wm,…)` (`window.h`) | kernel allocates the WindowRecord + region trio from the tenant's RECORDS arena; bounds in tenant DATA-space struct on stack |
| 3 | Close window / quit | 0x0021 / 0x0070 | `DisposeWindow` / `FlairProcess_terminate` | |
| 4 | Set title | 0x0022 `FLAIR_SETWTITLE(h, strPtr)` | title plumbing — **kernel seam needed**, see D2-3 | ShortString → ASCIZ copy into kernel-held storage |
| 5 | Get next event | 0x0010 `FLAIR_WNE(mask, sleepTicks) → bool` | park/resume (D1.4); event lands in tenant-registered `wneBufPtr` | the Pascal `EventRecord` is a *flat* record (D2-2) |
| 6 | Draw text | 0x0030 `FLAIR_TEXTDRAW(h, x, y, strPtr, fgIdx, bgIdx)` | `text_draw(bm,…, FONT_CHICAGO, fg, bg)` | indices, never RGB — C-8 discipline |
| 7 | Fill rect | 0x0031 `FLAIR_FILLRECT(h,l,t,r,idx)` | `blitter_fill_rect_clipped` | |
| 8 | Frame rect | 0x0032 `FLAIR_FRAMERECT(h,l,t,r,idx)` | 4× `blitter_fill_rect_clipped` | |
| 9 | Make control (button/check/radio) | 0x0040 `FLAIR_CTRLNEW(h,type,l,t,r,titlePtr,val,min,max) → ch` | `control_init` + tenant-RECORDS storage | |
| 10 | Draw controls | 0x0041 `FLAIR_CTRLDRAW(h)` | `DrawControl` | |
| 11 | Hit/test + track control | 0x0042 `FLAIR_CTRLTEST(ch, x, y) → part` / 0x0043 `FLAIR_CTRLTRACK(ch) → part` | `TestControl` / `TrackControl` (kernel supplies the tracked point sequence from the live mouse, exactly as kmain's drag/menu loops do) | |
| 12 | Set menu bar | 0x0050 `FLAIR_SETMBAR(barPtr)` | `app->menubar = ptr` + `flair_live_finish_tenant_switch` redraw | `barPtr` points into the tenant's *relocated image* — MZ fixups make tenant-resident `MenuBar`/`MenuItem` graphs directly consumable |
| 13 | Menu select (tracking done by kernel) | 0x0051 `FLAIR_MENUSELECT(x,y) → (menuID<<16|item)` | `flair_menu_track` with kernel-captured `pts` (the kmain `flair_live_do_menu_at` capture loop, factored into a reusable helper) | tenant never sees intermediate points |
| 14 | Scrap put/get/zero/info | 0x0060–0x0063 | `FlairPutScrap` / `FlairGetScrap` / `FlairZeroScrap` / `FlairInfoScrap` | V1 payload via ShortString (≤255) — see D2-4 |
| 15 | Exit | 0x0070 | D1.5 | |

Deliberately **out** of the minimum: TextEdit/List, Standard File dialogs, Resource Manager from Pascal, Dialog Manager (alerts) — R4 hosts those in C first; Pascal bindings ride a later increment.

## D2.2 Expressing the surface in the *current* TPS subset

Verified against the accepted grammar in evidence (`os/tps/tps.pas` declarations head; `TPS-M7-subset-plan.md` §2/§3): integer, boolean, char, ShortString, const, static arrays, records with scalar fields, procedures/functions, value **and var** params, recursion, `forward`, complete boolean evaluation. No pointers, no units, no `@`/`addr`, no procedural types.

Every construct the binding needs maps onto that subset with **zero subset growth**:

- **Opaque handles = `integer`.** `WindowPtr`, `ControlHandle`, menu results, scrap counts — all `LongInt` tokens. The kernel never requires the tenant to dereference them.
- **`EventRecord` = flat record of scalars, passed by `var`.** The kernel-side `EventRecord` (`what/message/modifiers/when` + `where.h/.v`) is copied field-wise into the tenant's registered flat buffer:

```pascal
type
  FlairEvent = record
    What, Message, Modifiers, When: integer;
    WhereH, WhereV: integer
  end;

function FLAIR_GetNextEvent(Mask: integer; var Ev: FlairEvent;
                            SleepTicks: integer): boolean;
```

  A *nested* `where: record h,v end` was rejected deliberately: the subset's record rules are specified for scalar fields (`tps.pas` semantic reference map: "named-record scalar fields"), and flattening costs nothing while avoiding a grammar question entirely.
- **Strings = ShortString, by value or `var`.** Stub layer converts ShortString ⇄ ASCIZ in static RTL buffers (`fileio.asm` :: `rtl_file_assign` already demonstrates the length-byte-strip + `rep movsb` pattern for exactly this).
- **Rects** are four scalar params, not a record — fewer marshalling rules, same ABI.
- **Arrays by `var`** cover any bulk need (none in the V1 surface).
- **cdecl externals:** the mechanism demonstrably exists — the B8 builtins link to hand-assembled `rtl_file_*` symbols through the same toolchain (`fileio.asm` globals; seed codegen emits calls to them; TPS mirrors seed per the divergence note). `FLAIR.PAS` declares its routines the same way. *(Flag: the exact external-declaration spelling must be lifted from the B8 builtin declarations in `seed/typecheck.c` / the TPS mirror — not in corpus; mechanical, not design.)*

**Forced assumptions (all verified-safe, none require subset growth):** (i) external cdecl routine declarations exist (B8 precedent); (ii) ShortString→ASCIZ conversion lives in RTL stubs, not language; (iii) the flat `FlairEvent` record is a plain scalar-field record; (iv) boolean returns map to AL. **No pointer, unit, `@`, or nested-record extension is invoked.**

## D2.3 The one kernel seam D2 needs: window titles

`NewWindow` takes no title and the corpus shows titles living outside the WindowRecord (`fd_win_titles[]` + a `title` parameter on `flair_draw_document_window`). For `FLAIR_SETWTITLE` to mean anything, the kernel needs title storage reachable by the WDEF draw path. **Ruling (D2-3): add a title field (fixed 32-byte inline buffer, or pointer into the tenant RECORDS arena) to the shell's window bookkeeping and thread it into `flair_draw_document_window`/`draw_titlebar_band` from there** — a small, additive artifact change, declared here as a required R3.7/R5.1 work item rather than discovered mid-flight. *(ADR-text-pending: confirm `spec/window_record.h` doesn't already carry a title the corpus omitted.)*

## D2.4 Scrap payload ceiling

`FLAIR_SCRAP_MAX_PAYLOAD = 8192` (`scrap.h`, Rule-8-locked) exceeds ShortString's 255. **Ruling (D2-4): V1 Pascal bindings expose scrap flavors capped at 255 bytes (ShortString payloads); the full 8 KiB surface waits for a Pascal `array [1..8192] of char` var-param overload — which the subset already supports — added only when a Pascal app actually needs it.** Note Pad and Calculator fit comfortably in 255.

## D2.5 The stub layer

Hand-assembled NASM, `bits 32`, in the `start_dos.asm` lineage: one object `flairstub.asm` linked into seed-compiled tenant images (and, later, shipped beside TPS output), mirroring `fileio.asm`'s discipline exactly — cdecl frames, `ebx/esi/edi/ebp` preserved, result in `EAX`, errors negative. One stub per trap; names `FLAIR_*`. The uniform argument rule (D1-2) makes every stub trivially regular: set `AX`, `int 0x81`, widen result.

Three stubs in full:

```asm
; flairstub.asm -- FLAIR Toolbox gate stubs (D2). Ref: seed/rt/fileio.asm
; (register discipline), seed/rt/start_dos.asm (runtime lineage),
; docs/plans/GUI-remediation-plan.md R5.2, ADR-0007 DEC-05.
bits 32

FLAIR_TRAP_GATE equ 0x81          ; D1-2: the locked gate vector

; ---------------------------------------------------------------------
; function FLAIR_GetNextEvent(Mask: integer; var Ev: FlairEvent;
;                             SleepTicks: integer): boolean;
; Trap AX=0x0010. Args remain on the trapped stack (D1-2); the kernel
; cooks the next event addressed to this tenant into the registered
; wneBufPtr and resumes us with AL = 0/1.
; ---------------------------------------------------------------------
global FLAIR_GetNextEvent
FLAIR_GetNextEvent:
        push ebp
        mov  ebp, esp
        push ebx
        push esi
        push edi
        mov  eax, 0x0010           ; AX = FLAIR_WNE
        int  FLAIR_TRAP_GATE
        movzx eax, al              ; boolean result
        pop  edi
        pop  esi
        pop  ebx
        pop  ebp
        ret

; ---------------------------------------------------------------------
; procedure FLAIR_TextDraw(Win, X, Y: integer; const S: string;
;                          FG, BG: integer);
; Trap AX=0x0030. ShortString S is converted to ASCIZ in a static RTL
; buffer first (the rtl_file_assign idiom: strip the length byte,
; rep movsb, NUL-terminate); EDX = ASCIZ pointer is pushed as a 4th
; stack arg so the kernel reads one uniform cdecl frame.
; ---------------------------------------------------------------------
section .data
flair_tmp_asciz: times 256 db 0

section .text
global FLAIR_TextDraw
extern pas_main                        ; (lineage anchor; unused here)
FLAIR_TextDraw:
        push ebp
        mov  ebp, esp
        push ebx
        push esi
        push edi
        ; -- convert ShortString at [ebp+20] to ASCIZ --
        mov  esi, [ebp+20]
        xor  ecx, ecx
        mov  cl,  [esi]                ; length byte
        inc  esi
        lea  edi, [flair_tmp_asciz]
        cld
        rep  movsb
        mov  byte [edi], 0
        ; -- rebuild the stack frame with the ASCIZ pointer as arg4 --
        sub  esp, 4                    ; room for injected pointer
        mov  eax, [ebp+8]              ; Win
        mov  [esp],   eax
        mov  eax, [ebp+12]             ; X
        mov  [esp+4], eax
        mov  eax, [ebp+16]             ; Y
        mov  [esp+8], eax
        lea  eax, [flair_tmp_asciz]
        mov  [esp+12], eax             ; ASCIZ S
        mov  eax, [ebp+24]             ; FG index
        mov  [esp+16], eax
        mov  eax, [ebp+28]             ; BG index
        mov  [esp+20], eax
        mov  eax, 0x0030               ; AX = FLAIR_TEXTDRAW
        int  FLAIR_TRAP_GATE
        lea  esp, [ebp-12]             ; discard rebuilt frame
        pop  edi
        pop  esi
        pop  ebx
        pop  ebp
        ret

; ---------------------------------------------------------------------
; function FLAIR_PutScrap(Flavor: integer; const S: string): integer;
; Trap AX=0x0060. Returns 0 on success, negative FLAIR_SCRAP_ERR_* on
; failure (scrap.h convention). Errors are RETURNED, never fatal: a
; failed put must not take the tenant down (Rule 2 adapted for apps).
; ---------------------------------------------------------------------
global FLAIR_PutScrap
FLAIR_PutScrap:
        push ebp
        mov  ebp, esp
        push ebx
        push esi
        push edi
        mov  esi, [ebp+12]
        xor  ecx, ecx
        mov  cl,  [esi]
        inc  esi
        lea  edi, [flair_tmp_asciz]
        cld
        rep  movsb
        mov  byte [edi], 0
        sub  esp, 8
        mov  eax, [ebp+8]              ; Flavor (FLAIR_OSTYPE value)
        mov  [esp],   eax
        lea  eax, [flair_tmp_asciz]
        mov  [esp+4], eax              ; data ptr
        mov  eax, ecx                  ; length (original ShortString len)
        mov  [esp+8], eax
        mov  eax, 0x0060               ; AX = FLAIR_PUTSCRAP
        int  FLAIR_TRAP_GATE
        lea  esp, [ebp-12]
        pop  edi
        pop  esi
        pop  ebx
        pop  ebp
        ret
```

(The `sub esp`/rebuild idiom is written out long-hand above for clarity; the implementing seat may substitute a fixed local frame — the *behavioral contract* is the trap id plus the marshalled argument block, not the instruction text.)

## D2.6 Compile/link story — one behavioral contract, two compilers

**Ruling (D2-5): the contract between seed-compiled and (later) TPS-compiled tenants is the ordered trap-call sequence `(AX, marshalled args)`, never the emitted assembly.**

- **Seed now:** new linker script `seed_flair.ld` (sibling of `seed_dos.ld`) emits an **InitechMZ container**: MZ header with `e_res[0] = MZ_INITECH_TAG`, load-module layout, and a relocation table covering every absolute dword the image contains. Factory-side, the cheapest correct producer is `ld --emit-relocs` plus a small factory C tool that wraps the relocs into the MZ header (factory = C, Law 3). Alternative rejected: teaching the seed backend image-relative addressing — more invasive, no benefit.
- **TPS later (resident rail):** the B9.4 generator (`GenText` buffered assembly → `TPSOUT.S`) gains an MZ-wrap + relocation pass over its own output. Identical trap sequences fall out automatically because both front-ends compile the *same* `FLAIR.PAS` against the *same* stub object.
- **Verification of "identical sequences":** the kernel gate dispatcher's serial trace (`TENANT-GATE ax=0x… args…`) from the OS run is diffed against the host differential's trace (next section). Byte-identical trace = the contract held; this is behavioral agreement, computable on both rails.

## D2.7 Oracle: the Pascal smoke tenant and the fpc differential

- **Smoke tenant (`SMOKETPS`):** opens one window, sets a title, draws one text run, creates one button, receives exactly one click (`FLAIR_WNE` → `mouseDown` → `FLAIR_CTRLTRACK` hit → exit). Runs on the booted 386; serial markers `TENANT-*` plus the gate trace constitute the golden.
- **Host differential, honestly specified:** the same smoke-tenant Pascal source is compiled by **fpc on the host** against a declared host harness unit `flair_host.pas` that implements every `FLAIR_*` routine as a **printer**: it appends one line `TRAP ax=0x<nn> <marshalled scalars>` to a log and returns canned values (window handle 1, one synthetic mouseDown, etc.). This is a *host print harness*, declared as such — it grades the **sequence of Toolbox requests the program makes**, not pixels, and it is factory-side (Law 3 clean: no host code ships in the artifact).
- **The differential:** `trace(fpc-host run of SMOKETPS) == trace(seed-built SMOKETPS executed on the booted OS)` — the latter captured from the gate dispatcher's serial trace. Where the two compilers legitimately differ (e.g., evaluation-order-neutral source mandated by the complete-evaluation rule), the smoke tenant is written order-independent so traces coincide exactly. Mutation-proofing (Rule 6): a `SMOKE_MUT_SKIP_CLICK` variant that drops the click handling must make the trace differential red.
- **Tri-emulator discipline** applies to the OS leg at phase close (QEMU dev; Bochs gate), per Rule 5.

## ADR-0007 amendment text (draft DEC block, ready for ratification)

> **DEC-XX — The FLAIR Pascal binding (R5.1/R5.2).**
> Turbo Initech-compiled applications reach the FLAIR Toolbox exclusively through the **Initech Toolbox Gate**: interrupt vector `0x81`, function code in `AX`, cdecl arguments on the trapped stack, result/error in `EAX` (GUI-remediation D1-2/D2 rulings; vector and AX map are locked spec-data, Rule 8). Disk GUI tenants are InitechMZ flat-32 images relocated to a FLAIR-heap load base (DEC-08a machinery reused; D1-1/D1-3). The binding surface is expressed entirely within the ratified M7 subset — opaque handles as integers, flat scalar-field `FlairEvent` records by `var`, ShortString parameters converted to ASCIZ in RTL stubs — with **no subset growth** (no pointers, units, `@`, or nested records; D2-2). Trap stubs are hand-assembled NASM in the `start_dos`/`fileio` RTL lineage, one stub per trap, names `FLAIR_*`, preserving `ebx/esi/edi/ebp`, errors returned not fatal (D2-5 exemplars). The cross-compiler behavioral contract is the ordered trap-call sequence, graded by gate-trace differential between the seed rail and fpc-host with a declared host print harness (D2-6/D2-7); textual assembly identity is explicitly NOT the contract. V1 scrap payloads from Pascal are capped at ShortString length (255) pending an array-based overload (D2-4). Cooperative tenancy is park/resume over `FLAIR_WNE` with the Layer-5 dispatch spine unchanged (BC-2 preserved; D1-4).

### DECISIONS — Part D2

- **D2-1:** Minimum Pascal-visible Toolbox surface = the 15-trap inventory of §D2.1; TextEdit/List/StandardFile/Resource-from-Pascal deferred past V1.
- **D2-2:** Zero subset growth: handles = integers; `FlairEvent` = flat scalar-field record by `var`; strings = ShortString converted in stubs; external cdecl via the existing B8 linkage mechanism. No pointers/units/`@`/nested records.
- **D2-3:** One additive kernel seam required: window-title storage reachable by the WDEF path, backing `FLAIR_SETWTITLE` (flagged because `NewWindow` carries no title in the current API).
- **D2-4:** V1 Pascal scrap payloads capped at 255 bytes (ShortString); array-based overload deferred until needed.
- **D2-5:** Stub layer = `flairstub.asm`, one regular stub per trap, `fileio.asm` register discipline; cross-compiler contract = ordered trap-call sequence, not assembly text; seed rail via `seed_flair.ld` + factory MZ-wrapper tool, TPS rail via a generator MZ/reloc pass.
- **D2-6:** Differential oracle = gate-trace equality between fpc-host (declared print harness) and the on-metal seed-built tenant; smoke tenant `SMOKETPS`; `SMOKE_MUT_SKIP_CLICK` as the Rule-6 mutant.

---

# PART D3 — Platinum pixel-gap audit (supporting R2)

**Method and limits.** Graded from `os/flair/chrome.c`, `menu.c`, `control.c` against `../system7-decomp/specs/sys8/{window-chrome,menus,scrollbars,controls,platinum-palette}.md` (sampled column throughout, per the brief and per `platinum-palette.md` §1). Because `spec/chrome_metrics.h` and `os/flair/flair_look.h` (the `FLAIR_PART_*` → pixel table) are not in the corpus, rows whose correctness reduces to "does part X resolve to sampled value Y" are marked **VERIFY** — the geometry is readable, the color resolution is not. This is a corpus limitation, not a hedge: each VERIFY row names the exact constant/table to check. Line numbers unavailable (§0.2); anchors are function/section.

Legend: **DONE** (matches spec), **WRONG** (rendered, contradicts spec — how stated), **MISSING** (spec'd, not rendered), **VERIFY** (geometry right, color/constant unproven here), **NOTE** (spec-side gap or deliberate interim).

## D3.a Window chrome (`chrome.c` vs `window-chrome.md`, `scrollbars.md`) — work-list bead **tdnl.6**

| # | Element | Spec anchor | Status | Detail |
|---|---------|-------------|--------|--------|
| 1 | Struct 1-px outline, all sides | wc §1 | DONE | final frame loop in `flair_document_window`; guarded by `CHROME_MUTATE_NO_FRAME` |
| 2 | Drop shadow: black, +1/+1, 2-px near-corner notch | wc §1 | DONE | `FLAIR_CHROME_SHADOW_NOTCH`; col from `top+notch`→`bottom`, row from `left+notch` (matches "+2 notch" rule); VERIFY notch==2 |
| 3 | Inactive shadow `#777777` | wc §1 | DONE | `frame_part = PLAT_INACTIVE_FRAME` when !active |
| 4 | Active title band 22 px: frame/hl/face×2/stripes×12/face×4/shadow/frame | wc §2.1 | DONE | exact row profile in `draw_titlebar_band`; VERIFY `FLAIR_CHROME_TITLEBAR_H==22` |
| 5 | Stripe colors light/dark | wc §2.2 | VERIFY | parts `CONTENT`/`PLAT_STRIPE_DARK`; must resolve to sampled `#FFFFFF`/`#969696` (nominal FF/77) |
| 6 | **Stripe 1-px row-parity x-offset** (dark rows shifted +1 at both ends) | wc §2.2 | **MISSING** | both parities fill uniform `w-2`; spec's stroke-offset mechanism not reproduced |
| 7 | Title text centered, active ink black | wc §2.3 | DONE | centering + `FLAIR_PART_TEXT` |
| 8 | Inactive title ink `#878787` | wc §6 | DONE | `PLAT_INACTIVE_TEXT`; VERIFY mapping |
| 9 | System font = **Charcoal** | wc §2.3 | **WRONG** | all chrome/menu/control text uses `FONT_CHICAGO`; sys8 titles/items are Charcoal. Needs a ruling: author a Charcoal strike, or ratify Chicago substitution as a documented era-substitution (the OQ-2 teal precedent) |
| 10 | Text-gap fill `#DADADA` | wc §2.3 | VERIFY | text bg = `PLAT_FRAME_FACE` on active — must be the DADADA-role gray |
| 11 | Widget placement: close L+4, zoom R−32, collapse R−16 (collapse RIGHTMOST) | wc §3.1 | DONE | `CLOSE_LEFT_OFF/ZOOM_RIGHT_OFF/COLLAPSE_RIGHT_OFF`, collapse drawn last/rightmost; VERIFY constants 4/32/16 |
| 12 | Widget anatomy: 12×12, `#A5A5A5` top/left edge, `#3F3F3F` ring at rows/cols 1&11, white outer hl right/bottom, white interior corner px, `#A5A5A5` interior bottom/right, 7-rung diagonal gradient keyed on dx+dy | wc §3.2 | DONE | `draw_platinum_widget` reproduces every element incl. `rung=(dx+dy-2)/PX_PER_STEP`; VERIFY ramp part table resolves to nominal `99…FF` ladder and `PX_PER_STEP==2` |
| 13 | Zoom glyph: right edge dx=5 + bottom edge dy=5 only | wc §3.3 | DONE | `PLAT_WIDGET_ZOOM` branch; VERIFY `ZOOM_GLYPH_EDGE==6` |
| 14 | Collapse glyph: full-width dark rows dy=3, dy=5 | wc §3.3 | DONE | `COLLAPSE_GLYPH_ROW_0/1`; VERIFY 3/5 |
| 15 | Close glyph absent (bare gradient) | wc §3.3 | DONE | no close branch |
| 16 | Per-window widget flags (CP window: close+collapse, NO zoom; collapse keeps R−16 slot) | wc §3.1 | **MISSING** | every document window draws all three widgets; no variant/flags plumbing |
| 17 | 4-px raised body rail `W dd dd b` per side, each strip lit as its own bar (right border: white INNER, b3 OUTER) | wc §4 | DONE | `draw_body_structure` left/right/bottom strips in the W/face/face/shadow order; VERIFY exact column offsets vs frame line (off-by-one risk noted in analysis) |
| 18 | Content inset: white top/left, `#C0C0C0`(nominal AA) bottom/right | wc §4 | DONE | `PLAT_CONTENT` top/left + `PLAT_WELL` bottom/right in `draw_body_structure`; VERIFY WELL→AA-nominal |
| 19 | Grow box 18×18 active: `#DADADA` fill, white top/left hl, 3 grip lines pitch 4 (white lead + `#969696` trail), `#C0C0C0`+`#969696` lower-left terminators | wc §5 | DONE | `draw_grow_box`; pitch math verified (line spacing = 4 px along any row); VERIFY `FLAIR_CHROME_GROW==18` |
| 20 | Grow box inactive: flat `#E7E7E7`, no grip, no hl | wc §6 | DONE | early-return inactive branch |
| 21 | Scroll gutters 16 px, right + bottom placement | sc §1 | DONE | sb_left/grow geometry in `flair_document_window`; G8 fixes (`javs`/`l0mh`) presumed landed; VERIFY constants |
| 22 | Gutter bounding lines: black active / `#777777` inactive | sc §1 | DONE | `frame_part` selection |
| 23 | **ENABLED page well: 5-value cross-section** `96/A5/C0×10/CD/DA` + near-end 2-px dark inset, far end unstroked | sc §2.3 | **MISSING** | `draw_vertical/horizontal_scrollbar` fill a single flat `PLAT_TROUGH`; no 5-value shading, no insets |
| 24 | Raised arrow tiles: `#FFFFFF` hl, `#CDCDCD` shadow, `#E7E7E7` face | sc §2.2 | **WRONG** | triangles drawn directly on the trough; no tile bevel at all |
| 25 | Arrow triangles: 8 base × 4 deep, widths 2/4/6/8 | sc §2.2 | DONE | `draw_up/down/left/right_triangle` shapes match; color role VERIFY (`WIDGET_EDGE` vs black) |
| 26 | Arrow-box separators: **black** in the active layout | sc §2.1 | **WRONG** | drawn with `PLAT_INACTIVE_FRAME` (`#777777` role) in the ACTIVE bar |
| 27 | **Accent thumb: 15 px, `clut` 208 ramp (DA/B3/87/54 entries 1–4), 4 grip lines + companions** | sc §2.4 | **MISSING** | chrome gutters draw no thumb whatsoever |
| 28 | DISABLED bar state: flat `#F3F3F3` trough, `#777777` separators, `#A5A5A5` arrows, **no thumb** | sc §3 | **MISSING** | chrome has no disabled concept; active bars always render enabled-looking wells |
| 29 | HOLLOW bar (inactive window): `#F3F3F3` trough, `#777777` frame, empty | sc §4 | DONE | inactive branch |

## D3.b Menu bar + panels (`menu.c` vs `menus.md`) — bead **sjvq**

| # | Element | Spec anchor | Status | Detail |
|---|---------|-------------|--------|--------|
| 30 | Bar 3-D profile: `#FFFFFF` row0, `#E7E7E7` ×17, `#B3B3B3` row18, black row19 | m §1.1 | **WRONG** | `DrawMenuBar` paints flat bg + 1-px baseline only; the white top row and B3 shadow row are absent |
| 31 | Rounded ~5-px corners, black pixels outside the round | m §1.2 | **MISSING** | bar drawn edge-to-edge square |
| 32 | Apple logo: full-colour rainbow apple | m §1.3 | **WRONG** | `apple_glyph.h` masked blit is monochrome `fg`; spec samples 4 rainbow colors. Interim monochrome may be ratifiable as substitution — decide explicitly |
| 33 | Proportional title layout | m §1.3 | DONE | `title_slot_w` cumulative via `text_measure` |
| 34 | Pulled-down title block: accent fill (`clut`208 rows: `87/54/00-A5` = entries 3/4/5) + white title text, 10-px side padding | m §1.4 | **MISSING** | `HiliteMenu` returns the rect but nothing ever fills it; `DrawMenuBar` has no hilited-title state; no accent plumbing |
| 35 | Panel black 1-px frame | m §2.1 | DONE | `fr_*` fills in `flair_draw_menu_panel` |
| 36 | Panel inner highlight `#FFFFFF` top/left | m §2.1 | **MISSING** | |
| 37 | Panel inner shadow `#B3B3B3` bottom/right | m §2.1 | **MISSING** | |
| 38 | Panel drop shadow `#3F3F3F` 1 px right/bottom (dark-gray, NOT window-black) | m §2.1 | **MISSING** | |
| 39 | Item pitch 16 px | m §2.2 | DONE | `FLAIR_MENU_ITEM_H==16` confirmed in menu.h |
| 40 | Text top = item rect +2; first item rect top = panel+2 | m §2.2 | **WRONG** | text drawn at `row_top` (+0) and rows begin at `panel.top+PANEL_FRAME` (+1); spec wants +2/+2 |
| 41 | Separator item height **6 px** | m §2.2 | **WRONG** | `FLAIR_MENU_DIV_H == 8` (menu.h) |
| 42 | Etched separator groove: `#A5A5A5` then `#FFFFFF` at rows +1/+2, full interior span | m §2.3 | **WRONG** | drawn as a single 1-px fg rule centered in the 8-px band |
| 43 | Item text left edge = panel left + 20 | m §2.2 | **WRONG** | computed `panel.left + PANEL_FRAME(1) + ITEM_LPAD(16)` = +17 |
| 44 | Command-key column (~x199) with cmd glyphs dimming with their item | m §2.2/2.3 | **MISSING** | `MenuItem.cmdChar` parsed and matched by `MenuKey`, never drawn |
| 45 | Disabled item ink `#A5A5A5` | m §2.3 | **WRONG** | disabled items drawn in normal fg; selectability is enforced but invisibly |
| 46 | Submenu arrow / hierarchical panels | m §3 gap | **MISSING** (deferred) | no submenu support; consistent with the spec's own gap |
| 47 | Tracking hilite (item under cursor) | m §3 gap (state UNKNOWN in sys8) | NOTE/DONE | classic invert implemented; sys8 truth unresolved per spec gap — acceptable until a capture resolves it |

## D3.c Controls (`control.c` vs `controls.md`) — bead **81ft**

| # | Element | Spec anchor | Status | Detail |
|---|---------|-------------|--------|--------|
| 48 | Dialog/CP content face `#E7E7E7` + 1-px inset bevel (white TL, `#C0C0C0` BR) | c §1 | **WRONG** | control/dialog bodies use `CTRL_WHITE`/`CONTENT`; no inset bevel on dialog content |
| 49 | Checkbox: 12×12, 1-px black outline enclosing 10×10 **raised tile** (white TL interior hl, `#A5A5A5` BR shadow, `#E7E7E7` face) | c §2 | **WRONG** | plain white square + black frame; no tile shading, wrong face role |
| 50 | Check mark: black **X** with `#969696`+`#C0C0C0` drop shade | c §2 | **WRONG** | checked state = solid `CTRL_ACCENT` interior fill; no X, no shade |
| 51 | Unchecked checkbox art | c §2/§8 gap | **MISSING** | only a degenerate empty-square path exists; sys8 gap (all goldens checked) noted |
| 52 | Etched group box (`#A5A5A5` line + `#FFFFFF` companion +1/+1, title gap in top line) | c §3 | **MISSING** | no group-box type in `flair_ctrl_type_t` |
| 53 | Popup button (rounded frame, bevel, arrows well, dual triangles) | c §4 | **MISSING** | no popup type |
| 54 | Push button frame: rounded radius 2, corner smoothing pixels `#3F3F3F`/`#CDCDCD` | c §5.1 | **WRONG** | 4 corner pixels cleared to `CTRL_DESKTOP` — background shows through instead of smoothing ramp |
| 55 | Push button asymmetric bevel: `#E7E7E7` inset TL, `#FFFFFF` hl row/col, `#E7E7E7` face, 2-px `#C0C0C0`+`#969696` shadow BR | c §5.1 | **WRONG** | flat face + 1-px black frame only |
| 56 | Default ring: black rounded rect = InsetRect(button, −3,−3), 2-px moat carrying outer shadow shading | c §5.2 | **MISSING** | no ring rendering anywhere (`defaultItem` exists logically in dialogs, never drawn) |
| 57 | Pressed/tracking button state | c §8 gap | NOTE | accent-fill hilite implemented; sys8 pressed art is a golden gap |
| 58 | Modal alert frame: pink outer bevel `#FF9999`/`#FF6666` (nominal), neutral inner rings, severity icons | c §6 | **MISSING** | dialogs draw dBoxProc 7-px or movableDBox 1-px frames; no alert kind, no tint, no icons |
| 59 | Round help button (gray-framed, yellow ? glyph) | c §7 | **MISSING** | |
| 60 | Progress bar | c §8 gap | NOTE | sys8 never captured a progress bar; FLAIR's black-frame/white-bg/accent-fill is unverifiable against Platinum — leave to FILE COPY canon grading, do not invent Platinum art |
| 61 | Control-Manager scrollbar: 16-px band ✓; thumb **16 px** vs spec 15; non-proportional thumb; System-7 face | sc §1/§2; control.h header TODO_GOLDEN | **WRONG** | `SB_THUMB_MIN==16` and framed-gray/accent thumb contradict the Platinum accent-ramp anatomy; header itself flags the heritage face. Fix belongs with tdnl.7: adopt the sc §2.4 thumb (15 px, clut-208 ramp, 4 grips) + sc §2.3 well |
| 62 | Radio buttons | c §8 gap; IM-I heritage | NOTE | square-with-cleared-corners stand-in; no sys8 measurement exists — grade against System-7 heritage until a capture lands |

## D3.d Palette policy (cross-cutting)

| # | Item | Anchor | Status | Detail |
|---|------|--------|--------|--------|
| 63 | Sampled-vs-nominal discipline: an indexed-8 implementation programming the VGA DAC must use the **nominal** CLUT column; SSIM-vs-capture grading must use the **sampled** column | pp §1 | **VERIFY / RISK** | `flair_desktop_present` programs the DAC from `flair_palette_rgb` (kmain). If the FLAIR palette table was minted by sampling the s8 captures, every chrome gray is gamma-shifted and *every* row above is wrong at the DAC even when the part-routing is right. The palette table's provenance must be audited first; it is not in the corpus |

### DECISIONS — Part D3

- **D3-1:** The audit's first action item is **63 (palette provenance)** — a wrong DAC table invalidates every color row mechanically; audit before painting anything (HER-02 discipline: never grade against the wrong ground truth).
- **D3-2:** Highest-density WRONG clusters, in fix order: menus panel anatomy (36–38, 41–45 — six defects in one function, `flair_draw_menu_panel`) → scrollbars enabled-state anatomy (23–27) → push button + checkbox faces (49–55) → title-bar residuals (6, 9, 16).
- **D3-3:** Row 9 (Chicago vs Charcoal) and row 32 (monochrome vs rainbow Apple) are **policy rulings, not bugs**: ratify either authored-Charcoal/rainbow strikes or documented substitutions, in writing, before code moves.
- **D3-4:** MISSING-but-spec'd items (group box, popup, default ring, alert frame, help button, disabled scrollbar state, accent thumb) are new capability, assigned: scrollbars → `tdnl.6` (chrome gutters) + `tdnl.7` (Control Manager face); menus → `sjvq`; controls/alerts → `81ft`; window-body residuals → `tdnl.6`.
- **D3-5:** Rows marked NOTE (47, 57, 60, 62) track sys8 golden gaps, not FLAIR defects — they resolve when the missing captures exist (INDEX-sys8 §GAPS items 1–3), and must not be "fixed" by invention in the meantime.

---

*End of report. All rulings are drafted for verbatim lift into an ADR amendment subject to the §0 corrections (especially the absent ADR texts and the two flagged VERIFY families: `FLAIR_PART_*`/palette resolution and `chrome_metrics.h` constants).*
