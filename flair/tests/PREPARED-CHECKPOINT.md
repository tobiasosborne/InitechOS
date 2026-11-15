Unlanded drafts saved at operator wind-down. None is registered as a current
smoke/test target; do not treat this commit as an approved main change.

- update-window-token.prepared.patch: initech-3zcsf gate + real two-window
  routing test. Old gate297 checks2 failures. SDK contract cd590563 is on main;
  candidate green/mutation/native checks remain. Apply from repository root.
- file-drive-gate.prepared.patch: proposed SDK-owned TBX_FILE_DRIVE0x008C gate
  half for truthful Word C:/A: labels. Await factory owner's SDK publication;
  no SDK file was edited. It uses published platform DOS AH19, no kernel addition.
- copy_capture.c, capture_state.h, test_capture_state.c and
  smoke-filecopy-stable.prepared.sh: initech-gqgrb external QMP observer and
  readiness test. Not wired into Makefile or the active smoke. Build the helper
  separately, supply INITECH_TEXT_SCREEN_ELF and INITECH_TEXT_SCREEN_TOOL to the
  existing harness callback, then read its stable P6 .screen files. The prototype
  passed all9 native cases before readiness was refined to reject no shape:
  final readiness waits only for an idle complete frame, never a desired cursor.
  The frozen copy_pixels.c oracle remains unchanged. Final helper rerun,
  registered idle-guard mutant and real smoke integration remain.
- copy-capture-witness.prepared.patch captures an actual shielded repaint in the
  diagnostic helper only. Its earlier native run reproduced168 missing glass
  pixels,0 restored mismatch. The improved diagnostic print was not rerun.

The prepared smoke still uses the old A-based fixture; coordinate C-image
migration before integrating it. Exact logs, source references, branches and
remaining steps are in the external desktop-owner-2.md handoff. The colour
candidate is on the separate desktop-2-filecopy-colours branch.
