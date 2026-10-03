<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -->

# WL-0093 -- THE DESKTOP GETS DRIVEN BY A STRANGER: TWO HANDS-ON AUDITS, PROPORTIONAL CHICAGO, DRAG-MOVE, SETMBAR, KERNEL X87

**Date:** 2026-10-03 evening to 2026-10-04 (orchestrated session; operator
directives in-session, see Context)
**Beads CLOSED:** initech-mbn1 (corpus specs), initech-cnpm (SETMBAR),
initech-zj6w (kernel x87), initech-yjlo (ADR-0009 numeric ruling),
initech-34dh (drag-move + drag-to-Trash staging), initech-tdnl.33 (Font
Manager slice 1).
**Beads FILED:** 43 -- the two audits (initech-tdnl.34-.55 and .58-.67, label
`gui-audit-2026-10`), plus tdnl.31/.32/.56/.57, d9tt, tgcu, xi0q, bykl, 3f13
and the font follow-ups (see Pointers).
**Certificate:** ALL GREEN 361 host + 123 emu (first-person, `make clean && make test`, 2026-10-04 00:56, on f30e300). The five initech-34dh gates had been left out of the default vectors by the lane; they were run green individually on the same tree and then wired in (the vector is now 363 host + 126 emu and has NOT yet been run as one vector at that size).

## Context
Operator, 2026-10-03: (1) ruled the four queued decisions on the PM's
recommendation -- x87 for Initech 123 with SAMIR unchanged; tdnl.12 stage 2 =
keep HELLO boot-foreground (no DISTINCT-CHIMERA re-key); InitechWord
dictionary = reduced list first, companion volume as the target; Face B gated
on the Font Manager. (2) New orchestration rules: stay under pace on Claude
and Codex per the quota app, coordinate with the peer session, Opus for
coding, Sonnet for busywork, no Fable subagents, Codex (gpt-6.1-sol xhigh)
for computer use, at most 2-3 subagents. (3) "I am still unhappy" with the
window system: it must look really awesome for the 90s and have every feature
a user of the time expected; Codex is to drive the live desktop freely and
find the problems.

## What changed
1. **Corpus specs (mbn1, sonnet).** `../lotus123-decomp` eeccebf and
   `../wordperfect51-decomp` f20e87f now have `specs/`. CORRECTION to
   WL-0092: the Lotus corpus holds 1090 formulas, not 1071.
2. **SETMBAR (cnpm, opus, 1d654c2).** INT 81h 0x0050: a disk tenant supplies
   its own band-2 bar, validated inside its image, restored on exit, close or
   crash. Clip `app_menubar`. Menu choices are not delivered to the tenant yet
   (tdnl.31).
3. **Kernel x87 (zj6w, opus, d365f77).** CR0 EM=0/MP=1/NE=1, FNINIT,
   spec-locked CW 0x037F, probe, FPU-absent path. The lane's red run showed
   the bead's premise was wrong: pre-init x87 did not #NM; Bochs silently
   returned 0 where QEMU returned 63. ADR-0009 Amendment DEC-01a (f33151c)
   reconciles DEC-07.
4. **Drag-move + drag-to-Trash staging (34dh, opus, d96333b).** Gray outline,
   lit drop targets, zoom-back on refusal, real FAT moves graded against
   mtools, kind=5 origin records. Clips `drag_move`, `trash_drag`,
   `drag_refused`. Cost about 7.2 KiB.
5. **Proportional Chicago 12 (tdnl.33 slice 1, opus, 8d668a0..d7c7149).**
   Real NFNT 5478 advances and bearings, hand-authored art, font tables
   linked once (recovered about 9.7 KiB), independent `test-chicago-metrics`
   oracle, every text golden re-derived, tenant TEXTDRAW / SETMBAR width
   rules proportional. PM rulings are in the bead's DESIGN field.
6. **Two hands-on audits (Codex, sandboxed, QMP-driven).**
   `docs/audits/2026-10-03-flair-gui-codex/` (F01-F18) and
   `docs/audits/2026-10-04-flair-gui-codex-pass2/` (G01-G11), each with
   REPORT, screenshots, serial evidence, replay driver and a PM TRIAGE.md.

## Frictions
- The permission classifier refuses `codex exec -s danger-full-access` even
  after an explicit operator greenlight; `-s workspace-write` from a scratch
  directory works and was enough (QEMU `-display none` + QMP).
- Two Bochs runs on one host collide on RFB port 5900 (false red; d9tt).
- `bd dolt push` prints its remote-setup help: no Dolt remote is configured,
  so bead changes are local only. Not investigated.
- Quota: two sessions burned about 5 points of Claude weekly per hour; both
  stopped starting Claude subagents at about 6 points under pace.

## Acceptance
First full vector on the combined tree (450f28f) went RED one minute in: `test-chicago-metrics` read an empty `SYSTEM7_DECOMP` (immediate `:=` before the `?=` default), LOUD-SKIPped, and only its golden-requiring mutant caught it. Fixed in f30e300 (deferred expansion); re-run from clean: ALL GREEN 361 host + 123 emu. Earlier in the session: ALL GREEN 357 host + 123 emu at 1cf74a6 (x87 + SETMBAR). Binding kernel headroom after all four lanes: 29,016 B. Clips are in each lane worktree and were NOT re-recorded on main after the merges; `make record-flair` on main is owed before the operator eyeball.

## Pointers -- NEXT AGENT START HERE
1. **The window system has four open P0s, all with root causes or leads in
   the bead notes:** initech-tdnl.34 (Finder window contents stored in
   absolute screen coordinates), tdnl.35 (scroll bars are drawn chrome only;
   FindWindow returns inDrag), tdnl.58 (Ctrl/Shift latched after release),
   tdnl.59 (track loops bounded to 150 ticks in kmain.c). Fix these before
   any new feature. Then the P1s: tdnl.36/.37/.38/.50/.67 (enabled no-ops,
   one shared cause in finder_cmd.c), tdnl.60 (TrackGoAway), tdnl.31 (menu
   delivery to disk apps), tdnl.61, tdnl.62, tdnl.39/6k12/p6st (Trash,
   Empty Trash, Duplicate/Get Info/rename).
2. **Validation method that works:** have Codex drive the image (briefs and
   drivers are in the two audit directories). Run a pass after each fix arc;
   neither pass reached the pie chart / `570-` scene, real Open/Save dialogs
   or a real application.
3. **Look:** Chicago is proportional now. Still open: Charcoal strike and
   cloverleaf glyph, menu-bar left-edge layout vs Sys 8, window body frame
   weight (tdnl.51), widget shading (tdnl.52), icon art (tdnl.53), txFace
   styles and pixel-wrapped TextEdit (font slice 2).
4. **Unchanged queue behind the GUI:** I123 P1 (initech-9u8w) and IWORD P1
   (initech-l4tj) can now cite the corpus specs; I123 P2 (94ah) owns the
   Initech 123 control-word decision.
