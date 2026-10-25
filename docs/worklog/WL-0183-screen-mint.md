# WL-0183 -- W4.0 real III+ full-screen mint harness

Context: LANE-BRIEF.md / initech-p223c, PRD 6.6 and INITECHBASE-ship-plan W4.0.
The operator explicitly prohibited commits, pushes, Bochs, full make test and
LaTeX typesetting. All changes remain uncommitted.

Changed: added a C screen codec/comparator and key-script compiler, thin-shell
mint/repro/self-test drivers, a guest capture TSR, and an opt-in exact text
capture beside QEMU PPM screenshots. Five seed cases contain six real dBASE
III PLUS 1.1 dumps, with case/oracle/screen SHA256 manifests. The existing
mint_clients_dbf fixture supplies three input records; only real III+ paints
the golden screens. No artifact sources or legacy mint scripts were changed.

Why: the old SCRDUMP helper captures after exit without cursor position;
mint_run.py's trimmed preview loses attributes. The new helper records active
pages before UI cleanup, and writes them after QUIT. The locked format keeps
every CP437 byte, CGA attribute and cursor coordinate. Existing Python mint
scripts should be ported when that transcript suite is maintained; this path
uses C, make, thin shell and the small real-mode assembly capture shim.

Frictions: dBASE replaces INT 1Ch at startup, requiring installation through
RUN SCREEN after initialization. ASSIST displays time; DOS AH=2Ch is frozen
at noon instead of masking captured cells. The local beads DB lacked
issue_prefix and the brief's issue; setting the existing prefix to initech
allowed recreating and closing initech-p223c without DB reinitialization.

Acceptance: test-screen-mint, test-screen-mint-repro and test-screen-mint-samir
all passed. Six dumps match two independent fresh real-program runs byte for
byte; stored screens and all three manifests match both runs. Same-file
comparison is GREEN; character, attribute and cursor mutations are RED with
precise first differences. All 256 glyph/attribute byte values round-trip;
malformed input and invalid script dumps are rejected. ASan/UBSan codec check
passed with leak scanning disabled because the sandbox prevents LeakSanitizer.
Existing QEMU progress and key-ack host gates passed; source/shell checks passed.

SAMIR capture itself is GREEN. Current SAMIR-vs-real parity is separately RED:
the existing DISPLAY STRUCTURE command reports `12  Variable not found.`;
the first screen delta is row=0 col=0 character 0x20 vs 0x49 (boot banner).
This harness result does not claim wave-3/4 UI implementation parity.

Pointers: docs/screen-mint.md; the exact 20-line case contract at
harness/diff/dbf_diff/xbase_prog_diff/screen-mint/README.md; verbatim two-run
hashes/verdicts in that directory's REPRODUCIBILITY.txt; retained run outputs
under build/screen-mint/repro.o4shRe and live SAMIR data under
build/screen-mint/samir. No remaining W4.0 harness blocker.

## PM FIX-THEN-MERGE revision

The independent review correctly identified five blocking gaps in that first
acceptance result. All five are now fixed and have focused regression checks.
SETUP rejects reserved oracle/capture/driver names before copying, and captures
are removed before launch. A fresh CAPTURE.OK with the complete dump mask is
required; the TSR writes it only after successful capture writes and closes.
The oracle verifier recompiles events and hashes current independent binaries,
helpers and producer scripts, comparing them to stored ORACLE.sha256 in both
gates. Those five manifests were deliberately refreshed for the new helper and
producer; none of the six screen goldens or their screen/input hashes changed.

Staged files have fixed 1986-07-24 noon timestamps; guest files are separated
from host logs/configuration. MOUNT geometry is 512/32/2048/1024 with fixed
free-space reporting (32MiB total, 16MiB free), checked through guest AH=36h.
RUN DIR CLIENTS.DBF and native DIR LIKE *.PRG each reproduce byte for byte even
after input host mtimes and host disk usage change. They report 07/24/1986
12:00p and exactly 16,777,216 bytes free, with no capture normalization.

Optional QEMU helpers inherit the absolute harness deadline; polling expires
into process-group SIGKILL plus reap, removes partial output and fails the
session. Capture failure immediately stops record/input and marker paths.
SAMIR integration requires harness exit 0, OK=1, both completed output files
and the helper's completion record before printing integration GREEN.

All six original screens matched two clean fresh runs, exit 0, under
build/screen-mint/repro.4O9lXj. All three manifests also matched the stored
versions. The final review checks, including altered pins through both full
gates, failed saves, altered binaries/events/helpers, incomplete capture,
failure after save and stalled single/record helpers passed under
build/screen-mint/review-tests.LlhWew. The earlier final read error caused by
editing a running shell script was discarded and the frozen script rerun.

QEMU boot, progress, key-ack, 34-EXEC completion and bare-key gates passed,
including their named mutants. Codec/SETUP ASan/UBSan checks, shell syntax,
ASCII checks and git diff --check passed. See the stored REPRODUCIBILITY.txt
and REVIEW-REGRESSIONS.txt for verbatim verdicts. No commits, pushes, Bochs,
full make test or typesetting were performed. No remaining review blocker.
