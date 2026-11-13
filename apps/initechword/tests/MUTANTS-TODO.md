# Mutation hooks awaiting patch conversion

75 conditional sites, 73 distinct IWORD_MUT_* macros. This exceeds the
approximately 15-hook conversion limit for this import, so guards remain in
product source and patch conversion is deferred. No mutation sweep is claimed.
Convert all sites for each repeated macro together. Current source locations:

Names in the catcher column are test_iword_<name> entries unless explicitly
prefixed test-flair-. The corpus cases run inside search, replace and undo.

| Location | Macro | Breakage | Catcher |
|---|---|---|---|
| app/face_a.c:158 | IWORD_MUT_SETUP_SHORT | Stops atomic settings save after one short write | setup |
| app/face_a.c:540 | IWORD_MUT_UNDO_BLOCK | Loses the block anchor in a deletion record | undo |
| app/face_a.c:577 | IWORD_MUT_UNDO_LEVEL | Keeps only two undo entries | undo |
| app/face_a.c:591 | IWORD_MUT_UNDO_ORDER | Reverses coalesced deletion byte order | undo |
| app/face_a.c:689 | IWORD_MUT_LINE_SCROLL | Scrolls only at document-line boundaries, skipping blank rows | face F12 |
| app/face_a.c:730 | IWORD_MUT_NO_BLOCK_ON | Omits Block on status | face F8 |
| app/face_a.c:788 | IWORD_MUT_REPLACE_PROMPT | Changes the confirmation prompt | replace |
| app/face_a.c:798 | IWORD_MUT_SEARCH_PROMPT | Changes the search prompt | search |
| app/face_a.c:850 | IWORD_MUT_SEARCH_CURSOR | Hides the matched span highlight | search |
| app/face_a.c:1090 | IWORD_MUT_UNDO_PREVIEW | Omits the undo preview highlight | undo |
| app/face_a.c:1149 | IWORD_MUT_SETUP_VALIDATE | Accepts invalid preference fields | setup |
| app/face_a.c:1153 | IWORD_MUT_SETUP_LOAD | Ignores saved preferences | setup |
| app/face_a.c:1161 | IWORD_MUT_SETUP_SAVE | Forces inches on save | setup |
| app/face_a.c:1253 | IWORD_MUT_SAVE_NOTHING | Writes an empty document | face F1; test-flair-iword-mutant |
| app/face_a.c:1510 | IWORD_MUT_LF_SELECTION | Retains a stale slot after directory resort | listfiles_ops |
| app/face_a.c:1525 | IWORD_MUT_LF_PROTECT | Allows protected or directory targets | listfiles_ops |
| app/face_a.c:1544 | IWORD_MUT_LF_TRUNCATE | Truncates the target before a failed copy | listfiles_ops |
| app/face_a.c:1563 | IWORD_MUT_LF_WRONG_PATH | Operates on WRONG.TXT instead of the selection | listfiles_ops |
| app/face_a.c:1618 | IWORD_MUT_LF_ALIAS | Fails to normalize dot path aliases | listfiles_ops |
| app/face_a.c:1960 | IWORD_MUT_NAME_SPACE_DROP | Drops spaces in Save/Retrieve names | face F17 |
| app/face_a.c:2211 | IWORD_MUT_PGDN_IGNORED | Ignores PgDn | face F16 |
| app/face_a.c:2333 | IWORD_MUT_FKEY_DROP | Drops plain F9 | fkeys |
| app/face_a.c:2362 | IWORD_MUT_FKEY_UNMAPPED | Shows a gray command for unmapped keys | fkeys |
| app/face_a.c:2401 | IWORD_MUT_GOTO_TYPES | Types Go To digits into the document | face F16 |
| app/face_a.c:2531 | IWORD_MUT_REPLACE_CASE | Omits case-preserving replacement | replace; replace-corpus |
| app/face_a.c:2552 | IWORD_MUT_REPLACE_CODE | Skips replacement of attribute codes | replace; replace-corpus MINT06 |
| app/face_a.c:2560 | IWORD_MUT_REPLACE_SKIP | Skips an adjacent replacement candidate | replace |
| app/face_a.c:2587 | IWORD_MUT_SEARCH_CANCEL | Moves the cursor to zero on cancellation | search |
| app/face_a.c:2610 | IWORD_MUT_SEARCH_NOTFOUND | Changes the not-found message | search |
| app/face_a.c:2672 | IWORD_MUT_REPLACE_CONFIRM | Treats No as Yes | replace |
| app/face_a.c:2687 | IWORD_MUT_UNDO_CANCEL | Restores a deletion when cancelled | undo |
| app/face_a.c:2698 | IWORD_MUT_UNDO_FIXUP | Leaves restored tab bookkeeping stale | undo-corpus MINT09 |
| app/face_a.c:2733 | IWORD_MUT_UNDO_MOVEMENT | Coalesces deletions across cursor motion | undo |
| core/block.c:81 | IWORD_MUT_COPY_BARE | Copies bytes without self-contained attribute scopes | codes block model |
| core/block.c:114 | IWORD_MUT_CUT_NO_MEND | Leaves attribute scopes unmended after cut | codes block model |
| core/block.c:152 | IWORD_MUT_RETRIEVE_DUP | Keeps redundant codes on retrieval | codes block model |
| core/block.c:209 | IWORD_MUT_BLK_CUR_START | Always places block-attribute cursor after the off code | codes; reveal_codes; wp5_roundtrip |
| core/codes.c:121 | IWORD_MUT_NO_FACE | Draws formatted text without style bits | face F7 |
| core/codes.c:188 | IWORD_MUT_TOGGLE_DUP | Leaves duplicate attribute-off codes after toggles | codes |
| core/codes.c:273 | IWORD_MUT_PAIR_HALF | Deletes only one half of an attribute pair | codes |
| core/codes.c:328 | IWORD_MUT_LABEL_ITALC | Uses incorrect italic labels | reveal_codes |
| core/codes.c:333 | IWORD_MUT_LABEL_SM_CAP | Uses menu spelling for Small Caps labels | reveal_codes |
| core/codes.c:365 | IWORD_MUT_LABEL_JUST | Labels every justification Left | reveal_codes |
| core/codes.c:409 | IWORD_MUT_BLOCK_LABEL | Omits the block anchor in Reveal Codes | reveal_codes |
| core/codes.c:445 | IWORD_MUT_PANE_ANCHOR | Anchors pane two lines above cursor instead of three | reveal_codes |
| core/edit.c:57 | IWORD_MUT_TAB_FIXED | Makes every tab five columns wide | codes geometry |
| core/edit.c:234 | IWORD_MUT_REFLOW_STALE | Preserves stale soft returns | core reflow model |
| core/edit.c:252 | IWORD_MUT_WRAP_EARLY | Wraps a column too early | wp5_roundtrip typed |
| core/edit.c:256 | IWORD_MUT_WRAP_65 | Uses the obsolete 65-column end rule | wp5_roundtrip typed; codes format |
| core/edit.c:271 | IWORD_MUT_NO_WRAP | Suppresses word wrapping | face F1; test-flair-iword-mutant |
| core/edit.c:417 | IWORD_MUT_NO_CENTER | Draws centered/right text at the left margin | face F11 |
| core/edit.c:659 | IWORD_MUT_NO_ASK | Silently skips protected codes without prompting | codes |
| core/edit.c:721 | IWORD_MUT_ASK_STAY | Leaves the cursor stationary after No | codes |
| core/edit.c:806 | IWORD_MUT_HHH_AFTER | Triple-Home Left stops after leading codes | reveal_codes; codes |
| core/edit.c:949 | IWORD_MUT_NO_D4 | Omits the hard-page D4/00 group | wp5_roundtrip typed |
| core/edit.c:1026 | IWORD_MUT_NO_D4 | Omits the hard-page D4/00 group | wp5_roundtrip typed |
| core/edit.c:1093 | IWORD_MUT_TAB_ABS | Keeps absolute tab positions after margins change | codes format; wp5_roundtrip typed |
| core/edit.c:1119 | IWORD_MUT_OLD_STALE | Keeps obsolete format payload values | codes format; wp5_roundtrip typed |
| core/edit.c:1223 | IWORD_MUT_NO_LEAD | Omits D4/05 leading line records | codes format; wp5_roundtrip typed |
| core/edit.c:1248 | IWORD_MUT_NO_LEAD | Omits D4/05 leading line records | codes format; wp5_roundtrip typed |
| core/edit.c:1321 | IWORD_MUT_MAR_NO_HRT | Inserts margins mid-line without a hard return | wp5_roundtrip typed |
| core/edit.c:1391 | IWORD_MUT_SP_HRT | Inserts an unwanted hard return before line spacing | wp5_roundtrip typed |
| core/edit.c:1414 | IWORD_MUT_SP_NEAR | Rounds stored fractional line spacing to nearest | wp5_roundtrip typed |
| core/edit.c:1424 | IWORD_MUT_SP_CEIL | Rounds displayed fractional spacing upward | face F12 |
| core/edit.c:1464 | IWORD_MUT_SENT_BARE | Leaves trailing spaces outside a sentence block | codes |
| core/listfiles.c:51 | IWORD_MUT_LF_COLUMN | Moves right-aligned directory numbers one column right | listfiles |
| core/listfiles.c:81 | IWORD_MUT_LF_ZERO_DATE | Prints a timestamp for a zero DOS date | listfiles |
| core/piece.c:120 | IWORD_MUT_SPLIT_OFF | Starts a split piece one byte too late | core piece-table model |
| core/piece.c:331 | IWORD_MUT_PREV_VAR | Treats a variable group's last byte as a whole unit | core unit model |
| core/search.c:12 | IWORD_MUT_SEARCH_CASE | Makes lowercase queries case-sensitive | search; search-corpus |
| core/search.c:16 | IWORD_MUT_SEARCH_CODE | Confuses attribute on/off codes | search; search-corpus |
| core/search.c:44 | IWORD_MUT_SEARCH_WRITE | Corrupts the document during read-only search | search; search-corpus |
| core/search.c:48 | IWORD_MUT_SEARCH_BACK | Performs forward search when backward requested | search; search-corpus |
| fs/wp5.c:75 | IWORD_MUT_DROP_CODE | Drops soft returns during serialization | wp5_roundtrip golden-rewrite |
| fs/wp5.c:89 | IWORD_MUT_CURSOR_FLAG | Omits the saved cursor prefix flag | wp5_roundtrip typed |

IWORD_SMALL_BUFFERS is a host compaction test configuration, not a mutant.
The setup-short hook enables SDK's TBX_MUT_SAVE_ONE_WRITE in that translation
unit; TBX_MUT_SAVE_IN_PLACE and other SDK hooks belong to the SDK importer.
The legacy -mutant host gates for core, codes, wp5-roundtrip, reveal-codes,
face, fkeys, search, replace, undo, setup, listfiles and listfiles-ops remain
retired. Their named faults/catchers are preserved here for patch conversion.
make mutants deliberately fails until a real patch runner is wired.
