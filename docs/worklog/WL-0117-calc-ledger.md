# R4.6 Calc tenant slice (initech-tdnl.20; bf2 deferred)

The corrected lane brief authorizes PRD Appendix A/B as content ground truth,
with unknown cells kept as TODO_FRAME. Neither requested bead exists in this
clone's local database (`bd show` returned no matching issue). Beads, CLAUDE.md,
HANDOFF.md and Git metadata were left alone; the PM owns review and commit.

CALC.EXE is a 17,804-byte disk application using os/apps/tbx and the existing
i123 sparse sheet, editing, parsing and numeric formatting core. It opens a
ledger and a separate chart through the existing Window Manager. TEXTDRAW now
accepts a Geneva9 selection bit; NEWWINDOW supports two stable creation-order
tokens within the existing single process slot and 16 KiB records budget.
Process ownership, grouping and teardown remain the existing process.c paths.

The deliberate locked-data addition is spec/assets/calc_ledger.json, generated
by the C factory tool tools/calc_ledger_gen.c into calc_ledger.h. The known
labels, footer total, trailing-minus example and five pie weights are PRD
transcriptions. The footer is a stated fixture, never a sum of invented row
amounts. Row/column counts, missing rows, amounts, pie names and the unlocated
negative cell address stay TODO_FRAME. The negative specimen is displayed
separately. Two-column layout, placement and the three-shade chart are stated
product policy, not recovered film geometry. Adjacent chart sectors have
different shades, including the seam. Geneva uses the existing clean-room v0
strike; the video oracle checks the independent seven-row NFNT label height,
not an invented claim of full glyph/advance equality.

The host routing oracle was written before the application (initial RED in
build/calc_host_red.log). The injected trace and video gate were added before
implementation (initial RED in build/calc_emu_red.log). A further independent
contour check caught merged same-shade sectors (build/calc_pie_five_red.log).
The repaired chart has five visible sectors; its named restoration mutant is
rejected on that same check.

Independent ground truth: InitechOS-PRD.md 6.5 and Appendix A/B;
../system7-decomp/goldens/captures/s8_doc_window_active.png (read directly by
the checker for title profile and widget), s8_doc_window_inactive.png via
specs/sys8/window-chrome.md section6 (inactive outline), and
goldens/resources/NFNT_5415.bin (Film Costs ink rows3..9). No film still was
searched for, acquired, rendered or graded.

Acceptance evidence: build/calc_host_final.log has test-calc-spec,
test-calc-tenant (415 checks, zero failures, including all four locked
Paint/copy/configuration strings) and test-calc-tenant-mutant GREEN.
build/calc_final.log has test-flair-calc, test-flair-calc-mutant and
test-flair-fonts GREEN. All five new mutants are
RED for their named property: CALC_MUT_PIE100, CALC_MUT_LEADING_MINUS,
TBX_MUT_TOKEN_ALIAS, TBX_MUT_GENEVA_IGNORED and CALC_MUT_MERGE_SLICES.
build/calc_host_regressions.log retains test-canon (67 checks), test-tbx-menu
(23 checks), Makefile ordering and volume-contract validation. The latest
factory checks are in build/calc_factory_final.log (17 declared data recipes).

Appending CALC.EXE after 123.MNU preserves previous application icon cells.
It increases locked purge from5 to6 and Empty Trash's clip marker from7 to8.
The damaged-volume stranded TRASH moves from cell5 to6; its locked trace is
re-keyed by 68px, preserving subsequent absolute waypoints. Required volume
regressions are GREEN on the final volume: test-flair-locked-purge, Finder
service, app launch and its six existing mutants in
build/calc_volume_final.log. All QEMU runs used
INITECH_QMP_STDIO=1. No Bochs or full make test vector was run.

make image is GREEN. build/calc_image_headroom.log measures164 kernels:
flagship175784 bytes, headroom20824; binding largest176904, headroom19704,
above the4096-byte floor. The image/data alias was refreshed again by the
final Calc-family run.

Rule14: calc_ledger's RECORD_SCRIPTS / RECORD_SPEC / RECORD_MARKER /
RECORD_SETTLE / RECORD_IMAGE block is before record-flair. The PM records and
reproves the clip on main after merge; this lane did not record or close beads.

FILE COPY and InitechPaint remain unimplemented in this slice. File > Save is
visibly disabled, so it cannot claim a fake copy. No runtime configuration,
progress-rate mutant, FAT-copy proof, file_copy clip or paint_bar clip is
claimed. Remaining film cells still require operator-supplied values.
