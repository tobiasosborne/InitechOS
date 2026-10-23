#!/usr/bin/env python3
"""Generate the ASCII report and TSV from the reviewed finding records."""
import json,pathlib
R=pathlib.Path(__file__).resolve().parents[1]
F=[
dict(id='L001',area='Finder / full disk',severity='P1',summary='A full volume without an existing Trash cannot be freed from Finder',
steps='Boot flair_tenants_interactive.img with full-data.img, made by evidence/prepare_fixtures.py (zero free clusters; no pre-existing TRASH). Dismiss the boot alert with Return and Escape. Double-click INITECH (600,64). Drag README.TXT (60,102) to Trash (600,420). Dismiss the refusal and double-click Trash.',
saw='The file remained in the root. Staging and opening Trash both failed. New Folder correctly reported disk full, but the Finder offered no deletion route that could free space. The stopped volume still had zero bytes free. This is a blocked Finder recovery task, not observed file corruption; Exit to InitechDOS is a possible workaround.',
expected='Allow an explicitly confirmed space-recovery operation even when no cluster is available for creating Trash, or reserve the required metadata before the disk can fill. This is my filesystem usability expectation, not a measured historical Mac behavior.',
evidence=['shots/114-full-root.png','shots/115-full-create-alert.png','shots/116-full-trash-real.png','evidence/full02-serial.log','evidence/full02-root.txt','evidence/full02-fsck.txt','evidence/full02-hashes.json'],
guess='os/milton/kmain.c:3654 initializes the service directory; os/flair/finder_windows.c:1084 rejects its absence. Guess: no space-recovery fallback exists when that initialization cannot allocate a cluster.'),
dict(id='L002',area='Finder / keyboard painting',severity='P2',summary='Keyboard selection changes internally while the old highlight remains on screen',
steps='Fresh edge-data.img: open INITECH (600,64), click folder A (60,154), press Right, and wait two seconds. Then press Ctrl-O. Independent replay: python3 evidence/reproduce_keyboard.py.',
saw='A stayed highlighted after Right. The entire root-window crop was byte-identical before and after the key, despite the wait. Ctrl-O then opened B. Type-ahead and vertical navigation likewise changed state without presenting the new selection until mouse-driven repainting. The independent keyboard-repro session reproduced it.',
expected='Present the new selection and scroll position immediately, before the next command acts on it. Ordinary GUI feedback expectation. Earlier G07 described absent keyboard selection; this is a defect in the newly functioning keyboard implementation, with an observed mismatch between displayed and operative selection.',
evidence=['shots/130-key-before-A.png','shots/131-key-right-stale-A.png','shots/132-key-opens-B.png','shots/repro-key-stale.png','shots/repro-key-opens-B.png','evidence/keys01-serial.log','evidence/keyboard-repro-serial.log','evidence/verification-metrics.json','evidence/reproduce_keyboard.py'],
guess='os/flair/finder_windows.c:2164 updates selection and invalidates it. os/milton/kmain.c:7530 dispatches the key, but its subsequent presentation paths service tenant switches and disk-tenant drawing. Guess: ordinary Finder keys lack the required paint/present cycle.'),
dict(id='L003',area='Finder / keyboard scrolling',severity='P2',summary='Horizontal keyboard navigation can leave the selected icon entirely outside the viewport',
steps='Fresh edge-data.img: open INITECH, then MANY (194,154). Select F000.TXT (78,122). Drag the grow box (389,289) by (-260,0), making the window 100 pixels wide. Press Right. Force repaint with a one-pixel title drag from (84,90) to (85,90). Finally grow from (130,289) by (260,0).',
saw='After repaint, no selected icon was visible in the narrow window and the horizontal thumb remained at the left. Widening the window revealed F001.TXT selected. This persists after an explicit repaint and is separate from L002. The pinned, overlapping labels in the narrow view are existing J02.',
expected='Scroll horizontally to keep the keyboard-selected item visible. This is my keyboard-navigation usability expectation; no live Mac keyboard comparison was performed.',
evidence=['shots/133-narrow-many-before.png','shots/134-key-hidden-after-repaint.png','shots/135-key-hidden-revealed.png','shots/repro-key-hidden.png','shots/repro-key-revealed.png','evidence/keys01-serial.log','evidence/reproduce_keyboard.py'],
guess='os/flair/finder_windows.c:2206 adjusts only WSCROLL_V in fw_key. Guess: the horizontal reveal calculation is missing.'),
dict(id='L004',area='Finder / application launch',severity='P2',summary='File > Open and Ctrl-O cannot launch a selected executable',
steps='Fresh normal data: double-click INITECH (600,64), APPS (126,102), then single-click CTENANT.EXE (148,122). Hold lower File at (53,29), dismiss it, and press Ctrl-O. Then double-click the same executable.',
saw='Open was disabled and Ctrl-O did nothing. Double-clicking launched C Tenant from the same selection and file. This is not the old inability to open README documents (F07); this executable is demonstrably launchable.',
expected='File > Open and its displayed shortcut should perform the same supported open operation as a double-click. This is a classic Finder expectation from memory and a consistency requirement within this session.',
evidence=['shots/150-app-open-menu.png','shots/151-app-command-open.png','shots/152-app-double-click.png','evidence/menus-serial.log'],
guess='os/flair/finder_windows.c:2094 enables Open only for folders; :1867 likewise processes only folders. Guess: the executable launch path in os/milton/kmain.c:3975 was not joined to the menu/keyboard command.'),
dict(id='L005',area='Finder / inactive windows',severity='P2',summary='A double-click in an inactive Finder window only activates and selects',
steps='Fresh normal data: open root and APPS. Move APPS by dragging (210,90) to (430,220), leaving the root DOS icon exposed. Double-click DOS at (195,102) with the driver\'s 130 ms click spacing. Double-click it again after the resulting screenshot.',
saw='The first double-click activated the root and selected DOS without opening it. The second double-click opened DOS. The same lost first click was encountered when trying to launch an executable from an inactive APPS window.',
expected='A double-click on an exposed Finder icon should open it even if its window was inactive. This expectation is from my memory of Finder and from ordinary double-click semantics, not from a captured historical input trace. Treat the period-specific click-through policy as a design decision to verify.',
evidence=['shots/160-inactive-folder-doubleclick.png','shots/161-active-folder-doubleclick.png','evidence/menus-serial.log'],
guess='os/milton/kmain.c:7665 calls the Finder gesture tracker only when chrome_was_active. Guess: the activation click never participates in the double-click recognizer.'),
dict(id='L006',area='Trash / Put Away',severity='P2',summary='Put Away silently restores only one item from a multiple selection',
steps='Fresh edge-data.img: open root, open A (60,154), drag A/SAME.TXT (145,122) to Trash (600,420), and close A with Ctrl-W. Open B (126,154), drag B/SAME.TXT (78,122) to Trash, and close B. Open Trash, press Ctrl-A, then Ctrl-Y.',
saw='Trash contained SAME.TXT and collision-renamed SAME001.TXT, and both were visibly selected. Ctrl-Y logged PUT_AWAY sel=2 followed by FINDER-PUT-AWAY n=1. SAME.TXT remained, now deselected. Extracted bytes show B/SAME.TXT restored with the BBB payload and the AAA payload still in Trash. A later reboot and single-item Put Away restored the remaining item successfully.',
expected='Apply Put Away to all selected restorable items, or report the partial result. Classic Finder command-selection semantics from memory; single-item origin persistence was independently observed working.',
evidence=['shots/200-trash-colliding-names.png','shots/201-trash-two-selected.png','shots/202-put-away-multiple.png','shots/203-putaway-remaining-state.png','shots/204-putaway-after-reboot.png','evidence/trashmulti-serial.log','evidence/trashmulti-trash.txt','evidence/trashmulti-A.txt','evidence/trashmulti-B.txt','evidence/trashmulti-remaining-SAME.TXT','evidence/trashmulti-restored-B-SAME.TXT','evidence/trashmulti-fsck.txt','evidence/trashresume-serial.log'],
guess='os/flair/finder_windows.c:2003 walks selected indices, while finder_ops_put_away in os/flair/finder_ops.c:601 repopulates the window after each item. Guess: that repopulation clears the remaining selection before the loop can process it.'),
dict(id='L007',area='Finder / error wording',severity='P3',summary='Opening unavailable Trash reports that an item could not be moved',
steps='Use the full02 setup from L001. Dismiss previous alerts, then double-click the desktop Trash at (600,420).',
saw='The open operation logged FINDER-WIN-OPEN-FAIL rc=-13. Its alert said, "The item could not be moved. The disk may be full or the folder may no longer be available." No move had been requested.',
expected='Name the failed operation and missing/unavailable Trash. This is an error-message accuracy expectation. This wording defect is independent of L001\'s inability to recover disk space.',
evidence=['shots/117-full-trash-open.png','evidence/full02-serial.log'],
guess='os/flair/finder_windows.c:1085 returns FINDER_WIN_ERR_NOTRASH, which falls through the generic move wording at :1771. Guess: an operation-specific NOTRASH message is missing.'),
dict(id='L008',area='Trash / command availability',severity='P2',summary='New Folder creates a fresh folder directly inside Trash',
steps='In the edge01 session, empty Trash while its window remains open (after shots/079-purged-child-closed.png). Press Ctrl-N, then choose lower Special > Empty Trash at (200,29) -> (200,64).',
saw='NEWFOLD appeared directly in Trash. The serial log confirms FINDER-NEW-FOLDER name=NEWFOLD parent=710, and the next destructive confirmation offered to remove that new folder. Creation did not redirect to an ordinary folder or explain the unusual destination.',
expected='Disable New Folder while Trash is frontmost. This is a period Finder expectation from memory and a protection against accidentally creating work in a deletion container; it is not backed by a captured Mac command-enable matrix.',
evidence=['shots/079-purged-child-closed.png','shots/080-after-purge-newfolder.png','shots/081-empty-twice.png','evidence/edge01-serial.log'],
guess='os/flair/finder_menu.c:124 authors New Folder as always enabled, and os/flair/finder_windows.c:1835 calls the ordinary folder-creation path without a Trash context guard. Guess: Trash-specific enablement is absent.'),
dict(id='L009',area='Shell / shipped manual',severity='P3',summary='Exit-to-DOS instructions promise date/time prompts that the shipped disk skips',
steps='Boot the supplied normal pair. Activate Finder by opening INITECH. Choose lower Special > Exit to InitechDOS at (200,29) -> (200,163), then Return. Wait for the console.',
saw='The banner was followed immediately by A:\\>; no date or time prompt appeared. The extracted shipped AUTOEXEC.BAT contains @ECHO OFF and PATH=A:\\DOS. An echo command confirmed the shell was live.',
expected='The shipped procedure should match the shipped disk. pubs/manuals/tps-report-generator/s2-leaving-flair.tex:71 says "Because the data volume has no \\file{AUTOEXEC.BAT}, InitechDOS asks for the" date/time; lines 86-91 then instruct the user to answer those prompts. Update that procedure or its assumed installation. This is a named local manual mismatch, not a request to alter standard DOS AUTOEXEC behavior.',
evidence=['shots/214-exit-dos-prompts.png','shots/215-dos-alive.png','evidence/power-serial.log','evidence/shipped-AUTOEXEC.BAT','evidence/references/s2-leaving-flair.tex'],
guess='No kernel defect inferred: the image\'s AUTOEXEC.BAT and the manual\'s installation assumption disagree.')
]
intro='''# FLAIR desktop and Finder audit - 2026-10-23

Audited supplied image commit: e67b8311092873fe16e6c6d62ad82f894bfb8d7d.
Read-only source HEAD matched that commit. Nine new findings: one P1,
six P2, and two P3. No observed panic, event-drop marker, or filesystem
corruption. The P1 is a blocked full-disk recovery workflow. Keyboard feedback,
application command opening, and multiple-item Put Away need attention.

All guest input and capture used headless QEMU, -cpu 486 -m 16 -display none,
with QMP on stdio. Only private image copies were booted. At most one emulator
owned by this audit ran at a time. No repository edits, builds, tracker commands,
display :0 input/capture, or unrelated process termination were performed.

## Evidence and replay conventions

Coordinates are native 640x480 guest coordinates, with (0,0) at top left.
Double-click means two 50 ms presses separated by 80 ms, unless stated otherwise.
Menu selection means press the title, move vertically below the bar, move to the
item if needed, then release. The later selectmenu helper avoids accidentally
crossing another title during a diagonal pointer move.

Use `python3 evidence/replay_session.py SESSION` from this directory for a full
recorded session. It replays the atomic events from evidence/actions.jsonl into
new replay-prefixed copies and outputs; it preserves original timing by default.
Each *-session.json records QEMU arguments, private PID, image hashes and clean
exit. evidence/prepare_fixtures.py builds the local many/deep/full fixtures;
the supplied images themselves were not modified. Retained fixture images make
the original dates and bytes available without regenerating them.

The decline test substitutes the archived, test-only CTENANT_DECLINE_QUIT binary
from the repository's already-built build/tenant/ctenant_decline.exe into a local
copy. It is not claimed to be behavior of the shipped consenting CTENANT.EXE.
Its bytes, provenance hashes, and replay preparation script are in evidence/.

The first final_sweep run stopped on the harness's invalid QEMU key name\n`grave`; its already completed scroll/drag captures are retained as inputfinal.\nThe corrected key name is `grave_accent`. The fast HMP-based inputfinish sweep\nleft ambiguous modifier state and its intended HELLO/NOTES captures 230-233\nwere actually Finder/About states. They are not evidence of built-in app\nbehavior. directinputs repeated the sweep with explicit QMP make/break pairs;\nCtrl-N/Ctrl-W then worked, and 250-*/260-* are the valid built-in menu captures.\nThe rejected key name was a host harness error, not a guest failure; the HMP\ntimer-overlap possibility is not attributed to the guest without isolation.\n\nEarly screenshots 003-008 and 009-014 were attempted menu captures after release
or before guest presentation, and 175's later accessory attempts were blocked by
a still-running Calculator. They are retained as an honest journal, but no
finding relies on those misleading names. The valid replacements are 017-*,
179-181, and 183/185. Screenshot 207 is the retained Trash folder, not its opened
contents; the stopped-disk extraction establishes its contents instead.

References are local, read-only System 7/Mac OS 8.1 specifications and captures,
copied under evidence/references/. Period expectations without a measured
historical input trace are explicitly marked as memory or my own expectation.
Source explanations below are guesses, not debugger-confirmed root causes.

New IDs below are local L001-L009. Old L IDs are always qualified with
2026-10-05 to avoid confusing them with this report's numbering. Earlier audits
and their headings/hashes are indexed in evidence/prior-report-index.json.

## 1. New findings
'''
parts=[intro]
for f in F:
 parts.append(f"\n### {f['id']} - {f['summary']}\n\nArea: {f['area']}. Severity: {f['severity']}.\n\nSteps: {f['steps']}\n\nObserved: {f['saw']}\n\nExpected and basis: {f['expected']}\n\nSource hypothesis: {f['guess']}\n\nEvidence: "+', '.join(f"[{p}]({p})" for p in f['evidence'])+'.\n')
parts.append('''
## 2. Still present

These are existing findings observed again, not new NITS.tsv rows.

| Earlier ID | Current observed scope and evidence |
| --- | --- |
| F07 | README.TXT has no usable document Open route; Open is disabled and Ctrl-O does nothing. shots/015-document-open.png; base serial. L004 above concerns an executable that double-click can actually launch. |
| F08 | Dragging the two selected B files repositions only the lead file; the other stays put. shots/089-group-drag.png, edge01 serial. |
| F09 | Manual file placement is reset on closing/reopening its folder. shots/089-group-drag.png versus 090-placement-reopened.png. |
| F10 | A resized root reopens at the default 360x220, rather than the saved 220x140. shots/191-size-before-close.png / 192-size-reopen.png, menus serial. Origin persistence did work for B (083-B-reopened.png). |
| F11 | Return, F2 and label clicks did not expose an inline filename editor. New Folder produces NEWFOLD. shots/138-no-inline-rename.png and 126-decline-desktop-live.png. Invalid-name cases cannot reach an editor. |
| F12, remaining portions | Find, Print, Page Setup, Make Alias, Sharing, Eject, Undo, Cut/Copy/Paste/Clear, Clipboard, Preferences, by Label, Buttons and Pop-up remain disabled. Current menu sweep in menus serial, shots/017-file-menu.png / 017-edit-menu.png / 017-view-menu.png / 172-edit-no-selection.png. List views and ordinary Put Away are no longer absent. Get Info/Duplicate are also unavailable, though their old F03 enabled-no-op presentation is fixed. |
| F13, Help portion | About Help and Show Balloons remain disabled. shots/017-help-menu.png; mouse attempts in menus serial. Apple access is fixed. |
| F14 | Alt-Tab produced no application-switching interface or foreground change. shots/022-alt-tab.png / 023-quit-context.png; base serial. |
| F15, residual | All four upper titles still expose the same About/Quit pair. About is now disabled; upper Quit remains an enabled no-op. shots/162-upper-file.png / 165-upper-special.png and 264-upper-noop.png; directinputs serial. The intentional second bar itself is not a finding. |
| F16, icon-art portion only | Native folder/document/app icons remain much simpler than the Mac 8.1 capture. Compare shots/002-root.png and 016-apps.png with evidence/references/s8_doc_window_active.png. The earlier missing body frame is fixed; the sampled title band and body frame now match. No new font-strike mismatch is asserted without a fresh glyph oracle. |
| F17 | No item count or free-space header appears in either icon or list views. shots/002-root.png and 167-name-view.png. Named reference: system7-decomp/specs/desktop/finder-windows.md, icon-view content header; s8_doc_window_active.png y=92..112. |
| F18 | Right-click and Control-click on README did not open a context menu. shots/143-context-unavailable.png; keys01 serial. |
| G07, partial | Keyboard selection now has an implementation, but its screen feedback is broken (new L002/L003). Page Up/Down and Home/End produced no visible navigation in the tested overflowing icon/list views. shots/041-many.png / 042-many-keys.png / 045-many-home.png / 048-list-pgup.png. |
| H07 | Wide 8.3 labels overlap; ordinary long APPS filenames also run together. shots/046-many-bottom.png / 016-apps.png / 188-close-cancel.png. |
| J02 | Narrow views pin hidden icons' labels over visible filenames. shots/133-narrow-many-before.png / 134-key-hidden-after-repaint.png / 191-size-before-close.png. |
| K07, functional limitation | HELLO and NOTES remain blank demonstration tenants, not usable document editors. Typed NOTES text never appears; file/edit commands are now disabled, so the old enabled-no-op aspect is fixed. shots/030-notes-file-real.png / 031-notes-quit-real.png; directinputs built-in menu sweep, shots/250-hello-0.png and 260-notes-0.png. |
| 2026-10-05:L024 | Empty Trash reports logical bytes as disk space, omitting directory/allocation costs. Four one-cluster objects (2 KiB at 512 bytes/cluster) were described as 1K in shots/075-empty-confirm.png. A newly created directory alone was 0K in 081-empty-twice.png. edge-data fixture and edge01 serial establish the tree and count. |
| 2026-10-05:L030 | New Finder directory entries still have invalid 1980-00-00 dates. evidence/decline-apps.txt (NEWFOLD), trashmulti-trash.txt (new TRASH dot entries), and stopped disk images. |
| M003 | Menu shortcut notation remains a caret, not the historical Command glyph. shots/017-file-menu.png; reference s8_menu_dropped.png shortcut column. This retains the existing ID; the PC keyboard mapping itself is not newly challenged. |

## 3. Now fixed or narrowed by this session

| Earlier ID | What no longer reproduced and evidence |
| --- | --- |
| F01 | Finder content follows moved/zoomed windows; CTENANT follows its frame too. shots/063-B-moved.png / 084-B-place.png / 189-root-zoom.png / 021-ctenant-moved.png. |
| F02 | Scroll arrows, repeat, track paging and thumb movement operate the content rather than dragging the window. edge01 serial; shots/046-many-bottom.png / 059-list-page.png. |
| F03 | Get Info/Duplicate are honestly disabled; Select All selects all items. shots/017-file-menu.png / 054-select-all.png / 201-trash-two-selected.png. Missing features remain listed above. |
| F04 | Trash staging, opening, moving out and emptying work on a normal disk. edge01/trashmulti/trashresume logs and stopped-disk payloads. |
| F05 | Restart and Shut Down work, ask the resident disk app to quit, and present the safe screen. shots/210-restart-complete.png / 211-safe-shutdown.png / 212-shutdown-restart.png; power serial. |
| G01, H01 | A menu stayed open for 35 seconds. A second 35-second hold then crossed from File to View; Arrange remained highlighted and dispatched ARRANGE on release. shots/183-held-cross-menu-valid.png / 185-cross-view-held.png / 186-cross-view-result.png; menus serial. |
| G02 | Close waits for release; moving off before release leaves the window open. shots/187-close-held.png / 188-close-cancel.png. |
| G03 | CTENANT File > Quit closes it; TenantFix About paints text and adds its menu, and Ctrl-Q exits. shots/155-ctenant-menu-quit.png / 156-tenantfx-about.png / 159-tenantfx-key-quit.png. |
| G04 | The MANY folder's entries beyond 64 are reachable, including F079 and both wide names. shots/046-many-bottom.png; evidence/edge-data-inventory.txt contains all 82. |
| G05 | Full-disk boot/New Folder failures now show an alert, and no phantom committed file appears in the stopped root. shots/110-full-boot.png / 115-full-create-alert.png, full02-root.txt and full02-fsck.txt. New L001 is the recovery dead end, not the old silent error. |
| G06 | Twenty-five Finder windows could coexist; opening the next produced a visible memory-limit alert. All slots 0..24 subsequently closed cleanly. shots/103-window-limit.png / 106-all-closed-root-reopened.png; verification-metrics.json. |
| G09, launch-refusal portion | A second resident disk app now produces an on-screen launch failure. shots/153-second-app-cap-alert.png and TENANT-SLOT-BUSY in menus serial. The message is generic and does not explain the one-resident-app limit. An actual crashing tenant was not injected. |
| G10 | Arrange changes the root to name order and repairs manual placement. shots/186-cross-view-result.png, menus serial. |
| G11 | Released Ctrl/Shift did not stay latched; ordinary nested-folder type-ahead did not create folders. edge01 nested-window sequence and directinputs explicit make/break modifier sweep (245/246 confirm Ctrl-N and Ctrl-W still work). |
| H02 | Cycle refusal now explains why the move cannot proceed. shots/136-cycle-refusal.png. Replace-on-exists also has a real Cancel/Replace dialog (065-070). |
| H04 | Trash visibly changes between full and empty. shots/073-A-staged-open-child.png / 079-purged-child-closed.png. |
| H06 | TenantFix zoom remained clipped above its scroll gutters. shots/158-tenantfx-zoom.png. |
| J01 | Shrinking enabled horizontal scrolling; restoring the original width removed the thumb and restored disabled-looking art. shots/133-narrow-many-before.png / 135-key-hidden-revealed.png; 223-grow-bars.png. |
| J03 | Open processes two selected folders: both A and B opened. shots/061-two-folders-selected.png / 062-two-folders-open.png; edge01 serial OPEN-COUNT n=2. |
| J05 | Minimum size is now 96x88 with visible, bounded scroll parts, rather than the old hidden-but-active controls. shots/091-minimum-window.png. |
| F13, Apple portion | Apple About and all four installed accessories launch. shots/032-about-real.png / 175-apple-calculator.png / 179-control-panels-launched.png / 180-note-pad-launched.png / 181-scrapbook-launched.png. |
| F12, views/Put Away portions | Small Icon and Name/Size/Kind/Date lists work; list header clicks change sort; single-item Put Away survives reboot and Trash renaming. shots/166-small-icon.png through 171-date-header.png and 204-putaway-after-reboot.png. L006 is the multiple-selection residual. |
| 2026-10-05:L002 | Purging A/CHILD closed their open windows before another New Folder command. The surviving window was Trash; no orphan allocation was found. shots/079-purged-child-closed.png, edge01 serial, edge01-fsck.txt. |
| 2026-10-05:L004 | Empty Trash and replacement buttons now wait for release and cancel the press when dragged off. shots/068-replace-ok-held.png / 069-replace-ok-outside.png / 077-empty-ok-held.png / 078-empty-ok-outside.png / 235-cancel-held.png / 236-cancel-drag-off.png. |
| 2026-10-05:L022, L023 | Caution triangle and thin beveled alert border now appear. shots/075-empty-confirm.png; look-metrics.json measures 264 yellow caution pixels and the thin edge. |
| 2026-10-05:L117 | CTENANT retains both key and click readouts through movement/repaint. shots/020-ctenant-input.png / 021-ctenant-moved.png. |
| 2026-10-05:L118 | Locked file staging is refused with an unlock instruction. A locked descendant survived recursive Empty Trash while deletable siblings were removed. shots/085-locked-refused.png / 206-locked-tree-retained.png; trashresume-trash.txt and extracted LOCKED.TXT. |
| M001, M002, M004, M005 | Apple/File placement and text inset now agree with reference geometry; selected titles have a dark face; shortcut clearance is 14 px. shots/017-file-menu.png and look-metrics.json. |
| M006, M010 | View now shows a real checkmark; narrow titles use an ellipsis. shots/174-checkmark.png / 133-narrow-many-before.png. |
| M007, M008 | Thumb now has highlight/shadow and visible grips: its 14x15 crop has 49 highlight, 102 teal face, 54 dark edge/grip and 5 corner pixels, rather than a flat face. shots/041-many.png and look-metrics.json. |
| M011, M012, M013 | Button bevels remain continuous, the checked button corner matches the reference, and the enabled arrow tile matches all 196 reference pixels. look-metrics.json; shots/075-empty-confirm.png / 041-many.png. |

No status is claimed for untested earlier IDs. In particular F06, G08, H03/H05,
J04, K14 and the rest of the other auditors' application/DOS findings are not
declared fixed by source inspection. Managed DESKTOP.DB/TRASH entries are now
hidden from the root UI, so the former direct service-file drag steps were not
available. The no-motion thumb probe in inputfinal stayed at value=16/max=16. That endpoint
case does not establish that J04's interior-range rounding defect is fixed.

## 4. Coverage, appearance and limitations

| Area | Observed coverage |
| --- | --- |
| Windows | Root, APPS, DOS, Trash, A/B, CHILD, MANY, 16 nested levels, multiple selection Open, 25-window limit and an additional open; odd-order closes; inactive title activation; 1-pixel drag; min grow and over-min request; left/right/top/bottom off-screen attempts; safe top/bottom title clamps; zoom/unzoom; collapse/expand; origin and size reopening. |
| Scroll and lists | Disabled and enabled bars, arrow click/hold, page area, thumb, horizontal clipping, resize/grow, one overflowing item, 82-item contents, all implemented View modes and a list header. Original 82 names and bytes are in the fixture inventory. |
| Selection/input | Click, Shift-click, Select All, rubber bands in four directions (one start landed on icon content; the other recorded marquees selected 12 items), multi-drag, click on an icon itself, same-window reposition, a folder cycle, slow double-click, fast triple-click, desktop triple-click, pointer corners, Ctrl/Alt/Shift sweeps over letters/digits/function/navigation/punctuation keys, and keys during a held window drag. Standard destructive/menu command chords were exercised separately rather than fired blindly during the sweep. |
| Menus | Both bars, all Finder titles and nonseparator rows, disabled mouse choices, implemented commands and shown shortcuts, no/one/many selection states, upper About/Quit pairs, all eight HELLO titles, NOTES File/Edit/Notes rows, CTENANT Quit, TenantFix File/Edit/Fixture/dynamic About, Apple About and each installed accessory launcher; release outside and long cross-title tracking. This is not an exhaustive Cartesian product of every item and every selection state. |
| Dialogs | Full-disk, replace/cancel/Return, cycle refusal, locked-file refusal, launch limit, window limit, Empty Trash Return/Escape/click/held OK/held Cancel/drag outside, background clicks and Ctrl-N blocked while modal, exit confirmation, safe shutdown. |
| Trash | Single files, two colliding names, original-name restoration, rebooted origin metadata, multiple-item Put Away failure, open descendant purge, a locked descendant, empty-twice state, and recursive deletion of MANY with 82 files. |
| Shell/app life | Built-in HELLO/NOTES, launch and quit of both test tenants, launch refusal with resident tenant, app-menu delivery, CTENANT retained drawing, consenting Restart/Shut Down, declining Restart/Shut Down/Exit, confirmed DOS exit, and the Jump to Conclusions Mat opening/choice/closing. |
| Long session | edge01 alone contains more than 230 high-level clicks/drags/keys/commands, with the 25-window limit near its end; menus and final input sessions add longer held gestures and repeated actions. The final JSON summarizes actual counts and all serial searches. No FLAIR-EVT-DROP or panic marker was observed. |

Look: the sampled 22-row title band is identical to the Mac 8.1 reference
profile, and the six-pixel body-frame cross-section now matches exactly.
Scroll thumbs, menu placement, alert edges and buttons are visibly improved.
The teal desktop, stacked bars, Chicago choice and safe-screen word "computer"
were not treated as bugs. Sparse icon art, crowded labels and missing window
information remain the largest visible gaps. Evidence/reference palette values
are sampled framebuffer RGB, not uncorrected nominal CLUT values. Geneva 9/12
and every Chicago glyph were not independently re-extracted in this run; no
font-byte-parity claim is made. No real Mac motion recording exists here for
frame-time comparison, and no live busy-hourglass interval was captured.

Unreached or bounded cases:

- There is no reachable Finder rename field, so empty/9-character/lowercase/
  dotted/spaced/illegal/duplicate/Trash-collision edit validation cannot be
  tested. That is F11, not eight invented validation bugs.
- Get Info/Duplicate, file association, most Edit commands, Help, aliases,
  printing, search, labels and preferences remain unavailable. Their known
  missing-feature IDs are retained instead of filing every disabled row anew.
- HELLO, NOTES and TenantFix expose no enabled Scrap transfer route, and
  CTENANT has none. Inter-window Scrap through those requested vehicles is
  blocked. NOTES cannot accept edits, so a real unsaved NOTES restart prompt
  could not be constructed. Note Pad is a different app, owned by the desk-
  accessories audit; only its Apple launch/close was tested here.
- Only one volume is exposed by this Finder image. Cross-volume drag to a
  separate full destination, removable-media eject, and physical write-protect
  were not reachable; the full-volume fixture tests the available failure path.
- The 82-item fixture exceeds the old 64 limit, but does not exhaust the full
  icon arena. Arbitrarily deep/cyclic FAT corruption and all key combinations
  involving simultaneous Ctrl+Alt+Shift were not injected. Ctrl+Alt+backslash
  was deliberately left to the database auditor.
- Decline behavior was tested with the existing explicit test binary, not by
  rebuilding a tenant or claiming a refusal control in the shipped executable.
- Static captures cannot prove absence of every transient flicker. No
  unobserved flicker, race, crash, or data corruption is listed as a finding.

## 5. Fifteen fixes I would make first

1. Provide a safe Finder recovery path when a full disk has no Trash (L001).
2. Paint keyboard selection before the next action can use it (L002).
3. Preserve and process the complete Put Away selection (L006).
4. Give executable File > Open / Ctrl-O the double-click launch path (L004).
5. Keep horizontal keyboard selection visible (L003).
6. Make the first double-click in an inactive Finder window work, after
   verifying the intended period click-through policy (L005).
7. Implement a usable rename/name-entry workflow (F11).
8. Open ordinary documents and provide usable built-in notes editing (F07/K07).
9. Move the complete multiple selection (F08).
10. Fix clipped/overlapping filenames and the grid's wide-label handling (J02/H07).
11. Preserve window dimensions and manual icon placement (F09/F10).
12. Supply item counts, free space, and honest allocated Trash space (F17 and
    2026-10-05:L024).
13. Implement Get Info, Duplicate and everyday clipboard/file commands (F03/F12
    remaining features), including a way to unlock the file named by an alert.
14. Finish shell affordances: meaningful upper menus, application switching,
    Help and context menus (F13-F15/F18).
15. Finish the remaining context/details: prohibit creation inside Trash (L008),
    name the failed Trash open accurately (L007), stamp valid dates
    (2026-10-05:L030), and update the DOS-exit procedure (L009).

## Cleanup and evidence integrity

All started QEMU instances were quit by QMP and waited for by their owning
driver. evidence/cleanup.jsonl and each session JSON record those exits; no
emulator is intentionally left running. Stopped copies checked with fsck.fat -n
are documented in evidence/*-fsck.txt. Extracted payloads and hex/hashes are in
evidence/verification-metrics.json. evidence/final-validation.json records the
final artifact, session, serial-marker and PNG checks.

NITS.tsv contains only this report's nine new findings. The screenshots remain
native-size PNGs. evidence/actions.jsonl, driver scripts, private images and
fixture preparation scripts preserve replayability, including unsuccessful
early attempts; only the explicitly cited, visually checked states support
the conclusions.
''')
report=''.join(parts)
report.encode('ascii')
(R/'REPORT.md').write_text(report,encoding='ascii')
(R/'NITS.tsv').write_text('id\tarea\tseverity\tsummary\n'+''.join('\t'.join(f[k] for k in ['id','area','severity','summary'])+'\n' for f in F),encoding='ascii')
(R/'evidence/findings.json').write_text(json.dumps(F,indent=2),encoding='ascii')
