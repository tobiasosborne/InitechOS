<!-- INITECH CONFIDENTIAL -- INTERNAL USE ONLY -- DO NOT DISTRIBUTE -->
<!-- Controlled Document. Printed copies are uncontrolled. Verify revision before use. -->
<!-- Research brief: Borland SideKick 1.5x for the IBM PC, the period ground truth for -->
<!-- Initech SideKick (bead initech-6fdt, sub-deliverable 1). Law 1: every claim cites a -->
<!-- local page, a local capture, a fetched URL, or is flagged UNSOURCED / HOUSE. -->
<!-- NO code and NO locked spec is changed by this document (Rule 8). -->

# Borland SideKick 1.5x -- Period Grounding Brief

**Issuing Body:** Initech Systems Corporation -- Platform Engineering, InitechDOS (MILTON) Section
**Document Class:** Research Brief (evidence base; no code)
**Bead:** initech-6fdt, sub-deliverable (1). Feeds (2) kernel TSR support, (3) SK.COM, (4) oracles, (5) manual, (7) clips.
**Last Reconciled:** 2026-10-20

> Incoming agent: this brief is the evidence base for Initech SideKick. It does not
> authorise code. Read `CLAUDE.md`, then ADR-0003 (DEC-04a vector map, DEC-08/08a loader),
> then `bd show initech-6fdt`, then this brief. Where this brief says MEASURED, the number
> came from running the real SideKick 1.56A under real MS-DOS 3.30 in DOSBox-X on
> 2026-10-20 (rig and hashes in `docs/research/sidekick/PROVENANCE.md` Sec. 3); every
> such frame is in `docs/research/sidekick/captures/` with an exact cell transcription.
> The oracle is the truth, not this document (Law 2).

Source tags used below (full provenance, URLs, sha256: `docs/research/sidekick/PROVENANCE.md`).

| Tag | Source | Where |
|---|---|---|
| HB15 | Borland, *SideKick Version 1.5 Owner's Handbook*, 1st ed. March 1985 (AST Research special edition). Cited by PRINTED page (PDF page = printed + 7). | local `docs/research/sidekick/manual/` (git-ignored) |
| HB10 | Borland, *SideKick Version 1 Owner's Handbook*, 1984 (printed page; PDF = printed + 11). Cross-check only. | local, same |
| FS15 | Borland, *SIDEKICK Version 1.5* fact sheet / technical specifications, 1985, p. 2 | local, same |
| RM156 | `READ-ME.SK` on the SideKick 1.56A distribution disk (13458 bytes, 1985-08-05): "New Features" and "Corrections/Omissions" to HB15 | local `media/Sidekick1985_v1.56A.ima` |
| STR | printable strings of `SK.COM` / `SKINST.COM` 1.56A (read with `strings`, never executed on the host) | same disk |
| CAP | our DOSBox-X captures of SK 1.56A under MS-DOS 3.30, file name given per claim | tracked `docs/research/sidekick/captures/` |
| AST | third-party screenshots of the AST OEM 1.56A disk (archive.org `ast_sidekick156a`) | local `media/` |
| SRC | InitechOS sources, `path:line` at lane base `546e5bc2` | this repo |

---

## 0. Honest limits (read first)

1. **Version mix.** The only local handbook is the 1.5 manual (March 1985). The binary we
   ran and captured is 1.56A (August 1985). Where 1.56A differs from the handbook, RM156
   is the authority and the difference is called out (function-key reassignments in the
   Dialer and Appointment windows, Sec. 2.8-2.9). The handbook's Figure 1-1 is a screen
   reused from version 1.10A (it says "Version 1.10A"); its memory figures are therefore
   NOT 1.5x figures (Sec. 2.2).
2. **Reviews not retrieved.** Byte / PC Magazine reviews were not fetched this lane. No
   claim below depends on them; they are listed as cited-only in PROVENANCE.
3. **Copyrighted sources stay local.** The scans, disk images and the DOS 3.3 boot disk
   are git-ignored per the house rule in `initech-manuals-ref/SOURCES.md` (copyrighted
   manuals are kept out of the public repo). This deviates from the lane brief's "commit
   files under 20 MB"; the PM/operator may reverse it with one `git add -f`. Model
   structure, not prose: nothing below reproduces Borland text beyond short quotes and
   the on-screen strings needed to specify the look.
4. **Emulated, not real, hardware.** Captures are DOSBox-X, not 86Box or real iron.
   Colours are the standard 16-colour text attributes, so they transfer; exact pixel
   timing (CGA snow, the "blinking display" install option) was not observed.
5. **Activation timing.** Under xdotool the chord occasionally did not register on the
   first press; a retry loop was used (the log in the rig). That is an artefact of
   synthetic key injection under host load, not a SideKick behaviour, and is NOT claimed.

---

## 1. What SideKick 1.5x is

- The "desktop organizer just a keystroke away": a resident program that stays in memory
  "until you turn the power off or reset" and pops windows over whatever is running; the
  interrupted program "continues as if nothing happened" (HB15 p.1).
- Six tools plus Help and Setup: **Notepad**, **Calculator**, **caLendar** with
  **Appointment** book, **Dialer** (phone directory + Hayes autodial), **ASCII table**
  (HB15 pp.1-2; FS15 "six essential desktop tools").
- Platform: IBM PC, XT, AT, Portable, PCjr, 3270 and compatibles; PC-DOS/MS-DOS 2.0 or
  later; 128K minimum (FS15 p.2). Load-time refusal on DOS 1.x:
  "Incorrect PCDOS version (must be 2.0 or higher)." and, for an unrecognised DOS,
  "Sorry, unknown PCDOS version. Please report to BORLAND." (STR). MEASURED: the second
  message is what 1.56A prints under DOSBox-X's built-in DOS for `ver=` 2.11, 3.10, 3.20
  and 3.30 -- it fingerprints DOS internals, not just the version number -- and it loads
  cleanly under real MS-DOS 3.30 (CAP `vga-00-logon.png`).
- Releases seen: 1.00a (disk dated 1984-07-23, archive.org `sidekick-v1.00a`), 1.10A
  (HB15 Fig. 1-1), 1.5 (HB15, March 1985), 1.56A (August 1985, D156/AST), 1.56B
  (December 1986 disk). Price $54.95 copy-protected / $84.95 not (FS15 p.2).

---

## 2. Behaviour catalogue (each item cited)

### 2.1 Distribution, installation, loading

- **Four builds** of the resident program on the disk (HB15 pp.6-7):
  `SK.COM` full system; `SKN.COM` no Calendar; `SKC.COM` no Notepad; `SKM.COM` Calculator
  and ASCII table only. Also `SK.HLP` (help text, read from the start directory at run
  time), `SKINST.COM` (installer), `READ-ME.SK`. 1.56A disk adds `SKINST.MSG`, `NOTES`
  (sample note file), `PHONE.DIR` (sample directory), `README.COM` (CAP
  `vga-01-foreground.txt` rows 6-16 is the `DIR` of the disk).
- **No command-line switches.** HB15 says only "type SK" (p.9) and every option is set
  either by SKINST (which PATCHES the `.COM` files on disk, HB15 pp.93, 101) or by the
  Setup window's Save (which writes "the Setup directly in this file", HB15 p.86). No
  `/option` syntax appears in HB15, RM156 or STR. => The bead's "SK.COM /options" has no
  period basis; the period configuration store is the executable itself (Sec. 6, D-3).
- **Load order:** non-Borland resident programs first, SuperKey next, SideKick LAST
  (HB15 p.8, p.111). **Start directory:** you must `CD` to the directory holding
  `SK.COM` and `SK.HLP` before typing `SK`, because Help and Setup-save look there, not
  on the PATH (HB15 p.8, pp.87-88; RM156 Q&A).
- **Log-on screen** (MEASURED, CAP `vga-00-logon.txt`; same layout in AST screenshot 01):
  ```
  ╒════════════════════════════════════╕      rows 0-7, frame attr 0x0F
  │ SideKick             Version 1.56A │      text attr 0x1B (lt cyan on blue)
  │                  IBM-PC/XT/AT/PCjr │
  │                                    │
  │                                    │
  │                                    │
  │ Copyright (C) 1984,85 BORLAND Inc. │
  ╘════════════════════════════════════╛

    Full System                              (SKN: "Calendar not included",
                                              SKC: "Notepad not included",
    655360 bytes total memory                 SKM: "Calculator & ASCII-table only")
    600368 bytes were free
    536192 bytes free
  ```
  The three memory lines are total, free-before-load, free-after-load (HB15 p.9).
- **Already loaded:** "Sidekick already active - Please press Ctrl-Alt or
  LeftShift-RightShift to activate" (STR; the two hotkey names are spliced in from the
  installed activation table).
- **Removal:** from the main menu press Ctrl-Home-End, and only at the DOS prompt with no
  resident program loaded after SideKick, else "DOS will go bananas" (HB15 p.10).

### 2.2 Resident size

| Build | Free before | Free after | Resident (bytes) | KiB | Source |
|---|---|---|---|---|---|
| SK.COM 1.56A (full) | 600368 | 536192 | **64176** | 62.7 | MEASURED, CAP `vga-00-logon.txt` |
| SKN.COM 1.56A (no Calendar) | 600368 | 545248 | 55120 | 53.8 | MEASURED, CAP `vga-skn-logon.txt` |
| SKC.COM 1.56A (no Notepad) | 600368 | 557376 | 42992 | 42.0 | MEASURED, CAP `vga-skc-logon.txt` |
| SKM.COM 1.56A (Calc + ASCII) | 600368 | 575248 | 25120 | 24.5 | MEASURED, CAP `vga-skm-logon.txt` |
| SK.COM 1.56A AST OEM | 633664 | 569440 | 64224 | 62.7 | AST screenshot 01 (independent rig) |
| SK.COM 1.10A | 4994?8 | 439328 | ~60160 | ~58.8 | HB15 Fig. 1-1 (one digit illegible in the scan) |

All with the default 4000-character note buffer (Setup shows "Size: 4000 chars (Set with
SKINST)", CAP `vga-12-setup.txt`). The note buffer is RAM-resident, so a larger
installed size grows the resident image (HB15 p.99; FS15 notepad "45K-50K").

### 2.3 The pop-up chord

- **Primary: Ctrl-Alt. Secondary: both Shift keys** (LeftShift-RightShift). HB15 p.13
  ("hold down the Ctrl and Alt keys at the same time. Another way ... hold down both
  Shift keys"); HB10 p.8 identical. MEASURED: SKINST 1.56A "Define Sidekick Activate
  commands" screen shows `Primary command:   Ctrl-Alt` / `Secondary command:
  LeftShift-RightShift` (CAP `skinst-activate.txt` rows 2-9).
- **The alternatives:** both commands are re-definable in SKINST (menu item `A`,
  "Activate commands", new in 1.5x; CAP `skinst-main.txt`). The rules on that screen:
  "You can use combinations of the Ctrl, Alt, LeftShift, and RightShift keys" -- i.e.
  only shift-state keys, never a character key (CAP `skinst-activate.txt` rows 17-23).
- **Fires on release.** MEASURED: a frame grabbed while Ctrl-Alt (or both Shifts) is held
  shows no SideKick; the menu is up in the frame after release (CAP `chord-shifts-held` vs
  `chord-shifts-released`, `chord-ctrlalt-held` vs `chord-ctrlalt-released`).
  Consistent with the chord being a modifier-only press with no other key in between.
- **The same chord inside SideKick leaves it**, and the next chord returns "to the exact
  same window setup that you left" (HB15 p.36). MEASURED: Ctrl-Alt with six windows
  open returned to DOS with the screen restored (CAP `vga-13-restored-after-ctrlalt`).
- **Alt alone held > ~1.5 s** pops the main menu from inside any SideKick window (HB15
  p.23); the same 1.5 s timer bites Alt+numeric-keypad character entry in the Notepad
  (HB15 p.106).
- **When it will not come up:** with a pending DOS critical error (Lotus/Symphony with no
  disk in B:) "your computer makes a chirping sound and Sidekick does not come up"
  (HB15 p.110). Over a graphics screen it switches to text mode (Sec. 3.1).

### 2.4 Main menu and the function keys

MEASURED (CAP `vga-02-main-menu.txt`, cols 29-52, rows 7-17, default position):
```
╔  SideKick Main Menu  ╗      frame 0x0F (bright white on black), double line
║                      ║      title 0x4F (bright white on red)
║ F1    Help           ║      key caps "F1".."Esc" 0x70 (black on grey)
║ F2    NotePad        ║      item text 0x03 (cyan), hot letter 0x0B (lt cyan)
║ F3    Calculator     ║      selection bar 0x17 / hot letter 0x1F (blue bar)
║ F4    caLendar       ║
║ F5    Dialer         ║
║ F6    Ascii-table    ║
║ F7    Setup          ║
║ Esc   exit           ║
╚══════════════════════╝
 ↑-↓-move bar. Select by pressing a highlighted letter, a function key, or ◄─┘   (row 24)
```
- **F1..F7 + Esc, not F1..F8.** HB15 Fig. 2-1 (p.13) and the 1.56A capture agree: F1
  Help, F2 NotePad, F3 Calculator, F4 caLendar, F5 Dialer, F6 Ascii-table, F7 Setup, Esc
  exit. "F1 through F7 on a full system" (HB15 p.14). The bead text's "F1..F8" is not
  period-correct.
- **Three ways to choose** (HB15 pp.13-14): the highlighted letter (upper or lower case,
  or with Alt held), the function key, or the bar plus Enter. **Alt-letter works from
  inside any window** and is the recommended way to stack windows: Alt-H Help, Alt-N
  Notepad, Alt-C Calculator, Alt-L caLendar, Alt-D Dialer, Alt-A ASCII, Alt-S Setup
  (HB15 p.23; corrected for Help to Alt-H by RM156).
- **Esc** closes the current window and returns to the previous one; from the last one
  back to the program (HB15 p.14). MEASURED (1.56A): Esc from Help opened directly from
  the menu returns to the main menu, not to DOS (CAP `vga-03b-help-esc-returns-to-menu`), which differs from
  HB15 p.15's "Esc will return you to the outside world".
- **Window stack.** Several windows open at once; re-entering an already-open window
  moves it to the top and reorders the Esc chain (HB15 pp.34-35, Figs. 2-8, 2-9). MEASURED:
  six windows open simultaneously, each saving what is under it (CAP
  `vga-12-setup.png`; CGA/MDA `*-six-windows.png`).
- **Moving windows:** Scroll Lock on, then the arrow keys move the current window; a
  `ScrollLock` flag shows bottom right (HB15 p.15). Positions persist only via Setup F3
  "save window setup" (HB15 p.86).
- **The command line** is screen row 24, always present while SideKick is up, listing the
  keys valid in the current window (HB15 p.16). Its colours double as the menu's selection
  bar (HB15 p.101). MEASURED attributes: background text 0x17, key caps 0x70, emphasis
  0x1F; lock-state flags (`NumLock`, `ScrollLock`) as 0x70 caps at the right end (CAP
  `vga-06-calculator-dec.txt` row 24).

### 2.5 Help

Context-sensitive: F1 in any window gives that window's help, from `SK.HLP` on the start
directory (HB15 p.14, p.40). MEASURED (CAP `vga-03-help.txt`): window cols 0-53, rows
0-11, single-line frame 0x70, body 0x20 (black on green), title "SIDEKICK HELP SYSTEM"
0x2F, bottom border caption `◄ Press Esc to exit help ►`, command line
`↓-next page ↑-previous page Esc-exit`.

### 2.6 Notepad

- **Editor:** full-screen, "completely compatible with the TURBO Pascal editor, and almost
  identical to WordStar" (HB15 p.16, p.39). 51 installable commands, each with a fixed
  WordStar secondary and an installable primary (HB15 pp.44, 95-96). The complete key
  table is HB15 Table B-1 (pp.97-98); the essentials (read off the page images, the OCR
  loses keycaps):

  | Action | WordStar key | PC key |
  |---|---|---|
  | char left / right | ^S / ^D | Left / Right |
  | word left / right | ^A / ^F | Ctrl-Left / Ctrl-Right |
  | line up / down | ^E / ^X | Up / Down |
  | scroll up / down | ^W / ^Z | -- |
  | page up / down | ^R / ^C | PgUp / PgDn |
  | line start / end | ^Q S / ^Q D | Home / End |
  | top / bottom of window | ^Q E / ^Q X | Ctrl-Home / Ctrl-End |
  | top / end of file | ^Q R / ^Q C | Ctrl-PgUp / Ctrl-PgDn |
  | end of file + time/date stamp | ^Q O | -- |
  | block begin / end | ^K B / ^K K | F7 / F8 |
  | mark word, hide, copy, move, delete block | ^K T, ^K H, ^K C, ^K V, ^K Y | -- |
  | read / write block from/to disk | ^K R / ^K W | -- |
  | sort / print / paste block | ^K S / ^K P / ^K E | -- |
  | save note file | ^K D | (F2) |
  | insert mode, insert line, delete line, delete EOL, delete word, delete char | ^V, ^N, ^Y, ^Q Y, ^T, ^G | Ins, -, -, -, -, Del (Esc S) |
  | find, find & replace, repeat | ^Q F, ^Q A, ^L | -- |
  | auto indent, restore line, time/date stamp, graphics on/off | ^Q I, ^Q L, ^Q T, ^Q G | -- |
  | set right margin, reformat paragraph | ^O R, ^B | -- |

  The bound-in "EDITOR QUICK REFERENCE" card (HB15 PDF p.128) is the cursor-diamond
  (^E/^S/^D/^X inside, ^R/^A/^F/^C outside) plus Delete, Find (options U W B G N) and Block
  panels. 1.56A adds a per-paragraph left margin (^P Space; shown as char 160 in graphics
  mode) and word wrap honouring auto-indent (RM156 New Features 2-3; set right margin 250
  to disable wrap, RM156 5).
- **Size limit:** "The default maximum size of the note file is 4000 characters"
  (HB15 p.83, p.99, p.109). Installable in SKINST up to about 45,000 for SK.COM and 50,000
  for SKN.COM; DOS 1.0 caps it near 8,000 / 16,000 (HB15 p.109). "File too big" on an
  over-size load (HB15 p.109; STR). FS15 advertises "45K-50K (22 to 25 pages)".
- **Files:** default note file `NOTES` in the current (or Setup) directory; F3 "New note
  file" accepts a name or a wildcard pattern (pattern opens a file-picker window, HB15
  Fig. 2-4); nothing is ever saved automatically; F2 (or ^K D) saves, the old file kept as
  `.BAK` (HB15 pp.17-19, p.40). `.LOG` in column 1 line 1 makes every open append a
  time/date stamp at the end (HB15 p.36).
- **Import from screen (F4):** the Notepad vanishes, the underlying screen shows, the cursor
  marks a rectangular block (^K B ... ^K K or Esc), and ^K C copies it into the note
  (HB15 pp.19-20, 41-42). **Paste out (^K E):** assigns the marked block to a key; pressing
  that key in another program types the block, as a whole or line by line; Setup's "paste
  delay factor" 0-999 throttles the ~1000 chars/s feed (HB15 pp.21-22, p.83).
- **Window:** opens at the bottom of the screen; F9 expand (max 23 lines x 78 chars), F10
  contract (min 1 line x 40) (HB15 p.42). MEASURED (CAP `vga-04-notepad.txt`): cols 0-79,
  rows 10-23, double-line frame 0x2F (bright white on green), text 0x0E (yellow on black),
  the **status line is drawn in the top frame row**:
  ```
  ╔     B:\NOTES.                       Line 1    Col 1   Insert    Indent       ╗
  ║                                                                              ║
  ...
  ╚══════════════════════════════════════════════════════════════════════════════╝
   F1-help F2-save F3-new file F4-import data F9-expand F10-contract Esc-exit       (row 24)
  ```
  Status fields: path/name, `Line n`, `Col n`, `Insert`/`Overwrite`, `Indent`, `Graph`
  (HB15 p.43, Fig. 3-2). "Graph" shows chars 128-255 as IBM graphics; off by default so
  WordStar high-bit files read as text (HB15 p.43).

### 2.7 Calculator (no paper tape)

- BCD arithmetic, **18 digits with 4 decimals**, range +/-99,999,999,999,999.9999; binary
  range 20 digits, hex range 12 digits; four operations, parentheses six levels deep,
  And/Or/Xor; memory (M then C/R/=/+/-/*//); NumLock forced on while the window is up and
  restored after (HB15 pp.24-25, 69-70; FS15 p.2).
- **Paste:** P then a key "programs" that key to type the displayed number into any
  application until cleared (P then C) (HB15 pp.25-26, 70-71; RM156 Q&A).
- **There is no paper tape** in 1.5x: neither HB15, HB10, FS15, RM156 nor the 1.56A
  screen has one. The handbook's cover art shows a printing calculator (HB15 PDF p.1);
  the software's calculator is a display plus memory. The bead text's "paper-tape" is
  not period-correct for 1.5x (Sec. 6, D-1).
- MEASURED (CAP `vga-06-calculator-dec.txt`, cols 51-79 rows 0-14, frame 0x6F bright white
  on brown, key caps 0x70, labels 0x60):
  ```
  ╔═══════════════════════════╗
  ║ ╔═══════════════════════╗ ║
  ║ ║              14.0000  ║ ║      after "2*(3+4)="
  ║ ║ Dec                   ║ ║      mode flag: Dec / Hex / Bin
  ║ ╚═══════════════════════╝ ║
  ║─ Hex ─┬─ Mem ─┬─ Numeric ─║
  ║       │   R C │ = 7 8 9 - ║
  ║ A  B  │ M + - │           ║
  ║ F5 F6 │   * / │ / 4 5 6   ║
  ║ C  D  ├ Modes ┤           ║
  ║ F7 F8 │ Dec   │ * 1 2 3 + ║
  ║ E  F  │ Bin   │           ║
  ║ F9 F10│ Hex   │  0   .    ║
  ║───────┴───────┼───────────║
  ║ And  Or  Xor  │  C  CE  ← ║
   F1-help P-paste Esc-exit                                NumLock          (row 24)
  ```
  Hex digits A-F are F5-F10; `H`/`B`/`D` switch mode and convert (255 then H shows `FF`,
  then B shows `11111111`: CAP `vga-07-calculator-hex`, `vga-08-calculator-bin`).

### 2.8 Calendar and Appointment book

- Perpetual calendar 1901-2099; Left/Right month, Up/Down year; today highlighted; opens
  on the PC's date (HB15 pp.27, 73). MEASURED (CAP `vga-09-calendar.txt`, cols 0-28 rows
  0-16): frame 0x09 (light blue), grid and current-month days 0x0E, adjacent-month days
  0x07, today 0x09:
  ```
  ╔═══════════════════════════╗
  ║   Oct      20     2026    ║
  ║───┬───┬───┬───┬───┬───┬───║
  ║Sun│Mon│Tue│Wed│Thu│Fri│Sat║
  ║───┼───┼───┼───┼───┼───┼───║
  ║ 27│ 28│ 29│ 30│  1│  2│  3║      (six week rows, each separated by ───┼───)
  ...
  ╚═══════════════════════════╝
   F1-help  Select: ←-→-month ↑-↓-year Appointments: enter date, hit ◄─┘ Esc-exit
  ```
  (The date is the emulator host's clock on the capture day; the artifact must take its
  date from the InitechDOS clock, Rule 11.)
- **Appointment book:** Enter on a date opens the day: a top "Title" line then half-hour
  slots 8:00 a.m. through 8:30 p.m. on two pages (PgUp/PgDn), Left/Right change day
  (HB15 pp.27, 73). One file per person, `<NAME>.APP` (name max 8 letters/digits) in the
  root unless Setup says otherwise (HB15 pp.27, 73, 84). Print one day or a range (a form
  feed per day, a full day needs 28 lines), print all, delete a range (HB15 pp.28, 73-74).
  **1.56A key change:** F2 Print, F3 Name (HB15 had F2 Name, F3 Print; RM156 item 4 and
  "Page 27-28"/"Page 73" corrections). MEASURED (CAP `vga-10-appointment.txt`, cols 3-38
  rows 3-20, single-line frame 0x1B, title `Name: B:APPOINT.APP` 0x1F, body 0x70):
  ```
  ┌Name: B:APPOINT.APP               ┐
  │   Oct      20     2026           │
  │──────────────────────────────────│
  │Title                             │
  │08:00a                            │
  │08:30a                            │
  ...
  └──────────────────────────────────┘
   F1-help F2-print F3-name F4-print all F5-delete ←-→-day ↑-↓-line Esc-exit
  ```

### 2.9 Dialer / phone directory

- Hayes 1200/1200B (or compatible), PCjr modem, VOAD Keyboard Phone, AT&T 4000; COM1/COM2
  set in SKINST (HB15 pp.29, 75, 99-100; CAP `skinst-dialer.txt`).
- **Picks a number off the screen** of the interrupted program: a number qualifies by a
  minimum digit count (default 6) and at least one "required character" (default `-`,
  `(`, `)`), and must not contain commas, periods or slashes; Right arrow moves to the next
  candidate, Space goes to the directory instead (HB15 pp.30, 75, 99). No candidate on
  screen loads the directory file `PHONE.DIR` (HB15 p.30).
- **Directory file:** plain text, one entry per CR/LF line `INITIALS PHONENUMBER COMMENT`,
  78 characters shown, `T`/`P` tone/pulse, `@` pause (HB15 pp.75-76); max 64K, ~1500
  names (FS15). **1.56A keys:** F3 new file, F5 search INITials, F6 search all, F7 stop
  search (HB15 had F2/F3/F4/F5; RM156 item 4). Without a modem it is a "Rolodex" (HB15
  p.31).
- MEASURED (CAP `vga-11-dialer.txt`): full width cols 0-79, rows 15-23, double frame 0x09,
  body 0x31 (blue on cyan), the bar 0x1E (yellow on blue); command line
  `F1-help F3-New file <─┘-dial ↓↑-scroll search: F5-INIT F6-all F7-stop Esc-exit`.

### 2.10 ASCII table

All 256 characters; the first two pages 16 per page with decimal, hex, the glyph, the
control-key name and mnemonic; the rest 32 per page (two columns) with decimal, hex,
glyph; arrows page (HB15 pp.32, 79). MEASURED (CAP `ascii-p1..p9.txt`, cols 1-23 rows
1-18, 9 pages, wraps): frame 0x1F (bright white on blue), body 0x0E, glyph cells
alternate 0x0E / 0x0B by row:
```
╔  D  H Ch   Ctrl Mem ╗            ╔  D  H Ch    D  H Ch ╗
║  0 00       ^@  NUL ║            ║ 32 20 SP │ 48 30  0 ║
║  1 01  ☺    ^A  SOH ║            ║ 33 21  ! │ 49 31  1 ║
...                                ...
║ 15 0F  ☼    ^O  SI  ║            ║ 47 2F  / │ 63 3F  ? ║
╚═════════════════════╝            ╚═════════════════════╝
 F1-help ←-↑-previous page →-↓-next page Esc-exit
```
These nine pages are also the glyph dictionary the cell transcriptions were decoded with
(PROVENANCE Sec. 3) -- and a ready-made CP437 golden for the bead's "ASCII table vs code
page 437" oracle.

### 2.11 Setup

Alt-S. Note file name/directory, paste delay factor, size (display only, "Set with
SKINST"), appointment file last name (`APP`), first name (`APPOINT`), directory, phone
directory name (`PHONE.DIR`) and directory; F2 save file setup, F3 save window setup, F4
save both -- **saved into the SK `.COM` file itself**, which must be in the start
directory (HB15 pp.81-86). MEASURED (CAP `vga-12-setup.txt`, cols 18-60 rows 6-19,
single frame 0x4F with `Version 1.56A` in the top border, body 0x70 / values 0x7F):
```
┌───────────────────────────Version 1.56A─┐
│ Notefile                                │
│    Name:      NOTES                     │
│    Directory:                           │
│    Paste delay factor: 0                │
│    Size: 4000 chars (Set with SKINST)   │
│ Appointment calendar                    │
│    Last name: APP                       │
│    Name:      APPOINT                   │
│    Directory: \                         │
│ Telephone directory                     │
│    Name:      PHONE.DIR                 │
│    Directory: \                         │
└─────────────────────────────────────────┘
 F1-help F2-save file setup F3-save window setup F4-save both ESC-exit
```

### 2.12 SKINST (the installer)

1.56A main menu (CAP `skinst-main.txt`; AST/OBS screenshots agree): "Installation program
for SideKick 1.50" with items Screen type, notepad Commands, Notepad size, right Margin,
Dialer, cOlors, Activate commands, Quit. Screen types: 0 Default (whatever mode is active
at load; always 80x25, colour if graphics was active), 1 Monochrome (incl. Hercules), 2
Colour 80x25, 3 B/W 80x25; then "Does your screen blink when the text scrolls?" and "Do
you want to use Sidekick on top of graphics?" (CAP `skinst-screen.txt`; HB15 pp.94-95;
RM156 New Features 1). Colours: per window foreground, background and frame from a
1-127 table (+128 blinks) with a live sample (HB15 pp.100-101, Fig. B-3). SKINST writes
every SK*.COM on the logged drive; the running copy is unchanged until reloaded (HB15
p.93, p.110).

---

## 3. Screen save/restore and the exact look

### 3.1 Text mode, 80x25, always

- SideKick always uses 80x25 character mode, even over 40x25; over graphics it switches the
  adapter to text mode, so the graphics screen turns into "a pattern of (possibly blinking)
  characters" until SideKick exits, when "the screen will return to graphics" (HB15 p.94,
  p.110; RM156 New Features 1). 1.56A can instead BLANK the screen over graphics and
  restore it on exit, an install option that "takes up 4K of RAM" (RM156 1; CAP
  `skinst-screen.txt` row 16).
- **Save-under, byte-exact.** Every window covers part of the screen "but everything is
  still present underneath it" (HB15 p.2). MEASURED: the DOS screen before pop-up
  (`vga-01-foreground`) and after leaving SideKick with six windows stacked
  (`vga-13-restored-after-ctrlalt`) differ in exactly one cell, (col 2, row 19) -- the
  blinking hardware cursor's phase; all 1999 other cells, characters and attributes, are
  identical. A full 80x25 text screen is 80*25*2 = **4000 bytes** (char + attribute per
  cell) -- the size of the save-under the bead calls "the 4000-byte save". (It is a
  coincidence of numbers that the default note buffer is also 4000 characters.)
- **Reads the foreground screen.** Notepad F4 import and the Dialer's number pick both read
  the interrupted program's character cells (HB15 pp.19, 30) -- SideKick needs read access
  to the text cells, not just a pixel save.

### 3.2 Box-drawing characters (CP437) used

| Use | Characters (CP437 code) |
|---|---|
| Double frame (main menu, Notepad, Calculator, Calendar, Dialer, ASCII) | ╔ C9, ═ CD, ╗ BB, ║ BA, ╚ C8, ╝ BC |
| Single frame (Help, Appointment, Setup) | ┌ DA, ─ C4, ┐ BF, │ B3, └ C0, ┘ D9 |
| Log-on box (double top/bottom, single sides) | ╒ D5, ╕ B8, ╘ D4, ╛ BE, ═ CD, │ B3 |
| Calculator / calendar dividers | ┬ C2, ┴ C1, ┼ C5, ├ C3, ┤ B4 |
| Command-line glyphs | ↑ 18, ↓ 19, ← 1B, → 1A, ◄ 11, ► 10, Enter key as ◄─┘ (11 C4 D9) |

Title text sits IN the top border row (main menu, Notepad status line, Appointment
`Name:`, Setup `Version`), padded with blanks; Help puts its caption in the bottom border.

### 3.3 Colours -- default attributes (colour adapter, MEASURED VGA and CGA)

| Element | Attribute | Meaning |
|---|---|---|
| Main menu frame / title / items / hot letter | 0x0F / 0x4F / 0x03 / 0x0B | white on black / white on red / cyan / lt cyan |
| Selection bar, command line, emphasis | 0x17, 0x1F | grey / white on blue |
| Key caps (menu, command line, calculator) | 0x70 | black on grey |
| Help frame / body / title | 0x70 / 0x20 / 0x2F | grey / green |
| Notepad frame+status / text | 0x2F / 0x0E | white on green / yellow on black |
| Calculator frame / labels | 0x6F / 0x60 | white on brown / black on brown |
| Calendar frame / grid / today / other month | 0x09 / 0x0E / 0x09 / 0x07 | |
| Appointment frame / title / body | 0x1B / 0x1F / 0x70 | |
| Dialer frame / body / bar | 0x09 / 0x31 / 0x1E | |
| ASCII frame / body / alt rows | 0x1F / 0x0E / 0x0B | |
| Setup frame / labels / values | 0x4F / 0x70 / 0x7F | |
| Log-on box frame / text | 0x0F / 0x1B | |

CGA (`machine=cga`) shows the identical attribute set (CAP `cga-main-menu.png`,
`cga-six-windows.png`). All are user-changeable in SKINST (Sec. 2.12); these are the
shipped defaults on the 1.56A retail disk.

### 3.4 Monochrome

MEASURED on `machine=mda` (CAP `mda-main-menu.png`, `mda-six-windows.png`): the same
layout; title bars and key caps render reverse video, the menu selection is shown
underlined, hot letters and emphasis bright, frames normal intensity. The B/W (composite
on a colour card) and Monochrome install choices exist precisely because colours collapse
to shades on such displays (HB15 pp.94-95, 109). The mono attribute byte values were not
transcribed (the 9x14 MDA cell decoder was not built); the PNGs are the record.

### 3.5 Interrupt vectors SideKick takes over (MEASURED)

DEBUG `D 0:0 L C0` before and after `SK` under MS-DOS 3.30 (CAP `ivt-before-sk.txt`,
`ivt-after-sk.txt`). SideKick's resident segment was 0D6D; the vectors it re-pointed:

| Vector | Before | After | Role (period DOS/BIOS meaning) |
|---|---|---|---|
| 08h | 0BBC:00AA | 0D6D:0907 | IRQ0 timer tick |
| 09h | 0BBC:0124 | 0D6D:0A97 | IRQ1 keyboard (the hotkey watcher) |
| 10h | C000:00EE | 0D6D:1128 | BIOS video |
| 13h | 0070:1DB1 | 0D6D:0A23 | BIOS disk (busy flag: never pop mid-disk-I/O) |
| 16h | F000:CFA0 | 0D6D:0ABE | BIOS keyboard (paste-key injection) |
| 1Ch | F000:CBE0 | 0D6D:08E0 | user timer tick |
| 21h | 0274:1460 | 0D6D:0E72 | DOS function dispatcher |
| 25h / 26h | 0274:15DC / 0274:161F | 0D6D:0EA9 / 0D6D:0EBF | DOS absolute disk read / write |
| 28h | 0274:1445 | 0D6D:09FE | DOS idle (safe pop-up while DOS waits for input) |

Unchanged: 2Fh (multiplex) and the rest of 00h-2Fh. (22h-24h differ only because the two
DEBUG instances had different PSPs.) This is the period answer to "which hooks"; the roles
in the last column are the standard DOS/BIOS meanings (DOS 3.3 internals), the handler
bodies were not disassembled.

---

## 4. Transcription index

Every committed capture with its `.txt` (char + attribute per cell) is listed with sha256 in
PROVENANCE Sec. 3. The ones a later lane will want first: `vga-02-main-menu`,
`vga-04-notepad`, `vga-06-calculator-dec`, `vga-09-calendar`, `vga-10-appointment`,
`vga-11-dialer`, `ascii-p1..p9`, `vga-12-setup`, `vga-03-help`, `vga-00-logon`,
`vga-01-foreground` + `vga-13-restored-after-ctrlalt` (the restore pair).

---

## 5. What InitechDOS must grow

Read from `os/milton` at lane base 546e5bc2. "Have" = present and gated today; "Partial" =
present but not enough for a SideKick; "Missing" = absent. The kernel lane
(initech-6fdt sub-deliverable 2, lane `sidekick`) owns all of it; nothing here is decided.

| SideKick behaviour | Period DOS mechanic | InitechDOS today (SRC) | Status / gap |
|---|---|---|---|
| Stay resident after load (Sec. 2.2) | INT 21h AH=31h KEEP, DX = paragraphs kept from the PSP | `do_keep` `os/milton/int21.c:4732`, dispatch `:5536`; `mcb_keep_resident` `os/milton/mcb.c:346` re-owns the PSP's first arena block to `MCB_OWNER_SYSTEM` (`os/milton/mcb.h:41-50`); host oracle `test_keep.c` + mutant `MCB_MUTATE_KEEP_NO_RESIDENT` | **Partial.** The MCB bookkeeping is there, but the program IMAGE is not an arena block: every flat `.COM` loads at the fixed `PROGRAM_IMAGE` 0x40100 (`spec/memory_map.h:172`) and the arena is DISJOINT above image+BSS (`os/milton/int21.c:576-583`). The next EXEC overwrites a "kept" TSR's code. The resident part must live in a kept arena block (a relocated copy, or an InitechMZ load at a parameterized base as ADR-0013 AC-3 does for FLAIR tenants). |
| Hook the hotkey (INT 09h) and read shift state | `AH=35h`/`AH=25h` get/set vector 09h; read 0040:0017 shift flags; chain to old handler | AH=25h/35h exist (`do_setvect` `:5099`, `do_getvect` `:5113`) but bind to `idt_install_trap(vec, ...)` for ANY vector (`os/milton/kmain.c:549-552`). Keyboard IRQ1 is vector **0x29**, not 09h (PIC master base 0x28, ADR-0003 DEC-04a.2 / `os/milton/pic.c:39`, `os/milton/sysinit.c:519`); vector 09h is CPU exception 9. Only hook seam: `kbd_set_scancode_hook` (`os/milton/kbd.c:295`), single slot, owned by FLAIR. Modifier tracking: Shift L/R and Ctrl L/R only (`os/milton/kbd.h:78-81`); **no Alt, NumLock or ScrollLock state**. | **Missing.** Needs a chainable keyboard hook (or a documented INT 09h-equivalent vector outside 0x00-0x1F and 0x28-0x37), Alt make/break (0x38), NumLock/ScrollLock state, and a SETVECT that refuses or remaps exception/IRQ vectors instead of overwriting their gates with a trap gate (an IRQ gate replaced by a 0x8F trap gate loses IF-clear and EOI). |
| Keys SideKick itself reads: F1-F10, arrows, PgUp/PgDn, Home/End, Ctrl-Home-End, Alt-letters, Ins/Del | INT 16h AH=00h/01h, extended keys as 00h + scan | CON delivers 00h+scan only for F1-F5, Ins, Del and Left (`kbd_scancode_put` `os/milton/kbd.c:254-262`) | **Partial.** F6-F10, Up/Down/Right, PgUp/PgDn, Home/End, Ctrl- and Alt- combinations are dropped. No INT 16h. |
| Paste a number/block into the foreground program (Sec. 2.6, 2.7) | hook INT 16h, feed keystrokes when the app reads the keyboard | no INT 16h; the DOS read path is `g_conin_get`/`g_conin_poll` (`os/milton/int21.c:58-64`) | **Missing.** Needs an injection seam on the CON read path. |
| Pop up only when safe (Sec. 2.3) | InDOS flag (AH=34h), critical-error flag, INT 28h idle hook (DOS calls 28h while waiting in AH=01h-0Ch), INT 13h busy | `g_indos` + `dos_in_dos()` `os/milton/int21.c:283-302`, bracketing `:5639-5652`, documented as "the future hook a deferring TSR checks" (`os/milton/int21.h:868-876`); IRQ-context guard fails loud if an ISR enters INT 21h (`os/milton/irq.h:1-35`). Blocking CON read "spins on hlt" (`os/milton/int21.c:58-59`). No AH=34h in `spec/int21h_register.json`. **Vector 0x28 is the PIT IRQ0 gate** (`os/milton/sysinit.c:518`), and DEC-04a.2 reserves 0x28-0x37 for IRQs ("shall not be used for any software-interrupt gate"). | **Partial.** The InDOS counter exists but is kernel-internal (no AH=34h pointer), there is no critical-error flag export, and the period idle vector number is taken by IRQ0. An idle call-out from the CON wait loop (the INT 28h equivalent) must be invented under another vector number or a kernel seam -- an ADR-level decision (DEC-04a C-1 binds the IRQ range). |
| Notepad/Appointment save through DOS while popped up | INT 21h file calls from the pop-up context, after switching to SideKick's PSP (AH=50h/51h) and DTA | INT 21h works from task context; AH=50h/51h (set/get PSP) are NOT in the register (only 62h GETPSP); calling DOS from IRQ context panics by design (`os/milton/irq.h`) | **Missing.** The pop-up must run in task context (deferred to the idle point), and PSP switching is needed so the TSR's file handles are its own. |
| Clock in calendar / time stamps; window timers | INT 08h / 1Ch tick (18.2 Hz) | PIT at **100 Hz** (`os/milton/pit.h:32`); single-slot `pit_set_tick_hook` owned by FLAIR (`os/milton/pit.c:50-61`); IRQ0 at vector 0x28; DOS time via AH=2Ch (RTC) | **Partial.** A chainable tick hook is needed; any period "18.2 ticks/s" constant in a port must be re-derived from 100 Hz. Date/time for stamps already via AH=2Ah/2Ch. |
| Save-under and restore of the 80x25 text screen (Sec. 3.1) | read/write B800:0000 (4000 bytes), INT 10h AH=03h/02h cursor, AH=0Fh mode | `console_t` draws into a 640x480 LFB with the 8x16 ROM font (`os/milton/console.c:1-12`, `os/milton/console.h:25-90`); it keeps cursor and current attribute but **no character/attribute cell array**; no INT 10h | **Missing.** A 80x25 cell shadow (char + attribute, 4000 bytes) is needed for (a) save-under, (b) Notepad import and Dialer pick-off-screen, (c) a byte-exact before/after screen oracle. Pixel save of a 640x400 region is possible but cannot serve (b). |
| Over graphics: switch to text and back (Sec. 3.1) | INT 10h AH=00h set mode, save/restore | FLAIR owns the LFB in the desktop kernels; Exit-to-DOS shell is text on LFB | **Decision.** Period behaviour over graphics was "modern art" or blank (4K option); over FLAIR the bead wants a pop-up -- the house must choose (Sec. 6, D-5). |
| Install check ("already active") | period SideKick checked its own hooked vector; INT 2Fh multiplex was NOT hooked (Sec. 3.5) | vector 0x2F is IRQ7 in the remapped PIC (DEC-04a.2) | **Note.** Do not use INT 2Fh for an install check; a signature at the hooked entry or a kernel query fits the period. |
| Uninstall (Ctrl-Home-End) only if last loaded | restore vectors, free the kept block (AH=49h) | AH=49h FREE exists; `MCB_OWNER_SYSTEM` blocks are skipped by owner frees (`os/milton/mcb.h:44-50`) | **Partial.** Freeing a SYSTEM-owned kept block needs an explicit path. |

---

## 6. Deviations from the bead text, and decisions for the operator

- **D-1 Paper tape.** Not in SideKick 1.5x (Sec. 2.7). Options: (a) period-exact, no tape;
  (b) a tape as a declared Initech addition (HOUSE), recorded in the VO-30 register of
  tolerated inconsistencies. Recommendation: (a).
- **D-2 F1..F8.** The main menu is F1-F7 + Esc (Sec. 2.4). Recommendation: period-exact.
- **D-3 SK.CFG.** The period stored setup INSIDE `SK.COM` (Setup save, SKINST patch),
  with no command-line options. A self-modifying `.COM` interacts with the reproducible
  image (Rule 11: a shipped `SK.COM` must stay byte-stable in the image; a user's save then
  changes it on the data volume). Options: (a) period-exact self-patch of the `.COM` on the
  writable volume; (b) a separate `SK.CFG` (HOUSE). Operator call.
- **D-4 The 4000-byte numbers.** Default note buffer 4000 characters (HB15 p.83) and the
  80x25 save-under 4000 bytes are two different things; both are period-true.
- **D-5 Over FLAIR / TPSREP.** The period switched a graphics screen to text and back (or
  blanked it). The bead wants SideKick over FLAIR's Exit-to-DOS shell (which is text on the
  LFB, so ordinary) and over TPSREP; popping over the live FLAIR desktop has no period
  precedent and needs a ruling.
- **D-6 Activation rule.** Period: modifier-only chords (Ctrl/Alt/LeftShift/RightShift),
  firing on release, re-definable. Recommendation: lock exactly that, with Ctrl-Alt and
  LeftShift-RightShift as the defaults, as spec data before code (Rule 8).

---

## 7. Open risks

- **KEEP that keeps nothing.** Until the resident image survives the next EXEC (Sec. 5,
  row 1) any TSR test that only checks MCB ownership is green on a dead TSR. The kernel
  oracle must call the resident code after another program has been loaded and run.
- **Vector-number collisions are structural** (0x08, 0x09, 0x10, 0x16, 0x1C are CPU
  exception slots; 0x28 and 0x2F are IRQ slots). Porting "INT 9 / INT 28h" literally will
  either clobber an exception gate or an IRQ gate through today's unchecked SETVECT. This
  is an ADR-0003 DEC-04a-level question, not a lane-local one.
- **Shared single-slot hooks.** `kbd_set_scancode_hook` and `pit_set_tick_hook` are owned by
  FLAIR's event loop; a TSR chain must not displace it (the "pop over FLAIR" case).
- **Golden independence (Law 2).** The calculator oracle must not reuse the artifact's BCD
  code to compute expectations; `bc` with scale=4 or the real SK 1.56A under this same
  DOSBox-X + MS-DOS 3.30 rig (now proven to run it) are independent goldens. The ASCII
  pages here are an independent CP437 golden; calendar maths vs `cal(1)`.
- **Emulator only.** The captures are DOSBox-X. A Bochs/86Box confirmation of the restore
  behaviour and the mono attributes would close the Rule-5 gap for the look.
