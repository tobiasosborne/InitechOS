# Audit pass 6: InitechOS appearance

Image commit: 3aaf6e0 (images/COMMIT). Measurements below are from this session.
Read-only source HEAD seen at handoff: f7ed504a779ceaf85c39b5a03caf4bb459e00e62.
Guest behavior is attributed to the image commit, not that later source HEAD.
This is a visual audit, not a source-code repair or a build. The source/reference
repositories were read only. No input was sent to X display :0. Every guest
used a local disk copy, -display none, and QMP over stdio. At most one emulator
was active at a time; all emulators started by these scripts were quit by QMP.

The result is mixed. The title-band profile, many sampled grays, disabled
scroll arrows, page well and grow art are remarkably close. The body frames,
thumbs, several menu details, button rendering and console presentation keep
the screen from having the finish of the references. Existing findings retain
their IDs; new measured nits are M001 onward.

## Measurement conventions and references

All rectangles in the report prose are inclusive when written as x=a..b.
JSON/Python crop rectangles [left,top,right,bottom] are half open. PNGs retain
native guest pixels. Enlargements in diffs use nearest-neighbor scaling.
Pink marks differing RGB pixels; ink comparisons normalize non-black to white.
If crops have different dimensions they are padded with white, as recorded in
findings.json. Counts over different text inventories are context, not a
universal rendering score. Every new finding identifies its focal measurement.

The Mac goldens are sampled framebuffer RGB under Mac HiRes Std Gamma. This
pass compares the sampled domain, because Initech spec/chrome_metrics.json
also selects that domain. Nominal #CCCCCC/#DDDDDD are not mistaken for the
sampled #DADADA/#E7E7E7. The palette caveat is documented in
/home/tobias/Projects/system7-decomp/specs/sys8/platinum-palette.md, section 1.

Mac paths below mean real captures under
/home/tobias/Projects/system7-decomp/goldens/captures/. Their full copied PNGs
and the exact crops used are in refs/. System 7 Chicago/Geneva glyph oracles
come from the original NFNT 5478/5415 resources in goldens/resources/. Font
oracle PNGs are reference-only renderings of those resources; they are not
claimed to be additional guest states. Mac OS 8 uses Charcoal in these
captures; Chicago-vs-Charcoal belongs to existing F16.

Manual crops are rendered from original PDF pages at a maximum dimension of
1400 pixels. They establish fields, words and layout, not native VGA RGB.
Their comparison images deliberately have no pixel heat map. The 1456x819
film reference photo (spec/assets/preview.webp) is also used only for scene
content and cursor presence; no CRT-photo pixels become native chrome or
palette expectations. Expectations
without a captured historical state are explicitly marked FROM MEMORY.

The earlier five REPORT.md and TRIAGE.md pairs, plus the later
2026-10-05-astra-full-drive pair, were checked. Their path/hash/heading index
is evidence/prior-report-index.json. No previously reported issue is silently
renumbered as an M finding.

## 1. Per-area verdicts

| Area | Verdict | Measured result and reference |
| --- | --- | --- |
| Menu-bar geometry/palette | close | Both strips are 20 px. At clear x=400: white row, 17 #E7E7E7 rows, #B3B3B3 row, black row: 0/20 different against s8_desktop_menubar.png x=400 y=0..19. Corners match the reference where not covered by the left-shifted Apple. Left slot and highlight are M001/M002. |
| Pull-down geometry | close | Finder File panel is x=20..174 y=39..245: 155x207, for 12 ordinary items and two separators. Reference File is x=33..230 y=19..278: 198x260, for 15 items/three separators. Inventory differences are not a panel-size bug. Ordinary rows are 16 px, separators 6 px; cap top is panel top+4 in both. Frame 1 px, inner bevel white/#B3B3B3, drop shadow #3F3F3F. Disabled ink #A5A5A5 and etched separator #A5A5A5/white match. M003-M006 cover shortcut/mark/ink placement. |
| Active title band | matches | 22 rows: black, white, two #DADADA, 12 alternating white/#969696, four #DADADA, #B3B3B3, black. 0/660 differing pixels in the stripe crop (90,60)-(120,82) against s8_doc_window_active (100,70)-(130,92). Six light/six dark stripes. HELLO light-row runs x=81..185 and 232..321 shift to x=82..186 and 233..322 on dark rows. Its light text gap x=186..231 has 6/5 px padding around ink x=192..226, and shifts +1 on dark rows, matching the Mac rule. |
| Close/zoom/collapse art | close | 12x12 boxes plus highlight footprint 13x13, at top+4, close left+4, zoom right-32, collapse right-16. Aligned 13x13 crops each differ at only 2/169 pixels (interior off-diagonal corners #A5A5A5 instead of #DADADA). This is residual F16, not three new widget findings. Mac s8_doc_window_active x=69/448/464 y=74. |
| Active body frame | wrong | At root y=200, x=20..25 is black then five whites. Reference y=150 x=65..70 is black, white, #DADADA, #DADADA, #B3B3B3, black. Four raised bar columns and the inner line are lost in visible body painting: existing F16. |
| Shadow/inactive chrome | close | Active one-pixel right/bottom black shadow and two-pixel near-corner notch are present. Inactive title is flat #E7E7E7 with #878787 ink, frame/shadow #777777, widgets absent; bars hollow #F3F3F3. Geometry is 22 px in both active and inactive. Reference s8_doc_window_inactive inactive window x=13..325 y=29..198. Different window dimensions/titles prevent a whole-window equality score. |
| Disabled scroll controls | matches | Total band 16 px, interior 14 px, 14x14 arrow tile, 20 #A5A5A5 arrow pixels in 8x4 triangle. Entire upper tile matches: 0/196 RGB differences, current (360,82)-(374,96), Mac active-window disabled tile (461,113)-(475,127). |
| Enabled scroll controls | wrong | Arrow tile 194/196 pixels match (M013). Well cross-section is identical, 0/14: #969696,#A5A5A5, ten #C0C0C0,#CDCDCD,#DADADA. Thumb is 14x15 but flat, with no dark grips (M007/M008). Inactive bars have no arrows/thumb/separators, matching the hollow state. Resize invalidation still shows J01. |
| Grow box | matches | 17x17 aligned crop matches all 289 pixels. Both have three diagonal lines, 18 #969696 grip pixels, 3 #C0C0C0 termination pixels, 54 whites and 183 #DADADA face pixels. Mac crop x=460..476 y=295..311; current x=359..375 y=259..275. |
| Chicago 12 | close | HELLO: 37 px advance, 142 ink pixels, 0/555 normalized-ink differences. File: 23 px advance, 83 vs 84 ink, 1/345 differences. New Folder: 74 px advance, 262 vs 250 ink, 34/1110 differences. Caps use 9 rows; menu cell top 2, baseline 14 (12 ascent). Font choice versus Mac 8 Charcoal remains F16. Bold/italic were not reachable. |
| Geneva 9 / icon labels | wrong | README.TXT advance is 64 px in the shipped strike vs 58 in NFNT 5415. Observed ink is 63x8 (146 pixels) vs 57x7 (134 pixels) in the oracle. Current label ink y=122..129 below icon body y=88..115. Selection is black field/white text and does not invert the entire window. Existing F16 covers font/label/art fidelity; no duplicate M font finding. |
| Icons / desktop | close | Deliberate desktop is a uniform #8DDCDC in the live FLAIR captures; no checker-pattern defect is filed. Folder/document/application/disk art is still sparse versus Mac crops (F16). Full Trash now differs by 75 pixels from empty Trash in a clean 50x61 patch; H04 is not still present. Unlabeled SAMIR volume label is blank (K14). |
| Dialog/alert geometry | wrong | Quit/Trash frame x=133..506 y=87..190 is 374x104, as the alert golden. Its black band is 7 px vs the golden's 1 px outline plus 2 bevel rows (L023). Caution area 26x26 is empty (L022). Buttons are 59x20 and default ring 65x26, +3 on each side; label erasure and corner fringe are M011/M012. No alert-offset defect was observed. |
| Film saving dialog | close | FILE COPY is x=140..499 y=200..279, 360x80, 22-pixel striped title. Message ink begins (155,228). Progress outer rect x=154..485 y=249..268, 332x20; filled interior is 224 px of a 330-px interior (4032 navy pixels = 224x18). These dimensions are authored Initech dialog.c/spec values, not a Mac captured progress-control oracle. The underlying chart/worksheet are blank (M024/M025); percentage and 570- value fidelity cannot be graded. |
| Pointer / text and busy cursors | close | Arrow hot point aligns with the tip but mask/stem differ (M009). No editable Macintosh text field was reachable, so I-beam comparison is uncovered. The steady film modal uses the arrow; the film saving state retains that arrow (M026). The hourglass strike itself and an actual I/O busy transition were not reached. No wristwatch replacement is proposed. |
| DOS screen | wrong | 640x480 graphics console, 8x16 cells (80 columns; 30 cell rows inferred from dimensions), #C0C0C0 on #6FA08E. Prompt, CLS, cursor and DIR are measured in M014-M020/M023. VGA-like fixed glyphs are plausible, but the original DOS manual is not a native font bitmap oracle. No 30-row legality claim is made. |
| Initech 123 | close | Three 16-pixel control-panel rows before column border y=110..125. Border 4 character cells; column widths 9 cells (72 px native, 81 px in the real VGA capture). Eight columns A..H; native shows 19 rows versus real 20 because of its window/chrome. Mode is 5 cells at x=578..617 y=62..77. Pointer D1 is 72x16 at x=250..321 y=126..141. These are coherent native adaptation choices, not new bugs. READY ink differs at 12/640 pixels after removing the real VGA ninth columns and normalizing reverse colours (diffs/i123-ready.png). The clock/UNDO status area is blank (L021). |
| SAMIR/database | wrong | Dot-prompt, three LIST records and database return were reached. Missing headings/status and shared palette are M014/M021/M022; fixed precision/right alignment are M027/M028. The question output leaves 42. with prompt attached (L046). Full-screen BROWSE failed via the known unsupported workflow; no editor/Assistant pixel-parity claim is made. |
| Motion | close, limited | Menu/drag/scroll samples and exact host times are in evidence/motion-samples.json. Menus stayed open under held input; drag outline stayed while held; one transient partial repaint appeared on release. The native collapsed title band is byte-identical to the expanded band (shots 113-115; evidence/collapse-check.json), as the Mac collapsed capture describes. No real Mac video/timing oracle exists in this corpus, so latency/flicker observations are not converted into unsupported historical findings. |

The white-to-selected-title contrast is 1.57:1 for current #8DDCDC,
versus 6.37:1 for the captured #5454B3 face. Console text/background
contrast is 1.63:1. These are computed from measured sRGB values;
they quantify the washed-out appearance, not a modern accessibility-policy audit.

Matching and residual crops: [evidence/comparisons.json](evidence/comparisons.json); focal numeric
samples: [evidence/metrics.json](evidence/metrics.json); visual overview:
[evidence/chrome-contact.png](evidence/chrome-contact.png).

## 2. New measured nits

### M001 - Apple slot and first title sit too far left

Area: menu bar. Severity: P3.

State and steps: Fresh flair_tenants_interactive.img + flair_data.img copies. Double-click disk (600,64); move pointer to (500,380). 

Initech draws: Lower-bar Apple black ink is x=4..13; File ink x=28..48. The same narrow Apple slot is on the upper bar. File starts 15 px left of the Mac capture; logo starts 13 px left.

Reference: Mac OS 8 Apple colour ink x=17..26; File ink x=43..62. Both bars are otherwise 20 px tall.

Source: /home/tobias/Projects/system7-decomp/goldens/captures/s8_desktop_menubar.png, x=0..79 y=0..19; specs/sys8/menus.md 1.3.

Comparison: 445/1600 differing rgb pixels in the displayed, documented crops. [Diff/comparison image](diffs/M001.png); [guest crop](diffs/M001-initech.png); [reference crop](refs/M001-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M002 - Selected menu title uses a pale flat fill

Area: menu highlight. Severity: P2.

State and steps: Fresh flair_tenants_interactive.img + flair_data.img copies. Double-click disk (600,64); move pointer to (500,380). Move to File (38,30), hold left button, wait 0.3 s. 

Initech draws: Selected File block x=17..59 has #8DDCDC fill; white letters sit on that pale fill. At x=21 the selected band has a single accent value through y=20..38, then black baseline y=39.

Reference: At x=34, Mac selected File uses #8787DA y=0, #5454B3 y=1..17, #0000A5 y=18, black y=19. The accent has a dark face and separate edge shades.

Source: /home/tobias/Projects/system7-decomp/goldens/captures/s8_menu_dropped.png, x=34 y=0..19; specs/sys8/menus.md 1.4.

Qualification: The desktop teal is deliberate. This finding concerns the selected-title ink/background contrast and lost accent shading, not the desktop colour. The real capture uses Lavender; no claim that every installation must use Lavender.

Comparison: 19/20 differing rgb pixels in the displayed, documented crops. [Diff/comparison image](diffs/M002.png); [guest crop](diffs/M002-initech.png); [reference crop](refs/M002-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M003 - Caret substitutes for the Command glyph

Area: menu shortcuts. Severity: P3.

State and steps: Fresh flair_tenants_interactive.img + flair_data.img copies. Double-click disk (600,64); move pointer to (500,380). Move to File (38,30), hold left button, wait 0.3 s. 

Initech draws: New Folder shows ^N. Its black shortcut ink is x=144..157 y=43..51.

Reference: New Folder shows the looped Command mark and N, x=199..216 y=23..31.

Source: /home/tobias/Projects/system7-decomp/goldens/captures/s8_menu_dropped.png, command column x=199..216 y=23..31; Menu Manager commandMark=17.

Qualification: Ctrl remains a usable PC shortcut. This is a glyph-fidelity nit, not a request to change key handling.

Comparison: 83/266 differing ink pixels in the displayed, documented crops. [Diff/comparison image](diffs/M003.png); [guest crop](diffs/M003-initech.png); [reference crop](refs/M003-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M004 - Shortcut column has excessive right clearance

Area: menu shortcuts. Severity: P3.

State and steps: Fresh flair_tenants_interactive.img + flair_data.img copies. Double-click disk (600,64); move pointer to (500,380). Move to File (38,30), hold left button, wait 0.3 s. 

Initech draws: New Folder N ends at x=157; panel right black frame is x=174: ink-to-frame distance 17 px.

Reference: N ends at x=216; right frame x=230: distance 14 px (13 intervening pixels).

Source: /home/tobias/Projects/system7-decomp/goldens/captures/s8_menu_dropped.png, x=209..232 y=21..34; specs/sys8/menus.md 2.2.

Qualification: The panels have different item inventories and widths. This compares the last shortcut glyph against each panel right edge, not total panel width.

Comparison: 105/364 differing rgb pixels in the displayed, documented crops. [Diff/comparison image](diffs/M004.png); [guest crop](diffs/M004-initech.png); [reference crop](refs/M004-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M005 - Item ink begins one pixel too far from the left frame

Area: menu text placement. Severity: P3.

State and steps: Fresh flair_tenants_interactive.img + flair_data.img copies. Double-click disk (600,64); move pointer to (500,380). Move to File (38,30), hold left button, wait 0.3 s. 

Initech draws: Panel left frame x=20; New Folder first N ink x=41: offset 21 px. Text cap top y=43 is the correct panel-top +4.

Reference: Panel left frame x=33; N ink x=53: offset 20 px. Cap top y=23 is also panel-top +4.

Source: /home/tobias/Projects/system7-decomp/goldens/captures/s8_menu_dropped.png x=33/53 y=23..31; specs/sys8/menus.md 2.2.

Qualification: This is a painted-ink comparison. Chicago and Charcoal left bearings differ; it is not evidence that the Chicago advance table is wrong.

Comparison: 83/390 differing rgb pixels in the displayed, documented crops. [Diff/comparison image](diffs/M005.png); [guest crop](diffs/M005-initech.png); [reference crop](refs/M005-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M006 - View check mark is a lowercase v

Area: menu marks. Severity: P3.

State and steps: Fresh flair_tenants_interactive.img + flair_data.img copies. Double-click disk (600,64); move pointer to (500,380). Hold View (120,30), wait 0.5 s.

Initech draws: The by Icons mark is a 6x7 ink box at x=101..106 y=45..51, 26 black pixels. It is a v-shaped letter.

Reference: System 7 checkMark is character 18. NFNT 5478 glyph 18 renders a 9x8 check with 18 ink pixels; its advance is 11 px.

Source: /home/tobias/Projects/system7-decomp/goldens/resources/NFNT_5478.bin glyph 0x12; specs/toolbox/menu-manager.md mark table (checkMark=18). refs/chicago-checkmark.png is rendered directly from the original NFNT location/offset/width tables.

Comparison: 22/165 differing ink pixels in the displayed, documented crops. [Diff/comparison image](diffs/M006.png); [guest crop](diffs/M006-initech.png); [reference crop](refs/M006-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M007 - Enabled thumb face has no bevel ramp

Area: scroll thumb. Severity: P2.

State and steps: Use many_source.img (a copy of flair_data.img with thirty F00.TXT..F29.TXT files added by prepare_fixtures.py). Boot tenants; double-click disk (600,64); move pointer clear. 

Initech draws: The 14x15 thumb interior x=360..373 y=97..111 contains 205 #8DDCDC and 5 #F3F3F3 pixels. No separate leading highlight or trailing shadow is visible.

Reference: The 14x15 interior x=608..621 y=311..325 contains 49 #DADAFF, 102 #B3B3FF, 26 #8787DA, 28 #5454B3 and 5 #F3F3F3 pixels. The outer face has distinct light and dark edges.

Source: /home/tobias/Projects/system7-decomp/goldens/captures/s8_doc_window_inactive.png: active Applications thumb x=608..621 y=311..325; specs/sys8/scrollbars.md 2.4.

Qualification: A teal accent could keep the deliberate desktop colour while retaining a three-shade thumb. This is not a demand for a Lavender desktop.

Comparison: 205/210 differing rgb pixels in the displayed, documented crops. [Diff/comparison image](diffs/M007.png); [guest crop](diffs/M007-initech.png); [reference crop](refs/M007-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M008 - Thumb grip lines disappear into the face

Area: scroll thumb. Severity: P2.

State and steps: Use many_source.img (a copy of flair_data.img with thirty F00.TXT..F29.TXT files added by prepare_fixtures.py). Boot tenants; double-click disk (600,64); move pointer clear. 

Initech draws: Only five pale #F3F3F3 cap pixels interrupt the flat teal face. The grip region x=363..369 y=100..106 has zero dark grip pixels.

Reference: The active Applications thumb has four #5454B3 dark grip lines, 7 px each, 28 dark pixels total, with companion highlight lines.

Source: /home/tobias/Projects/system7-decomp/goldens/captures/s8_doc_window_inactive.png x=610..619 y=313..321; specs/sys8/scrollbars.md 2.4.

Qualification: Separate from M007: this is the interior grip, not the outer bevel. Both states remain functionally draggable.

Comparison: 86/90 differing rgb pixels in the displayed, documented crops. [Diff/comparison image](diffs/M008.png); [guest crop](diffs/M008-initech.png); [reference crop](refs/M008-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M009 - Arrow stem and white mask differ from the Mac cursor

Area: pointer. Severity: P3.

State and steps: Fresh flair_tenants_interactive.img + flair_data.img copies. Move pointer to (500,380).

Initech draws: On a 16x16 canvas at (499,379), the cursor has 61 black and 53 white pixels. Black bbox x=500..508 y=380..393 (9x14); the stem reads as a squared vertical tail.

Reference: The arrow at (230,29) has 54 black and 43 white pixels on the corresponding 16x16 canvas at (229,28); black bbox x=230..237 y=29..42 (8x14), with an angled tail. Aligned symbol masks differ at 48/256 pixels.

Source: /home/tobias/Projects/system7-decomp/goldens/captures/s8_alert_modal.png x=229..244 y=28..43. Reference opacity is obtained by subtracting the same wallpaper patch in s8_desktop_menubar.png; current cursor is shots/001-desktop.png x=499..514 y=379..394.

Qualification: Gray in these two normalized masks means transparent, black means cursor ink, white means cursor paper. The initial cursor screenshot is in shots/, not a synthetic guest screen.

Comparison: 48/256 differing rgb pixels in the displayed, documented crops. [Diff/comparison image](diffs/M009.png); [guest crop](diffs/M009-initech.png); [reference crop](refs/M009-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M010 - Narrow title ends in one period instead of an ellipsis

Area: title truncation. Severity: P3.

State and steps: Fresh flair_tenants_interactive.img + flair_data.img copies. Double-click disk (600,64); move pointer to (500,380). Drag grow box (370,270) by (-200,-120); move pointer clear.

Initech draws: The 160x100 window draws Drive A Fil. The suffix is one 2x2 period cluster at x=131..132 y=73..74, 4 ink pixels.

Reference: An ellipsis is three dot clusters. The Chicago NFNT ellipsis (0xC9) has 12 ink pixels in a 10x2 ink box, advance 14 px. Using an ellipsis to signal title truncation is an expectation from my classic Mac memory.

Source: /home/tobias/Projects/system7-decomp/goldens/resources/NFNT_5478.bin glyph 0xC9; behavior expectation FROM MEMORY, no matching truncated-title Mac golden.

Qualification: The read-only Initech chrome_metrics.json explicitly selects a single period as current product policy. This is a requested appearance-polish deviation, not an undisclosed claim of a captured Mac title rule.

Comparison: 12/210 differing ink pixels in the displayed, documented crops. [Diff/comparison image](diffs/M010.png); [guest crop](diffs/M010-initech.png); [reference crop](refs/M010-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M011 - Opaque label background erases button highlight and shadow

Area: button bevel. Severity: P2.

State and steps: Fresh flair_tenants_interactive.img + flair_data.img copies. Double-click disk (600,64); move pointer to (500,380). Double-click APPS (123,101), then 123.EXE (211,124). Enter Hello, Right, 123, Right, 1.5, Right, -7 (Enter after each entry); Ctrl-Q; move pointer clear. 

Initech draws: Cancel is 59x20 at x=363..421 y=158..177. Across x=365..418 at y=160, only 11 pixels stay white and 43 become face #E7E7E7. At y=175, only 10 stay #C0C0C0 and 44 are face. The label creates gaps in both bevel strips.

Reference: Same Cancel rect. Row y=160 x=365..418 is all 54 white pixels; row y=175 has 53 #C0C0C0 and one face pixel. The bevel passes continuously above and below the label.

Source: /home/tobias/Projects/system7-decomp/goldens/captures/s8_alert_modal.png x=363..421 y=158..177; specs/sys8/controls.md 5.1.

Qualification: The raw full-button diff also includes the known Chicago/Charcoal label difference (F16). The new claim is restricted to rows 160 and 175, outside the label ink.

Comparison: 208/1180 differing rgb pixels in the displayed, documented crops. [Diff/comparison image](diffs/M011.png); [guest crop](diffs/M011-initech.png); [reference crop](refs/M011-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M012 - Rounded button corners acquire extra gray fringe pixels

Area: button corners. Severity: P3.

State and steps: Fresh flair_tenants_interactive.img + flair_data.img copies. Double-click disk (600,64); move pointer to (500,380). Double-click APPS (123,101), then 123.EXE (211,124). Enter Hello, Right, 123, Right, 1.5, Right, -7 (Enter after each entry); Ctrl-Q; move pointer clear. 

Initech draws: Cancel corner (363,158) is #C0C0C0 and (364,158) is #969696; both sit outside the top black span. Across four 4x4 corner patches, 13/64 pixels differ from the Mac button.

Reference: Both cited pixels are surrounding #E7E7E7 face. The captured corner follows the smaller rounded black edge with its local corner shading.

Source: /home/tobias/Projects/system7-decomp/goldens/captures/s8_alert_modal.png top-left x=363..366 y=158..161. Four-corner count also uses x=418..421 and y=174..177.

Qualification: Independent of M011 label erasure; this crop has no label ink. Geometry still matches 59x20 and a +3 px default ring.

Comparison: 3/16 differing rgb pixels in the displayed, documented crops. [Diff/comparison image](diffs/M012.png); [guest crop](diffs/M012-initech.png); [reference crop](refs/M012-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M013 - Enabled arrow tiles mispaint two off-diagonal corners

Area: scroll arrows. Severity: P3.

State and steps: Use many_source.img (a copy of flair_data.img with thirty F00.TXT..F29.TXT files added by prepare_fixtures.py). Boot tenants; double-click disk (600,64); move pointer clear. 

Initech draws: In the 14x14 up-arrow tile x=360..373 y=82..95, top-right is #CDCDCD and bottom-left #FFFFFF.

Reference: The corresponding tile x=608..621 y=296..309 has face #E7E7E7 at both off-diagonal corners. Only 2/196 pixels differ; the other 194 and the 20-pixel arrow glyph match.

Source: /home/tobias/Projects/system7-decomp/goldens/captures/s8_doc_window_inactive.png x=608..621 y=296..309; specs/sys8/scrollbars.md 2.2.

Comparison: 2/196 differing rgb pixels in the displayed, documented crops. [Diff/comparison image](diffs/M013.png); [guest crop](diffs/M013-initech.png); [reference crop](refs/M013-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M014 - Console keeps pale gray text on green

Area: DOS/database console. Severity: P2.

State and steps: Boot tenants + samir_list.img copies; press backslash; USE CLIENTS.DBF, then LIST; wait for output. Separately boot tracer_boot.img + flair_data.img and run DIR. In SAMIR try SET COLOR TO and LIST again.

Initech draws: Both consoles use foreground #C0C0C0 and background #6FA08E. The settled DOS DIR screen has 4792 foreground pixels and 302408 background pixels. SET COLOR TO leaves the same palette (and returns a text error).

Reference: dBASE III PLUS colour defaults are standard white on black, as specified on printed U5-204 (PDF 316). My default DOS gray/white-on-black expectation is FROM MEMORY; the DOS manual scan cannot establish historical framebuffer RGB.

Source: /home/tobias/Projects/initech-manuals-ref/dbase3plus/dBASE III Plus 1.1 (1986) Manuals/Using dBase III Plus.pdf printed U5-204, PDF 316, Defaults. DOS baseline expectation FROM MEMORY.

Qualification: The deliberate teal DESKTOP is not being reported as a bug. This is the suspended text-console surface. No scan RGB is treated as a VGA palette measurement.

Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. [Diff/comparison image](diffs/M014.png); [guest crop](diffs/M014-initech.png); [reference crop](refs/M014-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M015 - Ready prompt has no visible blinking cursor

Area: DOS cursor. Severity: P2.

State and steps: Boot tracer_boot.img + flair_data.img copies, wait for shell, type DIR, wait 1 s. Type CLS, wait 1 s; capture a complete blink-cycle sample series without input.

Initech draws: After CLS, only the four-character A:\> prompt is drawn (86 foreground pixels, bbox x=0..30 y=386..395). The next character cell x=32..39 y=384..399 contains zero foreground pixels. Fourteen further frames across the recorded 2+ s interval have identical RGB bytes.

Reference: The DOS manual printed p.5 (PDF 18) explicitly shows a cursor following A> and describes a blinking box or flashing underline.

Source: /home/tobias/Projects/initech-manuals-ref/dos33/MS-DOS_3.3_Users_Guide_198707.pdf printed p.5, PDF 18, The Default Drive and the MS-DOS Prompt, prompt/cursor illustration.

Qualification: Timing and hashes are evidence/cursor-cycle.json. A print illustration is not used as a pixel-exact cursor shape oracle.

Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. [Diff/comparison image](diffs/M015.png); [guest crop](diffs/M015-initech.png); [reference crop](refs/M015-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M016 - CLS clears pixels but leaves the prompt on row 24

Area: DOS CLS. Severity: P2.

State and steps: Boot tracer_boot.img + flair_data.img copies, wait for shell, type DIR, wait 1 s. Type CLS; wait 1 s.

Initech draws: All old text disappears, but the new prompt cell starts at y=384 (row 24 with 16-pixel cells); ink is y=386..395. Top 384 rows are blank.

Reference: The clean screen should put the prompt at the upper left (row 0): this homing expectation is FROM MEMORY. The manual printed p.45 (PDF 130) documents CLS leaving only a prompt and cursor, without giving framebuffer coordinates.

Source: /home/tobias/Projects/initech-manuals-ref/dos33/MS-DOS_3.3_Users_Guide_198707.pdf printed p.45, PDF 130, Cls; row-0 homing expectation FROM MEMORY.

Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. [Diff/comparison image](diffs/M016.png); [guest crop](diffs/M016-initech.png); [reference crop](refs/M016-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M017 - Directory listing omits the volume identity line

Area: DOS DIR header. Severity: P3.

State and steps: Boot tracer_boot.img + flair_data.img copies, wait for shell, type DIR, wait 1 s. 

Initech draws: The DIR block begins with one Directory of A:\ line at y=98..107. It has zero Volume in drive lines even though the pristine data disk is labeled INITECH.

Reference: The illustrated DIR header has Volume in drive A is ... followed by Directory of A:\: two identity/location lines.

Source: /home/tobias/Projects/initech-manuals-ref/dos33/MS-DOS_3.3_Users_Guide_198707.pdf printed p.28, PDF 41, DIR example heading.

Qualification: K10 covers wrong paths/ignored operands. This is the newly measured missing volume line; no operand test is re-filed.

Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. [Diff/comparison image](diffs/M017.png); [guest crop](diffs/M017-initech.png); [reference crop](refs/M017-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M018 - Name and extension are joined with a dot

Area: DOS DIR columns. Severity: P3.

State and steps: Boot tracer_boot.img + flair_data.img copies, wait for shell, type DIR, wait 1 s. 

Initech draws: README.TXT is one ten-character run at column 0, x=0..79 cell span. There is no separate extension column.

Reference: The manual table separates basename and extension into aligned columns, e.g. COMMAND and COM, ANSI and SYS.

Source: /home/tobias/Projects/initech-manuals-ref/dos33/MS-DOS_3.3_Users_Guide_198707.pdf printed p.28, PDF 41, first two DIR table rows.

Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. [Diff/comparison image](diffs/M018.png); [guest crop](diffs/M018-initech.png); [reference crop](refs/M018-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M019 - DIR rows omit date and time fields

Area: DOS DIR columns. Severity: P2.

State and steps: Boot tracer_boot.img + flair_data.img copies, wait for shell, type DIR, wait 1 s. 

Initech draws: Both directory entries contain only dotted name plus size or <DIR>. The right part x=200..639 y=128..159 is entirely background: 14080 pixels, no timestamp ink.

Reference: The same table shows date and time after the size for each entry, e.g. 8-21-87 and 12:00p. There are five displayed fields: name, extension, size, date, time.

Source: /home/tobias/Projects/initech-manuals-ref/dos33/MS-DOS_3.3_Users_Guide_198707.pdf printed p.28, PDF 41, first two DIR rows.

Qualification: No claim about timestamp storage (earlier L030) is made; this is absent display columns.

Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. [Diff/comparison image](diffs/M019.png); [guest crop](diffs/M019-initech.png); [reference crop](refs/M019-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M020 - Footer omits the free-space figure

Area: DOS DIR footer. Severity: P3.

State and steps: Boot tracer_boot.img + flair_data.img copies, wait for shell, type DIR, wait 1 s. 

Initech draws: Footer is only 2 file(s), at row 11. There are zero bytes free tokens or free-space figures after the count.

Reference: The continued DIR example ends with 37 File(s) and 17408 bytes free on the same footer line; the particular count/byte value depends on the disk.

Source: /home/tobias/Projects/initech-manuals-ref/dos33/MS-DOS_3.3_Users_Guide_198707.pdf printed p.29, PDF 42, final DIR footer.

Qualification: The sample byte value is not the expected free space of the Initech disk. The missing field is the finding.

Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. [Diff/comparison image](diffs/M020.png); [guest crop](diffs/M020-initech.png); [reference crop](refs/M020-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M021 - LIST lacks field headings

Area: database LIST. Severity: P2.

State and steps: Boot tenants + samir_list.img copies; press backslash; USE CLIENTS.DBF, then LIST; wait for output. Also try SET HEADING ON before LIST; wait 1 s.

Initech draws: Three records display, but zero headings for NAME, CITY, BAL. The first record follows the LIST command directly. SET HEADING ON returns the already-known text error path.

Reference: LIST sample prints field names above the rows (U5-144, PDF 256). SET HEADING is normally ON and supplies one column title for each field (U5-235, PDF 347).

Source: /home/tobias/Projects/initech-manuals-ref/dbase3plus/dBASE III Plus 1.1 (1986) Manuals/Using dBase III Plus.pdf printed U5-144, PDF 256, LIST example; printed U5-235, PDF 347, SET HEADING defaults.

Qualification: The error string is not a new finding: K08 covers inappropriate unsupported-command errors. The appearance finding is the missing table-heading row.

Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. [Diff/comparison image](diffs/M021.png); [guest crop](diffs/M021-initech.png); [reference crop](refs/M021-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M022 - SET STATUS ON never produces the status bar

Area: database status. Severity: P2.

State and steps: Boot tenants + samir_list.img copies; press backslash; USE CLIENTS.DBF, then LIST; wait for output. Issue SET STATUS ON; wait 1 s.

Initech draws: The bottom three native text rows (y=432..479, 30720 pixels) are all #6FA08E; no status or guidance text and no reverse band appear. SET STATUS ON returns a text error.

Reference: The manual Figure 5-17 shows a status bar containing Command Line, drive/database and Rec: position/count, plus a guidance line. With STATUS ON it should be visible; U5-16 reserves terminal row 22 for status.

Source: /home/tobias/Projects/initech-manuals-ref/dbase3plus/dBASE III Plus 1.1 (1986) Manuals/Using dBase III Plus.pdf printed U5-259, PDF 371, Figure 5-17; U5-16, PDF 128, row allocation.

Qualification: The illustrated blue is not graded as a framebuffer RGB oracle. The 30-row Initech console has no status band anywhere, not merely a band at a different bottom row.

Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. [Diff/comparison image](diffs/M022.png); [guest crop](diffs/M022-initech.png); [reference crop](refs/M022-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M023 - File-count label uses lowercase f

Area: DOS DIR footer. Severity: P3.

State and steps: Boot tracer_boot.img + flair_data.img copies, wait for shell, type DIR, wait 1 s. 

Initech draws: The actual footer spells 2 file(s), with lowercase f (a seven-character label; nine characters including count and space).

Reference: The original manual example spells 37 File(s), capital F. The counter length varies but the label casing does not in that example.

Source: /home/tobias/Projects/initech-manuals-ref/dos33/MS-DOS_3.3_Users_Guide_198707.pdf printed p.29, PDF 42, footer File(s).

Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. [Diff/comparison image](diffs/M023.png); [guest crop](diffs/M023-initech.png); [reference crop](refs/M023-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M024 - Film chart window is blank

Area: film chart. Severity: P2.

State and steps: Boot a fresh flair_live_interactive.img + flair_data.img copy, wait for FLAIR-LIVE-READY; move pointer clear to (500,380). Also inspect the sampled boot frames.

Initech draws: The visible chart-window content patch x=306..537 y=142..197 has 12992 pixels, all #FFFFFF. It contains zero non-white chart pixels or percentage-label ink. All steady film captures have the same blank bodies.

Reference: The supplied scene reference shows a blue shaded pie with percentage labels 40, 35, 18, 14 and 9, occupying photographed x=824..1128 y=211..457. The 116 percent sum is deliberate and is not flagged.

Source: /home/tobias/Projects/initech-os/spec/assets/preview.webp x=824..1128 y=211..457 (1456x819 CRT photograph); user brief identifies flair_live_interactive.img as the film-scene image.

Qualification: Presence comparison only. The photo is not converted into native window dimensions or a palette target. Existing F16 concerns chrome/icon/font finish; this is missing application content.

Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. [Diff/comparison image](diffs/M024.png); [guest crop](diffs/M024-initech.png); [reference crop](refs/M024-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M025 - Film worksheet body has no grid or expense data

Area: film worksheet. Severity: P2.

State and steps: Boot a fresh flair_live_interactive.img + flair_data.img copy, wait for FLAIR-LIVE-READY; move pointer clear to (500,380). Also inspect the sampled boot frames.

Initech draws: The unobscured left document patch x=66..298 y=102..197 is 22368 entirely white pixels: zero row/column lines, labels or numbers. The blank document continues around the foreground saving modal.

Reference: The scene reference shows an expense worksheet, with labels, numeric cells and repeated grid lines in photographed x=450..1064 y=450..642. This is the source of the deliberate 570- figure; no complaint about that figure value is made.

Source: /home/tobias/Projects/initech-os/spec/assets/preview.webp x=450..1064 y=450..642 (CRT photograph); user brief film-scene expectation.

Qualification: This is the film underlying document, separate from the working disk-launched 123 worksheet. The two application bodies are different missing visuals; no native grid pitch is inferred from the photographed screen.

Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. [Diff/comparison image](diffs/M025.png); [guest crop](diffs/M025-initech.png); [reference crop](refs/M025-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M026 - Saving modal retains the arrow instead of the scene hourglass

Area: film busy cursor. Severity: P3.

State and steps: Boot film-scene copies; move the pointer inside the saving modal to (470,228), wait 0.4 s; capture.

Initech draws: The pointer over the gray modal face is the normal arrow: 61 black and 53 white pixels in the 16x16 canvas x=469..484 y=227..242, the same counts as the idle desktop arrow. No hourglass appears.

Reference: The saving state in the supplied reference shows an hourglass at photographed x=805..823 y=356..384. The user explicitly preserves that hourglass instead of the historical Mac wristwatch.

Source: /home/tobias/Projects/initech-os/spec/assets/preview.webp hourglass x=805..823 y=356..384; deliberate hourglass requirement in the user brief and spec/assets/cursors.h.

Qualification: The hourglass asset itself was not rendered by the guest, so its strike art remains ungraded. This finding is limited to the visible saving-scene cursor; an actual I/O busy transition was not reached.

Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. [Diff/comparison image](diffs/M026.png); [guest crop](diffs/M026-initech.png); [reference crop](refs/M026-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M027 - LIST drops fixed decimal places for integral numeric fields

Area: database numeric format. Severity: P2.

State and steps: Boot tenants + samir_list.img copies; press backslash; USE CLIENTS.DBF, then LIST; wait for output. Inspect BAL in record 2; DBF descriptor is N(8,2).

Initech draws: The input record stores BAL as two spaces then -42.00 (eight bytes). LIST prints only -42: three character cells at x=264..287, ink x=264..286 y=82..91. The first and third records print 1234.50 and 7777.77 with two decimal places.

Reference: The original LIST example prints the integral UNITCOST value 814 as 814.00. Its original TOURS.DBF descriptor is N(10,2); both it and BAL specify two decimal places. Width differs, but precision does not.

Source: /home/tobias/Projects/initech-manuals-ref/dbase3plus/dBASE III Plus 1.1 (1986) Manuals/Using dBase III Plus.pdf printed U5-144, PDF 256, UNITCOST heading and first two values. /home/tobias/Projects/dbase3-decomp/mint/work/TOURS.DBF UNITCOST descriptor; evidence/CLIENTS-input.DBF BAL descriptor/record 2, offsets in evidence/dbf-input-fields.json.

Qualification: This is a plain stored field display, not a calculation-precision test. The DBF field metadata and stored bytes were read directly; no database mutation or real-dBASE invocation was invented.

Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. [Diff/comparison image](diffs/M027.png); [guest crop](diffs/M027-initech.png); [reference crop](refs/M027-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

### M028 - LIST aligns numeric fields to their left edge

Area: database numeric alignment. Severity: P3.

State and steps: Boot tenants + samir_list.img copies; press backslash; USE CLIENTS.DBF, then LIST; wait for output. Compare the right edges of the three BAL values.

Initech draws: BAL ink boxes are x=265..319, x=264..286 and x=264..318 for rows 1/2/3 at y=66..75, 82..91 and 98..107. Values begin in the same column and the short negative value ends 32-33 px earlier. Each number sits in an eight-character field starting at x=264.

Reference: The first two UNITCOST values in the LIST illustration start at x=745 and x=737, but both end at x=795 (scan threshold mean RGB <100, row bands y=858..879 and 881..901). The narrower number gains leading space; numeric right edges align.

Source: /home/tobias/Projects/initech-manuals-ref/dbase3plus/dBASE III Plus 1.1 (1986) Manuals/Using dBase III Plus.pdf printed U5-144, PDF 256, first two UNITCOST rows; rendered manual coordinates x=716..804 y=856..901. N(8,2) native field comes from evidence/dbf-input-fields.json.

Qualification: Scanner pixels are used only to locate alignment in the illustration, not to equate its glyph dimensions to native VGA cells. This is separate from M027 precision loss: even a three-character displayed number should be right aligned in its numeric field.

Comparison: reference illustration/layout comparison; no scanner/photo-to-framebuffer RGB count. [Diff/comparison image](diffs/M028.png); [guest crop](diffs/M028-initech.png); [reference crop](refs/M028-reference.png). Crop coordinates are in [evidence/findings.json](evidence/findings.json).

## 3. Still present (existing IDs only)

Only freshly observed IDs are included. This is not a status review of all
functional findings in the earlier reports.

| Existing ID | This session's evidence |
| --- | --- |
| F13 | The Apple slot does not open a menu: shots/002-system-menu.png, desktop action log. No real Apple menu panel could be compared. |
| F16 | Residual body frame, typography/Geneva labels, sparse icon art and the two-pixel widget corner differences: diffs/F16-body.png, F16-File.png, F16-New-Folder.png, F16-folder.png, F16-close/zoom/collapse.png, metrics.json. Chicago HELLO now matches its strike completely; the broad original typography description is not copied forward unchanged. |
| F17 | No Finder item-count/free-space header: shots/004-root.png and diffs/F17-header.png. The Mac folder capture has its header between title y=91 and content separator y=112 (20 face rows). |
| J01 | Horizontal bar remains enabled-looking after grow restores the 360x220 root: shots/051-many-grow.png and 118/119; evidence/bars-proof-serial.log has horizontal max=134 after shrink, grow back to 360x220, then FLAIR-SCROLL-IGNORED win 0 h while the thumb still shows. This is the known resize-state invalidation finding, not a new thumb-range finding. |
| K08 | BROWSE remains an error instead of a screen: shots/080-db-browse-unreached.png. Unsupported SET errors seen during appearance checks retain the earlier error-workflow ID. |
| K14 | Blank label on the unlabeled SAMIR volume: shots/112-unlabeled-disk.png. |
| L004 | Trash Cancel dismisses on mouse-down: shots/109-trash-cancel-down.png already shows the underlying desktop before release; 110 is after release. The early 103 quit-button attempt did not have an alert up and is not evidence. No pressed-button look state remains to compare. |
| L021 | Worksheet status line stays blank: shots/028-123-empty.png and 029-123-row1.png, bottom content row y=430..445; real lotus-01_grid.png has clock/date and UNDO at y=384..399. No new clock nit is filed. |
| L022 | Caution area is still blank in Empty Trash: diffs/L022-caution.png, shots/108-trash-alert.png. |
| L023 | Seven-pixel black alert border remains: diffs/L023-border.png, shots/108-trash-alert.png. |
| L046 | ? 42 leaves output and next prompt joined as 42.: shots/111-db-query-prompt.png after a one-second settle. |
| L117 | CTENANT readouts disappear after a 50x50 title drag: shots/064-ctenant.png / 065-ctenant-moved.png and diffs/L117-repaint.png. |

Fresh checks which do NOT support still-present claims: H04 (Trash changes
by 75 pixels after staging); H06 (zoomed TenantFix gutter strip x=21..405,
y=456..458 contains 1152 #F3F3F3 pixels and 3 inactive-outline pixels, not
white overpaint); F05 (safe screen appears and its Restart button boots the desktop; the Special > Restart menu was not retested); L008 (quit warning
appears and cancels); L010 (slash menu is visible in shot 101); L011 (Save
prompt is reached in shot 102, but no on-disk save was needed for this look
pass). These are limited retests, not blanket completion claims.

Two tempting observations were rejected during verification: an apparent
192-pixel alert offset, and a shutdown panel apparently vanishing after a
text click. Native pixel scans disprove both: alert black rows are always
y=87..93 and 184..190; shutdown before/after text-click images are byte
identical. Neither appears in NITS.tsv.

## 4. Coverage and limitations

Compared at native pixel level: both bar strips; a settled upper File panel (121) and
Finder File/View/Special menus; active/inactive HELLO, NOTES, root, APPS,
TenantFix, CTENANT and 123 window chrome; active widgets, held close/zoom,
window outline drag, grow/zoom, disabled/enabled/inactive scrollbars,
thumb drag, arrow hold/repeat, narrow-title truncation; icon/label/selection
surfaces and empty/full Trash; Empty Trash and changed-worksheet alerts;
default ring and Cancel art; film modal/progress surface; shutdown message
and its Restart button; DOS boot/VER/DIR/TYPE/PROMPT/CLS; SAMIR LIST/query/status and
return; Initech 123 empty sheet, row-1 data, LABEL entry, slash menu, Save
prompt and changed-work quit warning.

Reference pairing included Mac OS 8 active/inactive/collapsed/alert/menu/
Appearance controls captures, System 7 menu/window captures, original Chicago
12/Geneva 9 strikes, real Lotus MINT01 grid/entered/save screens, and the
original DOS/dBASE manual pages. Full reference screenshots and small crops
are preserved, with original source hashes in refs/manifest.json.

Not reached or not backed by a matching golden:

- Real Apple menu panel in Initech; a working About box, menu hierarchy,
  alternate Finder views, editable Macintosh text field, checkbox/radio/
  popup preference panels, or a standard Open/Save chooser.
- A live I-beam or rendered hourglass strike; bold/italic text styles; a same-title
  truncated Mac window. The ellipsis expectation is therefore marked memory.
- A real Mac OS 8 pressed arrow/thumb/button capture. The local sys8 specs
  explicitly mark pressed states and selected-menu-item fills as golden gaps.
  System 7 pressed captures were inspected for context, not silently used
  as Mac 8 pixel oracles. Current selected items/held controls are captured. The Trash Cancel commits before a pressed-state snapshot can be retained (L004).
- Pie art and 570- in the supplied film guest: steady captures 038/105 and
  35 boot samples show white underlying windows plus the saving modal. A
  boot contact sheet is evidence/film-boot-contact.png. M024/M025 record the missing application bodies; their deliberate percentages/figure values remain ungraded. M026 records the absent saving-scene hourglass. The supplied reference photo is a presence oracle, never a source of native dimensions.
- Database BROWSE/EDIT/ASSIST screens, because the reached BROWSE command
  failed. No real dBASE screen PNGs were present in that decomp corpus, so
  manual illustrations establish layout and default colours; no full-screen
  RGB diff against an invented dBASE capture was performed.
- The DOS manual does not supply a native VGA font/palette strike. Its
  printed examples are used only for prompt/cursor/field layout. The Mac
  palette RGBs are exact; manual palette statements are semantic colours.
- Original Mac shutdown screen coordinates were not captured in the corpus.
  The user-authorized computer wording is preserved and unflagged. The screen
  is measured (436x112 panel footprint at x=103..538 y=140..251, Restart ring at
  x=465..529 y=220..245); its location is authored product layout, not a
  claimed historical Mac measurement.

No panic or hang was observed in these sessions. No destructive source-repo
work, builds, make targets, commits, tracker writes or unrelated-process kills
were performed. Changed worksheet data and Trash staging were confined to
copied disks.

Motion observations (host wall time, QMP sampling limits): first menu sample
was at 4.7 ms and showed a partial update; the
fully highlighted panel was sampled at 47.2 ms
and stayed identical through 450.9 ms. Held drag
outline stayed stable from 71.5 through
378.4 ms. On release the outline remained
in early samples, one partial repaint was captured, and the final frame was
stable from 243.7 ms onward. Scroll
trace confirms a 16-pixel step; all ten post-click frames were identical.
This sampling can reveal intermediate blits, but cannot prove what a
period CRT user perceived or set a historically correct drop latency.

DOS cursor series: 14 frames, first 54.6 ms to last
2529.0 ms, 1 distinct pixel
hash(es). The earlier three CLS frames are also byte identical.

## 5. Twenty look fixes I would make first

This ranks visible repairs, including existing IDs; it does not refile them.

1. Restore the four-column raised body border and inner outline before content
   paint (F16). It affects almost every window.
2. Give enabled thumbs real light/face/dark edge shades (M007).
3. Restore the four visible dark thumb grips (M008).
4. Use a dark selected-title face with enough white-text contrast (M002).
5. Replace the seven-pixel alert slab with the captured outline/bevel (L023).
6. Preserve button top/bottom bevels when painting labels (M011).
7. Match Geneva 9 label advances and ink weight (F16; README is 64 vs 58 px).
8. Restore the film chart and expense-sheet content (M024/M025), plus the saving-scene hourglass (M026).
9. Draw the caution icon in its reserved alert area (L022).
10. Repaint scrollbar range/state after every resize (J01).
11. Restore a black console background and appropriate light text (M014).
12. Draw a blinking ready cursor and home CLS to row 0 (M015/M016).
13. Add the database heading row and status strip, preserving numeric precision/right alignment (M021/M022/M027/M028).
14. Show DIR date/time fields and align basename/extension columns (M018/M019).
15. Add Finder's item count and available-space header (F17).
16. Restore the 123 clock/date/UNDO status content (L021).
17. Increase the Apple slot margin to match the real bar (M001).
18. Draw Command/check symbols and align the shortcut column (M003/M004/M006).
19. Finish folder/document/app/disk icon art (F16) and preserve CTENANT's visible state during repaint (L117).
20. Replace the remaining small control/cursor corner differences, then the
    ellipsis and DIR header/footer polish (M009/M010/M012/M013/M017/M020/M023).

## Evidence and replay

NITS.tsv has one row per new nit. evidence/findings.json records state,
source, coordinates, difference count and qualifications for every row.
evidence/comparisons.json covers matching/known/fixed comparisons. Full guest
captures are in shots/; reference images/crops in refs/; heat maps and illustration
comparison panels in diffs/. The input CLIENTS.DBF field descriptors (NAME,
CITY, BAL; three records) are in evidence/dbf-input-fields.json.

Scripts: audit_driver.py (adapted from the read-only pass-5 text_driver.py),
prepare_fixtures.py, replay_visual.py, measure.py, motion_capture.py,
finish_capture.py, cursor_samples.py, chrome_finish.py, last_capture.py, upper_menu_check.py, build_report.py, validate_evidence.py. replay_visual.py makes
fresh copies and covers the principal surfaces; evidence/actions.jsonl is
the detailed timestamped record of this session's input.
Some early filenames name the state being attempted rather than achieved;
evidence/screenshot-notes.json identifies those attempts, and they are not
used as positive findings. Capture scripts
have finally cleanup and QMP quit. Logs/session image hashes are in evidence/.

Run python3 prepare_fixtures.py, then python3 replay_visual.py for a fresh
visual replay. The motion/cursor scripts add denser temporal samples. Running
build_report.py regenerates this report and its crops from the existing PNGs.
It does not boot or build anything. validate_evidence.py checks the generated
artifacts and measured focal values; evidence/final-validation.json is the
check result. Source image filenames remain pristine;
all writable guest copies have separate label-boot/label-data names.
