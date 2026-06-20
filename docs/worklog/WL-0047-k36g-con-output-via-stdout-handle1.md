<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->

# WL-0047 -- initech-k36g: route AH=09h/02h/06h CON output through the redirectable STDOUT handle 1

**Type:** DOS-authenticity feature on the most load-bearing CON path. **Date:** 2026-06-20.
**Branch:** command-com-default (follows WL-0046 o0td). **Bead:** initech-k36g (P2) -- the prerequisite
that unblocks `initech-hsct` (`> >> < |` shell redirection).

## Context

WL-0045 (the reverted hsct attempt) found BLOCKER 1: COMMAND.COM builtins emit most output via INT 21h
AH=09h (`do_puts -> con_putc` DIRECT), bypassing the JFT handle 1, so `DUP2(file,1)` did NOT redirect
them -- `echo HELLO > file` would print to CON and leave the file empty. Real DOS routes AH=02h/09h/06h
to STDOUT (handle 1), which IS redirectable. k36g makes InitechDOS match. It was gated behind o0td
(WL-0046) -- the kernel window had no room for the added code; o0td gave +32 KiB.

## What changed (os/milton/int21.c only, + the oracle + the Makefile gate)

The insight: `do_write` (AH=40h) ALREADY had the full output fan-out (resolve handle -> CON device =
`con_putc` per byte; OPEN-by-name device = `dev_route_rw`; FILE = positioned `write_at`; else
access-denied). The three "direct" output calls just bypassed handle resolution.

1. **Extracted `sft_write(e, buf, count, &written)`** -- the frame-less fan-out core, lifted verbatim
   out of `do_write`. `do_write` (AH=40h) now = resolve-handle + `user_buf_ok` (both unchanged) ->
   `sft_write` -> map onto EAX/CF/AX. AH=40h is byte-identical (test-int21 175/0, test-fileio 225/0).
2. **Added `stdout_emit(buf, count)`** -- resolves `sft_from_handle(g_cur_psp, INT21_HANDLE_STDOUT)`.
   If it resolves -> `sft_write` (CON branch = `con_putc` per byte = current behavior; FILE-redirected
   branch = bytes to the file). If it does NOT resolve (no current PSP / closed stdout -- the EARLY
   BANNER prints via AH=09h before the kernel JFT binds) -> FALLBACK to per-byte `con_putc` so output
   is NEVER dropped (Rule 2). This fallback is the load-bearing safety: the banner + every pre-PSP
   diagnostic survive unchanged.
3. **Rewired AH=02h (`do_putchar`), AH=09h (`do_puts`), AH=06h-output (`do_direct_conio`)** to emit
   through `stdout_emit` instead of `con_putc` -- preserving every other semantic (the '$'-scan +
   INT21_PUTS_SCAN_MAX bound + NULL guard + AL='$'; AL/CF returns; the ANSI hook; g_sink). Input-echo
   paths (cooked-read echo) correctly STAY `con_putc` (DOS echoes typed input to the screen regardless
   of stdout redirection).

Un-redirected console output (banner/prompt/ANSI/diagnostics) is byte-for-byte unchanged: handle 1 ->
CON slot -> `sft_write`'s CON branch -> `con_putc` -> the SAME ANSI FSM + g_sink path as before.

## Acceptance (graded independently -- Law 2/Law 4)

- NEW oracle `test-redir` (+ `test-redir-mutant`), wired into TEST_UNIT_GATES: 25 checks, 0 failures.
  (a) default JFT -> AH=09h/02h/06h reach the CON sink, file empty; (b) after binding handle 1 to a
  FILE SFT slot (DUP2(file,1)) -> bytes reach the mock `write_at`, NOTHING reaches CON (the `echo >
  file` case, multi-byte, offset advances); (c) no-PSP -> AH=09h/02h/06h still reach CON via fallback.
  Mutation-proven: `-DK36G_MUTATE_NO_REDIR` (always con_putc) makes assertion (b) go RED only.
- Regression: AH=40h unchanged (test-int21 175/0, test-fileio 225/0, test-exec-unit 64/0,
  test-int21-edge 101/0); ANSI/devices/CON-adjacent gates green.
- `make clean && make test` = **242 host (+2: test-redir + test-redir-mutant) + 39 emu** ALL GREEN.
  Bochs boot leg PASS (Rule 5 -- the banner + boot output flow through the rewired path; triple_fault=0,
  banner renders). The whole emu suite (banner, A:\> prompt, ANSI.SYS, SAMIR, every diagnostic) stayed
  green through a rewire of the single most load-bearing output path -- the fallback held.

## Frictions / notes

- The load-bearing risk was the banner / no-PSP case (a dropped banner would be a silent boot
  regression). `sft_from_handle` returns 0 cleanly (not fail-loud) for no-PSP/closed handles, so the
  `e==0 -> con_putc` fallback is clean and the banner is provably preserved (test (c) + the emu banner
  gate).
- Per-byte file writes on a redirected AH=09h string are correct but slow (one `write_at` per byte).
  A buffered `stdout_emit` (one write per AH=09h string) is a sound follow-up optimization -- filed if
  it matters; correctness-first for now.

## Pointers

- `os/milton/int21.c` (sft_write / stdout_emit / the three rewired handlers).
- `os/milton/test_redir.c`, Makefile `test-redir` (+mutant).
- Unblocks `initech-hsct` (the `> >> < |` redirection redo -- now both its prereqs, kernel-window room
  (o0td) and CON-via-handle-1 (this), are met). WL-0045 captured hsct's parser/driver design.

<!-- Tedium certified compliant with NFR-7. Verify revision before use. -->
