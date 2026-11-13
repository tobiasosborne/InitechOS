# Calculator

CALCULAT.EXE is the documentless four-function desk accessory. Dependencies:
sdk/include and the shared sdk/host fake for host routing. The target uses
mk/tenant.mk with plain flags; no semantic compiler switches are required.

Frozen inventory (legacy-final): decimal entry, chained + - * /, clear,
12-character fixed/scientific readout, keyboard and mouse keys, Close and
Quit. The rounded window and key/readout pixels follow the real Mac OS 8.1
Calculator capture. No files are persisted. Key press flashing is absent.
Five visible disabled commands do nothing: Undo, Cut, Copy, Paste, Clear.
The dead-command sweep required by GOLDEN-MASTER item 4 is still outstanding.

make test runs two programs in about one second: the real-Mac vector and
capture differential plus 20,000 independent reference sessions, and a
unit/regression routing test on sdk/host. Missing captures fail. References
are the system7-decomp mint-results-007 captures named by the claimed vector
file; include/calcda/calcda_layout.h documents their measurement. make mutants
kills seven patches, proving that the checks go red; stripping the hooks
leaves CALCULAT.EXE byte-identical (6,656 bytes). make smoke uses the booted
product image and grades the window against the independent real-Mac PNG.
Screendump: build/apps/calculator/smoke/window.ppm.

Remaining unclaimed legacy Calculator emulator traces and Apple-menu checks
belong to the desktop/factory owners; the app routing and pixel checker are
claimed here. No private Toolbox fake remains.
