<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->

# HANDOFF — Programme Continuity Briefing (InitechOS / STAPLER)

**Issuing Body:** Initech Systems Corporation — Platform Engineering
**Document Class:** Continuity Briefing (living document; supersede in place)
**Last Reconciled:** 2026-08-25, session close (**WL-0088: R3.2 THE DESKTOP MANAGER -- ICONS ON THE DESKTOP, PROVEN IN MOTION.** One orchestrated session per the operator directive (opus lanes + sonnet recon, "validate all gui with actual tested videos"), commits `cb9f9e2..122c835` (5, ALL PUSHED), **initech-tdnl.9 + tdnl.27 CLOSED**; closing certificate **ALL GREEN 347 host + 102 emu** (from 341+99; test-flair-desktop-icons + -mutant + -bochs join TEST_EMU_GATES). LANDED: **the finder_cmd SPINE** (one FINDER_COMMANDS[] table over Apple+512..515, one finder_dispatch, four predicates, caller-supplied trace sink -- os/flair stays serial-free; hand-authored 28-line trace golden, REAL packing differential vs menu.h's MenuResult, mutants TABLE_BYPASS/SILENT_UNKNOWN/PRED_STUCK_ENABLED); **the LOCKED 32x32 strikes** (spec/assets/desk_icons.h: diskette VOLUME + ridged-basket TRASH as three 32-bit bitplanes/icon, one word per ASCII-map row = auditable by eye; NEW flair_look parts ICON_INK/_FACE/_SHADE onto EXISTING canon rows, zero new colors; finder_icon clipped mask-honoring blit + hit; mutants MASK_IGNORED/ROW_OFF1); **the DesktopMgr core** (os/flair/finder_desktop.{h,c}: PURE trackers -- sprite-UNION-label hit, click/shift/marquee w/ the load-bearing +1 half-open edge, dblclick synthesis FINDER_DBLCLICK_TICKS=20 provisional, clamped drag commit; the LOCKED DESKTOP.DB 24-byte record codec; **WindowMgr.desktop_underlay** -- the first callback hook on WindowMgr, invoked at EXACTLY the two desktop.c fill sites w/ that site's clip, icons under windows BY CONSTRUCTION, NULL hook = every non-Finder build byte-identical; the Finder launches FIRST as a windowless App Contract tenant, Finder->NOTES->HELLO so HELLO stays boot-foreground and EVERY locked band-2 gate is unchanged -- the Finder bar arrives at tdnl.12; the dead inDesk pump arm now routes desktop gestures; DB codec/IO split os/flair-bytes vs os/milton-FAT, kmain joins). **THE BOOTED ORACLE + THE VIDEOS**: test-flair-desktop-icons 7 legs (default render probes hand-derived from the ASCII maps, selected label INVERSION, deselect restore, marquee n=2, dblclick FINDER-OPEN-VOLUME NYI, drag to (400,400) + DESKTOP-DB-SAVE, REBOOT leg -- DESKTOP-DB-OK + position survives + mtools 56-byte hidden DB); mutant images desk_no_underlay/desk_dbltick_off RED via a new finder_desktop.o-swap template (same macro spellings as the host knobs); locked traces spec/flair_desktop_icons_traces.mk ALL parked at (620,460); record-flair clips icon_select/rubber_band/icon_dragdrop **byte-identical repro PROVEN x3**, orchestrator frame-eyeballed (inverted band, trail-free 1-px marquee, diskette re-seated), sent to operator. Default icon rects (VOLUME sprite (584,48), TRASH (584,404)) chosen against the FULL union of existing probe rects/waypoints -- all 11 sibling emu gates re-proven green. FINDS: the design doc is WRONG about Geneva (FONT_GENEVA9 + geneva9.h fully exist; labels are Geneva, F1-4's Chicago substitution NOT needed -- recorded in finder_desktop.h); tdnl.8's DB skeleton is os/milton/desktop_db.{h,c} NOT the design's finder_db.c, and had no record I/O; measured dblclick on the booted image = 8 PIT ticks vs the 20 budget (constant stays, golden-resolves); qmp_inject_mouse has NO modifier tokens -> shift-extend is emu-unreachable, host-covered (bead `05rz`); an opus asset lane emitting long hex was KILLED by the API content filter -- workaround protocolized (bd memory opus-lane-content-filter-trap-2026-08-25: hand-author the ASCII map, shell-transcribe the rows). **KERNEL WINDOW PRESSURE: ~4KiB left** in the 352-sector tenants window even after KERNEL_TENANTS_OPT=-O1 (-Os trips a gcc bounds false-positive under -Werror) -- **bead `uzjc` (P2): land the KERNEL_SECTORS bump FIRST or alongside tdnl.10, as its OWN tri-emulator-gated change**. WORKING MODE (codex replaced this session per operator): sonnet recon pair -> opus lanes A/B in worktrees (spine + strikes, parallel) -> opus lanes C/D serialized on main (core + emu wiring), orchestrator graded every diff + re-ran every oracle first-person between lanes; baseline AND mid-point AND closing certificates run first-person. NEXT AGENT: **initech-tdnl.10** (R3.3 disk windows) per the F5-1 order 8->9(+27)->10->12->26->11->13->14, with `uzjc` first/alongside; operator rulings still pending: desktop PATTERN canon (desktop stays flat teal), rainbow-vs-mono Apple + Charcoal-vs-Chicago (D3-3), `81ft.4` alert pinks, ADR ratification of the four design drafts; the operator Law-4 eyeball of the icon art + clips is the standing ask (ssim.c still NOT BUILT). See WL-0088.) PRIOR -- **WL-0087: THE GUI REMEDIATION OPENS -- R0+R1+R2 LAND, THE KERNEL MOVES HIGH.** Operator directive 2026-08-23: the GUI was "a buggy pair of windows with menus that don't work" -- remediate to sellable-in-1996, pixel-perfect System 8, motion-gated, TPS-Pascal apps as bonus. One orchestrated overnight session, commits `b8f06d8..7c40b35` (16, ALL PUSHED), closing certificate class **ALL GREEN 341 host + 99 emu** (from 336+90). THE PROGRAMME: epic **`initech-tdnl`** (30+ children; plan docs/plans/GUI-remediation-plan.md R0-R6; sellability bar docs/research/period-gui-suite.md; FOUR ox-alpha 1M-seat design docs under docs/design/ -- D1-D2-D3 incl. the INT-0x81 Toolbox Gate + zero-subset-growth Pascal binding + 63-row pixel audit, the R3 Finder shell (the Finder = an always-resident compiled-in App Contract tenant, 3-seam icon underlay, DESKTOP.DB + TRASH staging), the ADR reconciliation (TWO ratified-text contradictions caught pre-code: park/resume WNE forbidden by ADR-0013 Sec 3.4 -> push-callback; no shipping watchdog), and the kernel runway relocation). LANDED (each codex-gpt-5.6-sol-xhigh laned, orchestrator-graded first-person, emu+clips run first-person, full vector green per slice): **R0 complete** -- THE POINTER EXISTS (CursorMgr on the final LFB, PARK CONVENTION born), WINDOWS HAVE NAMES (Platinum titles + SetWTitle = the D2-3 trap seam, real win-ids), the chrome tail x7 (shadow/notch/zoom glyph/bevels/scrollbar column); **R1 complete** -- unified hit zones + REAL zoom/grow/collapse + period outline drag; close TERMINATES the tenant (anti-8fhu per ratified 3.4), TRUE MODALITY (+ the movable modal drags itself; BOTH FO-9 gates had zn61 baked into their locked traces -- re-keyed to the modal-drag proof + reverse survival; menu gates relocated to a new no-modal image), BeginUpdate-fidelity visRgn clip (wlzp); **R2 substantially complete** -- Platinum MENUS (teal pulled-title hilite, panel bevels, cmd column, 9 mutants), Platinum SCROLLBARS (5-value wells, arrow tiles, 15px teal thumb, disabled state), Platinum CONTROLS (checkbox tiles + X, button bevels, THE DEFAULT RING; alert pinks Rule-8-STOPPED -> `81ft.4` sampled-canon accretion pending); **R3 opened** -- THE DESKTOP HAS A DISK (flair_data.img slave on every tenants boot, DESKTOP.DB + TRASH persistent w/ mtools differential, fat12 -15 collision fixed) and **THE KERNEL MOVES HIGH** (0x500000 via ONE post-PE rep movsd; program window BYTE-IDENTICAL w/ mutation-proven floor guards; KHI/KAT green QEMU+Bochs; ~0.9MiB runway; closes 5dr8/yrzo -- the DOS kernels were also delinked from the dead FLAIR objects they never referenced, ~60KiB back). TWO DEEP FINDS FIXED AT ROOT: (1) the freestanding artifact compiled at HOST default arch -- cmov (P6+) everywhere, silently violating ADR-0001's 386+ since forever; caught by the Bochs cpu=pentium leg PANIC vec=06 on the new R3.1 path; **-march=i386 now pinned** (bd memory march-i386-pin-2026-08 incl. the stale-objects-on-CFLAGS-change trap); (2) test_keep is a rare ASLR-draw flake (`ahp2` w/ forensics -- re-run standalone before blaming a diff). WORKING MODE PROVEN: serialized codex lanes (one main-tree writer; `codex exec resume --last` for orchestrator-directed rework, used twice) + ox-alpha 1M seats TOOL-LESS w/ inlined corpora (@file; tool mode returns empty; ~50% of long runs time out -- retry once then fall back to codex) + full first-person vector after EVERY lane (it caught both cross-cutting misses). **NEXT AGENT -- START AT `initech-tdnl.9`** (R3.2 DesktopMgr + command-table spine; the FULL lane brief is preserved verbatim in the bead's notes; a lane was dispatched and stopped pre-edit at session close, tree clean). Then the Finder order: 10 disk windows -> 12 the real Finder bar (THE deliberate band-2-at-rest re-key + operator Law-4 clip) -> 26 -> 11 -> 13 -> 14 (implement from the AC-3/DEC-09/10 blocks in docs/design/GUI-remediation-ADR-reconciliation.md Part B, NOT the older D1.4 prose) -> R4 apps -> R5 TPS Pascal. OPERATOR RULINGS PENDING (flagged, non-blocking): rainbow-vs-mono Apple + Charcoal-vs-Chicago (D3-3), the Finder-bar-at-rest clip sign-off, `81ft.4` alert-pink canon rows, ADR ratification of the four committed design drafts. Also open: `xu97` (high kernel stack), `tdnl.30` (TRASH hidden attr), zqy3 (640x480x8, now sequenced under R2.4), tdnl.7 (whole-desktop golden), and the pre-existing M7 blocker `6gkm` (untouched this session). See WL-0087.) PRIOR -- **WL-0086: test-compiler GOES REAL -- B9.5 CERTIFIED, M7 PENDING EXACTLY ONE BUG.** Commit `3e5ebd9`; closing certificate **ALL GREEN 336 host + 90 emu**; the working branch moved to `main` at session close (fast-forwarded from command-com-default; use main from now on). B9.5 landed: `make test-compiler` is the REAL shared-corpus fpc differential -- CORPUS.md registers 17 hand-golden fixtures (B1-B8 + the 3 gap families), host pass rate 100%, IN the unit vector; TPS_GEN_MUT_VARPARAM at a named seam. **THE FIND: the 17-program one-boot AUTOEXEC corpus rail exposed a LATENT MILTON batch/EXEC bug** -- programs silently skip or the batch stalls, position- AND content-dependent; the forensic exclusion ladder (EXEC count / image / program content / predecessor pairs all exonerated; every fixture correct at the prompt, bare-metal, and in short batches) lives on **P1 `initech-6gkm`, which BLOCKS `6m52`**. test-compiler-os + -os-mutant are WIRED but OUT of TEST_EMU_GATES behind the OMISSION note at the vector head; **they rejoin and M7 CLOSES when 6gkm lands**. Ceilings re-sized honestly (corpus 420s; the three tps-os gates 120s -- TPS runs all four passes every boot since B9.4). **NEXT AGENT -- START AT `6gkm`** (the batch bug; its acceptance criteria include a host test_batch regression + mutant, then restore both corpus legs + close 6m52 = M7); after that M8 (epic ls4: resident port, K2==K3, DDC -- watch items: 7,556B self-assembly arena headroom, four-pass re-read cost). ALSO OPEN: `sjvq` (operator Law-4 call on Platinum menu chrome) + the operator's standing clip eyeball (build/clips/), `81ft`, `zwo8` remainder, `x0i` 86Box, `e7qc` dBASE-Mac frontend (ruled, staged). THE SITTING (WL-0080..0086): 13 beads closed, TPS built from nothing to a compiler that compiles itself, 9 clean certificates, codex windows serialized w/ the rk session 11/11 (bd memory codex-exec-cross-session-kill-trap-2026-08). See WL-0086 incl. the certification-outcome section. PRIOR -- **WL-0085: TPS EMITS x86 AND COMPILES ITSELF -- B9.4.** Same sitting, commit `38b542e`. Fourth pass: deterministic nasm .s for the FULL accepted subset -> TPSOUT.S via B8 BlockWrite (behavioral contract w/ the seed runtime, not textual -- Law 2); CODEGEN.md emission contract + hand-checked excerpt. **THE K1 PRECURSOR EXISTS: TPS compiles tps.pas ITSELF -- 764,500B generated assembly, twice-byte-identical, links both targets, DOS end 0x6d224 = 7,644B under the hard ceiling (THE M8 WATCH ITEM).** THE FULL LOOP ON METAL (orchestrator-run test-tps-gen-os in TEST_EMU_GATES): TPS.COM compiles TINY ON InitechDOS -> extracted/assembled/linked -> the TPS-COMPILED .COM boots + runs correctly on the SAME OS; three-way agreement TPS==seed==fpc. Rule 6 split-axis: LABELS (nasm RED) + OFFBYONE (WRONG VALUE by execution on the booted 386, test-tps-gen-os-mutant). **Certificate: ALL GREEN 338 host + 92 emu.** NEXT: **B9.5 -- test-compiler goes REAL (100% corpus pass rate, PRD 6.7/8) = THE M7 CLOSE** (brief staged; corpus covers the behaviorally-unexercised families + respects the forward divergence). See WL-0085. PRIOR -- **WL-0084: TPS TYPECHECKS ITSELF -- B9.3.** Same sitting, commit `8392e4f`. AST-free check-as-you-parse typechecker + DEC-04 insertion-ordered symbol tables on the char pool (rules mirrored from seed/typecheck.c w/ citations in SYMBOLS-DUMP.md = the oracle interface + B9.4 codegen paper trail); third bracket TPS-TYPE symbol dump; SELF-TYPECHECK TOOTH green (tps.pas typechecks ITSELF); triple-bracket on-OS differential byte-identical; DUPOK/ASSIGN mutants RED. TWO STRUCTURAL FACTS: (1) whole-source arena -> two-ShortString ROLLING WINDOW (TPSIN.PAS re-read per pass; BSS 52,136B, headroom 82,280B, structural assertion pins it); (2) PINNED DIVERGENCE: TPS requires explicit `forward` for call-before-definition (seed precollects) -- STRICTER = self-host-safe; the B9.5 corpus + tps.pas itself must respect it. **Certificate: ALL GREEN 335 host + 90 emu.** NEXT: B9.4 CODEGEN (TPS emits x86 .s via the B8 write path; the differential deepens to EXECUTION: TPS-compiled == seed-compiled == fpc-compiled on the same fixture), then B9.5 real test-compiler = M7 CLOSE. See WL-0084. PRIOR -- **WL-0083: TPS PARSES ITSELF -- B9.2 THE PARSER.** Same sitting, commit `3a928d2`. tps.pas ~1950 lines: single-lookahead recursive descent over the B9.1 lexer (no token buffer/AST/symbols -- B9.3 scope), grammar mirrored rule-by-rule from seed/parser.c (the M7 grammar REFERENCE) w/ citations; the seed's lexes-but-does-not-parse `case` divergence PINNED w/ a negative golden; dual-bracket driver (TPS-LEX dump then TPS-PARSE trace; zero lexer re-keys). ORACLES: hand-computed 651-line trace golden + 5 located-error goldens + **THE SELF-PARSE TOOTH (tps.pas parses ITSELF, twice-deterministic -- the self-host pre-echo)** + test-tps-parse-os in TEST_EMU_GATES (seed-compiled TPS.COM on the booted 386 byte-identical to fpc-built -- the two-level differential); mutants PRECEDENCE + ELSE (valid-input wrong-trace, double-guarded sed) RED-for-named-reason. BSS 103,748B under a raised 128KiB soft budget (hard 0x6F000 ceiling untouched, ~60KiB headroom -- B9.3 symbol tables must budget here). `gamn` CLOSED not-reproducible (sandbox stale state; green standalone + both bracketing certs -- Rule 4 record on the bead). **Certificate: make clean && make test ALL GREEN 332 host + 89 emu.** NEXT AGENT: B9.3 typecheck + DEC-04 insertion-ordered symbol tables on the char-pool (the lh83 idiom is already load-bearing), then B9.4 codegen, B9.5 real test-compiler (M7 close). Codex windows serialized w/ rk (7/7 record; bd memory codex-exec-cross-session-kill-trap-2026-08). See WL-0083. PRIOR -- **WL-0082: TURBO INITECH IS BORN -- B9.1 THE LEXER.** Same sitting, commit `b71961f`. `lh83` RATIFIED solo (char-pool index-arena; array-of-string stays OUT) and B9 SLICED on `6m52` (B9.1 lexer -> B9.2 parser -> B9.3 typecheck/symbols -> B9.4 codegen -> B9.5 real test-compiler = M7 close). os/tps/tps.pas EXISTS: 934 lines of subset Pascal (single-file bootstrap), full-surface lexer + token-dump driver, char-pool identifiers, TPSIN.PAS via the B8 RTL, source bytes packed 3-per-integer (DOS BSS 62,780B inside the 64KiB reserve -- WATCH ITEM: B9.2+ buffers need a memory-map decision or continued packing). ORACLES: hand-computed independent golden + 4 error goldens + 4261-line SELF-LEX (host), seed-compile + both runtime links, and test-tps-lex-os IN TEST_EMU_GATES = the TWO-LEVEL DIFFERENTIAL (seed-compiled TPS.COM on the booted InitechDOS byte-identical to fpc-compiled stdout -- catches seed miscompiles AND TPS bugs in one gate; GREEN FIRST RUN). Rule 6: CASEFOLD/STRESC source mutants via double-guarded sed, RED-for-right-reason. **Certificate: make clean && make test ALL GREEN 329 host + 88 emu.** NEXT AGENT: B9.2 (the TPS parser -- same two-level differential shape, dump-the-parse-tree or accept/reject + located-error oracle; resolve the BSS headroom question FIRST), or `sjvq`/`81ft` (sjvq = operator Law-4 call pending), or `zwo8` remaining halves. Codex-window serialization w/ the rk session = standing practice (bd memory codex-exec-cross-session-kill-trap-2026-08). See WL-0082. PRIOR -- **WL-0081: B8 LANDS -- THE SEED SPEAKS FILE I/O, ON METAL.** Same sitting as WL-0080, commit `f7f1dc7`, bead `ogxv` CLOSED. Codex lane: thin untyped-file surface (Assign/Reset/Rewrite/BlockRead/BlockWrite, size-1 bytes only, storage-only `file`, loud rejections), 260-byte static file objects, hand-assembled fileio.asm RTL over INT-21h (DEC-04a flat ABI, CF -> FILEIO-ERROR + AH=4Ch AL=1), fileless links byte-identical, start.asm FROZEN untouched; fixture fileio.pas (TARGET-before-SINK for mutant observability) + file_shared.pas joins SEED_FPC_CORPUS (fpc round-trip byte-exact). THE ORCHESTRATOR FINDING: the seed's emu legs are bare-metal Multiboot -- NO INT-21h -- so B8's oracle needed a SECOND SEED RUNTIME TARGET (first-person): seed/rt/start_dos.asm + seed_dos.ld = flat .COM @0x40100 per the samir_crt0 precedent (NOLOAD-bss zeroed, AH=4Ch exit, serial helpers duplicated verbatim w/ grep marker SEED-RT-SERIAL-HELPERS), SAME compiled program objects. NEW EMU GATES in TEST_EMU_GATES: test-seed-fileio-os (COMMAND.COM EXECs FILEIO.COM off a FAT12 data disk; exact serial FILEIO W1=1 WB=8 R1=1 RB=8 DATA=NORTHSTAR -- GREEN FIRST BOOT) + -mutant (SHORT_WRITE RB=6 DATA=ORTHSTA?? / WRONG_HANDLE R1=0 RB=0 -- wrong-values-no-crash ON METAL). TRAP (Rule 4, own gate): a bare 'FILEIO' grep matched the DIR listing's FILEIO.COM line -- vacuous proof; tightened to ^FILEIO W1= (in-gate comment warns). **Certificate: make clean && make test ALL GREEN 326 host + 87 emu.** Codex windows serialized w/ the rk session throughout (final joint tally 4/4 clean in-window vs 3 kills outside; bd memory codex-exec-cross-session-kill-trap-2026-08). **B9 (`6m52`) IS NOW UNBLOCKED -- author Turbo Initech in os/tps/ (resolve `lh83` first); closing B9 closes M7. start_dos is B9's runtime from day one (the resident compiler runs ON InitechDOS).** NEXT AGENT: B9 `6m52` (+`lh83`), or `sjvq`/`81ft` Platinum chrome (sjvq = operator Law-4 call pending), or `zwo8` remaining halves. See WL-0081. PRIOR -- **WL-0080: THE WAVE B MENU ARC -- BAND 2 LIVES, THE CANCEL RESTORES, THE WART DIES.** One orchestrated session (codex exec gpt-5.6-sol xhigh lanes, orchestrator graded EVERY diff first-person + ran ALL emu/video proofs; commits `1483770`/`45a8860`/`81de93d`). Slice 1 `7tjp`: NOTES owns a distinct no-Apple File/Edit/Notes bar (flair_tenants_demo.h IDs 384-386; sys=128+/photoshop=256+ so serial menuIDs are ROUTING PROOFS); POST-DISTINCT appswitch leg + NOTES_BAR_SYS 5th mutant (the mutant IS the old behavior = RED-first proof). Slice 2 `t1rv` (DQ8): band-2 hit test dispatches ten_plist.head->menubar; flair_live_do_menu_at(y_top) renders through make_offset_view (menu geometry y=0-local, coords translated; FO-7/8 pump untouched via y_top=0 wrapper); LOCKED FLAIR_SOLID_MENU2_SPEC + solid leg E + menu=256 serial tooth; mutants MENU2_DEAD (pixel axis) + MENU2_BAR_SYS (routing axis, pixels REQUIRED green). Slice 3 `b3hl`+`j0vt`: the restore contract -- track-end restore ALWAYS + cross-title erase through the DQ2 spine via NEW WindowMgr_invalidate_desktop (distribute_exposure w/ no departing window); ZERO shell_render in the menu path (the tenant-content wipe closed at root); band-2 overlay slice redrawn by DrawMenuBar under band-1 panels; FLAIR-MENU-XDROP marker; MenuInfo_panel_rect locked to the drawer by a 4-edge painted-extent test_menu tooth; solid leg D = whole-frame PRE==POST byte-equality TOL=0 after a cross-title cancel (bites BOTH panel persistence and the shellrender wipe; mutants MENU_NO_RESTORE + MENU_RESTORE_SHELLRENDER RED on exact axes -- 8 solidity mutants total). ORACLES DE-WARTED (Law 2): harness qemu.c captures HELD-STATE markers (FLAIR-MENU-DROP/XDROP ONLY -- CaptureCtx.midtrack) between injected events; missing marker => NO dump (strictly louder). TWO traps the stricter rule caught, both fixed honestly: appswitch ignore_refcon mutant kills its own marker (loop now splits mutant-kill vs harness-fault) and **test-samir-boot went RED in the first closing cert** (SHELL-READY precedes injection; relied on the after-all-keys dump -- hence the midtrack narrowing, samir/menu/solid re-proven after). record-flair-repro flaked ONCE (solid_switch, re-run passed) -- live evidence on `zwo8` + the FAIL path overwrites its evidence (gap noted). **Certificate: make clean && make test ALL GREEN 324 host + 85 emu.** VIDEO: solid_menu2 + solid_menucancel join RECORD_SCRIPTS; clips eyeballed (band-2 panel anatomy zoom-verified; cancel clip frame7==frame0 byte-identical; Photoshop New/Open items are the standing kmain.c:971 placeholder fixture, not a bug). **OPERATOR RULING (direct): the SAMIR graphical frontend is dBASE-MAC-SHAPED** -- FLAIR windows, ugliness-as-content via the ratified chimera surface (GDI-facade BTNFACE + win31 accents); dBASE-for-Windows chrome REJECTED (bead `e7qc`, staged behind `586` text gaps + `81ft`). POSTSCRIPT same sitting: **`chd4` LANDED (commit `d1ac218`) -- Route 1 (DEC-10 OQ-7): the era axis is DATA at the policy seam**, flair_look PART->skin-slot offset map (pure data, no switch(era)) + flair_look_pixel_for_skin, chrome.c threads const flair_skin_t* beside the GrafPort, flair_skin_resolve gains its FIRST os/ callers (the Sec 5.3 Law-2 smell closed); LOOK-NEUTRAL proven (host render byte-identical sha 4b0ebf6c, zero re-keys, exact-profile emu gates, clip eyeball); FLAIR_MUT_SKIN_WRONG_ERA = 15th fidelity mutant RED on exactly the 5 slot-fed legs; flair_skins.h struct-tag-only touch (digest unchanged); own certificate ALL GREEN 324+85. ALSO: cross-session codex kill-trap CONFIRMED + protocolized (bd memory codex-exec-cross-session-kill-trap-2026-08: a fresh codex exec SIGKILLs OTHER sessions' running codex children ~20-30s after attach; SERIALIZE codex windows via SendMessage w/ concurrent sessions; adopt rk's host-wide lockfile when it ships). NEXT AGENT: `sjvq` (Platinum menu chrome -- operator Law-4 call pending on re-skinning the chimera bar), `81ft` (Platinum control chrome), `zwo8` (icount hardening, now evidenced), or the North-Star B8 seed lane (`ogxv`). WORKING MODE re-proven: codex exec gpt-5.6-sol xhigh for code+host-oracle lanes (sandbox blocks emulators), orchestrator runs ALL emu legs + grades every diff + eyeballs every clip; serial slices, ONE main-tree writer. See WL-0080. PRIOR -- **WL-0079: THE DESKTOP IS PLATINUM -- 3knt+i3si LAND TOGETHER, DEC-10 EXECUTED END-TO-END.** One orchestrated overnight session (codex exec gpt-5.6-sol xhigh lanes for every code+host-oracle slice, orchestrator graded EVERY diff first-person + ran ALL emu/video proofs; commits `d8d16e4`/`8671799`/`b880a62`/`c2e469d`). Slice 1: ERA_SYS8_PLATINUM row APPENDED as BASE w/ Option-B explicit default (FLAIR_DEFAULT_ERA + flair_skin_default); **digest 0xDEF099AC UNCHANGED** (D-10.5a accretion proof standing); frozen/teal oracles re-keyed strictly-stronger + 2 new mutants. Slice 2: canon era:multi + 11 SAMPLED-domain CIDX_PLAT_* ramp indices (OQ-3 zero non-neutral rows; accent = teal per OQ-2; the 192 row carries the C0C0C0 win31 false-friend note); LEG F vs specs/sys8 TOL=0 + the CANON_MUTATE_PLAT gamma-trap mutant (nominal #777777 relapse rejected). The joint landing (WL-0075 pattern, oracle-first): chrome_metrics + chrome_fidelity_golden re-keyed to Platinum (22px band KHFF+LDx6+FFFF+SK, 3 widgets L+4/R-32/R-16, 4px raised body frame, 2px shadow notch; SYS7 retained as FLAIR_CHROME_SYS7_*/FG_SYS7_* per BC-10.10), fidelity legs proven RED-for-Platinum-reasons against the old renderer FIRST, then chrome.c rendered the full delta through 11 new flair_look PARTs (C-8 clean) -- 17/17 legs + **ALL 14 fidelity mutants** (10 re-pointed + COLLAPSE/RAMP/NOTCH/BODYBAR). Emu: only 3 of 12 gates needed grader re-keys (desktop c/d, drag A, solid C -- WL-0076's behavioral grading paid off); scene mutants re-proven; tri-emulator w/ 3 Bochs legs. **Certificate: make clean && make test ALL GREEN 324 host + 85 emu.** VIDEO (operator directive): all 6 locked scripts recorded + repro byte-identical, drag/raise frame-eyeballed (appswitch==solid_switch same trace, script-map fact). TRAPS (bd memory system-8-pivot-re-key-lesson-2026-08): #969696/#C0C0C0 value-aliases re-ATTRIBUTED never deduped; stale-fixture short-circuit (refresh ALL dumps before grading grader re-keys); codex resume syntax is `codex exec resume --last -c sandbox_mode=workspace-write` (no -s; prefer explicit session ids when other sessions run codex). FILED: `chd4` (Route 1 era-as-data wiring, OQ-7 follow-up), `sjvq` (Platinum menu chrome -- operator Law-4 call on the chimera bar), `81ft` (Platinum control chrome incl. the pink alert bevel OQ), + 6 SAMIR gap beads under `586` (aggregates/DDL/macros P3, BROWSE/reports/ASSIST P4) from a dBASE completeness audit. NEXT AGENT: operator Law-4 eyeball of the Platinum desktop (clips in build/clips/, fresh 386 screendump) is the standing ask; then era-independent Wave B (`b3hl` DQ7 menu restore, `t1rv`/`7tjp` DQ8 band-2 dispatch), `zwo8` record-flair follow-ups, or the North-Star B8 seed lane (`ogxv`). See WL-0079 + ADR-0004-AMENDMENT-DEC-10 Sec 4. PRIOR -- **WL-0078: THE SYSTEM 8 PIVOT OPENS -- THE FRAME IS PLATINUM (operator ruling 2026-08-15), THE MAC OS 8.1 GOLDENS ARE MINTED, THE VIDEO HARNESS LANDS, TWO DQ RULINGS SHIP.** Epic `initech-t1i0`: 4/6 children CLOSED same-session -- `kd60` (Mac OS 8.1 boots Platinum under Basilisk II on the EXISTING Quadra-650 mint rig; ISO exact-size-verified; SIGKILL-poisons-HFS + Setup-Assistant-hang traps in bd memory), `slwi` (8 Platinum chrome captures + 329 resources; HONEST: no wctb/thme -- Platinum grays are HARD-CODED in Appearance Extension CDEFs, the captures ARE the color truth; full named CDEF taxonomy extracted), `4lmg` (../system7-decomp/specs/sys8/ ~1430 lines, orchestrator-spot-verified: title bar 22px, stripes FFFFFF/969696, close|zoom|collapse(RIGHTMOST); **THE GAMMA PROOF** -- all 160 sampled colors are the ROM clut 0x11 ladder under Mac HiRes Std Gamma, specs carry sampled+nominal columns; accent = clut 208 LAVENDER, cross-checked non-pixel), `l9cd`+v1.1 (**the GUI VIDEO harness**: `make record-flair SCRIPT=<locked trace>` -> GIF+MP4, `record-flair-repro` PROVEN byte-identical; TWO observer-effect traps caught BY SERIAL MARKERS not pretty frames -- record cadence blew the 250-tick pump budget + 150-tick drag guard, clips looked complete but the trace's back half never dispatched; both bounds now -D-overridable, RECORD image variant, default kernel sha-proven byte-identical; deferred halves -> `zwo8`). **`s97h` CLOSED = ADR-0004-AMENDMENT-DEC-10 RATIFIED Rev 1.0 (operator 2026-08-16, direct: "lavender->teal is canon"; commit 6de3320).** ALL 9 OQs dispositioned (ADR Sec 6): OQ-2 -- the WL-0053 substitution carries into Platinum, applying EXACTLY where real Platinum is lavender-tinged (the measured clut-208 accent: scroll thumb / menu-title highlight / accent ramps -> same-hue Initech teal; NEUTRAL grays stay neutral); OQ-7 -- **Route 2 ratified**: re-key the base constants, registry stays the graded VIEW (flair_skin_resolve has ZERO os/ callers -- verified), Route 1 = funded follow-up to file at 3knt close; OQ-8 -- the 8-bit device CLUT is CONFIRMED unchanged under 8.1 (the gamma proof), clut.json values stand; OQ-1 -- canon goes era:multi w/ per-row tags; OQ-3 -- no new non-neutral canon index (grays by formula, accent = existing teal rows); **BINDING value-domain rule: canon values enter in the SAMPLED screen domain (the domain the existing canon was minted in); the specs' nominal 0x11-ladder column is PROVENANCE ONLY -- a mixed-domain canon is the gamma trap.** Digest NOT re-baselined (append-only proof preserved; the two placeholder legs count!=2 / no-SYS8-row get INVERTED as strictly-stronger checks). Wave B: `r8r7` CLOSED (DQ6 pump-policy drag clamp, solid leg G + no_drag_clamp mutant, clip shows the title band pinned at y=40) + `haaq` CLOSED (DQ5 raise-on-title via a REAL flair_app_dispatch inDrag arm, switch WITHOUT click delivery; leg H + no_raise_on_title mutant + test_process pre-change-RED-on-exactly-4; **Rule-4 catch: codex's first draft synthesized a content click that would toggle the tenant marker on every title drag -- rejected, constraint lifted, reworked via codex exec resume**). Codex division of labor that works: pure code+host-oracle lanes only (its sandbox blocks Unix sockets -- no X/QMP/emulator); Claude agents drive emulators; orchestrator runs ALL emu proofs. Certificate: `make clean && make test` **ALL GREEN 321 host + 85 emu** (legs G/H joined test-flair-solid). **NEXT AGENT -- START HERE, everything is unblocked:** the arc is `3knt` (skin rows + canon accretion + oracle re-key) -> `i3si` (chrome.c Platinum rendering), and they LAND TOGETHER in one certificate (the re-keyed chrome oracles are RED until chrome.c renders Platinum -- the WL-0075 "gates land WITH the fixes" pattern). THE GROUND TRUTH: `../system7-decomp/specs/sys8/` (window-chrome/scrollbars/menus/controls/platinum-palette/INDEX -- every value cites capture+coords; 22px title bar vs Sys7's 19 SHIFTS content_top, the 92li-class geometry blast radius; stripes FFFFFF/969696 light-first; widgets 12x12 close-left / zoom@right-32 / COLLAPSE RIGHTMOST@right-16 w/ diagonal (dx+dy) gradient bevels; 4px raised body frame; 3 scrollbar states; USE THE SAMPLED COLUMN of every table, never nominal). THE RE-KEY SURFACE (ADR Sec 4): spec/flair_skins.h SYS8 rows (append; digest 0xDEF099AC UNCHANGED, invert the two placeholder legs), spec/assets/color_canon.json era:multi accretion, spec/chrome_fidelity_golden.h (~30 FG_* constants -- the BIG one, all 10 CHROME_FID_MUT_* re-proven RED), chrome_metrics.{h,json} 22px + widget offsets, ppm_flair_check legs (stripe probe RE-DERIVED never deleted), the 6 sibling ppm_flair_*_check graders + their locked traces' geometry expectations (solid legs A-H reference title-band rows!), test-color-canon LEG F, test-skin-teal/era-frozen re-key, check-win95isms re-audit (NB: Platinum has NO 0x808080 and its sampled C0C0C0 is nominal AAAAAA -- do not conflate with the Win31 btnface; the specs flag this false friend). VALIDATION: every GUI-visible change gets a `make record-flair` clip (l9cd harness; scripts solid_close/drag/switch/appswitch/clamp/raise; repro-gate PROVEN byte-identical; ADD scripts for new interactions) + tri-emulator Bochs legs + the operator's Law-4 eyeball on a fresh 386 screendump. WORKING MODE THAT WORKS (this session's proof): codex exec (gpt-5.6-sol xhigh, `-s workspace-write`, brief-file via stdin) for pure code+host-oracle lanes -- its sandbox blocks Unix sockets (NO X/QMP/emulator; danger-full-access is classifier-denied), so the orchestrator runs ALL emu legs + grades EVERY codex diff first-person (Rule 4 caught a real Law-4 bug in codex's first haaq draft; correction via `codex exec resume --last -c sandbox_mode=...`); Claude opus agents (screenshot->Read->xdotool loop) for anything driving Basilisk/emulators; ONE main-tree writer at a time. ALSO OPEN: era-independent Wave B (`b3hl` DQ7 menu restore, `t1rv`/`7tjp` DQ8 band-2 dispatch -- rulings on epic av7s design field); `zwo8` (record-flair follow-ups); system7-decomp commits 60c7a36+ are LOCAL-ONLY (no git remote -- operator may want one). Platinum mint env: boot81.img + prefs (mint/basilisk_prefs_quadra_81) + pristine backup /tmp/os81/boot81_pristine.img; traps in `bd memories basilisk` + `bd memories record-flair`. See WL-0078 + ADR-0004-AMENDMENT-DEC-10 Sec 4/6. PRIOR -- **WL-0077: FPC LANDS -- THE DEC-07 RUNG-2 DIFFERENTIAL IS WIRED, THE B5/B6 CORPUS BACKFILLED, AND A 16-BIT-INTEGER DIALECT HOLE CLOSED.** Operator installed fpc 3.2.2 (`altq` unblocked). Per the recorded B7-committee Q13 ruling the wiring landed TOGETHER with the `4yvg` backfill: NEW `seed/examples/fpc/array_shared.pas` (B5: const-bound fill/sum, var-param swap of INDEXED elements, function-call index, lower-bound-0 + NEGATIVE-lower-bound rebase, boolean array, frame-resident local array) + `record_shared.pas` (B6: named record, field r/w, var-param record mutation, whole-record COPY semantics, array-of-record nested l/r-value) -- SEED_FPC_CORPUS is now 4 fixtures. **THE FIND: the corpus ran fpc with 16-BIT integers** -- ADR-0007 DEC-02 dialect pin 1 (32-bit `integer`, BINDING) was pinned in-source for {$B+} but NEVER for the mode; fpc's default mode wrapped array_shared's REV=54321 to -11215, and the old corpus never crossed 32767 so the divergence was LATENT (the differential would have blamed the seed the first time any fixture exceeded 16 bits). Fixed corpus-wide: line 1 = `{$MODE DELPHI}{$B+}` (+`{$H-}` string_shared), mode FIRST (it resets directive state). **WIRED (`altq` CLOSED): test-seed-fpc-diff is IN TEST_UNIT_GATES**, Bochs-precedent (default-ON, LOUD FAIL when fpc absent, SKIP_FPC=1 the one shouting opt-out, Make-level ifeq); CLAUDE.md toolchain line + note updated (fpc is a base requirement). Rule-6 proof one-shot + right-reason-verified: mutated seeds (ARRAY_LO_SKIP, FIELD_OFF4) drive the diff RED with WRONG VALUES vs a green clean-seed control (first attempt was a wrong-reason RED from an unexpanded var in the ad-hoc pipeline -- caught, redone); the permanent per-family mutant gates unchanged green. **Certificate: `make clean && make test` ALL GREEN 321 host + 85 emu** (test-seed-fpc-diff joins the host vector). NEXT AGENT: B8 (`ogxv`) now has its Rung-2 harness ready (add file_shared.pas when it lands); the solidity Wave B (WL-0076) remains the staged GUI work. See WL-0077. PRIOR -- **WL-0076: SOLIDITY WAVE A LANDED -- THE ONE REPAINT CONTRACT IS LIVE AND THE SOLID GATE BITES.** Operator directive: proceed first-person, NO delegation; hunt latent bugs while landing. The stopped DQ committee was NOT resumed -- DQ1-DQ10 ratified solo from the drive-report proposals, binding record on epic `initech-av7s`'s design field (DQ1 validation ownership: `desktop_paint_damage` clears ONLY desktop_update, window validation moves to the content phase -- `flair_route_updates` validates EVERY walked window incl. unowned, or the NEW `desktop_validate_all`; DQ2 pump order WM-op -> chrome -> content -> present on every damaging dispatch; DQ3 `reaffirm_active` seeds strucRgn bbox on BOTH transitions; DQ4 deferred to the 8fhu lane; DQ5-DQ8 ratified for Wave B; DQ9/DQ10 as proposed; naming deviation: the leg-C knob is `WINDOW_MUTATE_NO_ACTIVATE_INVAL`). **LANDED (beads `gofc`+`rqz5`+`0zxp`(superseded)+`vtdo` CLOSED):** window.c 0->1 activation seed (+ new mutant); desktop.c paint_damage no longer validates window rgns + NEW desktop_validate_all; process.c route validates all walked; kmain `flair_live_ctx_t.plist` + `flair_live_content_phase` (route-or-validate, mutant `FLAIR_LIVE_MUTATE_NO_ROUTE_ON_CHROME`) on the inDrag/inGoAway dispatches; switch block's contRgn re-seed removed; scene builds close their books (flair_desktop_run + the tenants launch site). **`test-flair-solid` + `-mutant` WIRED into TEST_EMU_GATES: legs A/B/C GREEN on the booted 386 (every frame eyeballed -- the white-hole family and half-active bars are GONE live) and per-leg mutants proven RED.** Host contracts strengthened (test_window activation property 800-stack owner-grid-exact; HideWindow golden unions the successor's activation seed; test_process_update WU flips to validated-no-delivery; test_drag/test_shell adopt paint-then-validate). **BUGS FOUND while landing (operator asked):** `vtdo` (P1 FIXED -- ref_tenant painted at its LAUNCH-time cached content rect forever, incl. the graded accent at ABSOLUTE demo coords; all rects now derive from live contRgn, boot frame bit-identical; caught by solid leg B's white L-strip); `wlzp` (P2 open -- updateRgn-within-visible is an UNSTATED load-bearing invariant, tenant paint clips to contRgn+updateRgn only; the launch site was a live violation caught by appswitch TIER-A mid-landing, fixed narrowly; proper fix = BeginUpdate-fidelity clip at delivery); `j0vt` (P2 open -- tenants cross-menu restore via shell_render wipes tenant content, Wave B menu lane). **RULE-6 HONESTY: the `no_paintall_clear` emu mutant was STRUCTURALLY HEALED by the DQ3 seed** (every switch repaints both tenants fully -- the stomp is unreachable in the 2-tenant trace) and once MASKED by the new validate_all backstop (knob now shared, winh x ojxn); its proof moved to a SHARPER host case (test_drag.c leg (e): 3 windows, a window ARRIVES on the stale footprint, only a bystander damaged) in test-drag-mutant, emu image retired with an OMISSION note; TIER-C stays a live invariant check; lesson recorded (`bd memories mutant-structural-healing`). **Certificate: `make clean && make test` ALL GREEN 320 host + 85 emu** (solid + solid-mutant join the vector; Bochs legs inside). NEXT AGENT: Wave B per drive report Sec 7, first-person or lanes -- B1 chrome.c (`6yzb` notch, `9d0e` shadow, `l0ra`/`hber`/`jxsf` zoom cluster), B2 ref_tenant (`javs` scrollbar-column, `l0mh`, `vfd8` titles), B3 kmain interaction (`t1rv`+`7tjp` band-2 dispatch per DQ8, `b3hl`+`j0vt` menu restore per DQ7, `r8r7` clamp per DQ6, `haaq` title-click raise per DQ5, `883x` win-id, `tbef`+`ci4o`, `8fhu` close incl. the deferred DQ4 ruling); then `wlzp` (the BeginUpdate clip). B8 seed (`ogxv`) still queued behind the arc; fpc still NOT installed (`altq`, operator: sudo apt install fpc). See WL-0076. PRIOR -- **WL-0075: THE FLAIR SOLIDITY PIVOT -- THE DESKTOP IS DIAGNOSED, THE ORACLE IS RED, THE FIX ARC IS STAGED.** Operator redirected from B8 to FLAIR ("solid and dependable; a dozen little quirks sum to a real mess"). A 15-scenario FIRST-PERSON drive battery (QMP injection + marker-gated screendumps, every frame eyeballed; dev aid `harness/emu/drive_flair.py`; gotcha: harness `--out` must be SHORT -- QMP unix socket sun_path 108-cap) proved the live tenants desktop DEGRADES MONOTONICALLY inside an ALL-GREEN 320+83 certificate (re-verified first-person this session). **THE THREE-PAINTER DIAGNOSIS** (full report `docs/FLAIR-solidity-drive-2026-07-21.md`; 3 forensics agents, every claim re-verified): P1 boot (chrome then tenant content erases the scrollbar column), P2 compositor (chrome ONLY + `WindowMgr_validate` of EVERY updateRgn unconditionally, desktop.c:269-273; `flair_route_updates` only on a foreground switch, kmain.c:2406-2413 -- so drag/close/expose WIPES tenant content to WDEF white), P3 activation (content only; reaffirm_active seeds 1->0 but 0->1 seeds NOTHING, window.c:226-235 -- newly-active windows keep flat inactive bars; HALF-active bars clipped to damage). Plus: band-2 menus fully DEAD (y<20 test + bar_sys hard-coded, 3 sites), menu-cancel leaves the panel (DELIBERATE demo wart, kmain.c:1276-1285 -- the oracle bent the artifact), ghost drag (no raise), no drag clamp (window recoverable-never), zoom->drag misroute (documentProc vs drawn gadgets), `win -1` serial identity, notch = scrollbar top edge doubling the separator (chrome.c:699 vs 396), drop shadow drawn-but-clipped-away (chrome.c:854-865 outside strucRgn -- 9d0e root FOUND), zoom-box "teal bleed" is the RATIFIED lavender->teal bevel wrecked by the 8bpp BEVEL_SHADOW->black collapse (l0ra). **LANDED (oracle-first, Rule 1):** `tools/ppm_flair_solid_check.c` legs A (close-expose content) / B (drag preserves content) / C (activation chrome full-width + no stale band) -- each RED-PROVEN against the captured frames for the right reason; LOCKED traces `spec/flair_solid_traces.mk`; NOT yet wired into `make test` (gates land WITH the fixes). **BEADS: epic `initech-av7s`** (label flair-solidity, 17 children): NEW P1 roots `gofc` (the update contract) + `rqz5` (activation invalidate); NEW P2 `r8r7` (drag clamp) `b3hl` (menu restore) `883x` (win -1) `6yzb` (notch) `tbef` (variant/hit-test mismatch); annotated-with-roots `0zxp` `t1rv` `7tjp`(->P2) `8fhu` `9d0e` `vfd8`; adopted `javs` `l0mh` `ci4o` `8kx3`. **A 3-seat design committee on DQ1-DQ10 was STOPPED mid-run (operator wrap-up) -- the full DQ agenda + proposed rulings live in the drive report Sec 6; resume via the recorded runId or just re-launch.** NEXT AGENT: (1) run the committee -> ratify DQ1-DQ10; (2) Wave A first-person on main: `gofc`+`rqz5` (desktop_paint_damage stops validating -- validation moves to route/explicit; route on EVERY damaging dispatch; reaffirm_active seeds BOTH transitions; `flair_live_ctx_t` gains a plist member) + wire legs A/B/C + mutants NO_ROUTE_ON_CHROME / NO_ACTIVATE_INVAL; (3) Wave B lanes per report Sec 7; (4) full aggregate + Bochs + WL shard. The B8 seed step (`ogxv`) remains queued behind this arc; fpc still NOT installed (`altq`, operator: sudo apt install fpc). See WL-0075. PRIOR -- WL-0074: B7 STRINGS LAND BY COMMITTEE -- THE SEED SPEAKS SHORTSTRING.** Certificate: `make clean && make test` ALL GREEN **320 host + 83 emu** (test-seed-string-mutant joins the vector). NOTE: B4 (`63ce`, THE CODEGEN PIVOT), B5 (`54uu`), B6 (`rug7`) and the FO-5 repro gate (`3yv`) landed 2026-07-11 after WL-0073 was cut (commits `0766f38..5d0d45d`, no shard -- their stories live in the commit messages). **B7 (`39k2`, commit `62ce0db`) CLOSED this session, fully orchestrated:** a THREE-SEAT design committee (2 opus + 1 sonnet, chair-synthesized, NO gridlock; the full binding D1-D12 record is on the bead's design field) ruled the contested surface -- TP ShortString byte-packed round4(N+1) w/ ascending base for every storage class; ALL string compute as codegen-emitted intrinsics gated on uses_strings (start.asm FROZEN per DEC-05; a stringless program's .s is BYTE-IDENTICAL to pre-B7, hand-verified); frame-resident 256B temp pool via a deterministic per-statement pre-walk; silent truncation to compile-time dest cap; the D7 coercion table (char coerces ONLY at assign/concat/compare; char+char = located error, a RECORDED fpc divergence the fixtures pin around); loud rejections (array-of-string, string record fields, value string params, string[N<255] var formals, string function results) each w/ a named self-host idiom. **THE CHAIR'S CORRECTION (Rule 4, caught re-deriving both opus seats):** NO string temp is ever live across a call in this subset, so the deep bug is TWO-LIVE-TEMPS-IN-ONE-STATEMENT (right-nested a+(b+c), string.pas's RNEST clause), NOT recursion clobber. Mutants: STR_TEMP_CLOBBER corrupts exactly RNEST, STR_CMP_NOLEN flips exactly EQF -- both proven RED-for-the-right-reason; repro now 14 fixtures/42 hashes. ORACLE-INTEGRITY BEADS FILED: `altq` (P1 -- fpc NOT INSTALLED on this box AND test-seed-fpc-diff not in TEST_UNIT_GATES = the DEC-07 Rung-2 differential is ORPHANED; operator: `sudo apt install fpc`, then wire Bochs-precedent-style); `4yvg` (P2 -- B5/B6 never got their promised fpc fixtures); `lh83` (P3 -- B9 symbol-table names: char-pool idiom vs array-of-string, linked into `6m52`). TRAP RECURRENCE: the stale-worktree base bit AGAIN (first lane spawned from a June commit; its mandatory STEP-0 base gate aborted w/ zero edits) -- remediation in bd memory `worktree-lane-dispatch-pattern-2026-07-14-b7`: the orchestrator cuts the worktree ITSELF at the pinned SHA. NEXT AGENT: B8 `ogxv` (file-I/O RTL over INT-21h, FILEIO-class mutant) -> B9 `6m52` (author Turbo Initech in os/tps/; resolve `lh83` first) closes M7. See WL-0074. PRIOR -- WL-0073: ADR-0007 RATIFIED + TURBO INITECH B1-B3 LAND + THE CHROME TAIL CLOSES.** Certificate: `make clean && make test` ALL GREEN **313 host + 83 emu** (B3's gate verified on main, next cert reads 314). **ADR-0007 IS RATIFIED** (`79s`): authored + three-seat committee (unanimous ratify-with-amendments, 13 amendments applied at ratification -- incl. dialect pinning: 32-bit integer / fpc {$B+} / case-insensitive idents; the `forward` directive added to REQUIRED; FO-5 = a test-seed-repro gate MUST exist before B4; OQ-1 resolved option (b) w/ the PRD staging clause; citation loop verifiably closed). **THE NORTH STAR IS MOVING: B1-B3 landed same-day** -- `f0uc` booleans/relational w/ a NEW typecheck pass + complete evaluation enforced STRUCTURALLY (branch-free gen_binop grep leg); `80iw` if/while + for/repeat sugar (bounds-once __forlim_N, ISO 6.8.3.9), ONE threaded label counter; `7mo3` const folding + char/ord/chr (TP semantics cited) -- each RED-first, mutation-proven, double-compile sha256-identical. Chrome tail: `hv7u` full inactive-WDEF fidelity (canon extended via the EXISTING gray-ramp mechanism, locked table untouched; a9iq-era wrong leg corrected, declared); `jh7m` closed ALREADY-SHIPPED (85d5ec9 -- the audit's no-code-landed note was WRONG, all oracles re-verified); `vcq` closed SUPERSEDED (its frame-still quantizer ask is the HER-11 heresy post-ADR-0010; residual SEAFOAM-vs-teal bead extracted). NEXT AGENT: `3yv` (test-seed-repro, FO-5) -> `63ce` (B4 THE CODEGEN PIVOT: frames/calls/params -- the M7 watershed; hard mutation-proofing on var-param aliasing + frame offsets) -> B5..B9. Also: `x0i` 86Box (P1), `586.4`, `dam` (operator-gated). See WL-0073. PRIOR -- WL-0072: ORACLE HARDENING + THE MOUSE-POLARITY FIX + THE NORTH STAR SIZED.** Closing certificate: `make clean && make test` = **ALL GREEN 311 host + 83 emu**. 8 more beads closed (29 across WL-0070..72). HEADLINE: **the live mouse was VERTICALLY INVERTED (`rgt8`, operator-reported)** -- the producer packed raw PS/2 dy against the locked Sec-5 contract, every mouse-Y oracle was by-construction on the sign axis (HER-02), and every injection trace/golden had BAKED THE BUG IN. Fixed at the producer (`mouse_pack.h`), a hardware-contract wire-packet oracle (`test-mouse-producer`, independence proven by contrast) landed, ALL traces rebaselined (incl. the LOCKED appswitch trace, Rule-8 note in-file). Also: `9op1` live cross-menu redraw + deterministic crossdrag emu leg; `yx4v` hand-authored apple-with-bite strike (old ppm threshold ENCODED the square bug -- recalibrated, declared); `h5vg` .ndx bulk build arbitrary-depth (incremental paths fail-loud, golden-gated follow-up); `quke` softfp impl-mutant (the old one perturbed the REFERENCE) + ansi clamp/garbage legs; `in2g` three Bochs legs IN `make test` (loud-fail; SKIP_BOCHS=1 escape); **`qvtj` -- the North Star is SIZED**: docs/plans/TPS-M7-subset-plan.md + 10 dependency-ordered beads under epic `rnh` (B1 booleans -> B4 codegen pivot -> B9 real fpc differential closes M7), ADR-0007 scope draft on `79s` (now unblocked). LESSONS (bd memories): ONE main-tree writer at a time (in2g hunks swept into unrelated commits -- recorded in 393b4ec); when a producer convention flips, sweep ALL traces (grep _SPEC), not just failing gates. NEXT AGENT: `79s` ADR-0007 authoring -> `f0uc` (B1); chrome tail `hv7u`/`jh7m`; `x0i` 86Box (P1); `586.4`; `dam` (operator-gated frame stills). See WL-0072. PRIOR -- WL-0070 + WL-0071: AUDIT PHASE 1 EXECUTED + THE REDIRECTION ARC COMPLETE + THE 44ab CAP DECISION + FILTERS.** One orchestrated two-day session (subagent lanes graded + RE-VERIFIED FIRST-PERSON on main before every acceptance, Rule 4; committee process for the locked-spec change). **Closing certificate: `make clean && make test` = ALL GREEN 306 host + 78 emu (2026-07-11)** -- from 296+61 at the audit, every added gate mutation-proven. **21 beads closed**, 14 commits pushed (`c1f7e36..fb5449c`). LANDED: (1) **Audit Phase 1 complete** -- `6vr1` (8 KERNEL_*_OBJ prereq rules literal-pathed; incremental builds honest again), `ljck`+`mtqw` (all 10 orphaned gates folded in; TWO mutant kernels had never even LINKED -- missing devices/mcb objects -- fixed, both bite), `gymo` (stdfile landed + wired), `c921`; Phase 2 substantially done: `xi7x` (region generators over the FULL int16 domain, window-translated oracle, sign-dependent mutant proven), `0k2d` (all 6 prog-diff goldens now bite via ENGINE perturbations), `tf3c` (seed mutant gates; negative div/mod proven a coverage gap, not a bug), `msol` docs sweep. (2) **The MILTON redirection arc is DONE and `hsct` is CLOSED**: `bsy.9` (child PSP inherits the parent JFT on EXEC + sft_inherit refcount symmetry) -> `bsy.7` (`<` input, restore-handle-0-on-every-path, GOBBLE.COM) -> `bsy.8` (`|` authentic temp-file pipe, PIPEn.$$$, no exit-code short-circuit, 8-stage bound) -> `m0dc` (SORT /R + FIND /V/C/N/I + MORE with the AH=44h IOCTL device fork -- CON pager / file passthrough). `> >> < |` ALL work in COMMAND.COM, each leg host+emu mutation-proven. (3) **`44ab` -- the deliberate Rule-8 locked-spec change, 3-round committee**: RGN_ROW_X_MAX 256->640 (= the OD-3 domain bound; per-band merge output PROVABLY domain-limited -- Sec-5 rewritten as the two-bounds passage), RGN_X_POOL_CAP 1024->2048, ADR-0005 formally amended; the working set is DUAL-MODE (hosted static / freestanding FLAIR-heap-bound via region_engine_bind_ws) after TWO tripwires fired honestly: the 18KiB static WS vs the single 64KiB kernel stack, then kernel_shell's `_kernel_end<PROGRAM_BASE` guard bust (8,340B over; margin now 8.6KiB -- the structural runway bead is filed, PROGRAM_BASE relocation looming). Verification rounds caught: kstart.asm NEVER zeroes .bss (a naive guard flag would have blocked the desktop from ever rendering -- explicit region_engine_reset() now); guard-leak-on-early-return had zero coverage (T2 leg added). test-region now 65 checks; QEMU + all 3 Bochs boot legs green. (4) **Law 4**: `zvo6`+`a90f` (FILE COPY is the reference's moveable titled modal -- movableDBoxProc, the missing MTE Table 4-1 row, drawn by the SAME title-band code as documents -- with the canon 68% blue fill, static-assert-locked to the cited 65-70 range), `rl4v` (cross-menu drag works: per-point MenuBar re-hit per IM MenuSelect; the kmain live-redraw sibling bug is FILED). (5) **Factory**: `7s1z` -- `make image`/`make run`/`run-bochs` are REAL (flagship = flair_tenants desktop; -serial stdio needs a tty, use -serial file: when scripting). TRAPS RECORDED: **agent worktrees spawn from a STALE base** (`ck22` + bd memory -- check `git log -1` in every lane worktree; 4 lanes affected, all integrated via `git apply -3` + full first-person re-verification on HEAD; also nested worktrees break `../sister-repo` relative paths); integration re-measures (a lane's kernel-margin number is about the LANE's tree, not main's). NEW BEADS: `g0bm` (.PHONY prereq pattern), `mpi7` (stdfile doOpen fail-loud gap), prog-diff corpus gap (single-true-CASE flow.prg), kmain live-redraw menu bug, kernel low-mem runway. NEXT AGENT: `in2g` (promote the Bochs boot leg into `make test`), `quke` (softfp/ansi oracle audit), `h5vg` (.ndx 2-level ceiling), `hv7u`/`jh7m` (chrome fidelity tail), `x0i` (86Box, P1), then Phase 4 North-Star items (`qvtj` M7 sizing, `79s` ADR-0007, `lk29`, `586.4`, `dam` frame fixtures). See WL-0070 + WL-0071. PRIOR -- WL-0069: THE WHOLE-REPOSITORY AUDIT + IMPROVEMENT PLAN. A 12-lane parallel survey + adversarial critic + FIRST-PERSON verification of every load-bearing claim reviewed the ENTIRE programme. The WL-0068 certificate RE-VERIFIED first-person: `make clean && make test` = ALL GREEN 296 host + 61 emu (2026-07-09). VERDICT: the discipline is sound -- the failures found are META-failures (oracles that never run, prereqs that parse empty, docs that lag), not wrong shipped code. HEADLINE FINDINGS (each verified, each now a bead, label `audit-2026-07`, epic `initech-croo`): (1) `6vr1` WIDENED P2->P1 -- SEVEN `KERNEL_*_OBJ` Makefile rules reference later-defined vars in their PREREQ lists (parse-time-expanded => EMPTY): `build/region.o` does NOT depend on `region.c`; incremental builds relink STALE kernels (oracles unaffected -- they compile sources directly -- which is why it survived). (2) `ljck` -- NINE real mutation-proven gates ORPHANED from `make test`, incl. the ONLY `jf8p` regression guard. (3) `gymo` -- the untracked stdfile WIP is FINISHED (verified 29/29 green, 3/3 mutants bite, freestanding typecheck OK); needs Makefile wiring + commit only. (4) Law-2 soft spots: the real-DBASE.EXE differential (the PRD's stated primary SAMIR oracle) is loud-skipped everywhere (`586.4`); 6/7 prog-diff goldens have no proven mutant (`0k2d`); `.ndx` is 2-level-only (`h5vg`, new). (5) Rule 5 in practice: the default gate is QEMU-only (`in2g` promote Bochs; `x0i` 86Box). (6) `xi7x` -- the region property suite NEVER samples negative coords though every top-left drag produces them live. (7) Governance: ADR-0001/0002 cited as ratified but NO FILES exist (`lk29`); `os/tps/` is EMPTY and the M7/M8 gap unsized (`qvtj`). BEADS RECONCILED: closed `pipa`/`ax9.2`/`5l5z` (verifiably shipped); reverted `bea`/`40oq`/`hdlb`/`hsct`/`jh7m`/`re30.4` in_progress->open with evidence; claimed `gymo`; filed 15 new (see the plan); dep chain `bsy.7<-bsy.9`, `bsy.8<-bsy.7` registered. THE PLAN: `docs/plans/AUDIT-2026-07-09-improvement-plan.md` -- Phase 1 gate integrity (`6vr1`->`ljck`->`gymo`->`7s1z`->`c921`, ONE session), Phase 2 oracle strengthening (`xi7x`/`0k2d`/`tf3c`/`quke`/`in2g`), Phase 3 the GUI tail + the redirection arc in TRUE order (bsy.9 FIRST -- psp.c:141 hardcodes the JFT, structurally blocking bsy.7/bsy.8), Phase 4 the North Star (`qvtj` M7 sizing, `79s` ADR-0007, `lk29`, `586.4`, `dam`). NEXT AGENT: execute Phase 1, close with the authoritative clean gate. See WL-0069 + the Sec 5 CURRENT-STATE block. PRIOR -- WL-0068: THE SAMIR/xBASE P1 CORRECTNESS CLUSTER CLEARED -- 12 P1 correctness bugs (the InitechBase dBASE-III+ engine + the seed lexer + MILTON COMMAND.COM) fixed + committed + each mutation-proven, PLUS a Law-2 oracle heresy (`initech-bljs`) and a defense-in-depth mutant interaction (`winh` x `ojxn`) CAUGHT AT INTEGRATION -- on `command-com-default` (`6ed6db9..bc98821`, 15 commits). `make clean && make test` = ALL GREEN (296 host + 61 emu). ORCHESTRATED: 12 bugs mapped to 7 CONFLICT-FREE worktree lanes (2 opus + 5 sonnet, disjoint files -- only the Makefile a shared merge surface), each Red->Green->mutation with an INDEPENDENT (non-by-construction) oracle grounded in `../dbase3-decomp`; the orchestrator integrated LANE-BY-LANE, RE-RAN EVERY subsystem oracle itself on the main tree (never trusting a lane report, Rule 4), then the full clean gate. Beads closed: `3p9e` (the ?/?? logical .T./.F. ORACLE HERESY -- ? echoes DOTTED, LIST columns keep bare T/F; repl oracle de-heresied), `ue7y` (SET DATE/SET CENTURY), `wcb7`+`qw4e` (LOCATE RECORD/NEXT n scope bounds), `0g22` (ndx_delete_key walks the equal-key run across leaves), `x87g` (PACK FULL REINDEX of open NDX -- renumbering invalidates survivor recnos so per-recno delete is insufficient), `9u0f` (CTOD blanks calendar-invalid dates), `eyig` (ROUND floors negatives via rt_floor64), `jf8p` (dbf_open_rw +2-form header_length normalized ON FLUSH not at open), `dmrw` (SKIP -1 from EOF was one short), `2hg9` (seed scan_string pool-wrap -- FACTORY, not the artifact), `winh` (dos_read Carry-Flag -> TYPE/COPY/batch break on read error; new PRN-device EMU gate). TWO INTEGRATION TRAPS the orchestrator's COMPOSED grading caught that NO single lane could: (1) `bljs` -- merging the correct `3p9e` engine fix turned `test-dbase-diff` RED 4/8 because the xbase_prog_diff Tier-0 goldens carried bare T/F on the ?-echo lines (the SAME heresy, SECOND location); corrected ONLY whole-line T/F -> .T./.F. (LIST columns correctly UNTOUCHED -> proves 3p9e is precise) + re-pointed the SWAP_EQ mutant so it still bites; STRENGTHENS toward III+ ground truth, FLAGGED for post-hoc committee review. (2) `winh` x `ojxn` -- winh's read-error abort became a SECOND backstop that MASKED the WL-0067 copy-self mutant's data loss; the mutant now disables BOTH backstops (`-DCMD_MUTATE_NO_READ_CF` too) so it still bites. Lane C ran test-dbase GREEN in its OWN worktree BECAUSE it lacked A's fix -> the class is ONLY visible on the composed main tree + the authoritative `make clean`. Both OPUS lanes hit the opus weekly rate limit mid-flight -- NO work lost (per-bug commits on branch); SONNET finishers completed qw4e + x87g in place. NEXT AGENT: the GUI P2 tail + WL-0067 follow-ups, then the canonical app suite (Initech123/InitechWord) on the now-solid GUI + hardened SAMIR; follow-up `cq0j` (P3, LOCATE NEXT n EOF() bounded-miss oracle vs real III+). See WL-0068 + the Sec 5 CURRENT-STATE block. PRIOR -- WL-0067: THE FLAIR GUI FIX ARC COMPLETE -- the live desktop is the frame again. The WL-0066 mandate (a FIX arc, not features) is DISCHARGED: all 7 targeted bugs fixed + committed + each mutation-proven (`1ff449f..16b02a7` on command-com-default): 2 P0 -- `mswo` region xmerge stack-overflow (xmerge output-bounded + fail-loud; ASAN mutant) + `ojxn` COPY-onto-itself data-loss (src==dst guard, DOS-3.3 MSG-DOS-0020, EMU no-data-loss gate); 5 P1 compositor/chrome -- `v6t2` deactivation repaint (reaffirm_active invalidates on 1->0), `a9iq` inactive-window chrome (flat white title per the decomp title-bar.md, active byte-identical), `4w15` static System-7 shell bar + distinct-chimera boot (swap targets band 2; HELLO boot-foreground), `pipa` bars+modal survive a drag (a ~15-LOC `wm->overlay_rgn` fold into `fronts_union` -- occlusion + seafoam-erasure closed at the ONE shared primitive, no desktop.c change; EMU `dc4v` gate), `jmc5`+`qi8v` background-window teal-erasure (root cause = a stale `wm->desktop_update`; `desktop_paint_all` now clears it; appswitch TIER-C). ORCHESTRATED: a 3-seat committee + adversarial red-team ratified the CONTAINED overlay fold over the heavy uniform-window model AFTER a live re-test re-scoped qi8v/jmc5 from "the architecture" to a 1-liner; parallel worktree lanes (<=2 opus, <=6 sonnet); the orchestrator re-ran EVERY oracle on the main tree (never trusting a subagent report) + EYEBALLED the pixels -- the post-drag frame IS the Office Space "Saving tables to disk..." still. Traps caught, not shipped: jmc5's mis-filed root cause (saved a wasted arch lane), 4w15's two-identical-bars boot regression (caught by driving the screendump), a TIER-C mis-calibration that would have shipped a green-but-blind oracle (re-calibrated empirically to NOTES, the actually-erased window). `make clean && make test` = ALL GREEN (291 host + 59 emu gates; the closing run also CAUGHT + fixed a v6t2 test-isolation regression in `test-process-update` -- the mandatory clean gate doing its job). NEXT AGENT: the fix arc is DONE -- resume the P2 tail of the `gui-bughunt` set + the filed follow-ups (`44ab` region cap decision, `hv7u` full inactive-chrome fidelity, `zn61` true modal input-blocking, `vj28` COPY same-file edge cases, the Makefile KERNEL_DESKTOP_OBJ build bug, a permanent qi8v drag oracle), then the canonical app suite (Initech123/InitechWord) ON the now-solid GUI. See WL-0067 + the Sec 5 CURRENT-STATE block. PRIOR -- WL-0066: THE LIVE-GUI BUG HUNT -- FEATURE WORK HALTED. The operator ran `make run-flair-tenants`, found the live desktop badly broken (~5 bugs in seconds), and was right: Phase 4.5 (WL-0065) shipped 5 commits on GREEN *host* oracles + a single boot-frame screendump but NOBODY DROVE THE LIVE DESKTOP -- the oracles "grade math, not composited pixels or live behaviour" (initech-pipa). 3 adversarially-verified fan-out rounds (driving `qemu_harness --mouse/--keys` + screendump + comparing to the `../system7-decomp` goldens -- the method researched in `initech-l9cd` that had gone unapplied) found + filed 138 NEW bug beads (label `gui-bughunt`) + 4 known = 142 distinct: 2 P0 (`initech-ojxn` COPY-onto-itself truncates-to-zero data loss; `initech-mswo` region_op stack overflow), ~20 P1 (the compositor does not damage-track non-WindowMgr layers NOR repaint background windows -> drag/app-switch ERASE peer windows; NO active/inactive window chrome at all), rest P2. Full report `docs/FLAIR-GUI-bug-hunt-2026-06-28.md`; `bd stats` 438->579; lesson `bd memories flair-live-gui-testing-gap`. NEXT AGENT: a FIX arc, NOT new features -- start at the 2 P0s, then the compositor-damage + active/inactive-chrome P1 families, then stand up composited-pixel emu gates + the l9cd capture. See the Sec 5 CURRENT-STATE block for the full method + start order. PRIOR -- WL-0062: THE LIVE INTERACTIVE FLAIR DESKTOP + the chrome FIDELITY arc.** On the booted 386 windows now DRAG and menus DROP+SELECT under the mouse, and the window-chrome FIDELITY arc is COMPLETE -- all 6 elements (drop-shadow / close-zoom box / scrollbar / title-bevel + the prior phase/title) graded vs the INDEPENDENT `../system7-decomp` goldens via `test-chrome-fidelity`. Orchestrated session (committee fork + delegated lanes + independent grading), **13 commits `93e8cd4..7dcc4e1`, all pushed**; `make clean && make test` = **ALL GREEN 273 host + 53 emu**; both Bochs legs PASS; reproducible every step; `_kernel_end(flair_live)=0x37174 < 0x40000`. The event loop (`initech-5l5z`, behind `-DBOOT_FLAIR_LIVE`, default boot UNCHANGED): 3 ISR producers (PIT tick / kbd IRQ1 / mouse IRQ12 dual-PIC-EOI minefield) feed the FLAIR raw ring via function-pointer hooks; the WaitNextEvent cooperative pump replaces render-once-HLT and dispatches FindWindow part-codes -> DragWindow + D-5 desktop_paint_damage (DRAG) + MenuSelect (MENUS). Oracles: test-interact (HOST, independent-by-recomputation) + test-flair-drag + test-flair-menu (EMU, independent-canon screendump) + HER-14 drag-noop/menu-noop mutants. **Law 4's "live, draggable arrangement with working menus" is satisfied; the re-flair epic `qipc` step 7 core is delivered.** Remaining are FILED follow-ons (polish/hardening): menu close-restore + CombineRgn (rmsr); live drop-shadow via strucRgn-widening (`9d0e`); mouse-ACK drain; Bochs strict-PIC mouse probe (`04ae`); `test-arena-disjoint` ASLR determinism. See **WL-0062** + the §5 CURRENT-STATE block below. **PRIOR (2026-06-25): FLAIR WINDOW-CHROME FIDELITY ORACLE -- window APPEARANCE is now graded against the `../system7-decomp` captures (it was NOT).** The operator observed the live FLAIR windows "do not look like System 7" and suspected the goldens did not enforce window correctness. An audit (two workflows + first-person verification) CONFIRMED it: NO green gate graded window APPEARANCE -- `test-color-canon` grades color VALUES, `test-chrome` grades metric NUMBERS vs FLAIR's OWN `chrome_metrics.h` (by-construction; "STRUCTURAL compare, not SSIM", ADR-0004 D-8), `ppm_flair_check` grades scene topology calibrated to `test_shell.c`; the real System-7 window captures in `../system7-decomp/goldens/captures/` were NEVER diffed, and SSIM (`harness/ssim.c`) is unbuilt. Built **`test-chrome-fidelity`** (oracle-first, RED->GREEN) grading the REAL `chrome.c` via the host render skeleton against an INDEPENDENT pixel-measured golden **`spec/chrome_fidelity_golden.h`** (from the decomp `specs/chrome/*.md` + captures, NOT from `chrome_metrics.h`). **Element 1 (WL-0060, commit `cb76816`):** the pinstripe **PHASE** -- the free-running `L,D,L,D` fill was WRONG yet PASSED every "period-2 alternation" check; `chrome.c` is now phase-locked (`LLDLD..DLL` doubled-LIGHT pairs, patAlign mod-8). 5 strict-period-2 sites that encoded the bug were operator-ratified-amended to phase-agnostic "striped" checks (`test_chrome.c`, `test_shell.c`, `test_drag.c`, `ppm_flair_check.c` leg c + HER-02 demo). **Element 2 (WL-0061, commit `dc5ddca`, bead `initech-lxg9` CLOSED):** **TITLE TEXT + knockout** -- `chrome.c:279` drew BLANK title bars; now the window title is drawn CENTERED in Chicago over a knocked-out light gap (the biggest visual win), via the C-8 policy seam (colorblind-clean); root-cause fix `NewWindow` now defaults `titleHandle` to "" (was uninitialized). Live desktop now shows W0 "untitled-1" + W1 "Saving tables to disk", centered. **`make clean && make test-unit` = ALL GREEN 271 host; Rule 5 QEMU `test-flair-desktop` PASS + Bochs PASS; reproducible-build green; `_kernel_end=0x35b20 < 0x40000`.** **NEXT:** the remaining window-chrome fidelity elements (open dependents of umbrella `initech-hmll`): `ts3t` close/zoom box geometry (11x11, +9/-20 offsets, zoom glyph), `92li` bevel rows + 15-row interior recomposition (shifts `content_top` -- geometry blast radius), `jh7m` scrollbar arrows/thumb + black-outer/gray-inner separators, `54nw` drop shadow (1px L at (1,1); NOTE `ppm_flair_check` asserts the pixel outside the frame is bare teal -- amend). Each grades the REAL `chrome.c` against a decomp-pixel-measured golden, same pattern. **LESSONS:** the title sits at bar-center where 5 oracles scanned the pinstripe -- a grep (Rule 3/4) caught them all incl. the HER-02 demo (which the emu run caught); a FLAKY `test-kernel-repro` RED in the INCREMENTAL aggregate was GREEN on a CLEAN build (authoritative = `make clean`); the C-8 seam is load-bearing for text (a raw color literal would trip the colorblind oracle). **PRIOR (2026-06-22): FLAIR ARCHITECTURE RE-RATIFICATION COMPLETE (operator-ratified; epic `initech-qipc`).** A grading audit proved the FLAIR oracle graded **BY CONSTRUCTION** (`ppm_flair_check` computed expected RGBs from `flair_palette_rgb` -- the SAME function `kmain` paints with -- so the "very wrong" `preview.webp` palette passed EVERY gate), the decomp corpora were NOT the live golden master (only `test-clut` diffed one CLUT, and `clut.h` drove no rendering), and the booted OS ran **no FLAIR event loop**. An orchestrated overhaul followed: a **heresy audit** (20 candidates, **19 upheld / 1 struck**, adversarially adjudicated) -> **6 architecture committees** + a consistency critic (caught it did NOT compose: the color module was quadruple-owned) -> a **chief-architect reconciliation** (verified `composes: true`, ZERO by-construction oracles) -> **5 RATIFIED governance docs**. LANDED: `docs/adr/REVOCATION-RECORD-2026-06-21-FLAIR-Heresy-Purge.md` (the decree -- HARD-REVOKE HER-02 by-construction grading; FORMAL-REVOKE OD-4 seafoam->teal, the `preview.webp` render source, the seafoam `canon` hard-gate; AMEND the unwired `win31-decomp`, the unbuilt-but-documented SSIM, the mono-heritage ATKINSON, the dead event loop); **`ADR-0004-AMENDMENT-DEC-09`** (P1 mechanism/policy split -- binding C-8: the mechanism names palette INDICES, ZERO color literal; the 5 drifted `index->RGB` switches collapse onto ONE `flair_canon_rgb`; era-layering D-9; OD-4 seafoam->teal `#8DDCDC`); **`ADR-0005-AMENDMENT-AM-1`** (ATKINSON = the SINGLE dual-heritage spine -- a GDI/HRGN `CombineRgn` peer family rides the SAME `region_op` as QuickDraw's; the `RectInRgn` containment->overlap deep-bug fix; homomorphism oracle untouched); **`ADR-0010-FLAIR-Grading-and-Goldens`** (grade against an INDEPENDENT decomp golden, NEVER by construction -- one `test-color-canon` 4-leg oracle, `ppm_flair_check` re-key, `WIN31_DECOMP` wired, SSIM honestly deferred); **`ADR-0006-FLAIR-Live-Event-Loop-and-Behavioural-Grading`** (the booted-OS `WaitNextEvent` loop behind `-DBOOT_FLAIR_LIVE`, ISR-fed ring, behavioural grading by KIND). **THE INITECH COLOR CANON IS NOW LOCKED DATA: `spec/assets/color_canon.json`** (idx2 desktop teal `#8DDCDC`; white `#FFFFFF`; pinstripe `#F3F3F3`/`#969696`; navy `#000080`; BTNFACE `#C0C0C0`; lavender bevel->teal `#8DDCDC`/`#4E9BA3`; each entry graded vs its decomp golden + a wctb part<->index crosswalk; `preview.webp` demoted to provenance-only). Doc-drift purged: CLAUDE.md (SSIM unbuilt + a by-construction-is-not-an-oracle callout under Law 2), PRD (seafoam->teal headline + SSIM-planned notes), this HANDOFF, ADR-0004 OD-4 (in-place supersede pointer). **7-STEP ROADMAP (`bd show initech-qipc`):** `initech-n79q` region spine (color-independent, FIRST) -> `h714` canon generator -> `mwpw` `test-color-canon` value oracle GREEN (ORACLE-FIRST) -> `7x9k` collapse 5 switches + flip to teal (Rule-8) + `ppm_flair_check` re-key -> `6bq2` mech/policy oracles (C-8) -> `m6qx` era registry (`flair_skins.h`) -> `5l5z` live event loop. **re30.4 is reframed as steps 2+4** (a policy/mechanism cut from the decomp canon, NOT a dedup). **NEXT: implement `initech-n79q` (the region dual-heritage spine, lands first per the critic's sequence), then the oracle-first canon chain.** See WL-0054. **PRIOR (2026-06-21, WL-0053):** **THE INITECH COLOR CANON RATIFIED (operator); FLAIR palette source corrected to the decomp goldens. A DECISION shard -- re30.4 implements it NEXT session.** After re30.3 shipped the live desktop, the operator inspected it and ruled the PALETTE WAS "VERY WRONG": the FLAIR chrome colors in `spec/assets/palette.h` are MEASURED SAMPLES from the low-res `preview.webp` Office Space MOCK-UP (a dim CRT photo), so window "white" was a muddy `#7F7F86`, pinstripe dark, menu bar muddy -- windows rendered gray, not crisp Mac white. **The canon is the painstakingly-built sister-repo goldens/specs (`../system7-decomp`, `../win31-decomp`); FLAIR grades against THEM, not preview.webp.** (A committee had just ruled to unify onto the wrong `palette.h` source; the operator -- the human Law-4 judge -- OVERRODE it.) **THE INITECH COLOR (DEFINITIVE):** the signature "Initech teal" = VIC-20 cyan, hardware colour 3 -- **SCREEN/phosphor `#8DDCDC`** (operator motion read; alt VIC decodes `#85D4DC` Lospec / `#87D6DD` azulianblue), **PIGMENT `#57B19F` for PRINTED PROPS ONLY** (box/manuals/labels; the CRT shader carries pigment toward the screen cyan). **PALETTE RULE: System 7 (later System 8/Platinum) colours are GOLDEN, EXCEPT (1) replace ALL System-7 LAVENDER tinge (`#DADAFF`/`#CCCCFF`/`#B3B3DA`/`#8787B3`/`#333366` -- the 3D bevel/groove accents) with Initech teal `#8DDCDC` (teal-light `#8DDCDC` + darkened-teal shadow ~`#4E9BA3` to keep the 3D), AND (2) DESKTOP BACKGROUND = `#8DDCDC`** (this SUPERSEDES ADR-0004 OD-4 seafoam `#6FA08E`); **use Win 3.1 colours for the additional Windows/Photoshop chrome bits** (BTNFACE `#C0C0C0`, navy `#000080`, ...). The teal endows the OS with its character. **OPERATOR-APPROVED per-index map** (WL-0053 table): idx0 `#000000`, idx1 window-white `#7F7F86`->`#FFFFFF`, idx2 desktop `#6FA08E`->`#8DDCDC`, idx3 menubar `#67696C`->`#FFFFFF`, idx4 ink `#525A63`->`#000000`, idx5 accent `#1E2F87`->`#000080` (Win navy; drives the 116% pie), idx6 control `#BFBFBF`->`#C0C0C0`, idx7 pinstripe-light `#6B6B74`->`#F3F3F3`, idx8 pinstripe-dark `#8A8A93`->`#969696`. **Recorded: `bd memories initech-color` (`initech-color-canon-...-definitive`) + bead initech-hfeg (P1).** **NEXT SESSION = re30.4 RE-ORIENTED:** re-value the palette to the canon + swap chrome lavender->teal + re-point the single palette module to the decomp canon (NOT preview.webp) + re-calibrate the oracles (`ppm_flair_check`, test-chrome/control/dialog, and `test-palette-seafoam` -> assert teal `#8DDCDC`) + amend ADR-0004 OD-4 -> then a fresh 386 screendump for the operator's Law-4 verdict. The committee re30.4-palette-source ruling is SUPERSEDED. **LESSON (bd memory):** for any FLAIR color question the authority is the decomp goldens (rendered-golden pixels = Law 2 truth), NEVER the preview.webp mock-up samples. Prior: WL-0052 -- **re30.3 (M3.1) COMPLETE: the FLAIR chimera desktop renders LIVE on the 386 -- it IS the Office Space frame.** Commit **1bd47c2**: kmain `-DBOOT_FLAIR_SHELL` gives the FLAIR library (linked-but-callless since re30.2) its first call site -- after FLAIR-HEAP-OK it `flair_heap_init`s the 4 MiB window, allocates the ENTIRE scene + regions + a 640x480 indexed-8 offscreen FROM THE HEAP (not BSS -- runway 5dr8), builds the test_shell.c scene (2 windows + System-7 bar + Photoshop chimera bar + modal FILE COPY), `shell_render`s into the offscreen, and PRESENTS to the live LFB (8bpp DAC+copy / 24-32bpp `flair_palette_rgb` per pixel via a new shared palette.h inline), emits `FLAIR-DESKTOP`, halts. Orchestrator VISUALLY CONFIRMED the QEMU screendump is the frame (Law 4: seafoam + two stacked menu bars + two pinstripe windows + the "Saving tables to disk..." modal occluding them). Mutation-proven oracle `tools/ppm_flair_check.c` + `test-flair-desktop` (+ `-mutant`: 3 SHELL_MUTATE_* legs RED) wired into TEST_EMU_GATES; `test-flair-desktop-bochs` serial leg PASS. **254 host + 43 emu GREEN; Bochs PASS.** `_kernel_end=0x34bc0` (45 KiB under the 0x40000 ASSERT). **COMMITTEE (re30.3-chair, no gridlock)** ruled the display-mode strategy of record DUAL-DEPTH (request priority 32->24->8/VESA-0x101->fail-loud floor; indexed-8 OD-2 co-canonical + period-authentic; AM-6 host-model-per-mode, NOT a QEMU pin); the 8bpp DAC present path is correct-but-UNREACHED (stage2 requests only 32/24bpp -> Bochs's 320x200 mode-0x13 is rejected by the 640x480 guard) and is KEPT with a "do NOT delete" comment; M3.1 closes GREEN on the QEMU truecolor leg + Bochs boot-accuracy serial. **Follow-ups filed: initech-2gva (P1 -- stage2 0x101 fallback + an oracle leg that REACHES the 8bpp wrapper, before M4/86Box), initech-6rim (P2 -- record the strategy as an ADR-0004 amendment), initech-fgs1 (P3 -- operator note: 486+/period-SVGA minimum under consideration; PRD Forward Note + commit 38ee42c).** **re30 epic now 3/9** (B0+M3.0+M3.1 -- the critical-path arc DONE). **NEXT (re30 remaining, P2, off critical path):** the verb-track re30.4 (palette module) -> re30.5 (GrafPort verbs) -> re30.6 (flip Managers) -> re30.7 (CopyBits/GWorld); the input arc re30.8 (tick) -> re30.9 (keyboard); the live desktop coexisting with the REPL needs the event loop (re30.8+) + App Contract (epic 4e35). The MILTON redirection arc (bsy.7/.8/.9) + initech-2gva (P1) remain in parallel. Prior: WL-0051 -- **re30.2 COMPLETE: FLAIR now boots INSIDE the kernel image (linked, not yet drawn).** Two bisectable commits, green + pushed: **7c4daff (Step A)** raised PROGRAM_BASE 0x38000->0x40000 -- the pre-authorized +0x8000 whole-map shift mirroring o0td EXACTLY (kernel window 160->192 KiB; SAMIR arena disjointness preserved; 36 files; 4 adversarial verifiers + a kernel.ld-ASSERT mutation all PASS). This SPENT THE LAST conventional free gap -- the kernel stack [0x90000,0xA0000) now butts the 0xA0000 VGA aperture with ZERO slack, so 0x40000 is the MAXIMUM raise under the conventional scheme; **follow-up initech-5dr8 filed** (next growth = high-half/ext-mem relocation). **95cc6df (Step B)** linked all 12 FLAIR Managers (region/heap/event/window/blitter/chrome/text/menu/control/dialog/desktop/flair_shell; surface already linked) into KERNEL_OBJS+KERNEL_SHELL_OBJS only (NO call sites) -- the linker question ANSWERED: 61 symbols, ZERO unresolved/duplicate, no stubs; **B0 ASSERT HELD** (_kernel_end=0x3bad0 < 0x40000, ~17.4 KiB margin); geometry KERNEL_SECTORS 224->320 (+ stage2 equate) + IMG_SECTORS 256->384. **254 host + 41 emu + Bochs GREEN** throughout (the IMG geometry change = a Rule-5 tri-emulator obligation, re-verified on Bochs at the new 320-sector multi-cylinder boot). Orchestrated: each step delegated to an opus subagent; orchestrator owned grading (full clean aggregate + Bochs + adversarial workflow) + per-step commit; committee NOT re-convened (decision already ratified WL-0050). **NEXT = initech-re30.3 (M3.1) -- THE milestone:** static live desktop via shell_render to the LFB in kmain (existing cfill path, NOT the verb refactor) + a new test-flair-desktop screendump gate (QEMU ppm_flair_check + Bochs serial). The MILTON redirection arc (bsy.7/.8/.9) remains open in parallel. Prior: WL-0049+WL-0050 -- **FLAIR CONSOLIDATION + EXPANSION begun.** Operator opened the FLAIR build-out (north star: a period-authentic GUI that hosts all serious era apps as **Initech-versions**). A 7-lane understand workflow + critic mapped the state: **FLAIR is host-green but does NOT boot** -- only `surface.o` links into the kernel; the booted OS still shows seafoam + a console banner; the locked imaging spec is partly aspirational (no draw verbs). Authored **`docs/plans/FLAIR-implementation-plan.md`** (7 phases, dependency-ordered, era-layered **Sys7-now / Sys8-Platinum-accreted-later**; ratified forks: Doom = flat-32 source port [NO DPMI -- already 32-bit flat], Minecraft = native "Initech Mines", M5 frame apps before the hosting arc, canonical suite **InitechBase -> Initech 123 (Lotus) -> InitechWord (WordPerfect)**). **Phase 0/1 LANDED (commit fe4d9b1; 244 -> 254 host gates):** the era-layered canonical spec -- `spec/{control,menu,dialog}_record.h` + `drawing_ops.h` + the 256-entry `assets/clut.{json,h}` + ingested chrome RGBs (out of golden_resolves) + the **`#dfdfdf`->`#ffffff` Law-3 fix** + `CANON-MANIFEST.md` (88 corpus specs mapped) + `win95ism_guardrails.md`; docs reconciled (MZ ships per DEC-08a; M6 = dBASE III+ per ADR-0008); 5 new gates + mutants. **Phase 3 committee** (2 opus seats, chair-synthesized, no gridlock) ruled **GROW** (the GrafPort verb layer + one palette module from clut.h) on a **bisectable boot sequence** (M3.1 ships on the existing cfill path; the verb refactor lands parallel). **B0 (commit 50c1374):** a mutation-proven **kernel.ld fail-loud `ASSERT(_kernel_end < 0x38000)`** + corrected the stale `memory_map.h` headroom comment -- and FOUND the **BOOTING shell kernel has only ~29 KiB headroom (`_kernel_end=0x30920`) < FLAIR's ~51 KiB**, so M3.0 needs the **pre-authorized +0x8000 PROGRAM_BASE raise**. Both commits pushed to origin/command-com-default. **NEXT = `re30.2` M3.0** (the +0x8000 raise, o0td-style, preserve SAMIR's arena, FULL emu+Bochs grade) -> **`re30.3` M3.1** (static live desktop via shell_render + a new `test-flair-desktop` screendump gate). The MILTON redirection arc (bsy.7/.8/.9) remains open in parallel. **Prior: WL-0046..0048 -- THE KERNEL-GROWTH / REDIRECTION ARC: o0td** (PROGRAM_BASE 0x30000->0x38000, whole-map shift +0x8000 = **+32 KiB kernel window**; SAMIR's heap preserved byte-identical) -> **k36g** (INT 21h AH=09h/02h/06h CON output now routes through the **redirectable JFT handle 1**) -> **hsct OUTPUT redirect** (`echo HELLO > file` + `>>` append REALLY redirect on the emulated 386). 3 commits pushed (bae46d4 / 7de96e6 / 24194ed); **240->244 host + 39->41 emu + Bochs GREEN**. The committee disproved the prescribed o0td fix (SAMIR .bss measured 47.3 KiB not the documented 26 KiB) and the orchestrator amended the chair's ruling (kstart ESP). Next: **bsy.9** (loader child-PSP-inherits-parent-JFT -> external EXEC redirect), **bsy.7** (`<` stdin), **bsy.8** (`|` pipe). Prior: WL-0040..0045 -- DOS-3.3 PARITY: 5-WAVE ORCHESTRATED PUSH TO THE CAPSTONE. **224 -> 240 host gates; full emu (39) + Bochs GREEN throughout; 5 commits pushed (9a5fea3..4e60bd5).** Wave 1 xw1(.BAT/AUTOEXEC) + bo40(AH=31h KEEP) + x3mh(ANSI FSM); Wave 2 mvg(INT 24h critical-error) + 509.7(device chain); Wave 3 6zd9(device chain->INT 21h OPEN-by-name); Wave 4 p96i(ANSI CON wiring) + the FAT-cache share [a committee ratified raising PROGRAM_BASE; orchestrator grading (Law 4) caught that the raise BROKE SAMIR's heap arena -> reverted -> the committee's own option B instead]; **Wave 5 CAPSTONE 40oq -- the Appendix-A INT 21h functional-coverage CERTIFICATE is GREEN** (55 dispatched, 0 unwaived gaps; AH=03/04/05 AUX/PRN gap closed; FCB-waived; partitions+multivol deferred). Beads closed: xw1/bo40/mvg/509.7/6zd9/p96i/x3mh; 40oq functional-certified (stays open on kzfs/slvd). Wave 6 hsct(I/O redirection) ATTEMPTED + REVERTED -- found 2 blockers (AH=09h not redirectable; kernel window exhausted). **THE KERNEL WINDOW [0x10000,0x30000) IS FULL -- o0td (P1) is now the gating prereq for further kernel growth.** Prior: WL-0038 (Tranches E/F/G + MZ .EXE end-to-end); FLAIR PAUSED.)

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

> **CURRENT STATE (2026-07-10, WL-0069 -- THE WHOLE-REPOSITORY AUDIT + IMPROVEMENT PLAN.
> The programme reviewed end to end; the tracker reconciled; a 4-phase plan ratified.)**
>
> A 12-lane parallel survey (boot/kernel, milton-dos, flair, atkinson, samir, seed/tps,
> harness-oracles, spec/fixtures, worklog/handoff, beads, build-repro + a first-person docs
> pass) + an adversarial completeness critic reviewed the entire repo; EVERY load-bearing
> claim was re-verified FIRST-PERSON (Rule 4), including the certificate itself:
> `make clean && make test` = **ALL GREEN 296 host + 61 emu (2026-07-09)**. No artifact code
> changed; this was an audit + reconciliation session.
>
> **THE PLAN (docs/plans/AUDIT-2026-07-09-improvement-plan.md; epic `initech-croo`;
> label `audit-2026-07`):**
> - **Phase 1 -- gate integrity (ONE session, do FIRST):** `6vr1` (SEVEN KERNEL_*_OBJ rules
>   have EMPTY prereq lists -- `make -rpn` shows build/region.o does not depend on region.c;
>   incremental builds relink STALE kernels) -> `ljck` (NINE mutation-proven gates orphaned
>   from `make test`, incl. the only jf8p regression guard) -> `gymo` (the untracked
>   stdfile WIP is FINISHED + verified green w/ biting mutants; wire test-stdfile + commit)
>   -> `7s1z` (make image/run are M1 stubs) -> `c921` (worktree hygiene: .gitignore entry +
>   prune the stale agent-adeddefb35566901a worktree). Exit = the authoritative clean gate
>   at the new larger count.
> - **Phase 2 -- oracle strengthening:** `xi7x` (region generators never sample negative
>   coords -- the live desktop produces them on every top-left drag), `0k2d` (6/7
>   prog-diff goldens have no proven mutant -- the bljs class), `tf3c` (seed mutant gates +
>   negative div/mod fixture), `quke` (softfp/ansi oracle audit), `in2g` (promote
>   test-boot-bochs into `make test`; 86Box stays `x0i`).
> - **Phase 3 -- product arcs:** the GUI tail (`rl4v` cross-menu drag, `zvo6`+`a90f` FILE
>   COPY fidelity, `44ab` region cap decision, `hv7u`, `jh7m`) + the MILTON redirection arc
>   in TRUE dependency order: **bsy.9 FIRST** (psp.c:141 hardcodes the child JFT -- it
>   structurally blocks the rest) -> bsy.7 (`<`) -> bsy.8 (`|`) -> `hsct` closes. Dep chain
>   registered in bd. SAMIR parity: `h5vg` (.ndx 2-level B-tree ceiling), `cq0j`.
> - **Phase 4 -- the North Star:** `qvtj` (size the seed->Turbo-Initech gap into beads under
>   `rnh`), `79s` (ratify ADR-0007), `lk29` (ADR-0001/0002 cited-but-absent), `586.4` (the
>   real-DBASE.EXE differential -- the PRD's stated primary SAMIR oracle, today loud-skipped
>   everywhere), `dam` (P0 -- frame stills into fixtures/, gates the whole Law-4 SSIM axis).
> - **Docs track:** `jk8t` (HANDOFF restructure -- Sec 6/7 still describe WL-0029 as
>   "latest"), `msol` (6 verified stale-prose sites incl. CLAUDE.md CC_KERNEL->KERNEL_CC and
>   the phantom box86.c), `2h6j` (chimera_element_map.json has zero consumers).
>
> **BEADS RECONCILED THIS SESSION:** closed `pipa`/`ax9.2`/`5l5z` (verifiably shipped);
> reverted `bea`/`40oq`/`hdlb`/`hsct`/`jh7m`/`re30.4` in_progress->open with evidence notes;
> claimed `gymo`; widened `6vr1` (P2->P1, all seven objects); filed 15 new issues. NOT
> touched (operator call): the open M0-M2 P0 milestone epics (`njv`/`2rb`/`509`/`f8v`) --
> recommend an operator roll-up review.
>
> **STANDING LESSON (bd memories audit-trap-2026-07-09):** a persisted shell `cd` into a
> stale `.claude/worktrees/` checkout silently substituted a WL-0032-era Makefile mid-audit;
> verify pwd (or use absolute paths) before trusting repo-state reads.
>
> **NEXT: execute Phase 1** (`6vr1` -> `ljck` -> `gymo` -> `7s1z` -> `c921`), close with
> `make clean && make test`. See WL-0069 + the plan doc.
>
> ---
>
> **PRIOR STATE (2026-07-07, WL-0068 -- THE SAMIR/xBASE P1 CORRECTNESS CLUSTER CLEARED.
> The dBASE engine is hardened; a Law-2 heresy purged in a second location.)**
>
> 12 P1 correctness bugs fixed + committed + each mutation-proven on `command-com-default`
> (`6ed6db9..bc98821`, 15 commits): **query** -- `3p9e` (the ?/?? logical .T./.F. ORACLE HERESY:
> ? echoes DOTTED, LIST columns keep bare T/F; the repl oracle was de-heresied), `ue7y` (SET
> DATE/CENTURY), `wcb7`+`qw4e` (LOCATE RECORD/NEXT n scope bounds); **ndx/PACK** -- `0g22`
> (ndx_delete_key walks the equal-key run across leaves), `x87g` (PACK FULL REINDEX of open NDX);
> **fn** -- `9u0f` (CTOD blanks calendar-invalid dates), `eyig` (ROUND floors negatives);
> **dbf** -- `jf8p` (+2-form header_length normalized ON FLUSH); **nav** -- `dmrw` (SKIP -1 from
> EOF); **seed** -- `2hg9` (scan_string pool-wrap, FACTORY); **MILTON** -- `winh` (dos_read CF ->
> TYPE/COPY/batch break on read error). `make clean && make test` = ALL GREEN (296 host + 61 emu).
>
> **HOW IT WAS DONE (reuse the pattern):** 12 bugs -> 7 CONFLICT-FREE worktree lanes by file
> ownership (only the Makefile a shared merge surface); 2 opus + 5 sonnet in parallel; each lane
> Red->Green->mutation with an INDEPENDENT oracle grounded in `../dbase3-decomp`; the orchestrator
> integrated LANE-BY-LANE, re-ran EVERY subsystem oracle ITSELF on the main tree (never a lane's
> self-report), then the authoritative `make clean`. Both OPUS lanes hit the opus weekly rate
> limit mid-flight -> per-bug commits preserved on-branch, SONNET finishers completed the remainder
> in the same worktrees. **THE STANDING LESSON:** the two worst bugs were INTEGRATION traps that
> NO single lane's green oracle could see, only the composed main tree + `make clean` -- (1) the
> `bljs` golden heresy (merging 3p9e's correct fix turned `test-dbase-diff` RED 4/8; the
> xbase_prog_diff goldens carried the SAME bare-T/F heresy in a second location; corrected the
> ?-echo lines to .T./.F. + re-pointed the SWAP_EQ mutant -- Lane C ran test-dbase GREEN in its OWN
> worktree only because it lacked A's fix); (2) `winh` x `ojxn` -- winh's read-error abort became a
> second data-loss backstop that masked the WL-0067 copy-self mutant, now fixed by disabling BOTH
> backstops in that mutant. Integrate lane-by-lane and re-grade the COMPOSED tree; never trust a
> per-lane green.
>
> **NEXT:** unchanged from WL-0067 -- the P2 tail of the `gui-bughunt` label + the filed GUI
> follow-ups (`44ab` region cap, `hv7u`/`zn61`/`vj28`, `0i1m`, `7tjp`, the Makefile
> KERNEL_DESKTOP_OBJ build bug, a permanent qi8v drag oracle), THEN the canonical app suite
> (Initech123/InitechWord) on the now-solid GUI + hardened SAMIR. NEW FOLLOW-UPS from this arc:
> `cq0j` (P3 -- LOCATE NEXT n EOF()/RECNO() bounded-miss oracle vs real III+, a
> navigation-query-display.md [oracle-resolves] edge) and `bljs` (P1 -- CLOSED as fixed, but the
> golden edit invites post-hoc committee ratification). See WL-0068.
>
> ---
>
> **PRIOR STATE (2026-07-07, WL-0067 -- THE FLAIR GUI FIX ARC COMPLETE. The live
> desktop is the frame again; the WL-0066 fix mandate is DISCHARGED.)**
>
> All 7 targeted bugs are FIXED + committed + each mutation-proven, plus a test-isolation
> follow-through (`1ff449f..HEAD` on command-com-default; 8 commits): **2 P0** -- `mswo`
> (region xmerge stack-overflow -> output-bounded + fail-loud) + `ojxn` (COPY-onto-itself
> data-loss -> src==dst guard + EMU no-data-loss gate); **5 P1 compositor/chrome** -- `v6t2`
> (deactivation repaint), `a9iq` (inactive-window chrome, flat white per the decomp
> title-bar.md), `4w15` (static System-7 shell bar + distinct-chimera boot), `pipa`
> (bars+modal survive a drag: the `wm->overlay_rgn` fold into `fronts_union`; EMU `dc4v`
> gate), `jmc5`+`qi8v` (stale-`desktop_update` teal-erasure -> `desktop_paint_all` clears it;
> appswitch TIER-C). `make clean && make test` = ALL GREEN (291 host + 59 emu). The live
> desktop now composites CORRECTLY through drag, app-switch, and modal-occlusion -- the
> post-drag screendump IS the Office Space "Saving tables to disk..." still (EYEBALLED).
>
> **HOW IT WAS DONE (reuse the pattern):** orchestrated -- a 3-seat committee + adversarial
> red-team for the compositor architecture (a re-test re-scoped qi8v/jmc5 from "the
> architecture" to a 1-liner; the red-team ratified the CONTAINED overlay fold over the heavy
> uniform-window model), parallel worktree lanes, the orchestrator re-running EVERY oracle on
> the main tree + Law-4 EYEBALLING -- which caught 4w15's two-identical-bars boot regression
> AND a TIER-C mis-calibration (it probed HELLO, but on the integrated scene v6t2 repaints
> HELLO and the ERASED window is NOTES) before either shipped. THE STANDING GUI TEST METHOD
> is unchanged -- see "THE TESTING METHOD" in the WL-0066 block just below (still current).
>
> **NEXT (no longer a P0/P1 emergency):** resume the P2 tail of the `gui-bughunt` label + the
> filed follow-ups -- `44ab` (region RGN_ROW_X_MAX cap decision, maybe committee), `hv7u`
> (full inactive-chrome fidelity: gray frame + dim title + absent gadgets), `zn61` (true modal
> input-blocking / FindWindow hit-test), `vj28` (COPY same-file edge cases), `0i1m` (region
> from_rects O(n^2)), `7tjp` (tenants band-2 Apple-slot dup), the Makefile KERNEL_DESKTOP_OBJ
> build bug, a permanent qi8v drag oracle. THEN the canonical app suite (Initech123/
> InitechWord) ON the now-solid GUI. See WL-0067.
>
> ---
>
> **PRIOR STATE (2026-06-28, WL-0066 -- THE LIVE-GUI BUG HUNT. The origin of the fix arc
> above; the standing GUI test method it documents is STILL CURRENT.) The live FLAIR desktop
> was badly broken; 138 real bugs were filed.**
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
