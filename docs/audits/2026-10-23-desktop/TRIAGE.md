# Desktop source triage - 2026-10-23

## Finding challenged as WRONG

L005 is WRONG as a defect under the current, deliberate activation-only input
contract. The observed first double-click is real; the disputed expectation is
that its activating click should also enter the Finder click recognizer.
os/flair/process.c:745 records the pre-click active state, and :836-839 explicitly
consumes an inactive document's activating content click. The Finder follows the
same policy at os/milton/kmain.c:7665. This is not an accidental missing branch:
harness/proptest/test_finder_input.c:29-35 asserts activation without content
delivery followed by delivery of the next click; harness/finder_alerts.mk:47-53
treats PROC_MUT_CLICK_THROUGH as a regression. The locked
spec/flair_finder_alerts_trace.mk:1-5 and
docs/worklog/WL-0162-finder-alerts-pressed.md (Part 1, input policy and trace
re-keys) record the intentional policy. CLAUDE.md Rule 8 makes the locked
contract authoritative. Classify BY-DESIGN and do not file a click-through fix.
This establishes the project's contract, not a newly measured historical Finder
rule: the auditor explicitly had only a memory-based expectation. A future
Finder-specific historical comparison could justify a contract change; neither
this triage nor the audit supplies that comparison.

## Scope and tracking boundary

Audit: docs/audits/2026-10-23-desktop/REPORT.md and NITS.tsv, the nine new local
IDs L001-L009. Audited image/source commit:
e67b8311092873fe16e6c6d62ad82f894bfb8d7d.
Source citations below are repository-relative, pinned to inspection commit
3e44338f53906c23e54fc747a3d689a5d7e0ee2a in
/home/tobias/Projects/initech-os. The cited implementation causes also exist at
the audited commit. Reference paths beginning ../ name sibling corpora under
/home/tobias/Projects. Evidence paths below begin at
docs/audits/2026-10-23-desktop/ unless written in full.

Read CLAUDE.md, the 2026-10-05 full-drive TRIAGE.md, this report and its evidence,
AGENTS.md, relevant PRD 6.1/6.3/6.5 and source/spec/oracle material. This is source
triage, not a fresh emulator drive. No implementation edits, builds, emulator
runs, tracker mutations or oracle executions were performed. Acceptance work
below is required future work, not a claim of green tests. Only TRIAGE.md and
BEADS.tsv are deliverables.

D = source-confirmed defect in implemented behavior. F = feature never built at
the reported entry point. DOC = incorrect shipped instructions. BY-DESIGN =
deliberate contract retained. DUP = same defect owned by an existing open bead.
LANE takes precedence when LANES-IN-FLIGHT.txt assigns the exact finding ID.

LANES-IN-FLIGHT.txt assigns desktop L001/L006/L008/L007 to initech-vnl4. All four
are excluded from BEADS.tsv. L008 also overlaps the older reference-check bead
initech-tdnl.90, but its dispatched owner is still initech-vnl4; do not file it
again or settle the historical command-enable question from memory alone.

The supplied OPEN-BEADS.txt is the tracking authority, not a live status query.
No narrow existing owner was found for the four findings in the new batches.
initech-tdnl.80 covers scrollbar follow-ups and paging keys; L003 is the separate
omission of horizontal reveal from the implemented arrow/type-ahead handler.
Coordinate that shared scroll machinery without duplicating its other scope.
initech-tdnl.41 is mouse group drag, and initech-tdnl.83 is label clipping;
neither owns L002/L003. The broad menu/file-ops and publications owners
initech-tdnl.12, initech-tdnl.11 and initech-ipu5.14 are context, not evidence that
these particular gaps already have narrow owners. L004 concerns executable
command opening; the old F07 document-association gap remains separate.

Earlier IDs in the report's regression sections, including qualified
2026-10-05 L-ids, are historical cross-references, not additional local NITS
rows. Their retained/fixed/unreached states are not new issue candidates here.
The hourglass, teal, stacked bars, trailing minus, 116% pie and cooperative
scheduling are not challenged. No new panic-screen issue is reported.

## All nine findings

| ID | Verdict | Owner | Note |
| --- | --- | --- | --- |
| L001 | LANE | in lane initech-vnl4 | Full-disk recovery is blocked, not observed corruption. os/milton/desktop_db.c:348-352 needs mkdir to provision absent Trash; os/flair/finder_ops.c:364-367 refuses staging without it, and finder_windows.c:1084-1085 refuses opening. evidence/full02-root.txt proves zero free bytes and no TRASH. |
| L002 | D | B01 | fw_key changes selection and invalidates at os/flair/finder_windows.c:2203-2216, but the normal same-foreground key path at os/milton/kmain.c:7530-7550 never drains Finder damage and presents it; tenant service at :3101/:3113 does not do so for the native Finder. evidence/verification-metrics.json records zero changed pixels although Ctrl-O opens B. |
| L003 | D | B01 | os/flair/finder_windows.c:2207-2213 reveals only WSCROLL_V; the implemented keyboard selection never adjusts WSCROLL_H. shots/134-key-hidden-after-repaint.png and 135-key-hidden-revealed.png separate this from L002 by forcing repaint. |
| L004 | F | B02 | Menu/keyboard executable opening was never joined to the launcher: os/flair/finder_windows.c:1867 and :2093-2095 accept folders only. Double-click alone reaches FINDER_ICON_APP at os/milton/kmain.c:3982-3984. Disabled Open is honest missing command integration, not a failing executable loader. |
| L005 | BY-DESIGN | Activation-only contract; no new bead | WRONG as a current defect; see the evidence and limitation at the top. os/flair/process.c:836-839 and os/milton/kmain.c:7665 consume the activating click by deliberate policy. |
| L006 | LANE | in lane initech-vnl4 | os/flair/finder_windows.c:2003-2007 iterates live selected indices; finder_ops_put_away repopulates the same listing at os/flair/finder_ops.c:594. Repopulation reconstructs unselected icons, so the remaining selection is lost. evidence/trashmulti-serial.log:148-150 records sel=2 then n=1. |
| L007 | LANE | in lane initech-vnl4 | FINDER_WIN_ERR_NOTRASH returned by os/flair/finder_windows.c:1085 has no dedicated message and reaches its generic move wording at :1771. evidence/full02-serial.log:119-123 proves an open failure described as a move. |
| L008 | LANE | in lane initech-vnl4 | os/flair/finder_menu.c:126 authors New Folder enabled, and finder_windows.c:1167-1190 creates in the front window's dir_start without an is_trash guard; :1198 even documents creation inside Trash. Existing initech-tdnl.90 already asks for historical verification. Lane must establish that reference before changing policy. |
| L009 | DOC | B03 | pubs/manuals/tps-report-generator/s2-leaving-flair.tex:71-73 and :86-91 assume no AUTOEXEC and require date/time responses. Makefile:10406-10408/:10433 ships AUTOEXEC.BAT; os/milton/command.c:4782-4797 correctly prompts only when it is absent. The book's screens/SCREENS:7-8 already acknowledges the prompt-skipping shipped setup. |

## Existing lane: initech-vnl4

Do not add a BEADS.tsv row. The lane owns the full-disk recovery, complete Put
Away selection, Trash command availability and NOTRASH wording together.

For L001, the requested recovery path is an authored usability requirement, not
a measured Mac behavior. A visible disk-full refusal is already correct; it does
not solve the lack of a space-recovery path. Any permanent deletion alternative
must be explicit and confirmed, preserve files on cancellation/refusal, and be
graded on a genuinely full copied disk with no pre-existing service directory.
Use stopped-image mtools and fsck checks, plus a locked trace and recorded clip.

For L006, snapshot stable selected identities before operations rebuild views;
reversing the index loop is insufficient (the current loop is already reversed).
The stopped payloads prove partial restoration without loss: the BBB file is
back in B, while the AAA file remains recoverable in Trash. Cover collisions,
mixed success/refusal and rebooted origins using independent payload bytes.

For L008, source intent alone does not prove historical Finder behavior. The
existing reference-check bead and the audit both leave that question open. If
the measured policy disallows creation, align menu predicates and keyboard/direct
execution guards; simply graying one menu row is insufficient. For L007, test
the failed open's visible wording independently of whether recovery is added.
Evidence: evidence/full02-serial.log, full02-root.txt, full02-fsck.txt;
evidence/trashmulti-serial.log, trashmulti-A.txt, trashmulti-B.txt,
trashmulti-trash.txt, trashmulti-remaining-SAME.TXT,
trashmulti-restored-B-SAME.TXT, trashresume-serial.log; evidence/edge01-serial.log;
shots/116-full-trash-real.png, 117-full-trash-open.png,
201-trash-two-selected.png, 202-put-away-multiple.png,
080-after-purge-newfolder.png and 081-empty-twice.png.

## New developer batches, ordered by user harm

No unowned data-loss/corruption finding remains. The dispatched P1 recovery
block stays on initech-vnl4. B01 repairs misleading operative selection first;
B02 adds the missing executable command workflow; B03 corrects instructional
polish. Each batch has exactly one corresponding BEADS.tsv row.

### B01 - Present and reveal Finder keyboard selection (P2 bug)

Covers L002 and L003. One Finder keyboard/presentation pickup with two distinct
causes; neither is fixed merely by opening the selected item correctly.

Root causes: os/flair/finder_windows.c:2203-2216 changes the selected flag,
adjusts only vertical scroll and invalidates. Its updateEvt handler at
:2231-2242 can paint correctly, but os/milton/kmain.c:7530 dispatches the key
without the normal damage/content/present cycle afterward. The apparent repaint
opportunity at :7549 is guarded by unchanged list->head at :1713. The ordinary
service path at :3101 only presents when a disk tenant used drawing traps, and
:3113 returns without repaint when nothing was reaped. A native Finder key does
neither. Separately, :2207-2213 of finder_windows.c never updates the horizontal
scroll axis, so repaint alone cannot reveal the selected icon in a narrow view.

Ground truth: PRD 6.3's event-trace oracle;
../system7-decomp/specs/toolbox/window-manager.md:236-265 defines damage delivery
through updateEvt and drawing clipped to visible damage. The existing keyboard
contract is spec/flair_finder_alerts_trace.mk:10-14, with independent state checks
at harness/proptest/test_finder_limits.c:40-53. Prompt visual selection and
two-axis reveal are authored usability expectations from REPORT.md L002/L003,
not claims of a measured Mac key trace. Keep the existing selection algorithm,
shift extension, type-ahead timing and scroll-range ownership.

Acceptance: extend test-finder-limits/test-finder-windows with independent
expected selection and viewport geometry for arrows, type-ahead and Shift,
including a 100-pixel-wide view and leftward recovery after scrolling. A new or
extended emulator leg must capture immediately after the key, before any mouse
movement, command, clock update or forced repaint. Assert the previous highlight
is gone and the new operative item is visibly selected; then Ctrl-O must open
that same item. Separately force a repaint after a horizontal key and assert the
selected icon is in the viewport and the horizontal bar agrees. Include existing
vertical reveal and a viewport narrower than a full cell. Extend
test-flair-finder-limits rather than relying on its eventual OPEN marker alone.
Mutation-prove the pixel leg by omitting the post-key damage/present cycle while
retaining internal selection, and the geometry leg by reverting horizontal
reveal to vertical-only. The existing FINDER_MUT_NO_KEYS mutant alone cannot
distinguish these failures. Require QEMU acceptance, applicable Bochs regression
legs, and a locked record-flair trace with repro-proven GIF/MP4 per Rule 14.

Audit evidence: shots/130-key-before-A.png, 131-key-right-stale-A.png,
132-key-opens-B.png, 133-narrow-many-before.png,
134-key-hidden-after-repaint.png, 135-key-hidden-revealed.png,
repro-key-stale.png and repro-key-hidden.png;
evidence/verification-metrics.json, keys01-serial.log,
keyboard-repro-serial.log and reproduce_keyboard.py.

### B02 - Add executable File Open and Ctrl-O (P2 feature)

Covers L004. F follows the brief's entry-point discipline: a working double-click
launcher does not mean the command adapter was ever implemented.

Root cause: os/flair/finder_windows.c:2088-2095 sets selection_openable only for
folders, and :1865-1883 executes only selected folders. The ordinary MenuKey path
at os/milton/kmain.c:6305-6307 therefore finds Open grayed for an executable-only
selection. The application branch exists only in finder_surface_open at
:3982-3984, through flair_live_launch_app at :3127-3132. Reuse that real launcher
and its lifecycle/error path through the shared command spine; enabling the menu
without implementing its executable action would introduce an enabled no-op.
Keep document associations outside this pickup.

Ground truth: PRD 1.4/6.5 and ADR-0013's application contract, plus
docs/design/GUI-remediation-R3-finder-design.md F4.4's common mouse/keyboard
command spine. Menu/double-click equivalence for an already launchable executable
is the authored consistency requirement in REPORT.md L004, supported by the
audit's successful CTENANT double-click control; no new historical trace is
claimed. The period Mac menu keyboard semantics are recorded in
../system7-decomp/specs/toolbox/menu-manager.md.

Acceptance: extend test-finder-menu/test-finder-windows (and command/launch
adapter coverage as needed) to enable Open for selected applications, retain
folder and mixed-selection behavior, and leave unsupported documents honest.
Extend test-flair-app-launch with separate File > Open, Ctrl-O and double-click
legs on CTENANT.EXE, asserting one actual registration/foreground change and a
usable Quit/return. Include a second-resident-slot refusal and visible launch
failure; snapshot any selected file identities before a launch changes focus.
Mutation-prove both the application-enable predicate and the command-to-launch
adapter by removing each independently, requiring a named failure in its own
leg. Retain existing launch mutants and applicable Bochs app-launch coverage.
Lock and record the new GUI trace through record-flair before closure.

Audit evidence: shots/150-app-open-menu.png, 151-app-command-open.png and
152-app-double-click.png; evidence/menus-serial.log and menus-session.json.

### B03 - Correct the TPS manual's shipped DOS-exit procedure (P3 task)

Covers L009. This is a DOC change; the kernel correctly honors AUTOEXEC.BAT.

Root cause: pubs/manuals/tps-report-generator/s2-leaving-flair.tex:71-73 asserts
that the shipped volume lacks AUTOEXEC.BAT, :86-91 requires responses to prompts
that never appear, and :29 lists answering those prompts as an unconditional
objective. Its lab/step references at :123/:137 must remain coherent if steps
are renumbered. Makefile:10406-10408 writes @ECHO OFF and PATH=A:\DOS, and :10433
installs it. os/milton/command.c:4782-4797 branches correctly on its presence.

Ground truth: the extracted evidence/shipped-AUTOEXEC.BAT and the actual shipped
image recipe, not an assumed blank installation. The book's own
pubs/manuals/tps-report-generator/screens/SCREENS:3-8 already documents the real
FLAIR journey and skipped date/time prompts; screens/tpsdir.keys:1 begins with
CLS. These independently agree with shots/214-exit-dos-prompts.png and
215-dos-alive.png and evidence/power-serial.log. Explain prompts only
conditionally for a volume without AUTOEXEC; make the normal procedure proceed
directly to A:\>, CLS and CD APPS.

Acceptance: run test-pubs-tps and test-pubs-screens and the existing
test-flair-exit-to-dos journey after correcting all affected objectives,
responses and step cross-references. Add an acceptance leg checking the printed
TPS-P-01 expected response sequence against an emulator transcript on the actual
shipped volume. Existing screen comparisons alone missed the contradictory
prose, and existing TPS message-inventory mutants do not cover it. Mutation:
restore the unconditional no-AUTOEXEC premise/date-prompt response in an isolated
manual copy; the same procedure-contract check must fail for the named mismatch.
Retain a no-AUTOEXEC control proving the documented conditional date/time path;
do not force startup prompts on the ordinary shipped disk to make the book true.

Audit evidence: shots/214-exit-dos-prompts.png, 215-dos-alive.png;
evidence/power-serial.log, shipped-AUTOEXEC.BAT,
references/s2-leaving-flair.tex and verification-metrics.json.

## Completion accounting

Nine local findings, each assigned once: D=2, F=1, DOC=1, BY-DESIGN=1, DUP=0,
LANE=4. Three new batches cover four findings; no lane-owned finding appears in
BEADS.tsv. No P1 create is needed here. All outputs are ASCII. No live tracker
absence, historical activation/Trash policy, new emulator reproduction or green
acceptance result is claimed beyond the evidence described above.
