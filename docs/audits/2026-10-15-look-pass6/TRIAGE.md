# LOOK pass 6 source triage

Audit: REPORT.md / NITS.tsv, M001-M028 (28 new measured nits), plus the
still-present ids of Sec 3. Guest images at 3aaf6e0.
Triage date: 2026-10-15. Repo: /home/tobias/Projects/initech-os, main at
commit 3c0df0a. Reference paths beginning ../ name sibling corpora under
/home/tobias/Projects (system7-decomp goldens, initech-manuals-ref manuals).
Source citations are repo-relative.

This is tracker triage of a visual audit. REPORT.md names no source lines of
its own (it says so: "a visual audit, not a source-code repair"); the
file:line pointers below were located by reading main at 3c0df0a and are
starting points, not proofs. No builds, oracle runs or commits were done.
Named oracles are acceptance work for the developer, not green claims.

D = defect in implemented behavior. F = feature never built at the reported
entry point. Tracking is a separate axis; a mapped item keeps its D/F verdict.
Per CLAUDE.md Law 2 every "done" below needs an independent golden (the
Mac captures, NFNT strikes and manual pages named in REPORT.md Sec 2), never
a value computed from the renderer's own constants. Window-system rows also
need the Rule 14 record-flair clip. Canon is untouched: both menu bars, the
teal DESKTOP, the hourglass, 116 %, 570-, the "computer" wording.

Filed beads carry label audit-2026-10-look6; desktop/window nits are children
of epic initech-tdnl. The 22 new beads are one per M-id (no batch beads; the
batches below are pickup groupings only).

## All 28 findings

Verdict | area | sev | owner | oracle / done-means (what InitechOS draws vs reference)

| ID | V | Area | Sev | Owner | Oracle / done means |
| --- | --- | --- | --- | --- | --- |
| M001 | D | menu bar | P3 | existing initech-tdnl.70 | Apple ink x=4..13, File x=28..48 vs s8_desktop_menubar.png x=0..79 y=0..19 (17..26, 43..62); 445/1600 px differ. Re-derive layout constants from specs/sys8/menus.md 1.3, pixel crop 0 diff on the non-Apple region. |
| M002 | D | menu highlight | P2 | NEW tdnl.93 | Selected File x=17..59 flat #8DDCDC (contrast 1.57:1) vs s8_menu_dropped.png x=34 y=0..19 4-shade ramp (#8787DA,#5454B3,#0000A5,black; 6.37:1). Teal-family dark face + edges; column ramp assertion, mutant, clip. |
| M003 | F | menu shortcuts | P3 | existing initech-tdnl.69 | "^N" ink x=144..157 y=43..51 vs looped Command mark + N at s8_menu_dropped.png x=199..216 y=23..31 (83/266 differ); cloverleaf glyph (commandMark 17). iqhh (cmdChar drawing) is superseded by the live ^N and is stale. |
| M004 | D | menu shortcuts | P3 | NEW tdnl.94 | Last shortcut ink to right frame 17 px vs 14 px (s8_menu_dropped.png x=209..232 y=21..34). Distance assertion on the Finder File panel. |
| M005 | D | menu text | P3 | existing initech-tdnl.70 | Item ink 21 px from the left frame vs 20 (x=33/53, y=23..31); cap top already panel+4. Chicago vs Charcoal bearings are not the cause. |
| M006 | D | menu marks | P3 | NEW tdnl.95 | by Icons mark is a 6x7 v (26 px) vs NFNT 5478 glyph 0x12: 9x8, 18 ink px, advance 11. Ink bbox + count against the independent NFNT render. |
| M007 | D | scroll thumb | P2 | NEW tdnl.96 | 14x15 thumb interior = 205 #8DDCDC + 5 #F3F3F3 vs s8_doc_window_inactive.png x=608..621 y=311..325 (49/102/26/28/5 shades); 205/210 differ. Three-shade teal-family ramp. Related tdnl.80/.82/.86. |
| M008 | D | scroll thumb | P2 | NEW tdnl.97 | Grip box x=363..369 y=100..106 has 0 dark px vs four 7-px #5454B3 lines + highlights (x=610..619 y=313..321); 86/90 differ. |
| M009 | D | pointer | P3 | NEW tdnl.98 | Arrow 61 black/53 white, bbox 9x14, squared tail vs s8_alert_modal.png x=229..244 y=28..43 (54/43, 8x14, angled); 48/256 differ. Touches locked spec/assets/cursors.h: issue + worklog (Rule 8). hq7l is older, likely stale. |
| M010 | D | title truncation | P3 | existing initech-tdnl.68 | One 2x2 period vs NFNT 0xC9 ellipsis (12 px, adv 14); FROM MEMORY and chrome_metrics.json currently selects the period, so an operator ruling comes first. |
| M011 | D | button bevel | P2 | NEW tdnl.99 | Cancel row y=160: 11 white/43 face vs 54 white; row y=175: 10 vs 53 #C0C0C0 (s8_alert_modal.png x=363..421 y=158..177). Label fill erases the bevel (control.c draw_push_button). |
| M012 | D | button corners | P3 | NEW tdnl.100 | Corner (363,158)=#C0C0C0, (364,158)=#969696 vs #E7E7E7; 13/64 px over four 4x4 patches. Independent of M011. |
| M013 | D | scroll arrows | P3 | NEW tdnl.101 | Up-arrow tile (360..373, 82..95): top-right #CDCDCD, bottom-left #FFFFFF vs #E7E7E7 (s8_doc_window_inactive.png x=608..621 y=296..309); 2/196 differ. |
| M014 | D | DOS/db console | P2 | existing initech-r8hv, initech-up1b (flag below) | #C0C0C0 on #6FA08E (contrast 1.63:1) vs dBASE III+ default white on black (U5-204, PDF 316; DOS default FROM MEMORY). Operator ruling needed. |
| M015 | D | DOS cursor | P2 | NEW rtdw | Prompt cell x=32..39 y=384..399 has 0 fg px; 14 frames over 2.5 s identical vs DOS manual p.5 (PDF 18) blinking cursor. Two timed dumps differ only in the cursor cell. |
| M016 | D | DOS CLS | P2 | NEW 3sxp | Prompt at y=384 (row 24) after CLS vs row 0 (FROM MEMORY; manual p.45 PDF 130). Cursor reads back 0,0 on emu. |
| M017 | F | DOS DIR header | P3 | NEW vjtp | Only "Directory of A:\" vs "Volume in drive A is ..." + directory line (manual p.28 PDF 41). Related f2cz, k7tp, oqiu, f88h. |
| M018 | D | DOS DIR columns | P3 | NEW d31e | README.TXT one 10-char run vs separate NAME/EXT columns (manual p.28). |
| M019 | F | DOS DIR columns | P2 | NEW b0qk | x=200..639 y=128..159 all background (14080 px) vs size + date + time fields (8-21-87, 12:00p). FAT differential vs mtools timestamps. |
| M020 | F | DOS DIR footer | P3 | NEW irp2 | "2 file(s)" only vs "37 File(s)   17408 bytes free" (manual p.29 PDF 42). Free bytes vs mtools. |
| M021 | F | database LIST | P2 | existing initech-vz6z (B68) | No NAME/CITY/BAL heading row vs U5-144 (PDF 256) and SET HEADING default ON (U5-235). vz6z already owns query.c:1289 heading generation. |
| M022 | F | database status | P2 | NEW 3cz2 | Rows y=432..479 (30720 px) all background vs Figure 5-17 status bar (U5-259, PDF 371; row 22 per U5-16). |
| M023 | D | DOS DIR footer | P3 | NEW veub | "file(s)" vs "File(s)" (manual p.29), os/milton/command.c:2418. |
| M024 | F | film chart | P2 | NEW tdnl.102 | Chart window patch x=306..537 y=142..197, 12992 px all #FFFFFF vs shaded pie + 40/35/18/14/9 (spec/assets/preview.webp x=824..1128 y=211..457, presence oracle only). 116 sum stays. Related tdnl.20, 5f6. |
| M025 | F | film worksheet | P2 | NEW tdnl.103 | Document patch x=66..298 y=102..197 (22368 px) all white vs expense grid + labels (preview.webp x=450..1064 y=450..642); 570- stays. Related tdnl.20, fcm. |
| M026 | D | film busy cursor | P3 | NEW tdnl.104 | Arrow (61/53 px) over the Saving modal, patch x=469..484 y=227..242 vs hourglass (preview.webp x=805..823 y=356..384). Hourglass stays canon; related older 26d. |
| M027 | D | database numeric | P2 | NEW 1y0h | BAL -42.00 prints "-42" (x=264..286) vs fixed N(w,2) "-42.00" (U5-144, UNITCOST 814.00). query.c:1289. |
| M028 | D | database numeric | P3 | NEW r3pa | BAL ink starts x=264 for all rows vs right-aligned numeric edges (UNITCOST both end x=795, U5-144). |

New bead ids are given as initech-<id> (tdnl.93-104 are children of
initech-tdnl): tdnl.93 M002, tdnl.94 M004, tdnl.95 M006, tdnl.96 M007,
tdnl.97 M008, tdnl.98 M009, tdnl.99 M011, tdnl.100 M012, tdnl.101 M013,
tdnl.102 M024, tdnl.103 M025, tdnl.104 M026; rtdw M015, 3sxp M016, vjtp M017,
d31e M018, b0qk M019, irp2 M020, 3cz2 M022, veub M023, 1y0h M027, r3pa M028.

## Batch plan (pickup groupings; descending visible impact per Sec 5)

1. Scroll bars (os/flair/chrome.c draw_sb_thumb:633, control.c
   draw_ctrl_scroll_thumb:672): M007, M008, M013, with tdnl.80/.82/.86 and
   the J01 resize invalidation. Rank 2-3 in REPORT Sec 5.
2. Window chrome and controls (control.c draw_push_button:375, chrome.c):
   M011, M012, M009 (locked cursors.h), M010 (needs ruling), with F16 body
   frame/widget/icon beads tdnl.51/.52/.53 and L022/L023 (tdnl.87, rgz1).
3. Menus (os/flair/menu.c): M002, M001+M005 (tdnl.70), M003 (tdnl.69), M004,
   M006. One lane, the menu goldens are re-keyed once.
4. DOS console (os/milton/command.c:2385-2418 DIR, console.c, CMD_CLS
   command.c:3134): M015, M016, M017-M020, M023 (the DIR set is one function),
   M014 after the colour ruling.
5. Database screen (os/samir/cmd/query.c:1289, set.c): M021 (vz6z), M022,
   M027+M028. Film scene (M024, M025, M026, with tdnl.20/5f6/fcm/26d) and
   Initech 123 status (L021, 68iw.1) are separate small lanes; icons/fonts
   remain tdnl.53/.68.

## Direct mappings (still-present ids, not refiled)

F13 -> initech-tdnl.47; F16 -> initech-tdnl.51 (body frame), tdnl.52
(widgets), tdnl.53 (icon art), tdnl.68 and tdnl.33 (closed slice 1) for
typography; F17 -> initech-tdnl.54; J01 -> initech-tdnl.82; K08 ->
initech-u78k; K14 -> initech-kkht; L004 -> initech-tdnl.88; L021 ->
initech-68iw.1; L022 -> initech-tdnl.87; L023 -> initech-rgz1 (B83); L046 ->
initech-39zi (B85); L117 -> initech-6qsk (B86). Each of these now carries a
2026-10-15 note citing the evidence shots. The retested H04, H06, F05, L008,
L010, L011 are not still present and get no action.

## Items that need the operator, not the developer

- M014: the report's black console conflicts with initech-r8hv, which aims
  to move the console to canon teal #8DDCDC (it is still the revoked seafoam
  #6FA08E). Either way text contrast (1.63:1) needs a decision.
- M010: single period is the product policy in spec/chrome_metrics.json; the
  ellipsis is a FROM MEMORY preference. Locked spec change needs an issue.
- M016 and M014 DOS expectations are marked FROM MEMORY in the report.
- Older bug-hunt beads hq7l, whs3 and iqhh predate current rendering (the
  live capture already has a white cursor mask and ^N text) and look stale.
