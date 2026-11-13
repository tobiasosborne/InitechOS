# Read-only reboot triage: 2026-11-13

All **417 active issues** from the supplied export were classified exactly once: **404 open + 13 in progress**. The 926 closed issues were used only as evidence. Exported titles, numeric priorities and issue types are preserved verbatim in [triage.jsonl](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/triage/triage.jsonl). No issue status or repository file was changed; no `bd`, build, test, emulator or subagent was run.

The scope authority is [the reboot README](/home/tobiasosborne/Projects/InitechOS-legacy/docs/audits/2026-11-13-reboot/README.md), read after [report C and its Appendix A](/home/tobiasosborne/Projects/InitechOS-legacy/docs/audits/2026-11-13-reboot/report-C-product-state.md). Report C is navigation/evidence, **not** authority to carry its earlier GM-1 proposal, 86Box sign-off, full Base suite, kernel overlays or headroom programme into GM.

## Interpretation and evidence limits

- **GM** means an actual present-surface defect, the explicitly required frame/compiler/install completion, final documentation, or necessary release qualification. Where a command is inert, removing it satisfies GM; full historical implementation is not implied.
- **R2** means the requested feature/parity implementation is outside scope. Disabled/inactive placeholders are not permission to ship dead commands: remove their rows, key-strip entries and claims as part of `tdnl.12` (Finder), `mpim` (Word), and the explicit 123 branch-cleanup tickets. Required feature omissions must be named and frozen in `y29t4`/`1x6x2.10` for Base. A safe named refusal is acceptable there; silently accepted inert state is not.
- **OBSOLETE-FACTORY** applies to a ticket whose deliverable was the retired gate/fixture/mutant/certificate/process machinery. Useful test scenarios and independent captures remain worth migrating; their retirement does not assert that test coverage is unnecessary.
- **OBSOLETE-STALE** requires delivery evidence, supersession, or named concrete children. `duplicate_of` names the surviving record, including closed records when they record the delivered fix. Titles and original priority do not determine severity.
- **VERIFY** records have a specific reproduction/independent-reference check below. Their count is not zero unresolved defects, and source presence or old green gates are not a claim that the reboot image runs.
- Severity is the worst justified unresolved effect within the bounded GM action. Existing bytes stranded or silently replaced are `data-loss`; a clean refusal, extra paging pause or stale read is not automatically data loss. Only one crash/hang is source-supported here; the SAMIR triple fault is a cap-disabled probe and remains VERIFY.
- This is triage of existing issues, not a complete newly filed reboot backlog. The new chunked loader, per-component build/interface moves, single reproducible product image, exact release media layout and clean install/reboot qualification need explicit new-tree acceptance even where no clean legacy issue exists. No new tracker records were created.

## Counts by disposition and component

| Component | GM | R2 | OBSOLETE-FACTORY | OBSOLETE-STALE | VERIFY | Total |
|---|---:|---:|---:|---:|---:|---:|
| boot | 1 | 2 | 1 | 1 | 1 | 6 |
| kernel | 0 | 2 | 4 | 1 | 1 | 8 |
| dos | 17 | 41 | 0 | 8 | 0 | 66 |
| flair | 9 | 13 | 0 | 7 | 1 | 30 |
| flair-desktop | 11 | 11 | 0 | 6 | 0 | 28 |
| sdk | 1 | 2 | 0 | 0 | 0 | 3 |
| apps-calc | 2 | 0 | 0 | 3 | 0 | 5 |
| apps-calcda | 1 | 1 | 0 | 0 | 0 | 2 |
| apps-cpanels | 1 | 0 | 0 | 0 | 0 | 1 |
| apps-itext | 1 | 1 | 0 | 0 | 0 | 2 |
| apps-notepad | 1 | 1 | 0 | 0 | 0 | 2 |
| apps-paint | 0 | 2 | 0 | 0 | 0 | 2 |
| apps-fonts | 2 | 1 | 0 | 0 | 0 | 3 |
| apps-i123 | 15 | 6 | 0 | 1 | 0 | 22 |
| apps-iword | 14 | 35 | 0 | 1 | 2 | 52 |
| apps-samir | 26 | 20 | 0 | 24 | 10 | 80 |
| tps | 5 | 7 | 0 | 4 | 0 | 16 |
| factory | 0 | 0 | 49 | 2 | 0 | 51 |
| goldens | 1 | 16 | 0 | 2 | 1 | 20 |
| pubs | 10 | 1 | 0 | 2 | 1 | 14 |
| release | 1 | 1 | 0 | 0 | 0 | 2 |
| none | 0 | 1 | 0 | 1 | 0 | 2 |
| **Total** | 119 | 164 | 54 | 63 | 17 | **417** |

GM severity totals: **data-loss: 7**, **crash-hang: 1**, **wrong-behaviour: 67**, **missing-feature: 18**, **fidelity: 13**, **docs: 13**.

## Important source checks and corrected carryover

| Issue(s) | Read-only evidence | Decision consequence |
|---|---|---|
| rp8d | `os/flair/tbxgate.c:1747`, `tbx_fc_open`: source opens, then destination AH=3Ch before byte-copy loop. | GM/data-loss even though an old title only says “may”. |
| u6x8 | `os/milton/command.c`, `copy_transfer`: non-append path calls `dos_creat(dest)` before writing. | Preserve an existing target on failure; a period mint is not needed to justify avoiding data loss. |
| vtl9 | `os/apps/tbx/tbx_save.h`, `tbx_file_save_feed`: delete old path, then rename temporary, returning error with no rollback. | Shared GM/data-loss affects document-save consumers. |
| hnhy | `os/apps/calc/calc.c:307` accepts CR and LF; `begin_save` and FILECOPY completion both call `notice` on errors. `22bd3482`, `qxft`. | Stale, despite report C listing the old Return/failure findings. Footer clipping j89c remains real. |
| tdnl.57/.62/.72/.79/.88 | `0f33a169` implements alerts, original-name Put Away metadata, disabled demos and release tracking; `2f7b9a73` extends Put Away. Export notes say only clips owed. | Stale implementation tickets; new GM menu cleanup remains tdnl.12. |
| z1f5 | `os/milton/kmain.c:2950`: track-end `flair_live_erase_menu_panel`; named NO_RESTORE mutant. `b3hl` records the fix. | Stale panel bug; optional GDI facade coupling lives in R2 rmsr. |
| 9wx9 | `os/milton/fat12.c:1806` wraps for directory growth, but create/write allocation at 2943 and extension at 3287 still start above prev without wrap. | Partial fix; retain GM allocator issue, do not close all three paths on the directory fix alone. |
| s872 | `samir_main.c:526` now uses `conin_command`; `pal_milton.c:782` has cap-dependent raw editor, cap256 accepts255; overflow returns negative and REPL exits. | Original 128-byte diagnosis is stale, boundary/recovery behavior still merits GM. |
| 39zi / hgmx | `33fdb5c9`, closed 0spyf/fzsvo notes, WL-0195 record bottom-row prompt/native-page changes and steady-cursor exact Bochs boot. | Stale historical prompt/cursor symptoms. |
| 9uqp / ebwsw | `pal_milton.c:670` paints native BIOS pages without ANSI; closed 0spyf explicitly leaves SET STATUS pending on ebwsw. | Desktop BEL/terminal path VERIFY; current SET STATUS fidelity remains GM. |
| ra46 / 9dyu | `f7ce8ee0`, `62557893`, `ecfd4e0e`; `proc.c` owns disk DO/library lifetime and SET PROCEDURE. Closed 104j3. | Old missing nested DO/procedure library tickets are stale. |
| lo66 / pdj1 / hikn | `4937c97a`, `f2809281`, `9a295e47`: FIXED, HISTORY/DOHISTORY, UNIQUE creation. | Delivered initial features; retain .11/.13 and VERIFY .15 residuals. |
| ipu5.8.1 / miu6 | `set.c:489` stores relation text; TALK state is inspected in status but not consumed by execution. `proc.c:1079` explicitly accepts ON KEY unmodeled. | Do not retire the entire bundle: accepted inert RELATION/TALK and ON KEY need behavior or named refusal. |
| nbpj | `pal_milton.c:1222` still stores year modulo100; `fn_date` returns ctx_today. | Live century transport still differs from fixed CTOD historical-year semantics. |
| jc5s / 6mhb | `88787355`, closed bkmv notes and WL-0194: one blank => all comparisons false; both blank => all true; CTOD rollover; index blank-last unchanged. | Stale “blank compares low” and invalid-date-blank proposals. |
| rgd | `seed/lexer.c:298-372` relocates mid-string to pool start; closed 2hg9 and 99p1 record fix/tests. | Stale wrap defect; do not demand unbounded string storage for minimal self-host. |
| ur7k | `a6664cfd` plus `32225f6a`: real FDISK display/create/activate/delete and mounted-drive fixes. | Original first-pickup ticket is stale. |
| v872 / 4iq / ls4 | v872 notes report TP5.5 corpus landed; M7/6m52 delivered resident compiler; `selfhost`/`ddc` still deliberate failure stubs and host assembly/link remain. | Close stale acquisition/port/umbrella claims without calling native K2==K3 or DDC delivered. |
| j0x4o / z8lca | `ce7a0a7a` fixed HEADINGS/transcripts; export notes leave only gs-dir recapture duplicated on j0x4o. | Consolidate final requirements/capture work on j0x4o. |
| ugpsk | Volume One `entries/fn/os.tex` and `version.tex` already name the exact Initech literals; source matches. | Only ADR vocabulary bookkeeping remains: retire it rather than duplicate final manual work. |
| All cap/COLD/overlay/process records | Reboot README decisions 3–8 explicitly retire rulings, kernel overlays, size rounds, lane certificates and sentinel. | Technical source is retained; historical capacity/process tickets do not become GM prerequisites. |

No emulator was launched to convert these inspections into run-time acceptance. In particular, a delivered source fix can still regress on the new image; that is covered by the new interface smoke/release qualification rather than recreating old clip/certificate tickets.

## Full GM list

**119 existing issues**; grouped by component, then severity in order data-loss → crash-hang → wrong-behaviour → missing-feature → fidelity → docs. Within a severity: original priority, then ID. Some entries permit removal/refusal instead of implementation; read the reason, not the legacy title as a new scope commitment.

### boot (1)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| missing-feature | `initech-r2lqd` | DEC-03a minimum-RAM factory: explicit emulator memory profiles (QEMU -m 8M, Bochs megs: 8) in harness/emu + Makefile, boundary negatives (5 MiB and 7167 KiB refuse, 7168 KiB boots), QEMU + Bochs legs; period run owed to initech-x0i (86Box) | Qualify the one image at explicit 8MiB QEMU/Bochs RAM and reject insufficient RAM cleanly; drop its 86Box/factory obligations. |

### dos (17)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| data-loss | `initech-r7gf` | Execute complete batch files or reject an oversized script before execution | Batch loader executes a silently truncated 4096-byte prefix; reject oversize before any commands can mutate files. |
| data-loss | `initech-u6x8` | COPY onto a full disk leaves a 0-byte partial target; real DOS 3.3 behaviour unverified (delete the partial target?) -- mint the ground truth first | copy_transfer calls dos_creat before writing; failure can destroy an existing target. Preserve old bytes regardless of period partial-new-file policy. |
| data-loss | `initech-yg5r` | INT 26h on A: still enabled for a FAT16 A: volume (kmain.c ~706 pre-existing binding lacks the FAT-type guard DEC-15a.2 claims for every drive) | INT 26h allows raw writes to FAT16 on A: while other drives refuse; apply the same guard to protect read-only filesystems. |
| crash-hang | `initech-drm4v` | int21 CON: output between the two reads of an extended 00,scan pair can consume the scan byte as Ctrl-P/Ctrl-S flow control (pre-existing; now reachable via Alt-Q/Alt-R after initech-wuxo0) | Pending extended scan 13h can be consumed as Ctrl-S by output polling; preserve the key pair to avoid stuck input/output. |
| wrong-behaviour | `initech-g8x3` | Apply DOS hidden and system attribute filtering in handle searches | AH=4Eh ignores hidden/system search masks, exposing wrong files to DIR and wildcard consumers. |
| wrong-behaviour | `initech-hgeq` | Resolve qualified executable suffixes and preserve meaningful EXEC failures | Qualified executable lookup omits EXE/BAT suffixes and flattens load errors into Bad command; fix the shipped launch path. |
| wrong-behaviour | `initech-ijom` | Transfer batch control on a plain invocation and reserve return for CALL | Plain BAT invocation incorrectly returns like CALL, so commands meant to be unreachable can execute. |
| wrong-behaviour | `initech-p00t` | Complete FIND file operands, quoted strings, validation and result status | FIND ignores supplied files, mishandles quoted needles and reports success on no match; fix or refuse those operand forms. |
| wrong-behaviour | `initech-shkp` | Honor SORT starting columns and reject unsupported options | SORT silently ignores /+n and unknown switches; honor supported switches or explicitly refuse unsupported ones. |
| wrong-behaviour | `initech-sosa` | Preserve the last completed command template after Ctrl-C | Ctrl-C overwrites saved command-template bytes while retaining its old count; F3 recalls cancelled/garbled input. |
| wrong-behaviour | `initech-wel7` | Expose the child exit status after COMMAND /C | COMMAND /C restores the caller's old ERRORLEVEL, hiding the actual child result from batch control flow. |
| wrong-behaviour | `initech-xfg8` | Use one correct environment-expansion path for prompt and batch input | Existing %NAME% expansion differs between prompt/batch and preserves unset variables literally; define one correct bounded contract. |
| wrong-behaviour | `initech-07yn` | Show and clear the MORE indicator at interactive page waits | MORE blocks for page input without a visible --More-- prompt, making a normal pause appear hung. |
| wrong-behaviour | `initech-96gj` | Expand PROMPT $V using the native DOS version contract | PROMPT accepts $V but renders a literal V; supply the native version or reject that escape honestly. |
| wrong-behaviour | `initech-9wx9` | FAT12 allocator: free-cluster search hints from prev+1, can falsely report NO_SPACE under fragmentation | Directory growth now wraps, but file create/extend still search only above prev+1 and falsely report full under fragmentation. |
| wrong-behaviour | `initech-zdiq` | Flagship COMSPEC reads A:\COMMAND.COM while the file lives on C: (command_repl single-floppy seed) | COMSPEC names A:\COMMAND.COM while flagship stores it on C:; seed the real installed command-processor path. |
| missing-feature | `initech-hdyz` | Ship a real COMMAND.COM file on the tracer boot volume so FORMAT /S and SYS can transfer it (today the tracer kernel has COMMAND.COM built in) -- coordinate with initech-13ig's transient-overlay EXEC guard | Install/system transfer needs the real command processor file; carry this requirement onto the single release image, beyond tracer fixtures. |

### flair (9)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| data-loss | `initech-rp8d` | Kernel FILECOPY dialog may truncate the destination before copying (CALC.EXE saves through it; TABLES.TXT on a full disk) | tbx_fc_open calls AH=3Ch on the destination before successful copy; full disk/read/write failure destroys prior TABLES.TXT. |
| data-loss | `initech-vtl9` | tbx_file_save: no rollback if the final rename fails after the delete (the complete replacement survives only under NAME.$$$) -- decide whether to retry/rename-back or document the EDLIN precedent in the user manuals | Save deletes the old name then may fail final rename, stranding replacement under NAME.$$$; restore a safe commit/recovery contract. |
| wrong-behaviour | `initech-j2ts` | Activate an already-running accessory and report slot capacity clearly | Selecting an already running accessory produces a generic launch error; activate it and explain genuine single-slot capacity. |
| wrong-behaviour | `initech-mox1` | Give Standard File filename fields caret and selection editing | Standard File's visible filename field lacks caret/selection editing; default-name replacement and modifier handling need a working bounded route. |
| wrong-behaviour | `initech-tbgk` | Toolbox/IWORD: detect or eliminate silent directory enumeration truncation | FILE_FIND silently reports success after capping enumeration, hiding files and lying about totals in Word; page or visibly report overflow. |
| missing-feature | `initech-ddyn` | Add cancellation to the real modal FILE COPY operation | Complete the long live FILE COPY operation with Cancel/Escape and safe staging; cancellation must preserve the previous destination. |
| fidelity | `initech-qg22` | All five DrawControl draw paths ignore contrlHilite==255 — disabled controls render pixel-identical to active ones | Scrollbar disabled drawing now exists; other present control draw paths still omit disabled appearance and misrepresent availability. |
| fidelity | `initech-ucch` | draw_check_box and draw_radio_button never read contrlHilite — no pressed (hilited) feedback during TrackControl | Existing checkbox/radio drawing ignores tracked highlight state; show press/drag-out/release feedback on the shipped controls. |
| fidelity | `initech-wrlb` | MenuKey route does not flash the menu title; a tenant menu choice is only un-highlighted by the shell's panel restore | Existing keyboard menu commands execute without highlighting their menu title; provide coherent command feedback. |

### flair-desktop (11)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| wrong-behaviour | `initech-tdnl.12` | R3.5 The real Finder menu bar: Apple/File/Edit/View/Special/Help, every item functional or period-grayed, data-driven dispatch | Finish the present-depth Finder command audit; remove unavailable File/Edit/Help rows rather than requiring full Mac parity. |
| wrong-behaviour | `initech-tdnl.41` | Dragging a multi-selection moves only the grabbed icon (group icon drag) | Dragging a highlighted group moves/trashes only the grabbed item; honor selection or restrict the gesture honestly. |
| wrong-behaviour | `initech-tdnl.77` | Icon labels of wide 8.3 names overlap their neighbours in the default grid | Legal wide 8.3 labels overlap neighbors and selection; fit, wrap or ellipsize them in the shipped icon grid. |
| wrong-behaviour | `initech-tdnl.80` | Scroll bars follow-ups: horizontal scrolling on emu, Page Up/Down/Home/End, reference-backed thumb and steps, 4-px content left rail, unify control.c with winscroll.h | Existing horizontal scroll/thumb/content geometry needs one coherent contract; keyboard-parity extras may be omitted. |
| wrong-behaviour | `initech-tdnl.83` | Icon labels are clamped into the viewport instead of clipped with their icon when scrolled or narrowed | Offscreen labels pin to the viewport edge and remain clickable; clip icon and label at their document position. |
| wrong-behaviour | `initech-xkr6` | Present and reveal Finder keyboard selection | Keyboard selection mutates without present/repaint and lacks horizontal reveal; selected files can be invisible. |
| wrong-behaviour | `initech-kkht` | K14: Give unlabeled mounted desktop volumes a readable fallback name | Unlabeled mounted disks show a blank label/selection target; supply a readable display-only drive fallback. |
| wrong-behaviour | `initech-syf0` | B84: Count allocated Trash space including directories | Trash space total omits allocated directory clusters and understates bytes; report allocated space or label logical bytes. |
| wrong-behaviour | `initech-tdnl.85` | Clicking the scroll thumb without moving changes the scroll value (16 -> 14) | A no-motion scroll-thumb click changes the retained scroll value; preserve the drag start offset. |
| missing-feature | `initech-p6st` | R3.4c-UI: Duplicate + Get Info + rename-in-place (tdnl.11 reslice 3/3) | Duplicate/Get Info/rename are existing exposed Finder operations with only unmerged WIP; implement bounded routes or remove them. |
| missing-feature | `initech-tdnl.106` | FLAIR as a command: boot to InitechDOS, type FLAIR (or let AUTOEXEC.BAT do it) to start the desktop, and Special > Exit to InitechDOS RETURNS to the prompt without a reboot -- the Windows 3.x / GEM-over-DOS flow the PRD names (operator question 2026-10-17: 'we can boot to dos, then start flair right?') | GM must boot to DOS, start FLAIR and return to DOS; archived flaircmd lane is unverified integration, not delivered behavior. |

### sdk (1)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| docs | `initech-9dbgc` | spec/initechmz_extended.h: state the two parser placement rules mz.c enforces (e_lfarlc >= 0x38 even with zero relocs; relocation table must end within the header paragraphs) so an emitter working from the locked header alone cannot produce a refused image (Muse review note 4 on Lane R) | Document existing high-MZ relocation-table placement constraints so a resident compiler/emitter can produce loadable binaries. |

### apps-calc (2)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| wrong-behaviour | `initech-j89c` | Keep the film ledger total within its reachable content area | Film scene shrinks the ledger below its hard-coded total; make the required footer reachable in the live frame. |
| missing-feature | `initech-wlu` | Reproduce the entire reference frame as a live arrangement (M5 gate) | Complete TODO_FRAME ledger data and live measured ledger/chart/FILE COPY arrangement; staged tableau alone does not satisfy GM. |

### apps-calcda (1)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| fidelity | `initech-m6v0` | Render Calculator held-key feedback through release delivery | Shipped calculator keys lack held/released feedback; deliver release state and paint the pressed key coherently. |

### apps-cpanels (1)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| docs | `initech-tgt0` | Make Date and Time edit-snapshot behavior clear | Explain the Date/Time value as an editable snapshot and what OK commits; do not imply the system clock stopped. |

### apps-itext (1)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| wrong-behaviour | `initech-ulqb` | Use current text-window geometry and repaint every scroll change | Editor uses stale fixed geometry and loses repaint on scroll, hiding or misdrawing existing document text. |

### apps-notepad (1)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| wrong-behaviour | `initech-yhqb` | Make Scrapbook capacity notices fit the alert contract | Scrapbook capacity strings exceed both alert byte and pixel limits, so the required error notice never appears. |

### apps-fonts (2)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| fidelity | `initech-tdnl.69` | Charcoal 12 strike and the cloverleaf command-key glyph (D3-3 row 9; Sys 8 look) | Finish the film-facing system-font/command-glyph substitution against recovered Platinum art; do not expand into a full font platform. |
| fidelity | `initech-6grv` | FLAIR fonts: spec/assets/geneva9.h does NOT match real Geneva 9 (slashed zero, footed one) -- found by the Calculator DA lane, which authored its own strike; re-author the shared Geneva 9 strike against the Mac OS 8 NFNT / captures and re-key every gate that paints Geneva 9 text (ledger, Finder labels, InitechText) | Shared Geneva9 glyphs are visibly unlike the recovered font and affect the required ledger, Finder labels and editor. |

### apps-i123 (15)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| data-loss | `initech-uyi1` | I123: /Range Name Labels and Name Table write without a protection check | Name Labels/Name Table bypass cell protection and overwrite protected worksheet data; enforce the current protection contract. |
| wrong-behaviour | `initech-0c0t` | I123 B10: Complete inline editing and F4 reference-form cycling | Existing EDIT mode drops cursor movement and appends unconditionally; make its advertised bounded inline editor work, leaving F4 parity optional. |
| wrong-behaviour | `initech-li7r` | I123 B04: Resume Finder power actions after an asynchronous worksheet Save | Successful asynchronous worksheet Save cancels instead of resuming pending Finder shutdown/restart; retain the continuation safely. |
| wrong-behaviour | `initech-s5c8` | I123 B15: Connect the worksheet vertical scroll bar to the retained view | The native worksheet scrollbar is present but not connected to retained sheet/view state; connect it or remove that inert control. |
| wrong-behaviour | `initech-sd7r` | I123 B05: Return prompts and ordinary error notices to their proper caller | Go To Escape returns to the wrong caller and Return cannot dismiss ordinary notices; repair current prompt/error lifecycle. |
| wrong-behaviour | `initech-umts` | I123 B14: Complete the remaining File branch and worksheet file context | Existing @CELL filename returns empty despite a known file; prune unsupported File leaves and retain working Save/Retrieve. |
| wrong-behaviour | `initech-vdgd` | B15: Explain worksheet storage exhaustion without blaming valid input | Sheet capacity exhaustion is misreported as input EDIT and subsequent typing compounds it; report the limit while retaining the pending cell. |
| wrong-behaviour | `initech-3cl2` | I123 B19: Explain invalid 8.3 names and no-space saves accurately | Save input/error handling hides bad 8.3 names and no-space/short-write causes; show actionable failures without mutation on invalid input. |
| wrong-behaviour | `initech-i4x7` | I123 B20: Keep complete selected menu descriptions readable | Current menu descriptions truncate at normal/resized widths; provide a readable continuation without shortening resource text. |
| wrong-behaviour | `initech-q3oh` | I123 B18: Keep moved worksheet column-span metadata consistent with Lotus files | Existing Move serializes stale column-span metadata; align mutation bookkeeping with independently saved Lotus files, without claiming observed data loss. |
| missing-feature | `initech-1z3o` | I123 P4 /Data (Fill/Table/Sort/Query/Dist/Matrix/Regression/Parse) + /Print + macros + /Graph text-mode charts | Remove unimplemented /Data, /Print, /Graph and macro command promises from GM menus; full subsystem implementations are R2. |
| missing-feature | `initech-bnps` | I123 B09: Complete the remaining Worksheet branch and its Undo workflow | Unsupported Worksheet/Undo entries remain reachable; remove them for current-depth GM instead of requiring all Lotus structural editing. |
| missing-feature | `initech-k9st` | Initech 123 /File Save Backup (.BAK) is 'Command not implemented' -- buildable now on TBX_FILE_RENAME (0x0089) | Visible Save Backup reports Command not implemented; remove the option or implement safe bounded backup rotation before GM. |
| missing-feature | `initech-rl2g` | I123 B13: Complete the remaining Range menu branch | Enabled /Range leaves reach unsupported; GM must remove those leaves or implement a bounded route. Full Range parity remains R2. |
| missing-feature | `initech-suht` | I123 B17: Provide the native Add-In branch lifecycle | Enabled Add-In lifecycle commands all reach unsupported; remove that branch for GM. Native extension loading itself is R2. |

### apps-iword (14)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| data-loss | `initech-gj7z` | IWORD: honor Original Document Backup before replacing a saved version | Original Document Backup=Yes is persisted but ignored on Save, destroying the promised previous recoverable version. |
| wrong-behaviour | `initech-a092` | IWORD Block: connect paragraph extension and code-aware Search | Existing Block/Search/paragraph actions lose their route when a block is active; preserve anchor/cancel state or remove unsupported actions. |
| wrong-behaviour | `initech-aucx` | IWORD: keep status and modal prompts reachable after window resize | Resize can hide Word's only save/error/cancel prompts; adapt geometry or enforce a truthful minimum usable size. |
| wrong-behaviour | `initech-f4xa` | IWORD: consume the selected entry and status measurement units | Setup persists selected measurement units but status/input always use inches; apply the choice or remove the inert setting. |
| wrong-behaviour | `initech-hiai` | IWORD List Files: implement typed, default and Other Directory paths | List Files draws typed/default/Other Directory hints but ignores them and always opens root; honor supported paths or remove false hints. |
| wrong-behaviour | `initech-hift` | IWORD Look: restore cancellation, text scrolling and adjacent-document navigation | Current Look cannot scroll plain text or follow advertised cancel/adjacent-file routes; retain a real read-only viewport and truthful controls. |
| wrong-behaviour | `initech-mxv5` | IWORD: share word boundaries for Ctrl-arrow movement and Delete Word | Ctrl-arrow/Ctrl-Delete silently use one-unit operations instead of current word commands; distinguish modifiers or explicitly refuse unsupported chords. |
| wrong-behaviour | `initech-o3py` | IWORD Search: translate Ctrl-M from the actual FLAIR key event | Existing Search Ctrl-M handler is unreachable for actual FLAIR events and inserts m instead of the soft-return token. |
| wrong-behaviour | `initech-q7fu` | IWORD: implement inline Center and Flush Right codes and their entry commands | Imported recognized inline Center/Flush-Right codes render as zero-width fallback; fix supported code layout, keeping new entry commands optional. |
| wrong-behaviour | `initech-qv7t` | FLAIR/IWORD: deliver Finder power requests to Word Exit and expose the save decision | Finder quit recognizes only Quit while Word exposes Exit, so shutdown cannot reach Word's save decision; route by command identity. |
| wrong-behaviour | `initech-ri4c` | IWORD: distinguish zero margins from invalid numeric input | Margin parser conflates valid zero with invalid input, so an accepted zero cannot be applied; report parse success separately. |
| wrong-behaviour | `initech-mpim` | IWORD: show unavailable Font and Format choices consistently | Use real command availability across Font/Format/Setup/List Files/key strip; remove unsupported choices for GM instead of leaving selectable or grayed dead commands. |
| missing-feature | `initech-5nnt` | IWORD Block: save the selected stream as an independent WP5 file | Visible Save/F10 has no active-block route; provide a safe selected-stream save or remove the misleading contextual command. |
| missing-feature | `initech-ej9b` | IWORD: connect document hit testing and scrollbars to its viewport | Existing native document window offers ineffective point/scroll interaction; implement bounded hit testing or remove inert scrollbar affordances. |

### apps-samir (26)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| wrong-behaviour | `initech-ipu5.8.1` | InitechBase defects found by the manuals lane (WL-0133 Frictions 1-12): wrong catalogue message for an unknown SET option ('7 File already exists.'), SET INDEX / SET RELATION / SET CONSOLE / SET TALK accepted but inert, ON ESCAPE / ON KEY never fire, DO ... WITH a character constant not passed intact, REPLACE into a memo field stores nothing and empties the memo, LIST OFF ignores its field list, status bar says 'Enter a dBASE III PLUS command.' | Residual SET RELATION/TALK store accepted intent without runtime consumers; implement bounded effects or refuse them. Keep already fixed errors/DO/memo subitems closed. |
| wrong-behaviour | `initech-1x6x2.13` | SET UNIQUE: duplicate-key APPEND/REPLACE into an open unique index must be omitted from the index (real probe); ndx_insert_key still inserts; composite and numeric keys unminted | UNIQUE creation landed, but incremental APPEND/REPLACE still inserts duplicate keys into an already unique index. |
| wrong-behaviour | `initech-1x6x2.17` | SAMIR STR(): scientific fallback, range validation and precision differ from real III+; captures in corpus/fns/numfmt.prg, numfmtwide.prg + fns-mint (PENDING), numround PENDING | STR range/precision/scientific fallback differs on independently captured inputs; correct the existing function within the frozen subset. |
| wrong-behaviour | `initech-4zpa` | SAMIR DISPLAY with two or three STATUS/DISPLAY areas pauses once more than real dBASE III+ (extra pause after area 2) -- paging boundary | DISPLAY over two/three areas asks an extra page pause; correct the existing output paging boundary. |
| wrong-behaviour | `initech-51yc` | SAMIR: preserve typed precondition and malformed-index errors at command boundaries | Command boundaries collapse malformed-index/no-table causes into wrong catalogue errors; preserve typed failures. |
| wrong-behaviour | `initech-a7rwc` | SAMIR: CLEAR ALL / CLEAR FIELDS / CLEAR TYPEAHEAD not implemented (36); at-clear-c DC proves CLEAR ALL closes databases and releases memvars | CLEAR ALL must close databases/release variables in existing programs; implement that route or give a named omission, without adding forms. |
| wrong-behaviour | `initech-aql0` | SAMIR: run programs within the advertised byte limit regardless of short-line count | FLOW_MAX_LINES remains 512 despite the advertised 8191-byte PRG envelope; reject consistently or execute all supported short lines. |
| wrong-behaviour | `initech-byc5` | B19: Reconcile numeric overflow values with the independent III+ result | Division by zero still raises XBEE_NUM_OVERFLOW rather than the independent III+ overflow value, altering program control flow. |
| wrong-behaviour | `initech-fiwr9` | SAMIR: 1E20 numeric literal raises syntax error; @ r,c SAY coordinates do not use the III+ low-16-bit narrowing | Existing numeric literal parser refuses 1E20 and SAY range handling differs from III+; fix or explicitly bound these shipped inputs. |
| wrong-behaviour | `initech-gmt0` | SAMIR: set both BOF and EOF for physically empty work areas | wa_open/wa_refresh still set BOF false for physically empty tables; enforce empty-table BOF/EOF invariants in existing programs. |
| wrong-behaviour | `initech-jkhg` | SAMIR: an untrapped program error shows the III+ 'Called from - <drive>:<prog>.prg' trace and the Cancel/Ignore/Suspend prompt; today SAMIR prints the message and drops to the dot prompt | Unhandled program errors drop directly to dot prompt; supply a safe bounded recovery/trace or freeze the unsupported interactive choices explicitly. |
| wrong-behaviour | `initech-miu6` | SAMIR: ON KEY trap not carried out (accepted, never fires) | ON KEY is accepted but never fires; implement the supported trap or explicitly refuse it as a frozen omission. |
| wrong-behaviour | `initech-nbpj` | B18: Repair live-date century transport and blank-date display | PAL milton_today still reduces the live year modulo100, mapping 2026 to 1926; retain full live century separately from historical CTOD rules. |
| wrong-behaviour | `initech-o9l9` | SAMIR: remove failed new INDEX, SORT and COPY FILE outputs so retry works | Failed new INDEX/SORT/COPY leaves invalid output names blocking retry; clean only owned new outputs and preserve existing destinations. |
| wrong-behaviour | `initech-qvpw` | SAMIR: infer INDEX key geometry independently of the current record and enforce its limit | INDEX key type/width depends on current record and misses its length bound; empty/EOF creation can reject valid indexes or emit unsupported keys. |
| wrong-behaviour | `initech-s872` | SAMIR: accept complete 254-byte console commands with STATUS both off and on | Native command input removed the 128-byte cap, but cap256 accepts 255 and overflow exits REPL; enforce 254 with clean nonexecuting refusal. |
| wrong-behaviour | `initech-ywqf` | SAMIR: refresh the live memo reader after a successful replacement | Memo replacement persists correctly but old reader size snapshots return empty text in-session; refresh reader and preserve genuine read errors. |
| wrong-behaviour | `initech-1x6x2.11` | SET HISTORY residual divergences vs real III+: SET HISTORY ON while ON grows the ring (hx7 P2); IF/ELSE/DO WHILE/CASE lines not recorded when executed; NOTE lines, leading blanks and && comments not kept; DISPLAY HISTORY page pause; dot-prompt recording | HISTORY already ships but misses captured grow/recording/paging behavior; define and honor the supported session history contract. |
| wrong-behaviour | `initech-1x6x2.14` | SET MEMOWIDTH residuals: CR/LF inside memo text unminted; heading wider than width 0/1 wraps vertically in real III+; character-literal column after memo has no trailing padding; continuation columns misalign after q_putn flush on records wider than Q_LINEBUF | Existing MEMOWIDTH loses column alignment/padding on wrapped wide records; correct output within the supported memo/list subset. |
| wrong-behaviour | `initech-ws0b` | SET INDEX with no database in USE: real III+ prompts 'No database is in USE.' rather than raising a trappable error; SAMIR raises #52 with no golden behind the number | SET INDEX without an open database follows the wrong error/trap route; preserve the documented precondition response. |
| fidelity | `initech-ebwsw` | SAMIR: SET STATUS real layout and @/CLEAR interplay (guidance row 24, double-line separators, <C:>, Command during programs) | Existing SET STATUS puts guidance/separators/prompt on wrong rows and mishandles STATUS OFF; fix its current dot/program display. |
| fidelity | `initech-1x6x2.16` | SAMIR exponentiation precision: 10^49 scientific display differs from real III+ (.9999E+49 vs .1000E+50); captures in fns-mint/numdisplay + numfmt (PENDING) | Existing exponent/scientific output prints the wrong mantissa/exponent representation for 10^49; captured residual survived numfmt. |
| fidelity | `initech-7lgl` | SAMIR exponent width: real III+ prints 2.5^2 as 11-wide '       6.25'; SAMIR always prints 10+d+1 | Exponent expression pictures use the wrong captured display width (2.5^2); fix existing numeric output, not stored values. |
| fidelity | `initech-8jpa` | SAMIR LIST/DISPLAY: the deleted-record asterisk sits in column 0 while live rows start at column 1 (rows misaligned by one column after the Record# re-layout) | LIST/DISPLAY deleted marker shifts record/data columns relative to live rows; align both against the independent record layout. |
| docs | `initech-y29t4` | W5 ship: bug sweep (18 open), catalogue-message reachability (94 DRAFT), the mechanical completion gate, -j full vector + Bochs + 86Box + full pubs | Freeze a real dot-prompt/program feature-and-omission inventory; old W4/full-parity/86Box completion criteria are no longer the release contract. |
| docs | `initech-1x6x2.10` | Document CALL, LOAD, IMPORT, EXPORT as omissions (operator ruling 2026-11-01): w2-inventory classes, manuals, named refusal; IMPORT/EXPORT may reopen post-ship | Give CALL/LOAD/IMPORT/EXPORT explicit named refusal and document them in the frozen omission list, without claiming full III+ coverage. |

### tps (5)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| missing-feature | `initech-dh6` | Compile the compiler from inside the OS (K2 = K1(src)) | Compile the complete compiler source inside the OS, including native executable emission/assembly/link; host-assisted generation is insufficient. |
| missing-feature | `initech-foy` | DDC: diverse double-compilation against an independent seed | Execute diverse double-compilation with an independent seed and compare the resulting compiler; named ddc target currently fails deliberately. |
| missing-feature | `initech-kyh` | Turbo-Vision blue IDE: editor + compile + run | Provide resident editor, Compile and Run as the required minimal IDE; the blue IDE is absent. |
| missing-feature | `initech-ui3` | Two-stage fixpoint: assert K2 == K3 bit-identical | Build K2 and K3 natively from the same compiler source and establish byte-for-byte equality; hosted deterministic generation is insufficient. |
| fidelity | `initech-590o` | TP5.5 IDE chrome geometry for Turbo Initech blue IDE: menu bar cols, attributes, frames, dialog rects (measured) | Use recovered TP5.5 geometry/colors for the required minimal blue IDE; do not broaden the editor/compile/run scope. |

### goldens (1)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| fidelity | `initech-tdnl.7` | R2.5 Whole-desktop composite golden: locked scene screendump diffed pixel-exact vs sys8-derived golden | Review the whole live reference desktop against independent recovered art; per-widget correctness misses composition drift. |

### pubs (10)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| fidelity | `initech-lstk` | pubs: mono faces of initech-ops/xbase/borland still set the font's fi ligature (Ligatures=TeXOff only) | Family-specific monospaced faces still form fi ligatures in literal screen/messages; disable those ligatures in final manuals. |
| docs | `initech-ipu5` | [EPIC] Initech Publications: print-quality manuals for the OS and every app, mirrored on the originals in content/presentation/completeness, correctness enforced by local gates, updated in lockstep with the software (operator directive 2026-10-16) | Regenerate manuals once for the final shipped image, actual commands/omissions and installation; discard continuous lockstep/full boxed-set expansion. |
| docs | `initech-ipu5.9` | Initech 123 manuals: Reference + Setting Up/Tutorial, mirrored on Lotus 1-2-3 R2.2, slash-menu tree and @functions gated against the shipped 123.MNU/engine | Finish truthful manuals for the shipped bounded 123 surface; archived manuals-r step2 is unmerged, and full Lotus-volume parity is not required. |
| docs | `initech-bc2j` | pubs: InitechDOS Reference COPY entry (and User's Guide copy lessons) to the shipped DOS 3.3 COPY of lane doscmds | COPY manual still rejects now-supported wildcards/options and misstates timestamp behavior; align final books with working command. |
| docs | `initech-j0x4o` | DEC-03a publications lockstep: README system requirements, PRD Sec 5 reference, spec/hardware.json-cited manuals to 8 MB; fix the 4 MB-vs-5120 KiB drift; re-mint gs-dir.txt once (also owed by initech-z8lca); preserve CHKDSK captures (high window is MZ-only); run test-pubs | Correct supported RAM/video/input requirements and final installed-drive/size screenshots; release must not ship old 4MiB or false hardware claims. |
| docs | `initech-sx8y` | pubs: InitechDOS books to the DEC-18 real drive volumes of lane dosvolumes (0b429bc7) -- A:, C:, D: as mounted drives, drive selection, LASTDRIVE | Books still describe one drive although mounted A:/C:/D: and LASTDRIVE work; document the actual final image/install layout. |
| docs | `initech-tyvt` | Keep batch arguments beyond percent-9 and correct the SHIFT manual | SHIFT changes %0 but the manual says otherwise; correct that now. Retaining arguments past the documented nine is optional R2. |
| docs | `initech-4nxn` | Correct the TPS manual's shipped exit-to-DOS procedure | TPS manual falsely assumes no AUTOEXEC and asks for absent startup prompts; describe the actual exit-to-DOS path. |
| docs | `initech-7t9e` | IWORD manual: correct Esc instructions for leaving List Files | List Files exits with F7/F1, not the manual's Esc; correct the user instruction while retaining valid Look behavior. |
| docs | `initech-pian` | pubs: Reference Appendix A lists the FORMAT/SYS/DISKCOPY/DISKCOMP/FDISK messages that are not in the catalogue | DOS message appendix omits/misclassifies shipped disk-utility messages; document actual output without restoring catalogue gates. |

### release (1)

| Severity | ID | Title | GM reason |
|---|---|---|---|
| missing-feature | `initech-z62a.1` | A:SETUP install program (period-style installer) | Provide a reproducible clean-disk install/system/config transfer and reboot path; equivalent to SETUP is acceptable. |

## VERIFY list

These **17 issues** remain uncertain. Checks must use the final product image and the named independent references where indicated; reproducing an old mutant-only failure alone does not establish a production defect.

- **initech-i92wq — SAMIR: with the DO nesting cap disabled, RECINT triple-faults with ESP=0x00008A00 (below the declared 0x00070000 stack bound) -- the stack-bound guard does not hold; the unregistered test-samir-ctl-emu-mutant probe cannot meet its guard-HALT contract (found by lane t3/samir-dotprompt-cursor25-fidelity, 2026-11-06)** (apps-samir). Only the cap-disabled mutant triple-faults; production RECINT passes. Do not call this a shipped crash without reproduction. **Check:** Run production recursive programs to the supported 20-level/heap limits on QEMU/Bochs, inspect actual high-stack bounds/guard, then separately diagnose the cap-disabled probe.

- **initech-gtzi4 — FLAIR manuals: prove the 12 DRAFT entries in the emulator -- Apple menu Control Panels / Note Pad / Scrapbook, the 5 View modes (Small Icon, Name, Size, Kind, Date), upper-bar Quit no-ops -- captures + operation rows so GC-12 stops blocking release** (pubs). Twelve manual entries were written from source rather than demonstrated; actual shipped behavior needs confirmation. **Check:** Drive Control Panels/Note Pad/Scrapbook, five Finder View choices and upper Quit on the final image; keep working paths and remove unsupported claims/DRAFT entries before final manual build.

- **initech-1x6x2.15 — SET FIXED and numeric picture residuals vs real III+: ROUND ABS MAX MIN pictures, decimal*decimal width, round-half-even at 0 decimals (1234.5 -> 1234), -0.04 at 1 decimal prints no minus, >10 integer digits under FIXED** (apps-samir). SET FIXED residual ticket says to recheck after numfmt; that later formatter merge landed. **Check:** Compare ROUND/ABS/MAX/MIN pictures, decimal products, 1234.5 rounding, -0.04 and >10-digit FIXED output to real III+ numfmt captures on the release image.

- **initech-60hn — SAMIR: add a common printer output route for LIST and print settings** (apps-samir). Ticket predates SET PRINT/MARGIN and printer mirror landing; remaining LIST TO PRINT versus current sink behavior needs checking. **Check:** On the release image exercise LIST TO PRINT, PRINT ON/OFF, DEVICE and MARGIN with a captured sink and unavailable printer; identify inert accepted forms versus documented omissions.

- **initech-9uqp — SAMIR desktop launch: provide its terminal controls and consume BEL correctly** (apps-samir). Native BIOS-page rendering replaced the old ANSI-dependent path, but BEL/control behavior is not established from the old ticket. **Check:** Launch SAMIR from Finder without ANSI.SYS; exercise STATUS ON/OFF, SAY cursor controls and BEL. Verify no escape text/glyph leaks and correct return to FLAIR.

- **initech-u78k — K08 and K06 diagnostics: SAMIR workflows, shared dispatch and errors** (apps-samir). Old audit bundle says most verbs landed; RUN, generic errors and current program dispatch changed since then. **Check:** Replay USE/COUNT/SUM/DISPLAY STRUCTURE/COPY TO and disk DO on the release image; ensure omitted full-screen verbs refuse by name, and identify any live RUN/diagnostic defect.

- **initech-81ft.4 — FLAIR Platinum alert: mint sampled pink canon rows and icon truth** (goldens). Pink alert samples and icon provenance do not establish whether today's live alert is wrong. **Check:** Compare live Note/Caution/Stop alerts to matched Mac OS 8.1 captures; resolve tint cause and exact icon pixels before changing colors.

- **initech-z0od — ModalDialog's updateEvt -> DrawDialog case is unreachable — event.c WNE never synthesizes updateEvt; covered dialog regions never repaint in the modal loop** (flair). ModalDialog lacks synthesized updateEvt, but the live product uses additional routing/custom loops; shipped exposure is uncertain. **Check:** Open shipped modal alerts/choosers, cover and re-expose or drag them on QEMU/Bochs; verify damaged regions repaint while modal, using the actual release pump.

- **initech-ta2 — M1 boot robustness: LBA fallback, multi-bpp/mode, LinBytesPerScanLine, tri-emulator VBE** (boot). Old multi-part boot ticket predates current stage2/io_sys; supported QEMU/Bochs impact is not established. **Check:** Boot the same release image on QEMU and Bochs; inspect CHS/LBA fallback and VBE pitch at 640x480, 24/32bpp; separate unsupported mode/86Box requests.

- **initech-8h8m — SAMIR in the guest: a 500-line recursive procedure stops at about DO level 4 with #43 (PAL heap exhausted) where real III+ reaches #103 at 20 levels -- capacity gap, not a crash** (apps-samir). Reported four-level heap limit predates tagged extended-memory SAMIR and line-table changes. **Check:** Run the original 500-line recursive procedure on the high-memory release build to normal depth/heap refusal; distinguish clean documented exhaustion from a live capacity regression.

- **initech-wqhr — SAMIR: RETRY outside ON ERROR (re-runs the calling DO) is built but has no mint or test** (apps-samir). RETRY outside ON ERROR is implemented but has no independent behavior evidence. **Check:** Capture real III+ RETRY outside ON ERROR, then replay caller reexecution, nested calls and cancellation on QEMU/Bochs; check finite progress and no lost state.

- **initech-vz4e — IWORD: sentence detection treats '.' followed by a code (not a space) as not ending the sentence -- unminted; mint Ctrl-F4 Sentence at a code boundary** (apps-iword). Sentence/code-boundary behavior is unminted, so the reported selection mismatch is not yet a verified bug. **Check:** Mint Ctrl-F4 Sentence with punctuation followed by formatting code versus space; compare selected complete units, undo and saved bytes on the current image.

- **initech-naoz — IWORD: Tab/Indent after a word longer than the line -- slice-2 geometry model and slice-3 format reference disagree on the column (5000-session soak found it; layout follows the format reference); GAPS W-16 unminted: mint in wordperfect51-decomp, then align wrap** (apps-iword). Two internal geometry models disagree at long-word Tab/Indent; only independent period behavior can resolve which is wrong. **Check:** Mint a word wider than the line followed by Tab/Indent in WP5.1; compare cursor column, wrap, saved code stream and reopen on the release image.

- **initech-dzfq — SAMIR unminted: USE ... INDEX under SET DELETED ON (pointer placement) and SET INDEX TO (default extension + pointer); mint, then align** (apps-samir). Default extension/initial index pointer were fixed elsewhere; SET DELETED edge and SET INDEX TO remain unminted. **Check:** Mint USE INDEX with DELETED ON plus SET INDEX TO without extension, at BOF/EOF; compare selected record/order against current image and vfys/k7lc fixes.

- **initech-kd41 — SAMIR REPLACE / DELETE / RECALL NEXT n use their own scope loop in mutate.c and probably leave the pointer on n+1 (LIST/DISPLAY fixed by 5vmn); mint first** (apps-samir). NEXT n mutation pointer issue is only a hypothesis and differs from already fixed LIST/DISPLAY. **Check:** Mint REPLACE/DELETE/RECALL NEXT n for bounded hit/miss and EOF; compare final RECNO/BOF/EOF and exactly changed records with III+.

- **initech-cq0j — Oracle-verify LOCATE NEXT n EOF()/RECNO() semantics on a bounded (non-EOF) miss vs real dBASE III+** (apps-samir). LOCATE NEXT bounded-miss EOF/RECNO semantics are explicitly oracle-resolves; a wrong result is not yet established. **Check:** Capture bounded non-EOF LOCATE NEXT n miss in real III+, then compare FOUND/EOF/RECNO on the release image without scanning beyond the window.

- **initech-bcg.14 — P3 robustness + test-gap sweep (audit 2026-06-13)** (kernel). June audit bundle mixes fixed paths, unsupported ABI expectations and possible storage defects; blanket carryover would be misleading. **Check:** Reproduce each residual separately: malformed FAT capacity, creat-truncate/open handles, seek ABI, zero-row console, ATA IRQ disable; compare current code, preserve 9wx9 separately.

## Twenty most valuable R2 issues

Ranked by direct user value and shared enabling work; this is not a restoration of the old wave/lane ordering. Missing full parity commands must remain absent from GM, even where their eventual implementation is listed here.

| Rank | ID | Title | Value |
|---:|---|---|---|
| 1 | `initech-p3nn0` | W3 forms: @...GET, READ, PICTURE/FUNCTION templates, @...TO, @...CLEAR, CLEAR GETS (SAY already done) -- Opus, Sol review | Complete forms (GET/READ/picture/range input) once the GM dot/program boundary is stable. |
| 2 | `initech-qw9a` | SAMIR: full-screen BROWSE / EDIT / full-screen APPEND | Deliver the core full-screen BROWSE/EDIT/APPEND workflow; the largest direct database usability gain. |
| 3 | `initech-9x8cn` | W4.1 full-screen kit: key loop, menu bar + pulldowns, field editor (from READ), status/scoreboard line, screen save/restore -- Opus, Sol review | Build one reusable full-screen key/menu/field kit rather than duplicate app input machinery. |
| 4 | `initech-w0ej` | SAMIR: CREATE / MODIFY STRUCTURE (table DDL incl. CREATE FROM ... STRUCTURE EXTENDED) | Deliver structure creation/modification and independent DBF round trips. |
| 5 | `initech-oxte` | SAMIR: open and edit PRG files through MODIFY COMMAND | Edit PRG programs inside Base, after the shared full-screen kit. |
| 6 | `initech-jz16` | SAMIR: REPORT FORM / LABEL FORM runtime (+ MODIFY REPORT/LABEL designers later) | Add useful reports/labels runtime first, then designers over independently verified formats. |
| 7 | `initech-vda2` | SAMIR: ASSIST menu system (the III+ menu shell) | Bring ASSIST to non-programmers after real command consumers exist. |
| 8 | `initech-8q6j` | IWORD P4 print to PRN + headers/footers + footnotes/endnotes + columns + WP 5.1 tables | Make Word produce print output and then add pagination, headers, columns and tables. |
| 9 | `initech-b406` | I123 P5 Face B Mac-windowed view + InitechCalc (R4.6, 116% pie) convergence decision | Add the second spreadsheet face while keeping a single recalc/WK1 core. |
| 10 | `initech-xp4k` | IWORD P5 Face B Mac-windowed view (InitechText lineage) + spell-check/thesaurus data strategy | Add Word Face B and a realistic dictionary/thesaurus data strategy. |
| 11 | `initech-smp0` | R3.7 V2: multi-slot disk tenants (array of g_disk_tenant_slot) + multiple windows per tenant; DOS-heap-per-tenant explicitly deferred | Remove the one-disk-app residency restriction for genuine multi-app desktop use. |
| 12 | `initech-o5vm` | FLAIR Print Manager (device-independent draw-to-printer + PC LOAD LETTER) | Provide shared real print services instead of a separate printing implementation per app. |
| 13 | `initech-ncfu` | FLAIR TextEdit + List Manager: full TERec lineStarts + List scroll/multi-column/drag-select, as apps demand | Grow shared text/list services only against actual new consumers. |
| 14 | `initech-0lko` | FLAIR Resource Manager: resource->Toolbox-record instantiation + per-type decoders (NFNT/FOND/CURS/PAT) as apps demand | Instantiate resource templates and decode useful types as apps need them. |
| 15 | `initech-re30.7` | re30 verb-track 4: CopyBits + 8 src transfer modes + maskRgn + colorize + one offscreen GWorld | Expose CopyBits transfer modes/masks and GWorld for richer graphics apps. |
| 16 | `initech-lclc` | Build a bounded usable InitechPaint drawing surface | Turn the present Paint menu carrier into an actual bounded drawing application. |
| 17 | `initech-qutz` | Add Paint picture Open and Save through one verified native format | Give that drawing model one independently verified native file format and safe Open/Save. |
| 18 | `initech-vx9d` | Initech SideKick part (3): SK.COM -- the resident + pop-up menu bar (F1-F7 + Esc), Notepad (WordStar keys, 4000 chars), Calculator (dec/hex/bin, no paper tape in 1.5x), Calendar + Appointments, ASCII table, Dialer; screens and attributes per docs/research/SIDEKICK-period-grounding.md captures | Ship the actual SideKick accessory product over the already real TSR mechanics. |
| 19 | `initech-44m` | harness/emu/86box.c headless driver (flatpak + Qt offscreen) | Establish 86Box automation/support before taking on real period hardware. |
| 20 | `initech-k65i` | TP5.5 divergence: Integer is 16-bit and expression arithmetic is NOT widened (200*200=-25536; 32767+1 var=-32768; i shl 16=0; abs(-32768)=-32768) | Revisit and ground TP5.5 numeric dialect parity after the minimal native compiler fixpoint. |

## Knowledge worth keeping from obsolete issues

The brief quotations below preserve measured behavior or traps, **not** the old factory obligations. Supporting source/closed records are named when the obsolete ticket was corrected later. Broader emulator/toolchain/product memories are classified and quoted in full in [memories.md](/tmp/claude-1000/-home-tobiasosborne-Projects-InitechOS/49b9d057-8f0b-426e-924a-801f44e97515/scratchpad/triage/memories.md).

- **initech-p5u7:** “Device initech_disk2 is not removable”. QMP cannot change the medium of that fixed ATA attachment. A swap test needs a genuinely removable device/profile or a deliberate multi-image setup.
- **initech-9pzd:** “Bochs takes VBE 0101h since WL-0170”. Do not revive the old claim that Bochs always takes ENOMODE/mode13. This also invalidates the blanket no-VBE conclusion in archived memory117.
- **initech-hgmx:** “cursor in the other blink phase”. The exact 307200-pixel snapshot agreed while a later low-color RFB frame differed only at the underline. Synchronize phase/capture before calling this rendering drift.
- **initech-k8o5.11:** “NOT a cross-emulator byte-CRC”. Different framebuffer depths/resolutions need per-mode expectations; unequal raw CRCs across modes are not a rendering bug.
- **initech-v872:** “70 verified error numbers”. The real TP5.5 corpus/harness exists; reuse its provenance and measured IDE/error/runtime formatting rather than reacquiring a master.
- **initech-obk9:** “3 diverge only through Integer width”. The shared corpus has known TP16-bit versus FPC/Delphi32-bit differences; independent compiler agreement is meaningful only with an explicit consistent dialect.
- **initech-jc5s:** “exactly one blank operand -> .F. for every operator”. Corrected closed bkmv notes, not the stale low-blank title: two blank operands return true for every operator; NDX blank-last is a separate contract.
- **initech-6mhb:** “99/99/85 -> 06/23/1993 captured”. Closed bkmv captures establish III+ linear-month rollover; do not replace it with modern calendar validation or an inferred conventional rollover.
- **initech-rgd:** “false 'too long'”. Closed 2hg9 explains a mid-string pool-tail false rejection, now fixed. Token slices still have bounded reuse lifetime; callers must own values they retain.
- **initech-61rq:** “NUL discards, CON->serial, PRN->sink”. These are separate bound device semantics; OPEN-by-name exists and should not be mistaken for installable SYS-driver support.
- **initech-599k:** “two LF.TXT entries after an atomic save”. Fixed-slot host fixture planting plus rename can invent impossible duplicate directory entries. Replant by identity/name and model DOS missing-file error2.
- **initech-aaan:** “mkdir set_entry-EOC-fail leg uncovered”. Rollback scenarios should include entry-allocation/EOC failures and independently inspect disk bytes; an always-true assertion is not recovery evidence.
- **initech-ur7k:** “refuses its own volume”. The implemented FDISK (a6664cfd) has a real running-volume protection contract; preserve it when rehousing utilities.
- **initech-hnhy:** “the event path delivers LF”. The shared Return encoding is not always CR. Calc now accepts both and surfaces copy failures (qxft); do not reintroduce CR-only handling.
- **initech-tdnl.57:** “keep the original name across a Trash rename”. Put Away needs original identity/name/place, not just the staged Trash name. The merged kind6 companion metadata carries that information.
- **initech-ag4:** “only Turbo Pascal self-hosts”. Report C section7 reconciles the old all-Pascal premise: the OS is C. K2/K3 and the finale concern the compiler, not rebuilding the C kernel in Pascal.

## Memory recovery result

The final issue export dropped the memories. The preceding audit snapshot provided **134** exact memory values for classification: **29 STILL-TRUE-TECHNICAL**, **8 OPERATOR-PREFERENCE**, **97 RETIRED**. Full retained quotations and per-record validity limits are in memories.md. In particular, quota/model/provider/committee mandates are retired; old memory text is not a source of instructions for this task.

## Validation

Programmatic checks require exactly the 417 supplied active IDs, no duplicates or closed rows, verbatim titles/priorities/types, allowed dispositions/components/severities, GM-only severity, VERIFY-only check text, reason length ≤200 characters, and existing duplicate targets. Counts and all report lists are generated from the validated JSONL. Memory classification covers all 134 recovered keys once and quotes each retained value without truncation. The support scripts and recovered read-only evidence are stored only beside these reports in the authorized scratch directory.
