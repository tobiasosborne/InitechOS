<!-- INITECH CONFIDENTIAL — INTERNAL USE ONLY — DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->
<!-- Ground-truth brief: the FLAIR chimera GUI -- Mac System 6/7 chrome + Windows 3.x -->
<!-- accents + the Photoshop-menu-bar canon. Law 1: every claim cites a LOCAL file, the -->
<!-- locked spec, an ADR/PRD section, a bead, or a clearly-stated EXTERNAL source with its -->
<!-- legal/abandonware status. NO code ships from this brief; the locked spec-data + the -->
<!-- oracles do. Synthesised from five recon lanes (A inventory, B Mac refs, C Mac -->
<!-- acquisition, D Windows + chimera map, E minting toolchain). -->

# FLAIR GUI Ground-Truth Brief — System-7 Chrome, Windows-3.x Accents, the Chimera Element Map, and the Golden-Minting Toolchain

**Issuing Body:** Initech Systems Corporation — Platform Engineering, FLAIR Toolbox Section
**Document Class:** Ground-Truth Brief (per-milestone evidence base; supersede in place)
**Programme / Milestone:** InitechOS (STAPLER) — **M3/M4, FLAIR Toolbox** (PRD §6.3, §10; ADR-0004 RATIFIED, ADR-0005 RATIFIED)
**Governing Epic:** `initech-k8o5` (FLAIR Toolbox). Chrome lattice: `chrome_metrics.json` v0→v1 (FO-2, beads `initech-k8o5.8`); 86Box leg (DEC-04, beads `initech-q0gy`, BLOCKS M4 via `initech-k8o5.12`).
**Last Reconciled:** 2026-06-19

> Incoming agent: this brief is the *evidence base* for FLAIR chrome. It does not authorise code. Read `CLAUDE.md` (Laws & Rules), then PRD §6.3 / §10 / §12, then ADR-0004 (Manager decomposition, D-7 chrome, D-8 oracle vector) and ADR-0005 (region engine), then `bd show initech-k8o5`, then this brief. The oracle is the truth, not this document (Law 2): where a metric below is flagged **golden-resolves**, do **not** reason it out — mint it from real System 7 / Windows 3.x under an emulator and assert. Per ADR-0004 FO-2/AM-3, `chrome_metrics.json` v1 must be LOCKED **and** `test-chrome` MUTATION-PROVEN (three named mutants) before ANY Window/Control Manager drawing code ships.

---

## 1. Purpose and Law-1 framing

FLAIR is the System-7-style Toolbox layer of InitechOS — the Mac chrome of the *Office Space* "Saving tables to disk…" frame, fused with Windows-3.x chimera accents and the canonical Photoshop menu bar. The FLAIR oracles defined in ADR-0004 D-8 (`test-chrome` = chrome renders match `chrome_metrics` v1 + fixture crops; per-window SSIM as **guide not gate**; `canon` byte tests) all consume **real era metrics and real era screenshots as goldens**.

Law 1 is unconditional: *"Ground truth before code. Every decision cites a local source… If the source isn't local, acquire it before writing the code that depends on it."* We cannot author System-7 chrome from memory; the still in `spec/assets/preview.webp` is a CRT photo (Law-1 hallucination callout: *"too low-res/compressed to recover Chicago/Geneva strikes"*). The native-pixel constants must come either from Apple's own published source (the System 7.0 WDEF assembly, Inside Macintosh) or from a real System 7 / Windows 3.x boot in an emulator, then be locked as data (Rule 8) and mutation-proven (Rule 6).

This brief inventories what is LOCAL, fixes the Mac and Windows ground truth from cited sources, maps the chimera element-by-element, specifies the deterministic minting toolchain, and prioritises acquisition. It resolves contradictions surfaced across the five recon lanes.

---

## 2. What is LOCAL today (inventory)

`have` = on disk now. `acquirable` = one command / one download away (legal path stated). `build` = compile/author from source. `n/a` = present but irrelevant to FLAIR.

| Item | Kind | Locality | Path / source | FLAIR use |
|---|---|---|---|---|
| QEMU 8.2.2 | emulator | have | `/usr/bin/qemu-system-i386`; QMP driver `harness/emu/qemu.c`, `qemu_main.c` | InitechOS dev-loop framebuffer; `test-chrome`/`ssim` capture (P6 PPM via QMP screendump). Verified installed. |
| Bochs 2.7 | emulator | have | `/usr/bin/bochs`; driver `harness/emu/bochs.c` (RFB handshake + drain, no pixel capture yet) | Accuracy gate; pixel capture needs ~50 LOC RFB `FramebufferUpdate` extension. Verified installed. |
| DOSBox-X 2024.03.01 | emulator | have | `/usr/bin/dosbox-x` | Cheapest Windows-3.x chrome mint path (svga_s3/386, Ctrl+F5 / F12-S PNG to `capture/`). Verified installed. |
| 86Box 6.0 | emulator | have | flatpak `net._86box._86Box` (verified `flatpak list`) | Period-authenticity leg (DEC-04 deferred; beads `initech-q0gy`). |
| 86Box ROMs 6.0 | rom | have | flatpak `net._86box._86Box.ROMs` (verified); S3 Trio64/864, Cirrus 5428/5436, ET4000, IBM VGA, 478 machine BIOSes | Period VGA BIOS for 86Box Win-3.x boots. **Unverified:** that the flatpak ROM extension mounts where 86Box looks — must launch once to confirm. |
| `chrome_metrics.json` v0 | locked spec-data | have | `/home/tobias/Projects/initech-os/spec/chrome_metrics.json` | 4 fields, **FRAME pixels** (1456x819 photo), not native. v1 uplift is FO-2. |
| `chicago8x16.h` | font | have | `/home/tobias/Projects/initech-os/spec/assets/chicago8x16.h` | Hand-authored clean-room Chicago strike, 8x16 fixed cell. Title/menu/dialog text. |
| `palette.json` + `palette.h` | locked spec-data | have | `/home/tobias/Projects/initech-os/spec/assets/` | Frame-sampled colors. Note `desktop_bg` v0 gray vs SEAFOAM canon reconcile is FO-3 (beads `initech-ch81`). |
| `preview.webp` (frame still) | fixture | have | `/home/tobias/Projects/initech-os/spec/assets/preview.webp` (1456x819, gitignored) | SSIM reference (guide); canon source (Photoshop string, 570-, 116% pie). |
| dbase3-decomp mint harness | tooling | have | `/home/tobias/Projects/dbase3-decomp/mint/*.conf` (svga_s3/386) | Reusable headless DOSBox-X capture template for the Windows side. |
| QMP screendump harness | tooling | have | `harness/emu/qemu.c`, `tools/ppm_seafoam_check.c`, `tools/ppm_text_check.c` | Prototype consumers for `test-chrome`/`ssim` pixel oracle. |
| ImageMagick 6.9.12 + Pillow 10.2.0 + ffmpeg | tooling | have | `/usr/bin/convert`, Python `PIL`, `/usr/bin/ffmpeg` | SSIM via `compare -metric SSIM`; PPM crop/measure. Verified. |
| Amiga Kickstart ROMs | rom | n/a | `~/Dropbox/Backups/.../roms/Kick13.rom`, `KICK31.ROM` | Wrong platform (Amiga). No FLAIR use. |
| Basilisk II 0.9.20240401 | emulator | acquirable | `apt install basilisk2` (multiverse; verified Candidate present, Installed none) | Primary System-7 color-chrome mint tool (needs ROM + disk). |
| Mini vMac 36.04 | emulator | build | gryphel.com source tarball | System-6 B&W chrome (secondary; needs Mac Plus ROM). |
| scikit-image | tooling | acquirable | `pip install scikit-image` (NOT installed; verified import fails) | Optional Python SSIM; ImageMagick already covers the gate. |
| Mac 68K ROM (Plus / II / Quadra) | rom | acquirable* | EXTERNAL, abandonware-grey | **Hardest dependency.** Blocks all Mac native-pixel mints. |
| System 7.0.1 / 7.5.3 disk image | disk-image | acquirable | EXTERNAL, archive.org (7.0.1 free; 7.5.3 Apple-freeware) | Boot media for Basilisk II / Mini vMac. |
| Windows 3.1 / 3.11 disk image | disk-image | acquirable | EXTERNAL, archive.org / WinWorld (abandonware) | Boot media for DOSBox-X / 86Box Win-chrome mints. |
| Inside Macintosh PDFs | reference-doc | acquirable | EXTERNAL, developer.apple.com archive + archive.org (free) | Toolbox Essentials, Imaging With QuickDraw — Manager records + chrome metrics. |

---

## 3. Mac System 6/7 ground truth

### 3.1 Target system version: System 7.0 / 7.1 (color), NOT System 6, NOT 7.5

Resolved across lanes A/B/C/E. System 6 is too early — flat black-and-white chrome, no pinstripe gradient. System 7.0 (released 1991-05-13) introduced the gray-shaded 3D chrome with the pinstripe title bar on **color** machines (`minColorDepth = 8` in the WDEF source); it falls back to flat B&W on mono displays. System 7.5 (1994) adds Appearance-Manager refinements (slightly more 3D, thicker shadows) but the same fundamental geometry. The frame shows color chrome with a gray pinstripe title bar — System 7.0/7.1 on a color VGA display.

InitechOS runs indexed-8 (ADR-0004 OD-2) at 640x480, so the WDEF `minColorDepth=8` precondition is satisfied — FLAIR renders the full 3D-shaded System 7 chrome, not the B&W fallback. **Decision: target System 7.0/7.1 chrome.** If only a Quadra ROM is obtainable, System 7.5.3 is an acceptable proxy for geometry (same WDEF constants) but record the version in `chrome_metrics.json` v1 comments.

### 3.2 Primary references (all EXTERNAL, all free, all citable)

- **System 7.0 WDEF assembly source — StandardWDEF.a** — Apple developer archive sample code. The authoritative native-pixel constants:
  `https://developer.apple.com/library/archive/samplecode/System_7.0_WDEF/Listings/StandardWDEF_a.html` (free, no paywall; Apple retired sample code).
- **Inside Macintosh: Macintosh Toolbox Essentials (1992)** — Window/Control/Menu/Dialog Managers; PDF `https://developer.apple.com/library/archive/documentation/mac/pdf/MacintoshToolboxEssentials.pdf` (3.8 MB, free). HTML chapters: Toolbox-189 (title bar 20 px), Toolbox-303 (15 px scrollbar inset), Toolbox-313 (`kScrollbarWidth=16`, 1 px frame, 16 px grow box).
- **Inside Macintosh: Imaging With QuickDraw (1994)** — Region API semantics (NOT internal body format); PDF `https://developer.apple.com/library/archive/documentation/mac/pdf/ImagingWithQuickDraw.pdf` (free).
- **Inside Macintosh Vols I–VI (1984–1991)** — the verbatim Manager records/part-codes ADR-0004 D-3 carries: archive.org `bitsavers_applemacIn84_*`, vintageapple.org/inside_o (free, historical Apple docs).
- **Macintosh Human Interface Guidelines (1992)** — design intent (not pixel source): `https://vintageapple.org/inside_r/pdf/Human_Interface_Guidelines_1992.pdf` (free).
- **GetMBarHeight** — menu bar = 20 px (Roman script): `https://developer.apple.com/library/archive/documentation/mac/Toolbox/Toolbox-128.html`.

### 3.3 Native-pixel chrome metrics (authoritative, source-cited)

These supersede the v0 frame-pixel values for the native-OS constants that drive `test-chrome`. Mark each in v1 with its source URL.

| Metric | Native value | Source | Notes |
|---|---|---|---|
| Menu bar height | **20 px** | GetMBarHeight (Toolbox-128); Toolbox Essentials | Roman script. v0 has 19 (frame px) — see §3.4 resolution. |
| Title bar height (std doc window) | **19 px** | WDEF `minTitleH EQU 19` | 21 px (`minTitleHIcon`) with small icon. |
| Scrollbar width | **16 px** | WDEF `scrollBarSize EQU 16`; Toolbox-313 `kScrollbarWidth` | 16 px control rect; right 1 px overlaps the 1 px window frame, presenting 15 px of added width (Toolbox-303 "15-pixel-deep region"). The 15-vs-16 is one pixel of frame overlap, not a contradiction (see §3.4). |
| Window frame | **1 px** line + 1 px inner groove | WDEF frame inset = 1; Toolbox-313 "1-pixel-wide window frame" | The classic Mac double-line frame; v0 frame px = 2. |
| Grow box | **16 px** high | Toolbox-313 (same listing) | 2D box bottom-right; exact 2D fill pattern is **golden-resolves**. |
| Close box / zoom box | **~13 px** frame (golden-resolves) | WDEF WBoxDelta = `(titleHgt-13)/2`; goAway at x=32, zoom right margin=26 | The 13 is the box frame size used for vertical centering; the *rendered* 11–13 px appearance and 4x4 interior reduction need a screendump. |
| Dialog (`dBoxProc`) border | **7 px** | WDEF `dBoxBorderSize EQU 7` | Fancy modal border. |
| Drop shadow (`documentProc`) | **1 px** at (1,1) | WDEF | |
| Small icon in title | **20 px** | WDEF `IconSize EQU 20` | |
| Pinstripe period | **2 px** (1 dark + 1 light) | community CSS recreation + v0 frame measurement | Exact shade indices `wTitleBarLight`/`wTitleBarDark` and their 8-bpp RGB are **golden-resolves** (extract `wctb` resource ID -4096 via ResEdit once an emulator boots). |

### 3.4 Resolved contradictions

- **Menu bar height: 19 (frame) vs 20 (native).** Resolved: **20 px is the native authoritative value** (GetMBarHeight, Roman script). The v0 `menubar_height_px=19` is a CRT-photo frame measurement; v1 records native 20 alongside the frame value for traceability. (Lanes B and C both reached 20; lane A/D carried the v0 19 — superseded.)
- **Title bar 19 (WDEF) vs 24 (v0 frame).** Resolved: **19 px native** is authoritative; the 24 is upscaled photo pixels. The implied frame upscale (24/19 ≈ 1.26x) does NOT match a naive 640→1456 (2.275x) scale, because the OS window in the frame is not full-screen — the photo is not a linear upscale of a 640x480 buffer. **Do not back-compute native pixels from the frame photo.** Use WDEF/IM native constants for oracle datums; keep frame px only as annotation. (Lane C and lane D independently flagged this; both conclude native-from-emulator/WDEF is authoritative.)
- **Scrollbar 15 vs 16.** Resolved: control rect is **16 px**; the right edge overlaps the 1 px window frame, so it presents 15 px of *added* width past the content area. Both IM passages are consistent. Use 16 for the rendered control.

### 3.5 Fonts

- **Chicago 12 pt** — Mac system/menu/title/dialog font, 1984–1997. LOCAL: `spec/assets/chicago8x16.h` (hand-authored clean-room, fixed 8x16 cell; Chicago is proportional — proportional metrics are a later asset task per ADR-0004 D-7 "proportional NFNT text measurement"). Cross-check references (Apple copyright, **reference only, never shipped**, Law 1 clean-room): `Chicago-12.bdf` (github danfe/fonts, Fondu conversion), Chicago Kare (chicagokare.xyz, license unconfirmed — verify before any use), JohnDDuncanIII/macfonts (.dfonts).
- **Geneva 9 pt** — the FLAIR cell font (ADR-0004 D-7 "Geneva 9 (cell)"). NOT yet authored; hand-author clean-room like Chicago, using macfonts as reference-only specimen.
- **Monaco 9 pt** — monospace; needed later, same clean-room approach.

### 3.6 Cursor

The real System 7 busy cursor is an **animated wristwatch** (16x16, 8 frames, in the System file). **InitechOS canon is the HOURGLASS** — the wristwatch is "the bug," the deliberate frame anachronism (CLAUDE.md Law 4; ADR-0004 D-7: hourglass shipped as **fixed bytes** in `spec/assets/`, byte-stable per Rule 11; FROZEN locked-data per AM-4, gated by the `canon` oracle, beads `initech-zaqj`). The wristwatch is documented only to confirm the anachronism is real (impossiblue.github.io ResEdit article); **do not implement it**.

### 3.7 Region body format — closed, no action

QuickDraw's `rgnBody` (bytes after the 10-byte `rgnSize`/`rgnBBox` header) is **proprietary and unpublished** (confirmed by lane B; Apple tech notes qd_14/qd_02 redirect to IM and do not document it). Community lore (y-coord, x-pair list, `0x7FFF` per-row/region terminator) is partial reverse-engineering. ADR-0005 **rejects** porting it (A1) and **rejects** the `0x7FFF` in-band sentinel (A2/D-4: `0x7FFF` is a legal int16 coordinate; length-prefixed x-lists are unambiguous). ATKINSON is clean-room (DEC-04a-R1); its sole correctness signal is the homomorphism property suite (`make test-region`, 31 checks green, three mutants per Rule 6). **No external region golden is possible or needed.** This brief closes the verification loop on that claim — no further region research.

---

## 4. Windows 3.x + the chimera element map

### 4.1 Windows 3.x chrome is FLAT 2-D (not Win95)

Win 3.1 chrome is flat 2-D, NOT the Win95 CTL3D heavy bevel. Active title bar = navy `#000080` (RGB 0,0,128) with white caption text; button face `#c0c0c0`; shadow `#808080`; highlight `#dfdfdf`; inactive title bar `#808080`. Border is a 2 px beveled 2-layer line (white/black outer, light-gray/dark-gray inner). System-menu box = gray box with a single horizontal black line (the Win 2.0+ "toaster" glyph). Min/max buttons = flat gray boxes with down/up triangle-or-arrow glyphs (1 px bevel, not Win95 depth). Caption font = **System** (proportional bitmap); icon-title font = **MS Sans Serif 8 pt** (`SSERIFE.FON`). Sources: socket3.wordpress.com (pre-Win95 flatness), Win 3.1 WIN.INI `[colors]` defaults, Wikipedia/KB Q84169 (MS Sans Serif). **All Windows pixel heights (SM_CYCAPTION, SM_CXVSCROLL) are golden-resolves** — measure from a DOSBox-X screenshot; do not trust web claims (the common "17 px scrollbar" is unconfirmed).

### 4.2 The chimera element map (mac-vs-win)

The chimera is **mostly Mac chrome, universally rendered in the System 7 style**, with Windows accents confined to specific controls and the Photoshop menu text. Lane D's eleven-element breakdown, deduplicated and tagged. This must be written as locked spec-data `spec/chimera_element_map.json` (Rule 8) before chrome code ships.

| # | Element | Origin | Rendering |
|---|---|---|---|
| 1 | Pinstripe title bar | mac-system7 | Alternating 1 px light/dark scanlines, 19–20 px tall, Chicago 12 white centered text |
| 2 | Close box (top-left) | mac-system7 | ~13 px frame square, hollow, 1 px border |
| 3 | Zoom box (top-right) | mac-system7 | Same as close box |
| 4 | Vertical scrollbar | mac-system7 | 16 px wide, arrow buttons + pattern-fill track + thumb |
| 5 | Window frame | mac-system7 | 1 px line + 1 px inner groove (double-line, not Win thick border) |
| 6 | Grow box (bottom-right) | mac-system7 | 16x16 |
| 7 | Menu bar | mac-system7 | Solid fill, Chicago 12, 20 px tall |
| 8 | Apple-menu glyph (left of bar) | mac-system7 | — |
| 9 | **Photoshop menu items** `File Edit Image Layer Select View Window Help` | **chimera-intentional** | Mac-located top menu bar; the Photoshop item set is the deliberate inconsistency (see §4.3). Rendered in Chicago in the Mac bar. |
| 10 | Modal dialog buttons | win31-accent | Gray `#c0c0c0` face, flat 1 px bevel (NOT Mac rounded-rect) |
| 11 | MDI child chrome (if present) | win31-accent | Win 3.x MDI pattern |

### 4.3 The Photoshop-menu-bar canon (FROZEN, do NOT correct)

The Photoshop-style menu bar over a Mac window is **the spec, not a bug** (CLAUDE.md; PRD §1, Appendix A). ADR-0004 D-3 carries the exact string `File Edit Image Layer Select View Window Help` for InitechPaint, FROZEN locked-data (AM-4, beads `initech-zaqj`), gated by the `canon` oracle (D-8). **Resolved sub-point (lanes C/D):** real Adobe Photoshop 3.0 for Mac (Sept 1994 — first version with the **Layer** menu) had `Mode` where the frame shows `View`; `View` arrived in a later point release. This menu set is therefore historically impossible as a single Photoshop version — which is exactly why it is canon. **Do not "fix" it to match any real Photoshop.** InitechOS reproduces the prop's string verbatim from the locked constant; we do NOT need to boot Photoshop to confirm it (the string is frozen in-repo, not derived). A Photoshop boot would only confirm the anachronism for the record — lowest priority.

---

## 5. The golden-minting toolchain

Per ADR-0004 D-8: each emulator's screendump is compared against **the host model's prediction for that emulator's own mode** — NOT a cross-emulator byte-CRC (frozen, AM-6; a Stop condition to revert). SSIM is a per-window **guide, not a gate**. The hard gates are `test-chrome` (structural, vs `chrome_metrics` v1 + fixture crops), `canon` (frozen bytes), `test-event`, `fb-agree`, `drag-gate` (AM-8). Golden **minting** is one-shot (capture once, store) — bit-for-bit cross-run reproducibility is NOT required at mint time; it IS required for the runtime oracle, already satisfied (QEMU `-rtc base=T,clock=vm`; Bochs `clock:sync=none`).

### 5.1 Who mints what

| Golden | Emulator | Status | Path |
|---|---|---|---|
| InitechOS FLAIR framebuffer (the oracle target) | QEMU | wired | QMP screendump → P6 PPM → `test-chrome`/SSIM. `harness/emu/qemu.c`. |
| InitechOS accuracy / fb-agree leg | Bochs | needs ~50 LOC | Extend `harness/emu/bochs.c`: after ClientInit send `FramebufferUpdateRequest` (type 3, incr 0, 0,0,640,480), receive `FramebufferUpdate` (type 0), write raw rows → `build/<name>.bochs.ppm` (RFC 6143 §7). |
| System-7 native chrome (chrome_metrics v1, SSIM ref) | Basilisk II | **gated on Mac ROM** | `apt install basilisk2`; Quadra/Mac-II ROM; System 7.1/7.5.3; `xvfb-run` + `xwd`/`import` capture. |
| System-6 B&W chrome (secondary) | Mini vMac | build + Mac Plus ROM | Lower priority than Basilisk II for the color System-7 target. |
| Windows-3.x chrome accents (chimera) | DOSBox-X | cheapest, ready | svga_s3/386; `xvfb-run` + `xdotool key F12+s` (or Ctrl+F5) → PNG in `capture/`; reuse dbase3-decomp `.conf` template. |
| Windows-3.x period-authentic chrome | 86Box | **DEC-04 DEFERRED** | beads `initech-q0gy`; BLOCKS M4 sign-off. ROMs already local. |

### 5.2 Capture + determinism infrastructure

- **Headless GUI emulators (86Box Qt, DOSBox-X/Basilisk II SDL2):** `xvfb-run -a --server-args='-screen 0 1024x768x24' <emu> &`; `sleep <boot>`; `xdotool key <hotkey>`; `xwd -root -display :N -silent | convert xwd:- out.png` or `import -display :N -window root out.png`. Needs `apt install xvfb xdotool` (NOT installed). `import` already present.
- **SSIM gate:** implement `make ssim` with `compare -metric SSIM <crop> <fixture> null:` (ImageMagick present); crop PPM per-window with `convert -crop`. scikit-image optional (`pip install scikit-image`) — not needed.
- **Bochs pixel capture vs vncsnapshot:** prefer the ~50 LOC C RFB extension (lossless PPM, no runtime dep, factory-code discipline) over `vncsnapshot` (lossy JPEG default, extra dep).

---

## 6. Acquisition checklist (prioritized)

**P0 — unblock now, no ROM needed.**
1. `sudo apt install xvfb xdotool` — unblocks headless DOSBox-X + 86Box capture.
2. Download a Windows 3.1 DOSBox image — `curl -L 'https://archive.org/download/windows-3.1_20250322/Win.zip' -o /tmp/win31.zip` (abandonware; archive.org no rights statement; Microsoft non-enforcing). Boot under DOSBox-X (svga_s3/386), capture Program Manager + dialog + scrollbar PNGs.
3. Download Inside Macintosh PDFs — `MacintoshToolboxEssentials.pdf`, `ImagingWithQuickDraw.pdf` from `developer.apple.com/library/archive/documentation/mac/pdf/` to a gitignored `refs/` (free Apple developer archive). Cache the WDEF HTML listing too.
4. Extend `harness/emu/bochs.c` with RFB `FramebufferUpdate` capture (~50 LOC; no acquisition).

**P1 — Mac native-pixel mint (the highest-value golden), gated on ROM.**
5. `sudo apt install basilisk2` (multiverse; emulator binary only — verified Candidate present).
6. **Acquire a Mac 68K ROM** — Quadra/Performa (System 7.1+) or Mac II (350912 bytes). EXTERNAL, **abandonware-grey, Apple copyright, redistribution not cleared**: macintoshrepository.org ROM archive / github macmade/Macintosh-ROMs. **Operator decision required** (the single hardest legal dependency). Do NOT commit ROMs — add `*.ROM` to `spec/assets/.gitignore`. Legal-clean path is dumping from owned hardware (CopyROM).
7. Acquire System 7.5.3 (Apple-freeware) or 7.0.1 (free) disk image — archive.org `AppleMacintoshSystem753` / `AppleMacintoshSystem701`. Boot Basilisk II under Xvfb, capture window/dialog/menu/scrollbar; extract `wctb` (ID -4096) shade RGBs via ResEdit.
8. Update `chrome_metrics.json` v0→v1 — add native constants from §3.3 with per-value source URLs; keep frame px under a `frame_pixels` key. Then mutation-prove `test-chrome` (three named mutants, FO-2/AM-3) **before any Window/Control Manager drawing code**.

**P2 — period-authenticity + polish.**
9. Author `spec/chimera_element_map.json` (Rule 8) from §4.2.
10. Hand-author Geneva 9 + Monaco 9 clean-room strikes (macfonts reference-only).
11. 86Box Win-3.x period-authentic mint — beads `initech-q0gy` (DEC-04; ROMs local; BLOCKS M4 sign-off). First confirm the flatpak ROM extension mounts where 86Box looks (`flatpak run net._86box._86Box` once).
12. (Optional, lowest) Photoshop 3.0 boot to photograph the anachronistic menu bar for the record — the string is already frozen in-repo, so this confirms nothing load-bearing.

---

## 7. OQ-2 recommendation (fund vs defer 86Box capture)

**OQ-2 is already RESOLVED and ratified — do not re-litigate.** ADR-0004 §9 **DEC-04** (unanimous): **DEFER** 86Box pixel-capture funding. Cross-emulator framebuffer agreement runs **now** on QEMU + Bochs under the host-model-per-mode definition (each emulator vs its own host-model prediction, not a cross-emulator byte-CRC — D-8/AM-6, FROZEN). A FUNDED follow-up is already filed: **beads `initech-q0gy`** (verified OPEN, P2), scoped to (1) 86Box + period Cirrus CL-GD5422 / ET4000-W32 BIOS headless capture, (2) host-model DAC/palette calibration, (3) a mutation proof the 86Box `fb-agree` arm goes RED on real divergence. It **BLOCKS M4 sign-off** (via `initech-k8o5.12`).

This brief **endorses the ratified posture** and adds one finding that lowers the future cost: the 86Box ROMs and binary are **already local** (flatpak), and the Xvfb+xdotool capture pattern is shared with the P0 DOSBox-X work — so when `initech-q0gy` is funded, the only net-new work is the host-model DAC calibration and the mutation proof, not standing up the emulator from scratch. Recommendation: **keep deferred per DEC-04; do P0 DOSBox-X Win-3.x capture first** (proves the Xvfb pattern), **then** `initech-q0gy` 86Box before M4 sign-off. No ADR amendment is warranted.

---

## 8. Open risks

- **Mac ROM legality (P1, critical path).** No legal-clean ROM without owned hardware; abandonware copies are Apple-copyright-grey. Blocks every Mac native-pixel mint. Operator decision required; never commit a ROM (`.gitignore`).
- **Frame photo is not linearly scalable to native px.** The 24-vs-19 title-bar gap proves the OS window is not a full-screen 640x480 buffer in the still. Any attempt to back-derive native constants from `preview.webp` will produce wrong numbers — use WDEF/IM native values only (resolved §3.4, but easy to regress).
- **86Box flatpak ROM mount unverified.** The `.sh` wrapper `mkdir -p`s a roms dir but does not copy ROMs; 86Box must find them via the extension mount. Launch once before trusting any 86Box capture.
- **chrome_metrics v1 without a mutation-proven oracle.** FO-2/AM-3 is explicit: a locked metric with an unproven `test-chrome` "lets plausible chrome through." Do not ship Window/Control drawing on a locked-but-unproven oracle.
- **Close/zoom/grow box exact rendered geometry is golden-resolves.** WDEF gives the 13 px frame-delta math, not the rendered fill/interior. SSIM is a guide, not a gate — if the screendump never lands, these stay approximate and `test-chrome` cannot assert them precisely.
- **Pinstripe shade RGBs (wctb -4096) unextracted.** Until an emulator boots, the exact 8-bpp light/dark indices are unknown; the pinstripe oracle can assert *alternation* (period 2 px) but not the exact gray values.
- **Win 3.1 native metrics are all web-claimed, none measured.** SM_CYCAPTION / SM_CXVSCROLL must be pixel-scanned from a DOSBox-X screenshot before they enter `chimera_element_map.json`.
- **Determinism at mint vs runtime.** Mints are one-shot (fine). The runtime `test-chrome`/`fb-agree` path must stay deterministic (QEMU vm clock, Bochs sync=none) — do not let a mint shortcut leak nondeterminism into the gate (Rule 11).
