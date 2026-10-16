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
