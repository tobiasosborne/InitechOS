# IWORD P3: complete key dispatch and read-only Search (initech-qvgd)

This lane leaves a partial P3 implementation for PM review, uncommitted. The
48-slot template and read-only Search are implemented. Replace, Undelete,
List Files and persisted Setup remain period-grayed, pending the byte-level
reference captures described below. The kernel, CLAUDE.md, HANDOFF.md, beads
and Git metadata were not changed.

## Local ground truth

Read CLAUDE.md in full, InitechOS-PRD.md sections 6.1/6.3/6.5/8, the IWORD
plan P3, the initech-qvgd JSONL record (bd show found no database), the jojk
entries in WL-0114/WL-0131, and the mint-results-001..004 ledgers in the
read-only wordperfect51-decomp sibling.

WordPerfect 5.1 Reference, local
`initech-manuals-ref/wordperfect51/WPDOS51Reference.pdf`, printed pages:

| Pages | Contract used |
| --- | --- |
| 5-6 (PDF 22-23) | Template and the plain/Shift/Alt/Ctrl rows |
| 963-964 (PDF 980-981) | All function-key command names |
| 966, notes 3-4 (PDF 983) | F11/F12 aliases, six unmapped modified slots |
| 571-573 (PDF 588-590) | Search direction, case rule, code tokens, editing a query, repeat Search |
| 551-553 (PDF 568-570) | Replace and confirmation, inspected for the deferred item |
| 697 (PDF 714) | Three deletion levels and code restoration caveat, inspected for the deferred item |
| 347-353 (PDF 364-370) | List Files, inspected for the deferred item |
| 574-575 (PDF 591-592) | Setup persistence and menus, inspected for the deferred item |

Rendered and viewed the key table, notes, Template, Search, Replace,
Undelete, List Files and Setup pages at 110 dpi; also viewed the local
`WordPerfect 5.1 Manual/WP51 Funct Strip.jpg`. Prompt spellings are the
operator-specified Reference transcriptions: `-> Srch:`, `<- Srch:` and
`* Not found *`.

Existing byte authority is MINT01..28, MINT24A, MINT30..32, read from
goldens/minted or mint/work/WP51. MINT29 is the spoiled session identified
by mint-results-004.md and is deliberately excluded. No new WP output
goldens were synthesized or claimed.

The FAT differential uses the existing MILTON fat_dump reader against
mtools on identical images, with dosfstools checking allocation integrity.
ADR-0003 Appendix A supplies READ / FINDFIRST / FINDNEXT / GETSPACE's
sanctioned surface. IBM DOS 3.30 Technical Reference printed 6-137
(PDF 260, rendered/viewed) defines byte-preserving file reads. Those sources
establish the I/O check; mtools does not establish WordPerfect semantics.

## Changes and oracles

`spec/iword/fkeys.json` locks all 48 names and dispatch actions. The factory
C generator emits `fkeys.h` deterministically. The tenant dispatches through
the generated table. Already implemented commands remain live; unavailable
commands display their manual name in gray; unmapped slots have no invented
command name. F11/F12 alias Reveal Codes/Block, including with Block on.
Shift/Alt/Ctrl-F7 no longer accidentally invoke plain F7 Exit, and modified
F10 does not accidentally Save. The host oracle independently transcribes
every name and drives every slot through the real event handler. The QEMU
trace drives the whole template and checks actual gray draws and pixels.

Search has a separate editable unit stream; F6/F8 insert on/off attribute
tokens rather than document code pairs. Left, Right, Backspace and Delete
edit complete query units. F1 cancels; F2/Shift-F2/Esc execute; Up/Down
select direction. F2 twice repeats the retained query. Lower-case query
characters match either case, uppercase query characters are literal.
Search returns whole-unit spans without changing document bytes or dirty
state. The native 76-column query field scrolls to keep its cursor visible.

The corpus differential independently walks the actual WP bytes and derives
match positions without the artifact's decoder/search helpers. It exercises
text and attribute tokens in both directions from multiple positions over
32 real documents. MILTON-extracted bytes equal mtools-extracted bytes;
the two FAT images remain identical; fsck.fat is clean. The UI test drives
the actual tenant over the existing recording Toolbox mock. The emulator
checks the forward code match at Pos 1.6, backward text match at Pos 1,
exact prompts, message pixels, registration and desktop survival.

Host and QEMU gates were written and observed RED before implementation.
Initial host template: 37 failures; initial host Search: 14 failures.
Initial QEMU reasons: F9 End Field not dispatched; Search query/code/
direction/not-found UI missing. The query cursor was separately observed
RED before its rendering was added. Named compiled mutants exercise each
implemented rule. Every new primary gate and mutant aggregate is included
in TEST_UNIT_GATES or TEST_EMU_GATES.

New targets:

```
test-iword-fkeys
test-iword-fkeys-mutant
test-iword-search
test-iword-search-mutant
test-flair-iword-fkeys
test-flair-iword-fkeys-mutant
test-flair-iword-search
test-flair-iword-search-mutant
test-flair-iword-search_back-mutant
test-flair-iword-search_notfound-mutant
```

The last two are prerequisites of the emulator Search mutant aggregate.
`record-flair SCRIPT=iword_search` is registered before record-flair and
uses the same locked Search trace. Gate logs are in `build/iword_p3_report`;
the clip is `build/clips/iword_search.{gif,mp4}` when capture completes.

## Exact remaining reference acquisitions

The Reference establishes these commands' UI, but the existing corpus does
not establish the required resulting bytes. Per the lane's mint rule,
execution of these commands is stopped individually. Their key names are
dispatched and grayed; P3 as a whole is not claimed complete.

| Deferred item | Required real WP 5.1 mint |
| --- | --- |
| Replace (Alt-F2) | A document before/after replacing plain text with and without confirmation, No/Yes/Cancel at successive matches, case-preserving replacement, deleting/replacing paired codes, and saves with cursor flags. Capture every prompt. |
| Cancel/Undelete (F1) | Four distinct deletion groups separated by cursor motion; Restore and Previous Deletion through all three retained levels; contiguous Backspace/Delete coalescing; attribute-pair versus block-deletion restoration; saved files and highlighted previews. |
| List Files (F5) | A known FAT image with exact free/used header values, two-column short display and 1-9 menu; Retrieve and Look of existing MINT documents, plus the resulting save. Do not invent free-space accounting. |
| Setup (Shift-F1) | Units/Backup choices, exit/relaunch persistence, and the before/after settings files. The Reference names WP{WP}.SET but does not define its bytes. |

Search reduction: query entry currently supports text, F6/F8 and returns;
the remaining code selectors, extended/block searches and wildcard Ctrl-X
are not implemented. The fixed query capacity is 256 body bytes; text beyond
it is refused. No claim is made that this reduction is WordPerfect's limit.

No Bochs run or full `make test` was performed, as directed. All QEMU gates
use `INITECH_QMP_STDIO=1`. No commit, push, bead update or sibling-corpus write.

## Final acceptance evidence

All host and QEMU IWORD family targets, including their mutants, passed.
The corpus oracle made 13,944 independent span comparisons. The template
oracle drove 48/48 slots with 162 checks and zero failures. QEMU template
and Search observers passed 30 and 4 checks respectively. The larger shipped
volume passed the launch, locked-purge and Finder-service gates without any
re-key of their assertions. Appswitch, C tenant, tenant menu and modifier
release passed too. Fixture-volume declarations and Makefile ordering passed.

The first legacy typing emulator run, while host property tests were also
running, lost input (MEM instead of MEMO). Once the host family finished,
the unchanged legacy trace passed sequentially, including its byte-exact MEMO
comparison. Remaining QEMU runs were sequential. No assertions were relaxed.
The new all-function-key trace has an explicit hold at the Font-menu exit to
let the guest consume its key transitions before Ctrl-F9.

Both iword_search GIF/MP4 captures were byte-identical. Inspected the positive
QEMU screendump, a query frame (including the code label and query cursor),
and the final clip frame. The native blue editor, body cursor and exact
not-found message are visible. Clip: build/clips/iword_search.gif and .mp4.
The source ASCII scan found no non-ASCII bytes.

Terminal GREEN/PASS lines (logs: build/iword_p3_report):

```text
>>> test-iword-core: green
>>> test-iword-core-mutant: green (all three mutants RED for the named reason)
>>> test-iword-wp5-roundtrip: green
>>> test-iword-wp5-roundtrip-mutant: green (all twelve mutants RED for the named reason)
>>> test-iword-codes: green
>>> test-iword-codes-mutant: green (all twelve mutants RED for the named reason)
>>> test-iword-reveal-codes: green
>>> test-iword-reveal-codes-mutant: green (all seven mutants RED for the named reason)
>>> test-iword-face: green (MEMO byte-identical to the expected file; MEMO/COPY/GUARD/BIG/CODES/TABS read clean by wp5_ref.py; CODES carries bold/underline/italic pairs; the body of TABS == MINT09 (300 bytes))
>>> test-iword-face-mutant: green (all seven mutants RED for the named reason)
>>> test-iword-fkeys: green
>>> test-iword-fkeys-mutant: green (FKEY_DROP and FKEY_UNMAPPED RED)
>>> test-iword-search: green
>>> test-iword-search-mutant: green
>>> test-flair-iword: green
>>> test-flair-iword-mutant: green (both mutants RED for the named reason)
>>> test-flair-iword-codes: green
>>> test-flair-iword-codes-mutant: green (both mutants RED for the named reason)
>>> test-flair-iword-block: green
>>> test-flair-iword-block-mutant: green (both mutants RED for the named reason)
>>> test-flair-iword-format: green
>>> test-flair-iword-format-mutant: green (both mutants RED for the named reason)
>>> test-flair-iword-fkeys: green
>>> test-flair-iword-fkeys-mutant: green (FKEY_DROP and FKEY_UNMAPPED RED)
>>> test-flair-iword-search: green
>>> test-flair-iword-search_back-mutant: green (SEARCH_BACK RED)
>>> test-flair-iword-search_notfound-mutant: green (SEARCH_NOTFOUND RED)
>>> test-flair-iword-search-mutant: green (both named mutants RED)
>>> image data: build/flair_data.img (FAT12 primary-slave volume; README.TXT + APPS)
>>> test-flair-app-launch: green
VERDICT: PASS -- test-flair-locked-purge (locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat)
>>> test-flair-finder-service: green
>>> test-flair-ctenant: green
>>> test-flair-tenant-menu: green
>>> test-flair-modifier-release: green
>>> test-fixture-volumes: green (281 scratch names vs 128 fixture names; 68 inline image writers)
>>> test-makefile-vars: green
>>> record-flair-repro [iword_search]: PASS -- two captures byte-identical
>>> test-flair-appswitch [4/4]: grader PASS (DISTINCT-CHIMERA + POST-DISTINCT + TIER-A overlap + TIER-B accent + BAR1-STATIC + MENU-BAND swap)
```

Named mutants (IWORD_MUT_ prefix), all observed RED with normal summaries:

```text
FKEY_DROP: missing gray key row 0 F9 'End Field'
FKEY_UNMAPPED: FAIL template manual name or unchanged unmapped status
SEARCH_BACK: FAIL search backward capital query excludes lower-case jim
SEARCH_CASE: FAIL search lower-case query finds capitalized Jim
SEARCH_CODE: FAIL search Bold Off cannot match a Bold On query
SEARCH_WRITE: RED-REASON search-corpus: document bytes changed
SEARCH_PROMPT: FAIL search forward prompt
SEARCH_CANCEL: FAIL search Cancel preserves cursor
SEARCH_NOTFOUND: FAIL search not-found message
SEARCH_CURSOR: FAIL search query cursor follows the complete code label
```

SEARCH_BACK/CASE/CODE/WRITE also failed the independent corpus oracle.
QEMU FKEY_DROP lacked End Field; FKEY_UNMAPPED cleared the status line;
SEARCH_BACK failed the backward cursor
position; SEARCH_NOTFOUND failed the exact not-found message. Initial RED
exhibits are preserved in build/iword_p3_report/initial-*-red.log. Bead remains
unchanged/open for the PM; the four deferred command executions above are not
done and are not covered by the GREEN claim for the delivered subset.

Final Reference correction: p. 966 note 4's six unmapped combinations are
ignored; they do not draw an invented empty gray command. The tightened host
oracle was first RED with 12 failures, and QEMU was RED with "unmapped keys
do not clear the status line". FKEY_UNMAPPED reinstates the wrong clearing
behavior and is checked on host and QEMU. The affected host face/keyboard/
search tests and the full emulator family, volume checks and clip were rerun
after this correction; the document-core sources and their earlier GREEN
property/mutation runs were unchanged.

## Part 2 -- third-launch coding lane (initech-qvgd)

Preserved Part 1's complete template and Search. Added Replace, the three
deletion groups, and minimal persisted Setup. No kernel, Git metadata,
CLAUDE.md, HANDOFF.md, beads or sibling-corpus changes; no commit or push.

Law-1 sources read locally and rendered/viewed at 110 dpi:

| Source | Printed pages / contract |
| --- | --- |
| WP 5.1 Reference | 551-553: Replace direction, case preservation, code tokens, confirmation and editable strings |
| WP 5.1 Reference | 697-698: three deletion groups, cursor-motion boundary, reverse-video preview, Restore at cursor, paired-code caveat |
| WP 5.1 Reference | 42, 160, 574-575, 698-700: Setup, Environment, Backup Options, two independent unit selections and persistence |
| WP 5.1 Reference | 347-350: List Files furniture and the unresolved disk-versus-directory accounting |
| IBM DOS 3.30 Technical Reference | 6-137/138 (PDF 260/261): READ and actual transfer counts; 6-139/140 (PDF 262/263): WRITE and actual counts |
| ADR-0003 Appendix A | 3Ch/3Dh/3Eh/3Fh/40h file calls; 36h/4Eh/4Fh are the sanctioned enumeration/space surface |
| Real WP corpus | MINT01..28, MINT24A, MINT30..32; existing P2 code insertion, block copy/retrieve and fixup, not newly invented WP output goldens |

The PM explicitly authorized an Initech settings record. `spec/iword/settings.h`
locks IWORD.SET: eight bytes, IWS1, entry unit, status unit, original-backup
choice, reserved zero. This does not claim WP{WP}.SET interoperability. The
minimal menu persists these preferences; timed backup and the rest of Setup
remain unavailable. Automatic backup production and measurement rendering
are outside this preference-persistence implementation.

Replace is a separate modal flow: w/Confirm, editable search tokens, editable
replacement, then per-match No/Yes/Cancel or replace-all. Forward/backward
continuation excludes inserted text. Lowercase replacement preserves initial
capitalization or all capitals. Empty replacement deletes literal code units;
Tab, Indent and Hard Page replacements use P2's established code bytes and
fixup. F6/F8 select search tokens. Appearance replacements requiring the
Alt-R macro (Reference p. 553) and the unimplemented code selectors are not
claimed. The original WP.EXE string table corroborates `Replace with:`,
`w/Confirm`, the directional Srch strings and the not-found spelling.

Undelete records Backspace/Delete and block deletions. Cursor movement starts
a new group; the fourth group evicts the oldest. Preview borrows the spare
file bank and never edits the live document. Restore runs the P2 insertion
and fixup path. Paired attribute codes deleted with Backspace/Delete are not
restored, as p. 697 specifies. Each retained group is capped at 32 KiB to fit
the existing 512 KiB tenant BSS contract; overflow is refused before deleting.
There is no disk spill. Initial over-allocation was caught by the unchanged
tenant loader, then corrected without changing the kernel or its limit.

The host UI gates and injected QEMU gates were written and observed RED
before their implementation. The settings short-transfer test was separately
RED before the read/write loops. Block capture and restored tab geometry
each produced a concrete RED; named UNDO_BLOCK and UNDO_FIXUP mutants retain
those failures. The latter was caught on MINT09 after a deleted letter moved
the tab stop: Restore must recompute the coordinates after insertion.

The new host corpus oracle applies replacements to every eligible text/code
unit in 32 real documents and compares against the old piece-table primitive
model. Undelete deletes/restores every lowercase character and demands the
original body bytes, including code payloads. Inputs are MILTON- and
mtools-extracted from identical FAT images; fsck.fat verifies allocation.
The settings oracle compares captured tenant bytes through the same FAT
read differential. No Replace/Undelete-specific real-WP mint exists in the
current corpus; model checks are not described as newly minted WP goldens.

New primary/aggregate targets, all in TEST_UNIT_GATES / TEST_EMU_GATES:

```
test-iword-replace              test-iword-replace-mutant
test-iword-undo                 test-iword-undo-mutant
test-iword-setup                test-iword-setup-mutant
test-flair-iword-replace        test-flair-iword-replace-mutant
test-flair-iword-undo           test-flair-iword-undo-mutant
test-flair-iword-setup          test-flair-iword-setup-mutant
```

Registered iword_replace, iword_undo and iword_setup before record-flair,
alongside the existing iword_search clip. Logs and RED exhibits are under
`build/iword_p3_part2_report`.

List Files is stopped on its required byte-level reference acquisition. The
Reference p. 350 explicitly says used space can mean disk or current-directory
usage, and the MINT corpus contains no F5 capture. Required mint: a known FAT
image with recorded image bytes, exact two-column header/free/used/file-count
values, selected entry, Look (including scrolling), Retrieve and resulting
save, and prompts/menu. The kernel's DOS functions establish enumeration
and allocation counts, not which quantity WP prints. F5 retains its Part 1
period-grayed dispatch; no fake listing or guessed accounting was added.

Additional Part-2 acceptance details: Search-induced cursor motion also ends
a deletion group (Reference p. 697). The host regression was first RED when
Search went away and back before another Backspace. UNDO_MOVEMENT recreates
that bug. The full locked Undo emulator trace now checks the same case,
saving MOTION with body `abc`; its mutant aggregate boots both UNDO_LEVEL
and UNDO_MOVEMENT. The host corpus made 3,534 Replace checks and 1,769
Undelete checks across 32 real documents. REPLACE_CODE also goes RED on the
independent MINT06 piece-table model; UNDO_FIXUP goes RED on MINT09 bytes.

The first long Replace clip failed reproducibility solely at the live clock's
minute rollover (frame 80; the deterministic-RTC attempt exposed the same
boundary at frame 59). Full-frame before/after PNGs and failure logs remain
in the report directory. The full acceptance traces remain intact. Separate
short demo traces show confirmed code replacement, three deletion levels,
and Setup preferences; they retain the visible clock and the strict full-frame
byte-equality check. The new clips use the existing per-script deterministic
RTC option (the menubar_clock pattern; harness/emu/qemu.c 1418ff). No clock
pixels are masked and no oracle tolerance changed.

All four captures passed `record-flair-repro`: iword_search, iword_replace,
iword_undo and iword_setup. Inspected the paired-code query, Replace
confirmation, reverse-video Undelete preview and Setup's Centimeters/Points
frame. GIF and MP4 files are under `build/clips/`.

Final Part-2 runs are GREEN. Full host family: `host-family.log`; after the
cursor-motion fix, every affected face/key/Search/Replace/Undo/Setup host
gate and mutant was rerun (`final-face-family.log`, `part2-final-host.log`).
The core and codec sources did not change. The entire emulator IWORD family,
including all old and new mutants, was rerun on the frozen source, followed
by image, app-launch, locked-purge, Finder-service, appswitch, C tenant,
tenant-menu and modifier-release checks (`final-emulator-volumes.log`).
Fixture contracts and Makefile ordering are green: 37 declared flair-data
images, 288 scratch names versus 132 fixture names. New-source ASCII scans
are empty. All QEMU commands exported INITECH_QMP_STDIO=1; no Bochs or full
`make test` run.

Terminal tails for the twelve added gates:

```
>>> test-iword-replace: green
>>> test-iword-replace-mutant: green
>>> test-iword-undo: green
>>> test-iword-undo-mutant: green
>>> test-iword-setup: green
>>> test-iword-setup-mutant: green
>>> test-flair-iword-replace: green
>>> test-flair-iword-replace-mutant: green (REPLACE_CONFIRM correctly RED)
>>> test-flair-iword-undo: green
>>> test-flair-iword-undo-mutant: green (UNDO_LEVEL and UNDO_MOVEMENT correctly RED)
>>> test-flair-iword-setup: green
>>> test-flair-iword-setup-mutant: green (SETUP_LOAD correctly RED)
```

Sixteen named host mutants (IWORD_MUT_ prefix), all rejected with normal
test summaries. Relevant RED lines:

```
REPLACE_CASE: FAIL replace case-preserving replace all
REPLACE_SKIP: FAIL replace replacement is not searched again
REPLACE_CODE: FAIL replace empty replacement removes complete paired code tokens
REPLACE_CONFIRM: FAIL replace confirm No Yes Cancel affect only accepted match
REPLACE_PROMPT: FAIL replace confirmation choice prompt
UNDO_LEVEL: FAIL undo third retained group survives fourth deletion
UNDO_ORDER: FAIL undo group preview is in reverse video at cursor
UNDO_CANCEL: FAIL undo Cancel removes preview without inserting
UNDO_PREVIEW: FAIL undo group preview is in reverse video at cursor
UNDO_BLOCK: FAIL undo recording a block deletion preserves the selected span
UNDO_MOVEMENT: FAIL undo Search cursor motion separates deletion groups even when cursor returns
UNDO_FIXUP: MINT09 deletion at 47, first diff 54, got 08 expected 60, length 300/300
SETUP_SAVE: FAIL setup locked settings bytes persisted
SETUP_LOAD: FAIL setup units survive relaunch
SETUP_VALIDATE: FAIL setup invalid record uses defaults
SETUP_SHORT: FAIL setup locked settings bytes persisted
```

REPLACE_CODE also failed the real MINT06 corpus comparison. QEMU rejected
REPLACE_CONFIRM, UNDO_LEVEL, UNDO_MOVEMENT and SETUP_LOAD; the movement
mutant's exact line was `FAIL undo emulator Search cursor motion separates
deletion groups (line 174)`. All four clip repro runs ended with
`PASS -- two captures byte-identical`. Changes are left uncommitted for PM
review; the F5 mint described above is still required.

## Part 3 -- List Files (F5), lane iwordp3 (initech-qvgd remainder; 2026-10-20)

Context. List Files was the last P3 acceptance item, blocked on a real WP 5.1
mint of the directory screen. This lane minted it and implemented it.

The mint (../wordperfect51-decomp mint/work/LISTF/mint-results-005.md; the
lane may write only under mint/work, so the ledger, steps, shots, grids and
fixture all live there -- filing into goldens/ and re/ is the operator's).
Real WP.EXE under DOSBox-X on a byte-reproducible FAT12 fixture volume
(mkfixture.sh: names, sizes, attributes and stamps fixed; DOS clock pinned to
03/01/1992 09:00, the InitechOS harness's RTC default). Every shot pair is
transcribed cell-exactly (decode2.py: glyphs matched exactly against DOSBox-X's
own font atlas, colours per cell, the blinking cursor resolved). Facts it
settled that the Reference left open: header row 1 = Document size / Free
(the volume's free bytes) / Used (sum of the listed FILES' sizes, hidden
included) / Files (files only); directories first then files, row-major;
hidden entries listed; the selection bar and the cursor; Right/Left wrap
between rows; Enter on a directory puts "Dir <path>" under the list and the
new list opens on ".. Parent"; Esc leaves Look but does nothing on the list;
F7 leaves the list; "Retrieve into current document? No (Yes)"; a ZERO DOS date
prints no stamp at all (the kernel-made DESKTOP.DB / TRASH); WP's two-digit
year is '0' + (year-1900)/10, so a 2026 clock prints "<6". Fixture v3 makes the
mint volume's root the booted InitechOS data volume's root entry for entry, so
the gate's root listing is graded too. Document size for a non-empty document
is NOT resolved (retrieved files show size + 193 in three cases, typed
documents differ): InitechWord shows 0 for the empty document (minted) and
size + 193 otherwise [inferred], a GAP in the ledger.

What changed.
- spec/toolbox_gate.h Sec 10a (Rule 8, deliberate): TBX_FILE_FIND 0x0087 (a
  whole FINDFIRST/FINDNEXT walk per call, 22-byte records = the DTA's found
  tail) and TBX_DISK_FREE 0x0088 (AH=36h); os/flair/tbxgate.c implements them
  over the tenant's shadow PSP / root CWD with the kernel DTA swapped and
  restored; tbx.h/tbx_rt.c wrappers + tbx_get_clock (the existing TBX_CLOCK);
  test_flair_routing's callable-code list and stubs updated.
- spec/iword/listfiles.h (new, locked): every column, label, bright mask and
  the period-gray item table from the mint grids, and the stated 80->76 /
  25->24 adaptation (delete WP columns that are blank in every row of that
  kind: header row 1 59-60; list rows 0, 22, 39, 63; F5 prompt 46-48; Look
  header 50-53; the list's and Look's last row dropped).
- spec/iword/fkeys.json (Rule 8 note): F5 gray -> list; FLAIR_IWORD_FKEYS_SPEC
  gains kf1 after kf5 (F5 now opens a prompt); test_iword_p3 template/serial
  oracles updated to match.
- os/iword/core/listfiles.c (new, pure): rows composed at WP's 80 columns and
  fitted; os/iword/app/face_a.c: F5 prompt, the list, the Dir prompt, Look
  (a borrowed read-only editor, the M_UNDO pattern; text files as lines),
  Retrieve (onto an empty document at once, else the question), keys as
  minted; 2 3 4 5 7 8 9 N drawn GRAY and ignored. take_name accepts DIR\NAME
  so a document retrieved from a subdirectory saves back there.

Oracles. test-iword-listfiles: the real face over the recording mock serving
the fixture's directory, 25 keys of the mint session LF14 and 3 host-only
checks, every screen against the mint grid of the same key -- the test maps
80x25 to 76x24 with its OWN statement of the deletes and first checks each
deleted cell is blank on the real screen; colours: red -> BLACK on GRAY,
bright -> bold, the unperformed items GRAY. test-flair-iword-listfiles: the
same trace in QEMU on the stock volume + the mint's A:\DOCS (bytes and stamps
copied out of the md5-pinned mint volume), the TENANT-GATE DRAWCELLS trace
replayed between delivered keys and graded by the same code; Free is mdir's
figure of the booted copy, the clock is exact because the RTC is pinned at
09:00:00 and the run dies at a 30 s wall deadline. The mint inputs are pinned
by harness/diff/wp5_diff/listfiles_mint005.md5.

TDD. Both oracles were RED before the implementation: the host oracle built
over the unchanged main face (546e5bc2) failed 343 of 1926 checks, first
"LF13_01 row 23 == WP row 24" (the gray "List" where WP draws the prompt;
build/iword_lf_report/initial-host-red.log); the emulator leg replaying a
boot of main's WORD.EXE failed 312 of 1709 (initial-emu-red.log). Mutants:
LF_COLUMN (every right-aligned figure one column right) RED on host and in
QEMU at LF14_02 row 1; LF_ZERO_DATE (a zero date printed) RED on host at
LF14_02 row 5.

Acceptance (logs: build/iword_lf_report/; INITECH_QMP_STDIO=1; no Bochs, no
full make test):
```
>>> test-iword-listfiles: green          (1926 checks, 28 mint screens)
>>> test-iword-listfiles-mutant: green   (LF_COLUMN, LF_ZERO_DATE RED)
>>> test-flair-iword-listfiles: green    (1709 checks, 25 keys, 25 mint screens)
>>> test-flair-iword-listfiles-mutant: LF_COLUMN correctly RED
>>> test-kernel-size-r4: green; test-flair-routing(+mutant): green; test-tbx-file: green
>>> test-fixture-volumes: green; test-makefile-vars: green; test-flair-app-launch: green
```
Every test-iword-* host gate and mutant green; test-flair-iword, -codes,
-format, -fkeys, -search, -replace, -undo, -setup and their mutants green
(the iword and search mutant aggregates after a re-run alone).
test-flair-iword-block was green in the sequential batch, then RED twice
alone at 2/5 KEYS with FLAIR-EVT-DROP (the guest's event ring overran under
load ~12): the UNCHANGED main tree (546e5bc2, built in a scratch export) fails
the same gate the same way at the same moment -- environmental, not this
change; its mutant aggregate is therefore not re-confirmed here.

Not done. The Rule-14 clip (record-flair SCRIPT=iword_listfiles, registered
with the gate's trace, volume and RTC pin) could not be captured: under the
current load no InitechWord clip records -- the Finder double-click never
opens WORD -- and the existing iword_search / iword_setup clips fail the same
way, also on the unchanged main tree. Typing a path / "=" at the Dir prompt,
Look's 1 Next / 2 Prev, Retrieve of DOS text files (WP converts them;
InitechWord says Incompatible File Format) and the List Files items 2-5, 7-9,
N are not performed. MEMO's centered line (Shift-F6 Center, period-gray) is
why the gate retrieves LETTER.WP5, not MEMO.
