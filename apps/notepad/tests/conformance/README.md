This directory is a file-reader study on baseline 09d52a32. Product code
and the component's registered tests are unchanged. cases.tsv separates
observed behavior from independent expectations; OBSERVED.txt is evidence,
never an oracle. A pending entry is not a passing conformance check.

Run from the repository root after the ordinary component dependencies have
built (make -j2 test builds SDK host and generated font headers):

    nice -n 15 sh apps/notepad/tests/conformance/run.sh

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

Place N cases at NOTEPAD.DAT or S cases at SCRAPBK.DAT before launch.
For N01 view page 8, select its first character, replace it with Z, turn
back to page 1 and close/relaunch. For S01 view clip 32, Clear it, Paste the
single TEXT byte Z, close/relaunch. For corrupt cases, try an edit/paste
and close, dismissing the kept-file notice. Compare original bytes exactly.
The host observer exercises edit/stash/save; the relaunch/turn UI steps
and all saved-store content checks beyond damaged-source retention remain
owner follow-up. Expected invariants and construction come from the authored
include/notepad/store.json, independently implemented in generate.c.
There is no period-original on-disk format claim.
