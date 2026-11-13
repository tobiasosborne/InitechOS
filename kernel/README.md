# InitechDOS kernel

`make` builds build/kernel/INITDOS.SYS, a variable-length flat image at
0x500000 with a K386 loader header and linker-derived BSS bounds. The resident
stack remains [0x5f0000,0x600000); the retired transient and overlay reservations
no longer reduce the image window. No LTO, section GC, unusual ABI flags,
per-file optimizations or fixed-sector padding are used. All C uses the shared
TARGET_CC/TARGET_CFLAGS, with default cdecl. isr.asm already passes arguments
on the stack and cleans them at its call sites; its KERNEL_ASM_ABI declarations
explicitly preserve that convention. kstart.asm calls kernel_main with no
arguments; loader.c retains its existing nonlocal transfer assembly. Both
emulators exercise INT21, IRQ delivery, external program transfer and return.

main.c is the single production entry: startup, then permanent COMMAND.COM.
startup.c extracts the legacy-final BOOT_SHELL wiring: interrupt foundation,
console/banner, slave FAT mount and fixed-drive mounts, file/EXEC/absolute-disk
backends, RTC, ANSI console, vector table, CONFIG.SYS, then IRQ/input enable.
The old boot demos/selftest ISR are absent. Persistent mount/driver objects are
static because startup returns before the interpreter runs AUTOEXEC.BAT.
Reference behavior is legacy-final kernel_shell / tracer_boot.img; the hardware
and DOS references inside the original drivers remain the behavioral sources.

Public machine/DOS contracts, including the existing physical-disk/INT13 host
transport declarations in block_device.h, are in include/, claimed from attic/spec and the
old kernel headers. dos_messages.json is compiled by gen_messages.c (host C)
into C/NASM catalogues under build/kernel. loader_validation.c is the real
validation formerly in loader_cold.c, included by loader.c as before, with no
external COLD mechanism. The console has private legacy pixel-format primitives
in console_pixels.*; the kernel has no dependency on FLAIR or apps. The mouse
IRQ driver remains kernel hardware; desktop event routing is not built.

Dependencies: dos/command/entry.h is the sole temporary resident interpreter
entry boundary. COMMAND.COM is linked into this image until the process
boundary work described in dos/README.md. No other component private headers
are permitted. `make smoke` runs the outside factory smoke, including normal disk
programs on a separate test volume. `make test` runs the retained host gates
through tests.mk and mk/host-test.mk. tests/tools.mk registers the real C
harnesses; gate adapters preserve multi-command mtools/reference assertions in
isolated scratch directories. Shared host objects avoid compiling int21.c for
every invocation. All target-build rules remain in Makefile.

Dependencies used only by host tests: factory/testlib, kernel public headers,
and the resident shell headers already documented above. Tests do not build
from attic. `PYTHON_FAT_REFERENCE` and `PYTHON_MESSAGE_REFERENCE` isolate the
inherited Python oracle dependency pending the operator's language decision.
THOROUGH=1 restores full legacy seed sweeps; the default has fixed bounded
seeds. Missing oracle tools fail. See tests/IMPORT.md for exact parity and
remaining ownership, and tests/SMOKES.md for the full emulator migration.

All inactive mutation guards were converted to patches. `make mutants` sweeps
the proved host patches with unmodified-baseline and mutated-build checks.
Nested mutants/pending/ files have no accepted kill yet; tests/MUTANTS-TODO.md
lists every one. Conversion is not a complete mutation certificate.

Follow-up work:

- loader.c still exposes native tenant layout helpers and carries the old
  256-byte PSP / 512 KiB BSS limits from toolbox_gate.h. They are
  DOS executable-format/layout operations, with no GUI dependency, but the
  future SDK/kernel contract should own these constants explicitly.
- loader_validation.c retains rejection of COMMAND3 and FLAIROVL content, so
  old non-program files cannot become executable merely by being renamed.
  There is no kernel overlay loading or exact-address COMMAND transient.
- CONFIG.SYS retains legacy accepted(deferred) DEVICE/INSTALL/BUFFERS handling;
  windowed FAT16 EXEC limitations, fixed native/program windows, and the
  low-memory 8 MiB requirement remain unchanged. These need later scope work.
- The parked desktop quarry belongs to the FLAIR desktop owner and is not built.
  Diagnostic-image and remaining program/session work is mapped in tests/SMOKES.md.

The optional environment hook is kernel_graphical_environment (public
include/environment.h). Its weak DOS-only default reports FLAIR is not
installed and returns. The desktop supplies the strong implementation only
in the system/ composition. desktop_console_restore restores the original
framebuffer console and indexed palette after that implementation returns.
The kernel has no Toolbox include or link dependency.

loader_load_tenant no longer limits complete application files to 64 KiB.
There was no fixed staging buffer: the supplied callback carves PSP + rounded
file + BSS. 32-bit addition/rounding is checked for overflow, and allocation
failure returns LOADER_ERR_NOMEM. Existing EXE validation, e_minalloc format
and 512 KiB BSS policy remain. include/ now publishes the unchanged hardware,
loader, framebuffer and input/timer contracts used by the desktop binding.
