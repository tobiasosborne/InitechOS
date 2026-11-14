# InitechPaint

PAINT.EXE is, deliberately, the eight-title Photoshop menu bar over a blank
white canvas. That is all the frame needs. Dependencies: SDK and FLAIR's
public menu-canon asset header; host tests also use InitechCalc's public
ledger header to retain the legacy cross-check of the frozen PRD literal.
Plain mk/tenant.mk flags, no semantic switches, no resource files to load.

Frozen legacy-final inventory: File Edit Image Layer Select View Window
Help, no Apple slot, one Untitled-1 canvas window, white repaint, Quit and
close. It persists nothing. File now offers Close (W) and Quit (Q), both
ending the sole documentless canvas like its close box; the Finder restores
its menu ownership. All eight frame titles retain their order/position.
The other seven headings have zero items. This is an authored choice with
period precedent in Inside Macintosh Volume IV (1986), printed IV-55:
hold highlights a title, draws no panel/shadow, release selects no command.
No native Photoshop or Mac OS 8 empty-menu capture is claimed. The gate
accepts zero items; FLAIR's panel-geometry fix and target gesture proof belong
to its owner (the app-side sweep does not certify that shared rendering).

Frozen omissions (approved phase-2 GM4 split, 23 leaves): File New...,
Open..., Save; Edit Undo, Cut, Copy, Paste, Clear; Image Adjust, Image Size...,
Canvas Size...; Layer New Layer..., Delete Layer, Flatten Image; Select All,
None, Inverse; View Zoom In, Zoom Out, Show Rulers; Window Show Brushes,
Show Layers; Help Paint Help. No dummy row or separator replaces them.
Window Untitled-1 is not introduced. A painting engine remains Release 2.

The dead-command sweep walks every native row and checks exact equality
with an independent literal Close/Quit manifest plus seven empty titles,
checks both commands exit and all empty-title command identities do nothing,
and checks the blank canvas and absence of file/dialog activity. The event
switch exposes no custom buttons, editable keys or contextual menus.

make test runs the 33-check routing regression on sdk/host in <1s.
Expected bar text is the independent PRD literal; this is unit/regression,
not a period Photoshop differential. make mutants kills both patches,
including reinsertion of disabled Edit Undo. No mutation switches remain
in the product source. make smoke launches/quits
on QEMU and checks every pixel in a 260x190 interior canvas region is white
using the claimed independent C grader. Screendump:
build/smoke-desktop/canvas.ppm.
The grader's optional `empty idle.ppm held.ppm title` mode uses FLAIR's frozen
independent NFNT widths and compares all pixels below the bar without a
mask. Both captures must have the pointer at the same title position. Its
real held-menu inputs and the title/bar pixel checks remain with FLAIR/factory
until the shared geometry issue initech-9jq67 is fixed.

Initial windows meet the single menu bar (operator ruling, CLAUDE.md
"Hazards that are still true"). Host routing checks their structure top
against the public FLAIR_CHROME_MENUBAR_H metric; content sizes are preserved.

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: desktop
Interface use-gate: sdk/include/tbx.h
Interface use-records: flair/include/assets/menu_canon.h
<!-- end-interface-declarations -->
