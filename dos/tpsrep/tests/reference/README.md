# TPS Report Generator reference inputs

Claimed from legacy-final's harness/diff/tps and spec/tps_ritual.json.
The table was written by real dBASE III PLUS 1.1 using tpsmk.prg; only its
non-meaningful header year was normalized to 1989 in the original mint.
The report and console goldens came from the REALDOS build under MS-DOS 6.22,
as recorded in legacy-final harness/emu/fcb_tps.mk. They check the FCB path
against the same program running under DOS; they are not an independent
implementation of its formatter.

The mutation oracles are independent of the current assembly: ritual.json
requires seven alignment presses without stating the count/key in the prompt,
numeric LINES/PAGES validation, a checked failed write and return code 9,
and retaining the old report when backup rotation is refused. Tests supply
invalid bytes, read-only backup attributes and a full disk using mtools.
Unchanged-file checks compare the originally supplied bytes. Report bytes
also remain compared with the historical DOS output for compatibility.

SHA256SUMS pins the actual consumed bytes and the frozen ritual specification.
The remaining TPS manual sources in attic/pubs belong to the manuals owner.
