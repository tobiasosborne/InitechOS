# ADR-0003 Amendment DEC-19: The Resident Toolbox Entry

Status: DECIDED for initech-tdnl.106 ("FLAIR as a command: boot to
InitechDOS, type FLAIR, Exit returns to the prompt"; operator question
2026-10-17: "we can boot to dos, then start flair right?"). DEC-19 is the
next unused numbered decision in ADR-0003 and its amendments (DEC-17 is the
physical block device service, DEC-18 the drive letters over it).

## Context

Until this amendment every desktop kernel entered the FLAIR pump at the end
of SYSINIT and COMMAND.COM ran only after Special > Exit to InitechDOS, as a
one-way hand-off (initech-3p1a). PRD Sec 2 promises a DOS 3.3 personality
with a command shell; PRD Sec 1.4 / ADR-0012 put FLAIR forward as a platform
over that DOS, the way the period's GUI environments ran over MS-DOS. The
period behaviour is: the machine boots to the banner and the prompt,
AUTOEXEC.BAT may start the GUI, the GUI is an ordinary command, and leaving
it returns to the prompt.

## Decision

1. **The system boots to COMMAND.COM.** In every kernel that carries the
   Finder and its Exit (`FLAIR_AS_COMMAND` in `os/milton/kmain.c`: the
   flagship and its gate, record, interactive and mutant variants) SYSINIT
   ends in `flair_shell_boot`: drive A: is mounted, the INT 81h gate is
   installed with the locked attributes, the devices (PIT, keyboard, PS/2
   mouse) are brought up once, COMMAND.COM's DOS seams are bound, the
   transient COMMAND.COM is read from the boot disk (`COMMAND-TRANSIENT-OK`)
   and becomes the permanent processor, which runs `A:\AUTOEXEC.BAT`. The
   film-scene tableau kernel (no Finder, no Exit, no COMMAND.COM on its
   image) still boots to its desktop. DOS-only images are unchanged.

2. **FLAIR is an external command, `\DOS\FLAIR.COM`.** Chosen over a
   COMMAND.COM internal because the period's GUI shells were external
   programs (Windows 3.x's WIN.COM is an external command; nothing in the
   DOS 3.3 internal-command set starts an environment), and because an
   external keeps COMMAND.COM's internal-command table exactly the DOS 3.3
   set. FLAIR.COM is a 108-byte flat program (DEC-08,
   `os/milton/flair_program.asm`): it issues INT 81h with AX =
   `TBX_DESKSTART` and terminates with code 0; on a refusal it prints one
   of two authored lines and terminates with code 1. The shipped volume's
   AUTOEXEC.BAT ends with the line `FLAIR`, so the shipped boot still lands
   on the desktop, after the banner and AUTOEXEC.

3. **ONE gate: INT 81h AX = `TBX_DESKSTART` (0x0003).** Locked in
   `spec/toolbox_gate.h` Sec 3 and Sec 17 (the INT 81h Toolbox Gate is the
   Toolbox's only door, ADR-0013; an INT 21h vendor subfunction would have
   put a Toolbox verb into the DOS register, Appendix A). It is the one code
   the gate answers while the Toolbox is dormant. The rule
   (`os/flair/tbxdesk.h`, compiled into both the kernel and the host
   oracle): DORMANT -> ARMED when a running DOS program asks; a second
   request while ARMED or ACTIVE (from a DOS program or a disk tenant) is
   `TBX_ERR_BUSY` and changes nothing; a stray `int 81h` from the kernel's
   own context is `TBX_ERR_BADCODE`. Serial: `FLAIR-DESKSTART rc=<rc>`.

4. **The desktop runs inside COMMAND.COM's EXEC of FLAIR.** IBM DOS 3.30
   Technical Reference pp1-5/1-6 (PDF22-23): the resident portion of
   COMMAND.COM holds the EXEC routine; the transient issues EXEC for an
   external command and regains control when the program terminates. The
   FLAIR-as-a-command kernels bind COMMAND.COM's AH=4Bh backend to the
   resident EXEC `flair_cmd_exec`: it runs the program; if the program armed
   the Toolbox, it starts the desktop (ARMED -> ACTIVE) before COMMAND.COM
   resumes. The desktop therefore runs over the shell's PSP chain (the
   kernel PSP COMMAND.COM runs in), on the system stack below COMMAND.COM's
   frames, with the program window [PROGRAM_BASE, PROGRAM_ALLOC_END) free
   -- so the desktop's own full-screen DOS programs (the system hotkey's
   text tenant) still run there, and the transient window
   [TRANSIENT_BASE, TRANSIENT_CEIL) is untouched. The launcher has already
   terminated: no swap of the launcher is needed (Windows standard mode's
   DOS-application swap has no counterpart here).

5. **Exit returns to the prompt.** Special > Exit to InitechDOS keeps the
   whole existing teardown (the alert, every application asked to quit, the
   DESKTOP.DB commit, the hooks out, the fault triage unbound, the console
   re-initialised and the banner printed), then verifies the transient
   (below; `COMMAND-TRANSIENT-OK bytes=<n>`), prints `SHELL-READY`, returns
   the Toolbox to rest (`tbx_desk_end`: DORMANT, no host bound, the one-slot
   host empty) and returns -- by `__builtin_longjmp` to the session entry
   inside the resident EXEC -- to the COMMAND.COM that ran FLAIR, which
   continues its batch or prompts. The drive's current directory is
   COMMAND.COM's own (the loader restored it when FLAIR.COM terminated).
   FLAIR can be entered again: every desktop re-prepares its volume files
   (DESKTOP.DB, TRASH, the Apple menu, the label), re-initialises the heap,
   the event queue and the menu-bar clock, and is pixel-identical to the
   first.

6. **The transient checksum rule.** IBM DOS 3.30 Technical Reference p1-5
   (PDF22): "When a program terminates, a checksum methodology determines if
   the program has caused the transient portion to be overlaid. If the
   transient portion is overlaid, it is reloaded." The transient header
   gains `code_end` (`os/milton/transient.h`, `transient_hdr.asm`,
   `transient.ld`). At SYSINIT the kernel keeps its OWN copy of the
   validated header and the 16-bit word checksum of the code span
   [TRANSIENT_BASE, code_end) -- the header, .text and .rodata, the bytes no
   run of the interpreter changes. .data/.bss are the interpreter's live
   state (the environment, the batch in progress) and are not summed. After
   every program the resident EXEC sums the span; on a mismatch it re-reads
   exactly that span from the same COMMAND.COM (its header must equal
   SYSINIT's, or MSG-DOS-0014 + halt) in place, logs
   `COMMAND-TRANSIENT-RELOAD`, and returns into the repaired interpreter with
   its live state kept. The boot disk is fixed, so the period's
   "Insert COMMAND.COM disk" retry (MS-DOS 3.3 User's Guide printed p338,
   PDF423) has no counterpart; a reload that cannot be made halts with
   MSG-DOS-0014 (DEC-13 vocabulary; Rule 2). Nothing in the memory map
   overlays the transient, so the reload is exercised only by the
   fault-injection kernel `KMAIN_FIX_TRANSIENT_OVERLAY`.

7. **Boot-time absence of COMMAND.COM.** A FLAIR-as-a-command image without
   a valid COMMAND.COM now says MSG-DOS-0014 at SYSINIT and halts -- the
   initial interpreter activation the MS-DOS 3.3 User's Guide printed p310
   (PDF395) documents -- instead of at Exit.

## Deviations, stated

- The batch context lives in the transient's .data/.bss, not in a resident
  batch segment: a reload keeps it only because the reload restores the
  code span alone. An overlay of .data/.bss is not detected (DOS's checksum
  covered only the transient too).
- FLAIR.COM's two lines are authored (no local reference names them):
  `FLAIR is already running.` and `FLAIR cannot start the desktop.`
  (`spec/toolbox_gate.h` Sec 17).
- The bounded gate pump still halts at `FLAIR-LIVE-OK` when its tick budget
  runs out; the interactive images run until Exit.

## Oracles

- `test-flair-cmd-host` (+ `-mutant`): the gate rule (D1), the checksum rule
  (D2), FLAIR.COM's locked lines (D3); mutants `TBXDESK_MUT_FLAIR_TWICE`,
  `TBXDESK_MUT_LEAVE_KEEPS`, `TRANSIENT_MUT_NO_CHECK`,
  `TRANSIENT_MUT_SUM_DATA`.
- `test-flair-from-dos` (+ `-mutant`), QEMU: prompt first; FLAIR -> desktop
  -> Exit -> prompt, twice, then a third desktop byte-identical to a fresh
  boot's and graded by `ppm_flair_appswitch_check`; a second FLAIR from a
  running desktop refused; an overlaid transient reloaded. Mutants
  `KMAIN_MUT_EXIT_RESETS_CONSOLE_ONLY`, `KMAIN_MUT_TRANSIENT_NOT_PRESERVED`,
  `TBXDESK_MUT_FLAIR_TWICE`. The Bochs leg is owed (Bochs 2.7 ENOMODEs
  640x480; the desktop guard halts the session after the prompt).
- Every existing gate that starts at `FLAIR-FAT-MOUNT-OK` / `FLAIR-LIVE-READY`
  keeps its locked trace and pixels: the desktop now appears after the
  banner and AUTOEXEC.BAT.
- Rule 14 clip: `record-flair SCRIPT=flair_from_dos`.
