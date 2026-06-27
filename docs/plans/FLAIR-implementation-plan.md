<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# FLAIR -- Comprehensive Build-Out Plan (canonical spec + boot integration + M5 frame + app-hosting)

**Issuing Body:** Initech Systems Corporation -- Platform Engineering, Presentation Layer Section (FLAIR)
**Document Class:** Implementation Plan (living; supersede in place)
**Programme / Milestone:** InitechOS (STAPLER) -- M3/M4/M5 + the app-hosting north star (PRD Sec 3, Sec 6.2-6.5, Sec 8)
**Governing ADRs:** ADR-0004 (FLAIR Toolbox, RATIFIED), ADR-0005 (ATKINSON region engine, RATIFIED). New ADRs this plan introduces: **ADR-0013 (FLAIR App Contract, Phase 4, RATIFIED 2026-06-27)**, ADR-0011 (App-Hosting / Initech-versions, Phase 6, PENDING). **Numbering note (2026-06-27):** this plan originally reserved "ADR-0010" for the App Contract, but ADR-0010 was used for the (unrelated) FLAIR Grading-and-Goldens ADR; the App Contract is therefore **ADR-0013** (driven by the charter expansion ADR-0012, PRD Sec 1.4). A new **Phase 4.5 "Platform Services"** sits between Phase 4 and Phase 5 (ADR-0012 D-2b).
**Governing epics:** `initech-k8o5` (Toolbox arch), `initech-rf2l` (goldens), `initech-ib6` (M5 apps) + the new phase epics filed by this plan.
**Last Reconciled:** 2026-06-20 (authored from the 7-lane FLAIR understand workflow `wf_e7335f49-a55` + committee/operator rulings below).

> Read order for an implementing agent: `CLAUDE.md` (Laws/Rules) -> `InitechOS-PRD.md` Sec 3 / Sec 6.2-6.5 -> `docs/adr/ADR-0004` + `ADR-0005` -> **the sister corpora `../system7-decomp` (BASE) + `../win31-decomp` (ACCENTS)** -> this plan -> `bd show <the phase epic>` and the step's bead. The oracle is the truth, not this plan (Law 2). Where a step is tagged **[OP]** the operator/committee has ruled; where **GATED**, a golden/acquisition is not yet settled -- loud-skip, never silent-pass.

---

## 1. Executive summary

The 7-lane understand workflow established the ground truth (and the critic cross-verified it): **FLAIR is a green host-tested Toolbox library that does not boot.** Only `surface.o` links into the kernel (`Makefile:7016-7023`); the entire Manager/imaging stack is host-rendered to PPM and **never runs on the emulated 386** (the booted OS shows seafoam + a console banner). The 16 green oracles prove geometry / region algebra / layout / event-replay -- **not rendered fidelity at boot**. Two structural gaps sit under that: (a) the locked spec (`grafport.h`/`imaging.h`) describes a full QuickDraw imaging stack the impl does not provide (no drawing verbs, no CopyBits, `grafProcs` set to NULL only, transfer modes / Pattern / pen-color inert) -- it is currently aspirational decoration; (b) there is **no app contract** -- no way for a tenant to own a window's content GrafPort, receive routed events, or be launched (`os/apps/`, `os/tps/` are empty).

The two sister corpora are **complete and verified** and ready to consolidate: `../system7-decomp` (43 specs, the System-7 BASE -- ROM-extracted 256-entry CLUT, measured chrome RGBs, byte-verified WDEF geometry; era-stability of the lavender tinge proven 7.0.1<->7.5.3) and `../win31-decomp` (45 specs, confined Win-3.1 ACCENTS only).

This plan consolidates them into a **canonical, era-layered FLAIR spec + goldens** for red-green TDD, then drives FLAIR from library to a **live, draggable, color-chimera desktop** that reproduces the Office Space frame and ultimately hosts serious era apps as **Initech-versions**.

### Ratified decisions (operator, 2026-06-20)
1. **Doom = sequenced milestone (Phase 6b).** Initech-Doom is a **flat-32 source port** recompiled to the InitechOS native target -- **NO DPMI / DOS-extender** (the OS is already 32-bit flat; DOS4GW exists only to run 32-bit code on 16-bit DOS, a problem InitechOS skips). The real gaps are a per-frame framebuffer->window handoff + game-loop timer/input.
2. **Minecraft = native "Initech Mines."** Literal JVM is OUT (Law 3 no-2026-ism + ADR-0001 32-bit-flat). Build a period-plausible blocky-voxel demo in C / Turbo Initech -- the spiritual port, played straight; accretes after Doom-class graphics exist.
3. **M5 native frame apps before the app-hosting arc.** Once FLAIR is live + the app contract lands, build InitechCalc / File Manager / InitechPaint / FILE COPY (reproduce the frame as a live arrangement, the PRD product north star) FIRST; then the hosting arc.
4. **The canonical productivity suite is the top app priority (operator, 2026-06-20).** Among all apps, the three SERIOUS era productivity apps come first -- they are the proof that FLAIR "runs all serious apps of the era": **InitechBase** (= SAMIR, the dBASE-alike, DONE-ISH -- runs full-screen in-OS, M6 substantially complete), **Initech 123** (the Lotus 1-2-3 spreadsheet clone), and **InitechWord** (the WordPerfect clone). Games (Doom, Initech Mines) come AFTER this suite. OPEN ARCHITECTURE FORK for the committee at Phase 4: are Initech 123 / InitechWord **native FLAIR Toolbox apps** (Mac-chrome windows) or **character-mode tenants in the windowed text-console host** (6a) -- real Lotus/WordPerfect were character-mode DOS apps and SAMIR already runs character-mode, which argues the text-console host (6a) is the natural home and should be pulled EARLIER than "last." This reconciles with decision 3 if the frame's spreadsheet window is treated as Initech 123. The committee rules this when ADR-0010 (the app contract) is drafted; it shapes whether 6a precedes or follows the M5 frame apps.

### Era axis (operator, 2026-06-20) -- "never delete, always accrete"
The Office Space frame is most likely **Mac OS 8 Platinum**, not System 7 (the film is 1999; chrome reads as grayscale 3-D Platinum). We build the **System 7.0/7.1 base now** and **accrete a System 8 / Platinum appearance layer later** as additive era-deltas. The canonical spec is therefore **era-tagged from day one** (chrome metrics / color tables / WDEF geometry / title-bar renderer carry an `era` tag) so a future `../system8-decomp` (or a `platinum/` layer) lands without a base rewrite and per-era goldens stay valid. (See `bd memory flair-fidelity-target-era-axis-the-office-space`.)

### Honest framing carried into every phase (the no-2026 / no-real-binary invariants)
- "Host all serious era apps" can **never** mean run a real 16-bit binary -- no v8086 (ADR-0001), and ADR-0003-DEC-08a's honesty boundary forbids claiming genuine .EXE execution. It means **(Initech versions of)**: recompiled-from-source or native reimplementations (SAMIR is the precedent).
- **InitechMZ `.EXE` already ships** (ADR-0003-DEC-08a; `os/milton/mz.c`; `test-mzexec` in `TEST_EMU_GATES`). CLAUDE.md/PRD wording ("MZ deferred") is STALE and is reconciled in Phase 0.
- SSIM is a **guide, never a gate** (ADR-0004 D-8; turning it into a gate is a stop condition). Cross-emulator agreement = each emulator vs its OWN host-model prediction, NOT a cross-emulator byte-CRC (ADR-0004 AM-6).
- Region body stays **clean-room** -- the homomorphism property suite is the ONLY region oracle; **never** mint a region golden.
- The canonical "bugs" are SPEC, enforced not fixed: pie==116, 570- trailing-minus, hourglass-not-wristwatch, the historically-impossible Photoshop menu string, `PC LOAD LETTER`. A "correctness" pass that normalizes these BREAKS canon.

---

## 2. Phase structure (dependency-ordered; [OP] = operator/committee-ruled)

### Phase 0 -- RECONCILE + DECIDE (clears drift; mostly orchestrator-owned)
The forks are ruled (Sec 1). Remaining: clear the stale-doc drift so subsequent work is trustworthy.
- **P0-a** Fix the `#dfdfdf -> #ffffff` Law-3 bug in `spec/chimera_element_map.json` (Win95 COLOR_3DLIGHT value the accent corpus forbids in 4 specs; flat-3.1 highlight is `#ffffff`). Cite `../win31-decomp/specs/chrome/cross-version-guardrails.md`. Rule 8 deliberate spec change + worklog note.
- **P0-b** Reconcile stale docs: CLAUDE.md hallucination-callout + PRD "MZ deferred" -> note ADR-0003-DEC-08a (InitechMZ shipped); PRD "dBASE IV / .mdx" -> "dBASE III PLUS 1.1" per ADR-0008; soften `chrome_metrics.json` provenance wording (cited; values match documented constants; local cache via goldens repo).
- **P0-c** DAG cleanup: supersede legacy `initech-8oi` (M4) / `initech-ox7` (M3) in favor of `initech-k8o5`; re-point M5/M6/M8 deps so `bd ready` shows the true critical path.
- **P0-d** Canonicalize the frame fixture path: copy/move `spec/assets/preview.webp` into `fixtures/` (matching PRD Sec 3 + CLAUDE.md file map), repoint/close `initech-dam`.
- **DoD:** docs internally consistent; `#dfdfdf` gone; `bd ready` honest; a Phase-0 worklog shard.

### Phase 1 -- CANONICAL SPEC CONSOLIDATION (the red-green target) -- epic `initech-dh5k`
Adopt `../system7-decomp` as the System-7 BASE source-of-truth and `../win31-decomp` confined accents, projected into `spec/` via an explicit **manifest** (`spec/CANON-MANIFEST.md`: each corpus spec -> FLAIR spec file + os/flair module, so drift is mechanically detectable). All new headers carry an `era` tag (Sec 1 era axis) and `_Static_assert` oracles in the `window_record.h` style. Subagents author DISJOINT NEW files; the **orchestrator owns all Makefile integration**.
- **P1-1** `spec/control_record.h` (ControlRecord + part-codes inButton/inUpButton/inThumb...) from system7 `toolbox/control-manager.md`; lock the Win flat-button accent as a confined sub-section (win31 `button-control.md`). + host oracle.
- **P1-2** `spec/menu_record.h` (MenuInfo, item attributes, enableFlags, mark/style, MenuSelect result word) from system7 `toolbox/menu-manager.md`. + oracle.
- **P1-3** `spec/dialog_record.h` (DialogRecord + DITL item-type bytes, alert stages, ok/cancel=1/2) from system7 `toolbox/dialog-manager.md`. + oracle.
- **P1-4** Ingest resolved chrome RGBs into `chrome_metrics.json`/`.h`: pinstripe `#F3F3F3`/`#969696` + bevel `#DADAFF`/`#B3B3DA`, close/zoom rendered interior, grow DrawGrowIcon, scrollbar active thumb `#DADAFF`/`#8787B3` + track `#E7E7E7`/`#969696`, and win31 `SM_CYCAPTION=18` / `SM_CXVSCROLL~15` (17px refuted). Move them OUT of `golden_resolves`. Rule 8: bump `schema_version`, worklog note, mutation-prove each constant.
- **P1-5** `spec/assets/clut.json` + `clut.h`: the real 256-entry indexed-8 CLUT from `../system7-decomp/goldens/resources/clut_8_rom.bin` (+ Win DDK static palette for the accent indices), with a CLUT round-trip oracle (RGBColor->index determinism). The OD-2 indexed-8 foundation.
- **P1-6** `spec/drawing_ops.h`: the QuickDraw OPERATION semantics the data records assume -- the 5 verbs / GrafVerb 0..4, CopyBits maskRgn/scale/colorize, coordinate half-open + local/global, pattern origin phase-lock -- from system7 `quickdraw/{drawing-primitives,copybits,coordinate-system,patterns}.md`. + compile oracle.
- **P1-7** `spec/CANON-MANIFEST.md` (the projection map) + adopt win31 `cross-version-guardrails.md` Sec 3 as a FLAIR Win95-ism regression checklist (a locked checklist + a structural gate).
- **P1-8 (defer-able)** Resource/asset binary formats (NFNT/FOND, clut/pltt/wctb, CURS/PAT, WDEF/MDEF/CDEF, WIND/MENU/DLOG/DITL; Win .FON/FNT, DLGTEMPLATE, RT_MENU) -- locked as apps demand them.
- **DoD:** all new headers compile + `_Static_assert` green; each oracle mutation-proven; `make clean && make test` green; manifest committed; canon WARNING headers carried into every new spec file.

### Phase 2 -- GOLDENS PIPELINE + TDD ORACLE VECTOR -- epic `initech-rf2l`
Make fidelity gateable against real pixels.
- Wire the resolver: `SYSTEM7_DECOMP ?= ../system7-decomp` + `WIN31_DECOMP ?= ../win31-decomp` + a `need_flair_goldens` loud-skip mirroring `DBASE3_DECOMP`/`need_goldens` (`Makefile:648`). (Currently ZERO such wiring.)
- First FLAIR pixel-diff oracles: render documentProc window / dBoxProc alert / File menu / active scrollbar; crop; **structural crop-byte equality on the chrome bands as the HARD check; SSIM as GUIDE only**. Mutation-prove each.
- Build the SSIM tool (`initech-w06`, currently a stub at `Makefile:8040`) -- guide, not gate.
- CLUT entry-by-entry palette gate (vs `clut_8_rom.bin`) + a font-metrics oracle (StringWidth vs NFNT owTables; "File"=23px worked example).
- Region stays property-suite-only (no region golden).
- **DoD:** resolver loud-skips cleanly when goldens absent; first pixel-diff gate green + mutation-proven; SSIM tool reports per-window numbers (no gate).

### Phase 3 -- WIRE FLAIR INTO BOOT (the biggest reality gap; prerequisite to ALL hosting) -- epic `initech-re30`
- **P3-pre (serial, opus)** Unify the duplicated clip+CLUT into ONE palette module + implement the GrafPort **verb layer** on the blitter (FrameRect/PaintRect/EraseRect/InvertRect/FillRect, *Rgn, FrameOval/RoundRect, MoveTo/LineTo) as the single drawing path; make `grafProcs` actually dispatch (currently NULL only); delete `cfill/crect/cframe` across chrome/control/dialog. This makes `grafport.h`/`imaging.h` REAL, not decoration. CopyBits + transfer modes (srcCopy/Or/Xor/Bic) + mask region + an offscreen GWorld type.
- Add the Manager objects to `KERNEL_OBJS` (currently only `surface.o`); allocate Manager records/regions from the FLAIR heap at boot (DEC-03 high region); call `shell_build_scene` + `shell_render` to paint the LIVE desktop instead of the console banner.
- `initech-26d`: PS/2 mouse IRQ12 + hourglass **Cursor Manager** (software sprite + save-under). Live cooperative `WaitNextEvent` loop on real IRQ0/1/12 -> `flair_raw_post`.
- **NEW emu gate**: boot -> draw seafoam desktop + windows + FILE COPY -> screendump -> serial milestone, added to `TEST_EMU_GATES` on **QEMU + Bochs** (Rule 5). Land Bochs RFB leg (`initech-u9gf`) + dual-target digest (`initech-k8o5.11`).
- **[OP]** M4 sign-off: land 86Box (`initech-q0gy`) OR record an explicit DEC-04 waiver for M4.
- **DoD:** the booted OS (QEMU + Bochs) shows the live System-7 chimera desktop with the modal FILE COPY; the emu screendump gate is green + mutation-proven; mouse moves the hourglass.

### Phase 4 -- APP CONTRACT (the missing spine for every app) -- ADR-0013 (was mis-numbered "ADR-0010"), epic `initech-4e35`
- **[OP / committee] RATIFIED 2026-06-27 (ADR-0013, ADR-by-committee `wf_42352963-a57`).** How a tenant owns a window content GrafPort/bitmap_t; is launched from the shell; receives routed keyboard/mouse/update events via `WaitNextEvent`; yields cooperatively; exits cleanly -- **plus multi-app co-residency** (the MultiFinder / cooperative-Windows capability) over the existing cooperative pump, within flat-32 + no-isolation. Models the SAMIR-as-tenant precedent + the window content-rect + the event ring. The native-vs-character-host fork for Initech 123 / InitechWord is ruled in ADR-0013.
- **DoD:** ADR-0013 ratified; a reference "hello window" tenant AND a second co-resident tenant run on the live desktop via the contract, with app-switch, gated on an emu screendump + a host behavioural oracle (mutation-proven).

### Phase 4.5 -- PLATFORM SERVICES (the app-platform bar's table-stakes) -- ADR-0012 D-2b, NEW epic
The keystone (Phase 4 / ADR-0013) makes apps *possible*; Phase 4.5 makes them *credible* per the PRD Sec 1.4 app-platform bar (ADR-0012). These are the table-stakes a real System 7 / Win 3.1 competitor could not omit, promoted from Phase-5 one-liners / the deferred P1-8 into first-class, oracle-backed shared Toolbox services that tenants call. **They land BEFORE the canonical app suite** (Phase 5/6) so InitechBase / Initech 123 / InitechWord are built ON the contract + services, not hardwired (ADR-0012 D-2c). Each is built against the `../system7-decomp` (BASE) / `../win31-decomp` (ACCENTS) corpora, era-tagged, with its own independent mechanical oracle (Law 2), never by-construction.
- **P4.5-1 Resource Manager** -- un-defer P1-8: a keyed resource store (Get/Release by type+ID) + the templated UI resources (WIND/MENU/DLOG/DITL, NFNT/FOND, CURS/PAT). Tenant assets load into the tenant child arena (ADR-0013 Sec 3.6). The defining storage/extensibility idiom of the era. + round-trip oracle vs a hand-authored resource fixture.
- **P4.5-2 Scrap (Clipboard)** -- a shell-owned cross-tenant Scrap with text + picture flavors; the inter-app copy/paste that is the payoff of co-residency. + a round-trip + cross-tenant-flavor oracle.
- **P4.5-3 TextEdit + List Manager** -- shared editable-text (`TERec`: selection, word-wrap, cut/copy/paste against the Scrap) + `ListRec`; the text-entry floor for 123 cells / Word body / Standard File. + selection/word-wrap + list-hit oracles.
- **P4.5-4 Standard File / Common Dialogs** -- shell-owned Open/Save (and Color/Font/Print choosers) -- shell modal helpers like FILE COPY, NOT tenants (ADR-0013 Sec 3.6). The only sanctioned way an app names a file; built on the dialog manager + List Manager + the MILTON FAT directory enumerator. + a behavioural (navigate/select/return) oracle.
- **P4.5-5 Print Manager** -- device-independent draw-to-printer (replay the GrafPort verb layer to a printer port) + Page Setup/Print dialogs + spool. Turns `PC LOAD LETTER` from a panic gag into a real, on-brand path (Appendix B canon). The `570-` / 116% canon lives in app content, not the service. + a draw-to-printer-port differential oracle.
- **DoD:** each service compiles + its oracle is mutation-proven + `make clean && make test` green; at least one reference tenant exercises each service on the live desktop (emu-gated); era-tagged; `check-win95isms` clean (ADR-0012 D-4 era ceiling).

### Phase 5 -- TOOLBOX COMPLETION + M5 NATIVE FRAME APPS -- epic `initech-ib6`
- Complete higher Toolbox as apps demand: ResizeWindow/ZoomWindow/GrowWindow; Font Manager + proportional Chicago + txFace styles; un-stub apple glyph + window title text; TextEdit (TERec); List Manager (ListRec); Scrap/clipboard.
- Ship native FLAIR apps into `os/apps/` via ADR-0010: **InitechCalc** (ledger + `$112,276.87` footer + a `570-` cell + the pie summing to **116%** on screen), **File Manager** ("Drive A Files" + folder icons), **InitechPaint** (frozen Photoshop bar), **FILE COPY** (fills at `michael_bolton.conf` rate). Each promotes a data-only canon assertion into a live PIXEL oracle.
- Resolve content-body white (`initech-bmih`) before claiming frame fidelity.
- Motion acceptance: a scripted drag / menu-open / modal-dismiss event-replay (the only motion oracle; SSIM can't judge interaction) -- the PRD "short clip" leg.
- **DoD:** `make ssim FRAME=desktop` reports per-window (guide); the live arrangement reproduces the frame; the canon pixel oracles are green + mutation-proven.

### Phase 6 -- APP-HOSTING CAPABILITY ARC (the north star) -- ADR-0011, epic `initech-t4hp`
Invariants recorded up front: **(Initech versions of)** + no-v8086 (ADR-0001) + no-DPMI/extender (already flat-32).
- **6a (Class-1 text DOS apps -- THE CANONICAL SUITE, see decision 4)** windowed text-console host (`initech-ax9.3`: 80x25 CP437 cell grid in a window, keyboard from the cooperative loop). Promote **InitechBase** (SAMIR) from full-screen to FIRST windowed tenant; then **Initech 123** (Lotus 1-2-3) and **InitechWord** (WordPerfect) -- the operator's must-come-first canonical apps. (If the committee rules these are native FLAIR Toolbox apps instead, they move to Phase 5; either way they precede the games of 6b/6c.)
- **6b (Class-2 Doom)** [OP-ruled in scope]: write down that Initech-Doom is a flat-32 source port -> NO DPMI/extender. Raw-framebuffer window handoff (app writes an indexed-8 content bitmap/frame; FLAIR composites). Game-loop services: per-frame PIT tick + raw make/break keyboard. Keep fixed-point (no kernel x87). Isolate game timing from the reproducible-build surface (Rule 11).
- **6c (Minecraft)** [OP-ruled]: native "Initech Mines" voxel demo (C / Turbo Initech) on the 6b graphics substrate.
- **6d (sound, lowest priority)** SB/AdLib unbuilt; PC-speaker BEL currently dropped.
- **6e (executable-format hardening)** InitechMZ shipped; land MZ overlays (`initech-dtw.4`) only if an app needs DBASE.OVL-style packaging. NO PE/extender work.
- **DoD per sub-step:** the named app class runs windowed-on-the-live-desktop in-emulator, gated.

### Deferred-explicitly (do NOT block first-pass)
system7 P4 beads `mb3`/`cbu` (boot-gated pixel residuals); win31 `as3` (256-color S3 re-mint); the two abandonware acquisitions (`pvo4` System 7.0/7.1 image, `77wz`/`as3` 256-color Win driver) -- proceed with sibling 7.5.3 / 16-color values as provisional-with-honesty-caveats; final pixel-pinning when they land. Corpus-EXPANSION beads for north-star subsystems beyond the current spec scope (system7 Process/Standard-File/Sound/Time Manager, GWorld depth modes; full TextEdit/List impl) -- scope BEFORE claiming app parity.

---

## 3. Orchestration cadence (this build)
Operator-set: delegate each coding step to a subagent (sonnet default; opus for load-bearing); **<=6 sonnet / <=2 opus in parallel**; DISJOINT file ownership per lane, shared files SERIAL. The **orchestrator owns**: Makefile integration, independent re-grading (Law 2: re-run the oracle + mutant + the FULL `make clean && make test` aggregate + Bochs where geometry/boot changes -- NEVER trust the report), commit-per-wave, and the bead ledger. Convene the **committee** (ADR-by-committee subagent role-play) for serious/contested architecture decisions (ADR-0010 app contract; ADR-0011 hosting; any grow-impl-to-meet-spec vs trim-spec call). Raise beads the moment an issue surfaces. The committee has ultimate control; escalate to the operator only on committee gridlock.

## 4. Wave ledger (append per wave)
- **Wave 1 (this session):** Phase 0 reconciliation (P0-a..d) + Phase 1 canonical-spec authoring (P1-1..7), disjoint-file parallel lanes. Orchestrator integrates Makefile + grades + commits.
