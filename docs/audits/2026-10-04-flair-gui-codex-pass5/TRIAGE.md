# Fifth-pass audit source triage

Audit: REPORT.md, K01-K15, guest sessions on 2026-10-04.
Triage: 2026-10-05. Repository: /home/tobias/Projects/initech-os.

All 15 findings are confirmed by source inspection, with the qualifications
below. "Confirmed" means that the source explains the reported behavior; it
does not claim a new emulator reproduction. No repo edits, builds, tests,
commits, or tracker writes were performed.

Source citations are relative to the repo and pinned to initial HEAD
1c1249354c25a7bcb519fe9c09eaf94965580f17. Use
git show 1c12493:<file> to reproduce their line numbers. HEAD advanced to
9f169fa during this read-only investigation; that merge adds the separately
tracked Trash features and shifts kmain.c lines, but does not fix these roots.
The audited boot images were identified as 8b1bf6d; 1c12493 also contains the
subsequent disk-tenant menu fix. That fix does not implement the built-in
HELLO/NOTES commands in K07.

Read CLAUDE.md and AGENTS.md, relevant PRD sections, earlier audit triages,
locked specs, source, and local reference corpora. No independent DOS 3.3
golden for these COPY/DEL/DIR/REN/FOR operations was located in fixtures/ or
spec/. A source comment describing DOS is not a measured historical golden.
References marked "from memory" below deliberately retain that limitation.

## Tracker check and filing boundary

From the repo, bd search 'COPY' and bd show initech-tdnl.39 both failed:

    Error: failed to open database: embeddeddolt: opening lock file:
    open /home/tobias/Projects/initech-os/.beads/embeddeddolt/.lock:
    read-only file system

Per the brief, further live search/show checks were skipped for every finding.
No workaround, copied tracker database, or tracker mutation was attempted.
bd create --help succeeded: --type, --priority, --labels, --title and
--description are valid flags.

Read-only project documents nevertheless establish these overlaps:

- K01: initech-vj28, COPY same-file edge cases
  (docs/worklog/WL-0067-flair-gui-fix-arc.md:116). initech-ojxn is the earlier,
  narrower same-spelling fix, not a new ticket for the absolute-path residual.
- K04: initech-p4h7, the exact sixteen-match DEL defect
  (docs/FLAIR-GUI-bug-hunt-2026-06-28.md:596).
- K10: initech-f2cz covers the wrong directory header, not the ignored operands
  (docs/FLAIR-GUI-bug-hunt-2026-06-28.md:610).
- K11: wildcard COPY was explicitly recorded as filed, without an ID in the
  worklog (docs/worklog/WL-0038-DOS33-parity-tranche-E-F-G-MZ-EXE-orchestrated.md:37).
  Wildcard FOR was recorded in the existing xw1 bead notes
  (docs/worklog/WL-0040-parity-wave1-xw1-bat-autoexec-bo40-keep-x3mh-ansi.md:34).
  No existing wildcard REN mapping was located.
- K15: the DEL *.* confirmation follow-up was explicitly recorded as filed,
  without an ID (WL-0038:37, same file above).

Thus 5 findings have documented whole/partial overlap: 3 have explicit IDs
(K01, K04, part of K10), and 2 more have filing/notes evidence without a new
issue ID (K11, K15). Live status and coverage cannot be verified. This is not
a claim that the other 10 findings are proven absent from the tracker.

file_beads.sh contains 12 candidate create commands: it excludes K01, K04,
K15; K10 files operands/layout only; K11 files wildcard REN only. The PM
should reconcile the unavailable live duplicate checks before running it.
There is no separate duplicate bead for the error-code confusion shared by
K06 and K08: that repair is included under K08. Other primary roots are
distinct even where a batch shares a function or subsystem.

## Summary

| Finding | Verdict | Root cause file:line | Existing bead / overlap | Batch |
| --- | --- | --- | --- | --- |
| K01 | Confirmed; known residual | os/milton/command.c:594, :2664 | initech-vj28; earlier initech-ojxn | B02 |
| K02 | Confirmed; directory data loss | os/milton/command.c:2670; os/milton/fat12.c:2453 | Live check unavailable | B01 |
| K03 | Confirmed; wrong-target deletion | os/milton/command.c:2750, :2759 | Live check unavailable | B03 |
| K04 | Confirmed; exact duplicate documented | os/milton/command.c:2738, :2748 | initech-p4h7 | B03 |
| K05 | Confirmed; shared-engine deferral | os/samir/cmd/set.c:382; os/samir/cmd/query.c:852 | Live check unavailable | B06 |
| K06 | Confirmed; marker and SET omissions | os/samir/cmd/query.c:732; os/samir/cmd/set.c:619 | Live check unavailable | B05, B06, B07 |
| K07 | Confirmed; demo menu promises | os/milton/kmain.c:141; os/apps/ref_tenant.c:398 | Related disk menu initech-tdnl.31 is different | B10 |
| K08 | Confirmed; several omissions; DO diagnosis narrowed | os/samir/cmd/query.c:1292; os/samir/cmd/mutate.c:752; os/samir/samir_main.c:718 | Related phase tasks are not proof of coverage | B07, B11, B15-B20 |
| K09 | Confirmed; REPL name and error mapping | os/samir/samir_main.c:475, :481 | Live check unavailable | B08 |
| K10 | Confirmed; partial duplicate | os/milton/command.c:2982, :2382, :2386 | initech-f2cz for header only | B12 |
| K11 | Confirmed; three distinct shell omissions | os/milton/command.c:2653, :2779, :3873 | COPY filed, ID unavailable; FOR in xw1 notes | B13a-B13c |
| K12 | Confirmed; decoding and explicit-CON path | os/milton/kbd.c:119; os/milton/int21.c:1869 | Existing ^C checkpoint initech-4tw is a different path | B04 |
| K13 | Confirmed; target PAL contract violation | os/samir/pal/pal_milton.c:607, :623 | Live check unavailable | B09 |
| K14 | Confirmed; missing display fallback | os/milton/kmain.c:3309, :4847 | Live check unavailable | B21 |
| K15 | Confirmed; documented deferral | os/milton/command.c:2713, :2726 | Filed per WL-0038:37; ID unavailable | B03 |

## Why green host SAMIR results do not cover this audit

Makefile:24454 defines the target profile (32-bit, soft float, one interpreter
registry). Makefile:24457 includes query.c, mutate.c, set.c, proc.c and the
same core/storage/navigation sources as PROG_DIFF_ENG at Makefile:14091.
pal_milton.c:936 calls samir_repl, which registers all four modules at
samir_main.c:655. The desktop runs the disk SAMIR.COM too
(os/milton/kmain.c:2627); it has no alternative database interpreter.
No build flag here removes the audited verbs.

The differences are coverage and integration, not an intentionally reduced
command build:

- K05: SET FILTER stores text but the shared engine never applies it.
  test_interp_set.c:336 asserts successful storage; :357 explicitly skips
  the runtime effect. query.c:56 also declares FILTER/DELETED walks gated.
- K06: the shared renderer lacks the deletion marker; SET DELETED is absent.
  Correct DBF deletion bytes and DELETED() can pass while presentation fails.
- K08: most requested command/UI handlers do not exist. DO-file loading does
  exist; the program executor cannot execute USE. prog_diff.c:349 removes
  the leading USE and :491 opens the table in the harness before :498 runs
  its body. This bypass is not an end-to-end program test.
- K09: the REPL passes a filename verbatim and misclassifies codec I/O errors.
  The differential provisions its own table, and the REPL host tests use
  explicit .dbf paths (test_samir_repl.c:543).
- K13: target conin_line treats a file read as a console line. The host PAL
  uses fgets (pal_host.c:288); scripted REPL tests inject logical lines.
  Neither exercises the target file-backed stdin path.

A passing differential establishes its exercised subset, not every III+
command or target PAL behavior. No claim about current test pass counts is
made here: the read-only brief forbids running builds/gates.

## K01 - COPY absolute alias destroys the source

1. Root cause. cmd_same_file strips drive and one dot-slash, but its
   separator branch compares text rather than resolved identity
   (os/milton/command.c:574, :594). SELF.BIN and A:\SELF.BIN therefore compare
   different. builtin_copy opens the source (:2653), trusts that incomplete
   guard (:2664), then creates/truncates the destination (:2670).
   fat12_create frees the existing chain (os/milton/fat12.c:2453).
   The source SFT retains a snapshot of its old directory entry
   (os/milton/int21.c:2220). Reads now refer to a freed/reused chain. This
   explains the size-sensitive corruption; it is not evidence of a 512-byte
   maximum file size.
2. Existing filing. initech-vj28 is documented at WL-0067:116. Live show is
   unavailable. Do not create another K01 bead. K02 shares COPY's destructive
   create call, but its missing destination-type check is a distinct root.
3. Correct behavior and grounding. Refuse aliases of the same file before
   truncation, preserve every byte, print the same-file diagnostic and zero
   copied. Local contract:
   docs/adr/ADR-0003-InitechDOS-Base-OS-Personality.md:418 and
   spec/dos_messages.json MSG-DOS-0020. That ADR explicitly says a real-DOS
   golden remains to be captured.
4. Build explanation. Not a database finding.
5. Fix and oracle. Compare resolved volume, parent directory and entry/slot,
   or share a complete path canonicalizer with the kernel; include CWD and
   dot/dot-dot semantics. Do this before create. Extend test-command and
   test-copy-selfcopy with absolute, root-relative and nested-CWD aliases.
   Emulator acceptance must extract/hash an 8192-byte and a larger fixture
   before/after, including the tail, and retain the safe small-file control.
   Capture DOS 3.3 in 86Box for the exact refusal transcript.

## K02 - COPY overwrites a destination directory

1. Root cause. builtin_copy calls dos_creat on the second token without
   checking for a directory or appending the source basename
   (os/milton/command.c:2670). The backend calls fat12_create with archive
   attributes (os/milton/fileio_fat.c:264). fat12_create's found branch frees
   any matching chain without rejecting a directory/volume label
   (os/milton/fat12.c:2453), then replaces attributes, start cluster and size
   (:2500). This is both a shell omission and a destructive backend invariant
   failure; merely fixing shell syntax leaves direct CREAT callers unsafe.
2. Existing filing. Live check unavailable; no matching ID located in the
   read documents. Candidate new K02 bead. Distinct from K01.
3. Correct behavior and grounding. COPY README.TXT FILLED should create
   FILLED\README.TXT and preserve FILLED\SAVE.TXT. This syntax is from memory
   of DOS 3.3. At minimum, reject an unsupported directory destination
   without mutation. PRD section 6.1 grounds real FAT operations and their
   independent filesystem oracle, not this exact COPY grammar.
4. Build explanation. Not a database finding.
5. Fix and oracle. Reject non-regular existing entries before freeing any FAT
   chain. In the shell, resolve destination attributes and append the source
   leaf for a directory operand, then run K01's identity check on that final
   destination. Extend host FAT/file-I/O tests with empty and populated
   directory CREAT rejection and byte-for-byte unchanged FAT/dirent checks.
   Add an emulator COPY-to-directory trace; mtools must still see the
   directory, sentinel child and new file. DOS 3.3/86Box supplies syntax/output.

## K03 - Path-qualified DEL targets the current directory

1. Root cause. FINDFIRST correctly resolves the complete spec
   (os/milton/int21.c:3471), but the shell stores only DTA fname leaves
   (os/milton/command.c:2750) and unlinks those leaves (:2759). The original
   parent path is lost, so the delete resolves relative to CWD.
2. Existing filing. Live check unavailable; no matching ID located.
   Candidate new K03 bead. K04 is in the same collection loop but caused by
   its capacity, not path loss; K15 lacks a prompt, also a separate cause.
3. Correct behavior and grounding. Apply the full directory/pattern and
   preserve same-named files in other directories. PRD section 6.1 plus the
   path-resolution contract in int21.c:3471 ground the local expectation.
   Exact shell behavior is from memory of DOS 3.3.
4. Build explanation. Not a database finding.
5. Fix and oracle. Split the spec into parent and pattern once, and delete
   matched fully qualified paths or stable parent/entry identities. Preserve
   the original search directory across all chunks. Add a host shell seam
   test and a locked emulator trace with different root/subdir sentinels.
   Diff stopped FAT images with mtools: only requested subdir matches vanish.

## K04 - DEL silently ignores matches after sixteen

1. Root cause. roster[16][13] and count < 16 discard later names while
   FINDNEXT keeps running (os/milton/command.c:2738, :2748). Only the collected
   sixteen are unlinked (:2758). No overflow/partial-operation diagnostic.
2. Existing filing. Exact prior finding R2.32 maps to initech-p4h7 in
   docs/FLAIR-GUI-bug-hunt-2026-06-28.md:596. Live status unavailable.
   Exclude from new creates. Shared function with K03/K15, distinct root.
3. Correct behavior and grounding. Process all matches, or disclose an
   incomplete operation and its failure. From memory of DOS 3.3; also the
   project's fail-loud requirement (CLAUDE.md Rule 2).
4. Build explanation. Not a database finding.
5. Fix and oracle. Keep bounded memory but process repeated collected batches
   with a safe fresh search/continuation; stop on no progress and report
   failures rather than endlessly retrying undeletable files. Do not merely
   enlarge sixteen to another silent limit. Host cases: 0/1/16/17/20/100+
   files and refused deletions. Emulator/mtools gate: all 20 Z*.TXT disappear,
   KEEP.TXT survives and CWD/qualified-path variants agree.

## K05 - Accepted SET FILTER does not filter

1. Root cause. do_set_filter copies raw text into per-interpreter SET state,
   sets have_filter and returns success with an explicit deferred-runtime
   comment (os/samir/cmd/set.c:382). It does not install a work-area predicate.
   q_walk tests only command-local FOR/WHILE (os/samir/cmd/query.c:852).
   GO TOP chooses record 1 directly in physical order
   (os/samir/cmd/nav.c:270). No active filter reaches these paths.
2. Existing filing. Live check unavailable. The source requires a follow-up
   bead but does not name one (:384); that is not proof a ticket exists.
   Candidate new K05 bead. K06's missing deleted visibility should use the
   same future visibility seam, but its primary marker defect is distinct.
3. Correct behavior and grounding. Store the filter on the selected work
   area, lazily reevaluate it as navigation/scans advance; GO TOP must land
   on a passing record. Bare SET FILTER TO clears it. Local reference:
   /home/tobias/Projects/dbase3-decomp/specs/commands/navigation-query-display.md:472,
   especially :484, :494 and :502. Direct physical GO edge behavior should
   be resolved by the III+ oracle, not guessed from a scan rule.
4. Build explanation. Shared-engine omission, not a reduced build or desktop
   frontend. test_interp_set.c:336 tests storage and :357 loud-skips runtime
   filtering. A working LIST FOR therefore does not certify SET FILTER.
5. Fix and oracle. Add a per-area compiled predicate and one visibility rule
   shared by navigation, query and scoped mutation, composed with SET DELETED.
   Validate logical types/errors and clearing/switching areas. Extend host
   set/nav/query tests to reject record 2 after GO TOP; mint matching real
   dBASE III PLUS 1.1 traces, including indexes and empty filtered views.
   Replay this precise dot-prompt sequence in test-samir-boot-style gates.

## K06 - Deleted rows have no marker; SET DELETED is absent

1. Root cause. q_render_record emits record number, space and fields without
   consulting the deletion state (os/samir/cmd/query.c:732). DELETE/DBF state
   works; the renderer omits it. set_cmd_hook has no DELETED branch and falls
   into the unknown-option syntax error (os/samir/cmd/set.c:619).
   That returns internal INTERP_ERR_SYNTAX=7 with ec=0. flow.c:1645 records
   that internal number and samir_main.c:335 looks it up as catalog error 7,
   "File already exists." This diagnostic confusion is shared with K08.
2. Existing filing. Live check unavailable; no matching ID located. Candidate
   K06 bead owns marker/deleted visibility; common error translation belongs
   to the K08 bead and B07, not a second diagnostic ticket.
3. Correct behavior and grounding. Default SET DELETED OFF includes deleted
   rows with a leading *. ON hides them in scans/navigation; direct GO to a
   physical record remains possible. Local reference:
   /home/tobias/Projects/dbase3-decomp/specs/commands/navigation-query-display.md:518,
   :538 and :545; data-definition-and-manipulation.md:466.
4. Build explanation. Shared-engine renderer/SET omissions. Correct
   DELETED() and PACK do not certify LIST/DISPLAY decoration or SET behavior.
   No target build guard explains these missing branches.
5. Fix and oracle. Read the existing deletion flag for each rendered record;
   mint the exact marker placement, including OFF/expr-list variants. Add SET
   DELETED state to B06's shared visibility seam. Extend test-samir-query and
   test-samir-repl with DELETE/DISPLAY/LIST/RECALL and DELETED ON/OFF; diff
   transcripts against III+ 1.1 and replay in the emulator before PACK.

## K07 - Built-in demo apps expose enabled dead commands

1. Root cause. NOTES' File/Edit/About items are hard-coded enabled
   (os/milton/kmain.c:141, :147, :153); HELLO inherits enabled New/Open fixture
   items (:1053). ref_tenant.c's only shared event handler paints, activates
   or toggles a marker; keyDown and other events do nothing
   (os/apps/ref_tenant.c:356, :398). There is no document/text/save model.
   The later menu delivery fix calls tbx_menu_choice, which targets only
   g_slot.app, the disk tenant (os/flair/tbxgate.c:850, :874), so it does not
   make these compiled-in demos hear or implement their commands.
2. Existing filing. Live check unavailable. Earlier triages map disk menus
   to initech-tdnl.31, Finder no-ops to .36/.67 and upper-bar placeholders to
   .50. Those are related, not this built-in-app root. Candidate new K07 bead.
3. Correct behavior and grounding. Enabled items must execute an action or
   show a relevant limitation; unavailable actions should be gray.
   /home/tobias/Projects/system7-decomp/specs/toolbox/menu-manager.md:201
   grounds application result handling and enabled-only selection.
   PRD sections 3/6.3 require working menus. A complete Notes editor is from
   memory of classic Mac applications, not a PRD requirement for these demos.
4. Build explanation. Not a database finding. HELLO/NOTES are intentionally
   reference demo tenants; the defect is their exposed command promises.
5. Fix and oracle. First make menus truthful: disable unimplemented
   New/Open/Save/Edit and supply a useful About/demo explanation. Deliver
   built-in menu selections to their owner and make Quit use normal process
   teardown; gray unsupported shortcuts too. A real Notes editor is separate
   product work, not one sitting of this fix. Extend host process/menu tests
   and emulator app-switch traces for each enabled action, Quit and repaint.
   Record the required FLAIR acceptance clip (CLAUDE.md Rule 14).

## K08 - Missing database workflows and misleading errors

1. Root causes, separated:
   - SUM/COUNT and DISPLAY STRUCTURE: query_cmd_hook only knows record
     LIST/DISPLAY and the listed query/navigation verbs
     (os/samir/cmd/query.c:1292, :1337). STRUCTURE becomes an expression list
     (:664), resolves as an identifier and fails XBEE_UNBOUND=-1
     (os/samir/core/eval.c:746; include/samir/eval.h:477). The renderer prints
     magnitude 1 but generic Internal error for lookup -1
     (os/samir/samir_main.c:332). No structure-report handler exists.
   - Bare APPEND: m_append accepts BLANK only and returns internal syntax
     error otherwise (os/samir/cmd/mutate.c:750). Internal 7 is mistaken for
     catalog 7 as described in K06 (flow.c:1645; samir_main.c:335).
   - CREATE/BROWSE/EDIT/ASSIST/HELP/COPY TO: no handlers in the registered
     query/mutate/set/proc chain (samir_main.c:655; query.c:1337;
     mutate.c:1078); the executor reaches unrecognized-verb #16
     (os/samir/cmd/flow.c:1032). Full-screen application UI is absent;
     terminal extension support is still stubbed in pal_milton.c:641.
   - DO UPDATE: disk loading exists (samir_main.c:614, :739). The supplied
     fixtures/UPDATE.PRG:1 is USE CLIENTS.DBF. USE/CLOSE are intercepted only
     by the REPL (samir_main.c:718), whereas disk bodies go straight to
     proc_run (:639), then samir_do and the hooks with no USE handler.
     The first USE therefore reaches flow.c:1032 and #16. The report's
     "DO missing" inference is too broad; DO loading is implemented.
2. Existing filing. Live check unavailable. S5.4/S5.5/S5.8 and S8.4 phase
   tasks do not prove these exact omissions are filed. Candidate K08 bead
   covers the remaining workflows, with small execution batches below.
   Its error translation repair also fixes K06's misleading SET diagnostic.
3. Correct behavior and grounding. The local III+ corpus documents:
   data-definition-and-manipulation.md sections 1, 4, 6, 7, 12, 16
   (CREATE, APPEND, BROWSE, EDIT, COPY TO, SUM/COUNT);
   navigation-query-display.md:694 (STRUCTURE);
   full-screen/assist-ui-and-editors.md sections 1, 3, 4, 5;
   control-flow-and-procedures.md:518 (DO a disk program).
   These paths are under /home/tobias/Projects/dbase3-decomp/specs/.
   PRD section 6.6 has a narrower dot-prompt subset plus full-screen forms;
   missing editors/ASSIST are parity additions, not regressions of certified
   features. Interim unsupported responses must be accurate and leave data
   intact; they do not count as implementing these workflows.
4. Build explanation. Several commands were never implemented; this is not
   caused by removing their modules in the target. DO/USE is a REPL versus
   program-executor integration gap. The host differential deliberately
   provisions the table and strips USE (prog_diff.c:349, :491, :496).
   Thus it cannot catch this program failure.
5. Fix and oracle. Move USE/CLOSE into common command dispatch, retain the
   REPL as an I/O loop, and run the entire PRG including USE. Separate internal
   error types from dBASE message ordinals. Add structure/aggregate/export
   handlers; implement editors and assistance as separate UI slices.
   For each, mint real III+ 1.1 transcripts or full-screen key traces, then
   add host REPL/program tests and target emulator gates. COPY TO additionally
   needs real-dBASE readback of the DBF/DBT. Extend DO-file tests with UPDATE,
   not just the STORE/?-only GREET program at test_samir_repl.c:624.

## K09 - USE does not supply .DBF and reports the wrong error

1. Root cause. repl_do_use reads a token and passes it unchanged to
   wa_set_open_rw (os/samir/samir_main.c:433, :475). No default DBF extension
   is added. On error it tests only -WA_ERR_IO (:481), which is -8
   (include/samir/workarea.h:176). wa_set_open_mode propagates the codec code
   directly (cmd/workarea.c:417). Missing file open returns -DBF_ERR_IO=-1
   (fs/dbf.c:328; include/samir/dbf.h:149), so it wrongly becomes catalog #15.
2. Existing filing. Live check unavailable; no matching ID located.
   Candidate new K09 bead. The wrong mapping is distinct from K08's internal
   interpreter-error/catalog collision.
3. Correct behavior and grounding. USE CLIENTS opens CLIENTS.DBF. The local
   reference supplies stronger grounding than the audit's memory caveat:
   /home/tobias/Projects/dbase3-decomp/specs/commands/data-definition-and-manipulation.md:242.
   spec/samir/dbase_msg_codes.tsv distinguishes #1 missing file, #15 invalid
   database and #29 inaccessible file.
4. Build explanation. Same REPL on host and OS; no reduced target. The
   differential pre-opens tables, while REPL tests use explicit file paths.
5. Fix and oracle. Add .DBF only when the final pathname component lacks an
   extension; apply it in the shared USE handler from B11. Preserve explicit
   extensions. Translate typed PAL/codec errors accurately, distinguishing
   missing/unreadable files from malformed DBFs; the current DBF_ERR_IO alone
   cannot make all those distinctions. Extend host REPL tests and a target
   gate with short/explicit names, dotted directories, missing files, access
   failures and corrupt headers; mint III+ 1.1 transcripts.

## K10 - DIR ignores arguments and mislabels CWD

1. Root cause. CMD_DIR dispatch calls builtin_dir() without parsed.arg
   (os/milton/command.c:2982). builtin_dir prints a literal A:\ header
   (:2382), always finds "*.*" (:2386), and always uses the same formatter.
   No operand or /W can affect it. The lower find/path layer already supports
   a qualified spec (os/milton/int21.c:3471).
2. Existing filing. initech-f2cz is documented for the header in the earlier
   bug hunt at :610. Live show unavailable. File only the operand/layout
   portion as new K10 work; header belongs to the existing bead.
3. Correct behavior and grounding. Resolve a directory operand or file
   pattern, list only its matches, report no matches, label the directory
   actually listed, and honor /W. From memory of DOS 3.3. PRD section 6.1
   requires a working DIR; ADR-0003 section 5.11 supplies the shell contract,
   not a byte-exact DIR golden.
4. Build explanation. Not a database finding.
5. Fix and oracle. Pass parsed.arg; parse switches separately; distinguish
   directory and file specs using attributes; format the resolved directory
   name (reuse cwd_display for the no-operand case). Add a wide formatter.
   Extend test-command for parsing/formatting and emulator DIR traces for
   *.TXT, APPS, NOFILE.ZZZ, /W and nested CWD. mtools checks selected file sets;
   a DOS 3.3/86Box golden defines command layout and diagnostics.

## K11 - Wildcard COPY, REN and FOR are absent

1. Distinct roots, not one lower FAT failure:
   - COPY passes the pattern directly to dos_open; it never enumerates it
     (os/milton/command.c:2653). Single-file scope is explicit at :2637.
   - REN passes both wildcard tokens to the one-file rename syscall without
     DOS per-match destination-name substitution (:2779).
   - FOR substitutes each whitespace-delimited set token once, even if it
     contains wildcards (:3873, :3876). There is no FINDFIRST/NEXT expansion
     in that arm. Literal sets, parameter substitution and CALL can work.
2. Existing filing. COPY follow-up is recorded as filed (WL-0038:37, ID
   unavailable). FOR is recorded in xw1 notes (WL-0040:34), with live status
   unavailable. No wildcard REN mapping located; the script creates that
   unlocated portion only. Do not duplicate the earlier COPY/FOR tracking.
3. Correct behavior and grounding. Enumerate matching regular files and
   apply command-specific DOS semantics: wildcard COPY to one named target
   concatenates according to DOS text/binary rules, REN maps each old basename
   through the new pattern, FOR executes once per matched name while retaining
   literal-set behavior. From memory of DOS 3.3. Exact ASCII EOF/concatenation
   and destination wildcard rules require an independent DOS golden.
4. Build explanation. Not a database finding.
5. Fix and oracle. Share path-aware enumeration, but keep COPY, REN and FOR
   policies separate. Snapshot/protect DTA and search state when a FOR body
   runs other find commands. Detect self/destination inclusion before COPY
   truncation; handle rename collisions without losing files. Extend
   test-command/test-batch-exec through filesystem seams and emulator batch
   traces; diff outputs and resulting names/bytes against DOS 3.3 in 86Box.
   This is three small sessions, not a single "add globbing" patch.

## K12 - Ctrl-C/Ctrl-Z are letters and TYPE CON cannot finish

1. Two source causes. kbd_state has only Shift/Caps
   (os/milton/kbd.h:72). kbd_translate neither tracks Ctrl make/break nor
   maps letters to control bytes (os/milton/kbd.c:119, :152); the DOS ring
   therefore receives z/c at IRQ enqueue (:214).
   Separately, TYPE opens CON by name (command.c:2426). That SFT carries a
   device pointer (int21.c:2157), so reads take the device-chain branch
   (:2871), whose dev_con_read_adapter returns one raw byte and always count
   1 (:1869), with no EOF or Ctrl-C check. It bypasses the existing cooked
   input ^C checkpoint at int21.c:1217. Even fixing Ctrl decoding alone
   would leave this explicit-CON route unable to interpret ^C/^Z.
2. Existing filing. Live check unavailable. initech-4tw implements the
   existing CON-input checkpoint, not these raw keyboard/explicit-CON
   defects. Candidate new K12 bead; unrelated to FLAIR modifier-latch .58.
3. Correct behavior and grounding. Ctrl-C reaches the active command/program
   abort path; Ctrl-Z gives EOF for a DOS text console handle read.
   Local docs/adr/ADR-0003-AMENDMENT-DEC-16-INT21h-AH33h-CtrlBreak-State.md
   section 3.3 grounds Ctrl-C checks and BREAK behavior. Console Ctrl-Z EOF
   and explicit-CON parity are from memory of DOS 3.3; capture them before
   choosing detailed raw/cooked semantics. The pipe's failure to supply TYPE
   CON is not the finding: CON explicitly names the keyboard device.
4. Build explanation. Not a database finding; FLAIR uses a separate raw
   scancode/event modifier path and its working Ctrl latch proves nothing
   about DOS ASCII decoding.
5. Fix and oracle. Track left/right Ctrl and E0 prefixes in the DOS decoder;
   map control chords without altering printable/Shift/Caps behavior. Route
   handle CON input through the appropriate cooked/control-aware service,
   implement text EOF at that boundary, and ensure built-in abort unwinds
   handles/redirection and returns to COMMAND. Preserve no-check AH=07h/raw
   contracts. Extend test-kbd-unit, test_conin/device tests, then an emulator
   gate with explicit Ctrl down/key down/up/Ctrl up and a prompt-return marker.
   Test stdin CON and OPEN CON separately; mint DOS 3.3 recovery traces.

## K13 - Redirected SAMIR stdin is treated as one command

1. Root cause. milton_conin_line makes one AH=3Fh read of up to 256 bytes
   (os/samir/pal/pal_milton.c:604, :607), removes only trailing CR/LF (:623),
   then returns all remaining bytes. This works when the kernel cooked CON
   read happens to return one line, but file-backed handle 0 is a byte stream.
   USE/LIST/QUIT plus their internal newlines reach repl_do_use as one input:
   LIST becomes an unknown USE clause (samir_main.c:463), explaining #36.
2. Existing filing. Live check unavailable; no matching ID located.
   Candidate new K13 bead. Separate from K08's program-file dispatch gap:
   redirected input is supposed to run separate REPL lines.
3. Correct behavior and grounding. Return one logical line per conin_line,
   without trailing newline, preserving subsequent lines and EOF.
   Local contract: os/samir/include/samir/pal.h:143. This is application/PAL
   integration, not a claim that historical dBASE accepted this shell syntax.
4. Build explanation. Target PAL defect. Host conin_line calls fgets
   (pal_host.c:288); injected REPL tests also supply lines. The engine and
   frontend need not change to consume correctly separated lines.
5. Fix and oracle. Buffer read-ahead bytes in PAL state, split at CR/LF and
   retain the tail; handle CRLF across reads, blank/final unterminated lines,
   capacity limits and CF/errors. Do not naively read CON one byte at a time:
   its current cooked small-read path discards the rest of a line
   (os/milton/int21.c:2940). Add a PAL contract test with a byte-stream read
   stub. Emulator gate: SAMIR < INPUT.TXT > DBOUT.TXT twice, checking rows,
   clean exit, reopened table state and return to the shell. Compare to the
   interactive SAMIR sequence and host PAL output, not an invented DOS golden.

## K14 - Unlabeled disk has no desktop name

1. Root cause. desktop_db_volume_label returns NOT_FOUND without inventing
   metadata (os/milton/desktop_db.c:288). The caller sets the display label
   empty (os/milton/kmain.c:3309), then installs it on the desktop icon
   (:4847). finder_desk_set_name copies it without a fallback
   (os/flair/finder_desktop.c:184). There is no visible drive identifier.
2. Existing filing. Live check unavailable; no matching ID located.
   Candidate new K14 bead.
3. Correct behavior and grounding. Display a readable fallback such as
   "Drive A" when the mounted label is empty, preserving a real label.
   PRD Appendix A's "Drive A Files" grounds that chimera naming choice.
   The requirement for an unlabeled fallback is ordinary orientation
   semantics; "Untitled" as a precise classic Mac default is from memory,
   not established by a local golden.
4. Build explanation. Not a database engine defect; the SAMIR fixture merely
   supplies an unlabeled volume.
5. Fix and oracle. Select the fallback at the display boundary for absent,
   empty or all-space labels. Do not write a FAT volume label just to fix UI.
   Extend host desktop-label tests with an independent expected name.
   Emulator screenshot/selection/open traces must show the fallback on the
   unlabeled fixture, INITECH on the labeled fixture, and unchanged FAT label
   metadata. Record the required FLAIR clip.

## K15 - DEL *.* has no confirmation

1. Root cause. builtin_del expressly defers the all-files prompt
   (os/milton/command.c:2713). It proceeds directly from token parsing to
   wildcard collection/deletion (:2726, :2758). No response is requested.
2. Existing filing. WL-0038:37 records this follow-up as already filed but
   provides no ID. Live search/show unavailable. Exclude from new creates;
   PM should locate that ticket. Same function as K03/K04, different root.
3. Correct behavior and grounding. Confirm the all-files wildcard before any
   removal, with decline/abort preserving data. From memory of DOS 3.3.
   command.c:2713 independently documents the intended prompt, but is not
   a historical golden. This is DOS confirmation, not GUI Trash/undo.
4. Build explanation. Not a database finding.
5. Fix and oracle. Recognize the DOS all-files forms, prompt once for the
   resolved target and proceed only on affirmative input; resolve exact
   spelling/qualified forms against DOS 3.3 first. Do not re-prompt for
   continuation batches. Emulator gates for Y/N/abort must assert unchanged
   stopped disks before consent and complete deletion after Y, including
   more than sixteen matches. A DOS 3.3/86Box transcript is the prompt oracle.

## Fix batches, in descending user harm

These are bounded developer sessions, not parallel lanes. B01-B03 go first
because they destroy data. A finding can span sessions when it has several
roots. Existing-bead work stays on its existing bead.

| Order / batch | Findings and bounded session | Files and acceptance |
| --- | --- | --- |
| B01 | K02: prevent directory destruction and support directory destination | fat12.c, fileio_fat.c, command.c; host invariant test plus emulator/mtools sentinel preservation |
| B02 | K01: complete COPY identity check, on initech-vj28 | command.c and shared path/identity seam; alias cases and full multi-cluster hash preservation |
| B03 | K03/K04/K15: safe DEL target, bounded complete traversal and one consent | command.c, shell seam; keep K03 new, p4h7 and existing prompt follow-up; wrong-parent, >16, refused-file/no-progress and Y/N emulator cases |
| B04 | K12: DOS modifier decoding plus explicit-CON recovery | kbd.c/h, int21.c; real PS/2 input through both console-handle routes returns to prompt; host decoder/control tests |
| B05 | K06 primary marker: expose actual deletion state before PACK | query.c; III+ LIST/DISPLAY marker differential and target state trace |
| B06a | K05 plus K06 SET DELETED: per-area state and physical-order visibility | set.c, workarea.c/h, nav.c, query.c; unindexed GO TOP/SKIP/LIST, clearing and area switching; independent III+ filter/deleted cases |
| B06b | K05/K06: integrate indexed and scoped-command visibility | nav.c, query.c, mutate.c; indexed movement and mutation share B06a's predicate; independent III+ record-set/pointer cases |
| B07 | K08 plus K06 shared diagnostic root: distinguish engine errors from catalog codes | flow.c, samir_main.c, command hooks; exact unsupported/syntax/type messages, no accidental #7 existing-file errors |
| B08 | K09: extension default and typed open-error mapping | shared USE handler, workarea/dbf/PAL error boundary; short names and missing/corrupt/access cases on host and target |
| B09 | K13: buffered stdin logical lines in target PAL | pal_milton.c; host byte-stream contract test, repeated emulator redirects and clean exit |
| B10 | K07: truthful demo menus, About and real Quit | kmain.c, ref_tenant.c, owner menu dispatch; all remaining enabled mouse/key actions have visible outcomes; emulator clip |
| B11 | K08 DO: move USE/CLOSE into common dispatch | samir_main.c, new/common command hook, prog_diff.c; run whole UPDATE.PRG including USE; stop stripping USE in differential execution |
| B12 | K10: operand/switch parser plus live header | command.c; new operand bug plus existing f2cz; DOS DIR *.TXT/APPS/missing,/W and nested-CWD golden |
| B13a | K11: wildcard REN only (candidate new bead) | command.c and rename-pattern helper; DOS 3.3 collision and destination-pattern differential |
| B13b | K11: wildcard FOR (existing xw1 notes) | command.c, batch.c only if helper needed; protect DTA across body DIR and nested FOR; emulator file set |
| B13c | K11: wildcard COPY (existing follow-up) | command.c after B01/B02; DOS concatenation/EOF/destination-in-set differential, bytes preserved on refusal |
| B15 | K08: SUM and COUNT on the shared scoped walk | query.c/common scan; real III+ aggregates, FOR/WHILE/filter/deleted and output/TO-variable cases |
| B16 | K08: LIST/DISPLAY STRUCTURE | query.c; metadata report without pointer movement, III+ transcript/paging reference |
| B17 | K08: default DBF COPY TO export, explicit unsupported responses for other export modes | new export hook plus dbf/dbt APIs; real III+ opens resulting data, source unchanged, reject partial/unsafe writes |
| B18 | K08: interactive EDIT and bare APPEND entry/exit slice | new UI module and terminal PAL; one record form, typed field write/cancel, reference key trace and target persistence |
| B19a | K08: BROWSE record viewport/navigation slice | new browse UI module; limited viewport with correct selection and scroll, then later editing slices; III+ full-screen references and target traces |
| B19b | K08: CREATE structure-form slice | new structure UI module; fields/type/width form with validation and save/cancel; III+ reference and real-dBASE readback |
| B20a | K08: reachable contextual HELP | help command/UI module; independent reference text/navigation and target key trace |
| B20b | K08: one ASSIST command path | ASSIST UI module; one menu-to-working-command vertical slice, then further paths in later sessions; III+ reference navigation |
| B21 | K14: label fallback | kmain.c/display-name helper; labeled/unlabeled emulator screenshot gate and clip; FAT metadata unchanged |

B06a/B06b deliberately separate state/walk work from indexed/scoped integration.
B18-B20's full editor/ASSIST coverage
cannot honestly be promised as a complete one-sitting rewrite. Take the
listed vertical slices one at a time and leave K05/K08 open until the full
requested semantics are covered. Interim accurate unsupported responses
belong to B07, not a claim that UI parity has shipped.

The ranked gap list also repeats already mapped Finder/document/system
features. It adds no new finding IDs or filing commands here. Its main new
priorities are represented above: safe filesystem operations, truthful app
menus, correct database state and usable query/input workflows.

## Completion accounting

- Confirmed by source: 15 of 15; no finding left without a root cause.
- DO subclaim corrected: disk loading exists; USE inside UPDATE is unsupported.
- Already tracked in project documents: 5 whole/partial overlaps; 3 explicit
  IDs, plus 2 filing/notes overlaps without a separate ID.
- Live tracker status/duplicates: unavailable because bd cannot open its lock.
- New candidate commands: 12, narrowed to avoid the documented overlaps.
- Remaining uncertainty: exact historical DOS transcripts, III+ layout/UI
  edges identified above, and live bead coverage. No invented oracle runs.
