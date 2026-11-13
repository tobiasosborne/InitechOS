# Mutation hooks awaiting patch conversion

Product source is unchanged. Each row is an actual preprocessor hook location;
repeated macros must be converted together. Test names without a prefix denote
`test_i123_<name>` host entries (see Makefile); hyphenated names are deferred
emulator gates. The current legacy host mutant runners remain retired; this is
an inventory, not a claim that their replacement mutation sweep has run.

| Location | Macro | Breakage | Intended catcher |
|---|---|---|---|
| app/face_a.c:131 | I123_MUT_ENTRY_75 | Truncates entry at 75 characters | entry; test-flair-i123-entry |
| app/face_a.c:185 | I123_MUT_CLOCK_MISSING | Omits guest clock callback/read | clock |
| app/face_a.c:191 | I123_MUT_CLOCK_FROZEN | Keeps the first clock reading | clock |
| app/face_a.c:195 | I123_MUT_CLOCK_MISSING | Omits guest clock callback/read | clock |
| app/face_a.c:390 | I123_MUT_WIDE_HIDDEN | Hides a column wider than the viewport | wk1_roundtrip, resize; test-flair-i123-wide |
| app/face_a.c:436 | I123_MUT_FIXED_GEOMETRY | Ignores current window content size | resize; test-flair-i123-resize |
| app/face_a.c:626 | I123_MUT_GOTO_FOLLOW | Does not reposition the viewport for GOTO | keys K8 |
| app/face_a.c:638 | I123_MUT_SAVE_FAIL_QUITS | Quits after a failed Save | quit Q8 |
| app/face_a.c:698 | I123_MUT_GET_CLOBBER | Malformed retrieve clobbers the active sheet | test-flair-i123-file-ui-mutant |
| app/face_a.c:727 | I123_MUT_SAVE_EMPTY | Saves an empty sheet | test-flair-i123-files-mutant |
| app/face_a.c:938 | I123_MUT_PERIOD_TEXT | Treats POINT period as typed text | range_menu; test-flair-i123-copy-mutant |
| app/face_a.c:995 | I123_MUT_ABSENT_UNPROTECTED | Treats absent cells as unprotected despite global protection | cells C9; copy_move protection model |
| app/face_a.c:1017 | I123_MUT_CURRENCY_REJECT | Rejects currency-prefixed numeric entry | entry (saved CURRENCY.WK1) |
| app/face_a.c:1025 | I123_MUT_NO_RECALC | Omits recalculation after committing entry | test-flair-i123-formula-mutant |
| app/face_a.c:1044 | I123_MUT_NEVER_DIRTY | Does not mark edited sheets dirty | quit Q2 |
| app/face_a.c:1078 | I123_MUT_ARROWS_DEAD | Ignores arrow keys | test-flair-i123-mutant |
| app/face_a.c:1117 | I123_MUT_TAB_ONE_CELL | Moves Tab one cell rather than a screen | keys K18 |
| app/face_a.c:1193 | I123_MUT_EDIT_ESC_CANCELS | First EDIT Escape cancels rather than erasing edit line | keys K1 |
| app/face_a.c:1264 | I123_MUT_HIT_IGNORES_SCROLL | Maps mouse cells without viewport offsets | hit H5 |
| app/face_a.c:1292 | I123_MUT_PENDING_IGNORED | Pending entry does not count as unsaved work | quit Q5 |
| app/face_a.c:1321 | I123_MUT_CANCEL_QUITS | Cancel in exit alert quits | quit Q3 |
| core/eval.c:1257 | I123_MUT_ROUND_EVEN | Rounds halves to even instead of away from zero | formula MINT03 cache |
| core/fmt.c:198 | I123_MUT_GENERAL_ROUNDS | Rounds rather than truncating General display | wk1_roundtrip displayed-value rows |
| core/fmt.c:511 | I123_MUT_WIDE_HIDDEN | Hides a column wider than the viewport | wk1_roundtrip, resize; test-flair-i123-wide |
| core/formula.c:122 | I123_MUT_ERR_LOAD | Loaded ERR references resolve as relative minus one | copy_corpus MOVE01C |
| core/formula.c:130 | I123_MUT_ROW_ERR | Out-of-sheet relative rows become ERR instead of wrapping | copy_corpus WRAP01C |
| core/formula.c:525 | I123_MUT_PREC_FLAT | Gives binary operators equal precedence | formula typed/round-trip |
| core/menu.c:32 | I123_MUT_MENU_SPELLING | Misspells Worksheet as Vorksheet | menu_tree; test-flair-i123-file-ui-mutant |
| core/menu.c:74 | I123_MUT_MENU_SPELLING | Misspells Worksheet as Vorksheet | menu_tree; test-flair-i123-file-ui-mutant |
| core/range.c:129 | I123_MUT_MOVE_COL14 | Rewrites moved relative columns with 14-bit rather than 8-bit offsets | copy_corpus MOVE01C |
| core/range.c:134 | I123_MUT_ERR_WORD | Encodes invalid reference corner differently | copy_corpus MOVE01B |
| core/range.c:157 | I123_MUT_COPY_ABSOLUTE | Shifts absolute references when copying | copy_move absolute-relative model; copy_corpus COPY01B |
| core/range.c:186 | I123_MUT_COPY_RECT_REFUSED | Refuses Copy destination rectangles | copy_tile SINGLE/ROW/OVER/BLOCK |
| core/range.c:210 | I123_MUT_ABSENT_UNPROTECTED | Treats absent cells as unprotected despite global protection | cells C9; copy_move protection model |
| core/range.c:245 | I123_MUT_MOVE_STALE | Does not retarget other formulas after Move | copy_move move-referrers; test-flair-i123-menu-mutant |
| core/recalc.c:136 | I123_MUT_CACHE_ROUND | Rounds recalculated formula caches | formula poisoned-cache file differential |
| core/recalc.c:218 | I123_MUT_NO_PRECEDENTS | Skips natural-order precedent traversal | recalc_order |
| core/recalc.c:361 | I123_MUT_CIRC_BLIND | Suppresses CIRC detection | recalc_order |
| core/sheet.c:252 | I123_MUT_ALLOC_BEFORE_RECLAIM | Allocates replacement bytes before reclaiming old cell | cells C5 |
| core/sheet.c:417 | I123_MUT_DEFAULT_BLANK | Erasing a default-format cell retains a populated BLANK | cells C1 |
| fs/wk1.c:381 | I123_MUT_LABEL_OPCODE | Writes LABEL records with the wrong opcode | wk1_roundtrip independent reader |

The SDK header tbx_save.h also contains TBX_MUT_SAVE_IN_PLACE; that hook belongs
to the SDK importer and is caught by the shared fullsave gate. I123_MENU_DISK,
I123_MENU_HOST and I123_TEST_X87_CW are functional/test configuration, not mutants.
