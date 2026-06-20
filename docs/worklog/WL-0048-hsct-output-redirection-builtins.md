# WL-0048 -- hsct OUTPUT redirection (`>` / `>>`) for builtins

bead: initech-hsct (Shell: I/O redirection) -- OUTPUT increment

## Context

WL-0045 built the redirect parser+driver but REVERTED it: the shell's CON
output bypassed the JFT (AH=09h -> con_putc direct), so `echo HELLO > file`
would not redirect, and the kernel window (PROGRAM_BASE 0x30000) was
exhausted. Two prerequisites have since landed:

- **k36g**: AH=09h/02h/06h CON output now routes through the redirectable
  stdout handle 1 (`stdout_emit`), so a DUP2(file,1) catches builtin output.
- **o0td**: PROGRAM_BASE raised to 0x38000, restoring kernel-window room.

This increment rebuilds the OUTPUT path cleanly. `<` and `|` are DEFERRED.

## What changed

**os/milton/command.c + command.h**
- `cmd_redir_parse()` -- PURE, host-testable parser. Scans for `>` / `>>`
  (last wins), extracts the next whitespace-delimited token as the target,
  rebuilds a tidied `clean` command, sets `append`. Every write bound-clamped
  (Rule 2). Mutation hooks `CMD_MUTATE_REDIR_NO_APPEND` / `_KEEP_TARGET`.
- New inline-asm INT 21h helpers (same style as dos_open/close):
  `dos_open_mode` (3Dh w/ AL), `dos_dup` (45h), `dos_dup2` (46h),
  `dos_lseek_end` (42h AL=2).
- `run_with_redirect()` -- driver wrapping `dispatch_line`: parse -> open
  target (`>` = 3Ch creat/trunc; `>>` = 3Dh-RDWR open + 42h LSEEK-to-end,
  else 3Ch creat) -> `dos_dup(1)` save -> `dos_dup2(file,1)` -> dispatch the
  clean line -> restore `dup2(saved,1)` + close on EVERY path. Fail-loud on
  open/dup failure (prints a DOS diag, does NOT run unredirected). Mutation
  hook `CMD_MUTATE_REDIR_BYPASS`.
- Wired into `command_repl` (interactive), `run_batch` BL_COMMAND / IF-true /
  FOR-body, and **BL_ECHO_TEXT** (batch_classify peels `ECHO` into an inline
  handler, so `ECHO x > f` is routed through the driver only when the expanded
  text carries a `>`; no-redirect ECHO keeps the unchanged fast path).

**Oracles (Makefile)**
- HOST `test-redir-parse` (65 checks) + `test-redir-parse-mutant` (both mutants
  RED). Added to TEST_UNIT_GATES.
- EMU `test-hsct-redir` (+`-mutant`): AUTOEXEC.BAT does
  `ECHO RZZHELLO> OUT.TXT ; TYPE OUT.TXT ; ECHO RZZWORLD>> OUT.TXT ;
  TYPE OUT.TXT`. Asserts RZZHELLO count==2 (proves `>` wrote the FILE, not the
  screen) + RZZWORLD count==1 (proves `>>` appended) + clean EXIT (handle 1
  restored). REDIR-BYPASS mutant kernel goes RED. Added to TEST_EMU_GATES.
  Fixture: `harness/diff/fat_diff/fixtures/autoexec_hsct.bat`.

## Why

Authentic DOS strips `> file` from the command line and DUP2's the file onto
stdout around the command (MS-DOS 3.3 Tech Ref Ch.6). The `RZZHELLO`-count==2
discriminator is the load-bearing assertion: if `>` leaked to the screen (or
never wrote the file) the count would be 1, not 2.

## Frictions / the surfaced gap

External `.COM` EXEC output does **NOT** redirect. Root cause (empirically
confirmed: `GREET.COM> GOUT.TXT` prints GREETINGS to SCREEN, GOUT.TXT empty):
`psp_build` (psp.c) HARD-RESETS the child JFT slot 1 to CON (`jft[1]=0x01`)
on every EXEC and carries no inheritance param. The child runs under its own
PSP/JFT, so the parent's DUP2 is invisible to it. The `run_with_redirect`
driver already brackets the EXEC dispatch correctly -- when the loader learns
to inherit the parent JFT, external EXEC redirect will work with ZERO driver
change. Filed **initech-bsy.9** (loader child-PSP-inherits-parent-JFT); hsct
now DEPENDS ON it. Builtins (the canonical `echo > file`) are fully delivered.

## Acceptance

- HOST: test-redir-parse 65/0; both parse mutants RED.
- EMU: test-hsct-redir PASS (`>` + `>>` proven on the 386); REDIR-BYPASS
  mutant RED.
- No regression: test-command (166), test-batch-exec (63), test-autoexec,
  test-shell, test-type, test-redir (k36g), test-redir-mutant all green.
- Kernel headroom: _kernel_end=0x30920 < limit 0x37f00 (~30 KiB); BATCH_FILE_MAX
  stayed 4096 (no shrink needed -- o0td's room held).
- All touched files ASCII-clean (Rule 12).

## Pointers

- Driver + parser + helpers: os/milton/command.c (cmd_redir_parse,
  run_with_redirect, dos_open_mode/dos_dup/dos_dup2/dos_lseek_end).
- Parser decl: os/milton/command.h.
- Host oracle: os/milton/test_redir_parse.c; gate `test-redir-parse`.
- Emu gate: Makefile `test-hsct-redir` (+mutant kernel
  kernel_shell_mut_redirbypass); fixture autoexec_hsct.bat.
- Follow-ups: initech-bsy.7 (`<` stdin), initech-bsy.8 (`|` pipe),
  initech-bsy.9 (loader JFT inheritance -> external EXEC redirect).
