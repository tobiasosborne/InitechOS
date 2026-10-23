import json,pathlib
ROOT=pathlib.Path(__file__).resolve().parents[1]
findings=[]
def add(title,area,severity,steps,observed,expected,basis,shots,session,files=(),guess=None):
 findings.append(dict(title=title,area=area,severity=severity,steps=steps,observed=observed,expected=expected,basis=basis,shots=list(shots),session=session,files=list(files),guess=guess))
add('F2 followed by Return truncates a full-length label','Entry/edit','P1',
 'At A1 type 239 lowercase x characters followed by Z (240 characters), Return. /File Save LONG0. Press F2, Return without changing anything. /File Save LONGEDIT.',
 'LONG0.WK1 stores a label prefix plus all 240 characters. LONGEDIT.WK1 stores the prefix plus only 239 x characters; the final Z is gone. A no-op edit changes the data.',
 'Preserve the complete accepted label through an unmodified edit.',
 'Lotus R2.2 Reference pp. 1-16, 1-17, 1-22 (PDF 32, 33, 38); byte equality is the audit data-integrity expectation.',
 ['long-edit-data'], 'files', ['evidence/extracted/files/LONG0.WK1','evidence/extracted/files/LONGEDIT.WK1','evidence/long-edit-byte-proof.json'],
 'Guess: os/i123/app/face_a.c:1064 passes the 241-byte g_edit buffer to i123_cell_content; os/i123/core/fmt.c:614 copies the stored prefix into that same capacity, leaving room for only 239 label characters.')
add('A leading dollar sign is rejected in numeric entry','Entry/numbers','P2',
 'F5, B4, Return; type $12.34, Return.',
 'The cell stays blank and the rejected $12.34 remains in EDIT.',
 'Accept numeric 12.34; the dollar sign need not display until a currency format is chosen.',
 'Lotus Reference p. 1-19 (PDF 35), Numbers: "If you begin a number with a $ (dollar sign), 1-2-3 will enter the number".',
 ['011-currency'],'entry',guess='Guess: os/i123/core/fmt.c:638 accepts a sign but does not consume a leading dollar sign.')
add('F2 cannot move the insertion point into existing text','Entry/edit','P2',
 'Enter abcdef in A6. F2, Left, X; then Home, Y.',
 'The edit buffer becomes apostrophe-abcdefX and then apostrophe-abcdefXY. Both keys leave insertion at the end.',
 'Left moves one character left and Home moves to the first character, allowing edits inside the entry.',
 'Lotus Reference pp. 1-22/1-23 (PDF 38/39), editing-key table.',
 ['017-edit-left','018-edit-home'],'entry',guess='Guess: os/i123/app/face_a.c:1106 returns for EDIT Left/Right/Tab; no edit-cursor index is maintained.')
add('Copy refuses a multi-cell destination range','Worksheet/Copy','P2',
 'Populate A1 with 1. /Copy; FROM A1, Return; TO D1..D3, Return.',
 'ERROR says "Specify the upper left destination cell"; the target is not filled.',
 'Copy a single source cell over the specified destination range.',
 'Lotus Reference chapter 3, /Copy pp. 3-4/3-5; local spec/i123/era_shortfalls.md I123-P3-07 also records the missing tiling behavior.',
 ['copy-target-range-refused'],'transfers',guess='Guess: os/i123/app/face_a.c:742 explicitly rejects destinations with different corners.')
add('Period does not anchor an interactively pointed range','Range picking','P2',
 'F5 A1 Return; /Copy; press period, Down, Right, Return at the FROM prompt.',
 'The pointer reaches B2 but the prompt contains a literal period. Return produces Invalid range.',
 'Period anchors the starting corner so arrows extend a range; Escape or Backspace removes the anchor.',
 'Lotus Reference chapter 1, Specifying a Range (PDF 40/41); this is the period anchor operation called out in the brief.',
 ['point-dot-range','point-dot-result'],'transfers',guess='Guess: os/i123/app/face_a.c:873 treats period as ordinary prompt text and arrows only move the cell cursor.')
add('A permitted 240-character column width makes its cell disappear','Worksheet/width','P2',
 'Put 1 in A1. At A1 choose /Worksheet Column Set-Width, enter 240. Then press Right.',
 'At A1 the A heading, value, and cell pointer disappear; the worksheet body is blank. Right reveals B and later columns again.',
 'Show the visible portion of the wide active column and its pointer, or refuse a width the viewport cannot render.',
 'Own native-spreadsheet usability expectation; the application explicitly accepts widths 1..240.',
 ['width-240','width-240-right'],'transfers',guess='Guess: os/i123/app/face_a.c:341 uses whole-column i123_cols_fitting; col_x:351 then rejects the active column when none fits.')
add('Range names cannot be redefined while formulas exist','Range names','P2',
 'Put 5 in B1. /Range Name Create rates Return B1 Return. In C1 enter +rates*2. Repeat Name Create rates with range B2.',
 'The redefinition fails with the nonspecific "Command refused". The original binding remains B1.',
 'Permit a range name to be reassigned through Name Create without refusing the operation merely because formulas exist.',
 'Lotus Reference chapter 10, /Range Name Create pp. 10-14..10-18; restriction independently listed in spec/i123/era_shortfalls.md I123-P3-06.',
 ['name-redefine'],'transfers')
add('Erasing an ordinary cell does not recover capacity','Capacity/recovery','P2',
 'Retrieve supplied CAP2047.WK1. Enter 8 in A2048. At capacity, /Range Erase A1 Return; then enter 9 in A2049.',
 'A2049 still fails in EDIT. CAPERA.WK1 still has 2048 cell records, including a default-format BLANK at A1.',
 'Erasing an unformatted cell should free a slot, or offer a reachable way to release it without erasing the entire sheet.',
 'Own capacity-recovery expectation; distinct from earlier L080, which covers the unexplained full-store error itself.',
 ['capacity-after-erase'],'capacity',['evidence/extracted/capacity/CAPERA.json','evidence/extracted/capacity/CAPERA.WK1'],
 'Guess: os/i123/app/face_a.c:752 erases the cell and then recreates its default format as an allocated BLANK.')
add('@NOW returns ERR','Formula/date-time','P2',
 'In a fresh sheet enter @NOW in A1 (the function sweep places it at A25). Press F9.',
 'The formula evaluates to ERR. The saved result is positive infinity, the WK1 ERR sentinel; the desktop clock remains visible.',
 'Return the current date-and-time serial number and update it on recalculation.',
 'Lotus Reference p. 13-46 (PDF 334), @NOW.',
 ['func-25'],'functions',['evidence/extracted/functions/FUNCS.json'],
 'Guess: os/i123/core/eval.c:916 returns ERR when have_now is unset; the native application does not supply a clock value.')
add('Save Backup is offered but cannot create a backup','Files/Save','P2',
 'Save A1=original as SAVE. Change A1 to changed. /File Save SAVE Return, choose Backup.',
 'ERROR says Command not implemented. No SAVE.BAK is created.',
 'Copy the previous SAVE.WK1 to SAVE.BAK and write the new worksheet to SAVE.WK1.',
 'Lotus Reference p. 5-22 (PDF 154), Backup.',
 ['backup-refused'],'files',['evidence/extracted/files/directory.txt'],
 'Guess: os/i123/app/face_a.c:780 routes the third replace-menu choice to unsupported().')
add('The documented password-save syntax is rejected','Files/password','P2',
 'Enter a value, then /File Save; type PASS p, Return.',
 'ERROR says Invalid file name. No password prompt appears.',
 'A space followed by p requests password entry and confirmation.',
 'Lotus Reference pp. 5-22/5-23 (PDF 154/155), Saving a File with a Password.',
 ['password-refused'],'files',guess='Guess: os/i123/app/face_a.c:603 rejects every space before recognizing a password suffix.')
add('Invalid 8.3 names are diagnosed as generic I/O failures','Files/errors','P3',
 'Attempt /File Save with TOOLONGNAME, then 12345678.1234. Clear each notice with Escape.',
 'Both malformed DOS names produce File I/O error; the notice gives no name or length correction.',
 'Identify invalid filename syntax or the 8.3 length limit, as the separate space/wildcard cases already identify invalid file names.',
 'Own error-message usability expectation, supported by the contrasting observed Invalid file name response to A B and BAD?.',
 ['filename-0','filename-5','filename-1'],'files')
add('Three menu descriptions are clipped at the right edge','Menu/polish','P3',
 'Press slash, highlight Range (Right once). Also navigate /Worksheet Global Default Other International Punctuation and highlight B (,..) and D (,;.).',
 'The Range description ends with Sear instead of Search. Each indicated punctuation description loses its last character. All other 378 displayed descriptions match their full reference text.',
 'Keep the full selected-item description readable, for example by scrolling or wrapping it.',
 'Real Lotus installed 123.RI strings at the offsets in evidence/menu-verified.json; exact screen crops y=94..109. Native geometry is four characters narrower than the 80-column reference.',
 ['menu-143-selected','menu-062-selected','menu-064-selected'],'menus',['evidence/menu-verified.json'])
(ROOT/'evidence/custom-findings.json').write_text(json.dumps(findings,indent=2))
