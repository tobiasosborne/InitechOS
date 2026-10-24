# SAMIR inspection (initech-f99h)

The new `status` and `memory` programs were run by real dBASE III PLUS 1.1
under DOSBox-X. `xbase_prog_diff/mint/golden/{status,memory}` contains the
unmodified alternate files, programs, inputs and outputs. The binary hashes
are in `reference.sha256`; the binaries remain in the sister reference corpus.
`inspection-goldens.sha256` pins the files promoted to `build/samirdml`.

Ground truth: Using III+ U5-108/109/146/147; Programming P15-11/12;
`dbase3-decomp/specs/commands/navigation-query-display.md` STATUS/MEMORY.
The data drive is A: in both environments (the existing locked
`spec/drive_letters.json` emulator mapping). No filename or whitespace
normalization is applied. The audit's EDGE fixture covers memo filenames.

STATUS covers zero, one and three areas, two simultaneously open indexes,
filters, a configured relation, SET changes, and cursor/flag preservation.
MEMORY covers zero, five and twenty variables of each memory-variable type
(C/N/D/L), empty strings/dates, PUBLIC's initial .F., hidden PUBLIC values,
PRIVATE values and restoration. Unassigned PRIVATE declarations have no row.
Hidden values count toward both totals; C uses length+2, N/D 9 and L 2 bytes.

Both LIST and DISPLAY are in each transcript. Real DISPLAY MEMORY pauses
every 17 variables. DISPLAY STATUS pauses after two areas and before the SET
grid. The prompts are part of alternate output and are graded byte-for-byte.
DOSBox-X AUTOTYPE supplies paging keys; a closing DOS EOF is mandatory.
`make mint-samir-inspection DBASE3_DECOMP=/absolute/reference/path` remints in
an isolated directory and compares against the checked-in real output.

The host and locked QEMU gates are `test-samir-dml-{status,memory}` and their
`-emu` legs. Each has a `-mutant` and `-emu-mutant`: dropping area 2 or
reporting the wrong variable scope must lose the exact differential.

The existing REPORT and core host gates share scratch files with their
mutants. Run these gates serially: concurrent runs interfered, while unchanged
serial reruns passed. The real mint also reproduced its pinned output when
rerun alone after a loaded run exhausted its scheduled paging keys.

The round-2 size refactor recovers 2,712 bytes: SAMIR.COM is 97,673 bytes,
502 below the unchanged reclaim cap. All SAMIR host roots and QEMU gate
targets pass; the saved pre-round and final binaries both match the 20 DML
and 13 function guest goldens. Bochs remains unverified because this sandbox
denies its RFB socket before guest execution (also on an isolated retry).
See REPORT.md and samir-inspection-size-round2.md. No locked spec, budget,
or oracle tolerance changed. Relation navigation remains the pre-existing
gated behavior; inspection reports its stored intent.
