# InitechOS FLAIR desktop audit: fourth pass

Guest testing: 2026-10-04, 04:45:25-05:04:15 Europe/Berlin. Findings only.
Verdicts apply to the copied images and exercised paths, not every possible input.

| Developer claim | Verdict | Evidence and limits |
| --- | --- | --- |
| Finder icons follow drag, zoom, restore, collapse and grow; their visible positions work for selection and dragging (F01) | **holds** | A moved root kept its icons attached, including selection and manual repositioning: [moved](shots/09-root-moved.png), [selected](shots/10-moved-icon-selected.png), [repositioned](shots/14-moved-file.png). [Zoom](shots/12-root-zoom.png), [restore](shots/13-root-restored.png), [collapse](shots/11-root-collapsed.png), and [grow](shots/74-late-grow-release.png) retained the content relationship. The scrolled view also followed a move/collapse/zoom/restore sequence ([moved](shots/37-scrolled-window-moved.png), [restored](shots/40-scrolled-window-restored.png)). Selection worked at the left screen edge ([TRASH selected](shots/132-left-edge-correct-selection.png)); a scrolled F28.TXT could be selected and dragged to Trash ([selected](shots/129-scrolled-file-selected.png), [result](shots/131-scrolled-file-trash-result.png), [disk listing](evidence/scrolled-drop-trash-after.txt)). Sixty combined geometry/app cycles passed. Filename clipping has a separate exception, J02. |
| Scrollbars step/repeat, page and drag their thumbs, never drag the window, support scrolled selection, and look disabled when there is no range (F02) | **partly holds** | Arrow clicks changed the value by 16 pixels; held arrows repeated, stopped outside, and resumed on re-entry ([held](shots/30-arrow-repeat-held.png), [result](shots/32-arrow-reenter-result.png)). The track paged by 161 pixels ([page result](shots/36-track-page.png)); thumb feedback tracked while held and the view changed on release ([held](shots/34-thumb-held.png), [released](shots/35-thumb-release.png)). Scrolled selection worked ([F16 selected](shots/33-scrolled-selection.png)). These operations did not move the frame. Initial no-range bars ignored clicks. However, resize leaves enabled/disabled appearance stale (J01), an unmoved thumb click changes the value (J04), and minimum-height bars have invisible active parts (J05). |
| No enabled menu command is dead, apart from upper-bar Quit; Select All and Arrange work and the listed unimplemented commands are disabled (F03/F05/F07/G10) | **partly holds** | The specific Finder changes hold: [document File menu](shots/03-doc-file-menu.png), [disabled Get Info under the pointer](shots/04-get-info-disabled-held.png), [Special](shots/06-special-disabled.png), [upper About](shots/07-upper-about-disabled.png). Attempts at the disabled rows returned menu item 0; Ctrl-I/D/O on the document did not dispatch those commands. Empty Trash remained disabled after real staging ([nonempty case](shots/124-real-nonempty-trash-disabled.png), [disk contents](evidence/final-edge-trash-after.txt)). [Select All](shots/08-select-all.png) selected all four root entries; later it selected all 47, including hidden rows ([scrolled selection](shots/44-scrolled-select-all.png), `FINDER-SELECT-ALL ... n=47`). [Arrange](shots/15-arranged-root.png) repaired manual placement and put APPS, DESKTOP.DB, README.TXT, TRASH in name order; it also sorted the reverse-named fixture. The broader wording still has the already-filed G03 exception: TenantFix's enabled lower File > Quit returned menu 129/item 1 and left the app running ([after Quit](shots/60-tenant-quit-result.png), [trace](evidence/scroll-serial.log)). G03 is not filed again below. |
| Closing the last Finder window keeps Finder foreground, draws no app window active, and app/desktop clicks switch foreground correctly (F06) | **holds** | Both initial Ctrl-W and the final explicit last-window test left Finder's bar and plain inactive HELLO/NOTES titles: [initial](shots/19-last-finder-closed.png), [final](shots/125-last-finder-closed.png). Clicking Notes gave it active stripes/widgets and `File Edit Notes` ([Notes](shots/126-notes-front.png)); desktop click returned Finder's bar and inactive app chrome ([desktop](shots/127-desktop-finder-front.png)). Matching dispatches are in [final-edge trace](evidence/final-edge-serial.log). |
| Released modifiers do not latch; gestures continue until release; Close/Zoom/Collapse act only on release inside | **holds** | After 60 combined cycles, explicit releases of both left/right Ctrl and Shift followed by plain N/W produced no creation/close command; ordinary clicks left one item selected ([late check](shots/68-late-modifiers-clear.png)). A menu stayed held for 32 seconds ([held](shots/69-late-menu-held-32sec.png), [released](shots/70-late-arrange-release.png)); a window drag stayed an outline for 20 seconds ([held](shots/71-late-window-held-20sec.png), [final motion committed](shots/72-late-window-drag-release.png)); grow stayed an outline for eight seconds ([held](shots/73-late-grow-held.png), [released](shots/74-late-grow-release.png)). Close survived a six-second hold and cancelled one pixel outside its right edge ([held](shots/61-close-held.png), [cancelled](shots/62-close-edge-cancel.png)). Zoom and Collapse also cancelled outside ([Zoom](shots/64-zoom-cancel.png), [Collapse](shots/66-collapse-cancel.png)); release inside committed in the window/soak tests. Pressing/releasing right mouse while left held did not commit Zoom. No old gesture expiration or modifier latch was reproduced. The known cross-menu H01 sequence was excluded. |

## New findings

P2 means a substantial usability or state-consistency problem. P3 means a small
interaction/drawing defect. No spontaneous guest panic, reboot or desktop hang
was observed. All five findings below were observed here, including fresh-copy
reproductions; none is a re-file of the earlier audits.

### J01 - P2 - Resize leaves scrollbar appearance inconsistent with its actual range

**Do:** Start with the pristine normal data disk. Open INITECH at (600,64).
Drag the root grow box from (370,270) by (-200,-120), producing a 160x100
window. Wait 0.6 seconds. Click the horizontal right arrow at (151,147).
Click it four more times. Grow back by dragging (170,150) by (+200,+120).
Wait 0.6 seconds, then click the horizontal right arrow at (352,267).

**Saw:** Shrinking clipped two icons, but the horizontal bar still had pale
disabled arrows, a flat trough and no thumb. Clicking it nevertheless scrolled
16 pixels and drew the enabled bar: `h part=21 value=16 max=134`. After reaching
80 pixels and growing back to the original size, every icon fit and the view
returned to its top-left. The bar still showed black arrows, a recessed well,
and a thumb around the middle. Clicking that apparently enabled arrow logged
`FLAIR-SCROLL-IGNORED ... h` and left the stale enabled artwork in place. This
reproduced independently of the long session.

**Expected and why:** Repaint the enabled/disabled state and thumb when the
view's size or range changes. Ordinary control semantics require the affordance
to agree with its action. The local [Platinum scrollbar reference](evidence/references/scrollbars.md)
explicitly distinguishes enabled, disabled, and inactive states; the real
[Mac window](shots/ref-s8-window.png) shows pale arrows and no thumb when its
contents fit. This is new state-invalidation behavior, beyond old F02's inert
bars/window dragging.

**Evidence:** [Disabled-looking overflow](shots/101-j01-shrink-disabled-art.png),
[it actually scrolls](shots/102-j01-disabled-art-scrolls.png),
[all icons fit but the bar still looks enabled](shots/105-j01-grow-enabled-art.png),
[ignored click leaves that artwork](shots/106-j01-enabled-art-ignored.png),
[fresh trace](evidence/fresh-bars-labels-serial.log). The earlier session
also showed both directions in shots 23-26.

### J02 - P2 - Horizontal clipping pins hidden icons' labels over visible filenames

**Do:** Use the same pristine normal root and shrink it to 160x100 as above.
Click (151,147) five times total, scrolling horizontally to value 80. Click
(26,126), at the left edge of the filename row. Separately, shrink the scrolled
47-entry root to 180 pixels wide.

**Saw:** At the small size, ordinary labels including DESKTOP.DB and TRASH
were pushed together at the right edge rather than clipped with their icons.
After horizontal scrolling, README.TXT's sprite was completely off the left
edge, but its label remained pinned into the viewport under/alongside APPS.
Clicking that remaining label selected README.TXT; its black selection band
was itself overwritten by neighboring labels. The 47-entry view showed the
same problem with short names such as F23.TXT and F21.TXT.

**Expected and why:** A label should retain its document position and clip
with the icon cell, allowing scrolling to reveal the complete cell. Hidden
icons should not force their text over visible entries. This is ordinary
viewport/selection semantics. The [Mac capture](shots/ref-s8-window.png) gives
visual context for separated icon labels, but is not a paired horizontal-scroll
experiment. This is different from H07's maximum-width names overlapping in
the default, unscrolled grid: ordinary names here become ghost hit targets
because of viewport clipping.

**Evidence:** [Right-edge pile-up](shots/101-j01-shrink-disabled-art.png),
[horizontal-scroll result](shots/103-j02-horizontal-labels.png),
[hidden README selected through its pinned label](shots/104-j02-hidden-icon-selected.png),
[short-name overlap in the larger fixture](shots/41-narrow-scrolled-window.png),
[fresh trace](evidence/fresh-bars-labels-serial.log), which records
`FINDER-WIN-SELECT ... name=README.TXT count=1` at (26,126).

### J03 - P2 - Open processes only one of two selected folders

**Do:** Boot the valid scroll fixture. Open INITECH. Select DEST at (192,101),
hold Shift and select EMPTY at (261,101), then release Shift. Press Ctrl-O.
Close DEST with Ctrl-W, and double-click EMPTY to verify it can open. Close it,
select both folders again, and choose lower File > Open by pressing (38,28),
moving to (60,65), and releasing.

**Saw:** Both folder names were selected. Both keyboard and mouse routes
reported `OPEN ... sel=2`, but opened only DEST. EMPTY opened normally when
chosen alone. Only root and DEST were open after each two-folder Open, so the
known four-window ceiling was not involved. There was no indication that part
of the selection had been skipped.

**Expected and why:** Open should apply to the selected folders, or visibly
explain why it handles only one. Opening all selected folders is an expectation
from my memory of classic Finder, not a behavioral measurement in the static
Mac captures. This is distinct from F08's group drag and G06's exhausted window
slots. Select All itself now works; the consumer of a multiple selection does
not handle all its members.

**Evidence:** [Two selected folders](shots/107-j03-two-folders.png),
[keyboard opens only DEST](shots/108-j03-key-opens-one.png),
[EMPTY opens alone](shots/109-j03-other-folder-valid.png),
[enabled Open with both selected](shots/110-j03-open-menu-held.png),
[mouse also opens only DEST](shots/111-j03-menu-opens-one.png),
[fresh trace](evidence/fresh-multi-open-serial.log). The long session reproduced
it with DEST/EMPTY after Arrange in shots 54-56.

### J04 - P3 - Clicking the thumb without moving it changes the scroll value

**Do:** On the fresh 47-entry root at (20,60), click the down arrow at
(367,251) once. Then click the thumb at (367,106), pressing/releasing without
any pointer movement between those events.

**Saw:** The first click set the vertical value to 16, max 450. The no-motion
thumb click changed it to 14. The content shifted downward by two pixels even
though the pointer had not dragged the indicator. The action journal and raw
trace have adjacent mouseDown/mouseUp at exactly (367,106), then
`v part=129 value=14 max=450`.

**Expected and why:** A press/release on the indicator without dragging should
preserve the view position. This is an ordinary GUI expectation; no historical
pressed-thumb capture was available for a matching experiment. The observed
effect is a small unwanted jump, not data loss or a broken general drag.

**Evidence:** [Before](shots/112-thumb-before-no-motion-click.png),
[after](shots/113-thumb-after-no-motion-click.png),
[fresh trace](evidence/fresh-multi-open-serial.log),
[explicit input journal](evidence/actions.jsonl).

### J05 - P2 - A minimum-height window hides its scrollbar parts but still operates them

**Do:** Fresh scroll fixture, open INITECH. Click the down arrow at (367,251),
then drag the grow box (370,270) by (-264,-156), reaching the permitted 96x64
minimum. Click (103,101), move the pointer clear to (500,380), and capture.
Click (103,95), move clear again, and capture.

**Saw:** The vertical gutter was a blank white strip with no arrows or thumb,
despite 47 entries and an actual scroll maximum of 606. Its invisible lower
part scrolled down from 16 to 32 (`part=21`); the invisible upper part scrolled
back to 16 (`part=20`). Repainting through those real scroll actions did not
restore visible controls. This is persistent minimum-size behavior, rather
than J01's stale range artwork after a normal resize.

**Expected and why:** At an allowed window size, visible controls should identify
the usable scroll operations. Reserve sufficient size for the bar, provide a
compact form, or make a hidden control inactive. An empty-looking gutter that
secretly scrolls is misleading. This follows ordinary hit-testing/affordance
semantics; the supplied Mac stills do not establish the exact historical
minimum window size.

**Evidence:** [Minimum frame](shots/117-minimum-scrollbar.png),
[still blank after down-scroll repaint, pointer clear](shots/121-minimum-blank-control-scrolls.png),
[still blank after up-scroll repaint](shots/122-minimum-blank-control-reversed.png),
[independent final-edge trace](evidence/final-edge-serial.log).

## Appearance against the real Mac captures

The new text is a substantial improvement: menu items and titles read as
bold, proportional Macintosh UI, and the gray disabled commands are clear.
Compare the [FLAIR File menu](shots/03-doc-file-menu.png) and
[FLAIR title](shots/59-tenant-own-bar.png) with the [real Mac OS 8.1 menu](shots/ref-s8-menu.png)
and [window](shots/ref-s8-window.png). The FLAIR face looks closer to the
[System 7 Chicago capture](shots/ref-s7-menu.png) than to the smoother Charcoal
shapes in the 8.1 captures. The actual [Appearance panel](shots/ref-s8-controls.png)
explicitly names Charcoal. This is an aesthetic judgment, not byte-level font
certification. No text-bearing button was reached, so button text is unverified.

Drag feedback is understandable: the source remains, a gray outline follows
the pointer, and Trash darkens and inverts its name ([held scrolled file](shots/130-scrolled-file-trash-held.png)).
Its thin empty rectangle carries little of the dragged file's shape or name.
Beside the detailed icon cells in the real Mac window, it still looks like
basic scaffolding rather than finished Platinum artwork. The local
[drag-feedback research](evidence/references/finder-drag-feedback-ground-truth.md)
describes an outline representation of icon and filename, while documenting
the project's whole-cell-rectangle interpretation. My preference for a more
recognizable icon/name outline also comes from classic Mac memory. None of
the supplied Mac captures shows a live Finder drag, so I cannot claim exact
historical drag-pixel parity. This assessment does not re-file F16 or H07.

## Scope, replay, identity and cleanup

I read all three prior REPORT.md/TRIAGE.md pairs and their drivers, including
the triage correction to F06. F01-F18, G01-G11 and H01-H07 were the exclusion
baseline. No issue-tracker mutation, source edit, build, make invocation,
commit or remote write was performed. All writes were inside this workspace.
The desktop, bars, hourglass convention, intentional chart numbers, saving
wording and panic phrase were not treated as defects. No pie chart, 570-
figure, productive editor, real file chooser or text-bearing button was
reached in the tenants image.

Only copies of the normal `build/flair_tenants_interactive.img` kernel were
booted, with pristine or locally derived copies of `build/flair_data.img`.
The brief identifies the builds as commit 40581c5; the actual tested bytes are
identified in [image manifest](evidence/image-manifest.json):

- Boot: 229376 bytes; SHA-256 `c994804c91f378c5909e1eb8c550fb33779661a7c2aa07484ef618fc9a446640`.
- Pristine data: 1474560 bytes; SHA-256 `84390fdb93bfe532d7e927a0bd83d918ec6f00c71c7bf5c368d56e09e2f4c4e4`.

The scroll fixture adds DEST, EMPTY, ZLAST and F39.TXT down through F00.TXT
to the valid FAT disk. Boot-created DESKTOP.DB/TRASH bring its root to 47
entries, below the known 64-entry limit. It has ample free space. Exact mtools
commands, return codes, fixture hash, and listing are in
[preparation log](evidence/fixture-preparation.json) and
[input listing](evidence/scroll-fixture-before.txt).

QEMU used `-cpu 486 -m 16 -display none`, with the boot and data as first/second
IDE disks. All input was QMP PS/2 input; screenshots were QMP guest framebuffer
screendumps. Display :0 was never captured or given input. Seven emulator
instances ran sequentially. The longest continuous session was 785.55 seconds
(13 minutes 5.55 seconds). Its 60 combined cycles included 120 grows, 120 zoom
changes, 120 collapse changes, and 60 successful disk-app loads/Close exits.
Every teardown in that cycle section reported `TENANT-HEAPAVAIL n=3523968`.
This supports stability in those exercised paths, not overnight reliability
or general leak freedom. See [timing/cycle summary](evidence/audit-summary.json)
and [long trace](evidence/scroll-serial.log).

[audit_driver.py](audit_driver.py) is the earlier pass's harness, reused without
changes. Coordinates are guest pixels. `drag` takes x, y, delta-x, delta-y;
`move` homes the pointer, so use `rel` inside a held gesture. `menu` presses a
title, moves to a row, and releases. `keyheld` sends explicit press/release.
Run the harness in a separate writable copy of this directory and send JSON
lines, for example:

```text
["start","replay","images/pristine-boot.img","images/pristine-data.img"]
["click",600,64,2]
["drag",370,270,-200,-120]
["shot","small-root"]
["click",151,147]
["quit"]
```

[replay_findings.py](replay_findings.py) reproduces J01-J04 and an initial J05
case on fresh copies. J05's independent, pointer-clear replay is specified in
its finding and in the journal's `final-edge` session. Do not run either driver
alongside another emulator. [Actions](evidence/actions.jsonl) record UTC times,
session names, exact inputs and serial byte offsets. Raw `*-serial.log` files,
`*-session.json` launch arguments/hashes, [serial index](evidence/serial-index.txt),
and [screenshot index](evidence/screenshot-index.json) are under evidence/.

There are 122 native 640x480 guest screenshots, four copied reference captures,
and two labeled nearest-neighbor detail crops of the minimum bar. Every
screenshot cited in this report was opened and checked. Held-state assertions
also use the explicit button journal and dispatch ordering. Mis-targeted
actions and misleading intention-based filenames are documented in
[harness notes](evidence/test-scope.txt); they were not used as successful-test
evidence. In particular, 77 was a marquee, 78 still had an empty Trash, 82 was
not the last Finder window, and 119-120 followed an accidental Close. Correct
replays are cited above.

References were copied from `/home/tobias/Projects/system7-decomp/goldens/captures/`
and its `specs/` tree. Appearance comparisons use actual captured pixels;
memory-based behavioral expectations are labeled in each finding. No live
historical Mac emulator was started, and no gamma-adjusted RGB equality claim
is made.

All seven QEMUs received quit through their own QMP connection and were waited
for; every exit status was 0 ([cleanup](evidence/cleanup.jsonl)). Both audit
drivers also exited with status 0. No external process was killed. The report,
screenshots, raw evidence, replay drivers and private disk copies remain here.
The [final verification](evidence/final-verification.json) records ASCII/link
checks, cleanup, and repository image hashes still matching the pristine copies.

The core fixes largely hold. The remaining new problems are viewport/control
consistency, multiple-selection Open, and small scrollbar edge cases. Five
new findings are recorded; no panic or whole-desktop hang was observed.
