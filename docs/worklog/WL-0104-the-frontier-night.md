# WL-0104 -- The frontier night: fifteen lanes merged, one audit, one headroom recovery

## Context

Session of 2026-10-15, 00:00-05:10 AEDT, run by the agent-PM. Operator
directive: move the frontier -- bring InitechOS closer to a fully fledged OS
rather than polishing the already-green. Lanes: see HANDOFF Sec 5. Base: 1c93dde, certified ALL GREEN
395 host + 203 emu at 00:55. Head at the time of this shard: 4ab5a53 (see CERT below).

Pacing: the standing rules of the 2026-10-04 directive applied (up to 5 points
over pace on Claude and Codex; Opus codes, Sonnet does busywork, no Fable
subagents, Codex gpt-6.1-sol for computer use, two to three agents at a time;
`bd memories operator-directives-2026-10-04`). Three sessions shared the one
host, so load and quota were synced with the peer sessions. Per-night pacing
figures beyond that directive: not recorded here.

## What changed (merge order)

Fifteen lanes merged. One Codex LOOK audit ran alongside.

decaab3 -- initech-fvnj (audit L008). Initech 123 asks before discarding
unsaved work: File > Quit / Ctrl-Q, the close box (goAway=2) and the real R2.2
/Quit No/Yes all pass one exit guard; new Toolbox Gate call TBX_ALERT 0x0090.
Gate: test-flair-i123-quitguard (nine locked legs) + mutants.

9cc1848 -- initech-tdnl.78. Special > Restart and Shut Down work: kernel reset
(8042 0xFE, fail-loud) and halt, every window's view saved and DESKTOP.DB
committed first, a running disk application asked to quit through its own File
> Quit (a declined quit cancels the command), the period shut-down screen
(System 7 DSAT 2 alert) with a live Restart button. Harness gains
--allow-reboot and --quit-after-count. Gates: test-flair-shutdown,
test-flair-restart + mutants.

3aaf6e0 -- initech-j1lu (K12). DOS control chords: both Ctrl keys and E0
scancodes decode to control letters, Ctrl-Z ends a cooked CON read, Ctrl-C with
BREAK aborts through INT 23h and unwinds to the prompt. Gates: the K12 host
and emu set incl. test-j1lu-control-emu-mutant. Detail in WL-0102.

5695859 -- initech-iv8c + initech-ehz4. Missing reference corpora and tools
fail loud with named opt-outs (SKIP_SYSTEM7_DECOMP, SKIP_WIN31_DECOMP,
SKIP_DBASE3_DECOMP, SKIP_LOTUS123_DECOMP, SKIP_REFERENCE_TOOLS);
dbf_ref.py and ndx_ref.py honour DBASE3_DECOMP so test-samir is green in
worktrees. Gate: test-gate-honesty + mutant. Detail in WL-0103.

db4fd74 -- initech-opms. Kernel size policy round 2: FLAIRTENANTS-only objects
-O1 -> -Os (--param=min-pagesize=0 for the 0x500 boot_info read) and 15
pure-logic shared objects -O0 -> -Os -fno-strict-aliasing. Binding headroom
4,696 -> 24,888 B; boot geometry, MBR, stage2 and the 4,096 B floor untouched;
133-kernel clean double build identical. Design:
docs/design/kernel-size-policy-round2.md.

f50519e -- initech-94ah. Initech 123 formula engine: R2.2 compiler/decompiler
byte-exact to the .WK1 FORMULA RPN (1090/1090 corpus formulas), x87 evaluator,
90 named @functions, natural/columnwise/rowwise recalc with iteration and CIRC;
986 golden cached values reproduced bit for bit. Gates: test-i123-formula,
test-i123-recalc-order, test-i123-wk1-roundtrip, test-flair-i123-formula.

f7ed504 -- initech-uios (audit L002). Empty Trash retires every window of a
purged folder at any depth through DisposeWindow; a stale or recycled
directory identity is refused at the FAT mutation boundary
(FAT12_ERR_STALE_DIR). Gate: test-flair-trash-purge-views (mtools and
fsck.fat judge the disk).

(c5ac936: trash_purge_views record settle 300 -> 500 ms.)
397ce01 -- initech-l4tj. InitechWord P1: piece-table document core and .WP5
codec (all five real WordPerfect 5.1 goldens round-trip byte-identical; typed
MINT02 equals the real file), WORD.EXE Face A as a disk application (WP 5.1
editing screen, F10 Save / Shift-F10 Retrieve, F7 Exit through one guard).
Gates: test-iword-core, -wp5-roundtrip, -face, test-flair-iword + mutants;
WORDPERFECT51_DECOMP is a fail-loud corpus root.

3c0df0a -- initech-mfrb. The iword_typing clip outlived the record image's
6000-tick life (0.742 s per event x 99 events). No keystroke was dropped; the
harness now fails loud on events injected after the guest's end marker
(--record-end-marker, record_events_after_end), and a 12000-tick
RECORDXLONGDBL image variant carries the clip. Defaults unchanged.

7bec622 -- initech-tdnl.68. Font Manager slice 2: bold (1 px smear, advance
+1; pixel-exact against nine real Geneva 9 bold labels), italic (1:5 shear
measured from NFNT 6475 vs 5415), underline, styled advances, region-clipped
text runs for titles, menus, dialogs and controls, pixel-wrapped TextEdit,
truncated titles ending in Chicago 0xC9. chrome_metrics marker re-keyed
'.' -> 0xC9 (Rule 8, dated). Gates: test-font-styles, test-text-clip,
test-flair-fonts + mutants.

fac1326 -- initech-tdnl.96/.97/.101/.82/.86. Scroll bars graded against six
independent Mac OS 8 crops in both drawers at 8/32 bpp; ranges settled before
chrome paints a grown, zoomed or restored view; grow clamped to a bar-fit
96x88 minimum. Gates: test-scrollbar-look, test-flair-scroll-look + mutants.
Detail in WL-0107.

882ac7d -- initech-uqah. Initech 123 P3: the full R2.2 slash menu tree from
123.RI (82 levels, 381 entries), /Range Format/Label/Erase/Name/Protect/
Value/Trans, /Copy and /Move with the R2.2 reference-adjustment rules (model
property test, 996,023 checks; 12 real-sheet self-transfers). Gates:
test-i123-menu-tree, -copy-move-model, -range-menu, test-flair-i123-menu +
mutants. test-i123-copy-move is RED by design (no /Copy or /Move mint
goldens, initech-cjot) and sits outside TEST_UNIT_GATES. Detail in WL-0105.
Follow-up commits b50708d (the stale-menu gate volume passes the WORD.EXE
argument; test-makefile-vars) and de2ac6a (see Frictions).

825fefb (a fix, not a merge) -- the Rule-5 I123 smoke test-flair-i123-bochs
answers the quit guard with Don't Save, then reaps to an empty process list.

9cc9852 -- initech-rtdw/3sxp/vjtp/d31e/b0qk/irp2/veub. The DOS console looks
like DOS 3.3: PIT-driven blinking underline cursor, CLS homes to row 0, DIR in
the manual's format (volume line, NAME EXT SIZE M-DD-YY HH:MMa, n File(s) nnnnn
bytes free from the FAT, mdir-verified). MSG-DOS-0021..0026 added to the locked
catalogue (ADR-0003 App C amendment). Gates: test-console-look, test-dir-look,
test-dir-free-space, test-dir-volume, test-shell-dir-look + mutants. WL-0106.

1aff1ef -- initech-tdnl.93/.94/.95/.70. Menus graded against the Mac OS 8
capture: selected title in a dark teal face with distinct edges, origins and
pads derived from the capture (Apple slot 33, title pad 9, item LPAD 19,
shortcut clearance 14), the NFNT 5478 checkMark 0x12 for View; chrome_metrics
and menu goldens re-keyed by derivation (dated). Gates: test-menu-look,
test-flair-menu-look + mutants. WL-0108.

The LOOK audit. A Codex gpt-6.1-sol pass (docs/audits/2026-10-15-look-pass6/,
images from 3aaf6e0) measured the InitechOS look against the Mac OS 8 / System
7, 1-2-3, dBASE and DOS 3.3 references and filed 28 new nits (M001-M028).
Triage (6e804e6) mapped them: 22 new beads under label audit-2026-10-look6,
20 existing beads annotated, five pickup batches (scroll bars, window chrome
and controls, menus, DOS console, database screen / film scene / 123 status).
Batches 1, 3 and 4 were consumed tonight by lane-scroll, lane-menus and
lane-doslook.

Kernel headroom story. At the base the binding headroom (largest mutant
kernel) was 4,696 B against the 4,096 B floor, after tdnl.78 and fvnj. opms
levers A and B took it to 13,752 and then 24,888 B. The look and feature lanes
then spent it: 22,616 after doslook (WL-0106), about 21,208 B binding after
menus (WL-0108; flagship 22,328 B). Net: roughly 21 KB binding, enough for the
next several kernel-side features before the bounce-policy redesign (lever C,
parked) is forced.

Rule-14 clips repro-proven on main (two captures byte-identical each), the
eight tonight: shutdown, i123_quit_guard, i123_formula, trash_purge_views,
iword_typing, fonts_styles, scroll_look, i123_menu. All in build/clips/.

281c0cf -- initech-1y0h + r3pa + 3cz2 + vz6z (Codex lane-dbscreen). The SAMIR
screen per the dBASE III PLUS manual: numeric cells render from their N(w,d)
descriptors (d decimals, right-aligned), SET HEADING ON/OFF with the field-
heading row, program-only SET CONSOLE, SET STATUS ON with the reserved status
band (Fig. 5-17). Manual-derived, no real LIST/STATUS golden exists. Gates:
test-samir-numeric-format, test-samir-status, test-samir-status-emu,
test-samir-console-emu + mutants. Detail in WL-0109. Integration: the oracle
demanded INITECH_QMP_STDIO=1 (b9b041b dropped it) and test-samir-audit K05/K06
were re-keyed to the new layout (3dd9669); the deleted-marker column now
misaligns live and deleted rows by one column (its own bead).

## Why

The north star is a fully fledged period OS: Restart and Shut Down that put
everything away (tdnl.78), a spreadsheet with formulas and the complete menu
tree (94ah, uqah), a word processor (l4tj), working DOS control keys (j1lu),
data safety (fvnj, uios), and the look graded against independent captures,
not the renderer's own constants (Law 2). Gate honesty (iv8c) and kernel
headroom (opms) are the enabling work: a skipped oracle is worse than a red
one, and a kernel at its floor admits no features.

## Frictions

- Fixture name collision (initech-tflp). build/flair_i123_quit_data.img was
  both the quit-guard fixture volume (RO.WK1 +r) and the appl-boot scratch copy
  of the test-flair-i123 quit leg, so the smoke leg overwrote the fixture and
  make called it up to date. The uqah lane renamed the fixture
  (flair_i123_quit_fixture.img, ec74696). Still owed: a guard against name
  collisions and a fixture self-check at the end of the recipe.
- Stale locked-trash purge counts. As the data volume grew (APPS now holds
  CTENANT, 123, WORD and the Trash README) the locked-trash fixture purge
  counts went stale; efce1ae re-keyed them (found by the 7bec622 certificate's
  emu vector).
- The I123 smoke vs the quit guard: the typed row made the sheet dirty so
  Ctrl-Q asked; 825fefb answers the question (found by the same certificate).
- record-flair parse order. RECORD_IMAGE_<script> is read at parse time; a clip
  block defined after the record-flair target never had its image built
  (scroll_look, de2ac6a). Rule: clip blocks sit before the target.
- Record image life vs trace length (initech-mfrb): 6000 ticks at 0.742 s per
  recorded event cannot carry a 99-token trace. Fixed with the end-marker check
  and a wider variant, never a bigger default.
- Load-sensitive emu flakes (initech-7r2): test-flair-i123-formula dropped
  keystrokes once as a mutant prerequisite at host load about 12, and
  test-flair-app-launch once at load 15; both green alone. Re-run alone, not
  loosened.
- Appended-gate-block Makefile conflicts were resolved by keeping both blocks;
  test-makefile-vars caught a read-before-definition after uqah (b50708d).
- The Codex kill-trap memory did not reproduce on codex-cli 0.162.1; not
  relied upon.

## Acceptance

Base certificate: ALL GREEN 395 host + 203 emu at 1c93dde. Second certificate
at 7bec622: host ALL GREEN 416; emu RED on two stale fixtures, fixed in efce1ae
and 825fefb.

CERT: NOT fully certified at close (operator stop, 08:35, Claude over pace). Host vector ALL GREEN 440 gates twice at 29c4323 (the only later changes are two locked-trace / harness re-keys). Emu vector (235 gates): no attempt ran to the end -- each full run found one stale fixture or re-key from the night's volume/menu growth, which was fixed on main, or a load flake (green alone): 1c93dde ALL GREEN 395+203 at 00:55 (base); 7bec622 host 416 green, emu stopped at locked-purge (re-keyed); efce1ae emu 1..167 green then test-flair-i123-bochs (smoke answers the quit guard, 825fefb); 3dd9669 emu 1..170 then file-ui (re-keyed 6ae1205); 6ae1205 emu 1..171 then its mutant (re-keyed 29c4323); 29c4323 emu 1..183 then finder-service leg 2 (re-keyed 4ab5a53); 4ab5a53 emu 1..98 green when stopped. Every lane's family + mutants were run green first-person on main after its merge. FIRST ACTION NEXT SESSION: make clean && make test in ../initech-os-cert at main HEAD, then close the in_progress beads listed in WL-0104.

Per-merge evidence pattern: each lane carries its own gates and mutants; the
PM re-ran the touched families on main after each merge (e.g. 31/31 at
7bec622), because lanes sharing a data disk break each other's locked traces.

## Pointers

Worklogs: WL-0102 (j1lu), WL-0103 (gate honesty), WL-0105 (I123 slash menu),
WL-0106 (DOS console look), WL-0107 (scroll bar look), WL-0108 (menu look).
Audit: docs/audits/2026-10-15-look-pass6/ (REPORT.md Sec 1 and 5, TRIAGE.md).
Design: docs/design/kernel-size-policy-round2.md.

Beads opened tonight: initech-opms, initech-cqej (shutdown cancel leg, Bochs
leg, clip), initech-mfrb, initech-tflp, initech-cjot (/Copy and /Move goldens).
The 22 look beads (label audit-2026-10-look6): tdnl.93 .94 .95 (menus),
tdnl.96 .97 .101 (scroll), tdnl.98 .99 .100 .102 .103 .104 (cursor, buttons,
film scene), rtdw 3sxp vjtp d31e b0qk irp2 veub (DOS console), 3cz2 1y0h r3pa
(database screen).

Follow-ups:
- Bochs legs are owed for every new emu gate (Rule 5): shutdown, restart,
  quitguard, fonts, scroll-look, menu-look, iword, i123 formula/menu, the
  console and DIR gates. cqej also needs reboot-and-wait support in
  harness/emu/bochs.c.
- The operator's Law-4 eyeball of the eight clips above.
- Operator ruling: shut-down screen wording 'computer' (ADR-0012 / DEC-12) vs
  the DSAT 2 'Macintosh'.
- Operator ruling: M014 console colours (the report's black console conflicts
  with initech-r8hv).
- CLAUDE.md Build & test needs the SKIP_* lines proposed by iv8c.
- test-i123-copy-move stays out of the vector until cjot mints the goldens.
- The remaining look batches (window chrome and controls, film scene, icons)
  are unstarted.
