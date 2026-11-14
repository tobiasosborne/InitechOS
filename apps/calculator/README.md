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

make test runs two programs in about one second: the real-Mac vector and
capture differential plus 20,000 independent reference sessions, and a
unit/regression routing test on sdk/host. Missing captures fail. References
are the system7-decomp mint-results-007 captures named by the claimed vector
file; include/calcda/calcda_layout.h documents their measurement. make mutants
kills eight patches, including a live Copy no-op. Mutation switches are
absent from the product source. make smoke uses the booted
product image and grades the window against the independent real-Mac PNG.
Screendump: build/smoke-desktop/calculator.ppm.

Remaining unclaimed legacy Calculator emulator traces and Apple-menu checks
belong to the desktop/factory owners; the app routing and pixel checker are
claimed here. No private Toolbox fake remains.
