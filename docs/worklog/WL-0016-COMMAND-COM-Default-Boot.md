# WL-0016 -- COMMAND.COM is the default boot (M2-FINALE integration)

Issue: **initech-k6x** (closed) -- "Make COMMAND.COM the default boot + migrate
demo gates". Branch: `command-com-default` (off the `kernel-hardening` tip, so
it includes the standard-VGA fallback + Bochs gate from WL-0015).

## Context

Until now the real boot image (`TRACER_IMG`) ran a parade of baked demos
(PROGRAM / TYPE / DIR) then halted; the COMMAND.COM shell lived in a separate
`-DBOOT_SHELL` image (`SHELL_IMG`). M2-FINALE: promote the shell to the default
boot so the real DOS drops to `A:\>` right after the banner.

## What changed (1 commit)

- **kmain.c**: the baked PROGRAM/TYPE/DIR demos are gated `#ifndef BOOT_SHELL`,
  so the shell build boots clean to the prompt (banner + proto-DIR, then the
  REPL). The other self-test variants (EXEC/WRITE/MULTIOPEN/DATETIME/...) are
  not `BOOT_SHELL`, so they still run the demos -- their flow is unchanged.
  `run_baked` gains `__attribute__((unused))` (it is legitimately unused in the
  pure shell build, where neither the demos nor the `#ifdef`-ed self-tests
  compile).
- **Makefile**: `TRACER_IMG` -- THE real boot image -- now carries
  `KERNEL_SHELL_BIN` (the COMMAND.COM kernel). A new `DEMO_IMG`
  (`demo_boot.img`) carries the baked-demo kernel (`KERNEL_BIN`);
  `test-program` / `test-type` / `test-dir` were repointed to it.

## Why this shape (lowest-risk path)

The shell already had a complete parallel build (`KERNEL_SHELL_*`), so the
swap is just "which kernel does the boot image carry" + "give the demo gates
their own image" -- `KERNEL_BIN` / `KERNEL_OBJS` / `KERNEL_MAIN_OBJ` are
untouched. The alternative (compile the default `kmain.o` with `-DBOOT_SHELL`
and fold `command.o` into `KERNEL_OBJS`) was more churn for the same result.

## Frictions

- `run_baked` goes unused in the pure shell build (-Werror=unused-function);
  fixed with `__attribute__((unused))` rather than gating its definition (it is
  still used by `KERNEL_BIN` + the self-test variants).
- Risk watched: `test-boot` / `test-tracer-boot` / `test-fs` now boot the
  shell-default image, which BLOCKS on the `A:\>` `AH=0Ah` read and prints the
  prompt. Verified the banner band + seafoam-below-240 + proto-DIR band
  `ppm_text_check` assertions all still pass -- the prompt sits below the
  asserted bands, and the blocking shell times out exactly as the old
  hlt-looping demo kernel did (the gates never gated on the exit code).

## Acceptance

Full suite **55 host + 19 emu GREEN**; `test-boot-bochs` GREEN (the Bochs leg
boots the shell default via the mode-0x13 fallback and reaches the same kernel
markers); `test-shell` still PASS. The real boot now shows: banner -> proto-DIR
(with a data disk) -> `A:\>` prompt.

## Pointers / follow-ups

- `os/milton/kmain.c` (demos `#ifndef BOOT_SHELL`), `Makefile` (`TRACER_IMG`
  -> `KERNEL_SHELL_BIN`; new `DEMO_IMG`; `test-program/type/dir` -> `DEMO_IMG`).
- **initech-h58** (filed): retire the now-redundant `SHELL_IMG` (identical to
  `TRACER_IMG`) + point `test-shell` at `TRACER_IMG`; optionally add a
  screendump gate asserting the `A:\>` prompt renders on the framebuffer.
- Branch `command-com-default` is unmerged (local-only repo). Sibling work this
  session: WL-0015 (Bochs standard-VGA fallback + C Bochs gate) on the
  `kernel-hardening` tip this branched from.
