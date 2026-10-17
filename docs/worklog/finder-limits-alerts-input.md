# Finder limits, alerts and input

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
