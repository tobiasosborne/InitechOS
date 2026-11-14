# C application examples

CTENANT.EXE is the minimal C example of the SDK gate, never a product app.
FILETEN.EXE, built under filegate/, is a separate file-ABI test executable.
Dependencies: SDK only. Both use plain mk/tenant.mk flags, no semantic defines.

CTENANT.EXE registers, opens a window and menu, paints startup text, shows
key input in proportional and fixed-grid text, retains mouse clicks, repaints
those readouts after exposure, and quits. No dead commands or persistence.
FILETEN is factory test code under tests/: it creates/reads/seeks/deletes,
checks fifteen open handles and has an explicit emulator-only crash command
that corrupts its own PSP before trapping. It is not product code or a model
for an application to copy. No menu is installed in that test fixture.

make test runs seven C-input/repaint checks and four file-transport checks
on sdk/host in <1s; authored contract regressions, no period differential.
make mutants kills both retained-input/declined-quit patches; hook removal
keeps CTENANT.EXE identical. The shared fake lacks reserved DOS-device name
rejection: test_files.c supplies only that precondition for nul.txt; ordinary
file operations use sdk/host. Target pointers use uintptr_t so the same
fixture can be hosted without truncating addresses. The crash command is
never executed in host tests. make smoke fails explicitly: legacy composed
pixel/cleanup journeys are unimported and belong to factory.

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: desktop
Interface use-gate: sdk/include/tbx.h
<!-- end-interface-declarations -->
