This directory is a file-reader study on baseline 09d52a32. Product code
and the component's registered tests are unchanged. cases.tsv separates
observed behavior from independent expectations; OBSERVED.txt is evidence,
never an oracle. A pending entry is not a passing conformance check.

Run from the repository root after the ordinary component dependencies have
built (make -j2 test builds SDK host and generated font headers):

    nice -n 15 sh apps/initechword/tests/conformance/run.sh

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

Use Shift-F10 Retrieve CASE.WP5 on a clear document; view top, Home/Home/Down
(end), and Alt-F3 Reveal Codes. For successful retrieval save unchanged,
insert Z at the start, F10 Save to AFTER.WP5, close and reopen. The observer
runs the real Retrieve/Save helpers and insertion; full navigation and code
inventory in real WP remain pending.

Byte authority: the pinned wordperfect51-decomp
specs/file-formats/wp5-header-and-prefix.md and wp5-body-codes.md. W01 is
the captured MINT02; W02-W07 are cuts/corruption of that real prefix/body;
W08-W11 keep its 327-byte prefix with synthetic stress bodies. A changed
cursor byte at file offset 259 during host Save is recorded, not counted
as corruption; the codec documents this cursor-state field. No-op saved-file
parity needs a real session with the same cursor position.

Reference load runner:

    sh apps/initechword/tests/conformance/capture.sh ABS_RUNTIME ABS_FIXTURE ABS_NEW_OUT

Use runtime images from goldens/dataloss/runtime.sh. Capture.steps waits for
the Retrieve prompt and records after 20 seconds (the first retained W05,
W06, W09, W10 captures used 4 seconds; their actual input.steps are retained).
A screen still at a filename prompt does not establish a freeze or refusal.
Extract AFTER.WP5 only after an explicit successful edit/save replay.

W06 and W11 now also have capture-save.sh/capture-save.steps replays and
fresh-runtime reopens. W06-save/AFTER.WP5 is a real 582-byte recovered
document whose independent reader text is exactly ReadableZ. W11-save
contains all 65210 source x characters plus final Z (65211 text characters),
65784 file bytes and zero trailer errors. W06-reopen/W11-reopen use fresh
runtime images on the actual saved files. The load observer's refusal
differs from these real recovery/capacity results.

W12/W13 have no binary yet: obtain genuine WP5.0 and WP6.0 runtimes or a
hash/provenance-pinned sample. In each real editor enter 'VERSION TEST',
Return, F6 'BOLD' F6 and save with F10 in that version's NATIVE default
format (do not choose a WP5.1 compatibility export in WP6). Stop/extract;
rename a COPY to CASE.WP5 without byte changes. Retrieve it in the real
WP5.1 03-09-92 runtime using capture.steps, record conversion questions,
Reveal Codes, edit/save/reopen; then perform the same journey in
InitechWord. Neither later-version bytes nor a runtime were found locally;
header-version mutations would not answer this question.
