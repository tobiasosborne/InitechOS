# WL-0162 - Finder limits, alerts and input; the native pressed push button (finder-alerts-pressed)

Part 1 (below, to the STOP paragraph) is the previous Codex lane's handoff,
committed verbatim in 0f33a169. Part 2 is the Opus takeover with mint authority.


Scope: initech-tdnl.61/.63/.84/.62/.66/.72/.57/.64/.65/.88/.79.
References: PRD 3, 6.2-6.5 and Appendix A/B; the issue descriptions;
system7-decomp/specs/toolbox/control-manager.md (TrackControl release inside),
specs/toolbox/dialog-manager.md, and specs/toolbox/event-manager.md.

Finder window records now use the existing records arena's available storage
(up to the 32-bit slot-set representation). Directory icon buffers use the
existing data arena and grow before enumeration or a move commits. Allocation
failure leaves the previous view or tears down a failed open and shows an alert.
Open applies to every selected folder. Keyboard selection follows icon geometry
and a case-insensitive name prefix; it scrolls the selected cell into view.

The process dispatcher consumes an inactive document's activating content click.
Dialog Manager keeps the original pressed control through motion and mouseUp;
release outside cancels, reentry followed by release inside fires once.

Failure notices reuse the Dialog Manager alert layout. Their text is the
bead/operator wording, not a claimed transcription of native Finder strings.
The independent pixel authority is goldens/captures/s8_alert_modal.png in the
read-only system7-decomp corpus. HELLO/NOTES use disabled menu item flags;
disabled ink is read independently from s8_menu_dropped.png.

Rule 8 format extension, authorized by tdnl.57: IDB1 version/header/24-byte
record layout stay unchanged. A renamed kind=5 origin may be followed immediately
by kind=6, with the same origin directory and the original 8.3 name in bytes
4..16. Coordinates/flags are zero in kind=6. The maximum record count reserves
one companion per origin. Unrenamed records remain byte-identical. A legacy
renamed origin without its companion refuses Put Away instead of guessing.
The host oracle compares the encoded pair against a hand-authored byte golden,
then decodes the independent pair before restoring the original name and place.

Replace's recoverability policy is an authored adaptation: preserve the displaced
item through the existing Trash staging path, including its origin, before
committing the incoming move. A failed incoming move attempts to put the displaced
item back. This is not claimed as a pixel-captured native storage algorithm.
The confirmed incoming bytes and Put Away bytes are checked through mtools.

The disk-full oracle saturates a real FAT allocation table and verifies a failed
DESKTOP.DB bootstrap leaves no dirent. Storage is checked before DOS CREAT, which
otherwise publishes a zero-byte entry before the failed write.

Rule 14 blocks precede record-flair. The PM records the clips after merge.
No bead, git metadata, CLAUDE.md or HANDOFF.md is changed by this lane.

Locked-trace re-keys for tdnl.65/.72: folder_nav and finder_icons explicitly
activate the background root before their original double-click/selection;
app-launch DOUBLE adds the second content click that quits after activation;
file-ops REFUSE dismisses its first notice before driving the cycle refusal.
The app-launch byte golden shifts the base/tail by exactly 252*1024 bytes,
the Finder DATA budget increase from 4 KiB to 256 KiB. It still compares every
launch/gate/event/exit line and is checked by the original NO_REGISTER mutant.

The input oracle also exhausts the actual icon arena and checks that a failed
reservation preserves the committed listing. Open folder identities follow Put
Away and Replace staging through the ordinary move helper. A failed Replace
rollback keeps the displaced item's Trash origin recoverable and persists it.
The independent alert-capture loader uses the local corpus directly, without an
environment override (only the documented QMP stdio seam is used by this lane).

STOP: tdnl.88 has no independent Mac OS 8.1 pressed push-button capture.
The corpus sys8/controls.md section 5 explicitly records this GAP. The required
capture is goldens/captures/s8_alert_ok_pressed.png: the default OK at
x435..493,y158..177 held down, pointer at (464,168), in the same standard alert
as s8_alert_modal.png. The gate compares its left 16x20 bevel/face crop directly
against the native pixels. The gate is registered and intentionally RED until
that capture is supplied. Tracking/release ownership is implemented, but the
new path does not paint the unresolved pressed fallback. The final sequential
verification was stopped at this source gap; no bead is closed and no clips
are recorded. Prior GREEN evidence is not a certificate for the final tree.

## Part 2 -- takeover: the mint, the pressed button, the merge

### Context
The Codex lane stopped honestly on a missing ground truth (Law 1/2): no
Mac OS 8.1 capture showed a push button held down (system7-decomp
specs/sys8/controls.md Sec 5, "Pressed buttons are a GAP"). This takeover
had mint authority in ../system7-decomp.

### The mint (system7-decomp mint-010, re/mint-results-010.md)
Basilisk II, Quadra 650 ROM, scratch copy of boot81.img, private Xvfb :186
(mint/s8_icons_session.sh with its own S8_STATE). Special > Empty
Wastebasket... puts up the caution alert; its rect is byte-identical to
s8_alert_modal.png. xdotool mousedown on OK held; captures while held:
s8_alert_ok_pressed.png (pointer 464,168), s8_alert_ok_pressed_tl.png
(pointer 440,161), s8_alert_cancel_pressed.png, s8_alert_ok_held_outside.png,
s8_alert_ok_hover.png. Two presses at (464,168) are pixel-identical over the
alert; the two OK presses differ only inside their disjoint pointer
footprints, so a pointer-free golden is A with A's pointer box from B.

Pressed style (only x436..492 y159..176 changes; frame, 3F corners, ring
and moat unchanged): outer top row / left col #666666, inner top / left
#777777, face #878787, inner bottom / right #969696, outer bottom / right
#A5A5A5, corner steps per the ledger table, label #FFFFFF on the same glyph
pixels. Plain Cancel presses identically. Held and dragged outside: drawn
idle (byte-identical to the idle alert); back inside: pressed; released
outside: no action. controls.md gains Sec 5.3; INDEX-sys8 gap 3 updated.
Committed on system7-decomp master as c8855be (not pushed).

### What changed (InitechOS)
- os/flair/control.c draw_push_button: the hilited branch paints the
  mint-010 anatomy (new PART FLAIR_PART_PLAT_PRESSED_RIM -> the existing
  CIDX_ICON_GRAY_66 row, appended after the icon roles so no ordinal moves;
  #777777 / #878787 reuse PLAT_INACTIVE_FRAME / PLAT_INACTIVE_TEXT). The
  retired accent fill survives only as the named mutant CTRL_MUT_PRESS_ACCENT.
  The pressed label uses the same face-clipped run as the idle one.
- os/flair/dialog.c: the tracked control is redrawn (control + default ring,
  DrawDialog's order) at the press, on every hilite change during the
  excursion, and at release.
- os/flair/tbxgate.c v_dialog: the one-shot tenant transport (CPANELS)
  rebuilds its DialogRecord per call, so a press could never see its
  mouseUp -- every control click in Control Panels had silently become a
  no-op under Part 1's tracking. The transport now completes the press as a
  press + release at one point through the same tracking path.
- os/milton/kmain.c StdFile loop: a mouseUp now reaches the tracked button
  (Part 1 left Open/Save/Cancel clicks waiting for a mouseUp the loop never
  delivered) and the offscreen is presented while tracking.
- Merge with main (lvmerge list view): the per-window meta[] and the
  sort-key scratch move into the SAME arena block as the icons
  (finder_win_reserve_icons), so the list view keeps no fixed 64-entry
  ceiling; test_finder_list_view's scene gains the record/icon heaps and its
  64-row leg reserves its capacity first.
- Gates: test-flair-dialog-pressed-golden now grades the whole OK button,
  moat and ring (x432..496 y155..180, 1690 px) exactly against the composed
  pair, except the label zone (only #878787/#FFFFFF: Chicago is not the native
  Charcoal strike) and FLAIR's own arrow inks inside the held pointer's box.
  New: test-flair-dialog-pressed-golden-mutant (CTRL_MUT_PRESS_ACCENT, objswap
  on the hold image), host leg PRESSED-CAPTURE in test-control, and
  test-control-press-mutant.
- The three Finder data volumes declare fixture contracts and end their
  recipes with fixture-check (main's convention); the control.o recipe now
  depends on flair_look.h (a stale control.o painted the rim with the wrong
  PART ordinal mid-session -- the dependency gap was real).

### Frictions
- The FLAIR cursor is in some held screendumps and not in others (dump
  timing); the grade admits only the arrow's two inks inside the box where
  the Mac's own pointer sat in capture A.
- system7-decomp's default branch is master, not main.

## Part 3 -- takeover 2: commit, merge main, the size floor, the re-keys

### Context
The Part 2 Opus lane was paused by the PM with 15 files uncommitted and
the mint already landed (system7-decomp c8855be, re/mint-results-010.md,
goldens/captures/s8_alert_ok_pressed.png + _tl.png present). This shard
committed that state as-is (79cbd0f2, PM skim clean: no getenv, /home/,
2026 or non-ASCII in the os/ diff), merged origin/main (33 commits: boot
screen, DOS volumes + DPB, SAMIR size, kernel size round 5, the Bochs
VESA 0101h re-key) and ran the brief's gate set one emulator at a time.

### The merge (427d6ffb)
- Makefile: finder_alerts.mk included beside main's int21_dpb.mk and
  adr_citations.mk.
- flair_look.c: main's compact 3-byte part map (WL-0168). The pressed rim
  no longer adds a PART row: FLAIR_PART_PLAT_PRESSED_RIM is the existing
  icon-ramp role DESK_IC_GRAY_66 (ICON_CLUT + 10, CIDX_ICON_GRAY_66 at
  every depth, the same row the Part 2 PART carried), asserted against
  desk_icons.h in finder_icon.c. A new PART made test-flair-part-map RED
  (`+part=80 ...`); with the alias the 405-resolution transcript golden is
  unchanged and green, its mutant RED.

### The 16 KiB floor (test-kernel-size-r4) -- NOT met, stated
Measured on the merged tree: flagship loaded 183,948 B against the
180,224 B floor (main alone: 176,748 B; the bundle adds +6,320 B .text and
+896 B .rodata -- finder_ops_drop out of line 1,373, DrawControl +720,
fw_key 703, DialogHandleEvent +466, finder_win_reserve_icons 405, the
Put Away / launch / notice paths). Per-file -O levels are not a lever:
every LTO object already ends in KERNEL_COLD_LTO_FLAGS' -Oz.

Reclaimed in this lane (38f674da): the push button's idle and pressed
anatomies are two run tables walked by one loop instead of ~70 cfill calls
-- DrawControl 3,977 -> 2,462 B, flagship 183,948 -> 182,764 B (-1,184 B).
Pixel-identical by a scratch differential over 1,932 configurations (h
4..26, w 4..72, both states, default ring, 8 and 32 bpp, whole-region
hash), mutation-proven (one pressed entry flipped -> 436 configurations
differ); test-control (BUTTON-CAPTURE, PRESSED-CAPTURE), -mutant
(BTN_FLAT, BTN_FRINGE, LABEL_OPAQUE) and -press-mutant green.

Remaining: 182,764 B > 180,224 B, 2,540 B over. The floor was not
raised. Size round 6 is due (the WL-0168 note: "3,476 B under the floor
for the seven kernel lanes still to merge" -- this bundle alone spends
6,016 B net). test-kernel-headroom (the 4 KiB global floor) is green.

### Re-keys the bundle needed (a4fdeeae; Rule 8, stated)
- test-flair-file-ops leg 5: the renamed NEWFO001 origin is now followed
  by its tdnl.57 kind=6 companion (origin dir 0, original name NEWFOLD,
  flags/coordinates zero): DESKTOP-DB-SAVE n=6, DESKTOP.DB 8 + 6*24 = 152 B,
  the hand-authored tail grows from three records to four (byte-equal to
  what the guest wrote, checked by mtools); the trash_drag clip marker
  follows (n=6).
- test-flair-app-launch DOUBLE: the re-click gains a leading activating
  click (tdnl.65; the FOLDER_NAV precedent) and a kesc for the tdnl.84
  launch notice the refused second launch now raises (FINDER-ALERT
  reason=launch); the leg's one REGISTER / one SLOT-BUSY / one clean exit
  holds, and all six app-launch mutants stay RED for their named reasons.

### The refused leg raced the guest (ba020d1f)
test-flair-finder-refused dumped after injection keyed to the FIRST
notice's marker; since the tdnl.72 kesc dismisses that notice, the harness
(done injecting at ~3.0 s) grabbed a frame before the guest drew the
second (cycle) notice at ~3.1 s -- deterministic RED, ALERT-BANDS
369/6496. The dump now keys to the cycle notice's last serial line (it
stays up until the image's life ends at ~15.8 s); the samedir notice is
still asserted on serial. The move-alert mutant's dump waits for the same
cycle refusal, so its RED (369/6496, no notice) is not a timing artefact.

### Small fix (165013a0)
alert_capture.h's pressed-pair helpers are static-unused-safe, so
test-dialog / test-dialog-tracking (which include the header without the
pressed leg) compile under -Werror again.

### Bochs legs (Rule 5)
Green: test-flair-desktop-bochs, test-flair-appswitch-bochs,
test-flair-app-launch-bochs. RED, not touched by this lane:
test-flair-disk-windows-bochs, test-flair-file-ops-bochs,
test-flair-trash-window-bochs, test-flair-alert-chrome-bochs -- each fails
`marker VBE-ENOMODE missing`: they still expect the pre-vesa8 mode-13h
path that WL-0170 re-keyed elsewhere (Bochs now takes VESA 0101h). The
lane's diff does not touch these recipes or the video path; re-keying them
is WL-0170 follow-up work, not this bundle's.
The move-alert mutant's extra harness argument rides as the macro's
seventh parameter; the two other notice mutants pass it empty (a trailing
comma) so test-makefile-vars sees no read of an undefined `7`.

### Acceptance (merged tree, scratchpad env.sh, one emulator at a time; `make test` not run)
    make image                              green (build/initech.img + build/flair_data.img)
    test-makefile-vars, test-fixture-volumes   green (72 inline image writers, 42 contracts)
    test-kernel-headroom                    green (flagship headroom 13,844 B; floor 4,096 B)
    test-kernel-size-r4                     RED: loaded 182764 B > 180224 B (above)
    test-flair-part-map (+ -mutant)         green (405 resolutions == the pre-change table) / RED
    test-control, -mutant, -press-mutant    green; PRESSED-CAPTURE 1168/1168 at 8 and 32 bpp; CTRL_MUT_PRESS_ACCENT RED
    test-color-canon                        green
    test-dialog, test-dialog-mutant         green (81 checks; all nine mutants RED)
    test-finder-alerts                      green (8 host suites; 14 named mutants RED)
    test-flair-dialog-pressed-golden        green (held OK == native mint-010 pressed button, 1690 px)
    test-flair-dialog-pressed-golden-mutant RED: FAIL PRESSED-CAPTURE: 412/1690 pixels differ ...; label ink 43 (native 73)
    test-flair-finder-alerts                green (10 QEMU families; 13 named mutants RED)
    test-flair-disk-windows (+ -mutant)     green / RED (win_vollabel_shown, win_singleton_dup)
    test-flair-file-ops (+ -mutant)         green (5/5 legs, kind=6 companion on disk) / RED (TRASH_NO_STAGE, NO_HILITE)
    test-flair-app-launch (+ -mutant)       green (10/10) / all six RED for their named reasons
    test-flair-finder-icons (+ -mutant)     green / RED
    test-flair-desktop, test-flair-appswitch   green
    test-flair-trash-full, -trash-window, -empty-trash, -finder-service   green
    test-flair-dialog-tracking (+ -mutant)  green / DIALOG_MUT_ON_PRESS RED
    test-flair-cpanels (5 panels), -settings, -mutant   green / CTRL_MUT_RADIO_DESKTOP_BG RED
    test-stdfile, test-process, test-flair-i123-file-ui, -i123-files, -ctenant   green
    Bochs: desktop, appswitch, app-launch   green; disk-windows, file-ops, trash-window, alert-chrome RED (VBE-ENOMODE, above)

### Undone
- test-kernel-size-r4: 2,540 B over the 16 KiB floor -- size round 6.
- The four ENOMODE Bochs legs (WL-0170 follow-up).
- Clips: the 14 Rule-14 blocks stay for the PM to record after the merge;
  no bead closed, CLAUDE.md / HANDOFF untouched, nothing pushed.
