# SAMIR source triage - 2026-10-23

## WRONG attributions and evidence corrections

No L-id's observed failure is dismissed as correct behavior. L014's attribution
specifically to PACK with two indexes is WRONG: the failing trace runs REINDEX
immediately after PACK; the passing one-index control runs GO TOP and LIST first.
PACK clears navigation at os/samir/cmd/mutate.c:980. REINDEX closes its master
index at os/samir/fs/ndx.c:2671 before collecting keys. The d_key callback
(cmd/mutate.c:1093) calls wa_nav_goto, which materializes the cold master-order
cache at cmd/nav.c:344 through the closed index. The provider error leaves fd=-1
(fs/ndx.c:2679), explaining the later live-state errors. The control establishes
different cache state, not safety with one index. Keep L014 as D, with the
corrected root cause in B01. Compare the full paths
docs/audits/2026-10-23-samir/evidence/pack-two.json and pack-two-serial.log with
pack-index.json and pack-index-serial.log in that same evidence directory.
Neither trace establishes persisted index corruption.

L005's guessed cause also needs correction: os/samir/fs/dbt.c:521-531 already
refreshes the DBT high-water header. The separate writer extends the file
(fs/dbf.c:1763-1770); the retained Milton reader holds the old file size
(os/milton/fileio_fat.c:39-43; int21.c:3374 passes its cached dir_entry).
cmd/workarea.c:903-905 hides the short read as an empty memo. Persisted text is
correct. B03 owns this new read-after-write defect, rather than repeating the
older absent-writer portion of initech-ipu5.8.1.

L017 has a confirmed missing command-boundary check: the independent catalog
specifies 100 characters and archived KEY101.NDX declares 101. The local
data-definition spec sec.15 marks the key-value versus expression-source
boundary interpretation oracle-resolves. Mint that edge in real III+ before
locking the acceptance golden; this triage does not claim a live III+ rejection.
L023 likewise has an explicit independent empty-file invariant, with its exact
III+ transcript still to mint.

## Scope, references and filing discipline

Assigned report: /home/tobias/Projects/initech-os/docs/audits/2026-10-23-samir/REPORT.md
and NITS.tsv, L001-L048. Supplied image revision:
e67b8311092873fe16e6c6d62ad82f894bfb8d7d.
Source inspected against revision 3e44338f53906c23e54fc747a3d689a5d7e0ee2a;
the relevant sources are unchanged from the supplied revision. Later commits
add audits. Source paths are repository-relative unless absolute;
../dbase3-decomp means /home/tobias/Projects/dbase3-decomp.
Abbreviated cmd/, fs/, pal/, core/ and samir_main.c paths in the finding table
are relative to os/samir/. Other OS paths in that table are explicit.
Batch descriptions use explicit root-cause source paths.

Read CLAUDE.md Laws/Rules/canon, the 2026-10-05 full-drive TRIAGE.md discipline,
the assigned report/NITS/evidence, PRD 6.6 and relevant local III+ specs/manuals.
This is source triage, not a new emulator drive. No repository edits, builds,
tracker changes or oracle runs were made. Archived serial, plans, decoded data
and extracted bytes were inspected: failed output files are zero bytes;
KEY100/KEY101 declare widths 100/101; PACK artifacts remain readable.
The oracles below are required acceptance work, not green claims.

D = confirmed defect in implemented behavior; F = never built at the reported
entry; DOC = manual wrong instead of the system; BY-DESIGN = explicit canon or
a justified contextual panic. DUP/LANE override filing and retain the underlying
D/F interpretation in notes. An acknowledged manual limitation or source
comment does not make an unbuilt historical capability deliberate canon.
None of these IDs requires changing canonical year 00 -> 1900, trailing minus,
116% pie, hourglass or cooperative scheduling. A contextual invariant panic is
permitted by Rule 2; L003's crash has no visible reason and stays on its already
dispatched defect lane. There are no DOC or BY-DESIGN rows here.

OPEN-BEADS.txt and LANES-IN-FLIGHT.txt are authoritative exclusions. Offline
.beads/issues.jsonl descriptions clarify old scope without overriding supplied
open status; no live tracker was queried. Nine IDs are in dispatched lanes and
eleven have existing owners. Remaining 28 findings form 19 new pickups.
Explicit scope wins: DDL on w0ej; structure export/JOIN/UPDATE on 5sl4;
designers and the remaining LABEL runtime SAMPLE slice on jz16;
RELATION/TALK on ipu5.8.1; output filename context on 4i9l. Broad database/index
epics do not swallow distinct new defects. L018/L044 are distinct hook error
collapses, not u78k's old internal-status/catalog-number collision.

## Every finding

| ID | Verdict | Owner | Note |
| --- | --- | --- | --- |
| L001 | LANE | initech-ylqs | in lane initech-ylqs; D: fs/dbf.c:1654-1677 formats XB_D(0) as a calendar date instead of eight blanks; separate from canonical CTOD year 00. |
| L002 | LANE | initech-39e5 | in lane initech-39e5; D: cmd/flow.c:1428/:1503 never polls interruption; cmd/proc.c:770-774 accepts ESCAPE/KEY with no consumer. Cooperative scheduling does not excuse absent cancellation. |
| L003 | LANE | initech-39e5 | in lane initech-39e5; D: cmd/proc.c:607 allocates 8192 automatic bytes per call plus cmd/flow.c:1847-1848 line pools before the 64-scope guard; spec/memory_map.h:253/:264 allows a 64 KiB guest stack. No contextual panic was observed. |
| L004 | D | B08 | pal/pal_milton.c:581-596 assumes ANSI; desktop os/milton/kmain.c:2946 lacks the DOS terminal binding. Same integration owner as L048. |
| L005 | D | B03 | fs/dbf.c:1763 appends via another handle while cmd/workarea.c:903 retains a reader with stale DOS size; fs/dbt.c:521 already refreshes its header. New live-read defect, not the old absent writer. |
| L006 | LANE | initech-ylqs | in lane initech-ylqs; D: fs/dbf.c:847-856 decodes all-asterisk numeric fields through dec_parse to ordinary zero. |
| L007 | LANE | initech-ylqs | in lane initech-ylqs; D: core/fn_builtins.c:1580 passes digit slots as total dec_format width, omitting decimal-point space. |
| L008 | D | B04 | cmd/mutate.c:1124-1130 infers zero-width keys from the empty current record; fs/ndx.c:1336 rejects zero width despite support for an empty root. |
| L009 | F | B16 | cmd/flow.c:660-703 retains whole verb tokens; classification/hooks require full spellings. Abbreviation recognition was never connected. |
| L010 | F | B17 | cmd/query.c:1389-1397 treats STATUS as a record expression; no environment inspection path. |
| L011 | F | B17 | Same cmd/query.c:1389 route treats MEMORY as an expression; no memvar inspection path. |
| L012 | DUP | initech-w0ej | F: explicitly owns CREATE / MODIFY STRUCTURE. No DDL/editor hook is registered at samir_main.c:593-615; cmd/flow.c:1208 refuses it. |
| L013 | LANE | initech-39e5 | in lane initech-39e5; D: samir_main.c:661 handles QUIT at the prompt only; cmd/proc.c:784-858 has no program QUIT route and cmd/flow.c:1208 rejects it. |
| L014 | D | B01 | D retained; PACK/two-index causal attribution is WRONG. Cold-cache REINDEX closes the master before key-provider navigation (fs/ndx.c:2671; cmd/nav.c:344). |
| L015 | D | B04 | Same cmd/mutate.c:1124-1130 inference as L008; EOF gives zero-width C. GO TOP changes input state, not output accessibility. |
| L016 | LANE | initech-ylqs | in lane initech-ylqs; D: fs/dbf.c:859-875 returns XB_U for legal blank dates; cmd/mutate.c:244-249 rejects that representation in date keys. |
| L017 | D | B04 | cmd/mutate.c:1130 and fs/ndx.c:1339 enforce page fit but lack the 100-byte command limit. KEY101 declares 101; mint precise III+ boundaries. |
| L018 | D | B19 | cmd/set.c:415-417 collapses all NDX open errors to ordinal 1, including a structurally invalid present file. |
| L019 | D | B05 | pal/pal_milton.c:650 selects cooked DOS input with STATUS OFF; os/milton/int21.c:3227 caps that route at 128. Fix SAMIR selection, preserving the DOS contract. |
| L020 | D | B06 | cmd/flow.c:54/:1864-1867 imposes a hidden 512-line cap inside the supported byte limit. Enlarging automatic arrays alone worsens the recursion-lane defect. |
| L021 | F | B09 | cmd/proc.c:609 does internal lookup only; disk fallback exists only in the REPL at samir_main.c:675. This program entry has never been built. |
| L022 | LANE | initech-39e5 | in lane initech-39e5; D: cmd/flow.c:1572-1574 advances i++ after a handled error; no RETRY signal propagates from the handler. |
| L023 | D | B07 | cmd/workarea.c:538/:681 clears empty BOF; cmd/nav.c:263 does not repair it. Independent spec requires both boundary flags true. |
| L024 | DUP | initech-jz16 | F: explicitly owns later report/label designers. cmd/report.c:583-588 exposes runtime REPORT/LABEL only, without CREATE/MODIFY design entry. |
| L025 | DUP | initech-jz16 | F: same explicit designer owner as L024; cmd/report.c:583-588 implements runtime only. |
| L026 | F | B11 | samir_main.c:593-615 registers no editor/MODIFY command; cmd/flow.c:1208 is the reached unknown-verb exit. |
| L027 | DUP | initech-5sl4 | F: explicitly includes COPY STRUCTURE. cmd/mutate.c:1264 accepts COPY FILE only; :1432 routes COPY there. |
| L028 | DUP | initech-5sl4 | F: its description explicitly includes COPY STRUCTURE EXTENDED; same absent route as L027. w0ej consumes exported schema, not a second issue to file. |
| L029 | DUP | initech-5sl4 | F: explicitly owns JOIN WITH. cmd/mutate.c:1424-1468 has no JOIN hook; cmd/flow.c:1208 rejects it. |
| L030 | DUP | initech-5sl4 | F: explicitly owns UPDATE ON. Same missing hook as L029; rejected operands do not establish later update semantics. |
| L031 | F | B13 | cmd/query.c:478-490 has no destination clause. LIST printer integration is absent, independently of REPORT runtime printer refusal. |
| L032 | F | B12 | cmd/mutate.c:1122 rejects UNIQUE suffixes; cmd/set.c:732 rejects its setting; fs/ndx.c:1488 always writes non-unique policy. |
| L033 | F | B10 | No form/GET list is registered at samir_main.c:593-615; cmd/flow.c:872/:1208 has no SAY/GET/READ consumer despite terminal PAL slots. |
| L034 | F | B14 | cmd/flow.c:872-897 lacks TEXT block classification; literal lines reach ordinary expansion/dispatch. |
| L035 | F | B14 | cmd/set.c:732 rejects ECHO; cmd/flow.c:1503-1574 has no execution-trace consumer. Shared flow-output mechanism with L034. |
| L036 | F | B15 | cmd/set.c:660 handles DECIMALS but :732 rejects FIXED. Query formatters consume no fixed-mode state. |
| L037 | F | B13 | cmd/set.c:732 rejects MARGIN; cmd/workarea.c:1499-1506 has no printer output route. |
| L038 | F | B13 | Same missing setting/output mechanism as L037. Accepting PRINT text alone would not complete the feature. |
| L039 | F | B13 | Same missing setting/output mechanism as L037; DEVICE specifically routes @ SAY, so coordinate B10. |
| L040 | F | B18 | samir_main.c:644-656 retains no history; cmd/query.c:1389 and cmd/set.c:732 lack HISTORY dispatch. Share B05 input contracts, add separate session state/recall. |
| L041 | LANE | initech-39e5 | in lane initech-39e5; D: cmd/proc.c:826-829 explicitly maps RETURN TO MASTER to the same one-frame sentinel as RETURN. |
| L042 | DUP | initech-jz16 | F: retain this remaining LABEL FORM runtime slice on its existing runtime/designer owner. cmd/report.c:517-524 accepts only TO FILE after the name; SAMPLE is absent. |
| L043 | D | B02 | fs/ndx.c:1500 and cmd/mutate.c:1207/:1321 close failed outputs without removing owned new names; d_destination rejects retry. |
| L044 | D | B19 | cmd/mutate.c:609/:767/:917 reports no table as syntax with ec=0 despite catalog 52. Hook precondition error, not the old ordinal collision. |
| L045 | DUP | initech-ipu5.8.1 | D: explicitly owns accepted-but-inert RELATION. cmd/set.c:473-475 stores text and defers runtime maintenance. |
| L046 | DUP | initech-ipu5.8.1 | D: explicitly owns accepted-but-inert TALK. cmd/set.c:482-493 stores the flag without mutation result-count consumers. |
| L047 | DUP | initech-4i9l | D: its description explicitly requires identifying the affected file. samir_main.c:265 instead names the active DBF, yielding the source for SORT and nothing for closed-table COPY. |
| L048 | D | B08 | os/milton/console.c:237-270 lacks BEL handling; raw CON draws it. Fix with L004; input capacity belongs to B05. |

## New developer pickups, in user-harm order

B01-B08 repair live data access, blocked output/input/program workflows and
terminal behavior. No new persisted-data-loss P1 is filed: invalid-date writes
and uninterruptible/recursive execution already have lanes. B09-B18 implement
missing features; B19 fixes diagnostic polish. All new batches are P2; L048's
P3 BEL symptom shares the P2 terminal fix. Every batch shares an implementation
mechanism. Larger interfaces name a bounded first pickup; completing that slice
does not certify complete III+ parity or close the full feature.

BEADS.tsv has no header and exactly five tab-separated fields per row:
title, type, priority, description, L-ids. Its self-contained descriptions repeat
the root, grounding, acceptance oracle/mutant and evidence below. This task files
nothing. Emulator UI acceptance uses locked injected traces and recorded guest
interactions; host models alone do not certify it. Preserve applicable existing
tri-emulator gates. Dependencies do not create extra L-id ownership.

### B01 - SAMIR: keep REINDEX usable when the navigation cache is cold (P2, bug)

Covers: L014.

Root cause / pickup: os/samir/cmd/mutate.c:980 clears navigation after PACK. REINDEX at :1110 calls os/samir/fs/ndx.c:2671, which closes the index before ndx_build invokes its key provider (:1378). That provider calls wa_nav_goto (mutate.c:1093); os/samir/cmd/nav.c:344 materializes the master order through the now-closed index when the cache is cold. ndx_rebuild:2679 returns without reopening on provider failure, leaving fd=-1. Key collection must use physical navigation independent of the index being rebuilt; failure must leave a usable or explicitly detached handle.

Ground truth: ../dbase3-decomp/specs/commands/data-definition-and-manipulation.md sec.10/sec.15 require PACK/REINDEX to maintain open indexes. The audit one-index control warms navigation before REINDEX; index count is not the isolated trigger.

Acceptance oracle: Extend harness/diff/dbf_diff/test_ndx_pack.c and test_samir_dml.c with cold/warm caches, 1/2/7 indexes, each master order, and PACK with/without deletions. Drive PACK; REINDEX; GO TOP; LIST before any intervening read, then SEEK on every order. Check independent NDX traversal and DBF records; inject provider failure and replay pack-two.json on copied images. A mutant restoring index-aware key collection after closing the master must fail the cold-cache case.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/pack-two.json`, `docs/audits/2026-10-23-samir/evidence/pack-two-serial.log`, `docs/audits/2026-10-23-samir/evidence/pack-index.json`, `docs/audits/2026-10-23-samir/evidence/pack-index-serial.log`, `docs/audits/2026-10-23-samir/evidence/pack-two-files/BYNAME.NDX`, `docs/audits/2026-10-23-samir/evidence/pack-two-files/BYBAL.NDX`, `docs/audits/2026-10-23-samir/shots/pack-two-after.png`.

### B02 - SAMIR: remove failed new INDEX, SORT and COPY FILE outputs so retry works (P2, bug)

Covers: L043.

Root cause / pickup: os/samir/fs/ndx.c:1472 creates/truncates output, but :1500 and later failed-write exits only close it. os/samir/cmd/mutate.c:1202-1209 creates SORT output and closes on failure without remove; :1294 and :1309-1323 do the same for COPY FILE. d_destination at :1084-1085 then rejects the abandoned name under SAFETY. Use one owned-output failure/commit policy across these paths; never delete a pre-existing destination as failed-new-file cleanup.

Ground truth: ../dbase3-decomp/specs/commands/data-definition-and-manipulation.md sec.15 describes deleting an incomplete interrupted index; DBF/NDX format invariants require complete structures. Removal or safe staging of failed new SORT/COPY outputs is the audit recovery requirement, not a claim of historical transaction guarantees.

Acceptance oracle: Extend test_samir_dml.c, test_ndx_build.c and PAL fault injection across create, each short/failed write, and close. Verify source hashes, SAFETY and existing targets survive. At zero free clusters, run full-recovery, free FILLER.BIN and retry all three names without manual ERASE; extract valid outputs with mtools and independently parse DBF/NDX. A mutant omitting cleanup must leave a zero-byte name and fail the retry assertion.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/full-recovery-serial.log`, `docs/audits/2026-10-23-samir/evidence/full-recovery-bytes.json`, `docs/audits/2026-10-23-samir/evidence/full-recovery-files/FULLIDX.NDX`, `docs/audits/2026-10-23-samir/evidence/full-recovery-files/SORTED.DBF`, `docs/audits/2026-10-23-samir/evidence/full-recovery-files/COPIED.DBF`, `docs/audits/2026-10-23-samir/shots/full-recovery-failure.png`.

### B03 - SAMIR: refresh the live memo reader after a successful replacement (P2, bug)

Covers: L005.

Root cause / pickup: os/samir/fs/dbf.c:1763-1770 appends through a separate writable DBT handle and publishes the new pointer. The work area retains its reader (os/samir/cmd/workarea.c:487/:903). os/samir/fs/dbt.c:521-531 already refreshes next_free, contrary to the audit guess; Milton handles still retain independent file-size snapshots (os/milton/fileio_fat.c:39-43; os/milton/int21.c:3374). Reading the appended block through the old handle returns short, and workarea.c:905 converts that failure to empty text. Refresh/reopen the reader on successful write; do not require USE or hide real read errors as empty memos.

Ground truth: ../dbase3-decomp/specs/file-formats/dbt.md sec.4/sec.8 and commands/data-definition-and-manipulation.md REPLACE; PRD 6.6 requires correct DBF/DBT round trips. This is a distinct same-session read-after-write defect; persisted text is correct.

Acceptance oracle: Add a PAL with Milton-style per-open size snapshots to test_dbt_read.c/test_samir_core.c; a POSIX reader alone misses the defect. Repeat two replacements, direct reads, GO away/back and reopen; check lengths and DBF pointer/DBT bytes. Replay memo.json on desktop and DOS. A mutant retaining the old reader after append must return empty in the oracle and fail. Inject genuine short-read failures separately.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/memo.json`, `docs/audits/2026-10-23-samir/evidence/memo-serial.log`, `docs/audits/2026-10-23-samir/evidence/memo-files/EDGE.DBF`, `docs/audits/2026-10-23-samir/evidence/memo-files/EDGE.DBT`, `docs/audits/2026-10-23-samir/shots/memo-write.png`.

### B04 - SAMIR: infer INDEX key geometry independently of the current record and enforce its limit (P2, bug)

Covers: L008, L015, L017.

Root cause / pickup: os/samir/cmd/mutate.c:1124-1130 evaluates the incoming current record to determine key type/width. os/samir/cmd/workarea.c:866 returns empty typed character values at EOF/empty, so ndx_build rejects zero key length (os/samir/fs/ndx.c:1336). The command also lacks a 100-byte key check; ndx_build validates page geometry only (:1339-1345), permitting the archived 101-byte key. Derive stable expression geometry including zero-record tables and report catalog 23 for oversize character keys.

Ground truth: ../dbase3-decomp/specs/file-formats/ndx.md empty-root layout, commands/data-definition-and-manipulation.md sec.15, and spec/samir/dbase_msg_codes.tsv:31 (100 char maximum). Expression-source length is a different limit. The corpus marks precise boundary semantics oracle-resolves; mint III+ 100/101 cases before locking them.

Acceptance oracle: Extend test_samir_dml.c/test_ndx_build.c for empty C(12), EOF/BOF/current-record permutations, expressions and 100/101/254-byte values. An empty index must reopen, SEEK miss and accept an append. Compare nonempty traversal with real III+ and independent decoded bytes; keep blank-date L016 on initech-ylqs. Replay index-fresh.json/index-boundary.json. Mutants using the current zero-width value or removing the cap must fail distinct cases.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/index-fresh-serial.log`, `docs/audits/2026-10-23-samir/evidence/fixtures/EMPTY.DBF`, `docs/audits/2026-10-23-samir/evidence/index-boundary-serial.log`, `docs/audits/2026-10-23-samir/evidence/index-boundary-files/KEY100.NDX`, `docs/audits/2026-10-23-samir/evidence/index-boundary-files/KEY101.NDX`, `docs/audits/2026-10-23-samir/shots/index-fresh-empty.png`, `docs/audits/2026-10-23-samir/shots/index-boundary-at-eof.png`.

### B05 - SAMIR: accept complete 254-byte console commands with STATUS both off and on (P2, bug)

Covers: L019.

Root cause / pickup: os/samir/pal/pal_milton.c:650-654 selects application line input only with an active status screen; otherwise it frames AH=3Fh cooked console input. os/milton/int21.c:3227/:3252 intentionally cap that DOS editor at 128 bytes. Fix SAMIR console-input selection/framing, retaining the DOS handle contract and redirected streams. Consume an overlong application line without executing its truncated prefix.

Ground truth: pubs/manuals/initechbase-volume-one/ua-keys.tex:20-22 and u2-commands.tex:16 promise 254 characters; ../dbase3-decomp/specs/runtime/interpreter-and-execution-model.md sec.2.1 independently defines the limit. The DOS staging buffer is not itself defective.

Acceptance oracle: Extend test_samir_input.c/test_samir_repl.c and locked emulator traces for 127/128/129/253/254/255 characters, STATUS ON/OFF, both launches and redirected CRLF input. The 254-byte LEN case must return 245; 255 must raise 18 and leave the next command intact. Assert no truncated mutation executes. A mutant routing STATUS OFF back through 128-byte cooked input must fail.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/keys.json`, `docs/audits/2026-10-23-samir/evidence/keys-serial.log`, `docs/audits/2026-10-23-samir/evidence/keys-dos-serial.log`, `docs/audits/2026-10-23-samir/evidence/key-lengths.json`, `docs/audits/2026-10-23-samir/shots/keys-254.png`, `docs/audits/2026-10-23-samir/shots/keys-dos-on.png`, `docs/audits/2026-10-23-samir/shots/keys-dos-overlong.png`.

### B06 - SAMIR: run programs within the advertised byte limit regardless of short-line count (P2, bug)

Covers: L020.

Root cause / pickup: os/samir/cmd/flow.c:54 fixes FLOW_MAX_LINES at 512. samir_do at :1847-1867 uses fixed line/kind arrays and maps split_lines overflow to error 43; 7018-byte SHORT1K.PRG never executes although the loader permits 8191 bytes (os/samir/samir_main.c:459). Replace the hidden line-count bottleneck with bounded storage for the supported byte envelope. Simply growing automatic arrays aggravates the fixed-stack recursion defect on initech-39e5.

Ground truth: pubs/manuals/initechbase-volume-one/u5-entries-a.tex:331-333 advertises 8191 characters without a 512-line restriction; the fixture is 7018 CRLF bytes. PRD 6.6 requires working program execution. This undocumented lower implementation limit is not deliberate canon.

Acceptance oracle: Extend test_interp_flow.c/test_samir_repl.c for 511/512/513/1000 lines and the smallest-line worst case within 8191 bytes. SHORT1K must print 1000; over-byte-limit rejection must be clean. Include constrained PAL memory/guest stack bounds, a copied-image programedges trace and a subsequent command. A mutant restoring the 512-line cap must fail. Coordinate storage ownership with the recursion lane.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/fixtures/SHORT1K.PRG`, `docs/audits/2026-10-23-samir/evidence/programedges-serial.log`, `docs/audits/2026-10-23-samir/evidence/programs-real-serial.log`, `docs/audits/2026-10-23-samir/shots/programedges-1000.png`.

### B07 - SAMIR: set both BOF and EOF for physically empty work areas (P2, bug)

Covers: L023.

Root cause / pickup: os/samir/cmd/workarea.c:538-539 initializes empty BOF=0 and EOF=1; :678-681 repeats this on refresh. os/samir/cmd/nav.c:263-267 returns early for empty GO TOP without correcting flags. Apply one empty-table boundary invariant across USE, navigation and count-changing operations while preserving closed-area and nonempty BOF semantics.

Ground truth: ../dbase3-decomp/specs/commands/navigation-query-display.md:999 explicitly requires EOF and BOF true and RECNO=1 for empty files. The contrary nav.c comment is not independent ground truth; mint the exact III+ boundary transcript before locking it.

Acceptance oracle: Extend test_interp_nav.c/test_interp_use.c for USE EMPTY, GO TOP/BOTTOM, refresh, last-record PACK/ZAP and append back to nonempty. Distinguish physically empty from nonempty FILTER/DELETED-hidden tables. Replay verify-final.json against a newly minted III+ transcript. A mutant clearing empty BOF must fail both open and navigation assertions.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/verify-final-serial.log`, `docs/audits/2026-10-23-samir/evidence/fixtures/EMPTY.DBF`, `docs/audits/2026-10-23-samir/shots/verify-final-empty.png`.

### B08 - SAMIR desktop launch: provide its terminal controls and consume BEL correctly (P2, bug)

Covers: L004, L048.

Root cause / pickup: os/samir/pal/pal_milton.c:581-596 emits ANSI cursor/attribute controls for the status screen. The FLAIR launch at os/milton/kmain.c:2946-2963 clears the raw console and executes SAMIR, bypassing DOS-path ANSI binding at :8082-8095. os/milton/int21.c:764 requires the binding/gate; raw os/milton/console.c:237-270 lacks BEL handling and paints byte 7. Bind terminal capabilities on desktop text launch and handle BEL without visible glyphs; preserve byte-transparent file output.

Ground truth: pubs/manuals/initechbase-quick-reference/qr-set.tex:16 documents the status bar; ../dbase3-decomp/specs/full-screen/assist-ui-and-editors.md sec.10 describes it. BEL must not advance/paint text, per the audit terminal requirement. Intentional raw ANSI behavior without ANSI.SYS does not authorize an app to use unavailable controls.

Acceptance oracle: Extend test_samir_status.c/test_samir_screen_emu.c and console/ANSI tests. Drive status ON/OFF, query, scroll and return on DOS/desktop; inspect guest pixels for bar/cursor and rejected-key BEL for no extra glyph/cursor movement. Verify redirected bytes and desktop restoration; record a record-flair clip. Mutants omitting desktop binding and rendering BEL as printable must each fail.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/desktop-status-serial.log`, `docs/audits/2026-10-23-samir/evidence/dos-status-serial.log`, `docs/audits/2026-10-23-samir/evidence/keys-serial.log`, `docs/audits/2026-10-23-samir/shots/desktop-status-status.png`, `docs/audits/2026-10-23-samir/shots/dos-status-type.png`, `docs/audits/2026-10-23-samir/shots/keys-254.png`, `docs/audits/2026-10-23-samir/shots/keys-dos-off.png`.

### B09 - SAMIR: resolve nested DO calls to disk PRGs (P2, feature)

Covers: L021.

Root cause / pickup: os/samir/cmd/proc.c:609-613 searches only p_find_procedure and rejects a miss. Disk loading exists solely in os/samir/samir_main.c:550 and the outer REPL call at :675; no program-side fallback uses it. Add a shared owned-source loader/call-frame path, without invoking the REPL recursively or overwriting caller source.

Ground truth: ../dbase3-decomp/specs/commands/programming-and-io.md:71-85 defines DO of a procedure or PRG. Same-name precedence is oracle-resolves; mint it. The manual honestly documents internal-only nested DO. Existing u78k USE/CLOSE dispatch is a different missing route.

Acceptance oracle: Extend test_interp_proc.c/test_samir_repl.c for OUTER -> USEPRG, WITH character/numeric parameters, nested internal procedures, missing files, errors and repeated calls. Assert source lifetime, caller continuation, scope cleanup and table survival. Run control.json in an emulator against III+ output. A mutant removing file fallback must fail. Coordinate QUIT/retry/unwind/recursion with initech-39e5.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/control.json`, `docs/audits/2026-10-23-samir/evidence/control-serial.log`, `docs/audits/2026-10-23-samir/evidence/fixtures/OUTER.PRG`, `docs/audits/2026-10-23-samir/evidence/fixtures/USEPRG.PRG`, `docs/audits/2026-10-23-samir/shots/control-nested.png`.

### B10 - SAMIR: add a bounded SAY, GET and READ form workflow (P2, feature)

Covers: L033.

Root cause / pickup: os/samir/cmd/flow.c:660 tokenizes @ but :872-897 gives it no form classification; os/samir/samir_main.c:593-615 registers no forms module and flow.c:1208 reports unknown verb. Terminal PAL slots exist (os/samir/include/samir/pal.h:148), but no GET list/READ consumer does. First pickup: @ SAY and one character memvar GET/READ with actual positioning, write-back and measured accept/cancel behavior; retain broader grammar on this feature.

Ground truth: InitechOS-PRD.md sec.6.6 expressly includes @ SAY/GET/READ; ../dbase3-decomp/specs/full-screen/say-get-read.md sec.1-8 and real CLIENTS.FMT/MENUMASK.PRG ground coordinates, input and values. Exclude later Harbour syntax explicitly marked in the spec.

Acceptance oracle: Add independent host form-state/PAL cursor/input tests, then drive FORM.PRG and @ 2,3 SAY HELLO on both emulator launch routes. Compare coordinates and accepted values with real III+; cancel/invalid input must preserve measured state. Record the interaction. Mutants swapping row/column or dropping READ write-back must fail distinct checks. Coordinate B08 terminal binding and existing CONFIRM/ESCAPE settings without duplicating their beads.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/fixtures/FORM.PRG`, `docs/audits/2026-10-23-samir/evidence/programmore-real-serial.log`, `docs/audits/2026-10-23-samir/shots/programmore-real-form.png`.

### B11 - SAMIR: open and edit PRG files through MODIFY COMMAND (P2, feature)

Covers: L026.

Root cause / pickup: os/samir/samir_main.c:593-615 registers no program editor; query/mutate/set/proc/report hooks do not accept MODIFY, so os/samir/cmd/flow.c:1208 refuses it. The disk PRG loader at samir_main.c:472 supports execution only. First pickup: load FLOW.PRG, edit text, save/cancel and return to the session; retain broader editor commands on this feature.

Ground truth: ../dbase3-decomp/specs/full-screen/assist-ui-and-editors.md sec.9 describes MODIFY COMMAND and program editing. Use reference-confirmed III+ keys and a new real-software trace rather than inventing modern editor bindings.

Acceptance oracle: Add host buffer/file-lifetime tests and a locked guest open/edit/save/reopen/DO trace, plus cancel, missing file, size boundaries, full disk and failed close. Saved files must execute and open in III+; failed/cancelled saves must preserve original bytes. Record the interaction. A mutant omitting the save write must fail independently extracted bytes, not merely an editor-state assertion.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/ui-gaps-serial.log`, `docs/audits/2026-10-23-samir/evidence/fixtures/FLOW.PRG`, `docs/audits/2026-10-23-samir/shots/ui-gaps-design.png`.

### B12 - SAMIR: connect UNIQUE index creation and SET UNIQUE to the NDX engine (P2, feature)

Covers: L032.

Root cause / pickup: os/samir/cmd/mutate.c:1122 rejects tokens after the output name. os/samir/cmd/set.c:732-739 rejects UNIQUE; os/samir/fs/ndx.c:1488 always writes unique_flag=0. Add the setting/option with duplicate suppression and persistent policy, not syntax accepted without altered index contents.

Ground truth: ../dbase3-decomp/specs/commands/data-definition-and-manipulation.md sec.15 says UNIQUE keeps the first record for equal keys without deleting DBF rows; commands/set-commands.md:165 and file-formats/ndx.md define policy/header. This absent command policy is distinct from the generic ahu codec epic.

Acceptance oracle: Extend test_samir_dml.c/test_ndx_build.c with duplicate C/N/D keys, UNIQUE versus SET UNIQUE ON/OFF, reopen, REINDEX and insert/replace maintenance. Mint III+ first-record/maintenance rules and independently traverse NDX while DBF row counts remain unchanged. Replay the ui-gaps unique case in an emulator. A mutant accepting but ignoring UNIQUE must fail traversal and header checks.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/ui-gaps-serial.log`, `docs/audits/2026-10-23-samir/shots/ui-gaps-unique.png`.

### B13 - SAMIR: add a common printer output route for LIST and print settings (P2, feature)

Covers: L031, L037, L038, L039.

Root cause / pickup: os/samir/cmd/query.c:451-490 handles scope/FOR/WHILE/OFF, so TO PRINT becomes expression text. os/samir/cmd/set.c:732-739 lacks MARGIN/PRINT/DEVICE branches. os/samir/cmd/workarea.c:1499-1506 routes to mirror/console only; PAL has no printer sink. First pickup: one real output-router contract for LIST TO PRINT, PRINT ON/OFF mirroring, margin and DEVICE state; integrate @ SAY printing as its forms consumer lands. Missing printers must reach the real unavailable-printer error, never false success.

Ground truth: ../dbase3-decomp/specs/commands/navigation-query-display.md:656-665 and commands/set-commands.md:225-233/:359-370 define behavior: MARGIN is print-only; DEVICE routes @ SAY; PRINT mirrors ordinary output. report.c:460 already reaches catalog 126. Printer-position functions remain separately owned by initech-mgzg.

Acceptance oracle: Grade captured printer bytes through a PAL for routing, margins, page/cursor behavior, console and alternate combinations. Mint independent III+ output, add an emulator captured-printer leg and a no-printer leg reaching 126. Verify SCREEN/PRINT OFF controls. Mutants dropping LIST routing, applying margins to screen or ignoring DEVICE must fail. Parser-only success does not certify printing.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/ui-gaps-serial.log`, `docs/audits/2026-10-23-samir/evidence/settings-serial.log`, `docs/audits/2026-10-23-samir/shots/ui-gaps-data.png`, `docs/audits/2026-10-23-samir/shots/settings-format.png`.

### B14 - SAMIR: support literal TEXT blocks and program command echo in the flow executor (P2, feature)

Covers: L034, L035.

Root cause / pickup: os/samir/cmd/flow.c:872-897 lacks TEXT/ENDTEXT block classification; :1570-1574 expands/dispatches ordinary lines as commands. os/samir/cmd/set.c:732-739 rejects ECHO, and run_block has no trace consumer. Implement literal-line handling before macro/comment/verb processing plus stored ECHO state consumed at execution. These share the flow-output mechanism, not the expression printer.

Ground truth: ../dbase3-decomp/specs/commands/programming-and-io.md:494-517 requires literal whitespace-preserving TEXT without macros; commands/set-commands.md:150 defines ECHO. Use existing CONSOLE/ALTERNATE routing; mint exact III+ trace layout.

Acceptance oracle: Extend test_interp_flow.c/test_interp_set.c for literal blank/indented lines, ampersands, && and command-looking text; missing ENDTEXT must fail cleanly. Test ECHO ON/OFF around programs/nested calls without changing program output. Compare new III+ transcripts and run TEXT/USEPRG in an emulator. Mutants feeding literal lines to flow_expand and omitting trace must fail separate checks.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/fixtures/TEXT.PRG`, `docs/audits/2026-10-23-samir/evidence/fixtures/USEPRG.PRG`, `docs/audits/2026-10-23-samir/evidence/programmore-real-serial.log`, `docs/audits/2026-10-23-samir/evidence/settings-serial.log`, `docs/audits/2026-10-23-samir/shots/programmore-real-form.png`, `docs/audits/2026-10-23-samir/shots/settings-trace.png`.

### B15 - SAMIR: apply SET FIXED through the existing numeric formatter (P2, feature)

Covers: L036.

Root cause / pickup: os/samir/cmd/set.c:660 implements DECIMALS but :732-739 rejects FIXED. os/samir/cmd/query.c formatters have no FIXED-state consumer. Add the toggle and its display policy without altering stored numbers or DBF descriptor precision.

Ground truth: ../dbase3-decomp/specs/commands/set-commands.md sec.2/sec.3.1 defines FIXED with DECIMALS. Capture integer/fractional/negative/stored-field output in real III+ rather than extrapolating modern formatting.

Acceptance oracle: Extend test_interp_decimals.c/test_interp_setfmt.c and the numeric guest screen oracle for ON/OFF, DECIMALS, ?/??/LIST, descriptor-backed numbers and overflow. Replay settings-format. A mutant accepting FIXED but leaving formatting unchanged must fail an independent transcript; extracted DBF bytes must remain unchanged.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/settings-serial.log`, `docs/audits/2026-10-23-samir/shots/settings-format.png`.

### B16 - SAMIR: recognize four-letter command and keyword abbreviations consistently (P2, feature)

Covers: L009.

Root cause / pickup: os/samir/cmd/flow.c:660-703 uppercases whole tokens without canonicalization; :879-894 and hooks compare full spellings (os/samir/cmd/query.c:1389; mutate.c:1437). REPL special dispatch also compares whole words (samir_main.c:206). Add one recognition policy before classification, dispatch and applicable keyword parsing, without changing identifiers or strings.

Ground truth: ../dbase3-decomp/specs/runtime/interpreter-and-execution-model.md:188-224 documents minimum-four-character recognition and full matching for short verbs. The shipped u2-commands.tex:26 restriction is an honest limitation, not canon. Mint ambiguous/invalid long spellings before choosing behavior.

Acceptance oracle: Extend lexer/flow/REPL/test_samir_dml.c cases for DISP/REPL/APPE/LOCA/SELE/INDE/STOR, short DO/USE/SET and full-spelling equivalence. Cover PRGs/prompt, clauses, invalid suffixes and same-prefix variable names. Replay missing.json. A mutant disabling recognition for one hook must fail that abbreviation differential.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/missing.json`, `docs/audits/2026-10-23-samir/evidence/missing-serial.log`, `docs/audits/2026-10-23-samir/shots/missing-abbrev.png`.

### B17 - SAMIR: implement DISPLAY/LIST STATUS and MEMORY inspection subcommands (P2, feature)

Covers: L010, L011.

Root cause / pickup: os/samir/cmd/query.c:1389-1397 sends all DISPLAY/LIST tails through the record-expression parser. No STATUS/MEMORY subcommand precedes q_walk, so the words resolve as field/memvar names. Add separate inspection paths and bounded state enumeration for actual variable scope/type/size/value and work-area/settings state.

Ground truth: ../dbase3-decomp/specs/commands/navigation-query-display.md:713-743 specifies these commands. They differ from SET STATUS and existing u78k DISPLAY STRUCTURE work. Presentation is oracle-resolves; mint III+ captures.

Acceptance oracle: Extend test_samir_query.c/test_samir_repl.c and variable enumeration tests for no table, multiple areas/indexes, PUBLIC/PRIVATE shadowing and empty/populated variables. Assert no cursor/state mutation and DISPLAY versus LIST paging. Drive a locked emulator inspection leg against independent III+ content/captures. Mutants omitting one area or exposing the wrong variable scope must fail.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/missing-serial.log`, `docs/audits/2026-10-23-samir/shots/missing-inspection.png`.

### B18 - SAMIR: retain, recall and list command history (P2, feature)

Covers: L040.

Root cause / pickup: os/samir/samir_main.c:644-656 reads a new line without history storage; os/samir/pal/pal_milton.c:633 drops nonprintable keys on the app editor. query.c:1389-1397 has no HISTORY dispatch and set.c:732-739 rejects it. Add a bounded session-owned store and recall key transport; SET HISTORY and LIST/DISPLAY HISTORY must use that store.

Ground truth: ../dbase3-decomp/specs/commands/set-commands.md:539-550 and navigation-query-display.md:747-761 ground recording/listing. The corpus documents both HISTORY TO n and ON/OFF plus Up recall; mint key transport, capacity, recording and paging edges.

Acceptance oracle: Add host history/editor tests for recall/edit/wrap/capacity, ON/OFF and LAST n, quoted commands and empty/no-table sessions. Drive FIRST -> Up -> Enter and listing on both STATUS paths and launch routes; preserve redirected input. Mutants returning empty history or dropping Up transport must fail an independent replay. Record a short input clip.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/keys.json`, `docs/audits/2026-10-23-samir/evidence/keys-serial.log`, `docs/audits/2026-10-23-samir/shots/keys-history.png`.

### B19 - SAMIR: preserve typed precondition and malformed-index errors at command boundaries (P2, bug)

Covers: L018, L044.

Root cause / pickup: os/samir/cmd/set.c:415-417 maps all ndx_open_rw failures to missing-file ordinal 1. os/samir/cmd/mutate.c:609/:767/:917 returns syntax with ec=0 for absent tables, so the correct catalog mapper emits 10. Preserve typed causes at these hook boundaries: missing stays 1, malformed NDX gets its measured damaged-index code, and no database gets 52. This differs from old u78k internal-status/catalog collisions.

Ground truth: spec/samir/dbase_msg_codes.tsv contains 1/52/114; ../dbase3-decomp/specs/file-formats/ndx.md defines header validation and commands/data-definition-and-manipulation.md gives table preconditions. Mint the precise III+ corrupt-index response; a present malformed file is definitely not nonexistent.

Acceptance oracle: Extend test_interp_set.c/test_samir_repl.c for missing/truncated/bad-geometry NDX versus valid files, and DELETE/RECALL/PACK/REPLACE without a table versus genuine syntax errors. Replay errors.json/artifact-controls.json; assert no disk/work-area mutation. Mutants mapping malformed to 1 and no-table to 10 must each fail the error-class matrix.

Audit evidence: `docs/audits/2026-10-23-samir/evidence/errors.json`, `docs/audits/2026-10-23-samir/evidence/errors-serial.log`, `docs/audits/2026-10-23-samir/evidence/fixtures/BAD.NDX`, `docs/audits/2026-10-23-samir/evidence/artifact-controls.json`, `docs/audits/2026-10-23-samir/evidence/artifact-controls-serial.log`, `docs/audits/2026-10-23-samir/shots/errors-files.png`, `docs/audits/2026-10-23-samir/shots/artifact-controls-no-table.png`.

## Completion accounting

- 48 IDs, assigned exactly once: 13 D, 15 F, 11 DUP, 9 LANE; zero DOC/BY-DESIGN.
- 28 findings in 19 new TSV rows; every remaining D/F has one owner.
- Nine in-flight and eleven existing-bead IDs produce no new row.
- L014 causal attribution and L005 source guess corrected; neither observed failure dismissed.
- Acceptance is proposed, not run; historical edges needing real III+ minting are explicit.
