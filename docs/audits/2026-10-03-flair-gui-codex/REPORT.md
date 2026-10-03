# InitechOS FLAIR desktop review

Audit date: 2026-10-03, approximately 22:53-23:06 Europe/Berlin.

## Verdict

This is an interactive development demo with some real machinery, but it does not meet the owner's bar for an impressive, usable 1990s desktop. Booting, opening disks and folders, launching an executable, switching foreground windows, making directories, and saving some desktop state are real. The window system then undermines those successes: moving a window leaves its contents behind, a scrollbar drags the window instead of scrolling, and several enabled commands acknowledge input without doing anything useful.

The appearance has recognizable Platinum ingredients, especially stripes, dimmed inactive windows, zoom and collapse boxes, and the menu silhouettes. Its tiny fixed-width typography, thin body frames, and skeletal icons give it a cheaper, earlier, more prototype-like appearance than the actual Mac OS 8.1 reference. This is a visual judgment, not a claim of pixel-perfect conformance testing.

No guest panic or spontaneous hang was observed. Three consecutive executable launch/close cycles after the cold reboot completed, with the same reported available heap after each close. That is useful evidence of basic stability, not proof of general reliability.

## Worst problems, ranked

1. **F01: Moving a window does not move its contents.** Files vanish, become clipped, or appear at unrelated positions after dragging, zooming, and restoring.
2. **F02: Scrollbars are window-drag handles.** Overflowing directories cannot be scrolled normally. An ordinary attempt to operate the track moves the whole window.
3. **F03 and F05: Enabled commands are fake.** Get Info, Duplicate, Select All, Restart, and Shut Down accept input but produce no useful visible result.
4. **F04 and F07: Basic file work is blocked.** Trash cannot open, a file cannot be dragged into it from a folder window, and the supplied text document does not open or explain its failure.
5. **F06: Menu context and window activation disagree.** After Finder windows close, Notes appears active, but a blank part of its bar can open Finder's Special menu and replace the displayed bar.

There are no P0 findings in this session. P1 means a core workflow is broken or misleading; P2 means a substantial era feature or interaction is missing or incomplete; P3 means presentation quality.

## Scope, method, and evidence

The main and only guest boot image tested was a private copy of `build/flair_tenants_interactive.img`, accompanied by a private copy of `build/flair_data.img`. There were two boots of those copies. The Makefile's `run-flair-tenants` recipe was inspected, but never executed. No builds, source edits, commits, or issue-tracker writes were performed. A read-only `bd` query failed because its database attempted to open a lock file on the read-only repo; no old issue content was used as evidence.

QEMU used a 486 CPU, 16 MiB RAM, the boot image as the first IDE disk, and the data image as the second IDE disk. It ran with `-display none`. Inputs were sent through QMP to the emulated PS/2 devices, and screenshots came from QEMU's guest framebuffer. No X server was started. Display :0 was neither captured nor given input; no native computer-control tool was used.

Initial image identity:

| Copy | Bytes | SHA-256 before first boot |
| --- | ---: | --- |
| boot.img | 229376 | 656788838a0b9f717b1dcdf163238d692f5ba454a712eb2faec4d219f83f74dc |
| data.img | 1474560 | f51f79e8b75e46c9b9a379285466bffc04a2f4d1c7e4d23bbb2836bfc136dcd7 |

A late comparison of the source data image did not match the initial hash; a subsequent read found that source path unavailable. The repo was being used by other agents, so its build state was not stable during this audit. Findings therefore apply to the copied snapshot identified above, not to an assumed final state of `build/`. The delivered `data.img` contains this audit's folder and desktop-state changes.

The first emulator had `-no-reboot`. An externally requested QMP reset ended that emulator; this is recorded as a harness limitation, not a guest crash. It was replaced by a cold boot of the same disk copies without that flag. Both guests were stopped before any next emulator started. The final guest was shut down with QMP `quit`, and the driver waited for process exit. See [cleanup record](evidence/cleanup.json).

Evidence files:

- [Boot 1 serial log](evidence/serial-boot1.log), [boot 2 serial log](evidence/serial-boot2.log), and [numbered serial index](evidence/serial-index.txt).
- [Input/action journal](evidence/actions.jsonl), which also records the serial byte offset at each logged action.
- [Boot 1 configuration](evidence/session-boot1.json) and [boot 2 configuration and final image hashes](evidence/session-boot2.json).
- [Final FAT root listing](evidence/fat-root-after.txt) and [APPS listing](evidence/fat-apps-after.txt).
- Native 640x480 guest screenshots in [shots/](shots/), with a [filename index](evidence/screenshot-index.txt). There are 76 audit screenshots and four copied reference screenshots.

Screenshot names describe the intended test; the report determines what was actually reached. In particular, `32-context-menu.png` followed an unsupported driver command and is not evidence of a right-click test; the actual test is shot 34. `48-moved-folder.png` was an empty-space marquee, not a folder move. `73-tenant-relaunch.png` clicked the restored APPS title bar; the actual relaunch is shot 74. Screenshots 04 and 07 followed successful menu item selection but show no About dialog.

### Replay convention

Coordinates are guest pixels, with (0,0) at the top left. Unless stated otherwise, start with a fresh copy matching the initial disk layout: INITECH at approximately (600,64), Trash at (600,422), and root entries README.TXT, APPS, DESKTOP.DB, and TRASH. Double-clicking INITECH opens the root at (20,60); double-clicking APPS at (123,101) opens APPS at (40,80).

A menu selection means press and hold its title, move to the named item, then release. The top bar is at y=10; the lower application/Finder bar is at y=28. Clicking and releasing a menu title alone dismisses it in this build. Report coordinates name the initial layout; use the named object if your copied data has saved positions.

[drive.py](drive.py) is the input/screenshot harness used here. It starts one QEMU and accepts JSON commands on standard input, for example:

```text
["click",600,64,2]
["click",123,101,2]
["drag",219,90,190,100]
["shot","replayed-drag"]
["quit"]
```

`drag` takes x, y, delta-x, delta-y. `key` accepts QEMU key names such as `ctrl-n`, `ctrl-w`, and `ret`. The action journal provides the complete session sequence, including held modifier tests. For a fresh replay, run the harness in a separate writable directory with `boot.img`, a fresh `data.img`, and `shots/` and `evidence/` directories. It writes its logs there. Do not replay against the owner's running desktop or original images.

### Expectations and reference provenance

Disk reference material was used directly:

- `/home/tobias/Projects/system7-decomp/specs/sys8/window-chrome.md`: Platinum body frame, title widgets, inactive appearance, and Charcoal system font. Its corresponding [Mac OS 8.1 window capture](shots/ref-mac81-window.png) was visually inspected.
- `/home/tobias/Projects/system7-decomp/specs/sys8/menus.md` and the visually inspected [Mac OS 8.1 File menu](shots/ref-mac81-menu.png): proportional system typography, enabled/disabled commands, and menu appearance.
- `/home/tobias/Projects/system7-decomp/specs/desktop/finder-windows.md`: Finder item/free-space header and view menu. This document explicitly describes System 7/7.5.3 measurements; it is not treated as a Platinum pixel specification.
- `/home/tobias/Projects/system7-decomp/specs/sys8/controls.md`: control appearance, including the title-bar collapse preference. A [control-panel reference](shots/ref-mac81-controls.png) is included for comparison; no guest control-panel rendering was reached.
- A [Windows 3.1 listbox reference](shots/ref-win31-listbox.png) was copied from `win31-decomp/goldens/screenshots/clean/`; no claim of Windows dialog fidelity is made here.

The behavioral expectations below are labeled as either ordinary GUI semantics, a disk reference, or my memory of classic Mac OS/Windows. A memory-based expectation is not presented as a measured historical fact. Repo sources and worklogs were read for launch paths and shortcuts, but a source comment is not evidence that a feature works or is broken.

The stacked bars, teal desktop, busy-hourglass convention, deliberately odd chart numbers, saving-tables wording, and panic phrase are not criticized as design choices. The chart and saving dialog were not present in this tenants image, so their drawing or behavior was not tested. The two boot-time sample tenants are explicitly identified as HELLO and NOTES in the serial log; they are not mistaken for complete productivity applications.

## Findings

### F01 - P1 - Window contents stay at old screen coordinates

**Do:** Open INITECH, then APPS. Launch TENANTFX.EXE by double-clicking its icon near (75,124); click its content to quit. Drag APPS by its title from (219,90) to (409,190), a delta of (+190,+100). Collapse it with its rightmost widget near (578,190), then expand it. Zoom with the adjacent widget near (563,190), then restore using the zoom widget at its new screen position, near (609,50).

**Saw:** APPS became blank after the drag. Collapse/expand did not repair it. Zoom made TENANTFX.EXE reappear near its old screen position (75,124), not relative to the new content origin. Restoring made it disappear again. Moving the larger root window later left whole rows of icons partially clipped under its title and outside its new content rectangle. Closing and reopening the root re-enumerated and drew the icons in the correct new positions.

**Expected and why:** Moving, zooming, or resizing a window must keep its contents attached to its content area. This is ordinary window-system semantics. A user should not have to close a folder to recover its files visually.

**Evidence:** [APPS before moving](shots/13-apps-folder.png), [after moving](shots/17-finder-drag.png), [expanded but blank](shots/19-expanded-apps.png), [zoomed with old icon position](shots/20-zoomed-apps.png), [restored and blank](shots/23-restored-apps.png), [moved root with clipped rows](shots/64-moved-root.png), [reopened root repaired](shots/65-root-location-persistence.png). Boot 1 serial confirms `FLAIR-DRAG ... (40,80)->(230,180)` and `(20,60)->(90,100)`.

### F02 - P1 - Scrollbar input drags the window; overflowing files are unreachable normally

**Do:** In the root, create 18 folders total with the lower File > New Folder command and Ctrl-N. Click the vertical down arrow at (367,251) three times, the up arrow at (367,89), and press Page Down. Drag the track from (367,160) to (367,240).

**Saw:** The directory had 22 entries. Lower rows and labels were clipped at the bottom. Neither arrow nor Page Down changed the view. Arrow clicks logged zero-distance window drags. Dragging the track moved the root window from (20,60) to (20,140). Icons were still tied to old screen positions, exposing a different slice of the same content. The drawn bar had no useful scrolling response or visible position indication for this overflow.

**Expected and why:** Scrollbar arrows, track, and thumb must navigate content without moving the window. This is ordinary control semantics; scrollbars are present in both supplied era reference corpora. Page Down is a secondary convenience expected from my memory of PC GUIs, not the basis of the finding.

**Evidence:** [Overflow](shots/50-overflow-folder.png), [after down-arrow clicks](shots/51-scroll-down.png), [after Page Down](shots/53-page-down.png), [track drag moved window](shots/55-scrollbar-drags-window.png). Boot 1 serial logs `FLAIR-DRAG ... (20,60)->(20,140)` for the track drag. [FAT listing](evidence/fat-root-after.txt) confirms the additional directories really existed.

### F03 - P1 - Get Info, Duplicate, and Select All are enabled no-ops

**Do:** Select README.TXT in the root. Open the lower File menu and observe Get Info and Duplicate in black. Select Get Info near (84,120), then Duplicate near (84,136). Also try Ctrl-I and Ctrl-D. With one root item selected, press Ctrl-A; the lower Edit menu exposes enabled Select All.

**Saw:** No Info window, visible feedback, or duplicate appeared. Ctrl-A left only the previous item selected, despite other visible entries. Serial output logged `GET_INFO`, `DUPLICATE`, and `SELECT_ALL` command dispatches from the tested input paths. The final root listing still contained one README.TXT and no copied document.

**Expected and why:** An enabled command must perform its named operation or explain why it cannot. Get Info appears enabled in the on-disk Mac OS 8.1 File-menu reference. File duplication and Select All behavior are from my memory of classic Finder and ordinary application semantics. A disabled implementation would be more honest than a selectable command with no result.

**Evidence:** [Enabled commands with a selected file](shots/45-selected-file-menu.png), [mouse Get Info result](shots/61-mouse-get-info.png), [mouse Duplicate result](shots/62-mouse-duplicate.png), [Ctrl-A result](shots/30-select-all.png), [Edit menu](shots/41-lower-edit-menu.png), and the command entries in [serial index](evidence/serial-index.txt).

### F04 - P1 - Trash is a dead end; dragging a file to it does not trash the file

**Do:** Move README.TXT within the root to approximately (99,191). Drag it to the desktop Trash at (600,422). Double-click Trash. Check the root and disk contents afterward.

**Saw:** The file was clamped to the root's lower-right area, rather than dropped into Trash. Trash selected, but did not open a window. Serial explicitly reported `FINDER-OPEN-TRASH NYI`. The Special menu offered disabled Empty Trash. The file remained on disk.

**Expected and why:** Drag-to-Trash, inspecting the Trash, and emptying it are fundamental Finder operations, from my memory of classic Mac OS. Keeping a Trash icon that accepts selection but cannot open or receive a folder-window file creates a false affordance.

**Evidence:** [Attempted drop](shots/37-readme-to-trash.png), [Trash double-click result](shots/38-trash-open.png), [disabled Empty Trash](shots/43-lower-special-menu.png), serial `FINDER-OPEN-TRASH NYI`, and [root listing](evidence/fat-root-after.txt).

### F05 - P1 - Restart and Shut Down accept menu selections but do not restart or shut down

**Do:** Choose Restart from the lower Special menu, near (216,108), after the Finder-window tests. Reopen the root and choose Shut Down near (216,124). Then press Ctrl-W.

**Saw:** Each command was logged by name. Restart produced no reboot or confirmation. Shut Down left the root and tenants running; Ctrl-W subsequently closed the root and saved state. There was no shutdown screen, explanation, or transition to DOS. The later reboot was performed externally by the audit harness, not by either menu command.

**Expected and why:** Enabled session-ending commands must act or provide feedback. This is ordinary OS semantics; classic Finder's restart/shutdown behavior is also from my memory. Restart was reached through the retained Finder menu described in F06; Shut Down was tested with an open foreground Finder root.

**Evidence:** [Restart result](shots/67-restart-menu.png), [Shut Down result](shots/69-shutdown-menu.png). Boot 1 serial has `name=RESTART src=mouse`, `name=SHUTDOWN src=mouse`, followed by `CLOSE_WINDOW` and `DESKTOP-DB-SAVE`. No new boot sequence followed those guest commands.

### F06 - P1 - Closing Finder windows leaves an inconsistent active window and menu context

**Do:** Close the root and APPS using Ctrl-W. Observe the Notes window and lower bar. Press and hold at (196,28), which is blank in the displayed `File Edit Notes` bar.

**Saw:** Notes had active stripes and widgets, and the lower bar said `File Edit Notes`. Pressing the blank area opened Finder's Special menu and replaced that bar with `File Edit View Special Help`. Notes still looked active behind the menu. The serial trace identifies the menu as 515. This also made Finder session commands reachable through an apparently unrelated application's empty menu-bar area.

**Expected and why:** The active window, application menu bar, and menu hit regions must agree. A blank bar area must not be an invisible title for another application's menu. This is ordinary GUI semantics, with the classic Mac global menu/application relationship additionally expected from my memory.

**Evidence:** [Notes and its bar after closing Finder windows](shots/66-moved-volume-icon.png), [Finder menu reached through the blank area](shots/68-hidden-special-menu.png). Serial contains `FLAIR-MENU-DROP menu=515` at (196,28).

### F07 - P1 - The supplied README document cannot be opened, and failure is silent

**Do:** Select README.TXT in the root and press Ctrl-O. Observe the desktop. The audit also inspected that copied file with read-only `mtype`.

**Saw:** Ctrl-O logged `FINDER-CMD ... name=OPEN ... sel=1`, but no viewer, chooser, error, or explanation appeared. The file is a real 114-byte text document describing the volume, desktop database, and APPS directory.

**Expected and why:** Opening a document should launch an associated viewer/editor or explain that no suitable application is available. Document associations and a readable bundled README are expectations from my memory of classic Mac/Windows systems. Silent failure prevents the user from distinguishing unsupported files from a broken input path.

**Evidence:** [Ctrl-O result](shots/28-open-readme.png), Boot 1 `OPEN` serial entry, and [FAT root listing](evidence/fat-root-after.txt). This finding does not assert that a complete text editor exists elsewhere in the repo.

### F08 - P2 - Dragging a multi-selection moves only the lead icon

**Do:** In the root, select README.TXT, hold Shift and click APPS at (123,101), then release Shift. Drag README.TXT from (54,101) by (+80,+55).

**Saw:** Both items were selected and serial reported selection count 2. Only README.TXT moved. APPS remained in its original position with its selection highlight. The moved document overlapped the NEWFOL02 icon/label. Special > Clean Up repaired the document's position and logged only one moved item.

**Expected and why:** A group selection should move as a group when one selected member is dragged. This is an expectation from my memory of classic Finder, not a measured behavior in the supplied static references. Free placement can reasonably allow overlap; the broken group movement is the finding.

**Evidence:** [Two selected items](shots/58-shift-selection.png), [only the document moved](shots/59-group-icon-drag.png), [Clean Up repaired it](shots/60-cleanup-after-drag.png). Serial logs count 2, one `FINDER-WIN-DRAG name=README.TXT`, then `FINDER-CLEANUP ... moved=1`.

### F09 - P2 - File-icon placement is lost when its folder closes

**Do:** In the root, drag README.TXT from (54,101) to (99,191), then toward Trash so it ends near the root's lower-right corner. Close the root with Ctrl-W. Reopen INITECH.

**Saw:** README.TXT returned to its original grid cell. The close operation logged a desktop-database save, but the individual file arrangement did not survive the close/reopen cycle.

**Expected and why:** A spatial Finder should remember user-arranged icon locations in a folder. This is from my memory of classic Mac OS. The existing ability to drag icons and save other spatial state strengthens that expectation, but does not prove such persistence was promised by implementation code.

**Evidence:** [Moved icon](shots/31-icon-move.png), [final corner position](shots/37-readme-to-trash.png), [reopened root reset the arrangement](shots/44-root-reopen.png), and `DESKTOP-DB-SAVE` in the Boot 1 serial log.

### F10 - P2 - Saved window position survives, but resized window dimensions do not

**Do:** Move APPS to (230,180). Drag its grow box near (578,390) by (-140,-90), reducing it from 360x220 to 220x130. Close it. Move the root to (90,100), close it, and move INITECH to approximately (480,84). Cold boot the same copied disks. Open INITECH, then APPS.

**Saw:** The volume icon and both folder window locations survived. APPS reopened at (230,180) with its original 360x220 size, undoing the user's 220x130 sizing. The created directories also survived.

**Expected and why:** Remembering folder-window dimensions alongside position is expected from my memory of spatial Finder. It is particularly noticeable because adjacent pieces of the same layout are preserved correctly.

**Evidence:** [Resized APPS](shots/24-actual-resize.png), [volume position after reboot](shots/70-after-cold-reboot.png), [root position after reboot](shots/71-root-after-cold-reboot.png), [APPS reverted size](shots/72-apps-after-cold-reboot.png). Serial reports the grow dimensions and later `DESKTOP-DB-OK` / `DESKTOP-DB-VIEWS n=2`.

### F11 - P2 - New Folder creates directories but provides no usable naming workflow

**Do:** Choose lower File > New Folder in the root. Repeat Ctrl-N 17 more times. Earlier, select a root item and press Return to try the classic Finder rename interaction.

**Saw:** Real directories named NEWFOLD, NEWFOL02 through NEWFOL18 appeared. No name field or prompt appeared after creation. Return did not enter a visible name-editing state. No Rename command was found in the tested File/Edit menus. Creation immediately re-laid out the view, with no naming conversation.

**Expected and why:** A user needs to name folders for real work. Automatically entering name editing after New Folder and Return-to-rename are expectations from my memory of classic Finder. DOS 8.3 limits can explain short names, but do not explain the lack of any reached naming interface. This is limited to the tested creation/Return/menu paths; an undiscovered path is not ruled out.

**Evidence:** [First root folder](shots/46-menu-new-folder.png), [many generated names](shots/50-overflow-folder.png), [Return result](shots/29-rename-return.png), and [actual directories on disk](evidence/fat-root-after.txt).

### F12 - P2 - Alternate views, search, printing, and file-management conveniences are unavailable

**Do:** With APPS open and foreground, inspect the lower View menu. With README.TXT selected in the root, inspect File. Press Ctrl-F. Inspect Edit.

**Saw:** View only offered usable by Icons, Clean Up, and Arrange (by Name); by Small Icon, by Name, by Size, by Kind, by Label, by Date, as Buttons, and as Pop-up Window were disabled. File had disabled Find, Print, Page Setup, Make Alias, Put Away, Sharing, and Eject in the tested context. Ctrl-F opened nothing. Edit had disabled Undo, Cut, Copy, Paste, Clear, Show Clipboard, and Preferences. There was no reached list view or search interface. Disabled commands are missing functionality, not counted as fake enabled commands.

**Expected and why:** The disk Finder-window reference documents alternate sorting/view commands. Find appears enabled in the inspected Mac OS 8.1 menu reference. Usable clipboard/file operations, printing, aliases, and configurable views are additional expectations from my memory of classic Mac OS. Eject was observed disabled for this fixed disk; removable-media ejection was not tested and is not alleged broken.

**Evidence:** [View menu with folder foreground](shots/42-lower-view-menu.png), [selected-file menu](shots/45-selected-file-menu.png), [Edit menu](shots/41-lower-edit-menu.png), [Ctrl-F result](shots/47-find-key.png). Printing hardware and network sharing themselves could not be tested.

### F13 - P2 - Apple-menu access and Help are not usable

**Do:** Press/hold both visible Apple symbols, at approximately (10,10) and (10,28). Inspect the lower Finder Help menu at (250,28).

**Saw:** Neither Apple symbol opened a menu; serial returned menu 0/item 0. Help showed only disabled About Help and Show Balloons. No About This Computer, accessories, Control Panels, or help interface was reached through these expected entry points.

**Expected and why:** Apple-menu access to system information/accessories and Help/Balloon Help are from my memory of classic Mac OS. The on-disk Mac OS 8.1 reference visibly has Apple and Help titles, but its static image alone does not prove the contents or behavior expected here.

**Evidence:** [Lower Apple attempt](shots/33-apple-menu.png), [upper Apple attempt](shots/35-top-apple.png), [disabled Help](shots/36-finder-help.png), and Boot 1 menu-0 results. Appearance, mouse, keyboard, date/time, and other control-panel behaviors remain untested because no entry point was reached.

### F14 - P2 - There is no reached application-switching interface beyond clicking windows

**Do:** Launch TenantFix while HELLO, NOTES, and Finder windows are resident. Press Alt-Tab. Inspect the bar's right end. Also switch by clicking the Notes title.

**Saw:** Alt-Tab was delivered to the tenant as key events and did not change the foreground. No application menu, running-app list, or right-end switcher was displayed. Clicking the exposed Notes title did switch foreground and redraw its active chrome.

**Expected and why:** A running-app menu/switcher is expected from my memory of MultiFinder-era Mac OS; the right-end icon is also visible in the Mac OS 8.1 disk reference. Alt-Tab is a PC/Windows expectation from my memory, not a mandatory Mac shortcut. The material gap is the lack of a discovered switcher when windows obscure one another; the working click-to-switch path is acknowledged.

**Evidence:** [Before Alt-Tab](shots/15-tenant-input.png), [unchanged after Alt-Tab](shots/16-alt-tab.png), [Notes foreground after a click](shots/08-notes-typed.png), and `FLAIR-DISPATCH app=NOTES` in serial.

### F15 - P2 - The upper bar contains copied placeholder menus and an inert About item

**Do:** Press/hold File, Edit, View, and Special in the upper bar. Select upper File > About by dragging from (41,10) to (56,30). Repeat while Finder is foreground.

**Saw:** Each tested upper title exposed the same About / Quit ^Q pair. Selecting About logged item 1 and produced no dialog. The lower Finder bar, by contrast, contained contextual File/Edit/View/Special commands. The existence of two bars is intentional; these duplicated contents and an enabled About with no result are the issue. Quit was not selected, so its behavior is not alleged broken.

**Expected and why:** Visible menu titles should lead to meaningful corresponding commands, or unavailable commands should be disabled. An About item should identify its application/system or state its limitation. This is ordinary UI semantics, independent of the deliberate film-prop two-bar layout.

**Evidence:** [Upper File](shots/03-file-held.png), [upper Edit](shots/05-edit-menu.png), [upper View](shots/06-view-menu.png), [upper Special](shots/12-finder-special.png), [About selection result](shots/07-about-selected.png), and item-1 serial dispatch.

### F16 - P3 - Typography, body frames, and icon art fall short of the Platinum reference

**Do:** Inspect the initial HELLO/NOTES scene, an open root, and the dropped lower File menu. Compare them with the copied Mac OS 8.1 window and menu captures at native size.

**Saw:** Menu/title text has a small fixed-width, widely spaced appearance rather than the reference's substantial proportional system face. Finder's file labels are especially small. Window bodies have very thin border strips, missing the reference's substantial raised picture-frame effect. The title widgets look small and simple next to the reference's shaded widgets. Folder/document/application symbols are sparse outlines and a diamond, rather than the reference's more dimensional icon art. The monochrome disk/trash treatment reinforces the prototype impression.

**Expected and why:** The on-disk Platinum spec identifies Charcoal, a four-pixel raised body frame, and shaded widget faces; the actual capture shows those details and richer icons. There is no demand to copy the reference wallpaper or colors of every icon. The issue is the loss of visual weight and finish within the chosen style. Raw RGB parity was not graded, since the Mac captures have a documented gamma setting.

**Evidence:** [Initial scene](shots/01-boot.png), [Finder root](shots/09-disk-open.png), [FLAIR menu](shots/45-selected-file-menu.png), [real Mac OS 8.1 window](shots/ref-mac81-window.png), and [real menu](shots/ref-mac81-menu.png). This is a comparative aesthetic finding, not a claim that every chrome dimension is wrong.

### F17 - P2 - Finder omits item count and available-space feedback

**Do:** Open INITECH with four entries, then create folders until it has 22 entries. Inspect the region immediately below the title and the rest of the window for a summary.

**Saw:** Icons begin directly below the title. There is no item count, free-space figure, selected-item summary, or disk information strip. The same omission remains after cold boot. The title says Drive A Files rather than displaying the volume name; this generic title further weakens orientation, although the DOS drive-letter personality can reasonably retain A.

**Expected and why:** The supplied Finder-window document explicitly describes an item/disk-space header, and the Mac OS 8.1 reference visibly says `7 items, 416.3 MB available`. This is reference-backed. Counts and space are useful when directories overflow, when creating files, and when selecting a destination.

**Evidence:** [Four-entry root](shots/09-disk-open.png), [overflowing root](shots/50-overflow-folder.png), [after reboot](shots/71-root-after-cold-reboot.png), and [Mac reference header](shots/ref-mac81-window.png).

### F18 - P2 - No contextual menu is reached with right-click or Control-click

**Do:** Right-click empty desktop at (500,380). Later hold Ctrl and click README.TXT at (54,101), then release Ctrl.

**Saw:** The right-click opened no menu and produced no ordinary mouse event in the serial trace. Control-click altered/extended selection instead of showing a contextual menu; serial reported file selection count 2. No contextual commands were displayed.

**Expected and why:** Control-click contextual menus are an expectation from my memory of Mac OS 8; right-click is an additional PC convenience. This is not inferred from the supplied static captures, and the two gestures are not treated as identical historical requirements. A classic Mac Control-click path should be considered if Mac OS 8 is the stated usability target.

**Evidence:** [Actual right-click result](shots/34-right-click.png), [Control-click result](shots/63-control-click.png), and the recorded held-Ctrl input and selection event in the action/serial logs.

## Expected-for-the-era feature status

These statuses apply to the supplied image and this session. `Fake` means a reached affordance/command accepted input without delivering its advertised result. `Absent` means no usable entry point was found in the exercised desktop. `Could not test` deliberately avoids claiming an implementation is missing elsewhere.

| Feature | Status | Session evidence / limit |
| --- | --- | --- |
| Boot to an interactive desktop | works | Both boots reached FLAIR-LIVE-READY. |
| Mounted volume icon and disk opening | works | INITECH opens a real 22-entry directory after creation. |
| Folder double-click and nested windows | works | APPS opens separately. |
| Directory singleton / raise existing folder | could not test | A dedicated repeated-open singleton test was not completed. |
| File selection | works | Named selection appears and logs. |
| Marquee selection | works | Shot 57 selects two icons; serial count 2. |
| Shift-click multi-selection | works | Shot 58; held Shift, count 2. |
| Select All | fake | Ctrl-A leaves the old single selection. F03. |
| Single-icon movement | works | README moves inside its window; cleanup restores it. |
| Group-icon movement | broken | Only the lead icon moves. F08. |
| Persistent folder icon arrangement | broken | Close/reopen resets positions. F09. |
| New Folder creates filesystem directories | works | Menu and Ctrl-N; FAT and reboot confirm persistence. |
| Folder/file naming and rename | absent | No naming state reached after creation or Return. F11. |
| Open a supplied text document | broken | Ctrl-O silently does nothing. F07. |
| Launch an on-disk executable | works | TENANTFX.EXE loads and draws TenantFix. |
| Close app via its go-away box | works | Three consecutive post-reboot cycles; serial exit via=close. |
| Exit app via its own content action | works | TenantFix content click exits via=exit. |
| Productive text editor, spreadsheet, database, paint | could not test | APPS exposed one executable fixture plus audit-created NEWFOLD. |
| Multiple resident applications/windows | works | HELLO, NOTES, Finder, TenantFix coexist. |
| Foreground switch by exposed window click | works | Notes activates with matching serial dispatch. |
| Application switcher / running-app menu | absent | No reached entry point; Alt-Tab ineffective. F14. |
| Active/inactive chrome differentiation | works | Stripes/widgets and dimming change on activation. |
| Window title dragging preserves contents | broken | Geometry changes, content remains at old screen positions. F01. |
| Grow box changes frame dimensions | works | 360x220 -> 220x130 logged. |
| Resize/zoom/restore preserves a usable view | broken | APPS blank after restore; content origin wrong. F01. |
| Collapse and expand frame | works | Rightmost widget hides/restores body. |
| Collapse/expand repairs content | broken | APPS remains blank. F01. |
| Title double-click collapse preference | could not test | No control panel reached; widget tested instead. |
| Remember desktop volume icon position | works | Moved icon survives cold boot. |
| Remember folder window location | works | Root and APPS locations survive cold boot. |
| Remember folder window size | broken | APPS reverts to 360x220. F10. |
| Scrollbar arrows, track, thumb | broken | Input becomes window dragging; no useful scrolling. F02. |
| Horizontal scrolling | absent | No horizontal control in tested folder windows. |
| Alternate list/small-icon views | absent | Disabled in a foreground folder. F12. |
| Clean Up | works | Repairs moved icon and logs moved=1. |
| Arrange (by Name) | could not test | Enabled, but no dedicated sorting result recorded. |
| Get Info | fake | Enabled menu and keyboard route, no Info window. F03. |
| Duplicate | fake | Enabled menu and keyboard route, no copied document. F03. |
| Trash opening | fake | Icon selects; explicit OPEN-TRASH NYI. F04. |
| Move folder-window file to Trash | broken | Drag stays clamped inside source window. F04. |
| Empty Trash / restore trashed files | could not test | Trash could not receive/open; Empty Trash disabled. |
| File copy/cut/paste and Undo | absent | Disabled Edit commands in tested context. F12. |
| Text selection, editing, clipboard, Undo in a real editor | could not test | No productive editor launched. |
| Find File | absent | Disabled menu and ineffective Ctrl-F. F12. |
| Aliases / Put Away | absent | Disabled for selected README. F12. |
| Page Setup / Print | absent | Disabled commands; printing device not exercised. F12. |
| Disk ejection / floppy insertion | could not test | Only fixed IDE disks used. |
| Disk erase / format | could not test | Erase Disk disabled; no disposable removable medium added. |
| Item count and available disk space | absent | No Finder summary strip. F17. |
| Contextual menus | absent | Right-click and Control-click opened none. F18. |
| Apple menu / system About entry point | broken | Both visible symbols return no menu. F13. |
| Upper-bar About | fake | Selected item gives no dialog. F15. |
| Upper-bar Quit | could not test | Visible in every upper menu, not selected. |
| Calculator, Notepad, desk accessories | could not test | No usable Apple-menu launcher reached. |
| Control panels and user preferences | could not test | No usable launcher; Preferences disabled. |
| Help / Balloon Help | absent | Both offered Help items disabled. F13. |
| Clock on the desktop/menu bar | absent | None displayed in the observed scene. |
| Restart | fake | Logged command, no guest reboot. F05. |
| Shut Down / return-to-DOS workflow | fake | Shut Down logged; desktop continues accepting commands. F05. |
| Save/Open file chooser and default/cancel keys | could not test | No actual file dialog reached in this image. |
| Modal dialog blocking and progress cancellation | could not test | Film saving dialog not present in tenants scene. |
| Busy hourglass behavior | could not test | No sustained guest busy state exercised. |
| Sound, printing output, networking | could not test | No relevant setup or output path reached. |
| Sustained application load/close stability | works | Three post-reboot cycles, same heap figure, no panic. |
| General crash recovery / forced quit | could not test | No guest crash or spontaneous hang to recover. |
| Platinum visual finish | broken | Comparative visual shortfall in type/frame/icon finish. F16. |

## What held up, and what remains uncertain

Real filesystem creation survived a cold boot: the root retained NEWFOLD through NEWFOL18, and APPS retained its own NEWFOLD. The desktop database loaded successfully, remembered the moved volume icon and folder origins, and did not cause an observed startup failure. The executable fixture was genuinely loaded from disk, not just painted as a static launch result. Its repeated close path reported `TENANT-HEAPAVAIL n=3524240` each time. These are substantial working foundations.

The supplied scene's HELLO/NOTES tenants are test tenants, and its APPS directory exposes only TenantFix. This review cannot establish whether the larger repo's advertised productivity suite works, nor whether other prebuilt images have different features. No mutant image was booted. The absent film chart/dialog in this particular scene is not listed as a defect. Networking, devices, printing output, full editors, actual file dialogs, accessories, and panic recovery remain untested.

All findings above were observed on the copied images during this session. No repairs were attempted. The emulator and driver are stopped; the report, screenshots, raw evidence, harness, and audited disk copies remain in this working directory.
