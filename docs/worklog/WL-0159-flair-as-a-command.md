# WL-0159 -- FLAIR as a command (initech-tdnl.106)

## Context

Operator question 2026-10-17: "we can boot to dos, then start flair right?"
Until now every desktop kernel entered the FLAIR pump at the end of SYSINIT
and Special > Exit to InitechDOS was a one-way hand-off to the transient
COMMAND.COM (initech-3p1a). The bead asks for the period flow: boot to the
banner and A:\>, FLAIR starts the desktop, Exit returns to the prompt, FLAIR
works again. Decision record: `docs/adr/ADR-0003-AMENDMENT-DEC-19-Resident-Toolbox-Entry.md`.
(The lane was taken over by Claude Opus from a stopped agent; the clone held
no work, so everything here is this pass's.)

## What changed

- `os/milton/kmain.c`: the BOOT_FLAIR_LIVE arm of kernel_main is now the
  function `flair_live_session()` (byte-identical behaviour for the
  boot-to-desktop kernels). `FLAIR_AS_COMMAND` (every FLAIR_LIVE_TENANTS
  kernel except the film-scene tableau and the headless smoke) ends SYSINIT
  in `flair_shell_boot`: INT 81h gate installed dormant, PIT/kbd/PS2 mouse
  brought up ONCE, COMMAND.COM's seams bound with the resident EXEC
  `flair_cmd_exec`, the transient read (COMMAND-TRANSIENT-OK) and run as the
  permanent processor. `flair_cmd_exec` starts an armed desktop
  (`flair_desktop_session`, `__builtin_setjmp`) when FLAIR.COM's EXEC returns,
  then sums the transient. Exit (`flair_exit_to_dos`) keeps the whole
  teardown, verifies the transient, prints the banner and SHELL-READY,
  `tbx_desk_end()`s and `__builtin_longjmp`s back. The Finder's volume work
  (DESKTOP.DB, TRASH, Apple menu, label) moved to
  `flair_volume_desk_prepare()`, once per desktop; the menu-bar clock resets
  per desktop (`MenuClock_reset`).
- `os/flair/tbxdesk.h` (new): DORMANT/ARMED/ACTIVE and the gate rule.
  `os/flair/tbxgate.c`: AX=TBX_DESKSTART answered with no host bound
  (FLAIR-DESKSTART rc=), `tbx_desk_begin/end`, `tbx_trace_bind`.
- `spec/toolbox_gate.h`: TBX_DESKSTART 0x0003, Sec 17, FLAIR.COM's two lines.
- `os/milton/transient.h` / `transient_hdr.asm` / `transient.ld`: `code_end`;
  the DOS 3.30 TR p1-5 word checksum over header+.text+.rodata; the reload
  re-reads that span in place (`exitdos_load_transient(1)`).
- `os/milton/flair_program.asm` (new): FLAIR.COM, 108 bytes, `\DOS`.
- `os/milton/int21.c`: `int21_flush_input()` (AH=0Ch's flush, no read).
- `os/milton/loader.c`: `loader_program_running()`.
- Makefile: FLAIR.COM is the 15th DOS external; the shipped AUTOEXEC.BAT ends
  with `FLAIR`; every flagship-family image carries COMMAND.COM (icon object
  mutants, the visible-TRASH hygiene kernel); record block `flair_from_dos`
  (+ `RECORD_KEYS_AFTER_<script>`).
- `harness/emu/qemu.c`: `W<n>:<marker>` waits for the n-th marker.
- New gates: `harness/emu/flair_cmd.mk`, `spec/flair_cmd_traces.mk`,
  `harness/proptest/test_flair_cmd.c`.
- Re-keys (all for the new boot order, none weakened): NO_TRANSIENT,
  COMMAND_VOLUME and the command-file negatives refuse at SYSINIT (MS-DOS 3.3
  UG p310, the initial load, now literally); `spec/dos_folder_externals.txt`
  + FLAIR.COM; the DOS-folder host oracle's AUTOEXEC bytes; DOS_NOT_DIRECTORY
  starts the desktop as APPS\FLAIR; FLAIR_FIXTURES + DOS/FLAIR.COM; the
  SAMIR desktop volumes run FLAIR from AUTOEXEC.BAT
  (`SAMIR_LIST_FLAIR_IMG`, the dbsafe desktop leg).
- pubs: the InitechDOS Reference FLAIR entry.

## Why

Law 1 sources: IBM DOS 3.30 Technical Reference pp1-5/1-6 (PDF22-23: the
resident portion holds EXEC and the transient reload; "When a program
terminates, a checksum methodology determines if the program has caused the
transient portion to be overlaid. If the transient portion is overlaid, it
is reloaded."); MS-DOS 3.3 User's Guide printed p310 (PDF395, MSG-DOS-0014 at
start-up) and p338 (PDF423, the reload prompt -- no counterpart: the boot
disk is fixed); ADR-0003 DEC-03/DEC-11; ADR-0013 (INT 81h is the Toolbox's one
door). External over internal: the period's GUI shells were external
programs (WIN.COM), and COMMAND.COM's internal set stays DOS 3.3's.

## Frictions

- Leg [3] (a second FLAIR from the running desktop) found a real leak: the
  system hotkey's `\` reached the prompt after Exit, 4 runs in 4. Root cause:
  the desktop's menu-bar clock polls the time through `int21_dispatch`
  (AH=2Ch); with BREAK ON's keyboard check that call parks a pending key in
  INT 21h's one-key lookahead, which the exit's ring drain never saw. Exit
  now flushes both (`int21_flush_input`). The old one-way exit had the same
  hole; nothing exercised it.
- The tbxgate trace is routed through the bound host's `puts`; a dormant
  Toolbox has no host, so `tbx_trace_bind` gives it the serial puts alone.
- `test-fixture-volumes` is RED on origin/main itself
  (`build/panic-screen/bochs-data.img` lacks a contract, the PC LOAD LETTER
  lane); this lane's volumes passed its lint before the merge (green, 363
  scratch names vs 154 fixture names).

## Acceptance

(see the lane report; every gate below run on lane/flaircmd after merging
origin/main 1fedcd22)

## Pointers

- Bochs legs owed (not run in this lane): every flagship Bochs leg now boots
  through COMMAND.COM (the 640x480 guard halts the session after the prompt);
  a test-flair-from-dos Bochs leg is not written.
- Clip: `make record-flair SCRIPT=flair_from_dos` (PM, on main).
