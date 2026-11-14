This directory is a file-reader study on baseline 09d52a32. Product code
and the component's registered tests are unchanged. cases.tsv separates
observed behavior from independent expectations; OBSERVED.txt is evidence,
never an oracle. A pending entry is not a passing conformance check.

Run from the repository root after the ordinary component dependencies have
built (make -j2 test builds SDK host and generated font headers):

    nice -n 15 sh apps/initech123/tests/conformance/run.sh

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

Use /File Retrieve CASE.WK1; inspect A1 and the table-specific boundary;
for L07 use F5 IV2048, L08 F5 IV8, L09 F5 A9, L10 F2 then Return
without changing text, L11 /Range Name Table at A10, L12 F9. For successful
loads, save unchanged, then edit A1 to 73 and save to AFTER.WK1. The observer
runs the real Retrieve/Save helpers; it does not execute every boundary-view,
Name Table or F9 step. Those remain capture work.

Authority for byte construction: the pinned lotus123-decomp
specs/file-formats/wk1-format.md sections 1, 4 and 7, and its independent
RPN reader. Large labels/names/formulas are synthetic stress derivatives,
not files claimed to have been saved by Lotus. L01 is real MINT01 unchanged;
L02-L06 are cuts/trailing-byte edits of it. L07 is the requested IV2048
coordinate boundary, not proof that R2.2 has only 2048 rows: this component
documents 8192 rows and a separate 2048-populated-cell capacity.

Reference loads: first run goldens/dataloss/runtime.sh ABS_RUNTIME. Then:

    sh apps/initech123/tests/conformance/capture.sh ABS_RUNTIME ABS_FIXTURE ABS_NEW_OUT

The runner installs CASE.WK1 in the disposable runtime's C:\\LOTUS and
uses capture.steps. It verifies the input floppy is unchanged. Captures
here are load observations, not complete edit/save parity. Pinned raw VGA,
text, lossless PNG, monitor log, steps and post-load data disk are in
captures/. See review-app-readers.md for findings and remaining recipes.

The report also retains full default and component test logs under
validation/. The default full gate was RED; seven reviewed component suites
passed with the stated command-line runtime limits. No full-gate pass is
implied by this study.

L13 (no binary yet): obtain actual 1-2-3 Release 3 runtime and record its
hash/version. Enter A1=123 and B2='LATER in a fresh sheet, File Save in native
WK3 format to LATER.WK3; stop, extract, hash, and copy to CASE.WK1 WITHOUT
changing bytes. Run this same R2.2 load recipe, record the conversion/refusal
question, dismiss safely, then run Initech 123. No local WK3 or R3 runtime
was found. A fabricated BOF version is not a substitute.

Owner follow-up, initech-pmw9: `captures/L10-f2` loads this exact 240-character
fixture in real R2.2, presses F2 then Return without typing, and saves AFTER.
Its LABEL body is 246 bytes: five header bytes, apostrophe, 239 text bytes,
and NUL. R2.2 itself shortens this edit. The earlier issue's closed disposition
was correct; the study's host observation alone did not establish a mismatch.

The owner imported only this component's study material from 4fb96275 and
3ccb4a71. `test_i123_conformance` now registers the captured L05 missing-EOF
diagnostic and L10 no-op edit readback in tier 0. Historical OBSERVED and
validation logs remain observations of the study baseline, not current results.
