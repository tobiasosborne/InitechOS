<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0045 -- hsct (shell I/O redirection) ATTEMPTED + REVERTED: two real blockers found (a negative-result log)

**Type:** Feature attempt, reverted under the Laws; the value is the FINDING. **Date:** 2026-06-20. **Branch:** command-com-default (tree clean at the Wave 5 capstone 4e60bd5; nothing committed from this attempt). Continues WL-0044 (the DOS-3.3 parity capstone).

## Context

After the DOS-3.3 parity FUNCTIONAL capstone (40oq, WL-0044), the next consequential doable parity feature was `initech-hsct` -- COMMAND.COM I/O redirection (`> >> <`) + pipes (`|`). It builds on the certified handle layer (AH=45h DUP / 46h DUP2). An opus subagent built it; orchestrator grading (Law 4) found it functionally incomplete + window-blocked, so it was REVERTED rather than shipped (Law 2: the redirect oracle would be RED in-emulator; Rule 3: no bandaids).

## What was built (sound, host-proven) -- and why it was NOT shipped

- **Sound:** a PURE redirect parser (`cmd_redir_parse`: scans `< > >> |`, extracts targets, yields the clean command + pipe RHS) -- host-tested (test_command 203 checks, 2 mutants RED) -- and a DUP2-around-dispatch driver with strict handle-restore + a temp-file pipe.
- **BLOCKER 1 -- the redirect does not catch builtin output (the canonical case fails).** The shell builtins emit MOST output via AH=09h (`dos_print` -> `do_puts` -> `con_putc` DIRECT), which BYPASSES the JFT handle 1 -- so `DUP2(file, 1)` does NOT redirect it. Only AH=40h-handle-1 output (e.g. a future external filter) is caught. So `echo HELLO > file` -- the canonical interactive redirect -- would print HELLO to CON and leave the file empty. Real DOS routes AH=02h/09h to STDOUT (handle 1), so they ARE redirectable; InitechOS's direct-con_putc `do_puts` breaks that. **FIX (new bead): route AH=09h/02h/06h CON output through the redirectable handle 1** -- a careful int21.c CON-path change that interacts with p96i's con_putc ANSI hook + the g_sink.
- **BLOCKER 2 -- the kernel window is EXHAUSTED.** The redirect driver's ~1.9 KiB of .text blew the 0x30000 program-load guard; it only fit via BATCH_FILE_MAX 2048->1536 + a 192-byte margin (untenable -- the next byte overflows). This is the recurring kernel-window pressure reaching its limit (3rd time this session, after the Wave 1 batch buffer + the Wave 4 option-B FAT-share). **FIX (escalated to P1, o0td): raise PROGRAM_BASE 0x30000->0x34000 PAIRED WITH reducing PROGRAM_BSS_RESERVE 0x10000->0x8000** -- the raise gives the kernel +16 KiB; the BSS_RESERVE cut RESTORES/GROWS SAMIR's disjoint heap arena (~64 KiB) so the raise does NOT break SAMIR (the failure that reverted the Wave 4 raise, y206). Blast radius: every .COM org + the secondary scratch pages + samir.ld/crt0 + test_hardware_spec + hardware.json; VERIFY SAMIR's runtime BSS fits 32 KiB; full emu + Bochs.

## Outcome

- **Nothing committed.** The working tree was reverted to the Wave 5 capstone (4e60bd5); kernel-end guard OK (_kernel_end=0x2f8e0). The two blockers are filed; hsct stays OPEN with the design captured (redo it once the prerequisites land).
- **The kernel-window wall is now the gating architectural issue for ALL further kernel growth.** The principled memory-map redesign (o0td, P1) is the prerequisite for the next kernel feature -- and it must be done carefully (the Wave 4 raise broke SAMIR; the win-win is the raise + BSS_RESERVE cut, with SAMIR-BSS verification).

## Why this is the right call (the Laws over "keep working")

Landing hsct would have shipped (a) a redirect that silently fails for `echo > file` (Law 2 -- the in-emulator oracle would be RED) and (b) a 192-byte kernel margin + a BATCH_FILE_MAX regression coupled to an unrelated feature (Rule 3 -- bandaids). CLAUDE.md: "If a fast path conflicts with any Law, choose the Law." The valuable, honest output is the two precisely-characterized blockers, not a half-working feature.
