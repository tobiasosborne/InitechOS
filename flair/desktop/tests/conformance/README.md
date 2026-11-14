This directory is a file-reader study on baseline 09d52a32. Product code
and the component's registered tests are unchanged. cases.tsv separates
observed behavior from independent expectations; OBSERVED.txt is evidence,
never an oracle. A pending entry is not a passing conformance check.

Run from the repository root after the ordinary component dependencies have
built (make -j2 test builds SDK host and generated font headers):

    nice -n 15 sh flair/desktop/tests/conformance/run.sh

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

Install as root DESKTOP.DB and boot the product, inspect serial regeneration
notice and Finder responsiveness; move a volume icon, exit/restart FLAIR,
then extract and independently validate the DB and unrelated good files.
The observer calls only desktop_db_validate; bootstrap/read/write and the
Finder record codec are PENDING. D04/D05 are structural stress blobs with
zero-filled records, not claims of valid icon metadata. The header validator
intentionally relies on its bootstrap/write callers to enforce 4096 bytes;
D05 acceptance by that helper alone is not a defect. Finder's semantic
record-count cap is separate (finder_desktop.h FINDER_DB_MAX_RECORDS).
Existing failed DB write loss is already initech-yeznl and is not refiled.
Authority: desktop_db.h's authored layout and the user's conservation property.
