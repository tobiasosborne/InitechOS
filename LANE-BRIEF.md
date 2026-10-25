# LANE-BRIEF: W4.0 full-screen mint harness (bead initech-p223c)

Read CLAUDE.md in full first (Laws 1-4; factory is C + make + thin shell --
NO new Python in harness code you add; if existing mint scripts are Python,
read them, and say in the report whether they should be ported). Read
docs/plans/INITECHBASE-ship-plan.md (W4.0). Do NOT git commit or push. No
Bochs, no full `make test`, no LaTeX typesetting.

Goal: a reusable, deterministic minter of REAL dBASE III PLUS 1.1 full-screen
goldens (80x25 text cells + attributes + cursor position) driven by an
injected keystroke script, so that waves 3-4 (GET/READ forms, BROWSE, EDIT,
APPEND, CREATE/MODIFY STRUCTURE, MODIFY COMMAND, REPORT/LABEL designers,
ASSIST, HELP) can be graded against the real program.

Start from what exists: harness/diff/dbf_diff/xbase_prog_diff/samirctl-mint/
(scrdump.asm screen-dump TSR, mint_run.py, mint_setup.py, the .PRG/.TXT
pairs), harness/diff/dbf_diff/fns_mint.sh + fns-mint/ (the function-golden
minter: how it boots real III+ under dosbox-x from ../dbase3-decomp/mint and
captures output), and ../dbase3-decomp/mint/*.conf. Deliver:
1. A minter (make target + script) that takes a case dir {SETUP files, KEYS
   script with explicit waits, list of DUMP points} and produces, per dump
   point, a golden screen file in a locked, documented text format
   (25 lines of 80 chars + an attribute plane + cursor row/col), plus
   sha256 records, reproducibly (two runs byte-identical; prove it).
2. Five seed cases minted from real III+: (a) the dot prompt after
   `USE CLIENTS` + `DISPLAY STRUCTURE`, (b) `@ 5,10 SAY "X" GET` + READ
   with a typed value, (c) BROWSE on a 3-record file after one Down and one
   field edit, (d) EDIT record 2, (e) ASSIST main menu with one menu opened.
   Store under harness/diff/dbf_diff/xbase_prog_diff/screen-mint/<case>/.
3. A comparator in C: SAMIR-side screen capture (the emu harness can dump
   the text screen -- find how test-pubs-screens / the SAMIR emu legs capture
   screens) vs a golden, with a precise first-difference report; a self-test
   gate (golden vs itself GREEN; golden vs a one-cell-perturbed copy RED).
4. Document the case format and how a lane adds a case (docs, 20 lines).
Make your final message the full report: what exists vs what you built,
the format, reproducibility proof (hashes of two runs), the five goldens'
first lines, the comparator self-test verdicts verbatim, files touched,
and blockers. Do not commit.
