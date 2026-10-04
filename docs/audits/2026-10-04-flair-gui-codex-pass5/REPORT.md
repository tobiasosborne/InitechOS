# InitechOS FLAIR and DOS audit: fifth pass

Guest testing: 2026-10-04, approximately 23:03-23:45 Europe/Berlin.
Findings only; no fixes or builds.
Verdicts apply to the private disk copies and routes exercised below.

## The two new claims

| Claim | Verdict | Evidence |
| --- | --- | --- |
| Long held gestures dispatch the highlighted item in the visible menu | **holds** | The H01-style File jitter and cross-title gesture ended on visible View > by Icons and dispatched `VIEW_ICONS`, selection `0x02020001`: [held](shots/17-correct-long-menu-held.png), [result](shots/18-correct-long-menu-release.png). A later 75.14-second gesture crossed File/Edit/View/Special/Help and ended on enabled Arrange (by Name); it dispatched `ARRANGE_BY_NAME`, `0x0202000C`, and actually sorted the folder: [held](shots/99-late-enabled-arrange-held.png), [result](shots/100-late-arrange-result.png), [raw trace](evidence/endurance-serial.log). A 120.11-second gesture also kept tracking, but ended on a disabled item and dispatched nothing. [Timing](evidence/held-menu-timing.json). No wrong-command dispatch was observed. |
| Managed TRASH and DESKTOP.DB are invisible and inaccessible in Finder; the pristine root has two icons | **holds** | Fresh root and Select All showed only README.TXT/APPS, including after movement, zoom, close/reopen and a child-window visit: [fresh root](shots/02-root-two-icons.png), [Select All](shots/185-service-root-select-all.png), [zoom](shots/187-service-root-zoom.png), [reopen](shots/188-service-root-reopen.png), [APPS](shots/189-service-apps-child.png). Double-clicking the old service-icon grid slots selected empty space and opened nothing: [result](shots/190-service-old-slots-open-none.png), [trace](evidence/services-final-serial.log). No service item could be selected as an Open/move source or folder-drop target. Normal desktop Trash staging still succeeded after endurance and in the independent final test: [endurance disk contents](evidence/endurance-trash-after.txt), [fresh settled frame](shots/193-trash-five-seconds-after.png), [final disk contents](evidence/services-final-trash-after.txt). These conclusions concern the managed root objects, not every user file elsewhere with the same basename. |

The pass-4 spot checks did not establish a regression in its specific fixes.
Moved/zoomed/restored Finder contents followed their frames; arrows scrolled
without moving the window; Select All and Arrange worked; the tested disabled
Finder commands stayed disabled; last-window closure left Finder foreground;
explicit Ctrl/Shift releases did not turn plain N/W into commands; and Close
cancelled outside its box. See [box cancellation](shots/52-real-close-cancel.png),
[late selection](shots/101-late-select-all.png),
[late modifier check](shots/103-late-modifiers.png), and
[disabled File commands](shots/109-finder-disabled-file.png).
This was a spot check, not another reproduction of J01-J05. The broader
no-enabled-dead-command verdict remains partly true: the known disk-app issue
is excluded, and K07 adds evidence from the built-in apps.

## New findings

P1 means a broken core operation, data loss, or a blocked recovery path.
P2 means a substantial missing workflow or misleading behavior. P3 means a
presentation/orientation defect. The first three findings deserve immediate
attention because they change or remove data outside the intended operation.

### K01 - P1 - COPY onto the same file through an absolute path destroys a multi-cluster file

**Do:** Boot `pristine-dos.img` with `dos-repro-fixture.img` as disk 2. In A:\,
run `COPY SELF.BIN .\SELF.BIN`, `DIR`, then
`COPY SELF.BIN A:\SELF.BIN`, `DIR`. SELF.BIN is the valid 8,192-byte pattern
file prepared by [these commands](evidence/dos-repro-preparation.json).
Stop QEMU and extract SELF.BIN with mcopy. Independently, in dos-main,
`COPY SAMIR.COM BIG.BIN` then `COPY BIG.BIN A:\BIG.BIN` exercises an
80,928-byte source.

**Saw:** The .\ spelling was safely refused with "File cannot be copied onto
itself" and zero copied. The absolute spelling returned "Bad command or file
name" after shrinking SELF.BIN to 512 bytes. The extracted bytes also differ
from the original first 512 bytes. BIG.BIN likewise shrank from 80,928 to 512.
A separate 114-byte same-file alias copy survived; this is not a claim that
all sizes fail.

**Expected and why:** Resolve both names to the same file before truncation
and refuse the copy while preserving its contents. The same-file refusal and
absolute-path equivalence are expectations from my memory of DOS 3.3. Data
preservation when an operation fails is an ordinary filesystem expectation.

**Evidence:** [Safe spelling control](shots/168-copy-self-guard-control.png),
[large-file loss](shots/153-large-copy-after-alias.png),
[512-byte SELF.BIN visible above the next test](shots/170-copy-overwrites-nonempty-folder.png),
[fresh raw trace](evidence/dos-check-serial.log),
[extracted-byte verification](evidence/dos-check-data-loss-verification.json),
[stopped root listing](evidence/dos-check-root-after.txt).

### K02 - P1 - COPY to an existing directory replaces the directory with a file

**Do:** On the same fresh DOS fixture, verify
`TYPE FILLED\SAVE.TXT` prints ROOT-SENTINEL. Run
`COPY README.TXT FILLED`, then `CD FILLED` and `TYPE FILLED`.
The independently tested TARGET2 was created with MD and populated by
`COPY README.TXT TARGET2\KEEP.TXT` before copying README onto TARGET2.

**Saw:** COPY reported one file copied. CD now said "Invalid directory".
TYPE FILLED printed the README. The stopped FAT root identifies FILLED as a
114-byte ordinary file, and its former SAVE.TXT is unreachable through that
path. Both the empty TARGET and populated TARGET2 were also replaced.

**Expected and why:** An existing destination directory should receive
README.TXT inside it, preserving the directory and its children. That COPY
syntax comes from my memory of DOS. At minimum, an unsupported directory
operand should be rejected before modifying the directory.

**Evidence:** [Sentinel before and file replacement after](shots/170-copy-overwrites-nonempty-folder.png),
[independent TARGET2 result](shots/151-copy-directory-after-probe.png),
[fresh trace](evidence/dos-check-serial.log),
[stopped root](evidence/dos-check-root-after.txt),
[failed directory listing](evidence/dos-check-filled-after.txt).

### K03 - P1 - A path-qualified wildcard DEL deletes matching files in the current directory

**Do:** In A:\ on either DOS fixture, verify root KEEP.TXT prints
ROOT-SENTINEL and DELTEST\KEEP.TXT prints SUB-SENTINEL. Run
`DEL DELTEST\*.TXT`, then TYPE both names again.

**Saw:** Root KEEP.TXT disappeared with no warning. DELTEST\KEEP.TXT remained
readable, and the subdirectory's Z00.TXT-Z19.TXT also remained. Thus the command
removed an unrelated current-directory file while leaving its requested
matches in place. This reproduced on fresh copies.

**Expected and why:** Delete only files selected by the complete path/pattern.
This is ordinary path semantics and matches my memory of DOS DEL.

**Evidence:** [First result](shots/149-del-wrong-directory.png),
[fresh result](shots/171-del-wrong-directory-fresh.png),
[fresh trace](evidence/dos-check-serial.log),
[first stopped subdirectory](evidence/dos-main-deltest-after.txt),
[fresh stopped root without KEEP.TXT](evidence/dos-check-root-after.txt).
The fresh subdirectory was subsequently deliberately emptied in K15.

### K04 - P2 - Wildcard DEL silently stops after sixteen matches

**Do:** CD DELTEST on the 20-file Z00.TXT-Z19.TXT fixture and run
`DEL Z*.TXT`, then DIR. Repeat on a fresh copy.

**Saw:** Z00-Z15 were removed; Z16-Z19 remained. There was no partial-operation
message or continuation prompt. These are healthy short text files, not a
full disk or malformed directory.

**Expected and why:** Process every matching file, or explicitly disclose a
limit and the unfinished operation. This is ordinary wildcard-command
semantics; processing a modest directory is also expected from my DOS memory.

**Evidence:** [First remainder](shots/150-del-sixteen-only.png),
[fresh remainder](shots/172-del-limit-fresh.png),
[stopped first directory](evidence/dos-main-deltest-after.txt),
[raw fresh trace](evidence/dos-check-serial.log).

### K05 - P1 - SAMIR accepts SET FILTER but LIST ignores it

**Do:** From a fresh desktop with the pristine samir_list disk copy, press
backslash. `USE CLIENTS.DBF`; `LIST FOR BAL > 1000` as a control. Then
`SET FILTER TO BAL > 1000`, `GO TOP`, `LIST`. Also `GO 2` and
`? BAL > 1000`.

**Saw:** LIST FOR returned records 1 and 3. SET FILTER returned to the prompt
without error, but the subsequent LIST also included WADDAMS with BAL=-42.
The predicate at record 2 evaluated .F. The same filter failure appeared in
the broader session. FILTER() itself was unavailable and returned error 31.

**Expected and why:** After GO TOP, navigation and scans should honor the
active work-area filter. The on-disk
[SET FILTER reference](evidence/references/navigation-query-display.md)
explicitly documents that behavior and the need to reposition after installing
it. This is not an assertion based only on historical memory.

**Evidence:** [Working FOR control](shots/137-list-for-control.png),
[ignored filter](shots/138-filter-not-applied.png),
[fresh transcript](evidence/samir-filter-serial.log).

### K06 - P2 - SAMIR displays a deleted record without marking it as deleted

**Do:** `USE CLIENTS.DBF`, `GO TOP`, `DELETE`, `? DELETED()`, `DISPLAY`.
Then PACK and LIST to verify the deletion really happened.

**Saw:** DELETED() printed .T. DISPLAY printed record 1 PESTON as an ordinary
row with no deletion marker. PACK then removed that record, reducing the
count from three to two. LIST in the earlier delete/recall session likewise
showed an unmarked deleted row. SET DELETED ON/OFF could not provide an
alternative view: both returned "7 File already exists."

**Expected and why:** With deleted records included, distinguish their state
before the user packs the table. The on-disk
[SET DELETED reference](evidence/references/navigation-query-display.md)
specifies a leading * in LIST/DISPLAY when DELETED is off.

**Evidence:** [True flag and unmarked row](shots/175-deleted-row-unmarked.png),
[PACK result](shots/176-pack-result.png),
[earlier deletion workflow](shots/131-database-delete-recall.png),
[raw state session](evidence/samir-state-serial.log).

### K07 - P2 - Built-in apps expose enabled commands without delivering a usable result

**Do:** Activate NOTES by its title. Type `fifth pass notes`. Select lower
File > New, Open, Save and Quit, and Notes > About; also try Ctrl-S and the
Edit menu's Undo/Cut/Copy/Paste. Activate HELLO and select its lower File >
New and the tested Edit/Image/Layer items. Repeat NOTES Quit late in
endurance, then close with its title box.

**Saw:** NOTES never displayed typed text, a new document, a chooser, a save
result or an About dialog. Its enabled Quit returned menu 384/item 4 and left
the app live. The Edit selections had no visible effect. HELLO's enabled New
also produced no new window. Both apps' content clicks
toggled their demo marker and their window controls worked, so they were
receiving input. Title-box closure worked in all six app orders.

**Expected and why:** An enabled named command should act or explain its
limitation. This is ordinary GUI semantics. My expectation of New/Open/Save,
editable text and clipboard operations in a Notes-like application comes from
classic Mac memory. The built-ins are acknowledged demo tenants; their menu
promises still give the user no useful result.

**Evidence:** [Enabled NOTES File](shots/12-notes-file-open-held.png),
[typing/save result](shots/09-notes-save-result.png),
[About result](shots/10-notes-about-result.png),
[HELLO File](shots/69-hello-file-menu.png),
[late NOTES Quit result](shots/104-notes-still-live-late.png),
[menu dispatches](evidence/endurance-serial.log).
This finding concerns the built-ins. The filed disk-app menu-delivery issue
and upper-bar Quit are not reported again.

### K08 - P2 - SAMIR cannot perform basic interactive database and inspection workflows

**Do:** After `USE CLIENTS.DBF`, try SUM BAL, COUNT, DISPLAY STRUCTURE, BROWSE,
EDIT, APPEND, CREATE AUDIT, ASSIST, HELP, COPY TO BACKUP, and DO UPDATE.
UPDATE.PRG is a real file on the derived disk containing USE/GO TOP/REPLACE/
LIST/RETURN. The other tests use the shipped table directly.

**Saw:** SUM, COUNT, BROWSE, EDIT, CREATE, ASSIST, HELP, COPY TO and DO returned
"16 *** Unrecognized command verb." DISPLAY STRUCTURE returned
"1 Internal error." Bare APPEND returned "7 File already exists." No
interactive table/structure editor or assistance interface opened, no export
appeared, and UPDATE.PRG did not run. APPEND BLANK and direct REPLACE did work.

**Expected and why:** The copied dBASE III references document
[interactive creation/editing](evidence/references/assist-ui-and-editors.md),
[APPEND and COPY TO](evidence/references/data-definition-and-manipulation.md),
and [structure inspection and aggregates](evidence/references/navigation-query-display.md).
[DO of program files](evidence/references/control-flow-and-procedures.md) is
also documented. These are ordinary database-user workflows, not an expectation
of every obscure verb. Unsupported features should also give a relevant explanation instead of
an unrelated existing-file or internal-error message.

**Evidence:** [Inspection commands](shots/127-database-inspection-commands.png),
[interactive requests](shots/128-database-ui-gaps.png),
[export/program requests](shots/132-database-export-program-errors.png),
[fixture preparation](evidence/samir-fixture-preparation.json),
[raw session](evidence/samir-work-serial.log).

### K09 - P2 - USE requires the DBF extension and misidentifies the valid table

**Do:** On the pristine samir_list copy, type `USE CLIENTS`, then
`USE CLIENTS.DBF` and LIST.

**Saw:** USE CLIENTS returned "15 Not a dBASE database." The explicit .DBF
spelling immediately opened the same supplied table and listed its three rows.
The failed spelling did not identify a missing file or explain the extension.

**Expected and why:** Default an unqualified database name to .DBF. That is
an expectation from my memory of dBASE III dot-prompt use, not a measured
historical guest. A failure should accurately identify the problem.

**Evidence:** [Short USE failure](shots/136-use-implicit-extension.png),
[explicit-name success](shots/137-list-for-control.png),
[raw transcript](evidence/samir-filter-serial.log),
[pristine disk listing](evidence/samir-pristine-directory.txt).

### K10 - P2 - DIR ignores operands and always labels the listing as the root

**Do:** In the supplied DOS fixture, run DIR, `DIR *.TXT`, `DIR APPS`,
`DIR NOFILE.ZZZ`, `DIR /W`. Then MD WORK, CD WORK, MD SUB, CD SUB, CD ..,
CD, DIR.

**Saw:** All root DIR variants listed the whole root, including non-TXT files
and entries unrelated to APPS or NOFILE.ZZZ. /W gave the same layout.
Inside WORK, DIR enumerated WORK's actual entries but headed them
"Directory of A:\", while CD and the prompt correctly said A:\WORK.

**Expected and why:** Honor a file pattern/directory operand and identify the
directory being listed; a nonexistent file spec should not silently mean all
files. DIR patterns, directory operands and /W are expectations from my
memory of DOS 3.3. Accurate location labeling is ordinary shell semantics.

**Evidence:** [Argument tests](shots/142-dos-dir-arguments.png),
[nested header mismatch](shots/143-dos-nested-navigation.png),
[raw DOS transcript](evidence/dos-main-serial.log).

### K11 - P2 - COPY/REN and batch FOR cannot carry out wildcard file work

**Do:** With several existing .TXT files, `COPY *.TXT MERGED.TXT`,
`REN *.TXT *.BAK`, then run CHECK ARGUMENT. CHECK.BAT contains
`FOR %%F IN (*.TXT) DO ECHO ITEM=%%F` plus IF EXIST and CALL controls.

**Saw:** COPY said "File not found", REN said "Bad command or file name",
and FOR printed exactly `ITEM=*.TXT` once. It did not enumerate files.
The same batch's argument substitution, IF EXIST, nested CALL and following
commands worked, isolating this from a failed batch launch.

**Expected and why:** Wildcards should enumerate the matching files and apply
the command's DOS semantics. This comes from my memory of DOS COPY, REN and
batch FOR. Explain an unsupported pattern instead of claiming that existing
matching files are absent.

**Evidence:** [COPY/REN results](shots/148-dos-copy-wildcard-gaps.png),
[batch result](shots/155-dos-batch.png),
[exact batch](fixtures/CHECK.BAT), [raw transcript](evidence/dos-main-serial.log).

### K12 - P1 - DOS Ctrl-Z and Ctrl-C become ordinary letters; a console read cannot be ended normally

**Do:** Run `ECHO HELLO | TYPE CON`. When TYPE CON waits for console input,
send Ctrl-Z then Enter. Repeat with explicit QMP Ctrl down, Z down/up, Ctrl
up, Enter. Then the same explicit sequence for Ctrl-C and Enter.

**Saw:** The read printed `z`, another `z`, and `c`. It never returned the
COMMAND prompt. QMP remained responsive and stopped the guest normally.
BREAK was on in the fresh follow-up shell. This is a blocked console workflow,
not a claim of a spontaneous desktop or CPU hang; TYPE CON waiting for input
in the first place is not the finding.

**Expected and why:** Ctrl-Z should provide DOS console EOF and Ctrl-C should
allow interruption rather than deliver plain letters. These keyboard/recovery
expectations come from my memory of DOS. Explicit key states distinguish this
from a too-short harness chord or the earlier fixed FLAIR modifier latch.

**Evidence:** [Letters and no returned prompt](shots/158-con-control-keys-do-not-end.png),
[raw blocked session](evidence/dos-main-serial.log),
[explicit press/release journal](evidence/actions.jsonl),
[BREAK state in follow-up](evidence/dos-extra-serial.log).

### K13 - P2 - SAMIR cannot read multiple commands from redirected standard input

**Do:** In COMMAND, create INPUT.TXT with three ECHO commands and > / >>,
containing `use clients.dbf`, `list`, `quit`, one CR/LF line each. Run
`SAMIR < INPUT.TXT > DBOUT.TXT` and TYPE DBOUT.TXT. Repeat. As a control,
redirect a one-line USE file to SAMIR.

**Saw:** Both three-line runs returned only dot prompts and
"36 Unrecognized phrase/keyword in command." No table rows appeared. The
one-line USE control returned without that error. The same USE/LIST/QUIT
sequence worked when typed interactively in COMMAND-launched SAMIR.

**Expected and why:** Consume a logical input line at a time, preserving the
same command sequence across input sources. This follows the on-disk
[conin_line contract](evidence/references/samir-pal.h), which promises one input
line without a trailing newline. It is an application integration expectation;
I did not establish that historical dBASE III accepted this exact shell syntax.

**Evidence:** [One-line control](shots/164-single-line-stdin-control.png),
[three-line repetition](shots/165-multiline-stdin-reproduction.png),
[interactive program succeeds](shots/162-dos-program-returns.png),
[raw transcript](evidence/dos-extra-serial.log),
[extracted input and output](evidence/dos-redirection-files.json).

### K14 - P3 - An unlabeled mounted volume has no visible desktop name

**Do:** Boot FLAIR with the pristine samir_list copy, which mdir reports has
no volume label. Open its disk icon; also test the derived SAMIR disk.

**Saw:** The disk icon had an empty label/one-pixel black selection strip.
The window could open, but the icon gave no visible name or drive identifier.
The labeled flair_data disk displayed INITECH normally.

**Expected and why:** Supply a readable fallback identifier for an unlabeled
volume. This is ordinary orientation/accessibility semantics; an Untitled or
drive-letter fallback is an expectation from my classic Mac/PC memory.

**Evidence:** [Unlabeled desktop](shots/121-control-desktop-pre.png),
[opened unlabeled volume](shots/126-work-desktop-pre.png),
[no-label disk listing](evidence/samir-pristine-directory.txt),
[labeled comparison](shots/02-root-two-icons.png).

### K15 - P2 - DEL *.* removes the remaining files without confirmation

**Do:** After K04's fresh test leaves five real files in DELTEST, run
`DEL *.*`, then DIR. Do not supply any Y/N response.

**Saw:** The shell immediately returned. DIR showed only . and ..; all five
files had been removed. No confirmation or undo route appeared.

**Expected and why:** Ask for confirmation for the special all-files wildcard
before deleting. That safeguard is expected from my memory of DOS 3.3 DEL;
this is not a request for a GUI Trash mechanism on DOS.

**Evidence:** [Before/after with no confirmation input](shots/173-del-all-no-confirmation.png),
[raw transcript](evidence/dos-check-serial.log),
[empty stopped directory](evidence/dos-check-deltest-after.txt),
[input journal](evidence/actions.jsonl).

## Coverage, successful behavior, and limits

### Applications and endurance

One uninterrupted live desktop session lasted **949.05 seconds (15 minutes
49 seconds)**. It included 20 successful disk-app loads and 20 exits, 17 new
folders across several parents, 27 child-folder opens, 9 scrolling operations,
held menus, selection, many window moves, zoom/collapse changes, and
switches among Finder/HELLO/NOTES/TenantFix. Every disk-app teardown reported
`TENANT-HEAPAVAIL n=3523968`. Four repeat-state frames after mixed cycles 3,
6, 9 and 12 were pixel-identical. No accumulating drawing drift, obvious
slowdown, desktop hang or guest panic was observed in the inspected states.
This tests those paths for fifteen minutes, not all heap allocations or
long-term uptime. [Summary](evidence/endurance-summary.json),
[frame comparison](evidence/endurance-frame-comparison.json),
[raw trace](evidence/endurance-serial.log).

TENANTFX launched from Finder, showed its own bar, switched foreground,
repainted after geometry changes, cancelled a Close released outside, closed
inside, and relaunched. Launching it again while resident produced
`TENANT-SLOT-BUSY` and no second instance; I exclude the already-filed silent
launch-refusal/menu-delivery issues. Its deliberate content-click exit worked.
Built-in HELLO and NOTES launched again at each cold boot. Their tested
menus provided no user relaunch route after title-box closure (K07).

Six independent sessions closed all three apps in HNT, HTN, NHT, NTH, THN and
TNH order (H=HELLO, N=NOTES, T=TenantFix). Finder windows were closed first;
HELLO was moved to (20,350), NOTES to (330,250), TenantFix retained (100,160),
so all title boxes were reachable. Activate each title, then click its box.
Every session has three `FLAIR-CLOSE` markers, one disk teardown, and an empty
Finder desktop afterward. [All final frames](evidence/close-order-contact.png),
[order summary](evidence/app-order-summary.json), [actions](evidence/actions.jsonl).

### Database and desktop return

Four real SAMIR sessions were reached by the system backslash hotkey. The
first used an unmodified copy of samir_list.img; later valid derived disks
added UPDATE.PRG and the supplied APPS/TENANTFX.EXE for coverage. USE with an
explicit extension, LIST, GO/SKIP, DISPLAY, LOCATE/CONTINUE, FOUND/EOF/RECNO/
RECCOUNT/DELETED expressions, LIST FOR, direct REPLACE, APPEND BLANK, DELETE,
RECALL, PACK and ZAP were exercised. Writes survived CLOSE/reopen and were
verified in the stopped DBF: [parsed records](evidence/clients-work-after.json).
PACK reduced three records to two; ZAP reduced them to zero, and appending a
new LAST/TEST/1.25 record afterward worked.

The simple desktop, a moved Finder window with newly created folders, and a
resident disk app all returned **pixel-identical** at the same pointer
position. The disk app subsequently closed and reclaimed its heap. See
[control comparison](evidence/samir-control-frame-comparison.json),
[changed-desktop comparison](evidence/samir-work-frame-comparison.json), and
[disk-app comparison](evidence/samir-app-frame-comparison.json).
No SAMIR-triggered desktop damage, panic or whole-system hang was observed.

### DOS and dialogs/alerts

| DOS area | Observed result |
| --- | --- |
| Basic shell | Prompt, VER, TYPE of ordinary files, lowercase commands and backspace editing worked. |
| Directories | Nested MD/CD, CD .., CD without an operand and RD of empty directories worked; RD refused a nonempty directory and CD refused its removed path. |
| Single-file operations | Ordinary COPY, same-spelling self-copy refusal, single REN and DEL worked; destructive exceptions are K01-K04. |
| Output redirection | > created files, >> appended, and DIR/TYPE output passed through redirected stdout. |
| Batches | %1/%2 substitution, @ECHO OFF, IF EXIST/NOT EXIST, CALL returning to its caller, GOTO and SHIFT worked. Wildcard FOR is K11. |
| Programs | SAMIR.COM ran from COMMAND and returned to it. stdin/stdout binding reached SAMIR, with the multiline defect K13. |
| Pipes/devices | ECHO FIRST \| ECHO SECOND printed second and returned. TYPE PRN gave a finite error. TYPE CON's recovery defect is K12. No external filter binary was available on these input disks. |
| Environment/clock | SET PATH=APPS and SET listing worked. DATE/TIME reported the guest clock; invalid date/time inputs were rejected. The ordinary PATH and VOL commands returned Bad command or file name. |
| Bad input | Missing operands, missing text file, absent directory, unknown command and missing redirection target gave finite errors and returned the prompt. |

No GUI modal dialog or text-bearing button was reached through the supplied
tenants' menus, including NOTES Open/Save/About. Database and DOS errors were
text at the prompt; their inappropriate messages are identified above. No
confirmation preceded the destructive COPY/DEL cases. Disabled Finder menu
items remained disabled, and their known missing features are not re-filed.
The intentional film saving dialog, pie chart and 570- figure were not present
in the images exercised here. Their appearance/behavior is untested.

## Ranked gap list: ten things an era user would feel most

This list combines new findings and freshly observed known omissions as
product priorities. It does not file the known omissions again. Historical
behavioral expectations in this ranking come from my Mac/DOS memory unless
a reference is identified in the findings.

1. **Useful desktop applications and document Open/Save.** NOTES accepted text and enabled New/Open/Save with no document or chooser; HELLO New also did nothing (K07, shots 09/12/69).
2. **Safe copying and backups.** An absolute spelling of the same file destroyed most of an 8 KiB file, and a directory destination became a 114-byte file (K01/K02, stopped FAT and extracted bytes).
3. **Trustworthy bulk deletion and recovery.** DEL selected the wrong directory, silently handled only sixteen matches, and DEL *.* asked no confirmation (K03/K04/K15, shots 171-173).
4. **Interactive database creation and editing.** CREATE, BROWSE, EDIT and bare APPEND produced errors instead of the documented editors (K08, shot 128).
5. **Accurate database views and state.** An accepted filter included BAL=-42, and a truly deleted row appeared unmarked (K05/K06, shots 138/175).
6. **Everyday Finder file and clipboard tools.** The tested File/Edit menus disabled Duplicate, Find, Make Alias, Cut, Copy and Paste; NOTES' enabled versions also did nothing (shots 109/111, K07).
7. **Readable, navigable file listings and scalable file scripts.** DIR ignored paths/patterns, labeled a subdirectory as root, and FOR did not expand *.TXT (K10/K11, shots 143/155).
8. **Database summaries, structure inspection, export and program files.** SUM, COUNT, DISPLAY STRUCTURE, COPY TO and DO UPDATE all failed in the reached database session (K08, shots 127/132).
9. **A reliable console escape and unattended command input.** Ctrl-C/Ctrl-Z became letters during TYPE CON; SAMIR rejected a three-line redirected command file (K12/K13, shots 158/165).
10. **Discoverable help and system/file operations.** The tested Finder Help items and Special's Empty Trash/Restart/Shut Down remained disabled; SAMIR HELP/ASSIST returned unrecognized-command errors (shots 110/112/128).

## Method, identity, replay and cleanup

I read all four REPORT.md/TRIAGE.md pairs and their QMP drivers in the repo.
F01-F18, G01-G11, H01-H07 and J01-J05 were the exclusion baseline. No repo
edit, issue-tracker mutation, make invocation, build, commit or remote write
was performed. The user's read-only brief overrides the repository's normal
issue/commit/push workflow. Only this working directory was written.

The brief identifies the desktop build as commit 8b1bf6d; tested byte identity
comes from [image manifest](evidence/image-manifest.json) and each session JSON:

| Original private copy | Bytes | SHA-256 |
| --- | ---: | --- |
| flair_tenants_interactive | 229376 | f3412f052d85c94379f64ffeedb1744bca0f51852b64ccfdb54 |
| flair_data | 1474560 | 84390fdb93bfe532d7e927a0bd83d918ec6f00c71c7bf5c368d56e09e2f4c4e4 |
| samir_list | 1474560 | 6744b68f2a3cc7349778d7ad2aa4beaa662441e752124e4cd4bfaada18dca0b0 |
| tracer_boot | 229376 | e4684585c88d9c3cac3faf55ed56b2d1e0e11dd2b395d7a6c24d345d61263200 |

Copied historical reference documents under evidence/references/ are ASCII
transcriptions of the local source documents; the originals were untouched.

Only the interactive desktop and DOS boot images were booted. The bounded
flair_tenants copy was inventoried but not used. Data fixtures add ordinary
FAT files/directories with mtools, documented under evidence/. They are not
mutant kernels. A prepared nested-service-name fixture was not booted and is
not used as findings evidence.

QEMU used `-cpu 486 -m 16 -display none`, first/second IDE images, QMP PS/2
input and guest-framebuffer screendumps. X display :0 was never captured or
given input. One owned emulator ran at a time; other people's QEMU/Bochs
processes were untouched. All owned instances received QMP quit and were
waited for, with status 0: [cleanup](evidence/cleanup.jsonl).

[audit_driver.py](audit_driver.py) reuses pass 4's driver with the global
other-QEMU refusal removed, keeping its own single-instance guard.
[text_driver.py](text_driver.py) additionally supplies typed text, uses a
20 ms QEMU key hold and rejects monitor diagnostics. Run a driver in a
separate writable copy, with images/, shots/ and evidence/, and send JSON:

```text
["start","replay","images/pristine-dos.img","images/dos-repro-fixture.img"]
["type","copy self.bin a:\\self.bin"]
["shot","copy-result"]
["quit"]
```

Coordinates are guest pixels. `drag` takes x/y/delta-x/delta-y. `move` homes
the pointer, so use relative `rel` while a gesture is held. App menu title
coordinates change with the owner; NOTES File is near (18,28), Finder File
near (38,28). The [action journal](evidence/actions.jsonl) records exact inputs,
UTC timestamps, session and serial byte offsets. Session JSON files record
PIDs, launch arguments, times and before/after image hashes. Raw serial logs,
[screenshot index](evidence/screenshot-index.json), [serial index](evidence/serial-index.txt)
and [final verification](evidence/final-verification.json) are under evidence/.

Every report-cited screenshot was opened and checked. Filename intentions
are not findings: early obscured-window clicks, rejected JSON, the initially
rejected text-key syntax, and a pre-launch same-file fixture error are recorded
in [harness notes](evidence/harness-notes.txt). In particular, 116-120 contain no
successful SAMIR input; 147 shows a small copy that survived; 180-181 did not
close the inactive built-ins; the dedicated six order sessions supersede them.
Screenshots 115 and 191 retained the source in an early release frame and are
not used to prove completed visual removal. A fresh three-second wait, pointer
move and two-second wait produced the correct one-item view in 193; later
clicks and reopening confirmed it. Raw traces and stopped disks confirm the
earlier staging. No persistent stale-source defect was established.

The two new fixes held in the exercised routes. The major new risks are DOS
COPY/DEL data loss and incorrect database view/state behavior. Endurance and
text-mode desktop restoration held up. All emulators and audit drivers are
stopped; the report and evidence remain here.
