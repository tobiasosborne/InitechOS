# InitechPaint

PAINT.EXE is, deliberately, the eight-title Photoshop menu bar over a blank
white canvas. That is all the frame needs. Dependencies: SDK and FLAIR's
public menu-canon asset header; host tests also use InitechCalc's public
ledger header to retain the legacy cross-check of the frozen PRD literal.
Plain mk/tenant.mk flags, no semantic switches, no resource files to load.

Frozen legacy-final inventory: File Edit Image Layer Select View Window
Help, no Apple slot, one Untitled-1 canvas window, white repaint, Quit and
close. It persists nothing. Twenty-four visible disabled commands do nothing:
File New/Open/Close/Save (4); Edit Undo/Cut/Copy/Paste/Clear (5); Image
Adjust/Image Size/Canvas Size (3); Layer New/Delete/Flatten (3); Select
All/None/Inverse (3); View Zoom In/Zoom Out/Show Rulers (3); Window Show
Brushes/Show Layers (2); Help Paint Help (1). GOLDEN-MASTER item 4's dead-command
sweep is outstanding; this import preserves the intentionally shallow app.

make test runs the claimed 68-check routing regression on sdk/host in <1s.
Expected bar text is the independent PRD literal; this is unit/regression,
not a period Photoshop differential. make mutants kills both patches;
removing hooks leaves the 2,960-byte EXE identical. make smoke launches/quits
on QEMU and checks every pixel in a 260x190 interior canvas region is white
using the claimed independent C grader. Screendump:
build/smoke-desktop/canvas.ppm.
The grader's optional menu mode uses FLAIR's frozen independent NFNT golden;
that legacy pulled-menu journey and bar pixel checks remain with factory.
