# Desktop pixel reference coverage

`pixels-desktop reference.ppm actual.ppm [mask.ppm]` is exact and ENFORCED by
default. It compares all 307200 pixels, or every white pixel of an explicit
binary mask. One differing pixel, malformed/truncated image or empty/nonbinary
mask fails. `--informational` explicitly requests diagnostics only; it is used
solely for the live legacy whole-desktop comparison, not independent captures.

The product has no independent 640x480 reference for an entire current screen.
Legacy-final is the ancestor of this renderer, including the same art and
layout code. Its screenshots are regression evidence, not an independent
fidelity oracle. No capture of today's renderer has been installed as a golden.

## Enforced on the composed desktop

The QEMU `desktop`, `disk`, `apps`, and final `qemu` restart captures enforce
**1578 exact RGB pixels** each:
the 934 opaque volume-icon pixels and 644 empty-Trash pixels. Expected images
and masks are independently recovered from genuine Mac OS 8.1 Finder PNGs,
whose hashes are in references/desktop-icons.sha256 (checked on every smoke).
Source: system7-decomp/re/mint-results-008.md, section 2. The source cells are
(576,156) for volume and (576,412) for empty Trash; the clean desktop tile is
sampled from s8_icons_trash_full_sel.png at
(128 + x mod 128, 148 + (y - 148) mod 128). Opacity is the union of differences
from that tile in unselected and selected captures, which recovers cells whose
unselected colour happens to match the background. Population checks reject a
wrong crop. RGB comes directly from the source capture, with zero tolerance.
The desktop and restart use unselected volume; disk/apps use the captured selected volume.

Placement is an authored layout contract: right margin 24, icon size 32,
8 below the single 20-pixel bar (operator's single-bar ruling); volume origin
(584,28), Trash (584,404). It is stated here independently, not computed with
product layout functions. The clear pixels, wallpaper and labels are outside
the mask because their names/background differ from the captured Mac desktop.
The factory links only libpng to read the reference; no product art/renderer.

`smoke-desktop.sh` propagates pixels-check.sh's exit. Cert's mandatory
image-smoke stage therefore enforces these masks regardless of CERT_STRICT.
Calculator's real Mac capture crop and Paint's blank-canvas property are also
already enforced by their app-owned verifiers; they remain enabled.

## Screens without independent whole-screen references

Every screenshot below remains a retained diagnostic for full-screen fidelity:

* desktop, disk, apps, qemu (restart): icon masks enforced above; no independent screen has
  Initech's exact filenames, directory inventory, wallpaper, labels, clock or
  menus. Whole-image legacy differences are explicitly INFORMATIONAL.
* sum: Lotus data and RPN are independently read back, but the hybrid graphical
  123 face has no matching real-Lotus or film-frame full-screen reference.
* word, word-saved: real WP5 data/codes are read back; no independent capture
  matches this graphical Word face and this journey's document/settings.
* calculator: app capture crop is independently enforced; surrounding Finder
  desktop/layout and Initech window title are not a full Mac screenshot.
* notepad, scrapbook, text, patterns: independent persisted text/settings checks
  exist, but no reference captures match these entire Initech screen states.
* canvas: blank canvas property is enforced; no whole-screen Paint reference.
* calc: the film still is lossy, lower-resolution and has a different layout
  (including two bars before the single-bar ruling); the ledger still has
  unrecovered cells. It cannot serve as exact RGB ground truth for this scene.
* dos: serial/session and disk invariants are enforced; no capture matches this
  desktop-exit console transcript pixel for pixel.

Bochs currently supplies journey lifecycle/data verdicts but no corresponding
lossless per-journey desktop capture stream. Thus the new composed pixel masks
qualify QEMU presentation only. A Bochs whole-screen fidelity claim is not made.
No tolerance or skip is silently substituted for an absent reference.
