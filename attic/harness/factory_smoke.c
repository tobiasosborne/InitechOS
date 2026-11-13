/*
 * factory_smoke.c -- InitechOS factory toolchain smoke stub
 *
 * beads: initech-tse  ("Repo skeleton + Makefile + directory layout")
 * Ref:   CLAUDE.md "Build & test" (the `make factory` target),
 *        CLAUDE.md Law 3 / PRD Sec 14 (the factory is C, and only C).
 *
 * This is a deliberate PLACEHOLDER. It does nothing but prove that the C
 * factory toolchain is wired up: the Makefile can compile a C11 source
 * with -Wall -Wextra -Werror and produce a runnable binary in build/.
 * `make factory` building and running this is the acceptance criterion
 * for the repo-skeleton issue.
 *
 * It will be replaced by the real factory: the seed cross-compiler
 * (seed/) and the oracle/emulator harness (harness/emu, harness/diff,
 * harness/proptest, harness/ssim.c) described in CLAUDE.md's File map.
 *
 * ASCII-clean (CLAUDE.md Rule 12). No timestamps or other nondeterminism
 * (CLAUDE.md Rule 11 -- reproducible builds).
 */

#include <stdio.h>

int main(void)
{
    /* One-line banner to stdout, then a clean exit. That's the whole job. */
    puts("InitechOS factory: skeleton OK");
    return 0;
}
