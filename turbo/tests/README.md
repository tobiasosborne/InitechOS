# Turbo factory assertion controls

This host-only component checks the seed/compiler adapters used by the
shared mutation runner. Dependencies are the shared factory test protocol,
the seed host executable, FPC, NASM, binutils and mtools. It ships nothing.

`make test` exercises explicit PASS/SEMANTIC/ERROR expectations: empty or
unfinished runs, inconsistent summaries, real assertion failure, NASM/FPC
exit 1, timeout/startup failures and absent serial output. The guest controls
run the real factory producer with shell harness stand-ins in a private
scratch root; no emulator is started. `make mutants` restores the catch-all
exit-1 fabrication and requires a semantic failure of these controls.

Before the repair, a comment-only seed patch survived normally but was
falsely killed when NASM failed twice (initech-kkqbw). After the repair the
same setup injection is ERROR. Assertions are emitted where independent
bytes, captured protocols, literal values or bounded properties are graded.
Generated labels are checked for NASM scope uniqueness before invoking the
assembler; an assembler failure itself is an error, never a kill. The outer
adapter requires a completed positive summary consistent with the exit code.

The inherited `factory/seed-guest.sh` remains unmodified: the seed wrapper
checks a fresh successful harness, serial markers and independent stdout /
file bytes itself. Its missing-serial pipeline and raw tool exits therefore
cannot manufacture assertion evidence. All tests stay within Turbo's tree.
