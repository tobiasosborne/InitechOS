# WL-0116: EDLIN command editor and independent gates

Context: initech-udr1, EDLIN half only. The PM owns beads and commits.
DEBUG is outside this lane. Sources and per-command page citations are in
harness/edlin/README.md and the model and artifact source comments.

Changed: shipped flat C EDLIN.COM with streaming A/W, DOS template keys,
line commands, search/replace confirmation, save/backup and quit semantics.
The independent C command model supplies transcripts; complete file results
from printed manual pp.178 and 184 are also checked literally. Four named
artifact mutants run in both the host and emulator gate vectors.

Rule-8 deliberate fixture re-key: adding APPS/EDLIN.COM after CHKDSK.COM
moves the damaged-volume fixture's stranded APPS/TRASH to cell 7. The first
Finder-service run correctly selected EDLIN.COM at the old cell 6. The trace
now clicks one cell right and adjusts the park displacement to preserve its
absolute destination. The existing assertion still requires opening TRASH;
no oracle was relaxed. Locked-purge adds EDLIN.COM to writable-leaf absence
checks and changes the exact count from purged=6 to purged=7, refused=2.
The seeded-volume contract includes EDLIN.COM on every recipe variant.

Acceptance: the EDLIN host and QEMU families, four host/emulator mutants,
make image, app-launch, locked-purge, Finder-service, keyboard unit oracle,
fixture-volume contract and Makefile variable checks. QEMU exports
INITECH_QMP_STDIO=1. Separate clean assembly/C/link output is byte-identical.
No aggregate make test, Bochs, git mutation, or beads mutation was requested.
