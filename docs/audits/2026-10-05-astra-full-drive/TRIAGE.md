# Full-drive source triage

Audit: REPORT.md / NITS.tsv, L001-L118, driven supplied 7030e2f images.
Triage date: 2026-10-05. Repo: /home/tobias/Projects/initech-os.
Source citations are repo-relative and pinned to HEAD
50e084e15f6948f8cd5f33f9c8887862adea4685 (unchanged during inspection).
Reference paths beginning ../ name sibling corpora under /home/tobias/Projects.
Manual evidence/references paths refer to this audit directory's extractions of
the original manuals in /home/tobias/Projects/initech-manuals-ref.

Read CLAUDE.md first, then AGENTS.md, PRD 6.1/6.3/6.5/6.6 and the relevant
architecture/spec/reference material. This is source triage, not a new drive.
No repository edits, builds, commits, bd calls or oracle runs were performed.
The named oracles below are acceptance work for the developer, not green claims.

D = defect in implemented behavior (including omitted consumers of an accepted
setting and installation of existing binaries). F = feature never built at the
reported entry point, even if supporting codecs/APIs exist. Tracking is a
separate axis; a tracked item retains its D/F verdict. New means a candidate with no narrow existing owner found in the
provided exclusions/offline records, not a live proof of tracker absence.
Source confirms the
primary cause/absence for every L-id. A named missing feature is not evidence
that the current oracle covers it.

P1 sections are ordered by data harm/recovery, then productive-work blockage.
Batches follow in descending user harm: protection and destructive misrouting,
filesystem and primary workflows, productivity, broader compatibility features,
then polish. Within similar impact, shared ownership/dependencies break ties.
Each batch is one developer pickup through shared files or one implementation
area. Large missing utilities/report engines have an explicitly bounded first
sitting; completing that slice must not close the full feature or claim parity.
No giant 'all DOS utilities' or 'all database commands' implementation batch is
hidden here. Dependencies are described, not filed by this script.

## Tracking and filing boundary

The user-supplied exclusions are authoritative. No live tracker status was
queried. SET FILTER, deleted marker/SET DELETED, USE default extension, and
redirected database input remain existing work (worklog WL-0094 names hw4j,
3t55, nds1, xrbj). They have no new L-id here and no create command.

Direct mappings: L004 -> initech-tdnl.88; L006 -> initech-1nsj;
L009 -> initech-94ah; L010 -> initech-uqah; L011 -> initech-tdnl.91;
L021 -> initech-68iw.1; L022 -> initech-tdnl.87.

Pass-5 overlaps remain excluded, not revived under new L numbers:
WL-0094-disk-apps-trash-initech-123-dos-data-safety.md:107-110 maps j1lu to
K12 control chords, oqiu to K10 DIR operands, 68lh to K11 wildcard REN,
jzhh to K11 wildcard COPY (and K15 DEL confirmation), anf6 to K07 dead
HELLO/NOTES menus, and u78k to K08 missing database workflows/diagnostics.
The user also excludes kkht. Its exact ID-to-K14 fallback-name mapping was not
located in the read-only documents; the pass-5 TRIAGE.md:491 covers that
remaining fallback finding. Neither is refiled. A new command absent from K08's
enumerated scope is classified below separately, not automatically swallowed by
the broad phrase 'missing database commands'. In particular new INDEX/REINDEX,
MEM files, reporting and file verbs are not in that pass-5 create description.
Shared USE/CLOSE dispatch and internal/catalog error translation stay on u78k.

Additional narrow existing mappings were found by reading the offline exported
.beads/issues.jsonl (titles/descriptions, not a live status certificate):
CHKDSK kd4a; FORMAT du6v; ATTRIB gyvx; TREE/XCOPY zuvo; DISKCOPY/DISKCOMP kusw;
SYS bc7p; LABEL f88h; DEBUG/EDLIN udr1; CTTY f9z4; SHARE ws3x;
EXP/LOG/SQRT 7az.13. These tracked-only batches have no create command.
ws3x explicitly defers SHARE under a compatibility amendment; this feature gap
does not authorize an unreviewed architecture change. Existing m0dc covers the
implemented filters, not their omission from the driven installation.
Other legacy utilities such as NLSFUNC/GRAPHICS/EXE2BIN need a native substitute
or an explicit product scope decision: ADR-0001/ADR-0003 exclude running real
16-bit DOS binaries. The triage does not promise to run a historical TSR.

file_beads.sh contains only set -eu, the required cd and candidate bd create
commands, one per NEW P1 and NEW batch. A tracked batch is an existing-bead
pickup, so creating it would violate the no-duplicate instruction. The script
is written but has not been run.

## All 118 findings

Each row assigns exactly one owning P1 section or batch. Repeated mentions in
dependencies and source discussion do not create another ownership assignment.

| ID | Verdict | Owner / existing bead |
| --- | --- | --- |
| L001 | D - defect; new | P1 L001 |
| L002 | D - defect; new | P1 L002 |
| L003 | D - defect; new | P1 L003 |
| L004 | D - defect; tracked | P1 L004; initech-tdnl.88 |
| L005 | D - defect; new | P1 L005 |
| L006 | D - defect; tracked | P1 L006; initech-1nsj |
| L007 | D - defect; new | P1 L007 |
| L008 | D - defect; new | P1 L008 |
| L009 | F - unbuilt feature; tracked | B09; initech-94ah |
| L010 | F - unbuilt feature; tracked | B10; initech-uqah |
| L011 | F - unbuilt feature; tracked | P1 L011; initech-tdnl.91 |
| L012 | F - unbuilt feature; new | B80 |
| L013 | F - unbuilt feature; new | B11 |
| L014 | F - unbuilt feature; new | B11 |
| L015 | F - unbuilt feature; new | B11 |
| L016 | F - unbuilt feature; new | B11 |
| L017 | F - unbuilt feature; new | B31 |
| L018 | D - defect; new | B12 |
| L019 | D - defect; new | B13 |
| L020 | D - defect; new | B14 |
| L021 | F - unbuilt feature; tracked | B81; initech-68iw.1 |
| L022 | F - unbuilt feature; tracked | B82; initech-tdnl.87 |
| L023 | D - defect; new | B83 |
| L024 | D - defect; new | B84 |
| L025 | D - defect; new | B06 |
| L026 | D - defect; new | B07 |
| L027 | D - defect; new | B07 |
| L028 | D - defect; new | B04 |
| L029 | F - unbuilt feature; new | B05 |
| L030 | D - defect; new | B08 |
| L031 | D - defect; new | B28 |
| L032 | D - defect; new | B28 |
| L033 | F - unbuilt feature; new | B29 |
| L034 | F - unbuilt feature; new | B30 |
| L035 | D - defect; new | P1 L035 |
| L036 | F - unbuilt feature; tracked | B37; initech-kd4a |
| L037 | F - unbuilt feature; tracked | B38; initech-du6v |
| L038 | F - unbuilt feature; tracked | B39; initech-kusw |
| L039 | F - unbuilt feature; tracked | B34; initech-gyvx |
| L040 | F - unbuilt feature; tracked | B35; initech-zuvo |
| L041 | D - defect; new | B32 |
| L042 | D - defect; new | B32 |
| L043 | D - defect; new | B32 |
| L044 | D - defect; new | B18 |
| L045 | D - defect; new | B18 |
| L046 | D - defect; new | B85 |
| L047 | D - defect; new | B03 |
| L048 | F - unbuilt feature; new | B16 |
| L049 | D - defect; new | B17 |
| L050 | D - defect; new | B19 |
| L051 | F - unbuilt feature; new | B21 |
| L052 | F - unbuilt feature; new | B21 |
| L053 | F - unbuilt feature; new | B22 |
| L054 | F - unbuilt feature; new | B23 |
| L055 | F - unbuilt feature; new | B23 |
| L056 | F - unbuilt feature; new | B78 |
| L057 | F - unbuilt feature; new | B79 |
| L058 | F - unbuilt feature; new | B67 |
| L059 | F - unbuilt feature; new | B66 |
| L060 | F - unbuilt feature; new | B66 |
| L061 | F - unbuilt feature; new | B69 |
| L062 | F - unbuilt feature; new | B70 |
| L063 | F - unbuilt feature; new | B69 |
| L064 | F - unbuilt feature; new | B68 |
| L065 | F - unbuilt feature; new | B68 |
| L066 | F - unbuilt feature; new | B65 |
| L067 | F - unbuilt feature; tracked | B73; initech-7az.13 |
| L068 | F - unbuilt feature; tracked | B73; initech-7az.13 |
| L069 | F - unbuilt feature; tracked | B73; initech-7az.13 |
| L070 | F - unbuilt feature; new | B74 |
| L071 | F - unbuilt feature; new | B75 |
| L072 | F - unbuilt feature; new | B74 |
| L073 | F - unbuilt feature; new | B75 |
| L074 | F - unbuilt feature; new | B75 |
| L075 | F - unbuilt feature; new | B77 |
| L076 | F - unbuilt feature; new | B77 |
| L077 | F - unbuilt feature; new | B76 |
| L078 | F - unbuilt feature; new | B76 |
| L079 | D - defect; new | B02 |
| L080 | D - defect; new | B15 |
| L081 | F - unbuilt feature; new | B49 |
| L082 | F - unbuilt feature; new | B51 |
| L083 | F - unbuilt feature; new | B53 |
| L084 | F - unbuilt feature; new | B58 |
| L085 | F - unbuilt feature; new | B47 |
| L086 | F - unbuilt feature; tracked | B44; initech-f9z4 |
| L087 | F - unbuilt feature; tracked | B40; initech-kusw |
| L088 | F - unbuilt feature; new | B63 |
| L089 | F - unbuilt feature; new | B62 |
| L090 | F - unbuilt feature; new | B48 |
| L091 | F - unbuilt feature; new | B61 |
| L092 | F - unbuilt feature; new | B52 |
| L093 | F - unbuilt feature; new | B57 |
| L094 | F - unbuilt feature; tracked | B42; initech-f88h |
| L095 | F - unbuilt feature; new | B56 |
| L096 | F - unbuilt feature; new | B59 |
| L097 | F - unbuilt feature; new | B60 |
| L098 | F - unbuilt feature; new | B55 |
| L099 | F - unbuilt feature; new | B46 |
| L100 | F - unbuilt feature; new | B54 |
| L101 | F - unbuilt feature; new | B64 |
| L102 | F - unbuilt feature; tracked | B45; initech-ws3x |
| L103 | F - unbuilt feature; new | B50 |
| L104 | F - unbuilt feature; tracked | B41; initech-bc7p |
| L105 | F - unbuilt feature; tracked | B36; initech-zuvo |
| L106 | F - unbuilt feature; tracked | B43; initech-udr1 |
| L107 | F - unbuilt feature; tracked | B33; initech-udr1 |
| L108 | F - unbuilt feature; new | B20 |
| L109 | F - unbuilt feature; new | B24 |
| L110 | F - unbuilt feature; new | B26 |
| L111 | F - unbuilt feature; new | B26 |
| L112 | F - unbuilt feature; new | B26 |
| L113 | F - unbuilt feature; new | B25 |
| L114 | F - unbuilt feature; new | B27 |
| L115 | F - unbuilt feature; new | B71 |
| L116 | F - unbuilt feature; new | B72 |
| L117 | D - defect; new | B86 |
| L118 | D - defect; new | B01 |

## Ten P1 findings

### L001 - Make refused APPEND preserve a reopenable DBF

New P1 bug.

Root cause: os/samir/fs/dbf.c:1517 increments tbl->nrec before persistence. os/samir/cmd/mutate.c:754 appends, then :760 flushes; :761 returns on failure without undoing the count. os/samir/fs/dbf.c:1319 serializes the new count and :1324 writes the header BEFORE :1361 writes the larger record region. A short or failed body write leaves the new header over the old short file; later attempts keep incrementing it. The work-area count is refreshed only after success at mutate.c:765, explaining RECCOUNT() staying 12 while the disk header reached 21.

Correct behavior: A refused append leaves all prior records and the committed count consistent; close/reopen succeeds. In-memory table and work-area counts must describe committed records.

Grounding: ../dbase3-decomp/specs/file-formats/dbf.md sections 1, 2 and 9 define nrec, record offsets and the minimum file length. PRD 6.6 requires real DBF I/O and independent round-trip grading. Preserving the old table on refusal is a data-integrity requirement, not a claim that III+ has modern transactions.

Fix sketch: Add an append commit path that preserves the existing header geometry, reserves needed growth before publishing it, writes the new record/tail before the committed count, and restores in-memory nrec/cursor on refusal. A staged replacement is another option only if its failed allocation leaves the original untouched. Merely moving the header write in the whole-file rewrite is insufficient if an earlier write can damage old records or normalize +2 geometry. Stop subsequent mutations from flushing an uncommitted count. Keep the diagnostic repair in B02 separate.

Oracle: Extend harness/diff/dbf_diff/test_dbf_mutate.c and test_use_rw.c with an independent fault/short-write PAL at every append boundary, including +1/+2 headers and zero free clusters. Reopen with an independent DBF reader and real III+; compare prior record bytes and count. Replay db-full.json on a copied FAT disk, extract CLIENTS.DBF with mtools, check structural length and fsck, and reopen in SAMIR. These tests are proposed, not run.

### L002 - Retire all views of directories purged from Trash

New P1 bug.

Root cause: os/flair/finder_windows.c:1072 removes each child directory but never retires its open window; :1110 refreshes only the Trash root window. os/flair/finder_windows.c:705 takes the surviving window dir_start and :719 passes it straight to mkdir. os/milton/kmain.c:4155 purges then repaints the desktop at :4156, with no deleted-directory window teardown. os/milton/fat12.c:3770 treats a supplied nonzero parent as a directory chain without proving that its lifetime is still valid.

Correct behavior: Every view of a removed directory becomes closed or invalid before another command can use its cluster. A stale or recycled cluster identity cannot authorize new writes.

Grounding: MAC6 printed p.112, evidence/references/mac6-manual.txt, says Empty Trash permanently removes its contents. PRD 6.1/6.3 require actual FAT operations and coherent live windows. The stale-view/write rejection requirement is a filesystem lifetime expectation.

Fix sketch: During successful postorder removal, collect the removed directory identities and close every matching Finder slot through the real Window Manager disposal path; cancel selection, drag and command context referring to those slots. Failed removals retain valid windows. Validate directory identity at the mutation boundary, ideally with a generation/parent-entry identity rather than only a FAT allocation bit, because clusters can be reused. Do not merely blank the view.

Oracle: Extend test-finder-windows and test-fat12-mkdir with open parent/child/grandchild views, partial purge refusal and cluster reuse. Replay reproduce_trash_deleted_window.py and an independent repeat, attempt Ctrl-N after purge, and use mtools plus fsck.fat -n to prove no orphan or bogus dirents. Lock the GUI trace and record a record-flair clip.

### L003 - Give work-area tables independent allocation lifetimes

New P1 bug.

Root cause: os/samir/samir_main.c:762 calls wa_close_all on QUIT. os/samir/cmd/workarea.c:675 closes areas in numeric order. dbf_close reaches os/samir/fs/dbf.c:249 teardown, which resets the SHARED PAL arena to the table mark at :257. After closing the first-opened area, the second table and its mark are beyond the new bump pointer. os/samir/pal/pal_milton.c:797 rejects that mark and :799 calls milton_panic; :126-131 executes cli; hlt. This explains the disabled-interrupt HLT snapshot; duplicate filenames are not required for this lifetime error.

Correct behavior: Independent work areas remain usable after another area closes; QUIT closes all handles and returns to the suspended desktop. Rejecting an unsupported duplicate open must happen before allocation/mutation and must not strand the process.

Grounding: ../dbase3-decomp/specs/environment/work-areas-relations-and-limits.md:42 documents ten independent work areas. os/samir/include/samir/pal.h:185 explicitly says reset frees every allocation after the mark. ADR-0008/ADR-0009 define the shared engine/PAL, and PRD 6.6 grounds the hosted database. Clean process return is the OS lifetime expectation.

Fix sketch: Separate persistent table/area storage from transient expression storage, using per-area arenas or a safe reclaim scheme. Close codec handles without rewinding across another live object; reclaim only when ownership proves it safe. Reversing numeric close order alone is not a fix: areas can open in any order and CLOSE can target an older area while a newer one remains live. Apply the ownership rule to DBF/DBT/NDX close and failure cleanup, not just QUIT.

Oracle: Add a strict PAL lifecycle oracle that poisons reclaimed bytes and enforces reset marks. Exercise opens 1->2 and 2->1, two distinct files and the same file, CLOSE/reopen of either area, indexes/memos and failed opens. Extend test_interp_use.c/test_samir_repl.c and PAL contract tests; target db_exit_probe.py must return to FLAIR after each case and then accept another app launch.

### L004 - Commit dialog buttons only after release inside

Already tracked: initech-tdnl.88. Do not create another bead.

Root cause: os/flair/dialog.c:730 handles mouseDown and :755 calls TrackControl(item->ctrl, &pt, 1), treating the down point as a complete trace; :758 returns itemHit immediately. os/flair/control.c:1039 returns the final supplied point, so a one-point trace is still inside. The non-control item path at dialog.c:760 also returns on down. Empty Trash trusts the result before purge at os/milton/kmain.c:4151.

Correct behavior: Down highlights the button, drag-out removes the highlight, and only up inside the original button commits. Up outside cancels that press while keeping the dialog open. Return/Escape retain their documented default/cancel behavior.

Grounding: ../system7-decomp/specs/toolbox/control-manager.md:150-156 defines TrackControl button-like release semantics. The audit event ordering establishes purge before mouseUp.

Fix sketch: Implement modal press tracking across down/move/up using the same state machine for DialogHandleEvent and ModalDialog. Invoke the chosen item only on a qualifying release; retain background input blocking and keyboard activation. Include both OK and Cancel. Do this on the existing bead.

Oracle: Extend test-dialog with held/down-only, drag-out, return-in, release-out, disabled-item and keyboard traces; the real Empty Trash emulator trace must show no EMPTIED marker before release and preserve disk bytes on cancellation. record-flair clip required.

### L005 - Reject CREAT truncation of a read-only destination

New P1 bug.

Root cause: os/milton/command.c:2712 calls dos_creat after the identity/directory guards. os/milton/fileio_fat.c:264 calls fat12_create with archive attributes. os/milton/fat12.c:2459 rejects directory/volume-label attributes but omits DIR_ATTR_READONLY; :2465 frees the old chain and :2508 replaces its attributes. Protection is lost at the backend, not just at the shell UI.

Correct behavior: COPY to a +R destination fails before freeing its chain or changing its name, size, attributes, date or contents. The syscall/backend reports access denied. Removing +R explicitly permits the copy.

Grounding: MS-DOS 3.3 User's Guide, ATTRIB printed p.33, evidence/references/dos33-manual.txt:4090-4125, and the access-denied explanation at :14561-14567 ground read-only protection. PRD 6.1 grounds the FAT differential. This is distinct from the already repaired COPY-to-directory guard.

Fix sketch: Add the read-only check at the destructive create/truncate backend boundary before chain release. Preserve the DOS access-denied code through file I/O and show a useful COPY refusal. Audit write-open/write-at for the same attribute invariant, without replacing the separately tracked unlink fix.

Oracle: Extend test-dos-safety-create/test-fat12-write and test-fileio with root and subdirectory +R destinations, zero-length and multi-cluster files, and direct AH=3Ch callers. A target COPY trace plus stopped-image mtools/hash/attribute checks must leave the sentinel and both FATs unchanged; an explicit unprotect control must succeed.

### L006 - Honor read-only protection during unlink

Already tracked: initech-1nsj. Do not create another bead.

Root cause: os/milton/int21.c:2502 calls the unlink backend. os/milton/fat12.c:3051 checks existence, then :3055 begins freeing the chain without an attribute check; :3068 marks the entry deleted. A fresh DEL therefore removes +R even when COPY has not touched it.

Correct behavior: DEL/direct unlink refuse protected regular files before mutation and preserve the original file/attributes. The user must remove the read-only attribute explicitly.

Grounding: MS-DOS 3.3 User's Guide ATTRIB printed p.33; evidence/references/dos33-manual.txt:4124 says read-only prevents accidental deletion. PRD 6.1 requires independent FAT grading.

Fix sketch: Enforce +R in fat12_unlink before any FAT flush or dirent deletion; map the failure to access denied and make DEL distinguish refusal from success. Keep GUI lock feedback in B01; fixing unlink alone cannot prevent Trash staging or explain the refusal.

Oracle: Host FAT/direct-AH=41h and DEL wildcard protection tests, then a fresh guest DEL READONLY.TXT trace with mtools hash/attribute/FAT checks. Test protected and unprotected matches together and retry after explicit -R.

### L007 - Honor SET SAFETY before destructive ZAP

New P1 bug.

Root cause: os/samir/cmd/set.c:438-440 stores the SAFETY flag and set_get_safety at :694-699 exposes it. os/samir/cmd/mutate.c:1015 m_zap never reads that flag or a confirmation response; :1023 calls dbf_zap immediately and :1029 persists it. Accepted safety state exists, but its destructive consumer is disconnected.

Correct behavior: With SAFETY ON (also the default), display ZAP <filename>? (Y/N) and preserve records until an affirmative response. No, Escape, EOF or failed input must not commit. SAFETY OFF follows the measured no-prompt path.

Grounding: Using dBASE III Plus printed p.U5-284, evidence/references/dbase3-using.txt:14484-14486, explicitly gives the ZAP prompt. SET SAFETY default ON and overwrite protection are at :13564-13583; local set-commands.md:159 agrees.

Fix sketch: Provide a shared destructive-command confirmation hook using the PAL; consult set_safety before dbf_zap and consume a real answer without losing subsequent redirected input. Include filename and honor abort. Protect the existing table even if reading confirmation fails. ZAP index/memo/truncation parity is broader follow-up behavior, not evidence for this prompt bug.

Oracle: Extend test_interp_set/test_dbf_mutate/test_samir_repl with ON/default/OFF and Y/N/Escape/EOF. Assert DBF bytes/count unchanged before Y and after refusal. Replay db-final.json with explicit responses and reopen the extracted DBF in real III+. Capture exact acceptance/abort transcripts from III+ for edge rules.

### L008 - Guard worksheet exit against discarding edited work

New P1 bug.

Root cause: os/i123/app/face_a.c:431-442 routes the Quit menu event directly to tbx_exit(0). The state at :102-114 has no dirty/saved state; successful commits at :327 do not mark dirty. The worksheet is BSS-backed (:100-105), so teardown discards it. There is no save/discard/cancel path. This is an unsafe behavior in an existing editable application; the missing persistence facility is separately owned by the Save bead.

Correct behavior: A dirty sheet or uncommitted entry cannot disappear through Quit without an informed choice. Cancel keeps the sheet and pending input; failed/cancelled Save keeps it dirty. A clean saved sheet can quit normally. The exact Mac three-choice dialog is a product adaptation, not a measured DOS Lotus UI.

Grounding: From memory: period spreadsheet/Mac document protection and dirty-document warnings. Lotus R2.2 Reference Manual Chapter 9 provides a distinct Quit workflow; ../lotus123-decomp/mint/mint01.steps:38-40 drives /qy after Save. PRD 1.1/1.4 grounds useful native application behavior. The reference supports an explicit Quit choice, not a claim that Lotus uses the proposed Mac dialog.

Fix sketch: Track committed edits plus pending entry state and route every user exit through one guard. Integrate Save with initech-tdnl.91; until persistence lands, an explicit discard/cancel warning is the honest safe slice. Only reset dirty state after a successful save/retrieve. Check window-close and keyboard routes as well as menu Quit; do not suppress the normal app teardown.

Oracle: Extend test-flair-i123 with clean, dirty, pending-entry, Cancel, Discard, save-failure and successful save/relaunch traces. Model assertions must show Cancel preserves values/input; mtools plus independent WK1 parsing must show Save preserves content. Require a locked GUI trace and record-flair clip.

### L035 - Keep the permanent primary command shell alive

New P1 bug.

Root cause: os/milton/sysinit.c:89 sets the /P baseline and :448 prints parsed cfg.shell, but command_repl at os/milton/command.c:4180 receives no permanence/options state. dispatch_line at :3145-3152 unconditionally sets g_shell_exit for EXIT; the REPL returns at :4301-4302. os/milton/kmain.c:6598 then prints SHELL-DONE and :6600-6602 halts forever. /P is trace/config text, not enforced shell lifetime state.

Correct behavior: EXIT from the permanent primary /P shell retains a usable prompt, and VER still works. A nonpermanent secondary processor can exit back to its caller. AUTOEXEC, IF/FOR and redirection must not bypass this distinction.

Grounding: MS-DOS 3.3 User's Guide COMMAND printed pp.46-47, evidence/references/dos33-manual.txt around :4520-4560, documents /P persistence and /C returning to the primary. The precise primary EXIT ignore/refusal rule is from memory; capture DOS 3.3 before locking its transcript. ADR-0003 Appendix D requires SHELL=COMMAND.COM /P /E:512.

Fix sketch: Pass parsed shell options into a shell context and enforce permanence in the shared EXIT dispatch, including batch/conditional paths. Support secondary contexts without corrupting primary environment, exit flag or handles. Update oracle-only boot completion so it does not depend on incorrectly exiting /P; bounded test shutdown is a harness mechanism, not shipped EXIT behavior. /C implementation remains B30.

Oracle: Extend test-command/test-shell/test-batch-exec with primary EXIT; VER and nested secondary return, redirected EXIT and AUTOEXEC EXIT. Current emulator gates inject EXIT as completion, so they must be migrated to an independent bounded end marker/shutdown. Target transcript must prove the primary prompt remains responsive; DOS 3.3/86Box establishes the precise rule.

### L011 - Expose worksheet Save and Retrieve through Toolbox file verbs

Unbuilt feature; already tracked: initech-tdnl.91. Do not create another bead.

Root cause: os/i123/app/face_a.c:91-97 installs only File > Quit and :431-446 handles no file operation. Slash is ignored at :390. os/flair/tbxgate.c:741-770 has no file verb in its trap dispatch. The codec already exists: os/i123/fs/wk1.c:257 reads WK1 and :418 writes it, but neither is connected to an app file workflow. This is an unbuilt application/Toolbox integration feature, not a missing WK1 codec.

Correct behavior: Save asks for a destination and writes a recoverable WK1; Retrieve reads a chosen file. Cancellation and I/O failure preserve the current sheet and any existing target. Relaunch/retrieve recovers saved cells.

Grounding: Lotus R2.2 Reference Manual Chapter 5, File Save; ../lotus123-decomp/mint/mint01.steps:31-37 and real 03_save_prompt/04_saved goldens. spec/i123/wk1_format.h and ../lotus123-decomp/specs/file-formats/wk1-format.md govern bytes; PRD 1.4/ADR-0013 govern application services.

Fix sketch: Finish the existing Toolbox file-verb contract with tenant pointer/handle validation, ownership, errors and cleanup; connect File Save/Retrieve to the existing WK1 codec. Offer reachable native menu commands regardless of slash-menu delivery, then wire slash commands when that bead lands. Coordinate the dirty-exit guard separately.

Oracle: test-i123-wk1-roundtrip remains the independent codec oracle; add target file-dialog/save/retrieve/failed-save traces and stopped-image mtools extraction read by real Lotus. Target Trap API pointer/handle failure tests and a recorded record-flair clip are required. Codec round-trip alone does not catch the missing UI route.

## P2/P3 developer batches

### B01 - Refuse locked Trash staging with a protection-specific message (P2)

Covers: L118. New bug create.

Verdicts: L118 = defect in existing behavior.

Root-cause area: os/flair/finder_ops.c:331-372 stages by move without a read-only/locked check; finder_window icons do not provide an authoritative attribute check there. os/flair/finder_windows.c:1094 purges through unlink with no protection-specific branch. The backend unlink omission is P1 L006, already tracked.

Pickup: Stat the source attribute before staging; refuse an individually locked document with a useful unlock message. Preserve origin records and file bytes on refusal. Recheck protection during purge and report protected refusals accurately. The new scope is GUI policy/feedback; do not refile the low-level unlink invariant.

Grounding: MAC6 printed pp.88/112; evidence/references/mac6-manual.txt:5094-5095 explicitly requires unlocking before discarding.

Oracle/done means: test-finder-ops/test-finder-windows protected source and mixed purge tests; locked emulator stage/purge trace, mtools attributes/hashes and record-flair clip.

### B02 - Preserve disk-full errors through the database write stack (P2)

Covers: L079. New bug create.

Verdicts: L079 = defect in existing behavior.

Root-cause area: os/samir/cmd/mutate.c:761 (also :881/:1030) maps every flush failure to M_MSG_READONLY. os/samir/fs/dbf.c:1325/:1362 collapse failed/short writes to DBF_ERR_IO. os/milton/fileio_fat.c:269 collapses create faults to access denied.

Pickup: Carry typed no-space/access/device/short-write errors through FAT, INT21, PAL and DBF; map true no-space to the independent III+ disk-full catalog entry and identify the file. Coordinate count integrity with P1 L001; the earlier generic internal/catalog collision is on u78k, not this unconditional read-only mapping.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: PAL error injection plus target db-full.json, protected-file and device-write controls; exact catalog expectations from real III+ and existing spec/samir/dbase_msg_codes.tsv.

### B03 - Make CLOSE select the requested resource type (P2)

Covers: L047. New bug create.

Verdicts: L047 = defect in existing behavior.

Root-cause area: os/samir/samir_main.c:493-496 ignores args and always calls wa_close_all, including FORMAT and INDEX.

Pickup: Parse CLOSE variants; close only indexes, format, procedure or databases as requested and preserve active table/cursor for non-database forms. For a resource never opened, be harmless or accurately unsupported. Integrate shared REPL/program dispatch on u78k, while this batch owns resource-type selection. Depends on P1 L003 lifecycle safety.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: test_samir_repl and work-area tests proving DBF/cursor survival after CLOSE INDEX/FORMAT/PROCEDURE; real III+ CLOSE transcript (manual p.U5-53).

### B04 - Parse COPY text and binary switches as switches (P2)

Covers: L028. New bug create.

Verdicts: L028 = defect in existing behavior.

Root-cause area: os/milton/command.c:2658 uses cmd_pair_parse and :2664/:2712 open/create its first two tokens; there is no COPY-specific switch grammar.

Pickup: Parse source/destination /A and /B placement separately, reject surplus operands before any create, then honor text EOF and destination EOF-marker semantics. Retain resolved identity/directory/+R guards. Coordinate wildcard COPY on jzhh without expanding this ticket into it.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: test-command COPY parser cases and target CTRLZ fixture bytes compared with DOS 3.3/86Box; ensure no literal /A or /B file and no unintended source replacement.

### B05 - Add explicit plus-separated COPY source lists (P2)

Covers: L029. New feature create.

Verdicts: L029 = feature never built at this entry point.

Root-cause area: os/milton/command.c:2658/:2664 treats KEEP.TXT+CTRLZ.TXT as one OPEN filename. Single-file COPY exists; concatenation grammar and execution do not.

Pickup: Implement an explicit source-list parser and bounded copy sequence, using the switch parser above and safe destination identity checks. Scope this sitting to explicit names; wildcard enumeration remains jzhh.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: DOS manual COPY printed p.52 golden for explicit two/three files, spaces around plus, EOF modes, failures and destination-as-source; stopped-image byte comparisons.

### B06 - Resolve root-only CHDIR as an actual root change (P2)

Covers: L025. New bug create.

Verdicts: L025 = defect in existing behavior.

Root-cause area: os/milton/int21.c:4189-4193 incorrectly treats a single backslash as a no-op along with empty/dot paths.

Pickup: Route root-only input through the directory resolver or reset canonical CWD/root cluster consistently; retain empty/dot no-op behavior.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: test-fileio CHDIR seam and target MD TEST; CD TEST; CD backslash; CD controls, plus qualified A: root and nested CWD.

### B07 - Validate created names and the DOS device namespace (P2)

Covers: L026, L027. New bug create.

Verdicts: L026 = defect in existing behavior; L027 = defect in existing behavior.

Root-cause area: os/milton/fat12.c:1447-1469 checks width/separators but accepts wildcard/illegal leaf characters; mkdir uses it at :3778. os/milton/int21.c:2732-2759 resolves then creates a directory without the device-name precedence used by OPEN (device resolution at :2071).

Pickup: Separate search-pattern parsing from creation-name validation; reject DOS-illegal leaf characters before allocation. Reject reserved device base names at the DOS namespace boundary, including extensions and qualified spellings; preserve valid 8.3 and service names.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt); reserved-directory-name refusal is from memory pending exact DOS golden.

Oracle/done means: test-fat12-mkdir/test-fileio invalid-name matrix, no-mutation FAT checks, emulator MD BAD?NAME/CON/AUX and legal controls; capture exact reserved-name edges under DOS 3.3.

### B08 - Stamp new FAT entries with a valid guest modification date (P2)

Covers: L030. New bug create.

Verdicts: L030 = defect in existing behavior.

Root-cause area: os/milton/fat12.h:70-71 sets FAT12_FIXED_MTIME/MDate to zero; os/milton/fat12.c:2509-2510 and :3985-3986 use them for files/directories.

Pickup: Inject a deterministic clock into metadata writes, using current guest time in the artifact and a fixed VALID date in factory goldens. Validate packed month/day; reproducible build inputs do not justify runtime 1980-00-00.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt); CLAUDE.md Rule 11 requires deterministic factory inputs.

Oracle/done means: test-fat12-write/mkdir and guest ECHO/COPY/MD date/time fixture; mtools verifies valid decoded timestamps under a locked RTC.

### B09 - Implement worksheet formulas and functions on their existing bead (P2)

Covers: L009. Existing initech-94ah; no new create.

Verdicts: L009 = feature never built at this entry point.

Root-cause area: os/i123/app/face_a.c:318 only calls i123_parse_value (numbers) before commit; formula evaluation is explicitly deferred in :40.

Pickup: Follow the existing formula bead, grading +B1*2 and @SUM(B1..D1) against real Lotus, rather than filing another calculation engine.

Grounding: Lotus R2.2 Reference Manual Chapter 1 (printed pp.1-10 to 1-17), cited by REPORT.md; native mouse behavior is from memory.

Oracle/done means: Independent Lotus MINT01 values, formula bytecode corpora and test-flair-i123 target trace.

### B10 - Implement the slash command menu on its existing bead (P2)

Covers: L010. Existing initech-uqah; no new create.

Verdicts: L010 = feature never built at this entry point.

Root-cause area: os/i123/app/face_a.c:390 explicitly ignores slash in READY; the File menu at :91 exposes only Quit.

Pickup: Follow the existing slash-menu bead; route command menu selection to real actions and coordinate Save on tdnl.91.

Grounding: Lotus R2.2 Reference Manual Chapter 1 (printed pp.1-10 to 1-17), cited by REPORT.md; native mouse behavior is from memory.

Oracle/done means: Real Lotus /fs capture/menu navigation and a locked test-flair-i123 trace with record-flair clip.

### B11 - Add worksheet edit, Goto and screen-paging keys (P2)

Covers: L013, L014, L015, L016. New feature create.

Verdicts: L013 = feature never built at this entry point; L014 = feature never built at this entry point; L015 = feature never built at this entry point; L016 = feature never built at this entry point.

Root-cause area: os/i123/app/face_a.c:354-411 implements arrows/Home/entry only; no F2/F5/PGDN handlers exist, and Tab is rejected by :390. Existing cell-content extraction at :187 can seed F2.

Pickup: One Face A keyboard sitting: add F2 existing-cell edit/cancel, F5 bounded address prompt, PGDN one visible screen and READY Tab one horizontal screen. Share address and viewport helpers; preserve LABEL/VALUE entry rules.

Grounding: Lotus R2.2 Reference Manual Chapter 1 (printed pp.1-10 to 1-17), cited by REPORT.md; native mouse behavior is from memory.

Oracle/done means: Host key-state tests with independent expected cells/addresses plus target F2/F5/PGDN/Tab traces compared with Lotus manual pp.1-10/1-11; record-flair clip.

### B12 - Use actual resized worksheet geometry for rendering and following (P2)

Covers: L018. New bug create.

Verdicts: L018 = defect in existing behavior.

Root-cause area: os/i123/app/face_a.c:63-75 fixes SCR_COLS/AREA_CHARS/VIS_ROWS; :164/:166 follows the pointer against those constants. Rendering at :241 likewise assumes 72 characters. Resize clips drawings without updating these bounds.

Pickup: Obtain the actual content rectangle through a supported Toolbox contract, derive visible whole columns/rows, and use one geometry for paint/follow/hit-test. Reflow after resize and keep the cursor visible. Expose working scrollbar behavior or truthful unavailable controls.

Grounding: Lotus R2.2 Reference Manual Chapter 1 (printed pp.1-10 to 1-17), cited by REPORT.md; native mouse behavior is from memory; resized native-window visibility is from memory.

Oracle/done means: test-flair-i123 grow/shrink then arrow movement/scroll trace and pixel assertions for cursor visibility at each size; record-flair clip.

### B13 - Separate worksheet entry capacity from display width (P2)

Covers: L019. New bug create.

Verdicts: L019 = defect in existing behavior.

Root-cause area: os/i123/app/face_a.c:109 sizes g_edit to SCR_COLS+1 and :407 stops accepting characters at SCR_COLS-2 (75).

Pickup: Use a 240-character entry buffer plus NUL and a horizontally scrolling edit viewport; explicitly reject the 241st character without silently dropping earlier text.

Grounding: Lotus R2.2 Reference Manual Chapter 1 (printed pp.1-10 to 1-17), cited by REPORT.md; native mouse behavior is from memory.

Oracle/done means: Host boundary/state tests for 75/104/240/241 characters; emulator entry/commit/readback and independent WK1 label inspection.

### B14 - Move the SAMIR launcher off the plain repeating-label key (P2)

Covers: L020. New bug create.

Verdicts: L020 = defect in existing behavior.

Root-cause area: os/milton/kmain.c:2626 defines plain backslash and :5445-5449 intercepts it before tenant delivery regardless of modifiers or active text input.

Pickup: Choose and document a non-conflicting modified system chord; deliver plain backslash to the active tenant. Preserve launcher/recovery behavior and handle left/right modifiers.

Grounding: Lotus R2.2 Reference Manual Chapter 1 (printed pp.1-10 to 1-17), cited by REPORT.md; native mouse behavior is from memory.

Oracle/done means: Locked emulator repeating-label prefix in 123 plus the new launcher chord and return-to-sheet trace; keyboard routing/modifier tests and record-flair clip.

### B15 - Explain worksheet storage exhaustion without blaming valid input (P2)

Covers: L080. New bug create.

Verdicts: L080 = defect in existing behavior.

Root-cause area: os/i123/app/face_a.c:102 caps storage at 2048 cells; os/i123/core/sheet.c:244 returns I123_E_FULL. face_a.c:321-325 turns every error, including capacity, into unexplained EDIT.

Pickup: Distinguish invalid syntax, storage full and memory exhaustion; show a useful capacity message, preserve the pending valid entry, and allow cancellation or editing existing cells. Expanding the capacity is optional, not the defect.

Grounding: Resource-failure feedback is from memory/usability judgment, as REPORT.md states.

Oracle/done means: cell_stress.py 2048/2049 boundary and existing-cell replacement; host return-code mapping and target recovery trace.

### B16 - Add alias-qualified field expression syntax (P2)

Covers: L048. New feature create.

Verdicts: L048 = feature never built at this entry point.

Root-cause area: os/samir/core/lex.c:437/:442 tokenize minus and greater-than separately; parse.c:184-203 emits only a plain identifier. Cross-area aliases exist, but no arrow AST/resolver path is built.

Pickup: Add alias->field syntax and resolve through the already-open named area without changing selected area or record pointers. The misleading catalog diagnostic belongs to the already tracked u78k error translation.

Grounding: ../dbase3-decomp/specs/environment/work-areas-relations-and-limits.md:202-220 and the manual work-area chapter.

Oracle/done means: Lexer/parser/evaluator/work-area tests and real III+ two-area transcript; ? TWO->BAL must be -42 while ONE stays selected. Run after P1 L003.

### B17 - Make TYPE inspect the expression named by its string (P2)

Covers: L049. New bug create.

Verdicts: L049 = defect in existing behavior.

Root-cause area: os/samir/core/fn_builtins.c:763-773 returns xb_type_char(args[0].t), so a character argument always returns C. The comment at :753 claims literal-expression behavior the body does not implement.

Pickup: Parse/evaluate the argument string using an isolated bounded expression context, returning U for undefined/invalid inner expressions according to III+. Preserve outer state and avoid recursive scratch corruption.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: test_xbase_fn_a/test_samir_repl with TYPE of BAL, arithmetic, missing names, DATE(), invalid strings and nested calls; independent expected N/N/U/D and real III+ transcript.

### B18 - Repair live-date century transport and blank-date display (P2)

Covers: L044, L045. New bug create.

Verdicts: L044 = defect in existing behavior; L045 = defect in existing behavior.

Root-cause area: os/samir/pal/pal_milton.c:720 (first of two related date-pipeline roots) reduces full DOS year modulo 100; cmd/workarea.c:1186 unconditionally adds 1900. cmd/query.c:414 decodes JDN zero as a calendar date without the blank guard already present in core/fn_builtins.c:656.

Pickup: Carry full current year (or century separately) through PAL today while retaining explicit two-digit CTOD year-00 canon. Render blank/invalid date sentinels as the independently specified blank date across ?/LIST/DISPLAY and DTOC.

Grounding: ../dbase3-decomp/specs/runtime/dates-and-century.md and DB manual DATE/CTOD; intentional year-00 -> 1900 remains canon.

Oracle/done means: Inject 1999/2000/2026 full dates and zero/invalid/leap CTOD cases; test PAL/query/REPL and target locked-RTC comparison with DOS DATE. Preserve canon/y2k goldens.

### B19 - Reconcile numeric overflow values with the independent III+ result (P2)

Covers: L050. New bug create.

Verdicts: L050 = defect in existing behavior.

Root-cause area: os/samir/core/eval.c:515-521 deliberately returns XBEE_NUM_OVERFLOW for zero division because its value model has no overflow presentation state. That local design conflicts with the measured untrapped asterisk result.

Pickup: Record the contract adjustment explicitly; represent overflow so numeric formatting produces width-appropriate stars without turning the command into trapped error 39. Preserve true type/domain errors. Do not silently weaken a locked oracle to accept the current behavior.

Grounding: ../dbase3-decomp/re/mint-results-002.md:30-39, real III+ C1/E2 captures; implementation/spec reconciliation required.

Oracle/done means: test_xbase_eval/query/program differential against ../dbase3-decomp/re/mint-results-002.md:30; test 1/0 expression, variable assignment and output widths, not just a single hard-coded transcript.

### B20 - Expand command macros before command classification (P2)

Covers: L108. New feature create.

Verdicts: L108 = feature never built at this entry point.

Root-cause area: os/samir/cmd/flow.c:600-638 split_verb stops at ampersand and does not expand command text; :1032 reports unrecognized verb. No macro command expansion exists before this dispatch.

Pickup: Add bounded character memvar expansion before verb/assignment/flow classification, preserving quote and terminator rules. First pickup: the full-command &CMD case and independent tests for undefined/non-character/self-expanding variables; later macro forms remain explicitly open.

Grounding: ../dbase3-decomp/specs/language/memory-variables.md macro substitution and Using dBASE III Plus.

Oracle/done means: STRESS.PRG target replay must print 1005; real III+ command-macro corpus plus bounded recursion/error tests.

### B21 - Connect INDEX ON and REINDEX to the existing NDX engine (P2)

Covers: L051, L052. New feature create.

Verdicts: L051 = feature never built at this entry point; L052 = feature never built at this entry point.

Root-cause area: os/samir/cmd/query.c:1289-1330 and mutate.c:1045 command hooks contain no INDEX/REINDEX verb. The backend is built: fs/ndx.c:1305 ndx_build and :2623 ndx_rebuild; mutate.c:923 already provides a key callback for PACK.

Pickup: Add a command adapter that parses key expression/destination, evaluates keys and calls the existing build/rebuild APIs with SAFETY and failure cleanup. First sitting: character NAME index and reopening/rebuilding it; retain broader key/scope parity on the feature until graded.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: test_ndx_build/test_ndx_maintain plus full command differential against real III+; target INDEX ON NAME TO TESTIDX and REINDEX then SEEK, independent NDX traversal/readback.

### B22 - Add the DBF import path for APPEND FROM (P2)

Covers: L053. New feature create.

Verdicts: L053 = feature never built at this entry point.

Root-cause area: os/samir/cmd/mutate.c:750-752 accepts only APPEND BLANK, rejecting FROM before any import path. DBF read/write primitives already exist.

Pickup: First sitting: DBF-to-DBF typed field matching and appended rows with safe refusal on incompatible schema, protected input or no space. Preserve original counts on failure using P1 L001. Delimited/SDF variants remain separate future slices.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Real III+ APPEND FROM DBF golden, independent resulting records and target persistence/reopen; codec short-write and schema mismatch cases.

### B23 - Add memory-variable file save and restore (P2)

Covers: L054, L055. New feature create.

Verdicts: L054 = feature never built at this entry point; L055 = feature never built at this entry point.

Root-cause area: os/samir/cmd/flow.c:1016-1034 falls through without SAVE/RESTORE; registered modules at samir_main.c:655 contain no MEM codec/command adapter.

Pickup: First sitting: implement the verified scalar .MEM record subset and SAVE TO/RESTORE FROM with overwrite safety and replacement/additive rules; do not serialize native xb_val pointers. Keep unverified record types explicit.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Real III+ .MEM fixtures/readback and ../dbase3-decomp/specs/file-formats/mem.md; target save, clear, restore values and check failure preservation.

### B24 - Add sorted-table export (P2)

Covers: L109. New feature create.

Verdicts: L109 = feature never built at this entry point.

Root-cause area: os/samir/cmd/query.c:1289-1330 has no SORT command; flow.c:1032 is the common unrecognized-verb endpoint.

Pickup: First sitting: SORT TO on a verified character key for a small DBF, stable record export, independent output creation and SAFETY; preserve source bytes and cursor. Wider multi-key/scoped modes remain feature slices.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Real III+ SORT output ordering and independent DBF reader; target SORT TO SORTED ON NAME, reopen output and compare source unchanged.

### B25 - Add grouped TOTAL export (P2)

Covers: L113. New feature create.

Verdicts: L113 = feature never built at this entry point.

Root-cause area: os/samir/cmd/query.c:1289-1330 has no TOTAL adapter; flow.c:1032 rejects it. No grouped DBF output workflow is built.

Pickup: First sitting: reproduce the fixture TOTAL ON CITY TO TOTAL on independently established group/order and numeric-field rules; write a valid DBF under SAFETY without touching source. Do not infer grouping rules from modern SQL.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Real III+ TOTAL fixture/golden and ../dbase3-decomp/specs/commands/data-definition-and-manipulation.md; target readback of grouping and sums, disk-full preservation.

### B26 - Add database ASCII file verbs through the PAL (P2)

Covers: L110, L111, L112. New feature create.

Verdicts: L110 = feature never built at this entry point; L111 = feature never built at this entry point; L112 = feature never built at this entry point.

Root-cause area: os/samir/samir_main.c:655 registers no file-command module; cmd/flow.c:1032 rejects TYPE/RENAME/ERASE. PAL read/rename/remove already exist (pal_milton.c:484/:500).

Pickup: One small file-command module: parse TYPE and RENAME ... TO and ERASE; call PAL with extension/default-path semantics and accurate errors, protect +R and preserve current work-area state. Test genuinely existing files as well as the absent files used in the audit.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Real III+ named file-command transcripts, host mock PAL and target create/view/rename/erase/missing/protected cases; compare bytes and unchanged active table.

### B27 - Provide a safe RUN command handoff and return (P2)

Covers: L114. New feature create.

Verdicts: L114 = feature never built at this entry point.

Root-cause area: os/samir/cmd/flow.c:1032 rejects RUN; samir_pal_t has no command execution slot. command_repl is kernel-resident and there is no runnable secondary COMMAND in the installation.

Pickup: First sitting: define a bounded PAL command handoff and implement RUN VER with preserved database handles/state and return. Coordinate the secondary-shell feature B30; do not nest a loader over SAMIR storage without an ownership contract.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: PAL handoff mock tests and emulator RUN VER -> database query -> QUIT; failure/abort controls. Real III+ RUN transcript and kernel loader/process state oracle.

### B28 - Reuse existing IF and FOR execution at the interactive prompt (P2)

Covers: L031, L032. New bug create.

Verdicts: L031 = defect in existing behavior; L032 = defect in existing behavior.

Root-cause area: os/milton/command.c:50-142 classifies IF/FOR as external; batch control execution exists only inside run_batch at :3796. Interactive dispatch at :3090 never invokes that layer.

Pickup: Extract shared conditional/FOR execution with explicit prompt-vs-batch percent rules; preserve redirection, ERRORLEVEL and bounded nesting. Literal ONE/TWO and IF EXIST form are this sitting; wildcard FOR work remains on its earlier tracking.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt); IF at the prompt is from memory pending exact golden.

Oracle/done means: test-command/test-batch-exec plus target one-percent FOR and IF EXIST/NOT/ERRORLEVEL controls against DOS 3.3 transcripts.

### B29 - Add F3 command template recall (P2)

Covers: L033. New feature create.

Verdicts: L033 = feature never built at this entry point.

Root-cause area: os/milton/command.c:3067-3069 clears AH=0Ah template count and stores no previous command. os/milton/kbd.c:141 rejects scancodes >=0x3A, including F3, before the DOS ASCII ring.

Pickup: Add extended-key transport and a previous-command template to cooked line input; implement F3 copying the remaining template without conflating this with DOSKEY history. Preserve normal text/control keys and explicit-CON semantics.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Keyboard/cooked-line host state tests plus target ECHO REPEAT, F3, Return and partial-line recall matched to DOS manual pp.154-155.

### B30 - Add the secondary COMMAND /C entry point (P2)

Covers: L034. New feature create.

Verdicts: L034 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 has no COMMAND internal entry and run_external at :2991 searches for a file; the only command_repl at :4180 is linked into the kernel. COMSPEC does not make a runnable COMMAND.COM appear.

Pickup: First sitting: implement a bounded secondary command context for /C ECHO CHILD with environment/handles/exit flags restored to the primary. Support the native model rather than deploying a foreign 16-bit COMMAND. Coordinate P1 L035 permanence.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: test-command and emulator COMMAND /C ECHO CHILD followed by primary VER; nested/error/EXIT/redirection controls against DOS manual COMMAND pp.46-47.

### B31 - Add native worksheet cell hit testing (P2)

Covers: L017. New feature create.

Verdicts: L017 = feature never built at this entry point.

Root-cause area: os/i123/app/face_a.c:431-446 has no mouseDown case; the app draws a worksheet but never converts a content point to a cell.

Pickup: Use the actual viewport geometry from the resize batch to map content points to visible rows/column widths. Ignore borders/status/partial cells and preserve edit/commit cancellation rules.

Grounding: Native document-window direct selection is from memory/usability judgment, not a claim about DOS Lotus mouse setup.

Oracle/done means: Host coordinate-boundary tests and target click E4 after scroll/resize; assert address/highlight and record-flair clip.

### B32 - Install the existing FIND, SORT and MORE filters on the user DOS disk (P2)

Covers: L041, L042, L043. New bug create.

Verdicts: L041 = defect in existing behavior; L042 = defect in existing behavior; L043 = defect in existing behavior.

Root-cause area: os/milton/find_program.asm:1, sort_program.asm:1 and more_program.asm:1 implement the filters. Makefile:27254/:27336/:27423 plants them only in dedicated oracle images. The default flair data installation at Makefile:9777-9785 omits all three. No shared DOS distribution recipe ensures they are on the audit installation/PATH.

Pickup: Create one explicit user-facing DOS utility installation recipe and include the existing filters in a reached PATH location. Test the shipped image, not only bespoke filter disks. Preserve deterministic image metadata and avoid putting filters as arbitrary app icons.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt); existing initech-m0dc is the completed implementation, not evidence that this packaging defect is tracked.

Oracle/done means: Existing test-find-filter/test-sort-filter/test-more-filter plus a new installation-manifest/boot smoke oracle on the actual distributed image. A no-argument parameter response is sufficient to prove reachability; valid redirection/pipe controls prove function.

### B33 - EDLIN: native utility first pickup (P2)

Covers: L107. Existing initech-udr1; no new create.

Verdicts: L107 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native EDLIN command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty EDLIN routine.

Pickup: Load/list/edit/write one ASCII file; establish backup and quit/no-save behavior. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B34 - ATTRIB: native utility first pickup (P2)

Covers: L039. Existing initech-gyvx; no new create.

Verdicts: L039 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native ATTRIB command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty ATTRIB routine.

Pickup: List and set/clear R/H/S/A on one regular file via the existing AH=43h; then wildcard coverage. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B35 - TREE: native utility first pickup (P2)

Covers: L040. Existing initech-zuvo; no new create.

Verdicts: L040 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native TREE command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty TREE routine.

Pickup: Print a read-only directory tree with bounded recursion and correct paths. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B36 - XCOPY: native utility first pickup (P2)

Covers: L105. Existing initech-zuvo; no new create.

Verdicts: L105 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native XCOPY command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty XCOPY routine.

Pickup: Copy one verified directory tree with safe +R/collision/disk-full refusal; capture switch rules before broad parity. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B37 - CHKDSK: native utility first pickup (P2)

Covers: L036. Existing initech-kd4a; no new create.

Verdicts: L036 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native CHKDSK command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty CHKDSK routine.

Pickup: Report allocation/free-space and lost chains without repair; grade corrupt and healthy fixtures before /F. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B38 - FORMAT: native utility first pickup (P2)

Covers: L037. Existing initech-du6v; no new create.

Verdicts: L037 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native FORMAT command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty FORMAT routine.

Pickup: Define a supported FAT12 floppy geometry and a confirmed scratch-media initialize path; reject unsupported devices before any write; /S remains later. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B39 - DISKCOPY: native utility first pickup (P2)

Covers: L038. Existing initech-kusw; no new create.

Verdicts: L038 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native DISKCOPY command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty DISKCOPY routine.

Pickup: Define media/source/destination ownership and copy one supported scratch FAT12 image; preserve source and grade sector bytes. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B40 - DISKCOMP: native utility first pickup (P2)

Covers: L087. Existing initech-kusw; no new create.

Verdicts: L087 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native DISKCOMP command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty DISKCOMP routine.

Pickup: Compare supported image sectors and report the first verified mismatch without writing either medium. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B41 - SYS: native utility first pickup (P2)

Covers: L104. Existing initech-bc7p; no new create.

Verdicts: L104 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native SYS command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty SYS routine.

Pickup: Resolve the current boot/kernel image contract, then implement one supported system-transfer path on copied media; do not promise legacy two-file transfer before the split exists. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B42 - LABEL: native utility first pickup (P2)

Covers: L094. Existing initech-f88h; no new create.

Verdicts: L094 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native LABEL command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty LABEL routine.

Pickup: Read/set/remove a volume label through a verified FAT metadata adapter. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B43 - DEBUG: native utility first pickup (P2)

Covers: L106. Existing initech-udr1; no new create.

Verdicts: L106 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native DEBUG command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty DEBUG routine.

Pickup: First native slice: bounded inspect/dump of an explicit buffer/file and clean quit; raw memory mutation/assembler require later scoped design. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B44 - CTTY: native utility first pickup (P2)

Covers: L086. Existing initech-f9z4; no new create.

Verdicts: L086 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native CTTY command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty CTTY routine.

Pickup: First slice: safely validate and select a supported console device, retaining a recovery route; absent AUX/PRN drivers must be refused. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B45 - SHARE: native utility first pickup (P2)

Covers: L102. Existing initech-ws3x; no new create.

Verdicts: L102 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native SHARE command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty SHARE routine.

Pickup: First pickup is compatibility-policy/contract work on the existing deferred locking bead; record-locking and redirector execution are not a one-sitting feature. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B46 - REPLACE: native utility first pickup (P2)

Covers: L099. New feature create.

Verdicts: L099 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native REPLACE command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty REPLACE routine.

Pickup: Replace one explicit existing file safely with a matching source, honor +R and date rules; wildcard/switch extensions follow. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B47 - COMP: native utility first pickup (P2)

Covers: L085. New feature create.

Verdicts: L085 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native COMP command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty COMP routine.

Pickup: Compare two explicit files and report a bounded first difference/size difference. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B48 - FC: native utility first pickup (P2)

Covers: L090. New feature create.

Verdicts: L090 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native FC command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty FC routine.

Pickup: Implement the verified binary comparison path for two explicit files; line diff is a later slice. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B49 - APPEND: native utility first pickup (P2)

Covers: L081. New feature create.

Verdicts: L081 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native APPEND command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty APPEND routine.

Pickup: First pickup: specify a per-process fallback search-list and add one explicit native file-read fallback; keep it separate from database APPEND. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B50 - SUBST: native utility first pickup (P2)

Covers: L103. New feature create.

Verdicts: L103 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native SUBST command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty SUBST routine.

Pickup: First pickup: define DOS logical-drive mapping over native volumes and one create/query/remove mapping path; no counterfeit drive strings. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B51 - ASSIGN: native utility first pickup (P2)

Covers: L082. New feature create.

Verdicts: L082 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native ASSIGN command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty ASSIGN routine.

Pickup: First pickup: establish drive-redirection lifetime/reset rules and one supported mapping in the DOS resolver. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B52 - JOIN: native utility first pickup (P2)

Covers: L092. New feature create.

Verdicts: L092 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native JOIN command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty JOIN routine.

Pickup: First pickup: establish an independent DOS JOIN fixture and native resolver contract for one mounted subdirectory; implement only after identity/unmount rules are settled. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B53 - BACKUP: native utility first pickup (P2)

Covers: L083. New feature create.

Verdicts: L083 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native BACKUP command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty BACKUP routine.

Pickup: First pickup: establish the independent on-disk backup format and one small single-volume fixture; then a safe explicit-file writer; disk spanning follows. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B54 - RESTORE: native utility first pickup (P2)

Covers: L100. New feature create.

Verdicts: L100 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native RESTORE command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty RESTORE routine.

Pickup: First pickup: decode one independently minted DOS backup fixture into a separate explicit destination under +R/overwrite protection; disk spanning follows. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B55 - RECOVER: native utility first pickup (P2)

Covers: L098. New feature create.

Verdicts: L098 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native RECOVER command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty RECOVER routine.

Pickup: First pickup: establish the period bad-sector/recovery contract and a read-only damage report; repair/rewrite needs its own later fault oracle. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B56 - MODE: native utility first pickup (P2)

Covers: L095. New feature create.

Verdicts: L095 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native MODE command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty MODE routine.

Pickup: First pickup: query and configure one supported console/device mode with validated limits; absent serial/printer modes remain explicit. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B57 - KEYB: native utility first pickup (P2)

Covers: L093. New feature create.

Verdicts: L093 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native KEYB command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty KEYB routine.

Pickup: First pickup: parse one period keyboard-layout fixture and add one reversible layout switch through the PS/2 translation layer. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B58 - CHCP: native utility first pickup (P2)

Covers: L084. New feature create.

Verdicts: L084 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native CHCP command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty CHCP routine.

Pickup: First pickup: query/set one supported code page through a real shared NLS state; no claim that changing a number changes glyphs. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B59 - NLSFUNC: native utility first pickup (P2)

Covers: L096. New feature create.

Verdicts: L096 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native NLSFUNC command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty NLSFUNC routine.

Pickup: First pickup: lock a native equivalent of the country/code-page provider contract and expose one verified table to CHCP; foreign DOS TSR execution is excluded. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B60 - PRINT: native utility first pickup (P2)

Covers: L097. New feature create.

Verdicts: L097 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native PRINT command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty PRINT routine.

Pickup: First pickup: define a real supported printer sink and submit/cancel one ASCII job cooperatively; no successful stub for missing hardware. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B61 - GRAPHICS: native utility first pickup (P2)

Covers: L091. New feature create.

Verdicts: L091 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native GRAPHICS command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty GRAPHICS routine.

Pickup: First pickup: define the native screenshot-to-printer contract for the supported display and independently verified output format; no silent legacy TSR emulation. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B62 - FASTOPEN: native utility first pickup (P2)

Covers: L089. New feature create.

Verdicts: L089 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native FASTOPEN command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty FASTOPEN routine.

Pickup: First pickup: measure the period cache/invalidations contract and define a bounded native lookup cache; performance is a PRD non-goal, so record the product decision before implementing. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B63 - EXE2BIN: native utility first pickup (P2)

Covers: L088. New feature create.

Verdicts: L088 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native EXE2BIN command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty EXE2BIN routine.

Pickup: First pickup: establish file-conversion rules and supported input format; real 16-bit binaries cannot run here. Produce a fixture-backed native conversion design before any writer. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B64 - SELECT: native utility first pickup (P2)

Covers: L101. New feature create.

Verdicts: L101 = feature never built at this entry point.

Root-cause area: os/milton/command.c:50-142 is the internal registry; :2991 resolves external names. Makefile:9777-9785 is the current default data installation locus. No native SELECT command handler or executable/build target was found in os/ and Makefile. These are absence/integration areas, not a claimed faulty SELECT routine.

Pickup: First pickup: establish which installation workflow applies to the custom bootloader/native artifacts and one supported installation fixture; legacy DOS install behavior is not automatically applicable. This is a bounded first sitting; the full utility remains unbuilt until its supported command surface is delivered and graded. Plan larger paths on the same feature owner rather than treating an unsupported message as completion.

Grounding: MS-DOS 3.3 User's Guide, the named command section (local manual and evidence/references/dos33-manual.txt).

Oracle/done means: Capture the named DOS 3.3 command transcript/fixture from the local manual and period software; add the corresponding native host oracle and a locked emulator trace on copied media. Verify installed-name/PATH reachability; for writes use mtools, sector hashes and failure-preservation checks. No oracle for this absent utility was located, so it must be added.

### B65 - Load a SET PROCEDURE library through the existing procedure engine (P2)

Covers: L066. New feature create.

Verdicts: L066 = feature never built at this entry point.

Root-cause area: os/samir/cmd/set.c:619 rejects PROCEDURE; cmd/proc.c:798-869 handles procedures/DO but has no setting-owned external library lifetime.

Pickup: First sitting: load a bounded .PRG library, register its procedures, replace/clear the prior library and connect CLOSE PROCEDURE; retain work-area state. Use shared disk loading rather than stripping commands.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Real III+ RPRTPRO/REPORTS procedure fixture, host proc tests and target SET PROCEDURE; DO named routine; clear; failure controls.

### B66 - Add database default-directory and search-path settings (P2)

Covers: L059, L060. New feature create.

Verdicts: L059 = feature never built at this entry point; L060 = feature never built at this entry point.

Root-cause area: os/samir/cmd/set.c:619 rejects DEFAULT/PATH; samir_main.c:475 passes USE paths straight to work-area open, and no database-owned filename search policy exists.

Pickup: One filename-policy sitting: store default/path state, resolve bare table/program names through it, preserve explicit qualified paths and restore state consistently. USE default-extension handling is already on nds1; do not refile it.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Host path resolver/PAL search-order tests and target tables/programs in separate directories; real III+ missing/qualified/default/PATH transcript.

### B67 - Add alternate transcript output (P2)

Covers: L058. New feature create.

Verdicts: L058 = feature never built at this entry point.

Root-cause area: os/samir/cmd/set.c:619 rejects ALTERNATE; query.c output has no alternate-file routing/ownership state.

Pickup: First sitting: SET ALTERNATE TO and ON/OFF, an owned transcript handle and correctly mirrored query output, with safe close/failure behavior.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Real III+ transcript fixture; host sink tests and target extracted alternate file, ON/OFF/cancel/no-space controls.

### B68 - Apply console and heading output settings (P2)

Covers: L064, L065. New feature create.

Verdicts: L064 = feature never built at this entry point; L065 = feature never built at this entry point.

Root-cause area: os/samir/cmd/set.c:619 rejects CONSOLE/HEADING; query.c:1289 record display and :927 question output do not consult such state.

Pickup: Add settings and consume them in output routing/record heading generation; preserve redirected/alternate/printer destinations according to the III+ rules.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Independent III+ ON/OFF transcripts for LIST/DISPLAY/? and destination combinations; host query/REPL and target controls.

### B69 - Add ESCAPE and BELL input policies (P2)

Covers: L061, L063. New feature create.

Verdicts: L061 = feature never built at this entry point; L063 = feature never built at this entry point.

Root-cause area: os/samir/cmd/set.c:619 rejects ESCAPE/BELL; the PAL input contract has no engine-owned policies for these settings.

Pickup: First sitting: define policy callbacks and apply ESCAPE/BELL in a bounded console command/input path; capture exact abort/beep behavior before extending the missing full-screen editors.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Real III+ setting/key traces, host input mock and target escape/beep acceptance; no success-only storage test.

### B70 - Add CONFIRM behavior to field input (P2)

Covers: L062. New feature create.

Verdicts: L062 = feature never built at this entry point.

Root-cause area: os/samir/cmd/set.c:619 rejects CONFIRM. The field-input/READ editor consuming it is not built in the registered modules (samir_main.c:655).

Pickup: First pickup: implement the setting with one real bounded field input path and its end-of-field acceptance rule. Coordinate editor/READ work on u78k; accepting SET alone does not finish the feature.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Real III+ CONFIRM ON/OFF field-boundary key traces and host/target input acceptance.

### B71 - Add function-key definitions and line-input expansion (P2)

Covers: L115. New feature create.

Verdicts: L115 = feature never built at this entry point.

Root-cause area: os/samir/cmd/set.c:619 rejects FUNCTION; samir_main.c:706 reads plain PAL lines with no function-key macro state.

Pickup: One line-input sitting: SET FUNCTION 1 TO LIST, stored bounded key definitions and cooked function-key expansion with clear reset/disabled rules. Coordinate extended DOS key transport, not the existing generic error mapping.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Real III+ SET FUNCTION key golden and target F1 expansion/execute/clear controls; host bounded-input tests.

### B72 - Implement CLEAR MEMORY over existing variable storage (P2)

Covers: L116. New feature create.

Verdicts: L116 = feature never built at this entry point.

Root-cause area: os/samir/cmd/flow.c:985-988 already implements RELEASE ALL, but :1016-1034 has no CLEAR MEMORY route.

Pickup: Add the documented CLEAR MEMORY semantics to the existing scoped variable store after checking public/private/parameter rules; preserve procedures, open DBFs and screen state.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Real III+ scope/variable transcript and target STRESS then CLEAR MEMORY; ? N must be undefined while the active table still works.

### B73 - Implement transcendental functions on their existing feature (P2)

Covers: L067, L068, L069. Existing initech-7az.13; no new create.

Verdicts: L067 = feature never built at this entry point; L068 = feature never built at this entry point; L069 = feature never built at this entry point.

Root-cause area: os/samir/core/fn_builtins.c:2123-2134 dispatches only the freestanding arithmetic/date-name subset; :2169 rejects EXP/LOG/SQRT.

Pickup: Follow initech-7az.13 for freestanding numeric strategy and independent domain/precision goldens; do not create another math feature. The older closed 7az.11 description included these but its shipped title/subset did not.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Real III+ function-value/domain corpus and target EXP(1)/LOG(1)/SQRT(4); cross-host/soft-float behavior and mutation-proven differential.

### B74 - Expose actual time and free disk space to database functions (P2)

Covers: L070, L072. New feature create.

Verdicts: L070 = feature never built at this entry point; L072 = feature never built at this entry point.

Root-cause area: os/samir/core/fn_builtins.c:2169 rejects TIME/DISKSPACE; include/samir/pal.h:169 exposes only today, no time/free-space callbacks.

Pickup: Add injectable PAL time/free-space slots backed by DOS AH=2Ch/AH=36h and dispatch their functions; define error units/format and avoid hard-coded success values.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: PAL contract tests with fixed clock/disk geometry, independent III+ function semantics and target TIME/DISKSPACE versus DOS controls.

### B75 - Add database record-size and product-identification functions (P2)

Covers: L071, L073, L074. New feature create.

Verdicts: L071 = feature never built at this entry point; L073 = feature never built at this entry point; L074 = feature never built at this entry point.

Root-cause area: os/samir/core/fn_builtins.c:2142-2150 has DB cursor functions but no RECSIZE/VERSION/OS; :2169 rejects all three. DBF record size is already available at fs/dbf.c:688.

Pickup: Add RECSIZE through the cursor contract and period-plausible InitechBase/InitechDOS identification strings. Return 31 including the deletion byte for the audit table; handle no active DB per the independent III+ rule.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Builtin/cursor tests, real III+ return types and target calls; literal Initech version strings are product decisions, not copied dBASE identity.

### B76 - Expose the real console cursor position (P2)

Covers: L077, L078. New feature create.

Verdicts: L077 = feature never built at this entry point; L078 = feature never built at this entry point.

Root-cause area: os/samir/core/fn_builtins.c:2169 rejects ROW/COL; PAL has cursor positioning but no position query contract.

Pickup: Maintain/query actual row/column across console output and cursor movement, add PAL slots and builtins, and preserve III+ indexing and redirected-output behavior.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Host cursor state and real III+ movement/output traces; target ROW/COL after known text and cursor placement.

### B77 - Expose printer position through real printer output state (P2)

Covers: L075, L076. New feature create.

Verdicts: L075 = feature never built at this entry point; L076 = feature never built at this entry point.

Root-cause area: os/samir/core/fn_builtins.c:2169 rejects PCOL/PROW; no database printer-output/cursor contract is registered by samir_main.c:655.

Pickup: First pickup: define a real printer sink/state contract, maintain row/column on output and return it through builtins. This depends on printer output; constants or a success-only stub do not complete it.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: Real III+ printer-position golden with a captured printer stream; host sink/counter and target output tests once the supported printer path exists.

### B78 - Add a bounded REPORT FORM renderer (P2)

Covers: L056. New feature create.

Verdicts: L056 = feature never built at this entry point.

Root-cause area: os/samir/samir_main.c:655 registers no report module; flow.c:1032 rejects REPORT before any FRM read. No native FRM command renderer was found.

Pickup: First sitting: inspect one independently minted FRM and render its verified field/header/detail subset to a text sink with SAFETY-independent read behavior. Broader layout, scopes and printer control remain explicit feature slices.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: ../dbase3-decomp/specs/file-formats/frm.md and reports-labels/report-form-generator.md; real III+ report golden, host renderer and target form/DBF readback.

### B79 - Add a bounded LABEL FORM renderer (P2)

Covers: L057. New feature create.

Verdicts: L057 = feature never built at this entry point.

Root-cause area: os/samir/samir_main.c:655 registers no label module; flow.c:1032 rejects LABEL. No native LBL renderer was found.

Pickup: First sitting: read one independently minted LBL and produce its verified text/field label layout; keep multi-column/scoped/printer variants as later slices.

Grounding: ../dbase3-decomp/specs/ and Using dBASE III Plus, the named command/function section cited by REPORT.md.

Oracle/done means: ../dbase3-decomp/specs/file-formats/lbl.md and reports-labels/label-form-generator.md; real III+ label golden, host renderer and target fixture.

### B80 - Add reached worksheet context help (P2)

Covers: L012. New feature create.

Verdicts: L012 = feature never built at this entry point.

Root-cause area: os/i123/app/face_a.c:354-411 has no F1 handler/help state; :91 installs no Help command.

Pickup: One help sitting: F1 opens concise context help for the actual READY/LABEL/VALUE/EDIT behavior and returns without losing input; expose a native Help route too.

Grounding: Lotus R2.2 Reference Manual Chapter 1 (printed pp.1-10 to 1-17), cited by REPORT.md; native mouse behavior is from memory.

Oracle/done means: Lotus manual p.1-11 context behavior and target F1 open/close/state preservation trace plus record-flair clip.

### B81 - Add the worksheet status-line clock on its existing bead (P3)

Covers: L021. Existing initech-68iw.1; no new create.

Verdicts: L021 = feature never built at this entry point.

Root-cause area: os/i123/app/face_a.c:75 defines a status row, but paint_all does not render the period clock/date; header :43-44 explicitly defers it.

Pickup: Follow the existing status-line clock bead with injected time and reference-backed placement.

Grounding: Lotus R2.2 Reference Manual Chapter 1 (printed pp.1-10 to 1-17), cited by REPORT.md; native mouse behavior is from memory.

Oracle/done means: Real Lotus MINT01 status line plus locked-RTC target pixels and record-flair clip.

### B82 - Add the caution alert icon on its existing bead (P3)

Covers: L022. Existing initech-tdnl.87; no new create.

Verdicts: L022 = feature never built at this entry point.

Root-cause area: os/milton/kmain.c:4056 explicitly defers the alert icon; finder_trash_confirm at :4081 builds only text/buttons.

Pickup: Follow the existing icon bead using the independent Mac OS 8.1 caution-alert asset/layout reference.

Grounding: ../system7-decomp/specs/sys8/controls.md section 6 and goldens/captures/s8_alert_modal.png.

Oracle/done means: Mac OS 8.1 s8_alert_modal capture plus alert render oracle and target screenshot/clip.

### B83 - Render the Platinum alert edge instead of the heritage black slab (P3)

Covers: L023. New bug create.

Verdicts: L023 = defect in existing behavior.

Root-cause area: os/flair/dialog.c:423-440 draws four solid 7-pixel black bands for dBoxProc. This is explicit heritage geometry applied to the default Platinum alert.

Pickup: Implement era/skin-specific alert chrome from the independent Sys8 golden while retaining System 7 heritage geometry. Record any locked metric adjustment explicitly.

Grounding: ../system7-decomp/specs/sys8/controls.md section 6; separate heritage specs/chrome/dialog-borders.md.

Oracle/done means: Independent s8_alert_modal.png pixel bands and dialog host/target render trace; mutation of border colors/thickness must fail and record-flair clip required.

### B84 - Count allocated Trash space including directories (P3)

Covers: L024. New bug create.

Verdicts: L024 = defect in existing behavior.

Root-cause area: os/flair/finder_windows.c:1009 adds only dirent logical file_size, so directory chains contribute zero. kmain.c:4121 rounds that incomplete total up to K.

Pickup: Add an allocation-size query to the filesystem adapter and sum actual allocated file/directory chains without double counting; or explicitly label the display as logical bytes. The requested disk-space reading needs 2K for the three 512-byte chains.

Grounding: Filesystem-feedback expectation in REPORT.md; Mac alert reference labels the figure as disk space.

Oracle/done means: Independent FAT chain accounting for nested/empty/multi-cluster directories/files, target trash-fixture alert and mtools comparison; record-flair clip.

### B85 - Put the next database prompt on a fresh line (P3)

Covers: L046. New bug create.

Verdicts: L046 = defect in existing behavior.

Root-cause area: os/samir/cmd/query.c:907-928 implements the authentic leading newline for ? but no trailing newline; samir_main.c:704 writes the next dot prompt unconditionally without line-position policy.

Pickup: Track REPL output line position and start the next prompt on a fresh line. Preserve ? leading-newline and ?? concatenation semantics inside programs; do not fix prompt glue by changing those language operators.

Grounding: From memory for dot-prompt layout; manual examples separate commands; programming-and-io.md grounds ? versus ??.

Oracle/done means: Host/target REPL ?/??/LIST/error/empty command transcripts and independent III+ prompt layout; redirected output control.

### B86 - Repaint CTENANT input readouts from retained state (P3)

Covers: L117. New bug create.

Verdicts: L117 = defect in existing behavior.

Root-cause area: os/apps/ctenant/ctenant.c:49-54 paint clears the whole field and redraws only the startup text; key/mouse branches draw transient readouts at :77-82 without painting retained state.

Pickup: Retain last key and clicked state and paint all readouts in updateEvt; event branches update state then use the same renderer. The app is a diagnostic fixture, not a new document editor.

Grounding: Ordinary retained GUI state is from memory/usability expectation; Toolbox update event contract.

Oracle/done means: test-flair-ctenant extended with input then move/obscure/reveal, checking both proportional/cell key lines and Click; record-flair clip.

## Completion accounting

- Findings: 118, each assigned once; ten P1 sections and 86 P2/P3 batches.
- Verdicts: 34 defects in existing behavior; 84 unbuilt features.
- Tracking: 23 findings mapped to existing beads; 95 new findings.
- New commands: 7 P1 bugs + 68 batches = 75 bd creates.
- Source root-cause coverage: 10/10 P1s; no P1 cause left unfound.
- Remaining uncertainty: exact historical edge transcripts called out above,
  legacy-utility native/scope decisions, kkht's precise older-finding mapping,
  and live bead status. Nothing here claims to have run an oracle.
- The full utility/report/function roadmap remains on each feature owner after
  its bounded first pickup; unsupported replies alone do not close those gaps.
