# WL-0175 -- TPS backup rotation, round 2 (initech-56ch)

## Context and ground truth

PM round 2 requires rebasing lane/cx-tpsbak onto main's tpsrepfull merge
(`b34fc38a`), retaining both lanes' checks, and registering the backup refusal.
PRD Sec 6.1/8, spec/tps_ritual.json file_rules, IBM DOS 3.30 Technical
Reference pp. 6-70/71, 6-74, 6-79/80, and the real-DOS FCBX T15/T16 goldens
remain the references. Existing report/transcript goldens are unchanged.

## Changes and authorization

The rebase keeps the write/close and numeric checks from initech-ycvk plus
initech-56ch's checked delete/rename and hidden/system report search.
Both gate families remain registered. The first fixture inventory run found
that the six merged initech-ycvk fault images lacked declared contracts;
registering their exact pre-guest contents removes all six diagnostics.
The backup refusal is TPS-012, return code 10: messages 010/011 are occupied,
and the existing return-code table ends at 9. PM explicitly authorized the
additive locked-spec change: E12 cites initech-56ch; all prior rows remain
unchanged. Appendix G, the return-code table and Session 7 document E12.

## Verification

The updated backup gate first failed on the colliding TPS-010 diagnostic:
`TPSBAK: missing TPS-012 / ERRORLEVEL 10 refusal, or false success`.
After renumbering, its six QEMU cases pass; the unchecked-rotation mutant
is caught for both the read-only and hidden backup fixtures.
The report/fault/FCB/journey gate families and all their mutants pass.
test-pubs-tps passes (19 shipped messages documented), and the new manual
content was visually checked on PDF pages 17, 48 and 74. test-makefile-vars,
fixture-volume-recipe-lint, ASCII, reproducibility and diff checks pass.
test-fixture-volumes still fails solely on the unchanged main recipe:
`fixture recipe build/flair_data_itext_full.img does not end with verification`.
Its mutant is blocked by that prerequisite, pending PM's cx-dosmsg merge.
REPORT.md records every gate, the expected RED diagnostic and size deltas.

## Handoff

Work stays committed on the lane branch for the PM; no tracker writes or push.
