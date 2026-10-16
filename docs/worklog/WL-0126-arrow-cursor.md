# M009 arrow strike (initech-tdnl.98)

The lane brief authorizes the deliberate Rule-8 correction of
spec/assets/cursors.h FLAIR_CURSOR_ARROW from v2 to v3. The independent
source is ../system7-decomp/goldens/captures/s8_alert_modal.png,
half-open crop [229,28,245,44), with opacity recovered by subtracting the
identical wallpaper patch in s8_desktop_menubar.png. REPORT.md M009 in
docs/audits/2026-10-15-look-pass6/ names this capture pair and crop.
The resulting strike has 54 black, 43 white and 159 transparent cells.
The hotspot remains (1,1); the black stem angles down and right. The
isolated white pixel at cell (0,13) is retained exactly as captured.

The host suite now decodes the actual independent PNG pair at runtime;
its reference blitter never includes or reads the artifact strike. The
shared factory-only cursor_capture.h implements the capture subtraction
for both host and QEMU graders. A missing or malformed capture fails;
arguments supply paths, with no environment-variable override. The QEMU
grader checks all 256 ink/paper/clear cells at zero tolerance, including
transparent cells against the authored Initech desktop #8DDCDC
(spec/assets/color_canon.json, WL-0053). It no longer checks only the two
tip/outline pixels that could not catch M009.

The existing test-flair-cursor trace is deliberately re-keyed from
m80:40 / hotspot (400,280), over the gray NOTES window, to
m90:70,m90:70 / hotspot (500,380), over clear desktop as in the audit.
That distinguishes white mask pixels from transparent cells. Its capture
still waits on the exact FLAIR-CURSOR marker after the injected movement.
No other gate's coordinates, palette, pixel expectations or tolerance
are re-keyed for this change.

RED preceded the asset correction: build/cursor_lane_host_red.log and
build/cursor_lane_emu_red.log both report the audit's 48/256 differing
cells. GREEN evidence is build/cursor_lane_host_green.log (512 checks,
zero failures; 0/256 differing cells) and build/cursor_lane_emu_green.log
(0/256 differing cells at the injected hotspot). Existing save-under,
clipping, padding, nesting, move/erase and hotspot checks remain intact.

CURSOR_MUT_SQUARED_STEM restores the complete old ink and dilated mask.
The same macro is compiled into the existing host mutant family and the
new test-flair-cursor-mutant QEMU twin. Both must fail specifically at
48/256 differing cells, rather than accepting an arbitrary build/capture
failure as mutation evidence. Logs: build/cursor_mut_squared_stem.log
and build/flair_cursor_mut_squared.log. The new emulator target joins
TEST_EMU_GATES beside test-flair-cursor; the existing host vector names
are unchanged. The pre-existing NO_ERASE and HOTSPOT mutants still fail.
The added build targets are build/test_cursor_mut_squared_stem,
build/cursor_mut_squared_stem.o,
build/kernel_flairtenants_mut_cursor_squared.elf,
build/kernel_flairtenants_mut_cursor_squared.bin and
build/flair_tenants_mut_cursor_squared.img.

The hourglass bytes and cursor runtime behavior are unchanged. The
supplied clone has no busy-cursor selection API in cursor.c/cursor.h;
this lane adds none and does not alter the film-scene lane's other files.
test-canon passes all 67 checks. No executable, menu or resource is added
to the shipped data volume.

The lane brief exempts this cursor-only change from a new clip. No clip
block is added; the existing cursor_cross clip covers desktop/menu/window
movement. No Bochs or full make test vector is run. QEMU gates use
INITECH_QMP_STDIO=1 and run sequentially. Beads, CLAUDE.md, HANDOFF.md and
Git metadata are left alone; the PM owns review and commit. bd show could
not open a database in this clone, so the issue was read from the supplied
.beads/issues.jsonl without changing it.

Final acceptance: all 40 selected desktop/live/scene QEMU regression
targets are GREEN; build/cursor_lane_regressions.summary records each
exact target name and its final four output lines. One initial
test-flair-solid run failed its whole-screen comparison at menu-bar clock
pixel (563,5). The unmodified target passed on follow-up, including exact
PRE/POST equality; no clock region was excluded and no tolerance changed.
The original failure is retained in
build/cursor_lane_test-flair-solid.initial.log. Canon, host shell and its
four mutants, Makefile ordering and fixture-volume checks are GREEN in
build/cursor_lane_host_regressions.log. Source ASCII checks are clean.

make image is GREEN (build/cursor_lane_image.log). The final
test-kernel-headroom measured 180 kernels in the 196608-byte window:
flagship 181736 bytes / 14872 bytes headroom; largest 182856 bytes /
13752 bytes headroom, above the 4096-byte floor
(build/cursor_lane_headroom.log). No data-volume recipe or shipped
application inventory changed.
