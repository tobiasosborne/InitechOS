<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# HANDOFF — Programme Continuity Briefing (InitechOS / STAPLER)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Continuity Briefing (living document; supersede in place)
**Last Reconciled:** 2026-06-28 (**WL-0066: THE LIVE-GUI BUG HUNT -- FEATURE WORK HALTED. The operator ran `make run-flair-tenants`, found the live desktop badly broken (~5 bugs in seconds), and was right: Phase 4.5 (WL-0065) shipped 5 commits on GREEN *host* oracles + a single boot-frame screendump but NOBODY DROVE THE LIVE DESKTOP -- the oracles "grade math, not composited pixels or live behaviour" (initech-pipa). 3 adversarially-verified fan-out rounds (driving `qemu_harness --mouse/--keys` + screendump + comparing to the `../system7-decomp` goldens -- the method researched in `initech-l9cd` that had gone unapplied) found + filed 138 NEW bug beads (label `gui-bughunt`) + 4 known = 142 distinct: 2 P0 (`initech-ojxn` COPY-onto-itself truncates-to-zero data loss; `initech-mswo` region_op stack overflow), ~20 P1 (the compositor does not damage-track non-WindowMgr layers NOR repaint background windows -> drag/app-switch ERASE peer windows; NO active/inactive window chrome at all), rest P2. Full report `docs/FLAIR-GUI-bug-hunt-2026-06-28.md`; `bd stats` 438->579; lesson `bd memories flair-live-gui-testing-gap`. NEXT AGENT: a FIX arc, NOT new features -- start at the 2 P0s, then the compositor-damage + active/inactive-chrome P1 families, then stand up composited-pixel emu gates + the l9cd capture. See the Sec 5 CURRENT-STATE block for the full method + start order. PRIOR -- WL-0062: THE LIVE INTERACTIVE FLAIR DESKTOP + the chrome FIDELITY arc.** On the booted 386 windows now DRAG and menus DROP+SELECT under the mouse, and the window-chrome FIDELITY arc is COMPLETE -- all 6 elements (drop-shadow / close-zoom box / scrollbar / title-bevel + the prior phase/title) graded vs the INDEPENDENT `../system7-decomp` goldens via `test-chrome-fidelity`. Orchestrated session (committee fork + delegated lanes + independent grading), **13 commits `93e8cd4..7dcc4e1`, all pushed**; `make clean && make test` = **ALL GREEN 273 host + 53 emu**; both Bochs legs PASS; reproducible every step; `_kernel_end(flair_live)=0x37174 < 0x40000`. The event loop (`initech-5l5z`, behind `-DBOOT_FLAIR_LIVE`, default boot UNCHANGED): 3 ISR producers (PIT tick / kbd IRQ1 / mouse IRQ12 dual-PIC-EOI minefield) feed the FLAIR raw ring via function-pointer hooks; the WaitNextEvent cooperative pump replaces render-once-HLT and dispatches FindWindow part-codes -> DragWindow + D-5 desktop_paint_damage (DRAG) + MenuSelect (MENUS). Oracles: test-interact (HOST, independent-by-recomputation) + test-flair-drag + test-flair-menu (EMU, independent-canon screendump) + HER-14 drag-noop/menu-noop mutants. **Law 4's "live, draggable arrangement with working menus" is satisfied; the re-flair epic `qipc` step 7 core is delivered.** Remaining are FILED follow-ons (polish/hardening): menu close-restore + CombineRgn (rmsr); live drop-shadow via strucRgn-widening (`9d0e`); mouse-ACK drain; Bochs strict-PIC mouse probe (`04ae`); `test-arena-disjoint` ASLR determinism. See **WL-0062** + the §5 CURRENT-STATE block below. **PRIOR (2026-06-25): FLAIR WINDOW-CHROME FIDELITY ORACLE -- window APPEARANCE is now graded against the `../system7-decomp` captures (it was NOT).** The operator observed the live FLAIR windows "do not look like System 7" and suspected the goldens did not enforce window correctness. An audit (two workflows + first-person verification) CONFIRMED it: NO green gate graded window APPEARANCE -- `test-color-canon` grades color VALUES, `test-chrome` grades metric NUMBERS vs FLAIR's OWN `chrome_metrics.h` (by-construction; "STRUCTURAL compare, not SSIM", ADR-0004 D-8), `ppm_flair_check` grades scene topology calibrated to `test_shell.c`; the real System-7 window captures in `../system7-decomp/goldens/captures/` were NEVER diffed, and SSIM (`harness/ssim.c`) is unbuilt. Built **`test-chrome-fidelity`** (oracle-first, RED->GREEN) grading the REAL `chrome.c` via the host render skeleton against an INDEPENDENT pixel-measured golden **`spec/chrome_fidelity_golden.h`** (from the decomp `specs/chrome/*.md` + captures, NOT from `chrome_metrics.h`). **Element 1 (WL-0060, commit `cb76816`):** the pinstripe **PHASE** -- the free-running `L,D,L,D` fill was WRONG yet PASSED every "period-2 alternation" check; `chrome.c` is now phase-locked (`LLDLD..DLL` doubled-LIGHT pairs, patAlign mod-8). 5 strict-period-2 sites that encoded the bug were operator-ratified-amended to phase-agnostic "striped" checks (`test_chrome.c`, `test_shell.c`, `test_drag.c`, `ppm_flair_check.c` leg c + HER-02 demo). **Element 2 (WL-0061, commit `dc5ddca`, bead `initech-lxg9` CLOSED):** **TITLE TEXT + knockout** -- `chrome.c:279` drew BLANK title bars; now the window title is drawn CENTERED in Chicago over a knocked-out light gap (the biggest visual win), via the C-8 policy seam (colorblind-clean); root-cause fix `NewWindow` now defaults `titleHandle` to "" (was uninitialized). Live desktop now shows W0 "untitled-1" + W1 "Saving tables to disk", centered. **`make clean && make test-unit` = ALL GREEN 271 host; Rule 5 QEMU `test-flair-desktop` PASS + Bochs PASS; reproducible-build green; `_kernel_end=0x35b20 < 0x40000`.** **NEXT:** the remaining window-chrome fidelity elements (open dependents of umbrella `initech-hmll`): `ts3t` close/zoom box geometry (11x11, +9/-20 offsets, zoom glyph), `92li` bevel rows + 15-row interior recomposition (shifts `content_top` -- geometry blast radius), `jh7m` scrollbar arrows/thumb + black-outer/gray-inner separators, `54nw` drop shadow (1px L at (1,1); NOTE `ppm_flair_check` asserts the pixel outside the frame is bare teal -- amend). Each grades the REAL `chrome.c` against a decomp-pixel-measured golden, same pattern. **LESSONS:** the title sits at bar-center where 5 oracles scanned the pinstripe -- a grep (Rule 3/4) caught them all incl. the HER-02 demo (which the emu run caught); a FLAKY `test-kernel-repro` RED in the INCREMENTAL aggregate was GREEN on a CLEAN build (authoritative = `make clean`); the C-8 seam is load-bearing for text (a raw color literal would trip the colorblind oracle). **PRIOR (2026-06-22): FLAIR ARCHITECTURE RE-RATIFICATION COMPLETE (operator-ratified; epic `initech-qipc`).** A grading audit proved the FLAIR oracle graded **BY CONSTRUCTION** (`ppm_flair_check` computed expected RGBs from `flair_palette_rgb` -- the SAME function `kmain` paints with -- so the "very wrong" `preview.webp` palette passed EVERY gate), the decomp corpora were NOT the live golden master (only `test-clut` diffed one CLUT, and `clut.h` drove no rendering), and the booted OS ran **no FLAIR event loop**. An orchestrated overhaul followed: a **heresy audit** (20 candidates, **19 upheld / 1 struck**, adversarially adjudicated) -> **6 architecture committees** + a consistency critic (caught it did NOT compose: the color module was quadruple-owned) -> a **chief-architect reconciliation** (verified `composes: true`, ZERO by-construction oracles) -> **5 RATIFIED governance docs**. LANDED: `docs/adr/REVOCATION-RECORD-2026-06-21-FLAIR-Heresy-Purge.md` (the decree -- HARD-REVOKE HER-02 by-construction grading; FORMAL-REVOKE OD-4 seafoam->teal, the `preview.webp` render source, the seafoam `canon` hard-gate; AMEND the unwired `win31-decomp`, the unbuilt-but-documented SSIM, the mono-heritage ATKINSON, the dead event loop); **`ADR-0004-AMENDMENT-DEC-09`** (P1 mechanism/policy split -- binding C-8: the mechanism names palette INDICES, ZERO color literal; the 5 drifted `index->RGB` switches collapse onto ONE `flair_canon_rgb`; era-layering D-9; OD-4 seafoam->teal `#8DDCDC`); **`ADR-0005-AMENDMENT-AM-1`** (ATKINSON = the SINGLE dual-heritage spine -- a GDI/HRGN `CombineRgn` peer family rides the SAME `region_op` as QuickDraw's; the `RectInRgn` containment->overlap deep-bug fix; homomorphism oracle untouched); **`ADR-0010-FLAIR-Grading-and-Goldens`** (grade against an INDEPENDENT decomp golden, NEVER by construction -- one `test-color-canon` 4-leg oracle, `ppm_flair_check` re-key, `WIN31_DECOMP` wired, SSIM honestly deferred); **`ADR-0006-FLAIR-Live-Event-Loop-and-Behavioural-Grading`** (the booted-OS `WaitNextEvent` loop behind `-DBOOT_FLAIR_LIVE`, ISR-fed ring, behavioural grading by KIND). **THE INITECH COLOR CANON IS NOW LOCKED DATA: `spec/assets/color_canon.json`** (idx2 desktop teal `#8DDCDC`; white `#FFFFFF`; pinstripe `#F3F3F3`/`#969696`; navy `#000080`; BTNFACE `#C0C0C0`; lavender bevel->teal `#8DDCDC`/`#4E9BA3`; each entry graded vs its decomp golden + a wctb part<->index crosswalk; `preview.webp` demoted to provenance-only). Doc-drift purged: CLAUDE.md (SSIM unbuilt + a by-construction-is-not-an-oracle callout under Law 2), PRD (seafoam->teal headline + SSIM-planned notes), this HANDOFF, ADR-0004 OD-4 (in-place supersede pointer). **7-STEP ROADMAP (`bd show initech-qipc`):** `initech-n79q` region spine (color-independent, FIRST) -> `h714` canon generator -> `mwpw` `test-color-canon` value oracle GREEN (ORACLE-FIRST) -> `7x9k` collapse 5 switches + flip to teal (Rule-8) + `ppm_flair_check` re-key -> `6bq2` mech/policy oracles (C-8) -> `m6qx` era registry (`flair_skins.h`) -> `5l5z` live event loop. **re30.4 is reframed as steps 2+4** (a policy/mechanism cut from the decomp canon, NOT a dedup). **NEXT: implement `initech-n79q` (the region dual-heritage spine, lands first per the critic's sequence), then the oracle-first canon chain.** See WL-0054. **PRIOR (2026-06-21, WL-0053):** **THE INITECH COLOR CANON RATIFIED (operator); FLAIR palette source corrected to the decomp goldens. A DECISION shard -- re30.4 implements it NEXT session.** After re30.3 shipped the live desktop, the operator inspected it and ruled the PALETTE WAS "VERY WRONG": the FLAIR chrome colors in `spec/assets/palette.h` are MEASURED SAMPLES from the low-res `preview.webp` Office Space MOCK-UP (a dim CRT photo), so window "white" was a muddy `#7F7F86`, pinstripe dark, menu bar muddy -- windows rendered gray, not crisp Mac white. **The canon is the painstakingly-built sister-repo goldens/specs (`../system7-decomp`, `../win31-decomp`); FLAIR grades against THEM, not preview.webp.** (A committee had just ruled to unify onto the wrong `palette.h` source; the operator -- the human Law-4 judge -- OVERRODE it.) **THE INITECH COLOR (DEFINITIVE):** the signature "Initech teal" = VIC-20 cyan, hardware colour 3 -- **SCREEN/phosphor `#8DDCDC`** (operator motion read; alt VIC decodes `#85D4DC` Lospec / `#87D6DD` azulianblue), **PIGMENT `#57B19F` for PRINTED PROPS ONLY** (box/manuals/labels; the CRT shader carries pigment toward the screen cyan). **PALETTE RULE: System 7 (later System 8/Platinum) colours are GOLDEN, EXCEPT (1) replace ALL System-7 LAVENDER tinge (`#DADAFF`/`#CCCCFF`/`#B3B3DA`/`#8787B3`/`#333366` -- the 3D bevel/groove accents) with Initech teal `#8DDCDC` (teal-light `#8DDCDC` + darkened-teal shadow ~`#4E9BA3` to keep the 3D), AND (2) DESKTOP BACKGROUND = `#8DDCDC`** (this SUPERSEDES ADR-0004 OD-4 seafoam `#6FA08E`); **use Win 3.1 colours for the additional Windows/Photoshop chrome bits** (BTNFACE `#C0C0C0`, navy `#000080`, ...). The teal endows the OS with its character. **OPERATOR-APPROVED per-index map** (WL-0053 table): idx0 `#000000`, idx1 window-white `#7F7F86`->`#FFFFFF`, idx2 desktop `#6FA08E`->`#8DDCDC`, idx3 menubar `#67696C`->`#FFFFFF`, idx4 ink `#525A63`->`#000000`, idx5 accent `#1E2F87`->`#000080` (Win navy; drives the 116% pie), idx6 control `#BFBFBF`->`#C0C0C0`, idx7 pinstripe-light `#6B6B74`->`#F3F3F3`, idx8 pinstripe-dark `#8A8A93`->`#969696`. **Recorded: `bd memories initech-color` (`initech-color-canon-...-definitive`) + bead initech-hfeg (P1).** **NEXT SESSION = re30.4 RE-ORIENTED:** re-value the palette to the canon + swap chrome lavender->teal + re-point the single palette module to the decomp canon (NOT preview.webp) + re-calibrate the oracles (`ppm_flair_check`, test-chrome/control/dialog, and `test-palette-seafoam` -> assert teal `#8DDCDC`) + amend ADR-0004 OD-4 -> then a fresh 386 screendump for the operator's Law-4 verdict. The committee re30.4-palette-source ruling is SUPERSEDED. **LESSON (bd memory):** for any FLAIR color question the authority is the decomp goldens (rendered-golden pixels = Law 2 truth), NEVER the preview.webp mock-up samples. Prior: WL-0052 -- **re30.3 (M3.1) COMPLETE: the FLAIR chimera desktop renders LIVE on the 386 -- it IS the Office Space frame.** Commit **1bd47c2**: kmain `-DBOOT_FLAIR_SHELL` gives the FLAIR library (linked-but-callless since re30.2) its first call site -- after FLAIR-HEAP-OK it `flair_heap_init`s the 4 MiB window, allocates the ENTIRE scene + regions + a 640x480 indexed-8 offscreen FROM THE HEAP (not BSS -- runway 5dr8), builds the test_shell.c scene (2 windows + System-7 bar + Photoshop chimera bar + modal FILE COPY), `shell_render`s into the offscreen, and PRESENTS to the live LFB (8bpp DAC+copy / 24-32bpp `flair_palette_rgb` per pixel via a new shared palette.h inline), emits `FLAIR-DESKTOP`, halts. Orchestrator VISUALLY CONFIRMED the QEMU screendump is the frame (Law 4: seafoam + two stacked menu bars + two pinstripe windows + the "Saving tables to disk..." modal occluding them). Mutation-proven oracle `tools/ppm_flair_check.c` + `test-flair-desktop` (+ `-mutant`: 3 SHELL_MUTATE_* legs RED) wired into TEST_EMU_GATES; `test-flair-desktop-bochs` serial leg PASS. **254 host + 43 emu GREEN; Bochs PASS.** `_kernel_end=0x34bc0` (45 KiB under the 0x40000 ASSERT). **COMMITTEE (re30.3-chair, no gridlock)** ruled the display-mode strategy of record DUAL-DEPTH (request priority 32->24->8/VESA-0x101->fail-loud floor; indexed-8 OD-2 co-canonical + period-authentic; AM-6 host-model-per-mode, NOT a QEMU pin); the 8bpp DAC present path is correct-but-UNREACHED (stage2 requests only 32/24bpp -> Bochs's 320x200 mode-0x13 is rejected by the 640x480 guard) and is KEPT with a "do NOT delete" comment; M3.1 closes GREEN on the QEMU truecolor leg + Bochs boot-accuracy serial. **Follow-ups filed: initech-2gva (P1 -- stage2 0x101 fallback + an oracle leg that REACHES the 8bpp wrapper, before M4/86Box), initech-6rim (P2 -- record the strategy as an ADR-0004 amendment), initech-fgs1 (P3 -- operator note: 486+/period-SVGA minimum under consideration; PRD Forward Note + commit 38ee42c).** **re30 epic now 3/9** (B0+M3.0+M3.1 -- the critical-path arc DONE). **NEXT (re30 remaining, P2, off critical path):** the verb-track re30.4 (palette module) -> re30.5 (GrafPort verbs) -> re30.6 (flip Managers) -> re30.7 (CopyBits/GWorld); the input arc re30.8 (tick) -> re30.9 (keyboard); the live desktop coexisting with the REPL needs the event loop (re30.8+) + App Contract (epic 4e35). The MILTON redirection arc (bsy.7/.8/.9) + initech-2gva (P1) remain in parallel. Prior: WL-0051 -- **re30.2 COMPLETE: FLAIR now boots INSIDE the kernel image (linked, not yet drawn).** Two bisectable commits, green + pushed: **7c4daff (Step A)** raised PROGRAM_BASE 0x38000->0x40000 -- the pre-authorized +0x8000 whole-map shift mirroring o0td EXACTLY (kernel window 160->192 KiB; SAMIR arena disjointness preserved; 36 files; 4 adversarial verifiers + a kernel.ld-ASSERT mutation all PASS). This SPENT THE LAST conventional free gap -- the kernel stack [0x90000,0xA0000) now butts the 0xA0000 VGA aperture with ZERO slack, so 0x40000 is the MAXIMUM raise under the conventional scheme; **follow-up initech-5dr8 filed** (next growth = high-half/ext-mem relocation). **95cc6df (Step B)** linked all 12 FLAIR Managers (region/heap/event/window/blitter/chrome/text/menu/control/dialog/desktop/flair_shell; surface already linked) into KERNEL_OBJS+KERNEL_SHELL_OBJS only (NO call sites) -- the linker question ANSWERED: 61 symbols, ZERO unresolved/duplicate, no stubs; **B0 ASSERT HELD** (_kernel_end=0x3bad0 < 0x40000, ~17.4 KiB margin); geometry KERNEL_SECTORS 224->320 (+ stage2 equate) + IMG_SECTORS 256->384. **254 host + 41 emu + Bochs GREEN** throughout (the IMG geometry change = a Rule-5 tri-emulator obligation, re-verified on Bochs at the new 320-sector multi-cylinder boot). Orchestrated: each step delegated to an opus subagent; orchestrator owned grading (full clean aggregate + Bochs + adversarial workflow) + per-step commit; committee NOT re-convened (decision already ratified WL-0050). **NEXT = initech-re30.3 (M3.1) -- THE milestone:** static live desktop via shell_render to the LFB in kmain (existing cfill path, NOT the verb refactor) + a new test-flair-desktop screendump gate (QEMU ppm_flair_check + Bochs serial). The MILTON redirection arc (bsy.7/.8/.9) remains open in parallel. Prior: WL-0049+WL-0050 -- **FLAIR CONSOLIDATION + EXPANSION begun.** Operator opened the FLAIR build-out (north star: a period-authentic GUI that hosts all serious era apps as **Initech-versions**). A 7-lane understand workflow + critic mapped the state: **FLAIR is host-green but does NOT boot** -- only `surface.o` links into the kernel; the booted OS still shows seafoam + a console banner; the locked imaging spec is partly aspirational (no draw verbs). Authored **`docs/plans/FLAIR-implementation-plan.md`** (7 phases, dependency-ordered, era-layered **Sys7-now / Sys8-Platinum-accreted-later**; ratified forks: Doom = flat-32 source port [NO DPMI -- already 32-bit flat], Minecraft = native "Initech Mines", M5 frame apps before the hosting arc, canonical suite **InitechBase -> Initech 123 (Lotus) -> InitechWord (WordPerfect)**). **Phase 0/1 LANDED (commit fe4d9b1; 244 -> 254 host gates):** the era-layered canonical spec -- `spec/{control,menu,dialog}_record.h` + `drawing_ops.h` + the 256-entry `assets/clut.{json,h}` + ingested chrome RGBs (out of golden_resolves) + the **`#dfdfdf`->`#ffffff` Law-3 fix** + `CANON-MANIFEST.md` (88 corpus specs mapped) + `win95ism_guardrails.md`; docs reconciled (MZ ships per DEC-08a; M6 = dBASE III+ per ADR-0008); 5 new gates + mutants. **Phase 3 committee** (2 opus seats, chair-synthesized, no gridlock) ruled **GROW** (the GrafPort verb layer + one palette module from clut.h) on a **bisectable boot sequence** (M3.1 ships on the existing cfill path; the verb refactor lands parallel). **B0 (commit 50c1374):** a mutation-proven **kernel.ld fail-loud `ASSERT(_kernel_end < 0x38000)`** + corrected the stale `memory_map.h` headroom comment -- and FOUND the **BOOTING shell kernel has only ~29 KiB headroom (`_kernel_end=0x30920`) < FLAIR's ~51 KiB**, so M3.0 needs the **pre-authorized +0x8000 PROGRAM_BASE raise**. Both commits pushed to origin/command-com-default. **NEXT = `re30.2` M3.0** (the +0x8000 raise, o0td-style, preserve SAMIR's arena, FULL emu+Bochs grade) -> **`re30.3` M3.1** (static live desktop via shell_render + a new `test-flair-desktop` screendump gate). The MILTON redirection arc (bsy.7/.8/.9) remains open in parallel. **Prior: WL-0046..0048 -- THE KERNEL-GROWTH / REDIRECTION ARC: o0td** (PROGRAM_BASE 0x30000->0x38000, whole-map shift +0x8000 = **+32 KiB kernel window**; SAMIR's heap preserved byte-identical) -> **k36g** (INT 21h AH=09h/02h/06h CON output now routes through the **redirectable JFT handle 1**) -> **hsct OUTPUT redirect** (`echo HELLO > file` + `>>` append REALLY redirect on the emulated 386). 3 commits pushed (bae46d4 / 7de96e6 / 24194ed); **240->244 host + 39->41 emu + Bochs GREEN**. The committee disproved the prescribed o0td fix (SAMIR .bss measured 47.3 KiB not the documented 26 KiB) and the orchestrator amended the chair's ruling (kstart ESP). Next: **bsy.9** (loader child-PSP-inherits-parent-JFT -> external EXEC redirect), **bsy.7** (`<` stdin), **bsy.8** (`|` pipe). Prior: WL-0040..0045 -- DOS-3.3 PARITY: 5-WAVE ORCHESTRATED PUSH TO THE CAPSTONE. **224 -> 240 host gates; full emu (39) + Bochs GREEN throughout; 5 commits pushed (9a5fea3..4e60bd5).** Wave 1 xw1(.BAT/AUTOEXEC) + bo40(AH=31h KEEP) + x3mh(ANSI FSM); Wave 2 mvg(INT 24h critical-error) + 509.7(device chain); Wave 3 6zd9(device chain->INT 21h OPEN-by-name); Wave 4 p96i(ANSI CON wiring) + the FAT-cache share [a committee ratified raising PROGRAM_BASE; orchestrator grading (Law 4) caught that the raise BROKE SAMIR's heap arena -> reverted -> the committee's own option B instead]; **Wave 5 CAPSTONE 40oq -- the Appendix-A INT 21h functional-coverage CERTIFICATE is GREEN** (55 dispatched, 0 unwaived gaps; AH=03/04/05 AUX/PRN gap closed; FCB-waived; partitions+multivol deferred). Beads closed: xw1/bo40/mvg/509.7/6zd9/p96i/x3mh; 40oq functional-certified (stays open on kzfs/slvd). Wave 6 hsct(I/O redirection) ATTEMPTED + REVERTED -- found 2 blockers (AH=09h not redirectable; kernel window exhausted). **THE KERNEL WINDOW [0x10000,0x30000) IS FULL -- o0td (P1) is now the gating prereq for further kernel growth.** Prior: WL-0038 (Tranches E/F/G + MZ .EXE end-to-end); FLAIR PAUSED.)

> Incoming agent: read this top to bottom, then `CLAUDE.md`, then run `bd ready`. This briefing tells you *where the Programme stands and what to do next*; `CLAUDE.md` tells you *how to work*; the PRD and the ADRs tell you *what to build*.

---

## 1. Read order (do this first)

1. `CLAUDE.md` — the Laws & Rules (oracle-is-truth; fail loud; Red→Green; ASCII source; beads-only tracking).
2. `InitechOS-PRD.md` — the product spec (now reconciled to the ADRs).
3. `docs/adr/` — **ratified, authoritative decisions.** `ADR-0003` (InitechDOS, the active milestone) is the one to know cold. `CDR-0001` records the interim-toolchain deviation. ADRs **govern**; where the PRD/CLAUDE.md ever diverge, the ADR wins and the divergence is reconciled.
4. This briefing (current state + next steps).
5. `bd ready` / `bd show <id>` — the live work queue. Run `bd prime` for the tracker workflow.

## 2. What the Programme is

A bootable, period-plausible OS for emulated 386+ PCs — a DOS-3.3 personality (`MILTON`) under a System-7-style Toolbox (`FLAIR`), reproducing the *Office Space* "Saving tables to disk…" frame, with a dBASE-alike that really runs and a Pascal self-hosting compiler (`Turbo Initech`) as the finale. Built by an agent swarm whose fitness signal is the emulator itself.

**Design stance (governs every naming/structure decision):** the blandness is deliberate and rigorous. Keep every canonical name and every vestigial structure, in full, with a straight face. InitechDOS is not a parody of DOS — it is DOS with the soul extracted and the legacy lovingly preserved. Corporate software accretes and never deletes.

**The recursive joke (operator, 2026-06-14; see `bd memories`):** at first glance
InitechOS must be indistinguishable from a real early-90s corporate OS — the
presentation layer (README, manuals, box, the UI) NEVER admits the joke. The
reveal is layered: it looks like a vibe-coded AI toy, then it REALLY boots on
386-era hardware, then the software REALLY works — with deadpan absurdities played
straight (the 116% pie chart, the Y2K accounting system, Michael Bolton's
rounding-error virus, the TPS Report Generator that needs the vestigial FCB API).
Only the FINAL build (physical 5.25" floppies + period manuals/box,
`packaging-epic`) must be completely straight; the DEV JOURNEY is intentionally
transparent (the public repo + AI history are part of the reveal) and should
increasingly read as a found-footage CLEAN-ROOM reconstruction. ADR-by-committee
(subagent role-play) is for BIG features only.

## 3. Binding decisions in force

| Decision | Ruling | Source |
|---|---|---|
| **First deliverable** | **InitechDOS** (M2, codename MILTON). Toolbox/GUI (M3/M4) deferred behind it. | operator + ADR-0003 |
| **OS implementation language** | **C** (kernel, InitechDOS, Toolbox, bundled apps for now). | ADR-0002 / PNC-1 |
| **Pascal** | Reserved for **Turbo Initech** (self-host compiler, ADR-0007 *pending*) and programs it compiles. The seed compiler (`seed/`) is its genesis, NOT the OS bootstrap. | PNC-1 |
| **Self-host fixpoint** | Concerns Turbo Initech (`K₂==K₃`), not the C kernel (which the factory rebuilds). | PRD §7 |
| **Toolchain** | Target `i686-elf` (ADR-0002). **Interim: host `gcc -m32 -ffreestanding -nostdlib` + `nasm` + `ld`** until dev moves to a more capable device. | CDR-0001 |
| **Executable format** | Flat binary kernel; flat `.COM`-equivalent apps; **MZ `.EXE` deferred**. | ADR-0003 DEC-08 |
| **Documents** | All new docs in enterprise corporate-committee ("Initech") house style (NFR-7). | operator |
| **Tracking** | `bd` (beads) only; `bd remember` for persistent knowledge. No TodoWrite/markdown TODOs. No GitHub CI. | CLAUDE.md |

## 4. What is built and green (do not redo)

The Programme is well past foundations: **InitechDOS boots from disk, prints its
banner, mounts a real FAT12 filesystem, and drops to an interactive COMMAND.COM
`A:\>` prompt** — on QEMU *and* Bochs. Everything below is verified by a
mechanical gate (re-run any time; see §4.1).

**Foundations / boot (M0–M1):**
- `tse`/`uba`/`znb` — repo + Makefile, toolchain, seed Pascal→x86 compiler.
- `f2s` — QEMU oracle harness (`harness/emu/`): serial, triple-fault detect (via
  `-d` log, not reset-count), live-guest QMP screendump, timeout. Now also `--disk2`.
- `f8v.1`/`f8v.2` — `make smoke` + the real boot chain (`os/boot/`): MBR → A20/GDT/
  protected-flat → VESA LFB.
- `dt9`/`a9w`/`slz` — closed (the tracer already did MBR-load / protected / VBE-LFB).
- `d00` — **stage2 → C kernel handoff**: captures the VGA ROM 8×16 font + a
  `boot_info` block, loads the flat C kernel from disk, far-jumps into it.
- `yqb` — **8×16 LFB text console** (`os/milton/console.c`): glyph blit (bpp 24/32),
  putc/puts/newline/scroll.
- `bea` — **the InitechDOS banner** prints (via `int 0x21` AH=09h), byte-exact vs
  `spec/dos_banner.txt`. (Open only for the *tri-emulator* clause → `x0i`.)

**DOS internals (M2 / `509.x`):**
- `8e7` — `bpb_t` locked into `spec/dos_structs.h`.
- `adf` — **FAT12 read** (`os/milton/fat12.c`): mount/BPB, 12-bit decode + chain
  walk (anti-hang), root-dir enumerate, file read. **ATA PIO backend (`ata.c`) now
  validated on the emulator.** `5cu` — FAT differential oracle vs mtools/python.
- `a5a` — **interrupt foundation** (`idt.c`/`isr.asm`/`pic.c`/`panic.c`): IDT,
  exception handlers → fail-loud panic (not triple-fault), 8259 remap to **0x28/0x30**
  + masked.
- `509.5` (partial) / `1f9` — **INT 21h dispatcher** (`int21.c`): literal `int 0x21`
  trap gate, AH dispatch, controlled scope; CON functions 02h/09h/40h/30h/4Ch.
  Calling convention **ratified as ADR-0003 amendment DEC-04a** (by a delegated ARB
  committee; `spec/int21h_calling_convention.json`).
- `509.4` — **PSP** 256-byte construction (`psp.c`). Program **loader** (`loader.c`):
  lays out PSP + image, runs a flat `.COM`, returns to the loader on 4Ch/INT 20h.
- `saw` — **FAT12 mount over ATA + proto-DIR** + FAT-sourced `.COM` load/EXEC.
  See WL-0007.

**M2-finale, file handles, shell, devices (WL-0008–WL-0013):**
- `509.3`/`509.5` — **SFT/JFT + file-handle INT 21h** (OPEN/READ/WRITE/CLOSE/
  LSEEK/FINDFIRST/NEXT, DUP/DUP2); `fileio_fat.c` binds the FAT backend; FAT12
  **write** path + multi-open. `509.8` — INT 22/23/24 + SETVECT/GETVECT.
- `3rs`/`n62` — **PS/2 keyboard (IRQ1) + CON input**; `yv9` — MC146818 RTC;
  `509.2` — **SYSINIT + CONFIG.SYS** (FILES= cap). `xk2` — INT 21h reentrancy
  under an IRQ storm. `509.1` — diagnostic-message catalogue.
- `7pc` — **COMMAND.COM REPL** (`command.c`): `$P$G` prompt, DIR/TYPE/CD/CLS/
  VER/ECHO/EXIT + external `.COM` EXEC, all via real `int 0x21`.

**Kernel hardening + tri-emulator boot (WL-0014–WL-0016, this push):**
- `bcg` — **kernel hardening**: wave-A/B robustness fixes, edge/error-path
  suite, FAT-corruption + CONFIG.SYS + cmdline fuzzers, reproducible-build gate,
  INT 21h user-pointer guards (ADR-0003 DEC-14). See WL-0014.
- `6pj` — **standard-VGA mode-0x13 fallback** (`stage2.asm` `.vga_fallback` +
  `console.c` 8bpp renderer + `kmain.c` DAC palette): Bochs has no VBE LFB, so
  stage2 falls back to mode 0x13 and the OS boots there. See WL-0015.
- `564` — **C Bochs oracle** (`harness/emu/bochs.{c,h}`) + `test-boot-bochs`:
  the dual-emulator boot differential (RFB unblock in C, serial assertion, no
  triple-fault), mutation-proven. See WL-0015.
- `k6x` — **COMMAND.COM is the DEFAULT boot**: the real boot drops to `A:\>`
  after the banner; baked PROGRAM/TYPE/DIR demos moved to `DEMO_IMG`. See
  WL-0016.

**Kernel hardening sweep (WL-0017, `initech-bcg.1..11`):** a grounded read-only
audit of all nine kernel subsystems (0 P0/P1 escaped; 20 P2 + 20 P3 confirmed,
26 rejected) drove **11 fixes**, each RED->GREEN, mutation-proven, committed:
all four P1 correctness bugs (RDWR-write denied `bcg.1`; AH=59h stale error
`bcg.2`; FAT12 out-of-range cluster `bcg.3`; FAT16 mis-decode `bcg.4`) plus the
fail-loud/wedge P2s (do_puts guard `bcg.5`; spurious-vector resume `bcg.6`;
8259A spurious-IRQ EOI `bcg.7`; bounded fail-loud serial `bcg.8`; CONFIG.SYS
honor-first-1KB `bcg.9`; loader -O2-safe entry jump `bcg.10`; console geometry
`bcg.11`). Two new emu gates (`test-spurious`, `test-sysinit-oversize`); new
error codes `FAT12_ERR_UNSUPPORTED`, `CONSOLE_ERR_GEOMETRY`. **Remaining bcg
children (all P2-infra / P3, NOT correctness bugs): `bcg.12` (ata error-path
oracles + BSY/DRDY-before-command), `bcg.13` (shell msg-catalogue scanner),
`bcg.14` (P3 robustness/test-gap sweep), `bcg.15` (8bpp DAC screendump oracle).**
A fresh session is the right home for these (esp. `bcg.12`'s delicate ATA
command-sequence change).

### 4.1 Gates that must stay green
`make test` = **184 host + 35 emu gates** (WL-0033: SAMIR now RUNS INSIDE InitechOS --
+8 host [`test-arena-disjoint`, `test-loader-big`, `test-hardware-spec`, `test-samir-softfp`
x unit+mutant] and +6 emu: `test-samir-boot`(+mutant) boot->EXEC SAMIR.COM->USE->LIST;
`test-samir-write`(+mutant) REPLACE/APPEND persists to the .dbf on the FAT volume (independent-
reader-verified); **`test-samir-canon-y2k`(+mutant)** the Initech AR aging app with the ENFORCED
Y2K bug RUNS in-OS via a new `DO <file>` REPL feature (ASOF parses '00' as 1900 -> 1999 invoices
mis-age ~ -100 years, TOTAL OVERDUE wrongly $0.00 -- the deadpan canon, played straight on the
emulated 386). The S8.2 milestone + capstone is GREEN on QEMU: the dBASE-III+-1.1 engine boots
as a flat .COM, opens/lists/EDITS a .dbf, and runs real .prg programs, all via soft-float + the
disjoint AH=48h arena + the in-place FAT loader. See ADR-0009 + WL-0033.). Prior:
`make test` = **176 host + 27 emu gates** (WL-0031+WL-0032 took SAMIR/M6 from 124 to 176 host:
the full `.dbt` codec, `.ndx` keys/SEEK/build/maintain, the whole interpreter S5.1-S5.8 + the
dot-prompt REPL, writable USE, all five function families, and the Phase-6/7 oracles). **The M6
`make test-dbase` milestone is now GREEN** (was a stub_fail) -- it aggregates `test-dbase-roundtrip`
(S6.3 bidirectional round-trip) + `test-dbase-diff` (S6.4 program differential, 100%); InitechBase
passes its mechanical oracle (Law 2). SAMIR is still host-only, so the 27 emu gates are unchanged
(Milton integration S8.x is GATED). See WL-0032 for the wave ledger. Historical:
`make test` = **124 host + 27 emu gates** (WL-0030 added +20 host: the SAMIR `.dbf` codec +
expression-engine + `.ndx`-parse oracles -- `test-dbf-{header,fields,read,roundtrip,mutate}`,
`test-xbase-{lex,parse,eval,coercion}`, `test-ndx-parse`, each x unit+mutant; re-verified
`make clean && make test-unit` = ALL GREEN 124 host; SAMIR is still host-only so the 27 emu gates
are unchanged and `test-dbase` stays a milestone stub_fail until the M6 differential at S6.3/S6.4).
Prior: WL-0029 added +1 host: `test-samir`, the
SAMIR/M6 foundation umbrella -- pal/rt/pal-host/spec/value + the dbf_ref/ndx_ref Tier-1
gates; re-verified `make clean && make test-unit` = ALL GREEN 104 host. SAMIR is host-only,
not yet in the boot image, so the 27 emu gates are unchanged. `test-dbase` stays a milestone
stub_fail -- the M6 differential lands at S6.3/S6.4). Prior: WL-0028 added +10 host: the forward
tranche `80k`/`d27i`/`x8fs`/`er3h`/`4tw` x2 -- DOS 8.3 wildcard oracle,
windowed FAT16 mount, AH=3Fh cooked CON read, AH=33h Get/Set BREAK + ^C/INT 23h
check-point. Adversarial review caught + fixed a DEC-16 deviation (er3h SET
wrote AL); a host-oracle HANG (x8fs cooked read on the no-source sentinel) was
caught only by the FULL aggregate gate and root-fixed -- ALWAYS run
`make clean && timeout 1200 make test` at integration. WL-0027 added the FAT16 milestone
(dao streaming walk + z01 FAT16 decode): +3 host = test-fat-readfile-mutant +
test-fat16 + test-fat16-mutant, re-verified from clean;
WL-0026 discharged the WL-0025 follow-up debt -- +5 host (`test-nmpo`,
`test-fat-fault-rollback`(+mutant), `test-4nbn-mutant`, `test-nmpo-mutant`) +1 emu
(`test-absdisk-emu`), plus the test-gnrc non-root rename leg, the b53d MUTANT 8
dispatch-edge CX-reject, and the test-spec error_codes completeness assertion;
WL-0025 added the
INT 21h parity tranche gates (`test-kji0/qekc/b53d/gnrc-mutant`,
`test-m0bp-rollback`(+mutant), `test-absdisk`(+mutant), `test-spec[6/6]`) for
CREATNEW/FILETIME/CHMOD/RENAME/IOCTL 5Bh/57h/43h/56h/44h + INT 25h/26h; WL-0024 added the
EMU `test-ut6d`(+mutant, 2 legs), `test-zs24-exec`(+mutant) and the host/diff
`test-zs24`(+mutant, 5 legs) for shell MD/RD/CD + subdir file WRITE + subdir
EXEC, and amended the DOS catalogue 16->19; WL-0023 added `test-u6wa-mutant` +
`test-fat12-mkdir`(+mutant) for CHDIR/MKDIR/RMDIR; WL-0022 added
`test-mzxa-integration` + `test-mzxa-mutant` for ti8 L2; WL-0021 added
`test-fat-subdir`(+mutant) + `test-region`(+mutant); was 59+22 after WL-0019).
**Env gotcha (`bd memories`):** if you see "Clock skew detected", `make clean`
before trusting an incremental oracle -- future-dated build/ artifacts make
`make` skip rebuilds (false greens). Authoritative re-verify = `make clean &&
make test`.
**`make test-boot-bochs` PASS** with `KERNEL_SECTORS=160` (4tw grew it 144->160 in
WL-0028 as the kernel grew; qekc grew it 128->144 in WL-0025; `IMG_MIN`=1+16+160=177
<= `IMG_SECTORS=192` = 3 whole cylinders, no IMG change; a boot-geometry change is a
tri-emulator obligation, Rule 5 -- re-verified on Bochs in WL-0028). The boot image is padded to a whole 2x32 cylinder geometry
(`IMG_SECTORS=192`, build-guarded) so the **Bochs boot leg passes**. Plus the
separate `make test-boot-bochs` (the Bochs boot leg; env-specific Bochs +
~45 s, NOT in the default `make test`). `make factory` builds; `make` prints
help. The default boot image (`build/tracer_boot.img`) is now the **shell**
kernel; the baked-demo gates (`test-program`/`test-type`/`test-dir`) boot
`build/demo_boot.img`; `test-fs` adds `--disk2 build/fat_data.img`.

### 4.2 See it
`qemu-system-i386 -drive format=raw,file=build/tracer_boot.img -drive
file=build/fat_data.img,format=raw,if=ide,index=1 -serial stdio` → banner + a
`Directory of A:\` listing + the **`A:\>` COMMAND.COM prompt** on the seafoam
desktop. (Under Bochs: `make test-boot-bochs` — same boot via the mode-0x13
fallback, asserted on serial.)

## 5. Branch state + next work (resume here)

> **CURRENT STATE (2026-06-28, WL-0066 -- THE LIVE-GUI BUG HUNT. FEATURE WORK IS
> HALTED. START HERE: a FIX arc, NOT new features.) The live FLAIR desktop is badly
> broken; 138 real bugs are filed.**
>
> **What happened:** Phase 4.5 (WL-0065 block below) shipped 5 commits on GREEN
> *host* oracles + a single boot-frame screendump, but NOBODY DROVE THE LIVE DESKTOP.
> The operator ran `make run-flair-tenants`, found ~5 bugs in seconds, and (rightly)
> rejected it. The oracles "grade math, not composited pixels or live behaviour"
> (initech-pipa). An interaction-testing method was researched (bead `initech-l9cd`)
> and had not been applied.
>
> **The hunt:** 3 adversarially-verified fan-out rounds (inspect lenses -> opus
> verify killing NOT_REAL/canon/dup -> dedup). **138 NEW bug beads filed (label
> `gui-bughunt`) + 4 known (pipa/34gp/rgt8/z1f5) = 142 distinct: 2 P0, ~20 P1, rest
> P2.** Full report: `docs/FLAIR-GUI-bug-hunt-2026-06-28.md`. `bd stats` 438 -> 579.
>
> **THE TESTING METHOD (use it -- this is the standing way to test the GUI):**
> `build/qemu_harness --disk build/flair_live.img` (or `build/flair_tenants.img`)
> `--mouse 'm<dx>:<dy>,l1,l0' --keys SPEC --keys-after FLAIR-LIVE-READY --screendump
> --screendump-after <MARKER>` (markers FLAIR-LIVE-READY/FLAIR-DRAG/FLAIR-MENU/
> FLAIR-CLOSE, os/milton/kmain.c). PPM->PNG via `pnmtopng | convert -filter point
> -resize 200%`, then Read it; sample exact RGB from `build/*.ppm` with python.
> Compare composited pixels to the `../system7-decomp` goldens (Law 4). The harness
> mouse Y axis is INVERTED (rel +y -> cursor up). See `bd memories
> flair-live-gui-testing-gap`.
>
> **WHERE TO START (the FIX arc, dependency-ordered):**
> 1. **The 2 P0s:** `initech-ojxn` (COPY <file> <file> truncates the file to zero +
>    reports success; OPEN src -> CREAT dst truncates the same file -> READ 0) and
>    `initech-mswo` (region_op xmerge scratch[256] stack overflow when na+nb>256).
> 2. **The highest-impact GUI P1s:** the compositor does NOT damage-track the
>    non-WindowMgr layers (menu bars/modal/shadows/scrollbars) NOR repaint background
>    windows -> drag + app-switch ERASE peer windows (pipa, qi8v, jmc5, 4w15); and
>    there is NO active/inactive window chrome at all (`flair_draw_document_window`
>    has no `hilited` param -- a9iq + the v6t2 damage half). Fix these two families
>    and the desktop stops corrupting.
> 3. The rest of the ~20 P1s (dos_read CF `winh`; SAMIR result bugs eyig/9u0f/jf8p/
>    0g22/dmrw/...), then the P2 tail (chrome-fidelity placeholders + the Rule-6
>    mutation-unproven oracle-gap holes).
> 4. **Prevention:** stand up composited-pixel emu gates + the `initech-l9cd`
>    deterministic GUI-interaction capture so this class cannot ship green again.
>
> Each fix is Red->Green: reproduce the bug with a NEW failing oracle (drive the
> harness for behaviour bugs; a unit/property test for the code bugs), fix the root
> cause, re-run the FULL gate + the new oracle. Do NOT resume the canonical app suite
> (Initech123/InitechWord) until the GUI is fixed. Uncommitted `os/flair/stdfile.*`
> (Lane F, Standard File, HALTED mid-build) is left UNTRACKED -- the operator decides
> whether to land or discard it.
>
> ---
>
> **PRIOR STATE (2026-06-27, WL-0065 -- FLAIR PHASE 4.5 PLATFORM SERVICES,
> FIRST WAVE. THIS SUPERSEDES the WL-0064 App Contract block + the WL-0062 block
> below.) The App Contract is now FULLY COMPLETE and the first two shared Toolbox
> services ship.**
>
> Orchestrated session (committee for the scope/architecture fork + delegated
> coding lanes + independent orchestrator grading), 3 waves: `3eb39e4` (Wave 1)
> + `67f96f1` (Wave 2) + `1c895cc` (Wave 3), all pushed. The plan-of-record's next arc -- **Phase 4.5
> Platform Services (`initech-49ez`, ADR-0012 D-2b)** -- is underway; these shared
> services LAND BEFORE the canonical app suite so Initech123/InitechWord build ON
> them (ADR-0012 D-2c). Committee `wf_00931e9e` (3 seats -> chair -> adversarial
> verify = PROCEED-WITH-AMENDMENTS) ruled the wave + folded 5 amendments.
>
> 1. **ubd0 split-arena (ADR-0013 Amendment AC-2, `initech-ubd0` CLOSED)** -- app
>    DEATH now provably survives a corrupt child arena. A tenant carves TWO master-
>    heap blocks: a RECORDS block (FLAIR_CLASS_HANDLE -> `records_arena`:
>    WindowRecord + region pools) + the DATA block (GENERAL -> `arena`). The shell
>    reads ONLY `records_arena` at teardown; new `FlairProcess_kill` death path.
>    BC-6 now SATISFIED (was FALSE). Closing this AUTO-CLOSED the App Contract epic
>    `initech-4e35`.
> 2. **Resource Manager (`initech-0w45` CLOSED)** -- `os/flair/resource.{c,h}`: a
>    clean-room parser of a REAL big-endian Mac resource-fork subset (per
>    `system7-decomp` resource-manager.md), type+ID lookup into the tenant DATA
>    arena. Round-trip oracle vs an independent hand-authored fork+expect golden
>    (NOT by-construction). The verifier caught + fixed a 22-vs-24-byte map-preamble
>    off-by-two before any code.
> 3. **Scrap/Clipboard (`initech-b2vk` CLOSED)** -- `os/flair/scrap.{c,h}`: a shell-
>    owned cross-tenant Scrap (TEXT+PICT flavors), the co-residency copy/paste
>    payoff; independent op/byte oracle incl. the cross-tenant leg.
> 4. **TextEdit + List Manager (`initech-77dj` CLOSED, Wave 3, `1c895cc`)** --
>    `os/flair/textedit.{c,h}` (reduced TERec: half-open selection, CR+wrap line-
>    breaking, TECut/TECopy/TEPaste through the Scrap -- the first Scrap consumer) +
>    `os/flair/list.{c,h}` (reduced ListRec: cell store + LClick hit-test + lOnlyOne).
>    The text-entry + list floor. Oracles 55/55 + 49/49; 6 mutants RED.
>
> **`make clean && make test-unit` = ALL GREEN 291 host** (Wave 1+2 full run was
> 287 host + 57 emu + Bochs; Wave 3 is host-only -- neither module in any kernel
> object -- so emu/Bochs unchanged); all new mutants RED; reproducible; ASCII-clean.
>
> **NEXT WORK (Phase 4.5 remaining, dependency-ordered -- `bd show initech-49ez`,
> now 3/5 service children done):** `gymo` Standard File / Common Dialogs (now
> UNBLOCKED -- Dialog Mgr + List Mgr + TextEdit + the MILTON FAT enumerator all exist;
> honor the Law-1 acquisition gate for the SFGetFile navigation model first, no decomp
> spec in the corpus) -> `o5vm` Print Manager (BLOCKED until the GrafPort verb layer,
> re30 P3-pre -- grafProcs verified NULL today). Plus 3 filed follow-ons: `ww9c` (wire
> the Scrap singleton into the live desktop, copy/paste HELLO<->NOTES), `0lko` (Resource
> per-type record instantiation), `ncfu` (full TERec lineStarts + List scroll/multi-
> column/drag-select). Then the canonical app suite (Initech123/InitechWord) builds ON
> these services. See **WL-0065**.
>
> ---
>
> **PRIOR STATE (2026-06-26, WL-0062 -- THE LIVE INTERACTIVE FLAIR DESKTOP +
> the chrome FIDELITY arc. THIS SUPERSEDES the WL-0060/0061 block below.) The booted
> 386 desktop is now INTERACTIVE: windows DRAG and menus DROP+SELECT under the mouse.**
>
> Orchestrated session (committee fork + delegated coding lanes + independent
> grading), 13 commits `93e8cd4..7dcc4e1`, all pushed. TWO arcs landed:
>
> 1. **Window-chrome FIDELITY (umbrella `initech-hmll`, CLOSED)** -- all 6 elements
>    graded vs the INDEPENDENT `../system7-decomp` goldens via `test-chrome-fidelity`
>    (NOT by-construction): pinstripe phase + title text (prior) + `54nw` drop shadow +
>    title-bar-only groove + `ts3t` close/zoom box (11x11, +9/-20, 3-D double bevel +
>    zoom glyph, canon-teal bevel) + `jh7m` scrollbar (black-outer/gray-inner + arrow
>    glyphs + solid #F3F3F3 track) + `92li` title bevel rows + exactly-15-row interior
>    recomposition. The windows now LOOK like System 7. (Forced `KERNEL_SECTORS`
>    320->352, a Rule-5 boot-geometry change.)
> 2. **Live EVENT LOOP (`initech-5l5z`, core DELIVERED; re-flair epic `qipc` step 7)** --
>    behind `-DBOOT_FLAIR_LIVE` (default boot UNCHANGED, Rule 11): 3 ISR producers feed
>    the FLAIR raw ring via function-pointer hooks (FO-4 PIT tick / FO-5 kbd IRQ1 /
>    FO-6 mouse IRQ12 -- the dual-PIC-EOI minefield, no-wedge proven, weak-extern stub
>    so the 26 non-flair kernels stay byte-stable); the FO-7/8 WaitNextEvent cooperative
>    pump replaces render-once-HLT and dispatches FindWindow part-codes -> DragWindow +
>    D-5 desktop_paint_damage (windows DRAG) + MenuSelect (FO-8b menus DROP+SELECT) +
>    close-box. Oracles: test-interact (HOST, independent-by-recomputation) +
>    test-flair-drag + test-flair-menu (EMU, independent-canon screendump) + the HER-14
>    drag-noop/menu-noop mutants. **Law 4's "live, draggable arrangement with working
>    menus" is satisfied.**
>
> **`make clean && make test` = ALL GREEN 273 host + 53 emu** (entry 271+43); both
> Bochs legs PASS; reproducible every step; `_kernel_end(flair_live)=0x37174 < 0x40000`.
>
> **NEXT WORK -- the major arc is DONE; remaining are FILED follow-ons (polish/hardening):**
> menu close-restore + CombineRgn panel clip (rmsr/FO-D2-8); the live drop-shadow via
> strucRgn-widening (`initech-9d0e`); the mouse-ACK phantom-keyDown drain; the Bochs
> strict-PIC mouse probe (`initech-04ae`); the `test-arena-disjoint` ASLR determinism
> (a PRE-EXISTING Rule-11 flake -- mmap(NULL,MAP_32BIT) -- that can trip any `make test`).
> A fresh major FLAIR arc (e.g. the canonical app suite, or the Turbo Initich compiler)
> is the operator's call. See **WL-0062**.
>
> ---
>
> **PRIOR STATE (2026-06-25, WL-0060+WL-0061 -- WINDOW-CHROME FIDELITY ORACLE; see
> the "Last Reconciled" headline above for the full narrative. Superseded by WL-0062
> above; the re-flair epic `initech-qipc` steps 1-6 landed WL-0054..0059.)**
>
> **DONE this session (2 commits pushed; oracle-first RED->GREEN, each fully graded):**
> the audit established that NO gate enforced window APPEARANCE vs the
> `../system7-decomp` captures, then built **`test-chrome-fidelity`** (grades the REAL
> `chrome.c` vs the INDEPENDENT pixel-measured golden `spec/chrome_fidelity_golden.h`)
> and landed two elements: **pinstripe PHASE-LOCK** (`cb76816`, WL-0060) and **TITLE
> TEXT + knockout** (`dc5ddca`, WL-0061; `initech-lxg9` CLOSED; root-cause `NewWindow`
> titleHandle fix). **271 host gates GREEN; Rule 5 QEMU+Bochs PASS; reproducible.**
>
> **NEXT WORK (remaining window-chrome fidelity elements, open dependents of umbrella
> `initech-hmll`, each oracle-first + Rule-5):** `ts3t` close/zoom box geometry (11x11,
> close +9 / zoom -20 offsets, zoom nested-square glyph; FLAIR draws flat 13x13 boxes),
> `92li` bevel rows + 15-row interior recomposition (shifts `content_top` -> amend
> `ppm_flair_check` W1 geometry, a wider blast radius), `jh7m` scrollbar arrow glyphs +
> thumb + black-outer/gray-inner separators (FLAIR draws empty boxes), `54nw` window
> drop shadow (1px L at offset (1,1); `ppm_flair_check.c` currently asserts the pixel
> outside the frame is bare teal -- amend it). Recommended next: `54nw` (most visible,
> contained) or `ts3t`. The re-flair finale `initech-5l5z` (live WaitNextEvent loop,
> ADR-0006) remains the other open arc.
>
> ---
>
> **CURRENT STATE (2026-06-21, WL-0049+WL-0050 -- FLAIR CONSOLIDATION + EXPANSION;
> the active arc. Supersedes the WL-0046..0048 block below for FLAIR; the MILTON
> redirection arc bsy.7/.8/.9 remains open in parallel.)**
>
> **FLAIR is the crown-jewel north star now: a period-authentic GUI that boots
> live and hosts all serious era apps as Initech-versions.** A 7-lane understand
> workflow (`wf_e7335f49-a55`) + a completeness critic established the ground
> truth and the critic cross-verified it: **FLAIR is host-green (254 host gates)
> but does NOT boot** -- only `surface.o` links into the kernel image; the booted
> OS still shows seafoam + a console banner; the locked `grafport.h`/`imaging.h`
> is partly aspirational decoration (no drawing verbs; `grafProcs` NULL-only).
>
> **The plan-of-record is `docs/plans/FLAIR-implementation-plan.md`** (read it):
> 7 phases, dependency-ordered, **era-layered (System 7.0/7.1 base NOW; a System 8
> Platinum layer ACCRETES later** -- the Office Space frame is likely Platinum,
> "never delete, always accrete"). Ratified operator forks: **Doom = a flat-32
> SOURCE PORT (NO DPMI/extender -- the OS is already 32-bit flat); Minecraft =
> native "Initech Mines" (literal JVM out by Law 3 + ADR-0001); M5 native frame
> apps BEFORE the hosting arc; the canonical productivity suite comes first --
> InitechBase (=SAMIR, done-ish) -> Initech 123 (Lotus) -> InitechWord
> (WordPerfect).** Bead DAG: phase epics `v94x`(P0) `dh5k`(P1) `re30`(P3)
> `4e35`(P4) `t4hp`(P6) + the critical-path chain; stale `8oi`/`ox7` superseded.
>
> **DONE this session (2 commits, pushed; orchestrated -- delegate/grade/commit):**
> 1. **Phase 0/1 -- the era-layered CANONICAL SPEC (the red-green target)**
>    (commit `fe4d9b1`; **244 -> 254 host gates green**). 6 disjoint sonnet lanes,
>    orchestrator-graded: `spec/control_record.h`/`menu_record.h`/`dialog_record.h`
>    + `drawing_ops.h` (QuickDraw verb/CopyBits/coord/pattern op semantics) + the
>    real 256-entry `spec/assets/clut.{json,h}` (from the system7-decomp ROM
>    golden) + ingested chrome RGBs OUT of `golden_resolves` (pinstripe/bevel/box/
>    scrollbar + win31 SM_CYCAPTION=18) + the **`#dfdfdf`->`#ffffff` Law-3 fix** +
>    `spec/CANON-MANIFEST.md` (all 88 corpus specs mapped) + `win95ism_guardrails.md`.
>    Docs reconciled: CLAUDE.md/PRD "MZ deferred" -> **InitechMZ SHIPS** (DEC-08a);
>    PRD dBASE IV -> III+ (ADR-0008). All 5 new gates + mutants bite.
> 2. **Phase 3 committee ruling + B0** (commit `50c1374`; WL-0050). The committee
>    (2 opus seats) ruled **GROW the impl** (build the GrafPort verb layer + ONE
>    palette module on the blitter; the locked spec is ratified truth, not
>    decoration) on a **bisectable boot sequence**: M3.1 ships the static desktop
>    on the existing `cfill` path; the verb refactor lands PARALLEL, off the
>    first-pixels critical path. **B0** added a **mutation-proven kernel.ld
>    fail-loud `ASSERT(_kernel_end < 0x38000)`** (top-level; a bare ASSERT inside
>    SECTIONS is an ld syntax error -- caught during grading) and corrected the
>    stale `memory_map.h` headroom comment. **Key measured finding: the BOOTING
>    shell kernel has only ~29 KiB headroom (`_kernel_end=0x30920`) < FLAIR's
>    ~51 KiB** -- so M3.0 WILL trip the assert.
>
> **NEXT WORK (Phase 3, dependency-ordered -- `bd show initech-re30`):**
> 1. **`re30.2` M3.0 (the gating step):** the **pre-authorized +0x8000
>    PROGRAM_BASE raise** (0x38000 -> 0x40000, 192 KiB window) -- a Rule-8
>    whole-map shift mirroring **o0td EXACTLY**, PRESERVING SAMIR's heap arena
>    byte-identical (the Wave-4 raise-alone BROKE it; o0td's pairing did not),
>    then link the FLAIR Manager set into `KERNEL_OBJS`. **FULL `make clean &&
>    make test` incl. emu + Bochs** (kernel/memory-map change = tri-emulator
>    obligation, Rule 5; the WL-0028 hard lesson). The kernel.ld assert is the
>    safety net.
> 2. **`re30.3` M3.1 (the milestone):** static live desktop via `shell_render` to
>    the LFB in kmain + a new `test-flair-desktop` emu gate (QEMU screendump
>    structural-band + Bochs serial) -- the chimera desktop LIVE on the 386.
> 3. **Verb-layer track (parallel, off critical path):** `re30.4` one palette
>    module -> `re30.5` verbs + grafProcs -> `re30.6` per-Manager flip ->
>    `re30.7` CopyBits/GWorld. **Input arc:** `re30.8` tick -> `re30.9` keyboard
>    -> `initech-26d` mouse/IRQ12 (Bochs-gated for dual-PIC EOI). **Fidelity:**
>    `initech-u9gf` (Bochs RFB) + `k8o5.11` (dual-target digest); host-model-
>    PER-MODE (AM-6), SSIM guide-not-gate; LAND 86Box (`q0gy`) at M4, don't waive.
>
> **Orchestration cadence (operator-set):** delegate each coding step to a
> subagent (<=6 sonnet / <=2 opus parallel; DISJOINT files per lane, shared files
> SERIAL); the orchestrator OWNS Makefile integration + independent re-grading
> (Law 2/4: re-run oracle + mutant + the FULL clean aggregate, NEVER trust the
> report -- this session caught comment-only "decoration" mutants + a faked-pass
> ld syntax error that way) + commit-per-wave + the bead ledger; convene the
> **committee** for serious forks (it ruled Phase 3 GROW-vs-trim + the boot
> sequence); committee has ultimate control, escalate only on gridlock.

> **CURRENT STATE (2026-06-20, WL-0046..0048 -- supersedes the WL-0040..0045 block below).**
> **THE KERNEL-WINDOW WALL IS GONE and I/O REDIRECTION WORKS.** Three features
> landed + pushed + green this session (the o0td -> k36g -> hsct arc), all
> orchestrated (committee + delegated lanes + independent grading):
>
> 1. **`o0td` (P1, CLOSED, commit bae46d4)** -- the conventional-memory-map
>    redesign. PROGRAM_BASE 0x30000->0x38000 (whole-map shift +0x8000), kernel
>    window [0x10000,0x38000) = **160 KiB (+32 KiB)**, reclaiming half the dead
>    [0x90000,0xA0000) gap. An **ADR-by-committee ratified PATH 2** after the
>    orchestrator MEASURED that the bead's prescribed fix (cut PROGRAM_BSS_RESERVE
>    to 32 KiB) was UNSOUND -- SAMIR's real .bss is **47.3 KiB (0xBD20)**, not the
>    "26 KiB" documented in memory_map.h/ADR-0009 (corrected). Reserve HELD at
>    0x10000; SAMIR's heap arena preserved byte-identical (test-samir-boot PASS --
>    the exact oracle that went RED at the earlier raise, y206). Orchestrator
>    amendment (Law 4): kstart ESP 0x8FFFC->0x97FFC (the chair's "ESP unchanged"
>    would have overlapped LOAD_STAGING with the kernel stack). Full clean grade
>    caught 2 blast-radius misses (test_exec.c latent 0x5F000; the FACTORY
>    `harness/diff/dbf_diff/test_hardware_spec.c` + `spec/hardware.json` regression
>    guards) -- always `make clean && make test`. **BATCH_FILE_MAX restored 4096.**
> 2. **`k36g` (P2, CLOSED, commit 7de96e6)** -- INT 21h AH=09h/02h/06h CON output
>    now routes through the redirectable STDOUT handle 1 (`stdout_emit`, with a
>    `con_putc` fallback when handle 1 is unresolvable so the early banner / every
>    pre-PSP diagnostic still prints). Un-redirected console output is byte-for-byte
>    unchanged (still -> con_putc -> ANSI + g_sink). New mutation-proven `test-redir`.
> 3. **`hsct` OUTPUT increment (P2, IN_PROGRESS, commit 24194ed)** -- COMMAND.COM
>    `>` (create/truncate) and `>>` (append) redirection for builtins.
>    `cmd_redir_parse` + `run_with_redirect` (DUP/DUP2-around-dispatch, restore on
>    every path). **`echo HELLO > FILE.TXT` + `>>` append PROVEN end-to-end on the
>    386** (emu gate `test-hsct-redir`). HONEST GAP (Law 1/2, filed not papered):
>    external `.COM` EXEC output does NOT redirect -- `psp_build` hard-resets the
>    child JFT slot 1 to CON with no inheritance; **`bsy.9`** filed (loader
>    child-PSP-inherits-parent-JFT), **hsct depends on it**.
>
> **`make test` = 244 host + 41 emu + Bochs GREEN.** All three commits on
> origin/command-com-default.
>
> **NEXT WORK (the redirection arc continues -- all filed, dependency-ordered):**
> 1. **`bsy.9`** (loader: child PSP inherits the parent JFT) -- a loader/psp.c
>    change; UNLOCKS external `.COM` EXEC output redirect (`myprog > file`), the
>    last gap for full output redirection. Most consequential next step.
> 2. **`bsy.7`** (`<` stdin redirect) -- symmetric to output (DUP2 onto handle 0);
>    cheap in command.c, mirrors run_with_redirect.
> 3. **`bsy.8`** (`|` pipe via temp-file) -- two-command, temp-file between; the
>    most complex, depends on bsy.7.
> When bsy.7/.8/.9 land, hsct closes. Other ready directions unchanged below
> (40oq literal-100% / FCB; the filed emu-deepening gates; FLAIR in-OS arc).
>
> **Orchestration note (operator-set, proven this session):** convene the
> committee for serious/contested decisions (it ruled o0td PATH 2; the orchestrator
> graded + amended it), delegate each coding step to a subagent (sonnet default /
> opus for load-bearing; disjoint files parallel, shared files serial), and the
> orchestrator OWNS grading (re-run the oracle + mutant + the FULL clean aggregate
> + Bochs; NEVER trust the report -- Law 4 caught the unsound 26-KiB premise, the
> chair's staging overlap, two missed literals, and the EXEC-inheritance gap).

> **CURRENT STATE (2026-06-20, WL-0040..0045 -- supersedes the WL-0038 block below).**
> The DOS-3.3 parity push (MILTON) reached its **FUNCTIONAL CAPSTONE** over 5
> orchestrated waves (all pushed, 9a5fea3..4e60bd5; **240 host + full emu (39) +
> Bochs green**). The INT 21h Appendix-A surface is CERTIFIED feature-complete
> (`make test-40oq`: 55 dispatched, ZERO unwaived not-yet-impl gaps; FCB-waived;
> partitions+multivol deferred). ANSI.SYS works in-OS (colour/cursor/erase when
> CONFIG.SYS loads DEVICE=ANSI.SYS); the device chain (CON/NUL/PRN/AUX/CLOCK$)
> answers OPEN-by-name; INT 24h critical-errors raise from the disk layer; KEEP/
> TSR, .BAT/AUTOEXEC all land. Beads closed this push: xw1, bo40, mvg, 509.7,
> 6zd9, p96i, x3mh (40oq functional-certified, stays open on its deferred deps).
>
> **THE GATING ISSUE NOW: the kernel window [0x10000,0x30000)=128 KiB is FULL.**
> `_kernel_end` sits ~1.5 KiB under PROGRAM_BASE; the Wave-6 hsct redirect driver
> could not fit (reverted, WL-0045). **`initech-o0td` (P1) is the prerequisite for
> ANY further kernel-resident growth:** raise PROGRAM_BASE 0x30000->0x34000 PAIRED
> WITH PROGRAM_BSS_RESERVE 0x10000->0x8000 (the pairing PRESERVES SAMIR's heap
> arena -- the Wave-4 raise-alone BROKE SAMIR, y206 superseded). Do it carefully:
> the .COM-org blast radius (committee-mapped in y206) + verify SAMIR's BSS fits
> 32 KiB + full emu + Bochs (Rule 5). Then restore BATCH_FILE_MAX 1536->4096.
>
> **NEXT WORK (pick per operator steer):**
> 1. **`o0td` (P1) -- the kernel-window memory-map redesign.** Unblocks everything
>    kernel-resident. The careful raise+BSS_RESERVE-cut above.
> 2. **`hsct` (redirection) redo** -- after o0td (room) + the new bead "route
>    AH=09h/02h/06h CON output through handle 1" (so builtin `echo > file`
>    actually redirects, not just AH=40h filters). Its parse/driver design is
>    sound (WL-0045); redo once both prereqs land.
> 3. **40oq literal-100%** -- the OPERATOR go/no-go the cert surfaces: dispatch
>    FCB (509.9, consumer = TPS Report Generator 8479.1) and/or lift the kzfs
>    (MBR partition) + slvd (multi-volume) deferral. Operator deferred kzfs
>    2026-06-15 ("no in-tranche consumer"); raise when FAT16-HDD/multivol land.
> 4. **Filed emu-deepening gates** (host-proven, in-emu deferred): device-OPEN,
>    INT-24h-trigger, KEEP-survival, the in-emu ANSI screendump.
> 5. Other parity: `f9z4` (VOL/VERIFY/BREAK/CTTY/TRUENAME built-ins), the
>    installable DEVICE= driver loader, ANSI DSR (ESC[6n) kbd-inject.
>
> **Orchestration note (operator-set this session):** delegate each coding step
> to a subagent (sonnet default / opus for load-bearing; <=6 sonnet / <=2 opus
> parallel; DISJOINT files per lane, shared files SERIAL); orchestrator owns
> grading (re-run the oracle + mutant under -Werror; NEVER trust the report --
> Law 4 caught the SAMIR break + the hsct AH=09h gap), Makefile integration,
> commit-per-wave, the bead ledger; convene the committee for serious decisions
> (it ruled on the PROGRAM_BASE raise -- and the orchestrator overturned its
> implementation when grading found it broke SAMIR).

> **CURRENT DIRECTION (2026-06-20, WL-0038 -- supersedes the FLAIR "NEXT ARC" below).**
> The operator PAUSED FLAIR (golden masters minted in sister repos `../system7-decomp`
> + `../win31-decomp`) and directed an ORCHESTRATED push to full **DOS 3.3 parity**
> (MILTON). Two operator rulings (`bd memories`): (1) parity = "Appendix-A now,
> amendments later" -- COUNTRY/SHARE/INT-2Fh go to a separate amendment-gated epic
> (`om2a`/`ws3x`/`t1hl`); (2) the `40oq` capstone certifies with FCB stubbed.
> **DONE this session (see WL-0038):** Tranche E env-store COMPLETE (`1i0x` closed);
> MZ `.EXE` end-to-end (ADR-0003 **DEC-08a**, committee-ratified -- a real InitechMZ
> `.EXE` PROVABLY RUNS in-emulator via `test-mzexec`); Tranche F verbs (COPY/DEL/REN/
> DATE/TIME, `hpls`/`fyox`/`uy4l`); Tranche G start (PROMPT `dibc`, PATH/COMSPEC `atf`,
> the `.BAT` parser module `xw1`). **224 host gates + emu.** **NEXT:** the `xw1` command.c
> `.BAT`/AUTOEXEC integration (parser ready); Tranche I (`mvg` INT 24h wiring, `509.7`
> device chain); `bo40` AH=31h KEEP; then the **`40oq` capstone**. **HARD LESSON
> (`bd memories`):** EXEC-path / int21 / loader edits MUST re-run the EMU EXEC gates
> (test-exec/test-program/test-zs24-exec/test-ut6d/test-samir-boot), not just
> `make test-unit` -- a real do_exec env_block regression passed host-green but broke
> the emu `test-exec`.

**Branches + remote (a remote now exists — this supersedes the old "local-only,
do not push" note):**
- **`origin` = github.com/tobiasosborne/InitechOS** — PUBLIC, AGPL-3.0. The
  default/showcase branch on GitHub is **`main`**; `command-com-default` is the
  active working branch. BOTH are pushed. Session-close now pushes to `origin`.
- `command-com-default` — the active tip: WL-0016 + WL-0018 + WL-0019 (509.6
  wiring + Bochs geometry fix + the kernel-completeness plan). `main` == HEAD.
- `kernel-hardening`, `master` — older local branches, linear ancestors of the
  tip; left as-is.

**M2/M3 internals + the shell are DONE and green** (the stale "M3 in progress"
plan that lived here is superseded — see §4's WL-0008–WL-0016 lines). The DOS
personality boots to an interactive `A:\>` on QEMU and Bochs.

**ACTIVE WORKSTREAM — DOS 3.3 feature-parity push (epic `initech-bsy`, WL-0018).**
The operator directed a sustained push to DOS 3.3-5.0 parity (spirit, not literal,
where it conflicts with the north star), oriented around the existing beads. A
grounded gap-map established that within ADR-0003 Appendix A the real INT 21h gap
is `39h/3Ah/3Bh` MKDIR/RMDIR/CHDIR, `43h` CHMOD, `44h` IOCTL, `48h/49h/4Ah`
ALLOC/FREE/SETBLOCK, `56h` RENAME, `57h` FILETIME, `5Bh` CREATNEW (all recognized
by `ah_is_listed()` but NOT dispatched), plus the shell built-ins + batch + env.
`bd show initech-bsy` carries the full sequenced build order (Tranches A-I).
**Landed (WL-0019):** `initech-509.6` is DONE — AH=48/49/4Ah wired to the MCB
arena (authentic single-big-block over the locked [0x30000,0x70000) window, NO
spec edit) + the Bochs `IMG_SECTORS` geometry fix. The kernel-completeness gap is
now a **40-bead plan**: Phase 1 (kernel feature-complete, children of
`initech-bsy`, capstoned by **`initech-40oq`** the coverage certificate), Phase 2
(shell built-ins + the new `util-epic`), Phase 4 (the parked `dos5-epic`,
ADR-amendment-gated). FCB (`509.9`) backburnered (P4) but REQUIRED; its flagship
consumer is the **TPS Report Generator** (`8479.1`). Canon beads: Y2K accounting
(`586.1`), Michael Bolton's rounding-error virus (`586.2`), the `packaging-epic`
(the final straight build).
**`initech-ti8` Layer 1 is DONE + green (WL-0021)** — the additive fat12 layer
(`fat12_dir_t` + `fat12_read_dir` + `fat12_resolve_path`, READ-side; 4 root
primitives byte-unchanged; `test-fat-subdir` 3-way differential + 2 mutants).
Grounding split ti8 into L1 (fat12, done) and **`initech-mzxa`** = Layer 2, the
Core-tier INT 21h `int21_file_backend_t` vtable cross-cut (root-only today;
threading a resolved dir touches int21.h + fileio_fat.c + 3 host mocks + g_cwd +
5 rejection sites: do_open@858 / do_creat@931 / do_unlink@995 / do_findfirst@1405
/ do_exec@1476). **`initech-mzxa` (Layer 2) is now DONE + green (WL-0022)** —
the vtable threads a `uint16 dir_start_cluster` (0==root, root path byte-identical)
through `int21_file_backend_t`; `do_open`/`creat`/`unlink`/`findfirst` resolve
`\SUB\FILE`; `g_cwd` plumbing established (reset on launch/terminate, save/restore
around the kernel PSP rebinds, `do_getcwd` wired); the strongest oracle binds the
REAL `fileio_fat` backend over `fat12_nested.img` (45 checks). DOS-correct codes
(0x0002 missing leaf vs 0x0003 missing/non-dir component).

**`initech-u6wa` is DONE + green (WL-0023)** — CHDIR (3Bh) + root-level MKDIR
(39h)/RMDIR (3Ah). CHDIR added a `resolve_dir` backend member + fixed a latent
bug (`fat_resolve` ignored `cwd_start`, so relative resolution was root-anchored);
MKDIR/RMDIR added `fat12_mkdir`/`fat12_rmdir` + a dot-dir writer, the `..`=parent/0
convention pinned EMPIRICALLY from `mmd` and gated by an `mmd` differential. DOS
codes: MKDIR-exists/RMDIR-non-empty 0x0005, RMDIR-of-CWD 0x0010 (new
`INT21_ERR_CURRENT_DIR`).

**`initech-ut6d` + `initech-zs24` are DONE + green (WL-0024)** — the
subdirectory personality is now usable END-TO-END. `ut6d` (`ea4b47d`) wired the
COMMAND.COM REPL to AH=39/3A/3B + a GETCWD-composed `$P$G` prompt ("A:\SUB>"),
behind a new DEC-13 catalogue amendment (`mc7r`/`9e2238b`: MSG-DOS-0017/0018/0019,
catalogue 16->19). `zs24` lifted the ROOT-only write/EXEC restriction in two
serial landings: **L1** (`610c656`) subdir file CREATE/WRITE/UNLINK — the
parent-aware fat12 core (`fat12_scan_dir`/`_write_dirent_in_dir`/
`_subdir_slot_lba`/`fat12_grow_dir`), root fns now byte-identical is_root=1
wrappers, SFT carries `dir_start`, fail-loud `spc!=1` mount guard; **L2**
(`b67028b`) subdir EXEC — `do_exec` reuses the `do_open` resolve seam,
`load_program_from_fat` branches dir_start==0 (byte-identical `fat12_find`) /
!=0 (`fat12_find_slot_in`), with a dir-attr guard. Each landing was self-run
`make test`-gated AND independently adversarially verified before commit; the
adversarial pass caught `fat12_grow_dir` shipping untested despite a green suite
(closed with a grow oracle + 2 grow mutants).

**Resume the DOS-3.3 parity push (epic `initech-bsy`, 8/22): `bd ready`.**
**WL-0025 completed the Appendix-A INT 21h handler tranche** — CREATNEW (5Bh),
FILETIME (57h), CHMOD (43h), RENAME (56h), IOCTL (44h AL=00), nested MKDIR/RMDIR
— PLUS **ADR-0003 Amendment DEC-15** ratifying the absolute-disk vectors and
**INT 25h/26h** (`4mq7`) implemented against it. The Appendix-A INT 21h surface
is now fully dispatched. Filed follow-ups (none blocking): `4nbn` (IOCTL minors +
device-info bit-name labels), `5o6o` (CHMOD SET-reject deviation, doc), `isil`
(gnrc non-root same-dir rename leg), `ycb3` (cross-dir RENAME move), `nmpo`/
`glsw` (CREATNEW oracle/robustness), `lpf3` (write-fail fault-injection backend
for the rollback paths), `cnvp` (absdisk spec bad-buffer row), `8403` (in-emu
int 25h/26h self-test), `t1on` (DEC-07 MBR-partition amendment, blocking the
deferred `kzfs`). The natural Phase-1 closer is the capstone `initech-40oq`
(Appendix-A coverage certificate). The SERIAL, oracle-gated discipline for
shared-file kernel edits still stands (Rule 3 / WL-0023-25).

**WL-0026 discharged the WL-0025 follow-up debt** (orchestrated: a serial coding
workflow + parallel adversarial verifiers + a forward-grounding workflow). Landed
green (90 host + 27 emu): `nmpo`+`glsw` (CREATNEW FS-effect oracle + e2e FAT12
differential + fail-loud open==NULL), `4nbn` (IOCTL AH=44h minors AL=01/06/07/08 +
PRM bit-label fix), `lpf3` (fault-injecting blockdev backend driving the fat12
rollback paths RED), `8403` (in-emulator int 25h/26h asm-path self-test), `cnvp`
(DEC-15 bad-buffer spec row + completeness gate), `isil` (non-root RENAME leg),
`5o6o` (CHMOD SET-reject deviation doc). Adversarial review caught + fixed three
Rule-6 "decoration" findings on the green suite (dead completeness branch, a
phantom-mutant ADR citation, an unmutated headline gate). New P3 follow-ups (none
blocking): `fgdz` (8403 emu serial-capture race -- green serially, flaky under
concurrent QEMU relaunch), `jplu` (4nbn AL=08 BL=drive fidelity), `aaan` (lpf3
mkdir-EOC-fail leg coverage), `815i` (nmpo host leg-(e) mock artifact).

**NEXT FORWARD TRANCHE (pre-grounded in WL-0026, dependency-ordered, another
SERIAL kernel lane):** `dao` (streaming cluster walk; kills the on-stack
chain[2880]; unblocks z01) -> `z01` (FAT16 read-only; initrd/tarfs deferred to a
discovery bead) -> `80k` (DOS 8.3 wildcard engine) -> `x8fs` (cooked CON
line-read; MUST precede `4tw`, shared conin_get_pb seam) -> `4tw`+`er3h` (Ctrl-C/
INT 23h + BREAK=) -> `mvg` (wire INT 24h to real ATA/FAT errors; consumes lpf3's
fault backend). **TWO operator gates (ADR-by-committee draft -> ratify, per the
DEC-15 pattern):** (1) **AH=33h** Get/Set-BREAK is NOT in the locked
`spec/int21h_register.json` / Appendix A -- `4tw`/`er3h` need it added (Rule 8
amendment); (2) **DEC-07 / `initech-t1on`** must rule on the `kzfs` MBR-partition
forks (start-LBA authority: partition-table vs BPB hidden_sectors; application
layer: mount vs blockdev; detection heuristic: BIOS-drive-number vs sector-0
byte-sniff) before `kzfs` can be implemented. Full grounding briefs (file-touch
maps, oracle + mutation plans) captured in the WL-0026 orchestration output.

**WL-0027 landed the FAT16 milestone + ratified both ADR gates.** `dao` (streaming
cluster walk -- killed the on-stack `chain[2880]`) and `z01` (FAT16 read DECODE
layer + a 3-way host differential `test-fat16`, FAT12 path byte-identical) are
green (93 host + 27 emu). **z01 is a PARTIAL delivery of its bead** (honest): the
decode is proven on the host, but the kernel cannot mount a real FAT16 volume yet
because the whole-FAT `g_fat[12*512]` buffer is too small (fails loud) -- the
windowed FAT-sector read is filed as **`initech-d27i`** (P1, now a `40oq` dep) and
is the true completion of FAT16. The operator ratified **DEC-16** (AH=33h -> Appendix
A; unblocks `4tw`/`er3h`) and **DEC-07a** (MBR partition contract; unblocks `kzfs`)
as drafted; their locked-spec edits execute atomically with the implementing beads
(er3h/4tw, kzfs), NOT on ratification. Next NON-gated forward items: `80k` (wildcard),
`x8fs` (cooked CON read; before 4tw). New follow-ups: `d27i` (P1 in-kernel FAT16),
`qywt` (P2 initrd discovery), `7mjc` (P3 corrupt-fuzz flake). The shared-tree
workflow race (verifiers reverting/restoring the uncommitted tree) produced two
transient false-alarm P0/P1s this tranche -- standing mitigation for the next lane:
isolated worktrees for mutation-running verifiers, or commit-per-landing.

**WL-0028 landed the forward tranche + the M6 evidence base** (103 host + 27 emu,
Bochs boot leg PASS). DONE + closed: **`d27i`** (windowed FAT16 mount -- the true
completion of FAT16; mode-aware bind keeps FAT12 whole-FAT for WRITE, windows FAT16
for READ; FAT12 straddle byte-identical), **`80k`** (the FCB wildcard matcher was
already correct -- landed the mutation-proven oracle + corrected a ground-truth
guess), **`x8fs`** (AH=3Fh cooked CON read), **`er3h`** (AH=33h Get/Set BREAK +
CONFIG/built-in + DEC-16 spec edits), **`4tw`** (^C -> INT 23h on 01h/08h/0Ah; 07h/06h
RAW; `KERNEL_SECTORS 144->160`). Filed `docs/research/dbase-ground-truth.md` (M6
InitechBase/SAMIR evidence base) + re-parented the 7 engine beads under `586` + filed
`586.3/586.4`. Three lessons (see WL-0028): (1) **`t6nc`** -- Workflow worktree-isolation
spawns from STALE `main` (e56695a), not the branch tip; keep `main` fast-forwarded or
reset worktrees onto the tip. (2) **A host oracle HANG is worse than red** -- x8fs's
cooked CON read spun on `conin_get`'s no-source 0; only the FULL aggregate gate caught
it; ALWAYS `make clean && timeout 1200 make test` at integration (`bd memories`:
host-oracle-hang-pattern). (3) **Adversarial review caught a real DEC-16 deviation**
(er3h SET wrote AL) -- fixed + M7 mutant. Next: `bsy.1` (DIR wildcard emu leg), `bcg.16`
(in-emulator FAT16 mount oracle -> `40oq` FAT16-green), remaining MILTON shell built-ins.

**SAMIR / M6 (InitechBase) LAUNCHED -- architecture ratified + Phase-0 foundation green
(WL-0029).** The operator opened M6 and directed a corpus-grounded, platform-agnostic,
orchestrated build. **`ADR-0008` is RATIFIED** (ADR-by-committee): the **Platform Abstraction
Layer** (`os/samir/include/samir/pal.h` -- the engine's ONLY OS surface, so InitechDOS drift
touches only `pal_milton.c`), the artifact/factory storage split (`os/samir/` engine +
`harness/diff/dbf_diff/` grader + `spec/samir/` locked data), **dBASE III PLUS 1.1-ONLY**
(IV/`.mdx` dropped; `PRD §6.6` reconciled), and a three-tier corpus-backed grader. The full
**~38-step DAG is materialized** under epic `initech-586` (the granular plan is
`docs/plans/SAMIR-implementation-plan.md` -- THE authority). **Phase 0 (portability + harness
foundation) is COMPLETE + green**: `586.5.1..8` -- the PAL contract, freestanding `rt.c`
(JDN, dec_format ties->+inf) + `value.c`, host PAL binding, the LOCKED `spec/samir/` (imported
from the III+-only corpus: no `==`, `0x1C`/`0x1F` NORMALIZE, 151-code msg table), and BOTH
independent python readers `dbf_ref.py`/`ndx_ref.py` (the oracle independence barrier, 147/0 +
116/0 vs real goldens). Gate: `make test-samir` (7 sub-gates) green, wired into the aggregate.
**Standing dependency:** the sister corpus `../dbase3-decomp` (Tier-1 gates resolve
`DBASE3_DECOMP`; absent -> loud-skip; holds gitignored goldens + the dosbox-x mint harness).
**SAMIR PROGRESS -- Phase 1 codec COMPLETE + Phase 3 core (gmo) COMPLETE + Phase 4 OPENED
(WL-0030).** A second orchestrated session drove the engine forward in 5 parallel-lane waves
(disjoint files, orchestrator owns the Makefile + independent re-grading + commit-per-wave;
**104 -> 124 host gates**, all mutation-proven; 27 emu unchanged). **Phase 1 (.dbf codec,
`aul.1..5`) is COMPLETE**: `dbf_open`/`dbf_field`/`dbf_read_rec` (read) + `dbf_create`/`append`/
`flush` (deterministic write, round-trip vs `dbf_ref.py`) + `append_blank`/`replace`/`delete`/
`recall`/`pack`/`zap` (mutation, assignment-coercion). **Phase 3 core (the `gmo` epic,
`gmo.1..4`) is COMPLETE**: `xb_lex` (rejects IV `==`) -> `xb_parse` (corpus-minted `^` left-assoc,
unary>`^`) -> `xb_eval` (every `xbase_coercion.json` cell incl. the C+N=error#9 HAZARD) -> the
`dbf_coerce_fuzz` property-test (`os/samir/core/{lex,parse,eval}.c` + `eval.h`). **Phase 4 (.ndx,
`ahu.1`) is OPENED**: `ndx_open`/node-read parse vs the 11 corpus `.ndx`, verbatim key-expr
(`os/samir/fs/ndx.c`). See WL-0030 for the per-step ledger + boundary decisions.

**SAMIR / M6 (InitechBase) IS SUBSTANTIALLY COMPLETE -- `make test-dbase` GREEN (WL-0031 +
WL-0032).** Two further orchestrated sessions (21 waves total; 124 -> 174 host gates) drove the
engine to a dBASE-III+-1.1-compatible database that **really runs**: the full `.dbf`/`.dbt`/`.ndx`
storage layer (read/write/index parse+keys+SEEK+build+maintain), the xBase expression engine
(lex/parse/eval/coercion + five function families incl. full TRANSFORM), the complete interpreter
(USE/CLOSE/SELECT -> navigation -> control flow + memvars -> query/display -> mutation verbs -> SET
state -> procedures/scope/IO -> the dot-prompt REPL `samir_main.c`), writable USE (edit at the dot
prompt), and the M6 oracle: **`test-dbase` = `test-dbase-roundtrip` (S6.3 bidirectional round-trip,
mask-mutant = bead 586.3) + `test-dbase-diff` (S6.4 program differential vs authored III+ goldens,
100%)**. Plus the two Law-4 canon apps -- the Initech accounting **Y2K bug** (`586.1`: "00"->1900,
$0-overdue) and Michael Bolton's **salami-slicing rounding virus** (`586.2`: misplaced decimal,
BOLTON suspense skims dollars "too much too fast") -- both with the bug ENFORCED (a "fix" breaks the
gate). Two Law-2 catches this session: an `STR()`/SET-DECIMALS Law-1 grounding error (STR's default
decimals is 0, verified; SET DECIMALS's scope is division/VAL, not STR -- corrected) and a
program-diff harness liveness defect (summary to stderr -> mutant gate couldn't bite -- fixed).

**SAMIR RUNS INSIDE InitechOS -- M6 Phase 8 S8.1+S8.2 COMPLETE (WL-0033; ADR-0009 ratified).**
The operator opened the SAMIR<->Milton integration; an ADR-by-committee (period-authenticity
steer: dBASE did software FP, DOS ignored the 8087) ratified **ADR-0009**: SOFT-FLOAT engine
(no kernel FPU), flat `.COM` in conventional memory, kernel-x87 deferred. Landed + green +
pushed (`bf54c91`/`6f06411`/`8afa37d`/`6336efb`): `pal_milton.c` (the sole int 0x21 TU),
the DISJOINT AH=48h arena fix (`1q4u`), vendored `softfp.c` (`ap5g`), `samir_crt0`+`samir.ld`,
the `FLOW_MAX_REGISTRY=1` Milton profile (`qucm`), `spec/hardware.json` (`nh0m`), the in-place
FAT loader for >64 KiB apps (`za4m`), and the two emu gates: **`test-samir-boot`** (boot->USE->
LIST) + **`test-samir-write`** (REPLACE/APPEND persists to the .dbf, independent-reader-verified).
`make samir-com` -> `build/SAMIR.COM` (77792 bytes). Closed: ax9.1/ax9.2/hdlb/za4m/1q4u/qucm/
nh0m/ap5g/g6wx.

**Capstone DONE -- BOTH canon apps run inside InitechOS:** `586.1` Y2K accounting (`9a0f`,
`test-samir-canon-y2k`) + `586.2` Bolton's salami/rounding virus (`4hte`, `test-samir-canon-salami`),
both via the additive `DO <file>` REPL feature -- the Office Space deadpan bugs executing on the
emulated 386. **NEXT ungated deepening (filed):** SEEK/`.ndx` index + DELETE/PACK in-emulator;
`7az.13` transcendentals on the soft-float base.

**REMAINING SAMIR work -- GATED / deferred (no ungated host work left):**
- `7az.13` transcendentals SQRT/LOG/EXP -- now tractable on the soft-float base (poly approx;
  `softfp.c` is the home) + MINT for numeric edges.
- `7az` SET-DECIMALS-division (wire SET DECIMALS into its verified scope: division/VAL/computed
  display, NOT STR); `7az.17` (commands.h consolidation), `7az.18` (mutate.c #41->#111) -- cleanups.
- GATED TRANSFORM `@`-clauses + numeric/date MINT (MOD-sign, INT-on-neg, ROUND-tie, ITALIAN/FRENCH
  dates) -- need a **dosbox-x MINT** session vs real dBASE III+ 1.1.
- `586.4`/`586.4.1`/`17n.3` -- Tier-2 real-`DBASE.EXE` authenticity minting + re-mint the authentic
  `CNAMES.NDX` golden (faithful SAMIR rebuild after the WL-0031 test mishap; needs dosbox-x).
- ~~S8.1 `pal_milton` + S8.2 SAMIR-as-flat-`.COM` on Milton~~ **DONE (WL-0033).** Still GATED on
  **M4 (FLAIR)**: `ax9.3` (S8.3 SAMIR<->FLAIR text-console window) + `0tl`/`0tl.1` (S8.4 @SAY/GET/READ
  full-screen forms) -- these need the FLAIR window/event surface, which does not exist yet.

**Orchestration cadence (operator-set, proven across 21 waves):** delegate each step to a subagent
(sonnet default, opus for load-bearing; <=6 sonnet / <=2 opus parallel); the orchestrator owns
DISJOINT file ownership + Makefile integration + INDEPENDENT re-grading (Law 2: re-run the oracle +
mutant + clean aggregate + golden-integrity, never trust the report) + commit-per-wave + the bead
ledger; `make clean && make test-unit` at every integration; committee for serious decisions.

**FLAIR M3 is LIVE -- a draggable System-7 desktop, oracle-enforced (WL-0034 + WL-0035).**
The operator opened FLAIR as the crown jewel ("built on REAL measurements + RE/source-as-spec
+ goldens from real era software; 100% usable; obviously a System-7+Windows chimera" -- see
`bd memories` operator-directive-2026-06-19-flair-is-the). Driven as orchestrated parallel
waves; **ADR-0004 (FLAIR) + ADR-0005 (ATKINSON) RATIFIED by committee** (operator-delegated
authority, no gridlock; OQ-1 -> extended-memory heap DEC-03; OQ-2 -> defer 86Box DEC-04).
**Built + green + pushed (8 commits, FLAIR epic 22/33 = 66%):**
- GROUND TRUTH: `docs/research/gui-ground-truth.md` (System 7.0/7.1 target; Apple WDEF assembly
  source + Inside Macintosh as the authoritative native-pixel source); `spec/chrome_metrics.json`
  v1 (menu 20/title 19/scrollbar 16/frame 1/dialog 7 -- FIRST-HAND from StandardWDEF.a);
  `spec/chimera_element_map.json` (11-element mac-vs-win map).
- DRAWING: the ONE surface module (`os/flair/surface.c`, console is a client; `test-fbagree`),
  the extended-memory FLAIR heap (`spec/memory_map.h` FLAIR_HEAP_* + stage2 INT15h probe +
  `os/flair/heap.c` allocator), GrafPort/imaging + event/window + canon specs, the
  region-clipped blitter (`os/flair/blitter.c`), Chicago+Geneva text (`os/flair/text.c`),
  the seafoam desktop, the canon oracle (`test-canon`), and the chrome oracle
  (`os/flair/chrome.c` + `test-chrome` -- renders a System-7 window, diffs vs WDEF metrics).
- INTERACTIVE: the Window Manager (`os/flair/window.c`, z-order + DiffRgn damage, `test-window`),
  the Event Manager (`os/flair/event.c`, ISR-enqueue SPSC ring + WaitNextEvent, `test-event`),
  and the **M3 DRAG-GATE CAPSTONE** (`os/flair/desktop.c` compositor + `test-drag`): a window
  drags across a 3-window desktop with PIXEL-LEVEL no-over-repaint (D-5) proven against an
  independent owner-grid. Visually audited: `build/drag_{before,after}.ppm` = a real draggable
  Mac desktop.
- M4 MANAGERS + SHELL (WL-0036): the Menu Manager (`os/flair/menu.c`, canon Photoshop bar,
  `test-menu`), Control Manager (`os/flair/control.c`, buttons/16px-scrollbar/progress,
  `test-control`), Dialog Manager (`os/flair/dialog.c`, ModalDialog + the canon "Saving tables
  to disk..." FILE COPY box, `test-dialog`), and the **M4 CAPSTONE desktop shell**
  (`os/flair/shell.c`, `test-flair-shell`) which COMPOSES THE OFFICE SPACE FRAME: seafoam +
  two stacked menu bars (System-7 + canon Photoshop) + System-7 windows + the FILE COPY modal
  on top. Visually audited: `build/desktop_scene.ppm` = the reproduced frame. **`make test-unit`
  = 216 host gates** (was 184) + 35 QEMU + Bochs boot. KNOWN P1 fidelity polish (`bmih`): the
  window/dialog INTERIOR "white" is gray 0x7F7F86 (not crisp white), so the FILE COPY modal --
  though visible/legible via its 7px black border -- reads gray-on-gray-ish rather than the
  frame's white box; fix content-white across the palette lockstep.

**HOW TO SEE IT (the FLAIR renders).** The desktop is rendered HOST-side by the real artifact
code into `build/*.ppm` (640x480, deterministic). Regenerate + view:
`make test-flair-shell` -> `build/desktop_scene.ppm` (the composed frame); `make test-drag` ->
`build/drag_before.ppm`+`drag_after.ppm`; `make test-chrome` -> `build/chrome_window.ppm`.
Convert/montage with ImageMagick (`convert build/desktop_scene.ppm out.png`; a `flair_gallery.png`
montage helper was used this session). NOTE: this is NOT yet what `make run` boots -- the kernel
still drops to `A:\>` COMMAND.COM; the FLAIR desktop is host-rendered until the in-OS arc lands.

**NEXT FLAIR ARC (resume here).** The four core M4 Managers (Window/Menu/Control/Dialog) + the
desktop shell are DONE (this session, WL-0036). Remaining, in priority order:
1. **`bmih` (P1) content-white + palette single-source** -- make CIDX_WHITE/INITECH_WINDOW_WHITE_RGB
   actual white across the lockstep (chrome.c CHROME_PAL + render.c render_palette_rgb +
   palette.json `canonical` -> palette.h), re-verify test-chrome/test-flair-shell + re-audit the
   PPM. Now visible in the capstone. Ideally calibrate the exact white from a real System-7
   screendump (`pvo4`).
2. **THE IN-OS ARC** -- everything above is HOST-RENDERED (the oracles). Wire the FLAIR stack INTO
   the kernel + boot to a live desktop in the emulator: `initech-26d` (PS/2 mouse IRQ12, dual-PIC
   EOI, Bochs-verified) + the hourglass cursor + a desktop `kmain` path + an emu screendump gate.
   This makes the host-tested logic the bootable product (operator's "100% usable"); after it,
   `make run` shows this desktop in QEMU.
3. **GROUND-TRUTH / GOLDENS in a PARALLEL REPO (epic `initech-rf2l`; WL-0037).** Provenance audit
   found two Law-1 gaps: (1) `chrome_metrics.json` cites Apple's WDEF source by URL + the values
   match the documented System-7 constants, but `refs/` is NOT cached locally (WebFetch persists
   nothing), so "FETCHED AND VERIFIED" overstates what is auditable -- cache the WDEF/IM sources +
   correct the wording; (2) NO real-era-software screenshot goldens exist yet (all pixel-perfect
   values are `golden_resolves`). DECISION: mirror the SAMIR `../dbase3-decomp` pattern -- a
   SISTER GUI-ground-truth repo holds the refs/ROM/disk-images/mint-harness/real-screenshot
   goldens (copyright-grey + large, gitignored), and the main repo's FLAIR gates resolve a
   `FLAIR_GOLDENS` env var via a `need_goldens`-style helper + loud-skip (NOTHING heavy/copyrighted
   in the main repo). Pieces: `pvo4` Mac 68K ROM (Basilisk II System-7 goldens -> pinstripe RGBs,
   content-white; OPERATOR DECISION), `77wz` Win 3.1 via dosbox-x (NO ROM; `apt install xvfb
   xdotool`), `q0gy` 86Box (blocks M4 sign-off), `u9gf` Bochs RFB capture, `k8o5.11` fb-agree.
4. Other: autoKey. M5 apps (InitechCalc w/ the 116% pie) consume these Managers next.

**Other ready work** (`bd ready`; distinct directions — pick per operator
steer):
- `26d` — **PS/2 mouse (IRQ12) + the canonical hourglass cursor** (Law 4 canon)
  → toward the interactive desktop / Toolbox (M3/M4 GUI).
- `kg5` — **Chicago + Geneva 9 bitmap strikes + text rendering** (frame fidelity).
- `44m`/`x0i` — **86Box leg** of the tri-emulator gate (lowest priority; its
  Qt-offscreen headless automation is unbuilt — a deep environment task).
- `h58` — cleanup: retire the now-redundant `SHELL_IMG`; add a shell-prompt
  screendump gate.
- `75r` — specs-as-data scaffold (foundational).

The controlled spec-data in `spec/` is the contract; the harness
(`build/qemu_harness`, `build/bochs_harness`) is the oracle.

## 6. Where things live

```
CLAUDE.md            how to work (Laws/Rules)
InitechOS-PRD.md     what to build
docs/adr/            ADR-0003 (DOS, authoritative), CDR-0001 (toolchain deviation)
docs/worklog/        WL-0001..0007 (foundations -> FAT mount); WL-0008+ file
                     handles/SFT, WL-0009 SYSINIT, WL-0010 multi-tenant IO,
                     WL-0011 reentrancy fuzzer, WL-0012 message catalogue,
                     WL-0013 vector cluster, WL-0014 kernel hardening + Bochs
                     diagnosis, WL-0015 Bochs standard-VGA fallback + C Bochs
                     gate, WL-0016 COMMAND.COM default boot, WL-0017..0024
                     hardening + subdir chain, WL-0025 INT 21h/25h/26h parity
                     tranche (CREATNEW/FILETIME/CHMOD/RENAME/IOCTL + DEC-15),
                     WL-0026 the WL-0025 follow-up tranche (CREATNEW oracle/
                     robustness, IOCTL minors, absdisk spec/emu, fault-injection
                     infra, oracle hardening), WL-0027 the FAT16 milestone (dao
                     streaming walk + z01 FAT16 decode layer) + DEC-16/DEC-07a
                     ADR ratifications, WL-0028 the forward tranche (80k wildcard,
                     d27i windowed FAT16 mount, x8fs/er3h/4tw CON cluster) +
                     InitechBase (SAMIR) M6 ground-truth research brief, WL-0029
                     SAMIR/M6 architecture (ADR-0008) + Phase-0 foundation
                     (PAL, rt, value, spec lock, dbf_ref/ndx_ref) orchestrated (latest)
docs/research/       ground-truth briefs (fat12, fat16, boot-to-text, internals/
                     int21h, psp-loader, fs-mount-sft, dbase/SAMIR) -- the
                     per-milestone evidence base
docs/HANDOFF.md      this briefing
harness/emu/         qemu.{c,h}+qemu_main.c (QEMU), bochs.{c,h}+bochs_main.c
                     (Bochs, initech-564), rfb_unblock.py (diagnosis only)
spec/                LOCKED spec-as-data: int21h_register.json, dos_structs.h,
                     dos_messages.json, dos_{banner,config_sys,autoexec_bat}.txt,
                     chrome_metrics.json, assets/ (palette/glyph work, deferred)
seed/                C seed Pascal->x86 compiler (= Turbo Initech genesis)
harness/             C factory: emu/ (QEMU oracle harness), factory_smoke.c
os/boot/             C+asm boot chain (MBR -> protected -> LFB -> C-kernel handoff)
os/milton/           THE KERNEL (C+asm): kstart/kernel.ld/kmain, console, idt/isr/
                     pic/panic, int21, psp, loader, fat12, ata, blockdev, boot_info,
                     test_*.c host oracles, test_program.asm (baked .COM)
os/{flair,samir,tps,apps}  the rest of the OS (C; tps/ will hold Turbo Initech)
Makefile             factory + gates; CC interim = host gcc -m32 -ffreestanding
build/               artifacts (gitignored)
```

Beads conventions: issues are `initech-*`; epics carry `m0`..`m8`/`m0.5`/`stretch` + `adr-0003` labels; M2 children are `509.x`. Vestigial-but-required structures carry the `vestigial` label and are implemented **in full** (design stance).

## 7. Gotchas (learned the hard way)

- **Oracle is truth, not the agent's report.** Re-run the gate yourself; verify subagent claims. Mutation-prove goldens (perturb → must go red → restore).
- **Stub honesty.** Gate/oracle Makefile targets exit non-zero when unimplemented; only action targets (image/run) exit 0 when stubbed.
- **Banner/message bytes are controlled vocabulary** (ADR-0003 DEC-13/App D): exact spacing (`InitechDOS  Version 3.30` — double space) is load-bearing and enforced by `test-spec`.
- **Triple-fault detection** keys on QEMU `-d` log strings, NOT `cpu_reset` count (SeaBIOS resets ~2×/boot).
- **Screendump needs a live guest** (race if the guest clean-exits fast; bead `initech-xcg`). The kernel guest hlt-loops, so it's fine.
- **Look at the screendump, don't just trust a green gate (Law 4).** A green `test-fs` once hid a directory listing that never rendered: a **dangling console pointer** (console declared in a nested block, used after scope by the proto-DIR). The screendump check false-passed on the banner alone. Fixed (hoist to function scope) + the oracle was strengthened (`ppm_text_check` now takes an optional `[y0 y1 min_fg]` band; `test-fs` asserts the DIR band). Lesson: in `kernel_main`, anything whose address escapes into a global (`g_int21_con`, `g_dir_con`, the panic console) must outlive every later use — keep it at function scope.
- **The DEC-04a vector map is load-bearing:** `int 0x21` is a TRAP gate at vector 0x21; the 8259 PIC is remapped to **master 0x28 / slave 0x30** (NOT the conventional 0x20/0x28) precisely so 0x21 stays free for the DOS syscall (else IRQ1/keyboard would collide). `int 0x20` (legacy terminate) lives at the now-free vector 0x20. See `docs/adr/ADR-0003-AMENDMENT-DEC-04a-*.md`.
- **`ata.c` first-run guards:** floating-bus (0xFF) = no-drive must return an error, never spin; BSY/DRQ polls are bounded (timeout). A missing `--disk2` makes mount fail-loud-and-continue (boots without a data disk still pass).
- **A review committee earns its keep:** the DEC-04a ARB review caught a real `do_getver` BH-mask bug the unit oracle had missed (then the oracle was made to bite it). Independent perspectives + mutation-proving > a single green pass.
- The reference frame still (`spec/assets/preview.webp`) is a **local-only reference fixture** (gitignored); derive palette/metrics from it, never embed it in committed source.
- Open follow-up beads worth knowing: `509.3`/`509.5` (next work, §5), `saw` (FAT-sourced load), `n62`/`3rs` (keyboard/CON input), `we2`/`xk2` (DEC-04a forward obligations: ring-3 DPL, INT 21h reentrancy), `dao` (fat12 on-stack chain buffer), `x0i` (tri-emulator), `6pm` (i686-elf), `79s` (ADR-0007), `xcg` (screendump race), `ta2` (M1 boot robustness).

---

*— End of Briefing —*

<!-- Tedium certified compliant with NFR-7. If you have received this briefing in error, please shred it and notify the Help Desk (ext. 2504). -->
