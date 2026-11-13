Real dBASE III PLUS 1.1 abbreviation golden (initech-13d3 / audit L009).

Minted by `make mint-samir-abbrev` using the runnable DBASE.EXE, DBASE.OVL,
DBASEINL.OVL and DBASE.MSG in the sister corpus's `mint/work/`, following
`dbase3-decomp/re/harness-setup.md`. Those binaries are not copied here.
`DMLTEST.PRG` is the exact executed input; `MINT.PRG` and `CONFIG.DB` preserve
the setup. SET PROCEDURE makes this file's procedure visible to real III+;
SAMIR's proc_run already registers the program source for internal DO calls.
The rerun uses a clean `build/samir-abbrev-mint/` workspace, separate from
the promoted goldens read by host and emulator gates, and compares every
input/output against this committed capture.

The sibling `abbrfull/` was independently minted with the full spellings.
GOLD.OUT, BASE.DBF, NAMES.NDX and VALUES.MEM are byte-identical between the
two real runs. `abbrev-goldens.sha256` pins the inputs and outputs; the existing
DML gate also refuses a corpus that differs from its minted DMLTEST.PRG.
No expected output is obtained from SAMIR.

The transcript covers DISP/REPL/APPE/LOCA/SELE/INDE/STOR, clause prefixes,
short DO/USE/SET, REST as RESTORE (versus scope REST), flow terminators,
PROC/PARA/RETU, and distinct STORED/STORENO.
STOREX/STORX/STO/DISPX/SELECTOR/SETX/USEX/DOX all raise 16; BLANX raises 36.
These live results settle invalid long suffixes rather than accepting any
token with the same first four letters. See the sister corpus's
`specs/runtime/interpreter-and-execution-model.md` section 3.

`SAMIR_DML_MUTANT_abbrev` disables only abbreviated DISPLAY recognition in
the query hook. The exact real-output gate must fail for DISP while its
full-spelling control remains green. DBF records and NDX entries retain the
existing independent DML comparisons.
