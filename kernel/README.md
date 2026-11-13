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

Public machine/DOS contracts are in include/, claimed from attic/spec and the
old kernel headers. dos_messages.json is compiled by gen_messages.c (host C)
into C/NASM catalogues under build/kernel. loader_validation.c is the real
validation formerly in loader_cold.c, included by loader.c as before, with no
external COLD mechanism. The console has private legacy pixel-format primitives
in console_pixels.*; the kernel has no dependency on FLAIR or apps. The mouse
IRQ driver remains kernel hardware; desktop event routing is not built.

Dependencies: dos/command/entry.h is the sole temporary resident interpreter
entry boundary. COMMAND.COM is linked into this image until the process
boundary work described in dos/README.md. No other component private headers
are permitted. `make smoke` runs the outside factory smoke. `make test` reports
the pending host-suite import; this component is deliberately not yet listed
in top-level COMPONENTS as a completed tier-0 import.

Follow-up work:

- tests/ contains untouched kernel/test_*.c and test helper headers; tests/programs/
  holds the small ASM probes/fixtures formerly beside production drivers.
  Claim test-specific fixtures and host stubs from attic with git mv. Repair
  legacy relative includes to include/ and the new source locations. Inspect
  each legacy target's source/flags with make -n in the read-only legacy tree,
  then use mk/host-test.mk. Preserve hosted seams; do not compile probes into
  production startup. The heap/desktop host test is under flair/desktop/tests/.
- tests/MUTANTS-TODO.md enumerates every remaining guarded mutation macro in
  built kernel, DOS and bootstrap files. These are disabled by ordinary flags;
  convert each to a patch plus its independent red oracle. Small mutation
  blocks were removed where straightforward. Factory fault macros remain in
  its imported drivers, separately from product source.
- loader.c still exposes native tenant layout helpers and carries the old
  256-byte PSP / 64 KiB file / 512 KiB BSS limits from toolbox_gate.h. They are
  DOS executable-format/layout operations, with no GUI dependency, but the
  future SDK/kernel contract should own these constants explicitly.
- loader_validation.c retains rejection of COMMAND3 and FLAIROVL content, so
  old non-program files cannot become executable merely by being renamed.
  There is no kernel overlay loading or exact-address COMMAND transient.
- CONFIG.SYS retains legacy accepted(deferred) DEVICE/INSTALL/BUFFERS handling;
  windowed FAT16 EXEC limitations, fixed native/program windows, and the
  low-memory 8 MiB requirement remain unchanged. These need later scope work.
- Untouched tests/programs retain legacy codenames and paths by instruction;
  rename as their tier-0/tier-1 imports claim them. Parked desktop quarry also
  retains references to retired paths and headers and must not be built.
