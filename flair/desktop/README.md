# FLAIR desktop (parked)

Finder, shell, Apple menu, film arrangement, power notices and easter eggs,
with their host tests. These files are not built by the Toolbox component.
The spine's desktop half of kmain (kmain_legacy.c), tbx_gate.asm and support
files are parked alongside the Toolbox import's Finder/shell/film sources.
Both sets survive unbuilt. Follow ../NOTES-FOR-KERNEL.md to bind libflair.a
and build/flair/region/target/region.o to the bootable kernel.

finder_ops_cold.c and finder_windows_cold.c are size-split halves, not separate
managers. Rejoin each with its matching .c when importing the desktop; the old
kernel size cap and COLD placement are retired. No halves are rejoined here.

Drag/compositor and menu-handler tests are parked here because they link the
desktop implementation. Their Toolbox dependencies remain imported separately.
Assets and the render harness are now in ../include/assets and ../tests/render.
