# LANE-BRIEF: W2 mint-first -- SET options and batch verbs (bead initech-emqcq)

Read CLAUDE.md in full first. Read docs/plans/INITECHBASE-ship-plan.md
(waves 2a/2b/2c). Do NOT git commit or push. No Bochs, no full `make test`,
no LaTeX. Do NOT implement any SAMIR feature -- this lane only mints goldens
and classifies.

The proven shape is wave-1 Lane 0: harness/diff/dbf_diff/fns_mint.sh mints
real dBASE III PLUS 1.1 goldens (fns-mint/<case>/{FNSTEST.PRG,GOLD.OUT,
sha256.txt}) under dosbox-x from ../dbase3-decomp/mint; read it, its Makefile
targets (grep fns-mint / fns_mint in Makefile) and two existing cases first.

1. Inventory: the III+ SET options not shipped by SAMIR are (from HELP_topics
   vs the shipped inventory): BELL CARRY CATALOG CONFIRM DEBUG DEFAULT
   DELIMITERS DOHISTORY ECHO FIELDS FIXED FORMAT FUNCTION HEADINGS(check
   vs shipped HEADING) HELP HISTORY INTENSITY MARGIN MEMOWIDTH MENUS MESSAGE
   PATH PRINT PROCEDURE SCOREBOARD STEP TITLE TYPEAHEAD UNIQUE VIEW; batch
   verbs not shipped: AVERAGE COUNT SUM JOIN UPDATE INSERT EJECT TEXT/ENDTEXT
   DIR SUSPEND CALL LOAD IMPORT EXPORT. Verify each against
   ../dbase3-decomp/specs/commands/*.md and HELP_topics.txt, and against the
   shipped list (build the inventory: `make pubs-samir-inventory`, file
   build/pubs/correctness/samir.shipped).
2. Classify each into: PURE (observable through ? / LIST / DISPLAY output in a
   batch program -- mintable now), UI (only observable on full-screen
   commands, wave 2c/4), OMISSION-CANDIDATE (CALL/LOAD binary modules,
   IMPORT/EXPORT PFS files, DEBUG/ECHO/STEP debugger, ...) with one line of
   reasoning and a spec citation each. Write it as a TSV:
   harness/diff/dbf_diff/xbase_prog_diff/w2-inventory.tsv
   (name, kind set|verb, class, spec ref, notes).
3. For every PURE item mint a golden with the fns_mint.sh machinery (or a
   sibling script if SET/verb programs need a different harness -- reuse, do
   not fork, and keep 'MINT-ERRORS:' declarations), each program exercising
   defaults, a set/changed value, the observable effect, and the error paths
   (bad operand, missing TO, etc.) as the spec describes, under
   harness/diff/dbf_diff/xbase_prog_diff/set-mint/<name>/ and
   .../verb-mint/<name>/, with sha256.txt; register each as a PENDING gate in
   the same way fns cases are (pending gates are run by name, never in the
   default vector).
Make your final message the full report: the TSV summary counts per class,
the list of minted cases with their first GOLD.OUT lines, any item where real
III+ surprised you (like LASTKEY not being a III+ function), files touched.
Do not commit.
