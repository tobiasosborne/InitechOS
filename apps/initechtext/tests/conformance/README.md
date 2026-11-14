This directory retains the file-reader study on baseline 09d52a32, imported
from 4fb96275. cases.tsv separates
observed behavior from independent expectations; OBSERVED.txt is evidence,
never an oracle. A pending entry is not a passing conformance check.

Run from the repository root after the ordinary component dependencies have
built (make -j2 test builds SDK host and generated font headers):

    nice -n 15 sh apps/initechtext/tests/conformance/run.sh

Fixtures are committed bytes pinned by SHA256SUMS. The run verifies that
manifest before compiling the observer. Observers include the real app face
inside this component, or explicitly say when only a decoder/validator is
exercised. SDK host stores files but does not implement FILE COPY or boot
preference loading. Nothing here proves a booted desktop cannot freeze.
Each host process is limited to 30 seconds.

Rebuild inputs for inspection only: compile generate.c with a host C compiler,
run the binary in a NEW scratch directory, then compare that directory with
fixtures/ and SHA256SUMS. Do not regenerate expected observations from product
output. The generator takes one argument for Initech 123 (the pinned real
Lotus goldens/minted/MINT01.WK1), InitechWord (real WP MINT02.WP5), or
InitechCalc (data/LEDGER.TXT); the other generators need no argument.
All generators are test-side C and use no product codec.

User steps: put the case at the application's normal file name on a copy of
the data disk. Open/retrieve (or launch the accessory), inspect first/last
readable data and any warning, make the edit specified below, Save, close and
reopen. Keep the pre-run disk and extract saved bytes after the emulator has
stopped. For an error, dismiss it and verify the prior document and original
file still exist before doing any edit. Never script typing blindly into an
error prompt.

Use File Open CASE.TXT, inspect all line breaks and the final text, then Save.
T04 additionally caps every successful SDK read at four bytes; this models a
legal positive short read, not a truncated fixture. The normal-read control
must preserve all twenty bytes; silently granting Save permission to only the
first four violates the study's conservation property. On the baseline,
run.sh exits 1 for this P0; the repaired loader passes this property.
The registered test_itext_load now checks these pinned fixtures against an
independent stdio reader over disk-backed file bindings, including failures
after a prefix, early EOF, seek/close errors, capacity and prior-document
preservation. make test verifies SHA256SUMS before running the regressions.
The study observer itself remains diagnostic, outside make test.

For T02 try one more character at capacity; for T03/T05 confirm the refusal
does not replace the already-open document. For T01 the stated TEXT contract
expects exactly alpha CR bravo CR CR charlie CR (21 bytes). Source:
component README and the user's safety property; no claim of native
SimpleText damaged-file parity. The observer records normalized lengths
and save results; a separate native-Mac capture is not the oracle here.
