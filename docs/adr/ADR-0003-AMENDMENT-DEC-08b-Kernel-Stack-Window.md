# ADR-0003 Amendment DEC-08b: Kernel stack window in the high runway

Status: DECIDED (PM lane kstack, bead initech-xu97, 2026-10-20). Amends
only the stack clause of the DEC-08b high-kernel relocation drafted in
`docs/design/kernel-runway-relocation.md` K2 (bead initech-tdnl.29). The
kernel format, handoff, KERNEL_BASE/KERNEL_CEIL, the 5120 KiB RAM minimum,
the FLAIR heap (ADR-0004 DEC-03) and every PROGRAM_* constant are unchanged.

The design's stack window [0x4F0000,0x500000) lies inside the locked FLAIR
heap [0x100000,0x500000); the tdnl.29 Step-0.3 audit stopped it and kept the
conventional stack [0x90000,0xA0000). This amendment assigns the kernel stack
the top 64 KiB of the runway, [KERNEL_STACK_BOT, KERNEL_STACK_TOP+4) =
[0x5F0000,0x600000), ESP = 0x5FFFFC. The runway is now partitioned: kernel
image including .bss [0x500000,0x580000) (bounded by the new
KERNEL_IMAGE_CEIL == TRANSIENT_BASE at link in kernel.ld, at build in the
Makefile kernel-end guard, and at boot in the kmain KAT panic); transient
COMMAND.COM [0x580000,0x5C0000); 192 KiB unassigned [0x5C0000,0x5F0000);
stack. Why this window: it is the only free 64 KiB the map proves unused
that needs no RAM-gate change (it is below KERNEL_CEIL, so INITECH_MIN_EXT_KB
already backs it), the stack grows down into unassigned runway rather than
toward a live window, and kstart.asm's existing runway zeroing clears it
before the first push. The largest of all 246 kernel ELFs ended at
0x541A0C when this was ratified, so the tighter image bound takes no room
any kernel uses. Placing the stack above KERNEL_CEIL was rejected because it
would raise the installed-RAM minimum; returning to conventional memory was
rejected because it is the exhausted gap this amendment retires.

`spec/memory_map.h` names ten windows (with `os/milton/boot_info.h`'s
boot_info block and font stash) and asserts every pair disjoint at compile
time. `make test-memory-map` prints the map, and `make test-memory-map-mutant`
proves the asserts catch a regression: the stack re-overlapped with the heap
(the original defect), the stack overlapped with the transient window, and
a drifted kstart.asm stack equate. Each must fail with its own named
diagnostic. The VBE linear framebuffer is a runtime BIOS value and cannot be
a compile-time window. The static half of the argument is that every window
below KERNEL_CEIL is RAM-backed, which the boot gate enforces. The
mode-0x13 aperture [0xA0000,0xC0000) is asserted directly.
