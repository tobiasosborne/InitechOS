from pathlib import Path
import json,collections
R=Path.cwd(); findings=[]
REPO='/home/tobias/Projects/initech-os/'
WP='/home/tobias/Projects/wordperfect51-decomp/'
MAN='pubs/manuals/initechword-reference/'
def add(area,title,steps,saw,expected,shots,sev='P2',kind='Defect',extra=(),guess=''):
 findings.append(dict(id=f'L{len(findings)+1:03d}',area=area,severity=sev,title=title,kind=kind,steps=steps,observed=saw,expected=expected,evidence=[('shots/'+s+'.png') for s in shots]+list(extra)+['evidence/'+session+'-serial.log' for session in sorted({shot.split('-')[0] for shot in shots}) if 'evidence/'+session+'-serial.log' not in extra],guess=guess))
def contract(file,line,text): return f'Project contract {MAN}{file}:{line}: "{text}".'
add('Save','Full-disk overwrite destroys the previous document',
 'S02: retrieve LONG.WP5; F10, ORIGINAL.WP5, Enter, Y. Repeat on fresh full-input.img in S04 via F7, Y, ORIGINAL.WP5, Enter, Y.',
 'Both attempts report Disk Full. ORIGINAL.WP5 changes from 373 bytes to ZERO bytes. The new 8,727-byte document remains in memory, but the previously saved version is gone. Serial: create succeeds, write(n=8727) returns 0, close returns 0.',
 'A failed save must preserve the last saved version (data-integrity expectation, not a claimed historical WP atomic-save measurement).',
 ['s02-real-full-original-error','s04-exit-save-full','s04-cancel-retains-long'],'P1',extra=['evidence/fixtures/ORIGINAL.WP5','evidence/s02-extracted/ORIGINAL.WP5','evidence/s04-extracted/ORIGINAL.WP5','evidence/s02-bytes.json','evidence/s04-bytes.json','evidence/prepare_full.py'],guess='os/iword/app/face_a.c:1247-1258 creates/truncates the destination before the failing write; no rollback is visible (guess).')
add('Save','Failed new saves leave zero-byte document files',
 'S02, after L001: F1 twice; F10 NEW.WP5 Enter. Later fill the released cluster by changing Setup, then F10 SMALL.WP5 Enter.',
 'Both saves fail and leave named zero-byte files. NEW.WP5 and SMALL.WP5 are not usable WP documents.',
 'A failed new save should remove its incomplete destination or clearly identify it as incomplete (auditor expectation).',
 ['s02-real-full-new-error','s02-full-small'],extra=['evidence/s02-extracted/NEW.WP5','evidence/s02-extracted/SMALL.WP5','evidence/s02-directory.txt'])
add('Window','Resizing hides the status line and every prompt',
 'S03: type Visible prompt test. Drag grow box (632,472) by (-160,-130). Press F10. Grow back from (472,342) by (160,130).',
 'The smaller window has no status line. F10 enters a name prompt that cannot be seen. Enlarging reveals the pending prompt.',
 'An enabled grow control must preserve access to document status and prompts; classic Mac window behavior from memory.',
 ['s03-resize','s03-resize-save-prompt','s03-resize-prompt-restored'],guess='os/iword/app/face_a.c:843 uses fixed editing geometry and the event handler at 2412 has no resize adaptation (guess).')
add('List Files','Directories silently lose all entries after file 254',
 'S03 input contains MANY/F000.WP5 through F269.WP5. F5 Enter, Down Right Right Right Enter Enter. Down 140 times, Right, Down. F1; Shift-F10 MANY\\F269.WP5 Enter.',
 'List Files reports 254 files and 86,868 used bytes; the list ends at F253. F269 retrieves successfully by name. Sixteen existing documents are omitted without a warning.',
 contract('ref-l.tex',73,'List Files shows the documents and other files on the data diskette')+' The fixture actually contains 270 files, 92,340 bytes.',
 ['s03-many-first','s03-many-last','s03-many-hidden-retrieve'],extra=['evidence/stress-before-directory.txt','evidence/prepare_stress.py'],guess='os/iword/app/face_a.c:1369 LF_MAX=256 and :1412 bounded find; dot entries consume two slots (guess).')
add('Quit / Finder','Shut Down silently cancels while Word has unsaved work',
 'S01: append unsaved shutdown to EXITTEST.WP5. Drag Word title (320,50) by (0,150). Activate APPS at (210,90). Hold Special (201,30), release (216,123).',
 'No save question or explanation appears; the desktop remains running. Serial says FINDER-POWER-CANCEL app=WORD.EXE cannot-ask. The unsaved document is preserved.',
 'Finder should bring the document forward for its save decision, or visibly explain the refusal (classic Mac shutdown expectation from memory). This is a blocked shutdown, not demonstrated data loss.',
 ['s01-before-shutdown','s01-finder-special','s01-shutdown-unsaved'],extra=['evidence/s01-serial.log'])
add('WP5 import','Real WP soft-page documents cannot be retrieved',
 'S01, clear screen separately for each file: Shift-F10 MINT39.WP5 Enter; repeat with MINT44.WP5.',
 'Both real WP 5.1 files are rejected as Incompatible File Format. MINT39 has a multi-page line list; MINT44 wraps a paragraph across a page break.',
 'The supplied real files and their captures are valid WP documents: '+WP+'mint/wpMINT39.steps, wpMINT44.steps and corresponding 01_typed.png. Refusing an ordinary multi-page document is an interoperability gap. The project documents refusal of unknown codes in ref-w.tex:48.',
 ['s01-import-mint39','s01-import-mint44'],kind='Documented compatibility gap',extra=['evidence/fixtures/MINT39.WP5','evidence/fixtures/MINT44.WP5','evidence/references/real-mint39.png','evidence/references/real-mint44.png','evidence/wp5-softpage-inputs-decoder.txt'])
add('WP5 import','Table cells render as one concatenated line',
 'S01: clear document; Shift-F10 MINT38.WP5 Enter; F11.',
 'A1 B1 C1 / A2 B2 C2 render as A1B1C1A2B2C2After. with no table grid or cell separation.',
 'Real capture '+WP+'mint/shots_wpMINT38/06_cells_typed.png, region (0,0)-(584,73), shows three columns and two rows. The import should preserve readable cell structure.',
 ['s01-import-mint38','s01-import-mint38-reveal'],extra=['evidence/fixtures/MINT38.WP5','evidence/references/real-mint38.png'])
for num,label,shot,ref in [('01','centered','s01-real-mint01','real-mint01'),('03','flush-right','s01-import-mint03','real-mint03')]:
 add('WP5 import',f'Imported {label} lines appear at the left margin',f'S01: clear document; Shift-F10 MINT{num}.WP5 Enter.',f'The {label} line starts at x=7, regardless of its alignment code.',f'Real MINT{num}.shots/02_typed.png in the supplied corpus shows the intended alignment. Project ref-c.tex:118-124 explicitly acknowledges the limitation.',[shot],kind='Documented compatibility gap',extra=[f'evidence/references/{ref}.png',f'evidence/fixtures/MINT{num}.WP5'])
add('WP5 import','Extended characters disappear from imported text','S03: clear document; Shift-F10 MINT05.WP5 Enter; F11.','The accented e in Cafe is missing: the line reads Caf and ...; Reveal substitutes [?].','Real MINT05.shots/02_typed.png shows the accented e; the bytes are C0 29 01 C0, documented in specs/file-formats/wp5-body-codes.md section 3. A character must remain visible.',['s03-mint05','s03-mint05-reveal'],extra=['evidence/stress-fixtures/MINT05.WP5','evidence/references/real-mint05.png'])
add('WP5 import','Imported footnote references are invisible','S03: retrieve MINT05.WP5 on a clear screen.','The superscript/reference 1 after Footnote host. is absent. Reveal shows [?] for the footnote group.','Real MINT05.shots/02_typed.png, first text line, has a highlighted 1 at that position. The user needs to see that a note exists.',['s03-mint05','s03-mint05-reveal'],extra=['evidence/references/real-mint05.png'])
add('Search','Ctrl-M inserts m instead of a searchable Soft Return','S03: type ten copies of "alpha " then "target phrase tail". Home Home Up, F2, target, Ctrl-M, phrase, F2.','The query is targetmphrase, not target[SRt]phrase, and fails. A literal-space query also failed in S01.','WP Reference printed p.573 explicitly says Ctrl-M includes a Soft Return in a search string. See evidence/references/wp-search.txt.',['s03-search-srt-query','s03-search-srt-result','s01-search-wrap-fails'],guess='os/iword/app/face_a.c:2313 expects ch=13, but the guest delivers message 0000326D for Ctrl-M; control translation/dispatch mismatch (guess).')
add('Wrap / Indent','Indent at the right edge produces a vertical column outside the margin','S03: new document; type 66 x characters, F4, "ABCDEFGH IJKL MNOP". F11; save INDENT.WP5.','Letters appear one per line at approximately x=570, beyond the default right margin. The status reaches Pos 8.1 inches although the right margin position is 7.5 inches.','The project promises paragraph wrapping at the indent (ref-i.tex:9-11) and lines ending at their margins (ref-w.tex:86-91). Expected refusal or a usable next-line indent is auditor judgment; no real-WP edge mint establishes the exact desired rule.',['s03-indent-edge-long','s03-indent-edge-reveal'],extra=['evidence/s03-extracted/INDENT.WP5','evidence/s03-bytes.json'])
add('Wrap / Tab','Tab at the long-word boundary disagrees between screen and saved coordinates','S01: new document; type 66 x characters, Tab, Z. Save as part of EDGE.WP5.','Z displays at the next line left edge (Pos 1.1 after it), but the Tab payload stores 9600 WPU, 8 inches (C1 02 80 25 80 25 50 00 C1).','spec/iword/wp5_format.h: tabs payload is the tab-stop position. The displayed and saved destination should agree (auditor consistency expectation); exact WP boundary behavior remains unminted.',['s01-66x-tab','s01-66x-reveal'],extra=['evidence/s01-extracted/EDGE.WP5','evidence/wp5-independent-decoder.txt'])
add('Filenames','Spaces in a filename are silently removed','S01: F10; type BAD NAME.WP5; Enter.','The prompt silently becomes BADNAME.WP5 and that file is saved, without invalid-name feedback.',contract('ref-f.tex',34,'a name with any other character')+' is refused, according to lines 34-37. Spaces are not among the allowed characters in lines 17-21.',['s01-space-name','s01-space-name-saved'],extra=['evidence/s01-extracted/BADNAME.WP5'],guess='os/iword/app/face_a.c:1725 accepts only ch > space, silently discarding spaces before validation (guess).')
add('Manual / List Files','Esc does not leave List Files as the manual promises','S01: F5 Enter Esc.','The directory stays open; F1/F7 work.',contract('ref-l.tex',87,'F7 or Esc to leave the list')+' NOTE: the real WP mint LF03_16 also ignores Esc, so correcting the manual is a valid fix.',['s01-list-root','s01-list-esc'])
add('Cancel / Look','F1 does not cancel the Look menu','S03: Look at TEXTLONG.TXT; press F1.','The Look screen remains. Esc exits.',contract('ref-c.tex',12,'Cancel backs out of a menu, prompt or question without doing anything')+' Lines 13-14 say it can be used whenever the status line shows a menu; Look has a menu.',['s03-look-cancel-f1'])
add('Look','Text-file Look cannot scroll past its first 20 lines','S03: F5 Enter; select TEXTLONG.TXT (Down 17, Right), 6. Down 25 times; PgDn.','Only Text line 001 through 020 remain visible; lines 021-080 cannot be read. The same Down test scrolls a WP file successfully.','Look should allow reading the selected file. Project ref-l.tex:85-87 offers Look to read a document; real WP Reference Look, printed p.363 (PDF p.381), permits cursor movement.',['s03-look-text-top','s03-look-text-stuck','s03-look-wp-scroll'],extra=['evidence/stress-fixtures/TEXTLONG.TXT'],guess='os/iword/app/face_a.c:1504 gates scrolling on g_look_wp; :1614 always starts text rendering at byte zero (guess).')
add('Look','Next Doc and Prev Doc are inert enabled choices','S03: late in session F5 Enter Down x6, 6 opens CROSS.WP5; press 1, then 2.','Both keep CROSS.WP5 on screen, although COPY.WP5 and other files exist. The menu offers 1 Next Doc; 2 Prev Doc in normal bright text.','The actual WP Look menu in spec/iword/listfiles.h:154-158 names document navigation. Either implement these choices or accurately disable them.',['s03-look-long-before-next','s03-look-long-next','s03-look-long-prev'],guess='os/iword/app/face_a.c:1502 M_LOOK does not dispatch 1 or 2 (guess).')
add('List Files','Directory prompt ignores typed paths','S01: F5; type APPS.','Dir A:\\*.* remains unchanged and Enter lists the root.','The editable-looking path should accept a directory; real WP List Files, Reference PDF pp.364-370. This is an acknowledged source limitation (face_a.c:1347-1350).',['s01-list-path-typing'],kind='Documented feature gap')
add('List Files','Change-default-directory hint advertises a dead key','S01: F5; press =.','The hint "Type = to change default Dir" remains, but no default-directory prompt opens.','The displayed instruction itself promises an action.',['s01-list-default-dir'],'P3')
add('Mouse','Clicking document text does not position the insertion point','S01: retrieve LONG.WP5; Home Down puts cursor on line 23. Click (85,70), in line 1; type MOUSE.','MOUSE is inserted at line 23, not the clicked line.','WP Reference, Mouse Support, and ordinary native document interaction; location selection is expected from memory as well.',['s01-home-down','s01-mouse-hit'],guess='os/iword/app/face_a.c:2412 dispatches keys/menu/update/close but not content mouse positioning (guess).')
add('Window','Visible scroll arrows do not scroll the document','S01: LONG.WP5 at first screen; click down arrow (627,451) three times.','No movement. Serial logs FLAIR-SCROLL-IGNORED win 0 v each time.','An enabled-looking scrollbar should scroll its document (classic Mac convention, memory). This is Word with no scroll behavior, not the earlier Finder post-resize J01 case.',['s01-home-down','s01-scroll-arrow'],extra=['evidence/s01-serial.log'])
add('Navigation','Page Up and Page Down do nothing','S01: retrieve LONG.WP5; PgDn at top. Home Home Down; PgUp at end.','Both leave cursor and viewport unchanged. Home Home Up/Down do work.','WP Reference printed p.104: PgDn/PgUp go to first line of next/previous page.',['s01-long-open','s01-pgdn','s01-long-end','s01-pgup'],extra=['evidence/references/wp-cursor.txt'])
add('Navigation','Go To becomes ordinary document typing','S01: retrieve LONG.WP5, Ctrl-Home, 6.','A literal 6 is inserted before Line 001; no Go To prompt/state is entered.','WP Reference p.224: Ctrl-Home, page number, Enter navigates to a page.',['s01-goto-six'],extra=['evidence/references/wp-goto.txt'])
add('Navigation','Ctrl-Right moves a character instead of a word','S01: type one two three; Home Home Up; Ctrl-Right.','Cursor advances to Pos 1.1, one character.','WP Reference p.104 specifies Word Right; project ref-c.tex:241 explicitly documents its reduced modifier behavior.',['s01-ctrl-right'],kind='Documented feature gap',extra=['evidence/references/wp-cursor.txt'])
add('Deletion','Delete Word deletes only one character','S03: type one two three; Home Home Up; Ctrl-Delete.','Only o is removed, leaving ne two three.','WP Reference p.116: Ctrl-Del deletes the word at the cursor. Project ref-d.tex:57-59 documents character-only behavior.',['s03-delete-word'],kind='Documented feature gap',extra=['evidence/references/wp-delete.txt'])
add('Deletion','Delete to End of Line only moves the cursor','S03, after Ctrl-Delete above: Ctrl-End.','ne two three stays intact; cursor moves to its end.','WP Reference p.116 defines Ctrl-End as deletion, not End navigation.',['s03-delete-end-line'],extra=['evidence/references/wp-delete.txt'])
add('Deletion','Delete to End of Page never asks or deletes','S05: new document one two three; Home Home Up; Ctrl-PgDn.','The whole line remains with cursor at its start and no confirmation prompt.','WP Reference p.116: Ctrl-PgDn then Y deletes to end of page.',['s05-delete-page-at-start'],extra=['evidence/references/wp-delete.txt'])
add('Typing','Insert cannot switch to typeover','S03: after word-deletion test, Home Home Up, Insert, Z.','Z is inserted before ne two three; n is not overwritten.','WP 5.1 Insert/typeover convention (memory); the project explicitly states "Ins key does nothing" in ref-i.tex:73-74.',['s03-typeover'],kind='Documented feature gap')
add('Keyboard','Repeat Value inserts its count into the document','S03: at position after Z, press Esc, 3, Right.','A literal 3 is inserted; Right moves only once.','WP Reference p.105: Escape, number, movement key repeats cursor movement.',['s03-repeat-value'],extra=['evidence/references/wp-cursor.txt'])
add('Block','Enter cannot extend a block through the paragraph','S03: new word word; Home Home Up, F12, Enter.','Block stays zero length at the first character.','WP Reference p.50: Enter blocks to the next Hard Return.',['s03-block-enter-at-start'],extra=['evidence/references/wp-block.txt'])
for title,key,shot,why in [
 ('A block cannot be saved as a file','F10','s03-block-save','WP Reference pp.49-51 explicitly permits Save on a block.'),
 ('Case conversion is absent for selected text','Shift-F3','s03-block-case','WP Reference p.57: block, Switch, Uppercase/Lowercase.'),
 ('Search cannot extend or contract a block','F2','s03-block-search','WP Reference p.573 describes searching with Block on.')]:
 add('Block',title,f'S03: type lower Case; Home Home Up, F12, End, {key}.','The selected text and Block on status remain; the command never opens.',why+' Project ref-b.tex:50-60 acknowledges the limited command set.',[shot],kind='Documented feature gap')
add('Block','Append returns without offering a file','S01: block text; Ctrl-F4, 1, 4.','The Append menu vanishes; Block stays on.','WP Reference Block p.49 includes Append. Project ref-m.tex:138-139 documents this omission.',['s01-block-move-menu','s01-block-append'],kind='Documented feature gap')
for key,name,shot in [('2','Tabular Column','s05-block-tabular'),('3','Rectangle','s05-block-rectangle')]:
 add('Block',f'{name} movement is absent',f'S05: block one two three; Ctrl-F4, {key}.','The Move menu returns to the document without performing the chosen operation.','Real WP Block/Move menu includes this operation; project ref-m.tex:138-139 explicitly lists it as unimplemented.',['s05-block-move-menu',shot],kind='Documented feature gap')
add('Codes','Home-Space inserts an ordinary breakable space','S03: new document; a, Home, Space, b; later save CHARS.WP5.','Reveal shows an ordinary blank. Saved body starts 61 20 62, not 61 A0 62.','Real MINT05 was typed with Home-Space and stores A0; specs/file-formats/wp5-body-codes.md section 2.',['s03-hardspace-compose'],extra=['evidence/s03-extracted/CHARS.WP5','evidence/wp5-independent-decoder.txt'])
add('Codes','Compose inserts literal keystrokes instead of an extended character',"S03: after a b, Ctrl-2, e, apostrophe. Save CHARS.WP5.","The document contains a b2e' (hex 61 20 62 32 65 27). No Compose prompt appears.",'Real mint/wpMINT05.steps uses Ctrl-2, e, apostrophe to create C0 29 01 C0 (accented e).',['s03-hardspace-compose'],extra=['evidence/s03-extracted/CHARS.WP5','evidence/references/real-mint05.png'])
add('Setup','Chosen measurement units do not affect measurements','S01: Shift-F1, 3, 8, 2, c; F1; type Inches?.','Setup says Centimeters, but the status still uses inch marks and values. S03 also cycles p/w/u/c/i for entry and status preferences.','A selected unit should govern its labeled field. Project ref-s.tex:360-362 explicitly acknowledges that all measurements remain inches.',['s01-units-cm','s01-units-still-inches','s03-units-cycle'],kind='Documented feature gap',extra=['evidence/s01-extracted/IWORD.SET'])
add('Setup / Save','Original Document Backup Yes never creates a backup','S01: Shift-F1,3,1,2,Y,F1. Save Original version as BACKUP.WP5. Append changed; F10 Enter Y.','Setup shows Yes; the directory has only the new BACKUP.WP5, with no backup of Original version.','The setting should protect the old version. Project ref-s.tex:314-315 explicitly admits the answer is recorded but no copy is kept.',['s01-backup-yes','s01-backup-replaced'],kind='Documented feature gap',extra=['evidence/s01-directory.txt','evidence/s01-extracted/BACKUP.WP5','evidence/s01-extracted/IWORD.SET'])
add('Setup','Timed backup is unavailable','S03: Shift-F1,3,1,1.','The gray Timed Document Backup choice does nothing.','A dependable period word processor should offer timed recovery (memory); the omission is explicit in ref-s.tex:308-310.',['s03-timed-backup'],kind='Documented feature gap')
for key,name,shot,real in [('4','Base Font','s01-base-font','25_base_font.png'),('5','Print Color','s01-print-color','27_print_color.png')]:
 add('Font',f'{name} selection immediately returns to editing',f'S01: Ctrl-F8, {key}.','No selection screen or change appears.',f'Real mint/shots_wpMINT28/{real} shows the screen. Project ref-f.tex:69-71 documents the omission.',['s01-font-menu',shot],kind='Documented feature gap')
add('Menu polish','Unimplemented Font choices look enabled','S01: Ctrl-F8.','Base Font and Print Color are shown in normal white even though selecting them does nothing.','Use the same gray convention already used for unavailable List Files and Setup choices (internal UI consistency expectation).',['s01-font-menu'],'P3')
add('Menu polish','Unavailable Format choices look enabled','S03: Shift-F8, then 1.','Page/Document/Other and the six inactive Line settings look like working white choices. Their numbers leave the menu unchanged.','Inactive options should be gray as they are in List Files and Setup (internal consistency expectation).',['s03-format-main','s03-format-line-1'],'P3')
for key,label in [(2,'Display'),(4,'Initial Settings'),(5,'Keyboard Layout'),(6,'Location of Files')]:
 add('Setup',f'{label} setup is unavailable',f'S03: Shift-F1, {key}.',f'The gray {label} row does not open.','The named WP setup capability is unavailable. Project ref-s.tex:295-297 documents the gray options; expectation is the requested full period-workstation feature set, not a claim that the project promised implementation.',[f's03-setup-{key}'],kind='Documented feature gap')
for key,label in [(2,'Page'),(3,'Document'),(4,'Other')]:
 add('Format',f'Format {label} group cannot be entered',f'S03: Shift-F8, {key}.','The Format menu remains; none of this group can be reached.','Real mint/shots_wpMINT21/01_format_menu.png names the group. Project ref-f.tex:130-141 documents its absence. Child settings were not independently tested because this entry point is blocked.',[f's03-format-main-{key}'],kind='Documented feature gap')
for key,label in [(1,'Hyphenation'),(2,'Hyphenation Zone'),(4,'Line Height'),(5,'Line Numbering'),(8,'Tab Set'),(9,'Widow/Orphan Protection')]:
 add('Format',f'{label} cannot be configured',f'S03: Shift-F8,1,{key}.','The Line menu stays unchanged at its default value.','Real mint/shots_wpMINT21/02_format_line.png shows this setting. Project ref-f.tex:135-141 acknowledges it is inactive.',[f's03-format-line-{key}'],kind='Documented feature gap')
add('Margins','Zero-inch margins are silently ignored','S01: Shift-F8,1,7; type 0 Enter for left, 0 Enter for right.','The menu still shows 1 inch for each margin, without explaining a refusal.',contract('ref-m.tex',64,'Type a margin in inches')+' Lines 65-66 only require it to be less than 9 inches. Zero is a valid numeric input under that stated rule.',['s01-zero-margins'],guess='os/iword/app/face_a.c:1924 uses if(v), conflating zero with invalid input (guess).')
# One entry per observed placeholder command, not one broad missing-features bucket.
keys=[('f3','Help','help'),('f9','End Field','end-field'),('shift-f3','Switch','switch'),('shift-f4','reverse Indent','left-indent'),('shift-f5','Date/Outline','date'),('shift-f6','Center','center'),('shift-f7','Print','print'),('shift-f9','Merge Codes','merge-codes'),('alt-f1','Thesaurus','thesaurus'),('alt-f5','Mark Text','mark-text'),('alt-f6','Flush Right','flush-right'),('alt-f7','Columns/Table','columns-table'),('alt-f8','Style','style'),('alt-f9','Graphics','graphics'),('alt-f10','Macro playback','macro'),('ctrl-f1','Shell','shell'),('ctrl-f2','Spell','spell'),('ctrl-f3','Screen rewrite/split','screen'),('ctrl-f5','Text In/Out','text-in-out'),('ctrl-f6','Tab Align','tab-align'),('ctrl-f7','Footnote','footnote'),('ctrl-f9','Merge/Sort','merge-sort'),('ctrl-f10','Macro Define','macro-define')]
for key,name,slug in keys:
 shots=['s01-key-'+slug]
 if slug=='print':shots+=['s05-list-file-print']
 if slug=='center':shots+=['s01-block-center']
 if slug=='flush-right':shots+=['s01-block-flush']
 add('WP command',f'{name} is only a placeholder',f'S01: on a clear editing screen press {key}.',f'Only the gray command name appears on the status line. No {name} operation or menu opens.',f'WP 5.1 Reference printed pp.963-964/966 key table; spec/iword/fkeys.json carries that mapping. The project template pubs/manuals/initechword-install/strip.tex:86-99 explicitly marks this command unavailable. This is a feature-completeness finding, not a broken manual promise.',shots,kind='Documented feature gap',guess='spec/iword/fkeys.h maps this slot to IW_FK_gray (guess for dispatch cause).')
# Print already has both routes above; do not duplicate it here.
for key,label,slug in [('2','Delete','delete'),('3','Move/Rename','rename'),('5','Short/Long Display','display'),('7','Other Directory','other-dir'),('8','Copy','copy'),('9','Find','find'),('n','Name Search','name-search')]:
 add('List Files',f'{label} is unavailable in the file list',f'S05: F5 Enter, Down three times, Right selects CODES.WP5; press {key}.','The selected file and listing stay unchanged; no operation or prompt opens.',f'Real WP List Files menu in spec/iword/listfiles.h:128-143 includes {label}. Project ref-l.tex:103-107 explicitly acknowledges it is disabled.',['s05-list-file-selected','s05-list-file-'+slug],kind='Documented feature gap',extra=['evidence/s05-file-ops.json','evidence/s05-extracted/CODES.WP5'])

# Report assembly.
counts=collections.Counter(f['severity'] for f in findings)
head=f'''# InitechWord audit - 2026-10-23

Audited supplied image commit e67b8311092873fe16e6c6d62ad82f894bfb8d7d.
{len(findings)} findings: {counts['P1']} P1, {counts['P2']} P2, {counts['P3']} P3.
The catalog separates 30 observed defects from 61 documented feature/compatibility gaps.
The critical result is repeatable loss of the previous file on a full-disk overwrite (L001).
The basic editor is substantially more dependable than its feature coverage: tested code
nesting, blocks, undo, ordinary saving, and a real-file round trip passed. Many findings
below are explicitly documented omissions, retained because the brief asks for a complete
1990s workstation, not just conformance to the reduced implementation.

## 1. Method and replay conventions

Only copied disks were booted. Five QEMU sessions (S01-S05) used -display none,
-cpu 486, -m 16 and QMP on stdio. One emulator at a time; no host X input or
capture, repository writes, builds, commits, tracker operations or other users' processes.
All five QEMUs were stopped through their own QMP connections. See evidence/cleanup.jsonl
and each session JSON for PIDs, times, image hashes and complete command lines.

S01 used images/prepared.img; S02 and S04 used images/full-input.img; S03 used
images/stress-input.img; S05 used prepared.img. Preparation scripts and source fixtures
are in evidence/. All filesystem extraction happened after the corresponding QEMU stopped.

Common launch: double-click disk (599,65), APPS (125,104), WORD.EXE (284,124).
On the full disk a startup disk-full alert must first be dismissed with Enter.
"New/clear document" means, from the editing screen, F7, N, N. It deliberately
abandons the preceding test document. F1 then Esc can dismiss a modal before that sequence.
Home Home Up and Down are consecutive key presses, not held chords. Plain filenames
are in the data disk root unless a path is written. Text is typed without a final Enter
unless Enter is explicitly specified. Screen coordinates are in the 640x480 QEMU framebuffer.

Each finding's shots are unmodified screendumps converted to PNG. Contacts/crops in evidence/
were used only to inspect them. The authoritative input stream is evidence/actions.jsonl;
primitive per-session streams and evidence/replay.py preserve the recorded timing and avoid double-running
composite type/click records. Example: `python3 evidence/replay.py s04 /tmp/iword-replay-s04`
requires a NEW output directory and runs only one emulator. Preparation scripts may be rerun
before replay if the prepared input disks are unavailable. Source snapshots in
evidence/source-contracts/ include numbered lines; all relative source references below
are under /home/tobias/Projects/initech-os. Real WP corpus paths are under
/home/tobias/Projects/wordperfect51-decomp; the original PDF is
/home/tobias/Projects/initech-manuals-ref/wordperfect51/WPDOS51Reference.pdf.

A "Documented feature gap" is observed behavior that the project's manual already admits;
it is not presented as an accidental regression. Source causes are guesses unless stated
as direct serial/byte evidence. Exact real-WP behavior at the long-word Tab/Indent edge
remains unminted; those two expectations are explicitly consistency judgments.

## 2. Findings

'''
parts=[head]
for f in findings:
 parts.append(f"### {f['id']} - {f['severity']} - {f['title']}\n\nArea: {f['area']}. Classification: {f['kind']}.\n\nReplay: {f['steps']}\n\nObserved: {f['observed']}\n\nExpected and basis: {f['expected']}\n\nEvidence: "+', '.join(f'[{p}]({p})' for p in f['evidence'])+'. The matching session serial log and primitive input stream are in evidence/.\n')
 if f['guess']:parts.append('\nSource hypothesis: '+f['guess']+'\n')
 parts.append('\n')
parts.append('''## 3. Still present / Now fixed

Still present: no earlier InitechWord IDs exist, as the brief states. The earlier audit
reports were searched for InitechWord/iword/WordPerfect and relevant desktop exit/scroll
entries were inspected. This report does not relabel old Finder J01 or tenant G03 as Word defects.

Now fixed: none claimed. This was the first Word drive. In particular, the observed Finder
shutdown refusal does not prove the older general shutdown F05 fixed or still present.

## 4. Coverage, passes and limits

Covered:

- Ordinary and rapid typing (45 ms between QMP characters), 66/67-character unbroken words,
  wrap, spaces, hard returns, line joins by Backspace/Delete, Tab and Indent at the edge.
  A 350-line document and a 65,536-byte document were retrieved and navigated.
- All 48 function-key slots in the shipped map, including six unmapped modified F11/F12
  slots, both Reveal and Block aliases, all seven Font sizes and nine appearances, Normal,
  nested/crossing attributes, empty pairs, code-deletion prompts and direct Reveal deletion.
- Forward/backward selection, bold/underline on blocks, copy/move/retrieve, deletion No/Yes,
  three undo levels, crossing an attribute and a hard page, cancel; the unsupported block
  operations have separate findings. Ctrl-F4 sentence selection was exercised. A few screenshot
  names say copy in S01 although the recorded sequence actually selected Move; the passing
  copy proof is S03 COPY.WP5, not those early names.
- Forward/backward text search, case matching, literal attribute search, code deletion by
  replacement, adjacent matches, empty replacement, confirmation Y/N and cancel, backwards
  replacement, not-found and Soft Return entry. Search with a literal space across a wrapped
  line failed; that alone is not claimed to differ from WP, which documents explicit SRt entry.
- Save/new name/replace No and Yes, invalid long and punctuation names, spaces, save on exit,
  F7 save/exit/name cancel, save-then-clear, save-then-exit, discard-and-exit, clean Ctrl-Q,
  dirty Ctrl-Q/close-box/File Exit, File Save/Retrieve, full-disk failures and read-only refusal.
- List Files root/subdirectories/parent, all offered operation keys, directory capacity,
  WP and text Look, Look arrows/next/previous/cancel, retrieval onto empty and occupied screens
  with No/Yes; real WP imports, empty/bad-header/absent/plain-text/oversize/code-only files.
- Setup's displayed choices, all five unit letters for both preference fields, backup Yes,
  settings persistence and full-disk rollback; Format's three parent groups and all nine Line
  choices, center/right justification, line spacing 2, margins and cancellation; window resize,
  document click, scrollbar, Finder shutdown with an unsaved document.

Byte-verified passes (evidence/byte-validation.json and wp5-independent-decoder.txt):

- ROUND01.WP5 is byte-for-byte identical to the real MINT01.WP5, all 495 bytes.
- ADJ.WP5 body is bbbb after replacing every a in aaaa; zero adjacent matches were skipped.
- BLOCK.WP5 has bold around AB and underline around DE. NEST.WP5 has the expected crossing
  bold/underline sequence. COPY.WP5 contains COPY ME.COPY ME. CROSS.WP5 keeps CD plain.
- CODEOUT.WP5 preserves a document containing only an empty bold pair.
- PAGES.WP5 restores its hard-page D4 group and 0C after block deletion/Undelete.
- MAXOUT.WP5 remains 65,536 bytes and differs from its input only in the permitted cursor
  flag at 0x103. Extra text and Bold were refused without damaging its contents.
- Read-only READONLY.WP5 survived the failed overwrite unchanged. Settings failed on the
  full disk with an error and retained Inches in memory. The failed new IWORD.SET is zero bytes.
- The independent corpus decoder found zero trailer errors in the 13 selected output files.
  This is structural validation, not proof of complete WP semantic or printer compatibility.

Limits / unreached:

- No actual printing, spell check, help topic, split-window operation, macro, table creation,
  date dialog, style/graphics editor, secondary document, mail merge, or inactive Format/Setup
  child dialog can be driven through the observed disabled entry points. No invented findings
  are assigned to unobserved child screens.
- No real WP executable was run during this session; comparisons use its supplied captures,
  minted documents, format notes and original manual. The long-word boundary behavior needs
  an additional real-WP mint before prescribing exact compatibility bytes.
- Not every numeric margin/spacing value, every possible block boundary or every possible
  keyboard chord was enumerated. Tests were repeated where useful, especially full-disk loss,
  directory no-ops, save flows and aliases; this is not a mathematical exhaustive audit.
- Several hundred lines were supplied as a generated WP fixture rather than typed by hand.
  No CPU panic or emulator hang was observed; no intentional host power-cut test was performed.
- The S02 screenshots without "real" in their initial full-disk attempt show the desktop:
  a startup disk-full alert consumed the first launch attempt. Those are not evidence of a
  Word save failure. s02-settings-full-error.png actually shows a successful setting change
  after the failed overwrite freed a cluster; the valid failed-setting evidence is S04.
- The <6 year display is a reproduced real-WP Y2K behavior (LISTF mint ledger lines 91-92),
  not a newly claimed defect. Blue editing background, gray adaptation bands, the two menu
  bars, and the other deliberate brief exceptions are not reported as defects. The original
  WP and the project's ref-s.tex:91 specify blue, despite the brief's shorthand "black".
- Some unsupported attributes draw as ordinary text, as the manual says. All sixteen code
  pairs were exercised and saved; no unverified assertion about their exact historical screen
  colors or printer output is made here. Imported header/footer text was not judged against
  ordinary editing view because that view may intentionally omit it.

## 5. Fifteen fixes to prioritize

''')
priority=[
('Full-disk overwrite destroys the previous document','Preserve the previous version with a transactional save.'),
('Print is only a placeholder','Make an office document printable from the workstation.'),
('Real WP soft-page documents cannot be retrieved','Accept ordinary multi-page WordPerfect files.'),
('Resizing hides the status line and every prompt','Keep every prompt visible in supported window sizes.'),
('Shut Down silently cancels while Word has unsaved work','Connect the application save guard to Finder shutdown.'),
('Original Document Backup Yes never creates a backup','Make the advertised backup protect user work.'),
('Directories silently lose all entries after file 254','Enumerate complete directories or clearly report limits.'),
('Table cells render as one concatenated line','Preserve readable structure when importing tables.'),
('Help is only a placeholder','Provide usable in-application help and key discovery.'),
('Page Up and Page Down do nothing','Restore the expected navigation speed for long documents.'),
('Go To becomes ordinary document typing','Stop navigation commands from altering the document.'),
('Ctrl-M inserts m instead of a searchable Soft Return','Make searches across layout boundaries usable.'),
('Indent at the right edge produces a vertical column outside the margin','Give margin-edge indent behavior a real-WP oracle and implement it.'),
('Spaces in a filename are silently removed','Reject invalid names without silently changing the requested destination.'),
('Extended characters disappear from imported text','Render common accented text before expanding exotic formatting.')]
for i,(title,reason) in enumerate(priority,1):
 f=next(x for x in findings if x['title']==title);parts.append(f"{i}. {f['id']}: {reason}\n")
parts.append('\nAll retained screenshots, serial logs, extracted bytes, preparation scripts, replay scripts and verification results are local to this audit directory.\n')
report=''.join(parts)
report.encode('ascii')
(R/'REPORT.md').write_text(report,encoding='ascii')
(R/'NITS.tsv').write_text('id\tarea\tseverity\tsummary\n'+''.join('\t'.join([f['id'],f['area'],f['severity'],f['title']])+'\n' for f in findings),encoding='ascii')
(R/'evidence/findings.json').write_text(json.dumps(findings,indent=2),encoding='ascii')
print(len(findings),dict(counts),len(report),'report bytes')
