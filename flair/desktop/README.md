# FLAIR desktop (parked)

Finder, shell, Apple menu, film arrangement, power notices and easter eggs,
with their host tests. These files are not built by the Toolbox component.
The kernel engineer supplies the desktop half of kmain and its support files
separately. The desktop import follows the bootable kernel import.

finder_ops_cold.c and finder_windows_cold.c are size-split halves, not separate
managers. Rejoin each with its matching .c when importing the desktop; the old
kernel size cap and COLD placement are retired. No halves are rejoined here.

Drag/compositor and menu-handler tests are parked here because they link the
desktop implementation. Their Toolbox dependencies remain imported separately.
Assets and the render harness are now in ../include/assets and ../tests/render.
