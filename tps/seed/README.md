# seed/ — the seed cross-compiler (C)

The from-scratch, single-pass **Pascal compiler written in C**, hosted on
the modern dev machine and targeting freestanding InitechOS/x86. It exists
only to bootstrap: it emits `K1`, the first resident compiler binary, so we
can reach the basin of attraction of the self-host fixed point (PRD §4, §7).

This is **factory** code, so it is C and only C (CLAUDE.md Law 3, PRD §14).
It is *not* the in-universe artifact — bugs in the seed are not bugs in the
OS (CLAUDE.md "Two compilers, never conflated"). The resident compiler that
self-hosts lives in `os/tps/` and is written in Pascal.

Governed by: **PRD §4, §6.7, §7, §14.**
