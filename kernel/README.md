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
- Windowed FAT16 EXEC limitations, fixed native/program windows, and the
  low-memory 8 MiB requirement remain.
- The parked desktop quarry belongs to the FLAIR desktop owner and is not built.
  Diagnostic-image and remaining program/session work is mapped in tests/SMOKES.md.

The optional environment hook is kernel_graphical_environment (public
include/environment.h). Its weak DOS-only default reports FLAIR is not
installed and returns. The desktop supplies the strong implementation only
in the system/ composition. desktop_console_restore restores the original
framebuffer console and indexed palette after that implementation returns.
The kernel has no Toolbox include or link dependency.

`make smoke-conpair` builds the product image and runs a focused extended-key
regression on QEMU and Bochs at 8 MiB; `make smoke` runs this before the
aggregate smoke. A normal disk program reads 00h, outputs a line, then reads
the Alt-Q/Alt-R scan byte. Required guest verdicts and independent mtools
readback compare with literal pairs 00h/10h and 00h/13h. The program is installed
only on disposable test disks, never in the product image. Source: MS-DOS
Encyclopedia AH=07h two-read key contract and the IBM set-1 keyboard mappings
already exercised by the keyboard driver tests (initech-drm4v).

The regression's negative control is `mutants/con-pair-flow-control.patch`,
which removes the scan-byte guard from CON flow-control interpretation. Its
registered tier-0 kill is `test-conkeys-unit`. To repeat the emulator kill,
from the repository root, build a disposable source snapshot (no shared source
is edited):

```sh
conpair_copy=$(mktemp -d)
git archive HEAD | tar -x -C "$conpair_copy"
patch --fuzz=0 -d "$conpair_copy/kernel" -p1 < kernel/mutants/con-pair-flow-control.patch
make -C "$conpair_copy/kernel" -j6 smoke-conpair
```

That patched run must fail the byte/verdict assertions; the unmodified
`make -C kernel -j6 smoke-conpair` must pass. Both retain per-emulator serial,
fault reports, disk images and independent expected/actual bytes below
`build/kernel/smoke/conpair/`. The snapshot uses committed HEAD; commit the
test/client changes before replaying it.

loader_load_tenant no longer limits complete application files to 64 KiB.
There was no fixed staging buffer: the supplied callback carves PSP + rounded
file + BSS. 32-bit addition/rounding is checked for overflow, and allocation
failure returns LOADER_ERR_NOMEM. Existing EXE validation, e_minalloc format
and 512 KiB BSS policy remain. include/ now publishes the unchanged hardware,
loader, framebuffer and input/timer contracts used by the desktop binding.

Failed replacement CREAT and UNLINK preserve the original directory entry,
allocation map and data after recoverable metadata I/O errors (initech-rg6d5).
The shared destructive-metadata helper snapshots the FAT, validates the complete
old chain before mutation, and restores both FAT copies and the directory entry
on failure, including an error reported after a write reached the medium.
Rollback failure reports WRITE; persistent hardware failure and power loss are
not certified recovery cases. No data sector is written by either operation.
`test-file-conservation` faults every physical metadata write, before and after
persistence, in root and subdirectory cases with fragmented originals. An
independent FAT12 reader checks allocation ownership and matching FAT copies;
mtools extracts the original bytes after reopening; failed images must equal
the input image byte for byte. Successful delete/truncate controls also run.
Source: the FAT allocation invariant, CLAUDE.md's The constitution, and the
metadata layout in the Microsoft FAT specification. These checks establish
conservation, not historical DOS error-state parity.

Resident SFT entries 0..3 are pinned for the system lifetime. Their initial
counts remain fixed across inherit, DUP, DUP2, CLOSE and process teardown;
process-owned file and by-name device entries still count every JFT reference.
This completes the existing resident-device representation without changing
boot or shadow PSP construction. `console-inherit` checks parent handles across
child redirection/exit, with and without a saved console DUP; `review-handles`
checks repeated resident DUP/CLOSE. Source: IBM DOS 3.30 Technical Reference
p.4-15 handle ownership and the invariant that a live parent JFT cannot point
to a freed SFT entry. Pinning describes this kernel, not real DOS internals.

The kernel remains host-green, not imported: main.c and startup.c include the
private DOS command/entry.h. Publishing that resident interpreter header needs
the DOS owner, plus corresponding include paths and dependency documentation.
The import check is unchanged; no local duplicate declarations hide the boundary.
The runtime resident interpreter exception remains an operator decision.

Seven pending mutations now have confirmed host kills: full-disk error mapping
(test-zs24), BREAK cleanup/recovery/disk-entry and caret-C (test-dos-break),
not-ready output (test-dos-not-ready), and character-output redirection
(test-redir). Those patches are active. The remaining 32 pending patches still
have no accepted kill. The hide-directory patch was refreshed after metadata
helper insertion changed its context; its existing selection still survives.

CREATE_NEW treats only an explicit FILE_NOT_FOUND as proof of absence;
lookup/media errors refuse before the truncating backend (initech-06l23,
IBM DOS 3.30 TR p.6-215). `test-create-new-error` checks that a one-shot
existence-probe read fault preserves the original bytes and complete image.

Positioned extension and MKDIR now have a complete metadata rollback boundary,
covering late FAT-copy, payload and directory-publication errors. RMDIR shares
the destructive-metadata rollback with CREAT/UNLINK. Failed writes acknowledge
zero bytes. Newly allocated/free data sectors may have changed, but both FAT
copies and allocation ownership are restored, and old named payloads survive.
This does not make ordinary in-place payload writes transactional. The additional
`test-allocation-conservation` sweep checks empty-file growth, append, MKDIR and
RMDIR in root/subdirectories with before/after-persistence faults, an independent
FAT reader, mtools readback and successful-operation controls (initech-lelwp,
initech-rxh8o and the RMDIR follow-up to initech-rg6d5).

WRITE refuses a failed final metadata refresh rather than acknowledging bytes
with stale SFT state. An opened file's I/O failure reports general failure,
not FILE_NOT_FOUND. General failure is the chosen primary error policy; exact
INT24 Fail/AH=59h parity is not claimed without further captures. Full root
refusal returns access denied, matching the retained genuine DOS 3.30 capture
in tests/fixtures/rootfull33/ (initech-knlbk, initech-2x5l7, initech-xhtlm;
IBM DOS 3.30 TR pp.6-42, 6-137..140). Tests exercise the real FAT backend through
INT21, not a mock file backend.

CREAT's runtime timestamp now participates in its directory commit instead of
being a separate fallible write after truncation. `test-create-stamp-conservation`
binds a fixed DOS clock and faults each backend write, including the former
post-truncation timestamp write; errors preserve the original image, and success
publishes the known DOS timestamp. The lower-level fat12_create entry retains
its deterministic default timestamp. Read-only positioned writes refuse before
entering the recovery boundary, so refusal does not issue rollback writes.

Qualified character-device opens validate the selected drive and containing
 directory before binding the device; device basenames still take precedence
 over ordinary files in an existing directory. Bare device names need no media.
 `test-device-paths33` compares OPEN/IOCTL/CLOSE classifications with the genuine
 DOS 3.30 DEVICES trace, including valid SUB aliases and missing-parent/invalid-
 drive refusals (initech-905b8). The capture proves refusal, not the exact AX code.

CONFIG.SYS refusals are visible on the console with captured DOS 3.30 wording;
malformed lines do not abort boot or silently change a setting. Numeric tokens
accept the captured whitespace-delimited suffixes; numeric junk and out-of-range
values refuse. FILES accepts DOS syntax5..255, with an implemented capacity5..20.
One additional physical SFT slot accounts for separate CON input/output entries;
real independent OPEN counts at FILES5..8 match the native DOS captures. The
process JFT remains20 handles.

SHELL selects the resident COMMAND.COM with /P and optional /E:512. The frozen
interpreter omissions are external/qualified interpreter paths, nonpermanent
root shells and other environment capacities/options. They report the captured
Bad or missing Command Interpreter refusal and stop before AUTOEXEC; no request
is logged as applied while silently using the fixed512-byte environment.
`test-shell-config33` checks the real SYSINIT/FAT refusal and supported control;
`test-shell-startup` checks the actual production entry never invokes COMMAND
after refusal. `make smoke-shell-config` grades both emulators on the ordinary
image, with mtools image conservation on refusal and an AUTOEXEC write control.

The frozen CONFIG omissions are installable drivers other than the built-in
ANSI selector, configurable BUFFERS, INSTALL commands and FILES above20.
They receive a visible period refusal rather than a deferred-success marker.
The baseline still selects20 files. `test-config-diagnostics33` mounts captured
CONFIG inputs with the real SYSINIT/INT21/FAT stack, compares console messages
with genuine VGA transcripts, checks native DOS file capacity and verifies that
reading CONFIG leaves the image unchanged. Its malformed cases were red before
the repair; negative controls also kill silent BUFFERS and incorrect resident
console accounting. CONFIG is limited to1024 bytes; truncation is reported.

The PS/2 Ctrl-Pause make sequence E0/46/E0/C6 produces one DOS break byte and
uses the existing batch cancellation path. Source: IBM PS/2 Common Interfaces,
Keyboards September1991 p32/Figure14(part3), and genuine DOS3.30 BREAK captures.
`test-kbd-unit` checks left/right Ctrl, releases and plain Pause/Scroll Lock
controls. `make smoke-ctrl-break` sends the actual QEMU physical chord during
PAUSE, matches the native termination question, answersY and verifies an
unchanged disk/no continuation file. A plain-key control continues the batch
and independently reads the expected output with mtools. No input scenario is
compiled into the kernel.
