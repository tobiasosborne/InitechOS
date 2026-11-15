# Calculator

CALCULAT.EXE is the documentless four-function desk accessory. Dependencies:
sdk/include and the shared sdk/host fake for host routing. The target uses
mk/tenant.mk with plain flags; no semantic compiler switches are required.

Frozen inventory (legacy-final): decimal entry, chained + - * /, clear,
12-character fixed/scientific readout, keyboard and mouse keys, Close and
Quit. The rounded window and key/readout pixels follow the real Mac OS 8.1
Calculator capture. No files are persisted. Key press flashing is absent.
Edit offers Copy of the unpadded readout as TEXT and Clear (the captured C
reset). Copy leaves arithmetic unchanged and reports scrap transport errors;
Clear preserves the scrap. These menu bindings are authored adaptations of
Macintosh System Software User's Guide v6, p.118 and the captured C vectors
p00/s10/s11; they are not claims of a captured native Calculator Edit menu.

Frozen omissions (approved phase-2 split, GM4): Edit Undo (Cmd-Z), Cut
(Cmd-X), Paste (Cmd-V). No arithmetic history or numeric clipboard importer.
The removed shortcuts are unmapped. Copy has no advertised shortcut.

The dead-command sweep in test_calcda_route checks the exact ordered menu
and shortcut set, independent literal readouts/scrap bytes, every painted key
by pointer and keyboard, pending-state Clear, Copy failure and both exits.
No contextual/popup menu or dialog button surface exists beyond an OK error
notice. The shell owns menu-key/close-box routing; the desktop smoke drives
those real routes. tests/COMMANDS.md records the audited route ledger.

make test runs three programs in about two seconds: the real-Mac vector and
capture differential plus 20,000 independent reference sessions, and a
unit/regression routing test on sdk/host, and the GM4 command sweep. Missing captures fail. References
are the system7-decomp mint-results-007 captures named by the claimed vector
file; include/calcda/calcda_layout.h documents their measurement. make mutants
kills ten patches, including a live Copy no-op, a grayed Clear and a dead
'.' key that the command sweep catches. Mutation switches are
absent from the product source. make smoke uses the booted
product image and grades the window against the independent real-Mac PNG.
Screendump: build/smoke-desktop/calculator.ppm.

Remaining unclaimed legacy Calculator emulator traces and Apple-menu checks
belong to the desktop/factory owners; the app routing and pixel checker are
claimed here. No private Toolbox fake remains.

Initial windows meet the single menu bar (operator ruling, CLAUDE.md
"Hazards that are still true"). Host routing checks their structure top
against the public FLAIR_CHROME_MENUBAR_H metric; content sizes are preserved.

Host geometry tests also depend on flair/include/chrome_metrics.h.

GM4 dead-command sweep (test_calcda_commands, tests/command_sweep.h): the
block below is the frozen inventory as data; the sweep checks the installed
bar in every context, chooses each row where it must work, sends its command
key as the shell does, and sends every character byte and key code under each
modifier, so any key that changes anything must be listed and every listed key
must work, judged only from the host Toolbox record. The keys are the frozen
legacy map: digits, . + - * / =, c or C, the Enter key code 1C (main and
keypad) and the key code 45 where the Mac keypad has Clear. Shift and Option
do not change a key; Control is Command and reaches only the menus. Pointer
presses on the painted keys are swept by test_calcda_route.

<!-- command-inventory CALCULAT.EXE -->
Context: entry = key c31
Context: pending = key c32; key c2B; key c33
Title: File | 2
Menu: File | Close | W | base | exit
Menu: File | Quit | Q | base | exit
Title: Edit | 2
Menu: Edit | Copy | - | base | scrap
Menu: Edit | Clear | - | entry | screen
Key: none | c2E c31-39 | base | screen
Key: none | c30 c43 c63 v45 | entry | screen
Key: none | c2A c2B c2D c2F c3D v1C | pending | screen
Key: shift | c2E c31-39 | base | screen
Key: shift | c30 c43 c63 v45 | entry | screen
Key: shift | c2A c2B c2D c2F c3D v1C | pending | screen
Key: option | c2E c31-39 | base | screen
Key: option | c30 c43 c63 v45 | entry | screen
Key: option | c2A c2B c2D c2F c3D v1C | pending | screen
<!-- end-command-inventory -->

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: desktop
Interface use-gate: sdk/include/tbx.h
<!-- end-interface-declarations -->
