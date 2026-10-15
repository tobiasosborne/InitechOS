# WL-0114 -- The second night: seven lanes merged, two certificates, the oracle bites real Lotus

## Context

Session of 2026-10-15 23:40 to 2026-10-16 ~03:00 AEDT, run by the agent-PM
(Fable 5.1 orchestrating; Opus coding lanes, Sonnet busywork, Codex
gpt-6.1-sol xhigh code lanes). Operator directive 2026-10-15 (bd memory
operator-directives-2026-10-15): orchestrate; monitor Claude and Codex via
the quota app; work up to 10-20% over pace on both; no Fable subagents;
Opus codes, Sonnet does busywork, Codex sol for review/computer use/coding,
astra for hard cognition; 2-3 agents at a time; coordinate with the two
peer sessions; decide open questions, proceed, record.

Base: 00a8e83 (WL-0104 close). First action per HANDOFF: the certificate.

## Peer coordination (three sessions on one 12-core / 30 GB host)

Agreed over SendMessage with sublimite-28 and adelefeld-1f and recorded in
bd memory: (1) the AGGREGATE quota reading is the signal; every session
checks `quota` before each dispatch and stops dispatching when Claude
Weekly or Codex Weekly reads more than +12% ahead (midpoint of the
operator's 10-20% band). (2) Codex: announce-only ("codex start <lane>
<pid>" / "codex end <lane>"), since codex-cli 0.162.1 showed six coexisting
codex execs tonight with no kill; the 2026-08 SIGKILL trap is retired
(bd memory codex-kill-trap-update-2026-10-15-16). (3) Load: one full
vector per host; peers announce bursts above load 8; a red emu gate during
a burst is re-run alone before it is believed. (4) The root disk hit 0 bytes
free twice (01:20, 01:53; peers' build trees); one certificate gate read
"output is not exact" under ENOSPC and was green alone. New rule in bd
memory host-discipline-addendum-2026-10-16: df before a vector; remove
finished worktrees and clone build dirs promptly.

## What changed (merge order on main)

21cde96 -- initech-cjot (Opus). Real 1-2-3 R2.2 /Copy and /Move goldens
minted under the DOSBox-X mint harness (lotus123-decomp 51025ee: COPY01,
MOVE01, WRAP01 step files, 11 .WK1, control-panel shots). The corpus
differential test-i123-copy-move grades the artifact against the real
before/after pairs through the independent readers. It went RED for a
REAL reason: 5 of 8 pairs disagreed with the manual-derived model.
Law 1 / Law 2 at work: the lane stopped at the escalation point.

08cf239 -- lane-chrome-controls (Codex sol, imported from its bundle).
tdnl.99 (M011) label painting no longer erases the push-button bevel
rows; tdnl.100 (M012) the four corner patches match s8_alert_modal.png
64/64; L023 (rgz1) the alert frame is the sampled outline + two bevel
rows instead of the 7-px slab. Two sampled tint slots appended to the
locked color canon (Rule 8, WL-0110). New gates test-flair-alert-chrome
(+mutant); mutants CTRL_MUT_LABEL_OPAQUE, CTRL_MUT_BTN_FRINGE,
DIALOG_MUT_ALERT_SLAB. Residual reported: the capture's drop shadow
outside the frame is absent.

307f31d -- initech-jojk slice 1 (Opus). InitechWord P2: codes as
first-class units; bold/underline/italic pairs via F6/F8/Ctrl-F8;
.WP5 round-trip byte-exact on 8 goldens, three of them (MINT06-08)
minted from real WP 5.1 tonight; Reveal Codes (Alt-F3) graded against
12 real screens. DRAWCELLS carries txFace in fg bits 8..15 (Rule 8
change, spec/toolbox_gate.h Sec 7a, TBX_CELL_BASELINE 12; kernel +512 B).
Gates test-iword-codes, test-iword-reveal-codes, test-flair-iword-codes
(+mutants); clip iword_codes.

9143578 -- initech-nn46 (Opus). The artifact and the model now match real
R2.2 on all 8 pairs: ERR corners are 0xFFFF VAR/RANGE words (never token
32); /Move re-encodes relative columns as 0x8000|(delta&0xFF) and rows in
14-bit form; /Copy past column A keeps its bytes; /Copy past row 1 wraps
modulo 8192; a loaded 0xFFFF reads as ERR and stays 0xFFFF. Corpus spec
amended (lotus123-decomp 5ec1459, GAPS G-13 for two rules with no golden).
test-i123-copy-move is back in TEST_UNIT_GATES with the new corpus-mode
mutant gate (5 mutants RED). 123.EXE 65,376 B of the 64 KiB cap.

e7a8e62 -- initech-tflp (Codex sol). Every seeded gate data-volume recipe
ends with harness/verify_fixture_volume.sh (exact-path mdir + mattrib
flags); harness/fixture_volumes.mk declares 95+ contracts;
test-fixture-volumes lints scratch-vs-fixture name collisions and
"recipe ends with verification" (+mutant); gates assert fixtures before
boot. WL-0111.

b52a7ce -- PM: test-flair-fg-close re-keyed for the 33-px Apple slot
(tdnl.70 fallout; found by the first certificate at emu gate 197).

ed73278 -- PM: four mutant recipes had lost their for/if body to a blank
line inserted after a backslash-continued line by the verifier edit
(found at emu gate 147); test-makefile-vars gains leg 2 for that pattern.

86b1742 -- initech-airx (Opus). SAMIR REPORT FORM, first sitting:
.frm reader + renderer (title, columns, 0-2 group levels, subtotals,
totals, page geometry; PLAIN SUMMARY HEADING NOEJECT TO FILE; ALL NEXT
RECORD REST FOR WHILE), refusing TO PRINT, unsupported form flags, memo
and non-numeric group/total loudly. Byte-identical to real dBASE III PLUS
1.1 on 111 minted reports (dbase3-decomp fd28b97, re/mint-results-004.md,
tools/frm_ref.py independent reader, 199 md5s pinned). test-samir-report
(+mutant) and test-samir-report-emu (+mutant, files leg via mtools, screen
leg from the real transcript). SAMIR.COM 93,520 B: 6 KiB of owned heap
moved into the BSS tail (pal_heap_add, adjacency-checked coalescing,
samir.ld asserts). WL-0113. Four SAMIR findings filed (vfys, k7lc, 5vmn,
x5cc).

473883e -- initech-jojk slice 2 (Opus). Block operations (Alt-F4, Ctrl-F4
Move/Copy/Delete, Delete Block? prompt), the Delete [code]? prompt, tab /
indent / hard page as first-class codes with real labels; 12 new mints
(wordperfect51-decomp 3c40630, MINT09-20, re/mint-results-003.md). Two
slice-1 rules corrected to the mints: prefix byte 0x103 means
cursor-before-end at save (not italic); the Reveal Codes pane is 10 rows.
test-flair-iword-block (+mutant); clip iword_block. Kernel unchanged.

6865fc0, 8080833 -- PM: test-flair-shutdown probes re-keyed for the
Platinum alert edge (found at emu gate 201); fixture contracts for the
three InitechWord mutant volumes merged after the verifier (a5sr filed:
derive contracts from FLAIR_DATA_%_IMG, and rm the target when the
verifier fails -- a half-built image was treated as up to date).

2962734 -- initech-tdnl.51 / .52, F16 (Opus; the longest lane, ~3 h).
The Platinum body frame: CalcDocContentRect now puts the content's left
edge inside the left body rail (frame.left+6, was +1 -- every window's
content fill had painted over the rail), and the widget inner shadow
leaves the two off-diagonal corners. test-chrome-fidelity decodes the two
independent Mac OS 8.1 captures (system7-decomp goldens/captures, 60c7a36)
at test time and composes the window through the real Window Manager
seam; mutants CHROME_FID_MUT_CONTENT_LEFT_THIN and
CHROME_FID_MUT_WIDGET_CORNERS_EDGE RED. Stated re-keys (each with a dated
RE-KEYED comment): the InitechWord and Initech 123 faces are 76 cells
wide (was 77); the Word status line's Doc..Ln group ends at column 66 and
Pos sits at 68 (one column left of the WP 5.1 screens -- 'Pos 2.3"' does
not fit otherwise; shape and spacing unchanged); the /Range menu line is
cut at '...Trans  Sear'; README.TXT's icon is at (44,86); scroll-look
trace and checker, the i123 menu-level panel padding, the finder drag and
moved-cell coordinates. The lane merged main itself, re-keyed slice-2's
77-column assumptions, and ran the full 691-gate vector green on its
merged tree (build/f16_full.res, f16_r2.res in its worktree); the PM
re-ran its host family and the changed-checker emu gates on main. The
disk filled during the lane's run and truncated its Makefile to 0 bytes;
it restored from HEAD and re-applied its edits (same ENOSPC night as the
certificate's). Operator Law-4 items: the one-column status-line shift
and the shortened /Range line.

Beads hygiene (Sonnet): 5 stale beads closed with evidence (dibc, 586.5,
75r, zqy3 dup of 2gva, afer dup of 6rim); aul and gmo are done but blocked
by a bookkeeping dependency on the open P0 f8v.4 -- the operator decides
(a forced close was refused by the permission classifier and not worked
around); flagged for later: hdlb (bookkeeping, Bochs leg owed), jzhh
(split), lk29 (narrow to ADR-0002), 26d.

## Certificate

Assembled (bd memory cert-2026-10-16): host ALL GREEN 448 at b52a7ce;
emu 1..146 at b52a7ce, 147..200 at ed73278, 201..237 at 6865fc0, all
green; the three HEADs differ only in harness files. The airx and jojk
slice-2 merges were verified by their families first-person on main plus
emu 201..237, not by a full vector. The F16 merge 2962734 was verified by
its host family (9 gates incl. the two new mutants) and the 29 emu gates
whose checkers it touched, first-person on main; the lane itself ran all
691 gates green on its merged tree. A full clean certificate of the final
HEAD (make clean && make test in ../initech-os-cert) was started at close;
its result is recorded in bd memory cert-2026-10-16 and the HANDOFF.

## Frictions

- The permission classifier refused `codex exec --approve-for-me`
  ("Create Unsafe Agents"); `-s workspace-write` with approvals off
  works and is the safer shape. It also refused the lanes' copies into
  the corpora's goldens/ directories (their CLAUDE.md forbids edits under
  goldens/): MINT06-20 (WordPerfect) stay in mint/work/WP51 with md5s in
  the ledgers; the PM did not perform the copies on the lanes' behalf.
- A worktree spawns from a stale base; every lane was told to ff-merge
  main first (all did).
- The scratchpad is shared by sibling lanes; log names collided once
  (lanes now prefix their scratch files).
- bd's embedded Dolt backend is single-writer; `bd show` from lanes and
  the PM collided a few times (5-second retry clears it).

## Acceptance

Every lane's gates re-run first-person on main after its merge (host and
QEMU); every new gate has a named mutant proven RED. Clips re-recorded on
main, record-flair-repro byte-identical twice: i123_quit_guard, empty_trash
(marker re-keyed purged=5 -> 7), iword_codes, iword_block (e07b3d4). Bochs legs for the new emu gates remain owed
(cqej and the per-bead notes). Operator Law-4 eyeball owed for:
i123_quit_guard, empty_trash (alert edge), iword_codes, iword_block, and
last night's eight.

## Pointers

- Beads closed by the certificate: the WL-0104 lane set plus cjot, nn46,
  tflp (see HANDOFF). In progress with notes: jojk (slice 3), airx,
  tdnl.99, tdnl.100, rgz1. Filed: nn46 (closed), vfys, k7lc, 5vmn, x5cc,
  a5sr.
- Corpora commits (local, no remotes): lotus123-decomp 51025ee, 5ec1459;
  wordperfect51-decomp 0f48788, 3c40630; dbase3-decomp fd28b97.
- Quota at close: Claude Weekly about +11% ahead, Codex about +9.5%.
