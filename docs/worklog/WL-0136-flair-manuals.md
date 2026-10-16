# WL-0136 -- The FLAIR manuals: User's Guide, Getting Started, Accessories Guide, Quick Reference

**Issuing Body:** Initech Systems Corporation -- Publications Department
**Document Class:** Worklog Shard (lane record)
**Programme:** STAPLER -- Initech Publications (bead `initech-ipu5.11`, lane `lane/flairman`)

## Context

One-shot writing and gating lane for the FLAIR desktop books of the
Documentation Set (PUB-REF-0001 Sec 12): FLR-UG (1031-0000-10), FLR-GS
(1031-0000-11), FLR-ACC (1031-0000-12) and FLR-QR (1031-0000-20). Family
F-MAC (Sec 2): the 1991 Macintosh User's Guide Learning/Reference split on
the Mac OS 8 Platinum grid; What's New in System 7 feature topics for the
Accessories Guide; the Mac OS 8 Installation manual for Getting Started
(docs/pubs/originals/apple-macintosh.md). Binding rules: AN-14, AN-20..24,
NT-13/14/20..23, GC-01..GC-04, D-3, D-14. Operator directive of
2026-10-17 (via the coordinator): the slogan "Is This Good for the
COMPANY?" on every verso under the edition notice, every back cover, the
first page of the operations chapter (FLR-GS Ch. 2); interim
`\providecommand{\IsThisGoodForTheCompany}` in each main.tex (a `\par`
added after the rule, so the next paragraph does not run on to the rule's
line); the class definition wins once the design lane merges it. The
Reader's Comment Form foot is the class's (not touched here).

The bead could not be read: `bd` reports the embedded database locked by
another process, and the worktree's issues.jsonl does not contain the id;
the lane brief is the authority.

## What changed

Toolchain (all new files; existing gates only extended):

- `harness/pubs/flair_inventory.c` -- the SHIPPED FLAIR surface read out of
  source (GC-01): includes os/flair/finder_menu.c whole (the static
  predicate map: items lit by a predicate are `cond`), finder_cmd.c,
  apple_menu.c (every accessory whose program is in FLAIR_FIXTURES),
  shell_menus.h x kmain.c `fd_sys_titles` (the upper bar), HELLO's titles
  from menu_canon.h and its item pair from kmain.c, the NOTES bar from
  kmain.c (kmain.c preprocessed with the flagship kernel's -D set), the
  TENANTFX bar from the assembler's MINFO/MITEM/MDIV macros, and the
  INITECH volume (FLAIR_FIXTURES). Line form
  `CTX>Menu>Item>Key>State|menu`, `APPS\X.EXE|file`.
- `harness/pubs/flair_app_menus.c` -- per C disk application (CALC,
  CALCULAT, CTENANT, TEXT, PAINT): includes the application's own source
  and walks the `mbar` it hands SETMBAR (the int21_inventory single-TU
  pattern; --gc-sections drops the Toolbox traps).
- `harness/pubs/flair_alerts.sh` -- every alert's first line (or constant
  leading words) quoted from its call site in kmain.c, finder_windows.c,
  dialog.h, itext.c; runs of spaces collapsed (TeX reads them as one).
- `harness/pubs/flair_grey.c` -- NT-21/22: QEMU screendump (P6) -> crop ->
  grey through the locked palettes (every `"rgb"/"hex"` of clut.json and
  color_canon.json; an off-palette pixel is RED) -> an ASCII run-length
  listing `.grey` (committed; the publications tree stays ASCII for GC-05
  and the build's ASCII check) -> `png` mode: a deflate PNG written in C (no
  zlib) into build/pubs/gen/flair/<book>/, printed at 3 x 3 or 4 x 4 on the
  600 dpi grid, interpolation off.
- `pubs/screens/flair_capture.sh` -- FLAIR capture route: per row of
  `screens/FLAIR` (`name|marker[#N]|crop[|proves[|image]]`) boot the
  record image with transient (FLAIRTENANTS_RECORDDBL_EXITDOS_IMG: shipped
  kernel, 3000-tick pump), a fresh copy of the shipped data volume, the
  locked trace `screens/<name>.mouse`, screendump at the marker
  (FLAIR-CURSOR x= y= is printed after the LFB draw; FLAIR-MENU-DROP is
  captured with the button held); `proves` lists operation markers that
  must reach serial (RED otherwise); `image` = `step` (the record image
  whose FILE COPY bar steps per event, for the mid-copy figure) or `nodisk`;
  crop `text` decodes the InitechDOS console through harness/console_text
  into `.console`. RTC pinned to 1992-03-02 09:30 (wall-paced: under
  --rtc-deterministic an application launch spends the pump's life in
  virtual time).
- `harness/pubs/check_flair.sh` -- SCREEN-PAIR / SCREEN-UNREPLAYED /
  SCREEN-UNPRINTED for FLAIR figures; OPERATION-UNSHIPPED (marker not in
  os/) and OPERATION-UNPROVED (no capture settles on it or proves it).
- `harness/pubs/flair_mutants.sh` -- 14 named Rule-6 mutants (below).
- `pubs/class/initech-flair.sty` -- \FlairScreen, \FlairText, \FlairInline,
  \CmdKey (the "^N" the Menu Manager draws), AN-14 \MenuItemEntry /
  MenuReference, \RegisterMenuItem, \VolumeFile, \AlertEntry, \Operation
  (all through initech-syntax's \RegisterCommand), F-MAC TaskSteps with
  grey step squares, MacNote/MacTip/MacWarning, SummaryTable, Symptom /
  Cause / Solution, GlossaryTerm, \FlairPart, F-MAC continuous arabic
  folios (\FlairMainMatter) and "Chapter N: Title" feet only on numbered
  chapters, BackChapter, a 3-column AN-24 index, \BackCover (PK-05 with the
  slogan), \tm/\reg.
- `pubs/flair.mk` (included from pubs.mk): pubs-flair-inventory,
  pubs-flair-gen (a prerequisite of pubs / test-pubs-build / -repro /
  -repro-mutant), test-pubs-flair (joined to test-pubs), pubs-flair-screens,
  test-pubs-flair-screens (joined to test-pubs-screens),
  test-pubs-flair-mutant (joined to test-pubs-mutant).
- `harness/pubs/pubs_gate.c`: four more KINDs accepted in a .cmds line
  (menu, file, alert, operation); nothing else changed.
- `pubs/pubs.mk`: the DOS gate's "other books must be empty" loop skips
  the FLAIR books' main.cmds (they have their own inventory and gate); their
  .mentions stay checked by it.
- `harness/pubs/flair.allow`: new, EMPTY (no shipped FLAIR item is
  undocumented).

Books (pubs/manuals/flair-*): see Acceptance for extents and counts.

## Why

GC-01 for a desktop means menus, keyboard equivalents, enable state,
alerts and the files of the volume, all of which are data in the shipped
source; the inventory reads them there so a change in finder_menu.c or an
application's bar turns test-pubs red until the book follows (mutant
SOURCE_KEY_DRIFT proves it). Desktop operations have no table: they are
proved by replay -- every \Operation names the serial marker the shell
prints, and some locked trace must make the running desktop print it.

## Frictions / defects found (beads are out of this lane's scope; each wants one)

1. InitechCalc ends an entry only on CR; the PS/2 table cooks Enter to LF
   (os/flair/event.c), so Enter does not place an entry (calc.c). DRAFT in
   FLR-ACC Ch. 4.
2. InitechCalc's ledger shows the placeholder text TODO_FRAME (three
   amounts and the empty entry line) on screen: calc_ledger.json is not
   locked. DRAFT in FLR-ACC Ch. 4.
3. HELLO's and NOTES' menu items are enabled but act on nothing (band-2
   choices reach only a disk tenant); the upper bar's Quit is enabled and a
   no-op. 29 DRAFT entries (FLR-UG Ch. 8).
4. A HELLO/NOTES content click draws its marker into the offscreen but the
   screen is not presented until a later redraw (not documented).
5. The backslash key is taken by the SAMIR hotkey whatever the active
   application: no FLAIR application can receive a backslash; SAMIR.COM is
   not on the flagship volume (documented as shipped, FLR-UG Ch. 12).
6. build/initech.img (the shipped image) ends its pump after 250 ticks;
   manual figures use the record image (same kernel, longer pump).
7. About This Computer reports QEMU's 128 MB as "Built-in Memory: 130944K".
8. New Folder with no Finder window in front, a fifth Finder window, a
   second disk application, a document double-click: nothing visible
   (serial only). Documented as shipped behaviour.
9. FLAIR started without its data disk shows a nameless disk icon and the
   clock at 12:00 am (documented in FLR-GS troubleshooting with a capture).
10. The TRASH directory and DESKTOP.DB that FLAIR creates carry the date
    0-00-80 12:00a in a DIR listing (FLR-UG Ch. 14 figure shows it as
    captured).
11. Capture timing: the record image with transient was first the 3000-tick
    recorddbl kernel; a_text ran its pump out under host load, so the lane
    links its own kernel (build/kernel_flairtenants_pubs.elf: the
    recordxlong objects + the transient import list) and image
    (build/flair_tenants_pubs.img). The RTC is wall-paced (record-flair's
    pattern); a capture slower than a minute would show 9:31 -- under a
    very loaded host test-pubs-flair-screens can therefore fail on time, not
    content (seen once: a QEMU stall on a_textpaste; the retry took 7 s).

## Acceptance

(filled in at the end of the lane run; see below)

## Pointers

- pubs/manuals/flair-users-guide, flair-getting-started,
  flair-accessories, flair-quick-reference
- pubs/class/initech-flair.sty, pubs/flair.mk, pubs/screens/flair_capture.sh
- harness/pubs/flair_inventory.c, flair_app_menus.c, flair_alerts.sh,
  flair_grey.c, check_flair.sh, flair_mutants.sh, flair.allow
