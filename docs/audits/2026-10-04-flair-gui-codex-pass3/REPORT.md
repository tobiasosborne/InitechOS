# InitechOS FLAIR desktop audit: third pass

Audit: 2026-10-04, 02:11:59-02:35:04 Europe/Berlin. Findings only; no fixes.

## The six claims

Verdicts describe the copied images and tests below. "Holds" is bounded by this session's coverage, not a proof for every possible input. "Partly holds" identifies either an observed exception or an explicitly unverified part of a compound claim.

| Developer claim | Verdict | Session evidence and limit |
| --- | --- | --- |
| Released Ctrl / Shift no longer stay latched (G11) | **holds** | Explicit press/release followed by plain N did not create a folder; plain W did not close one; plain A was lowercase; sequential ordinary clicks left one item selected. Both left/right modifier combinations worked: releasing left Ctrl while right Ctrl remained held still allowed Ctrl-N, then releasing right Ctrl stopped it. The analogous Shift test produced uppercase then lowercase A. Releases also worked during a held drag and after the sustained app cycling. [Initial Ctrl check](shots/03-plain-n-after-ctrl.png), [Shift check](shots/04-after-shift-release.png), [after cycling](shots/48-post-soak-modifiers.png), [release during drag](shots/101-release-during-drag-modifiers-clear.png); [claims trace](evidence/claims-serial.log), [final hold trace](evidence/final-holds-serial.log). |
| Menus, window drags, grows and icon drags track until release (G01) | **partly holds** | The old expiration was not reproduced. A menu stayed held for 32 seconds and executed New Folder only after release. Window movement stayed an outline for 16 seconds, grow for 8 seconds, and an icon drag for about 11 seconds; their final motions committed on release. However, a longer pointer path followed by a cross-title menu switch selected the wrong command: H01. [32-second menu](shots/103-menu-held-32sec.png), [after release](shots/104-menu-released-after-32sec.png), [held window](shots/97-final-window-held-16sec.png), [released window](shots/98-final-window-release.png), [held grow](shots/99-final-grow-held.png), [released grow](shots/100-final-grow-release.png), [held icon](shots/14-icon-held.png), [settled icon](shots/16-icon-settled.png). |
| Close, zoom and collapse boxes show pressed state and act only on release inside (G02) | **holds** | All three visibly depressed while held, cancelled on release outside, and acted on release inside. Close survived a five-second hold and cancellation, and closed after leaving/re-entering its box. A release at x=116, one pixel beyond the box's right edge, cancelled. Pressing/releasing right mouse while left remained held did not prematurely commit Zoom. [Close pressed](shots/20-close-pressed.png), [outside](shots/21-close-outside-held.png), [cancelled](shots/22-close-cancel.png), [re-entered](shots/31-close-reentered.png), [closed](shots/32-close-commit.png), [Zoom pressed](shots/23-zoom-pressed.png), [Zoom committed](shots/33-zoom-commit.png), [restored](shots/34-zoom-restored.png), [Collapse pressed](shots/26-collapse-pressed.png), [collapsed](shots/28-collapse-commit.png), [edge cancellation](shots/90-close-edge-cancel.png). Trace records `FLAIR-TRACKBOX ... in=0` for cancellations and `in=1` for commits. |
| Icons move into folders, between windows and onto Trash, with outline and target highlight | **partly holds** | All three ordinary moves committed to disk. The outline followed the pointer outside the source window; folder/Trash bodies darkened and labels inverted, then returned when the pointer left. Two same-named trashed files survived as SAME.TXT and SAME001.TXT. Invalid moves gave ambiguous feedback (H02), and moving the service TRASH directory broke subsequent Trash use (H03). Fullness feedback also fails (H04). [Folder held](shots/35-folder-drop-held.png), [target left](shots/36-folder-unhighlight.png), [folder result](shots/38-apps-after-folder-drop.png), [between windows held](shots/40-between-windows-held.png), [result](shots/41-between-windows-release.png), [Trash held](shots/42-trash-held.png), [Trash left](shots/43-trash-left.png), [staged file listing](evidence/claims-trash-after.txt), [collision listing](evidence/collision-trash-after.txt). The outline's rectangular appearance is assessed below. |
| Menu, title and button text is proportional Chicago rather than fixed 8-pixel cells | **partly holds** | The reached menu and title text is visibly proportional and substantially improved; narrow letters no longer occupy the same visible advance as wide ones. The film scene also showed the new-looking proportional text. No booted scene exposed a text-bearing button, so that part is unverified. The actual Mac OS 8.1 captures use Charcoal; Chicago is an earlier-Mac look, not the same finish. Default filename layout also fails with wide legal names (H07), although filename text is a separate surface from this literal claim. [FLAIR menu](shots/06-menu-held-15sec.png), [title/application text](shots/86-late-app-own-menu.png), [film text](shots/95-film-static-font-check.png), [real Mac menu](shots/ref-s8_menu_dropped.png), [Appearance panel specifying Charcoal](shots/ref-s8_controls_dialog.png). |
| A disk-launched app shows its own menu bar | **holds** | TenantFix displayed `File Edit Fixture` on launch. Activating Notes changed it to `File Edit Notes`; returning to TenantFix restored its own bar. This also survived the repeated launch/Close run. [TenantFix](shots/86-late-app-own-menu.png), [Notes](shots/87-switch-notes-menu.png), [TenantFix restored](shots/88-app-menu-restored.png); [metadata trace](evidence/metadata-serial.log). This verdict concerns bar ownership/display. The already-filed G03 menu-command issue was not re-tested or re-filed. |

## New findings

P1 means a core operation is wrong or breaks a service. P2 means a substantial usability or consistency defect. P3 means a drawing defect. No spontaneous guest panic, reboot or whole-desktop hang was observed.

### H01 - P1 - A long cross-menu gesture executes a different command from the highlighted one

**Do:** Use a fresh normal copy. Open INITECH at (600,64). Hold lower File at (42,28), move to (60,50), then alternate relative moves (+4,0) and (-4,0) 40 times, pausing 0.08 seconds after each move. Still holding, move to View at (119,28), wait 0.4 seconds, then move to its first item at (135,50). Capture, release, and wait one second. [replay_menu.py](replay_menu.py) performs this exact reproduction and a short control gesture; run it in a separate writable copy of the audit harness and pristine images.

**Saw:** View was the visible dropped menu and `by Icons` was highlighted. Releasing created NEWFOLD. The short control selected `VIEW_ICONS`, with result `0x02020001`; the long gesture selected File > New Folder, result `0x02000001`, and logged `FINDER-NEW-FOLDER name=NEWFOLD parent=0`. The same failure first appeared after 80 jitter pairs in the longer initial session. Both guests remained responsive. This is not the previously filed timeout: tracking continued and the error occurred on release.

**Expected and why:** Release should execute the highlighted item in the currently visible menu. That follows ordinary menu semantics and the local Menu Manager description of returning the chosen menu/item. Pointer history must not make the visible choice disagree with dispatch. The demonstrated unintended action was directory creation; no unsaved-document loss was demonstrated.

**Evidence:** [Short control held](shots/78-short-cross-menu-held.png), [control result](shots/79-short-cross-menu-result.png), [long gesture visibly selecting View](shots/80-long-cross-menu-held.png), [wrong-command result](shots/81-long-cross-menu-wrong-command.png), [fresh reproduction trace](evidence/menu-repro-serial.log), [replay console](evidence/menu-replay-console.log). Initial reproduction: [held View](shots/45-menu-overflow-visible-view.png), [unexpected folder](shots/46-menu-overflow-result.png), [claims trace](evidence/claims-serial.log).

### H02 - P2 - Refused moves highlight their targets but give no explanation

**Do:** Boot `images/edge-data.img`, the valid FAT fixture containing root SAME.TXT and another SAME.TXT in B, plus A/CHILD. Open INITECH. Drag root SAME.TXT from (54,153) onto folder B at (261,101), hold three seconds, then release. Also test a window-body drop onto B's existing SAME.TXT: open B, move its window to (390,240), close/reopen it to populate at that position, and drop the root file at (434,284). For the folder-cycle case, open A, place/reopen it at (390,60), and drag root A from (192,101) onto CHILD at (425,104).

**Saw:** B and CHILD darkened and inverted their names while held. Release left the source items in place, with no alert or explanation. Serial identified `reason=exists name=SAME.TXT` and `reason=cycle name=A`. The name collision did not overwrite the destination, and the cycle did not damage the tree. A later legitimate A-to-B move worked, and NEWFOLD could be created inside the still-open moved A. Thus the refusals were real safeguards, not a broken general move path.

**Expected and why:** Explain a name collision or an invalid folder destination so the user can choose another operation. A return to the source alone does not distinguish name conflict, folder cycle and storage failure. Visible error identification is an ordinary GUI expectation; my expectation of classic Finder name-conflict alerts comes from memory, not a paired historical capture. The local drag-feedback research calls for a return outline on refusal; the missing explanation is an additional usability judgment. This is the new move path on a healthy disk, separate from G05's full-disk creation failure.

**Evidence:** [B highlighted](shots/54-duplicate-folder-highlight.png), [refused without explanation](shots/55-duplicate-folder-refused.png), [window-body refusal](shots/53-duplicate-refused.png), [CHILD highlighted](shots/57-cycle-child-highlight.png), [cycle refused](shots/58-cycle-refused.png), [trace](evidence/edges-serial.log), [fixture preparation](evidence/edge-disk-preparation.json), [intact moved A](evidence/edges-before-reboot-b-a.txt), [intact CHILD](evidence/edges-before-reboot-b-a-child.txt).

### H03 - P1 - Moving the service TRASH directory breaks Trash and splits its contents after reboot

**Do:** Fresh normal boot, open INITECH. Drag README.TXT from (54,101) to desktop Trash (600,422); release and wait. Then drag the root directory named TRASH from (261,101) into APPS at (123,101). Press Ctrl-N and try dragging the resulting NEWFOLD at (54,101) to desktop Trash. Stop QEMU and cold boot a copy of that modified data disk. Open root TRASH at (261,101), then APPS and its TRASH at (143,124). Finally close those folder windows and retry trashing root NEWFOLD.

**Saw:** README initially staged successfully. The system accepted moving TRASH into APPS. The next drop still highlighted desktop Trash, but NEWFOLD remained in root and serial said `FINDER-MOVE-REFUSED reason=err`. There was no visible service-error explanation. The stopped disk had no root TRASH; APPS/TRASH contained README.TXT. After reboot, a new root TRASH appeared and opened empty, while APPS/TRASH still displayed README. The retry staged NEWFOLD into the new root TRASH. Final disk listings therefore contain two separate Trash directories: root TRASH with NEWFOLD, APPS/TRASH with README.TXT.

**Expected and why:** An ordinary Finder drag should preserve the Trash service and its existing staged items, or visibly reject moving its managed directory. My expectation that trashed items remain in the same Trash across a restart comes from classic Mac memory; continued service operation and consistent storage identity are ordinary filesystem/UI expectations. The old README was not deleted: it became stranded in a different directory from the desktop service's new staging area. This is separate from the already-filed dead desktop Trash-open command.

**Evidence:** [README staged](shots/68-trash-service-readme-staged.png), [service directory moved](shots/70-trash-directory-moved.png), [failed later drop](shots/72-second-item-staged-relocated-trash.png), [TRASH inside APPS](shots/73-relocated-trash-in-apps.png), [new root TRASH empty after reboot](shots/75-reboot-root-trash-open.png), [old TRASH still contains README](shots/76-reboot-relocated-trash-has-readme.png), [original-session trace](evidence/trash-service-serial.log), [reboot trace](evidence/trash-reboot-serial.log), [root absent before reboot](evidence/trash-service-before-reboot-trash.txt), [old contents before reboot](evidence/trash-service-before-reboot-apps-trash.txt), [new contents after reboot](evidence/trash-service-after-reboot-trash.txt), [old contents after reboot](evidence/trash-service-after-reboot-apps-trash.txt).

### H04 - P2 - Trash still looks empty after successful staging

**Do:** On a fresh normal boot, open root and move the pointer to (500,380); capture the empty Trash. Drag DESKTOP.DB from (192,101) onto Trash at (600,422), wait, move the pointer back to (500,380), and capture again. Separately stage README.TXT, and stage the two SAME.TXT files in the collision fixture.

**Saw:** Successful staging removed the source entries and was confirmed by the stopped disks. The Trash body and label stayed exactly the same. With the cursor clear of the icon in both frames, RGB bytes for rectangle (576,395)-(625,456) were identical before/after the DESKTOP.DB drop. The two-file collision test also ended with the same empty-can artwork, even though TRASH held SAME.TXT and SAME001.TXT.

**Expected and why:** A nonempty Trash should visibly indicate that it contains items. That full/empty distinction comes from my memory of classic Mac Finder; the supplied static Mac captures do not provide a paired empty/full state for exact pixel comparison. This finding concerns changing status after a real successful operation, not the already-filed general quality of the icon artwork or Empty Trash functionality.

**Evidence:** [Empty state](shots/82-trash-empty-clean-pointer.png), [nonempty state](shots/83-desktop-db-trashed-absent-from-view.png), [pixel comparison](evidence/trash-icon-comparison.json), [staged DESKTOP.DB](evidence/metadata-trash-after.txt), [two-file result](shots/94-trash-collision-result.png), [two staged names on disk](evidence/collision-trash-after.txt).

### H05 - P2 - Trashing DESKTOP.DB recreates it without updating the open folder's inventory

**Do:** Fresh normal boot, open INITECH and drag DESKTOP.DB at (192,101) to Trash (600,422). Wait one second. Close the root with Ctrl-W and reopen INITECH. Open the root TRASH directory to inspect the staged file.

**Saw:** The drop logged `FINDER-TRASH name=DESKTOP.DB origin=0` followed by `DESKTOP-DB-SAVE n=3`. DESKTOP.DB disappeared from the open root view, leaving a gap. Closing/reopening root made DESKTOP.DB reappear. The TRASH window also displayed DESKTOP.DB. The stopped disk contained a regenerated 128-byte root DESKTOP.DB and the original 8-byte copy in TRASH. The live view did not disclose the immediate regeneration. This occurred on a healthy disk with ample space.

**Expected and why:** Keep the visible directory inventory consistent with regenerated managed files, or clearly refuse/explain an operation on them. A successful-looking Trash operation should not silently turn into deletion plus hidden recreation. This is ordinary UI/filesystem consistency, not a demand for a specific Mac metadata format. The trigger is the new Trash path, distinct from G05's full-disk phantom-file case.

**Evidence:** [Root omits DESKTOP.DB after drop](shots/83-desktop-db-trashed-absent-from-view.png), [it returns after reopen](shots/84-desktop-db-reappears-after-reopen.png), [original copy in TRASH](shots/85-old-desktop-db-in-trash-folder.png), [root disk listing](evidence/metadata-root-after.txt), [Trash disk listing](evidence/metadata-trash-after.txt), [trace](evidence/metadata-serial.log).

### H06 - P3 - TenantFix paint cuts into the horizontal scroll gutter after Zoom

**Do:** Fresh normal boot. Open INITECH, APPS, and TENANTFX.EXE at (600,64), (123,101), and (75,124). Click Zoom at (373,170), wait one second and move the pointer clear to (500,380). Restore at (609,50), then zoom again and capture.

**Saw:** The zoomed frame had an abrupt white cutout across the upper part of its bottom horizontal scroll gutter. The cutout ended at x=405; the rest of the well remained gray. It reproduced on the second zoom and had appeared earlier in the initial session. TenantFix's repaint trace records its ordinary 400x400 white fill. The problem is the drawn boundary between app content and chrome, independent of the already-filed fact that the scrollbars do nothing.

**Expected and why:** Clip application painting to the content area, preserving a continuous scroll gutter. This is ordinary window-system semantics. The real Mac OS 8.1 document capture also visibly separates the white content area from the intact horizontal bar. No claim about the scroll position or thumb function is made here.

**Evidence:** [First fresh zoom](shots/106-app-zoom-horizontal-gutter-overpaint.png), [second zoom](shots/107-app-second-zoom-gutter-overpaint.png), [initial-session occurrence](shots/33-zoom-commit.png), [paint/zoom trace](evidence/paint-clip-serial.log), [real Mac window](shots/ref-s8_doc_window_active.png).

### H07 - P2 - Legal wide filenames overwrite each other in the default icon grid

**Do:** Copy the pristine data disk and add two ordinary text files named WWWWWWWW.WWW and MMMMMMMM.MMM, using mcopy as recorded in [preparation evidence](evidence/wide-disk-preparation.json). Boot the normal kernel with that disk. Open INITECH without moving any icon, capture, then select WWWWWWWW.WWW at (192,101) and capture again.

**Saw:** The two default-grid labels ran together. The M label's background/text overwrote the end of the W filename. Selecting the W file did not expose its complete name: the neighboring label also covered the end of its black selection band. Both complete 8.3 names exist on disk, and serial identifies the selected item as `WWWWWWWW.WWW`. No manual placement or group drag caused this layout.

**Expected and why:** Keep automatically placed filenames readable and distinguishable, including ordinary maximum-length DOS names with wide glyphs. Wrapping, abbreviation or sufficient spacing would meet that expectation. This is ordinary naming/selection usability; the real Mac reference also shows separated, sometimes multi-line icon labels. It is a concrete overlap in automatic layout, separate from F08's group drag and F16's general font/icon appearance criticism.

**Evidence:** [Default grid](shots/108-wide-names-default-grid.png), [selected filename still obscured](shots/109-wide-name-selected.png), [complete disk names](evidence/wide-labels-root-after.txt), [input/selection trace](evidence/wide-labels-serial.log), [Mac labels](shots/ref-s8_doc_window_active.png).

## Appearance judgment against the captures on disk

The new menu/title text is a substantial improvement over the earlier fixed-cell appearance. It reads as bold, proportional period Macintosh UI. The File menu is now compact and legible, and the title text sits naturally in the striped band. At native 640x480 size, the Mac OS 8.1 reference has a smoother, less mechanically blocky Charcoal shape; its Appearance panel explicitly names Charcoal. The System 7 Chicago menu capture supplies useful earlier-era context. Calling the new text Chicago therefore describes its intended face, but does not make it a complete Mac OS 8.1 typography match. I did not certify the strike as byte-identical to Apple's Chicago resource.

Compared frames: [FLAIR held File](shots/06-menu-held-15sec.png), [FLAIR TenantFix](shots/86-late-app-own-menu.png), [Mac OS 8.1 File](shots/ref-s8_menu_dropped.png), [Mac OS 8.1 title](shots/ref-s8_doc_window_active.png), [Charcoal preference](shots/ref-s8_controls_dialog.png), [System 7 Chicago context](shots/ref-s7_menu_edit.png). Button typography remains unverified: the film saving panel had text and a progress bar, with no text-bearing button.

The new drag feedback works visually as feedback: the original icon remains in place, a gray outline follows the pointer beyond its source window, and a hovered folder or Trash visibly darkens. Target darkening and label inversion are clear in [folder hover](shots/35-folder-drop-held.png) and [Trash hover](shots/42-trash-held.png). The outline is a thin rectangle around the entire icon/label cell; [cross-window drag](shots/40-between-windows-held.png) shows an empty box rather than a recognizable dragged file silhouette/name. It looks rudimentary beside the substantial, detailed icon cells in the [Mac window capture](shots/ref-s8_doc_window_active.png). My expectation of an outline retaining the icon/name shape comes from memory and the local historical passages, not a matching animated Mac capture. No supplied capture here records a live Finder drag, so pixel-perfect drag parity cannot be established.

The local [drag-feedback research](evidence/references/finder-drag-feedback-ground-truth.md) quotes period descriptions of an outline representation of the icon and filename and of highlighted destinations. It also documents InitechOS's deliberate whole-cell-rectangle interpretation. I consider that a functional approximation with limited visual finish. H04 is the independent observed failure of the Trash's state indication. The already-filed general body-frame and icon-art criticisms are not new findings.

## Scope, identity, replay and exclusions

I read both prior REPORT.md and TRIAGE.md files and both QMP drivers before testing. Their F01-F18 and G01-G11 findings and triage corrections were treated as the known baseline. In particular, detached Finder contents, inert scrolling, unavailable commands/features, existing menu-command failures, saved-size/placement omissions and window limits are not re-filed here.

All interactive tests used private copies of `build/flair_tenants_interactive.img` with a copied `flair_data.img` or a derived valid FAT data disk. The brief identifies the build as commit 52ede68; the copied bytes are the actual identity used for the conclusions:

| Pristine copy | Bytes | SHA-256 |
| --- | ---: | --- |
| images/pristine-boot.img | 229376 | 68ab384de718e2948195a9eebdffeefd9956df97ba71842ea23295e65b6e2b61 |
| images/pristine-data.img | 1474560 | 84390fdb93bfe532d7e927a0bd83d918ec6f00c71c7bf5c368d56e09e2f4c4e4 |

See [image manifest](evidence/image-manifest.json) and each `*-session.json` for exact QEMU arguments and before/after image hashes. The static `flair_desktop.img` copy was additionally booted only for the film text frame. Its intentional halt was not counted as a hang. The copied `flair_live.img` was not booted. No mutant kernel was used.

QEMU used `-cpu 486 -m 16 -display none`, two IDE images for the desktop tests, QMP input into emulated PS/2 devices, and QMP screendumps of the guest framebuffer. Display :0 was never captured or given input. No native desktop automation was used. No repo edit, build, make invocation, commit, issue-tracker mutation or remote write was performed. All report artifacts and modified image copies are in this working directory.

There were 11 sequential QEMU instances and 109 guest screenshots plus five copied reference captures. The longest continuous guest session lasted 503.06 seconds (8 minutes 23 seconds). It included 60 scheduled launch/Close cycles, with 62 successful app loads and 62 Close exits total including the earlier widget tests. Every reported teardown had `TENANT-HEAPAVAIL n=3523968`. This supports reclamation and bar stability in that path, not general leak freedom or an overnight soak. See [claims summary](evidence/claims-summary.json), [session timing/cleanup summary](evidence/audit-summary.json) and [raw claims trace](evidence/claims-serial.log).

All coordinates are guest pixels. `drag` in [audit_driver.py](audit_driver.py) means x, y, delta-x, delta-y. `move` homes the emulated pointer before moving to an absolute guest point; do not use it within an ongoing gesture, where `rel` is appropriate. `button` sets an explicit state; `keyheld` supplies explicit QEMU key state. For a held gesture, issue press, relative moves/pauses, then release. The complete [action journal](evidence/actions.jsonl) records UTC timestamps, session names and serial byte offsets for each operation. The [serial index](evidence/serial-index.txt) points into raw traces; the [screenshot index](evidence/screenshot-index.json) records dimensions and hashes.

Run replays in a separate writable directory containing copies of the harness and relevant `images/` inputs, with `shots/` and `evidence/` directories. For example:

```text
python3 -u audit_driver.py
["start","replay","images/pristine-boot.img","images/pristine-data.img"]
["click",600,64,2]
["drag",54,101,546,321]
["pause",1]
["shot","readme-staged"]
["quit"]
```

The edge fixture's preparation commands/listings are in [edge-disk-preparation.json](evidence/edge-disk-preparation.json). Wide-name preparation is separately recorded above. Neither fixture is corrupt, full or missing the shipped executable.

Evidence corrections: screenshots 12-13 were a desktop marquee, and 17-18 were a frame-border drag, not grow-box tests. The real grow tests are 29-30 and 99-100. Screenshots 07-08, 11 and 15 caught release-time composition before settling and are not used as completed-operation proof. The intended Trash attempts in screenshot 62 actually landed in the B window, which covered desktop Trash; its filename is not an assertion of successful staging. The clean service reproduction starts at screenshot 66. Screenshot 72 shows a failed drop, despite its intention-based filename. These distinctions are supported by the raw action/serial logs and the inspected frames.

Every screenshot cited above was opened and visually checked against the statement using it. Held-state conclusions additionally rely on the explicit button-state journal and dispatch ordering; a single still image alone does not prove a button is down.

Reference provenance: captures were copied directly from `/home/tobias/Projects/system7-decomp/goldens/captures/`. The locally read [Mac OS 8.1 menu specification](evidence/references/menus.md), [window chrome specification](evidence/references/window-chrome.md), and [Window Manager reference](evidence/references/window-manager.md) and [Menu Manager reference](evidence/references/menu-manager.md) supply the typography/chrome context and release-inside behavior. Memory-based expectations are labeled in the relevant findings. No historical interactive Mac session was run, and no raw RGB parity claim is made against the gamma-adjusted Mac captures.

The stacked bars, teal desktop, hourglass convention, odd pie total, `570-`, saving-dialog wording and panic phrase were not treated as defects. No pie chart or `570-` figure was reached. Productive editors, real Open/Save dialogs, text-bearing buttons, printing, networking, audio and removable-media behavior remain outside this session's evidence.

All 11 QEMU instances were quit through their own QMP connection and waited for; each exited with status 0. The interactive audit driver and replay driver exited with status 0. No external process was killed, and no QEMU remained in the final process inventory. The desktop is improved, but wrong menu dispatch and a breakable Trash service remain the most consequential new failures; the report records seven new findings.
