# InitechOS FLAIR desktop audit: second pass

Audit dates: 2026-10-03 to 2026-10-04, Europe/Berlin. Guest tests ran from 23:37:27 to 00:04:37 local time. This is a findings report, not a repair.

## Worst problems, ranked

1. **G11 - P1: Released modifiers stay latched.** Plain N creates a folder after Ctrl is released; plain W closes a window. Released Shift still extends selection and capitalizes keys.
2. **G01 - P1: Mouse capture expires during ordinary gestures.** A menu dismisses while the button is held; lingering over New Folder executes it before release. A slow window drag commits early and ignores the rest of the drag.
3. **G02 - P1: Pressing Close immediately terminates the app.** Dragging away before releasing cannot cancel the close.
4. **G03 - P1: The disk application's own menu commands do not work.** TenantFix's enabled Quit and About items accept selection but produce no result; Ctrl-Q also does not quit.
5. **G04 and G05 - P1: Directory completeness and disk-full handling fail.** A 69-entry directory drops five entries without a visible warning. A full disk fails silently and displays an uncommitted DESKTOP.DB file.
6. **G06-G10 - P2:** A four-folder-window ceiling silently blocks further navigation; keyboard selection is missing; activation clicks execute inactive app content; app failures have no visible notification; Arrange by Name does not arrange the icons.

P1 means a core workflow is broken or misleading. P2 means a substantial interaction or recovery gap. No spontaneous guest panic or unplanned whole-desktop hang was observed. The deliberate crashing app fixture is discussed separately in G09.

## Scope and method

I read the first pass's REPORT.md, TRIAGE.md, and drive.py before testing. The triage's correction to F06 is accepted: its lower bar was Finder's bar, not an invisible menu title. This report does not re-file F01-F18. In particular, the known detached Finder contents, fake scrollbars, missing Trash workflow, disabled features, fonts/icons, and saved-size omissions are not new findings here.

The principal boot image was a private copy of `build/flair_tenants_interactive.img`, with private data disks. The brief identifies these builds as newer than the first pass and built from commit 1cf74a6. Because other people are changing the repo, the copied bytes, not a later source checkout, identify this audit. The boot image's SHA-256 is `507f78cdf2beae89a8e9176d18d75ec415ee8f053841c3c117c23bcbb15711c3`; the pristine `flair_data.img` copy is `84390fdb93bfe532d7e927a0bd83d918ec6f00c71c7bf5c368d56e09e2f4c4e4`. See the full [image manifest](evidence/image-manifest.json) and per-session configuration files under [evidence/](evidence/).

QEMU ran with `-cpu 486 -m 16 -display none`. All input went to QMP's emulated PS/2 devices; every audit screenshot is QEMU's guest framebuffer at 640x480. No input or screenshot operation touched X display :0. The repo was only read. There were no builds, make invocations, edits, commits, or issue-tracker writes. Only this working directory was written. Each QEMU was stopped before starting the next; the final cleanup record is linked below.

Booted image families:

| Boot image | Data | Work performed |
| --- | --- | --- |
| flair_desktop.img | none | Static film-scene rendering |
| flair_live.img | none | Timed modal edge drags, blocked background menu input, Return/Escape |
| flair_live_nomodal.img | none | Timed nonmodal film-window tests |
| flair_tenants_interactive.img | flair_data.img | New app menus, gesture capture, widget cancellation, edges, overlap, keyboard use, sustained cycling and odd input |
| flair_tenants_interactive.img | folders-data.img | Four-window limit and repeated-open singleton behavior |
| flair_tenants_interactive.img | many-data.img | 69-entry directory, dropped entries, Arrange by Name |
| flair_tenants_interactive.img | full-data.img | Zero-free-space boot, New Folder, storage failure feedback |
| flair_tenants_interactive.img | flair_data_tfx_crash.img | Deliberate app exception and desktop recovery |
| flair_tenants_interactive.img | flair_data_tfx_noreg.img | Deliberately unregistered app launch refusal |

The copied `flair_tenants.img` and bad-menu data fixture were inventoried but were not booted. No mutant kernel was tested. The available film images have deliberate bounded lifetimes, and the static scene deliberately halts after rendering. The Makefile's interactive film image, `flair_live_interactive.img`, was absent at copy time; I did not build it. Film interaction conclusions therefore concern short timed tests, not a long-running film desktop. The deliberate halt of a bounded demo is not reported as a hang. See [boot recipe notes](evidence/makefile-boot-notes.txt).

The film saving dialog, stacked bars, teal background, hourglass convention, odd chart numbers, and panic phrase are not criticized as design choices. The tested film frame had empty document bodies and a saving progress dialog, not a displayed pie chart or 570- figure. There was no reached editable modal, real file chooser, checkbox, radio button, or popup control. Their behavior remains untested, rather than inferred from library source or host tests.

### Evidence and replay

[audit_driver.py](audit_driver.py) adapts the first pass's QMP driver. Run `python3 -u audit_driver.py` in this directory and send one JSON command per line. Its `start` command makes another private copy, starts one QEMU, and waits for the boot marker. For example:

```text
["start","replay","images/flair_tenants_interactive.img","images/flair_data.img"]
["click",600,64,2]
["click",123,101,2]
["click",75,124,2]
["shot","replay-app"]
["quit"]
```

Coordinates are guest pixels. `drag` takes x, y, delta-x, delta-y. `rel` moves relative to the current pointer. `button` sets the left button state unless another button is named. `keyheld` sets a QEMU key's explicit state. `menu` presses a title, moves to an item, and releases. `pause` is seconds. `batch` runs a list of commands in order. Keep individual JSON lines below the PTY line limit; the action journal is a record of actual method calls, not a blindly executable list of high-level gestures.

The [action journal](evidence/actions.jsonl) records session names, UTC timestamps, inputs, screenshots, and serial byte offsets. Each `*-serial.log` is raw guest output. Each `*-session.json` records QEMU arguments, PIDs, start/stop time, and disk hashes before/after. [prepare_disks.py](prepare_disks.py) creates the awkward disks from the pristine copy with mtools; the exact commands and hashes are in [disk preparation evidence](evidence/disk-preparation.json). These preparations do not modify repository images.

See also [harness notes](evidence/harness-notes.txt) for mistargeted, transient, and rejected harness steps that were excluded from findings. In particular, shots 18-20 are not the actual close-cancellation test, and shot 35 is not a successful Arrange selection. The findings cite checked frames and the corresponding input/serial evidence.

### Expectation provenance

The on-disk Window Manager reference explicitly describes TrackGoAway as returning true only if released inside. It also describes selecting a window to bring it forward, and the separate track/apply operations for drag and grow. A copy is preserved at [window-manager.md](evidence/references/window-manager.md). The [Dialog Manager reference](evidence/references/dialog-manager.md) describes modal event routing and default/cancel items, while the [Platinum controls reference](evidence/references/platinum-controls.md) and [window chrome reference](evidence/references/platinum-window-chrome.md) supplied appearance context.

Other behavioral expectations are labeled below. In particular, type-to-select, arrow navigation, and consuming the first click on inactive Mac content come from my memory of classic Mac OS, not an interactive historical reference run. An enabled named command producing its advertised result, keeping mouse capture until release, and explaining errors are ordinary GUI expectations. This report makes no pixel-perfect Mac OS conformance claim.

## Findings

### G01 - P1 - Menus and window drags expire while the mouse button is still down

**Image:** `flair_tenants_interactive.img`; normal and full private data disks.

**Do, menu dismissal:** Open INITECH. Move to lower View at (125,28), press the left button, wait 0.2 seconds, capture, then wait two seconds and capture again without releasing. Move to (190,214), the Arrange item, then release.

**Do, premature command:** On a normal disk with the root open, press lower File at (42,28), move by (+35,+22) onto New Folder, wait 0.2 seconds and capture, then wait two seconds and capture while still holding. Finally release.

**Do, slow drag:** From a fresh normal boot, press HELLO's title at (200,70). After 0.2 seconds, move by (+50,+60), wait 0.2 seconds and capture the outline. Wait two seconds without releasing and capture. Move another (+80,+50), then release.

**Saw:** The View menu disappeared before release, and the later release selected nothing. New Folder executed before release: NEWFOLD appeared while the button was still held. HELLO's drag outline became a committed window at (110,120) before release, and the final (+80,+50) motion was ignored. The serial command and drag markers precede their mouseUp events. These are not consequences of a bounded demo ending; the interactive desktop continued accepting input.

**Expected and why:** A held menu or drag must keep tracking until release or explicit cancellation. A user can reasonably take more than 1.5 seconds to read a menu or place a window. Committing a command before release removes the user's opportunity to move away and cancel. This is ordinary GUI semantics; the on-disk Window Manager reference also separates tracking from applying a move/size. The observed cutoff is approximately 1.5 seconds, and the replay uses two seconds to cross it reliably.

**Evidence:** [Menu initially held](shots/39-menu-held-initial.png), [same hold after timeout](shots/40-menu-expired-while-held.png), [New Folder highlighted while held](shots/45-new-folder-item-held.png), [folder created before release](shots/46-new-folder-executed-before-release.png), [drag outline](shots/42-drag-held-before-timeout.png), [committed before release](shots/43-drag-committed-before-release.png), [final motion ignored](shots/44-slow-drag-final.png). Raw [full-disk trace](evidence/full-disk-serial.log) and [slow-gestures trace](evidence/slow-gestures-serial.log) show the ordering.

### G02 - P1 - The Close box terminates on mouse-down and cannot be cancelled

**Image:** `flair_tenants_interactive.img` with the normal data disk.

**Do:** Launch TenantFix. Its normal frame is (100,160)-(400,340). Move to its Close box at (110,170), press and keep holding for 0.5 seconds, then capture. Move by (+150,+100), outside the box, release, and capture again.

**Saw:** TenantFix disappeared while the button was still held. Serial logged `TENANT-EXIT rc=0 via=close`, heap reclamation, and `FLAIR-CLOSE` before the mouseUp at (260,270). Dragging away did not cancel; this was actual app teardown, not just a temporary hidden frame.

**Expected and why:** The close box should track its pressed state and commit only on release inside. This expectation is explicitly supported by TrackGoAway in the on-disk Window Manager reference. Press-drag-out is a normal way to recover from a mistaken press. The fixture has no unsaved document, so data loss was not demonstrated, but the system applies the same premature termination gesture to an application window.

**Evidence:** [Before pressing](shots/21-tenant-before-close-cancel.png), [gone while held](shots/22-close-while-held-real.png), [after attempted cancellation](shots/23-close-cancel-failed-real.png), and [tenant trace](evidence/tenants-main-serial.log).

### G03 - P1 - The disk app's new Quit and About menus accept input without acting

**Image:** `flair_tenants_interactive.img` with `flair_data.img`.

**Do:** From a fresh layout, double-click INITECH (600,64), APPS (123,101), and TENANTFX.EXE (75,124). Select its lower File > Quit by pressing (42,28), moving to (62,56), and releasing. Try Ctrl-Q. Select lower Fixture > About TenantFix by pressing (140,28), moving to (182,52), and releasing. Wait, then drag TenantFix's title to verify it remains live.

**Saw:** The bar really changed to `File Edit Fixture`, and Quit was visibly enabled with `^Q`. The trace recorded menu 129/item 1 and menu 131/item 1. Neither selection closed the app or opened an About dialog. Ctrl-Q was delivered as tenant key events and left it running. The subsequent drag caused live TenantFix repaint calls. Its Close box and content-to-quit action worked in other tests.

**Expected and why:** An enabled Quit command should quit, and an enabled About item should identify the app or give a useful result. This is ordinary named-command semantics. This is the newly supplied app-owned bar, distinct from first-pass F15's upper-bar placeholders and F03's Finder commands.

**Evidence:** [App and its own bar](shots/06-disk-app-own-bar.png), [enabled Quit](shots/11-own-file-held.png), [after About and prior Quit attempts](shots/13-own-about-no-effect.png), [still live during a subsequent move](shots/14-tenant-edge-top-left.png), and [tenant trace](evidence/tenants-main-serial.log).

### G04 - P1 - Folder enumeration silently drops entries beyond 64

**Image:** `flair_tenants_interactive.img` with `many-data.img`.

**Do:** Make the data copy using prepare_disks.py. It contains README.TXT, APPS, X000.TXT-X063.TXT, then folder AAAAA, all valid FAT entries. Boot; the guest adds DESKTOP.DB and TRASH, bringing the root to 69 entries. Open INITECH. Close/reopen it, then select lower View > Arrange (by Name), (125,28) to (190,214).

**Saw:** Opening logged `n=64` and `FINDER-WIN-ICONS-FULL win=0 dropped=5`. No on-screen warning identified the omitted items. AAAAA never appeared. Reopening did not fix it, and the Arrange selection did not expose it. The disk listing proves AAAAA exists and the volume has ample free space. Five entries were excluded from the view's inventory, not merely below the visible scroll region. Arrange itself also failed to sort; that separate behavior is G10.

**Expected and why:** A folder view should enumerate all of a modest directory, or clearly explain a limit and offer a way to access the remainder. Sixty-nine files/folders on a 1.44 MB disk is an ordinary 1990s workload. This is a completeness and feedback expectation, not a Mac-specific measurement. The first pass's F02 covered inaccessible overflow in a 22-entry view; this test demonstrates additional entries absent from the view entirely.

**Evidence:** [Initial large root](shots/33-many-files-open.png), [after reopen and Arrange selection](shots/36-sentinel-missing-after-reopen.png), [preboot FAT listing](evidence/many-before.txt), [many-files trace](evidence/many-files-serial.log), and [preparation commands](evidence/disk-preparation.json).

### G05 - P1 - Full-disk errors are invisible, and Finder shows a file that was not committed

**Image:** `flair_tenants_interactive.img` with `full-data.img`.

**Do:** Use prepare_disks.py to fill all 1,455,616 remaining bytes of the pristine data disk with FILLER.BIN. Verify zero bytes free. Boot this disk, open INITECH, and press Ctrl-N. Inspect the screen, then inspect the disk with mdir after QEMU has stopped.

**Saw:** Boot logged `DESKTOP-DB-WRITE-FAIL rc=-11` and `TRASH-INIT-FAIL rc=-11`. Ctrl-N dispatched New Folder and logged `FINDER-NEW-FOLDER-FAIL rc=-5`. No visible error, retry/cancel interface, or full-disk explanation appeared. Finder showed four root icons, including DESKTOP.DB. The stopped disk's root listing still had only README.TXT, APPS, and FILLER.BIN; DESKTOP.DB and the requested new folder were absent. Thus the visible file inventory also disagreed with committed storage. No claim is made about an exact internal cache/rollback cause.

**Expected and why:** Disk-full failure must be visible to the person performing the operation. The displayed directory should reflect committed files, or clearly mark an uncommitted operation. This is ordinary filesystem/GUI consistency and recovery behavior. A full floppy-sized volume is a foreseeable workload, not a malformed disk.

**Evidence:** [Full-disk boot screen](shots/37-full-disk-boot.png), [after failed New Folder, with DESKTOP.DB icon](shots/38-full-disk-new-folder-failure.png), [zero-free-space input listing](evidence/full-before.txt), [stopped-disk listing](evidence/full-after.txt), and [full-disk trace](evidence/full-disk-serial.log).

### G06 - P2 - A fifth folder window silently refuses to open

**Image:** `flair_tenants_interactive.img` with `folders-data.img`.

**Do:** Open INITECH, then A at (192,101). Bring the root forward with its title at (180,70), open B at (261,101), bring root forward, and open C at (54,153). Bring root forward and double-click D at (123,153). Then reopen A, close A with Ctrl-W, bring root forward, and retry D.

**Saw:** Root+A+B+C opened. D selected but did not open and there was no on-screen explanation; serial said `FINDER-WIN-FULL`. Reopening A raised the existing window with `singleton=1`, so singleton handling itself worked even at capacity. After closing A, D opened normally in the freed slot. This isolates the refusal to the four-window ceiling rather than a bad folder or missed double-click.

**Expected and why:** A basic folder workflow should support more than four windows, or explain resource exhaustion and how to recover. Nested navigation can hit this ceiling after only three child folders. The scale expectation comes from my memory of classic Finder; visible feedback on a refused Open is ordinary GUI semantics. The exact number of supported windows is not asserted to be mandated by the historical static references.

**Evidence:** [Four folder windows](shots/29-four-folder-windows.png), [D refused without explanation](shots/30-fifth-folder-refused.png), [A raised at capacity](shots/31-singleton-at-capacity.png), [D opened after freeing a slot](shots/32-retry-after-freeing-slot.png), and [window-limit trace](evidence/window-limit-serial.log).

### G07 - P2 - Finder cannot select or navigate files from the keyboard

**Image:** `flair_tenants_interactive.img` with the normal data disk.

**Do:** Cold boot a fresh normal copy. Open the root and click blank content at (300,220) to clear selection. Before pressing any modifier key, press Right, Down, then type `apps`, allowing 350 ms between keys. Observe. Click APPS at (123,101), press Right, observe, then press Ctrl-O.

**Saw:** The arrows and typed name selected nothing. After mouse-selecting APPS, Right left APPS selected instead of moving to the next item. Ctrl-O then opened APPS successfully. The trace confirms the key events reached the guest. Selection and navigation required the mouse even though the keyboard Open command worked.

**Expected and why:** Arrow navigation and type-to-select are expectations from my memory of classic Finder. A desktop aimed at ordinary era usability should provide a keyboard path to selecting the target of Open. This is separate from first-pass F03's Select All no-op and F14's missing app switcher. This test does not assert untested keyboard shortcuts or a universal DOS-key mapping.

**Evidence:** [No selection after arrows/name typing](shots/75-clean-keyboard-cannot-select.png), [Right leaves APPS selected](shots/76-clean-arrow-selection-unchanged.png), [Ctrl-O works after mouse selection](shots/77-clean-keyboard-open-after-mouse-select.png), and [clean keyboard trace](evidence/keyboard-clean-serial.log). This was repeated on a fresh boot after G11 exposed modifier contamination in the earlier keyboard run.

### G08 - P2 - Clicking inactive app content executes its action during activation

**Image:** `flair_tenants_interactive.img` with the normal data disk.

**Do:** Launch TenantFix, drag its title from (250,170) by (+200,+120), then activate the root by clicking its exposed title at (180,70). TenantFix's inactive title is visibly exposed at y=280. Click once in its exposed content at (450,350).

**Saw:** That single click both activated TenantFix and invoked its content-to-quit action. The app immediately exited. Serial shows the activation event, then `TENANT-EVT what=1`, `TBX_EXIT`, and teardown from the same mouseDown. No second click was needed.

**Expected and why:** From my memory of classic Mac OS, the first click on inactive document content normally selects/activates its window, with a subsequent click operating the content. Click-through controls can be a deliberate exception, but no such distinction or affordance was presented here. TenantFix's content-to-quit action is intentional; executing it on the activation click is the issue. This expectation is memory-based, not measured from a historical interactive guest.

**Evidence:** [Inactive TenantFix visibly exposed](shots/49-inactive-tenant-visible.png), [gone after one content click](shots/50-first-activation-click-exits.png), and [slow-gestures trace](evidence/slow-gestures-serial.log).

### G09 - P2 - App launch refusal and app crashes have no on-screen explanation

**Image:** `flair_tenants_interactive.img` with the supplied `flair_data_tfx_noreg.img` and `flair_data_tfx_crash.img` data fixtures. The kernel is the normal interactive kernel.

**Do, refusal:** Boot the unregistered fixture disk, open INITECH and APPS, and double-click TENANTFX.EXE at (75,124). Wait 0.5 seconds, then retry once.

**Do, crash:** Boot the crashing fixture disk, launch the same icon, then click its content at (250,230). Wait 0.5 seconds. Press Ctrl-N to test continued Finder operation.

**Saw:** Both refused launches returned to the unchanged APPS window with no visible error. Serial logged a real load, rejected toolbox calls, and `TENANT-UNREGISTERED` on each attempt. The crash fixture logged `TENANT-CRASH vec=6` and `TENANT-EXIT rc=-1 via=crash`; the window vanished with no alert or explanation. Finder then created a folder normally, proving that the exception was contained and the desktop remained responsive.

**Expected and why:** Failed launch and unexpected app termination should tell the user what happened and permit recovery. This is ordinary error-feedback semantics; classic Mac crash alerts are additionally expected from my memory. The deliberately invalid fixture and deliberately injected exception are not themselves reported as product defects. The finding is the absence of visible failure notification. No whole-OS panic occurred in this test.

**Evidence:** [Refused launch](shots/55-refused-app-no-alert.png), [retry equally silent](shots/56-refused-app-retry-no-alert.png), [crash fixture before](shots/52-crash-fixture-before.png), [after exception, no alert](shots/53-crash-fixture-after.png), [Finder still usable](shots/54-post-crash-finder-responsive.png), [launch-refusal trace](evidence/launch-refusal-serial.log), and [crash-recovery trace](evidence/crash-recovery-serial.log).

### G10 - P2 - Arrange by Name leaves both order and manual placement unchanged

**Image:** `flair_tenants_interactive.img` with the normal data disk; also seen on the many-file disk.

**Do:** Open the normal root, with README.TXT before APPS in the original row. Drag README.TXT from (54,101) by (+90,+100), leaving it well below the row. Select lower View > Arrange (by Name), from (125,28) to (190,214), releasing promptly before the G01 timeout. Wait 0.5 seconds.

**Saw:** The Arrange item was enabled and highlighted. Serial logged menu 514/item 12 and `FINDER-CMD id=10 name=ARRANGE_BY_NAME src=mouse sel=1`. README.TXT stayed at its manual lower position, APPS stayed in the second grid column, and the gap in the first column remained. There was no reordering, grid arrangement, or explanation. The same command on the many-file disk left README.TXT before APPS in the original unsorted order.

**Expected and why:** An enabled Arrange (by Name) command should arrange the icons in name order, rather than silently leaving them where they were. This follows the command's label and ordinary sorting semantics; name-based Finder arrangement is also expected from my memory of classic Mac OS. The first pass explicitly left Arrange untested, so this is additional evidence, not a repeat of its disabled-view finding.

**Evidence:** [Manually placed file before Arrange](shots/63-before-arrange-manual-position.png), [enabled Arrange selection](shots/64-enabled-arrange-held.png), [unchanged result](shots/65-arrange-leaves-order-and-position.png), [many-file result](shots/36-sentinel-missing-after-reopen.png), and [soak trace](evidence/soak-serial.log).

### G11 - P1 - Ctrl and Shift remain active after key release

**Image:** `flair_tenants_interactive.img` with the normal data disk. Observed after the stress run and reproduced in three fresh-boot tests.

**Do, minimal Ctrl test:** Cold boot a fresh copy and open INITECH. Send an explicit Ctrl press, then an explicit Ctrl release, wait 0.5 seconds, and send plain `n`, without any modifier. The driver's commands are `keyheld ctrl true`, `keyheld ctrl false`, `pause 0.5`, and `key n 0.35`.

**Do, combined test:** On a separate fresh boot, first press plain N; it performs no command. Press Ctrl and Shift, press/release A, explicitly release Ctrl and Shift, wait 0.5 seconds, then press plain N and plain W.

**Do, Shift-only test:** On another fresh boot, press/release Shift, wait 0.5 seconds, single-click README.TXT (54,101), then single-click APPS (123,101). Type plain A.

**Saw:** In the minimal Ctrl test, plain N dispatched New Folder and created NEWFOLD. In the combined test, the initial unmodified N did nothing, but the later plain N created a folder and plain W closed the root. The Ctrl and Shift release scancodes appear in the trace before those commands. In the Shift-only test, both README.TXT and APPS remained selected (count 2), and plain A was cooked as uppercase A. Thus this is neither an HMP chord timing artifact nor only a consequence of a flooded input queue. Fresh boots reproduced it with explicit press/release events and a half-second settling interval.

**Expected and why:** Releasing a modifier must end its effect. Plain letters must not unexpectedly invoke commands or change selection mode after the physical modifier is up. This is ordinary keyboard state semantics. The demonstrated effects include unintended filesystem creation and window closure; no unsaved-document loss was demonstrated.

**Evidence:** [Plain N before modifiers](shots/72-plain-n-fresh.png), [plain N after explicit releases creates a folder](shots/73-plain-n-after-modifier-release.png), [plain W closes the window](shots/74-plain-w-closes-window.png), [Ctrl-only minimal reproduction](shots/78-ctrl-only-release-plain-n.png), [Shift-only two-item selection](shots/79-shift-only-release-two-selected.png), [combined trace](evidence/modifier-repro-serial.log), [Ctrl-only trace](evidence/ctrl-only-serial.log), [Shift-only trace](evidence/shift-only-serial.log), and the exact release actions in [actions.jsonl](evidence/actions.jsonl).

## Additional coverage and behavior that held up

- The film saving modal rendered above both document windows. A background upper-menu attempt was blocked, and Return/Escape did not accidentally route commands to the background. Dragging it to both extremes kept the box on-screen below the stacked bars and restored its old footprint in these tested frames. These were short bounded-image tests; an indefinitely stalled save was not alleged. See [initial film frame](shots/01-film-static.png), [blocked background menu result](shots/03-modal-blocks-menu.png), [top-left modal](shots/02-modal-top-left.png), and [bottom-right modal](shots/04-modal-bottom-right.png).
- The nonmodal film window retained a reachable part of its title after an extreme left/top drag, and zoomed below both bars. No new edge or zoom defect was established in those short tests. See [edge frame](shots/70-film-nomodal-edge.png) and [zoom frame](shots/71-film-nomodal-zoom.png).
- TenantFix content followed its moved frame, including partial off-screen placement, unlike the Finder-content issue already recorded in the first pass. Its minimum resize clamped to 96x64 with separate usable widgets; clipped text at that deliberately small size is not a finding. See [edge tenant](shots/14-tenant-edge-top-left.png), [bottom-edge tenant](shots/15-tenant-edge-bottom-right.png), and [minimum-size tenant](shots/51-minimum-tenant-size.png).
- Reopening an already open folder raised the singleton even when all four folder slots were occupied. Freeing a slot allowed the previously refused folder to open. Heavy overlap did not produce a new independent repaint failure in the inspected frames. The known F01 behavior was visible again during Finder movement and is not re-filed here.
- The longest continuous guest session lasted 672.4 seconds (11 minutes 12 seconds). It included 240 disk-app launch/Close cycles, all 240 successful loads and 240 close exits. Every teardown reported `TENANT-HEAPAVAIL n=3523968`, and every load used the same reported base. This supports repeatable reclamation in that tested path, not a general leak-free claim. See [cycle summary](evidence/soak-cycle-summary.json), [240-cycle frame](shots/62-soak-240-cycles.png), and [raw soak trace](evidence/soak-serial.log).
- The same session received 100 seeded rapid relative-pointer/button gestures (left, middle, and right) and 100 explicit key press/release pairs with Ctrl+Shift, followed by explicit releases. It remained responsive. The subsequent plain-N check exposed G11 rather than proving successful modifier reset. The system could still execute later New Folder commands, and there was no panic. See [after pointer sequence](shots/66-after-rapid-pointer-input.png), [after modified keys](shots/67-after-rapid-modified-keys.png), and [plain N wrongly creates a folder](shots/68-post-fuzz-unmodified-n.png).
- The deliberately injected app exception was contained and reclaimed. The failure-notification gap is G09; the exception itself is not counted as a spontaneous crash of the product. No guest OS panic, reboot loop, or unplanned whole-desktop hang was observed across the 18 sessions.

There are 79 guest screenshots. Every screenshot cited in this report was opened and visually checked against its claim. The [screenshot index](evidence/screenshot-index.json) records dimensions and SHA-256 hashes; the [serial index](evidence/serial-index.txt) points into the raw traces. Some screenshots intentionally show a held gesture; they are paired with the action journal to establish that the button had not yet been released.

This was a 27-minute guest-testing window, with one 11-minute continuous session. It was not an overnight soak. Unsupported/unreached full editors, real Open/Save dialogs, editable modal controls, printers, sound, removable-media change, and network features remain outside the established evidence. No visual conclusions about the absent pie chart are inferred from the tested blank scene bodies.

All 18 QEMU instances were stopped through QMP quit and waited for; each exited with status 0. The driver then exited with status 0. No external process was killed. See [per-instance cleanup](evidence/cleanup.jsonl) and [cleanup verification](evidence/cleanup-verification.json). The report, checked screenshots, raw logs, driver, and private disk copies remain in this working directory.
