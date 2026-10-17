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
