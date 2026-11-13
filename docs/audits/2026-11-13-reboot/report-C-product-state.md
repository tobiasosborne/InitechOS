# InitechOS product-state audit: distance to a production golden master

**Audit date:** 2026-11-13. **Source snapshot:** `b571f1e35cc386803ccd763f427c1cabe41866b9` (last commit dated 2026-11-06). **Method:** read-only source, specifications, Makefile recipes, git history, tracker JSONL, handoff, plans, packaging research, latest 40 numbered worklog shards, and October audit report/triage summaries. No builds, tests, emulator launches, tracker changes, commits, or repository writes. An existing untracked `.beads.gate.lock` was observed and left alone. Another process was building, so changing build artifacts were not treated as audit evidence.

## Executive assessment

This is a substantial working OS and application project, not an empty screenshot demo. There are real boot stages, FAT12/16 operations, DOS services, an event-driven desktop, disk-launched applications, database storage/interpreter code, and Pascal code generation. The factory includes meaningful differentials, negative tests, and mutation checks. That is much stronger evidence than the enthusiastic status language.

It is **not a ship-grade production golden master**. The highest-confidence remaining gaps are:

1. The original self-hosting finale is absent: `selfhost` and `ddc` deliberately fail, the blue IDE is not implemented, and compiler assembly/link work still crosses the host boundary.
2. The reference frame is only partly reconstructed: ledger spec-data contains literal `TODO_FRAME` values, layout is authored, and the staged tableau is not equivalent to the live file-copy operation.
3. InitechBase's core is real, but forms GET/READ and the full-screen application suite remain incomplete. The W4 gate infrastructure explicitly instantiates **no app targets**, and its ten app groups all say `not-started`.
4. The kernel has a reported **120 bytes of margin under a 169,984-byte resident ceiling** after the last merges. Extended memory improved SAMIR capacity; it did not remove the kernel constraint.
5. An installer and a genuine end-user distribution diskette set are not established. README's `A:SETUP` is a product promise with no implementation found.
6. 86Box validation remains unimplemented; real hardware is an explicit stretch. QEMU and Bochs work is meaningful but not a completed tri-emulator certificate.
7. Latest worklog acceptance expressly excludes a full current-tree certificate. A broad green suite would still not certify the missing milestones, because important pending cases and M8 are outside that suite.

**Consultant's diagnosis:** the remaining work is concentrated in a few architectural/product completions and release integration, not 417 equally weighted bug fixes. A scope freeze and an explicit release contract matter more than adding more lane/process infrastructure. Existing failures need fresh reproduction later; this audit does not certify current behavior.

## 1. Evidence and counting rules

Evidence levels used below:

- **Source-confirmed:** production source and its wiring/target exist; the relevant code path was inspected. This does not establish that it runs successfully today.
- **Gate-confirmed:** a real checking recipe and checker source exist. A target name or an inventory validator alone does not prove the end-user feature.
- **Reported:** a dated worklog/commit/issue records execution; that run was not repeated here and may concern a different tree or test subset.
- **Missing:** no implementation/acceptance path found in the scoped source; this is distinguished from an open tracker ticket alone.

Percentages estimate delivery against the stated product promise, including integration and acceptance, not lines of code or fraction of closed tickets. They are rough engineering judgments, generally uncertain by 10–20 percentage points; zero/near-zero values reflect demonstrably missing deliverables. They are **not schedule estimates** and should not be averaged across rows.

The export has **1,343 issues: 926 closed, 404 open, 13 in progress**. There are also **134 `_type: memory` records**, which are excluded. Active priority totals are **P0 4, P1 32, P2 213, P3 148, P4 20**. Counts below use `open + in_progress`; include epics; and retain stale/duplicate records rather than silently closing them.

**Count notation:** `P0/P1/P2/P3/P4`. Major domain rows use the complete, exclusive theme allocation in section 5 and Appendix A. Small-component and subcomponent rows use explicitly focused issue subsets, sometimes overlapping parent rows. A zero focused count is not a declaration that a component is complete; shared platform, save, and release issues still apply. These subsets are recorded in Appendix B so the counts can be reviewed.

### Milestone cross-check against the original PRD

| Milestone | Source-based status, distinct from tracker state |
|---|---|
| M0 / M0.5 | Foundations/tracer demonstrably implemented; M0 epic stale-open, M0.5 closed. SSIM now exists despite PRD saying absent. |
| M1 | Boot-to-text implemented with two emulator drivers; the literal three-emulator acceptance is incomplete. |
| M2 | Original small read/loader/shell bar substantially implemented; expanded DOS semantic parity remains active. |
| M3 | Region/blit/font/cursor/window-drag implementation and real property/replay gates exist; shared font fidelity still needs work. |
| M4 | Core managers and live menus exist; formal fidelity/86Box sign-off and later platform-bar work incomplete. |
| M5 | Partly implemented live frame; missing locked content/composition and final acceptance. |
| M6 | DBF/DBT/NDX/interpreter corpus is real; original GET/READ forms remain incomplete. IV/MDX explicitly deferred by later ADR. Expanded ship definition much larger. |
| M7 | Closed and supported by real seed/Pascal/FPC corpus, hosted deterministic self-source generation and assembly/link recipes. This does not prove resident K2==K3. |
| M8 | Resident compiler work exists; IDE/fixpoint/DDC accepted proofs absent. |

### 1.1 Platform, desktop and DOS

| Component | What demonstrably exists; estimate and basis | Remaining work to ship | Active issues by priority | Biggest risk |
|---|---|---|---|---|
| Boot chain | `os/boot/{mbr,stage2,io_sys}.asm`; `tools/mk_boot_system.sh` and Makefile boot-system pipeline; protected-mode/VBE/console paths; QEMU and Bochs boot checks. **90%**: implementation exists; hardware/mode/media coverage incomplete. | Explicit 8 MiB profile/boundary runs, complete target-video matrix, distribution boot/install path, 86Box. | 2/0/1/0/0 | A bootable emulator HDD fixture being mistaken for a bootable physical floppy product. |
| InitechDOS overall | `fat12.c`, ATA/block layer, `int21.c`, loader/MZ/arena/PSP/devices, shell/batch/config; real external tools. **80%** against the bounded DOS-3.3 personality, lower against unrestricted DOS compatibility. | Finish semantic defects and partial startup directives; validate writes/recovery, memory lifetimes and end-user boot. See service analysis below. | 1/2/23/50/3 | Broad API presence hides partial semantics; low kernel margin blocks fixes. |
| FAT/storage subset | FAT12/16 read/write, directories, attributes, timestamps, truncation, mounted volumes, ATA PIO and absolute-block operations. **85%**: extensive storage implementation and differential gates. | Fragmented allocation, attribute search filtering, cross-directory rename contract, failure rollback coverage, drive-specific INT 26h guard and media-swap validation. | 0/0/1/6/0 | Persistent data loss on failure paths; not merely cosmetic DOS mismatches. |
| INT 21h / loader subset | **77 distinct AH values** in the current machine-readable register all have dispatcher case labels; handle and FCB families, MCB allocation, vectors, EXEC, TSR, DPB, error API exist. InitechMZ and high-tagged SAMIR loading exist. **85% API delivery**, not 100% semantic conformance. | IOCTL/extended-error details, explicit env blocks, extended key flow-control race, return/error fidelity; re-prove stack/TSR/high-loader boundaries. Ring-3 DPL remains future work, not required isolation. | 0/0/2/10/0 | Listed/dispatched can be misread as fully compatible; flat-32 ABI is not DOS binary execution. |
| COMMAND.COM built-ins and batch subset | `command.c`, `batch.c`, transient command machinery; DIR/TYPE/CD/MD/RD/COPY/DEL/REN/SET/PATH/PROMPT/VER/CLS/ECHO/BREAK/DATE/TIME/EXIT/COMMAND; IF/FOR/GOTO/CALL/SHIFT/PAUSE/REM, redirection/pipes. **80%**. | Batch chaining vs CALL, oversized-script handling, args after `%9`, environment expansion, prompt ECHO/DATE/TIME behavior, secondary COMMAND lifecycle, COMSPEC and child result. | 0/0/10/5/0 | Accepted syntax can execute the wrong control flow or silently truncate work. |
| CONFIG.SYS / AUTOEXEC subset | Parsers, SYSINIT and startup batch processing exist; FILES/BREAK/LASTDRIVE have applied behavior. **65%** against advertised configuration. | Real installable `DEVICE=` drivers; decide BUFFERS/INSTALL/SHELL semantics and documentation; verify installed-disk baseline. | 0/0/1/1/0 | Parsing/logging a directive is mistaken for implementing it. |
| DOS external utilities | FORMAT/SYS/DISKCOPY/DISKCOMP in `diskutil*`; FDISK, CHKDSK, EDLIN, XCOPY/TREE, ATTRIB/LABEL and FIND/SORT/MORE have source and gate wiring. **70%** for the core promised suite; substantially lower for every period/DOS-5 utility ticket. | Reliable disk swaps and system transfer; DEBUG absent; missing utilities must be scoped or implemented; SETUP separate below. | 0/0/6/23/1 | Formatting/system-transfer tests mostly exercise ATA-backed fixtures, not the final install media. |
| Kernel overlays/capacity | ADR-0014, resident trampolines/imports, `FLAIR.OVL`, cold Finder bodies, preload/no-evict design and size/overlay tests. **70%**: mechanism landed but expansion unfinished. | Recover real growth budget (`xo63b`), validate matched SYS/OVL pairs and refusal paths; do not introduce eviction without fixing stored cold pointers. | 0/2/2/1/0 | Last reported margin 120 B; a tiny change can prevent shipping kernels from linking. |
| FLAIR toolbox/platform | Region engine, window/chrome, menus/controls/dialog/event managers; resource-fork store, Scrap, TextEdit/List, Standard File, font drawing, app contract, INT 81h gate. **65%** against ADR-0012's full platform bar. | Drawing verbs/CopyBits modes/GWorld, resource instantiation, fuller TE/List, Print Manager, multi-slot disk tenants, shared-service refinements. | 0/4/19/14/0 | Strong desktop primitives are being equated with a general application platform. |
| Finder/desktop | Real file/volume icons, folder windows/list views, launches, drag/file ops, Trash, alerts, persistent desktop DB, power/exit flow; implementation distributed through `finder_*`, `desktop*`, `kmain.c`. **75%** for bounded daily use; not a complete Mac OS 8 Finder. | Duplicate/Get Info/rename integration, disabled menu branches, multi-selection/persistence, full-disk recovery and error feedback, repeatable clips/composite golden, DOS-to-desktop round trip. | 1/5/16/4/0 | Usable happy path with incomplete recovery and misleading enabled/dead actions. |
| File Manager / “Drive A Files” | Finder's live folder-window model and FAT enumeration; not an additional separately implemented standalone app. **75%**, sharing Finder's implementation and acceptance. | Match frame's folder/content arrangement; viewport/selection and persistence defects; volume/count/free-space presentation. | 0/0/7/1/0 (focused subset of Finder) | Double-counting it as a separately completed product hides shared Finder gaps. |

### 1.2 Bundled apps and accessories

| Component | What demonstrably exists; estimate and basis | Remaining work to ship | Focused active issues P0/P1/P2/P3/P4 | Biggest risk |
|---|---|---|---|---|
| InitechCalc (`CALC.EXE`) | C disk tenant over the I123 engine; ledger and chart windows; trailing-minus formatter, 116% slices, editing, dirty/quit handling and real staged copy-save. **45%** of faithful frame app: several central data cells remain `TODO_FRAME`. | Recover/lock ledger contents, layout and pie labels; reachable total; input/Return behavior; safe copy failure handling. | 0/0/2/0/0 | A live but partly invented/placeholder ledger gets counted as M5. |
| CalcDA (`CALCULAT.EXE`) | Separate calculator core/view; decimal arithmetic and real-Mac vector tests, Apple-menu launch and power-query route. **85%** for the bounded calculator. | Clipboard Copy/Paste, release/held-key feedback and shared accessory-slot behavior. | 0/0/1/1/0 | Good arithmetic obscures unfinished application integration. |
| Control Panels (`CPANELS.EXE`) | Date/time seam, desktop patterns, mouse tracking/double-click, sound toggle/beep, memory display, persisted settings with corrupt fallback. **80%** for the five panels, not an extensible driver control-panel ecosystem. | Edit-snapshot semantics, verify user-visible routes in publications; target-hardware validation. | 0/0/0/1/0 | A panel or book entry can exist without the corresponding live effect being proved. |
| Note Pad (`NOTEPAD.EXE`) | Shared TextEdit surface; eight pages/dog-ear UI; Scrap exchange; persisted record format and restart/corruption gates. **80%** of bounded accessory. | Shared selection/caret/geometry usability; accessory activation/capacity; finish manual proof. | 0/0/3/0/0 shared text/accessory issues | Persistence works in selected traces, but platform limits and missing live evidence constrain confidence. |
| Scrapbook (`SCRAPBK.EXE`) | Paged text clip store, desk Scrap routing, persisted format. **75%** of explicitly TEXT-only accessory. | Long-clip mouse reading, capacity notices, shared activation and DRAFT manual proofs. | 0/0/2/1/0 | Text-only storage could be presented as full mixed-media Scrapbook parity. |
| InitechPaint (`PAINT.EXE`) | Disk tenant with exact eight menu titles, white canvas, Quit/close; almost every operation is deliberately grayed. **15% as a usable paint app; ~80% as the PRD's narrow menu-bar carrier**. | GM-1: faithful bar and honest reduced description. GM-2: drawing tools plus native picture Open/Save and failure paths. | 0/0/2/0/0 | An executable named Paint is reported as a painting application. |
| Fonts specimen / Font Manager | Bitmap text and synthesized styles/clipping/ellipsis; `os/apps/fonts/fonts.c` is a **test-volume specimen**, explicitly absent from flagship; Style items intentionally no-op. **90% specimen; ~60% broader font facility**. | Shared Geneva-9 corrections, Charcoal/command glyph and MacRoman repertoire; Face-B font selection/rendering work if promised. | 0/0/2/1/0 shared font issues | Test-only specimen confused with a shipped font manager app, or synthetic styles with complete font support. |
| InitechText / `itext` (`TEXT.EXE`) | SimpleText-like bounded editor; shared TE/Scrap, Standard File, scrolling, notices, one-level Undo and save helper. **80%** of bounded light editor. | Anchored Shift-selection, geometry/repaint details, caret feedback and save-commit edge cases. Preserve explicit 4,096-byte scope. | 0/0/2/2/0 shared text/save issues | Replacement succeeds in normal tests but commit/rename failure can strand the replacement. |
| FILE COPY dialog | Canonical caption, real rate-configured chunked copy, progress feedback and result event; distinct staged film modal also exists. **60%** as dependable shipping operation. | Cancellation; destination preservation/rollback; faithful film integration; close source/destination on all paths. | 0/0/3/0/0 | `tbx_fc_open` truncates the destination with AH=3Ch before successful copying. This remains source-visible. |
| TPS Report Generator (`TPSREP.COM`) | Separate FCB-based office-report utility, shipped data/ritual, output and validation/backup tests, own manual. **85%** bounded utility, pending current-tree verification. | Verify complete media workflow, error/backup behavior and docs on release image. | No separately focused active ticket found; shared DOS/pubs apply. | “TPS works” can refer to this report utility, not Turbo Initech's compiler/IDE. |
| SideKick | Period-grounding document/captures; kernel KEEP/InDOS/idle/IRQ/page-save mechanics; TSR and post-TSR MZ EXEC gates. **25%** of SideKick product. No `SK.COM` UI implementation found. | Build popup/accessory UI and persistent files; replay six accessories; exercise post-TSR apps. Old blocking reference to `h4bk` is stale: that fix is now closed. | 0/2/0/0/0 | Real TSR support is mistaken for a shipped resident accessory suite. |

### 1.3 Database, spreadsheet, word processor and compiler

| Component | What demonstrably exists; estimate and basis | Remaining work to ship | Active issues P0/P1/P2/P3/P4 | Biggest risk |
|---|---|---|---|---|
| InitechBase overall | DBF/DBT/NDX codecs; real-golden interpreter/functions/SET/commands; flow/procedures/memory variables; dot prompt; class-2 desktop suspend/return; `SAMIR.EXE` high-memory target and retained OVL. **60%** against the current III+ ship definition, much higher against the original small M6 corpus. | Complete W2–W5, omissions inventory and message reachability, docs, compatibility residuals and release validation. | 0/8/52/25/3 | “M6 complete” grades an older corpus, not the expanded III+ product. |
| W1 functions/SAY | Wave-1 parked slices landed after overlays; SAY plus ROW/COL/PROW/PCOL, NDX/LOG/SQRT work recorded. Overall function mint tree currently has 45 ACCEPTED and 13 PENDING markers, **not** a wave-1 completion denominator. **90% W1 planned tranche**. | Reconcile old transcendental tickets; formatting/precision and printer-position residuals cross into W2/W5. | 0/0/0/2/0 focused historical follow-ups | Obsolete task state or a handful of numerics is mistaken for complete function parity. |
| W2a/b/c SET and batch verbs | Many additional SET consumers/history/fixed/memowidth and batch operations exist. `set.c`, `query.c`, procedure/report modules and real mint cases show progress. **55%** across the intended tranche; not the stale October 25 count of 17/~47 SET options. | Remaining pure/UI-coupled SETs; batch verbs and output/error semantics; SET ECHO/DEBUG/STEP; explicit CALL/LOAD/IMPORT/EXPORT omissions. | 0/0/20/4/0 focused wave records | Accepting an option without an effect; equivalence inferred from too few probes. |
| W3 forms | SAY; slice 1 `@ TO`, `@ CLEAR`, bare `@`, CLEAR variants. Source explicitly says GETS/FIELDS have nothing to drop until GET/SET FIELDS exist. **30%** against forms including GET/READ/PICTURE/FUNCTION/RANGE. | Slices 2–4: GET, READ, editing/pictures/ranges, CLEAR GETS; preserve byte-exact screen evidence. | 0/1/1/1/0 | Spec inventory names future forms gates even though implementations remain absent. |
| W4 full-screen apps | Real mint tools, nine format comparators, locked ten-group regrade inventory and trace/check framework. `w4.mk`: “Factory only; no W4 app target instantiated.” **10% product delivery**, with considerably more acceptance infrastructure. | Full-screen kit; BROWSE/EDIT/APPEND; structure/program/report/label designers; QUERY/SCREEN/VIEW; ASSIST/HELP/CONFIG.DB; per-group L1/L2/L3. | 0/2/1/1/3 focused umbrella and app records | Experimental SAMIRX branch progress is counted as mainline delivery; oracle readiness counted as app readiness. |
| W5 completion/release | Dated ship criteria and parts of the gate infrastructure exist. **10% as a final closure activity**; most foundational work was done in other waves. | Bug sweep, all required messages reachable or omissions recorded, complete readiness inventory, current full vector/Bochs/86Box/pubs evidence. | 0/1/0/0/0 focused W5 umbrella (other Base bugs are in overall count) | No completed mechanical ship certificate; 94 manual catalogue entries explicitly say no command raises them. |
| Initech 123 | Native Face A tenant, sparse worksheet/range engine, formula parser/evaluator/recalc, real `.WK1` codec and save; real-golden differentials and GUI traces. **50%** against full R2.2-style suite. | Data/Print/macros/charts; large Worksheet/Range/File menu branches, inline editing/reference cycling, help/backup, scrollbar and Face B; corpus uncertainties. | 0/1/15/10/0 | Mature core and a few high-quality traces coexist with hundreds of unavailable menu leaves in October audits. |
| InitechWord | Native Face A, piece-table document/.WP5 codec, editing/wrap, formatting codes/Reveal Codes, blocks, search/List Files, save lifecycle; real-file and host properties. **40%** against full WP5.1-style suite. | Printing/page/document formatting, footnotes/columns/tables, macros/merge, styles, spell/thesaurus, graphics, remaining hotkeys/help, Face B. | 0/1/35/21/0 | Core editor reliability is mistaken for coverage of the promised full word processor. |
| Turbo Initech seed | C lexer/parser/typechecker/codegen and RTL; sufficient subset to build `tps.pas`; deterministic/corpus tests. **90%** of M7 seed scope. | Lexer's pending scratch-pool issue, target/toolchain reproducibility and release bootstrap packaging. Do not expand language merely to resemble TP7. | 0/0/0/0/1 focused defect | A passing small corpus is not a proof that it can bootstrap every needed compiler/tool stage. |
| Turbo Initech resident compiler | Pascal `os/tps/tps.pas` implements lexer/parser/types/code generation; seed-built `TPS.COM` and DOS file RTL; actual in-OS source-to-assembly recipe, plus hosted deterministic self-source assembly/link gate. **65%** of end-to-end resident compiler delivery, rather than 0%. | Finish on-OS assembly/link/native binary emission needed by the self-host loop; compiler-source scale/capacity, diagnostics and integer-width policy, resident package. | 0/1/6/0/1 focused records | Host NASM/linker completes present tests; native code emission inside the OS is not demonstrated end to end. |
| Turbo Initech IDE | Period TP5.5 research/golden/geometry work; no resident blue editor/compile/run IDE source found. **5%**. | Implement real editing, file dialogs, compile/error navigation and run/return, plus finale trace. | 0/0/1/0/1 | Source compiler and manual titles are misread as an integrated development environment. |
| K2 == K3 self-host fixpoint | Real M7 compiler corpus exists, but `selfhost` calls `stub_fail`. **0% of accepted fixed-point proof**. | Compile the compiler twice inside InitechDOS, include every assembler/link step, compare bit-identical compiler artifacts, preserve full transcripts and hashes, reproduce cleanly. | 0/0/0/0/4 | A deterministic compiler or compiling small programs is substituted for a compiler reproducing itself. |
| DDC | `ddc` calls `stub_fail`; no completed diverse-double-compilation path found. **0% accepted**. | Independent seed/toolchain path, documented trust roots and exact comparisons against the self-host result. | 0/0/0/0/1 | Two runs of the same trusted chain are mislabeled DDC. |

### 1.4 Publications, distribution and validation

| Component | What demonstrably exists; estimate and basis | Remaining work to ship | Active issues P0/P1/P2/P3/P4 | Biggest risk |
|---|---|---|---|---|
| Manuals (`pubs/`) | **18 manual `main.tex` projects, 48 main/cover/spine/strip source documents**; LuaLaTeX class/family styles, fonts lock, indices, screens, transcript/source lockstep/repro/lint gates. **65% of whole promised documentation set**. | I123 books not merged; DRAFT FLAIR entries; DEC-03a requirements and gs-dir recapture; stale command/path/error statements; final content/print audit. | 0/4/7/4/0 focused publications records | Polished PDFs and green lockstep checks can document a reduced or partly DRAFT product. |
| Packaging/physical edition | Detailed October 4 research/proposals, design system, manual/package inventory, suppliers/media research and open decisions. **15%** of manufactured physical edition. | Freeze SKUs/content/dielines/media capacity, resolve actual measurements/quotes, proof books/boxes/labels, procure/write/read-back media and assemble pilot. No manufactured pilot evidence found. | 0/0/1/1/0 focused physical records | Research/specification pages mistaken for production assets or an available stock of good media. |
| SETUP installer | README install procedure plus open `z62a.1`; **no installer source/target found**. **0–5%** (requirement exists). | Implement target selection/preparation/system copy/configuration, clear destructive-action prompts, interrupted/failure recovery, reboot verification. | 0/0/0/1/0 | README instructs a new user to run a nonexistent product path. |
| Distribution diskette images | Real `make image` creates `build/initech.img` from `FLAIRTENANTS_IMG` plus separate `flair_data.img`; fixtures and system-file minting exist. **30% of release distribution**, not 30% of boot development. | Versioned contents manifest, fix the boot-image media fit (2,944 sectors exceeds a 2,880-sector 1.44 MB floppy), partition/boot assumptions, disk-set creation, SETUP and clean-install/readback. | 0/0/1/3/0 focused media/install records | Data image size or “floppy” naming is mistaken for a complete BIOS-floppy distribution; dev path uses IDE drives. |
| QEMU/Bochs/86Box validation | QEMU driver and rich traces; real Bochs driver, boot legs and some exact pixel/input checks. 86Box driver file absent. **~65% QEMU/Bochs acceptance coverage; 0–10% 86Box delivery**. | Full current-tree run after process ownership allows it, minimum-RAM matrix, actual 86Box driver/period VGA replay/calibration and realistic media. Boot-only Bochs legs do not replay QEMU workflows. | 0/1/9/2/1 whole emulator-validation theme | “Tri-emulator” claims while only two drivers exist, and many second-emulator legs stop at boot. |
| Real hardware | Explicit S3 stretch; no completed hardware validation evidence found. **0% demonstrated**. | If physical hardware compatibility is sold: define supported machine/BIOS/controller/video/floppy matrix and actually boot/install/use it. Otherwise label emulator-only. | 0/0/0/0/1 focused stretch ticket | A physical floppy box creates an expectation of real-PC operation that the current accepted-shortfall contract does not meet. |

**Compiler evidence boundary:** Makefile `test-tps-gen-fpc` (around lines 18333–18370) runs the FPC-built Pascal compiler on its own source, compares two emitted assemblies, and uses host NASM/LD for both bare and DOS links. That is substantive M7 self-source coverage, not an empty stub. Separately, `test-tps-gen-os` (around lines 18499–18507) extracts `TPSOUT.S` from the guest and invokes host NASM/LD. No recipe inspected closes the native compiler-to-compiler K2/K3 chain; `selfhost`/`ddc` still fail deliberately.

## 2. DOS service and utility coverage: what “DOS 3.3” actually means

The PRD section 6.1 is a small personality (filesystem, loader, basic handle I/O and shell). The more demanding implementation list is **ADR-0003 Appendix A**, reflected in `spec/int21h_register.json`; ADR-0012 supplies the application-platform bar, not the DOS AH appendix. This distinction matters when reporting conformance.

A read-only expansion of every register range yields **77 distinct AH numbers, including the newer 34h InDOS entry**; every one has a case label in `int21.c`'s final dispatcher. This is strong evidence that the registered surface is present. It is **not 77 behaviorally certified services**: the FCB range includes reserved no-ops and other legacy selectors; subfunctions, pointers, drive handling, errors and device semantics need their own tests.

| Required family | Source assessment | Important remaining qualification |
|---|---|---|
| 00h, 4Ch/4Dh, 4Bh, 31h, 62h | Termination/result, flat/MZ EXEC, KEEP and PSP implemented. | Explicit env pointers and shell child status; post-TSR flat COM refusal differs from relocatable MZ; high-tag memory model is newly landed. |
| 01h–0Ch | Console/direct/buffered input, AUX and printer routines present. | Extended key/output flow-control race `drm4v`; AUX/PRN handle-read debt; absence of real device input cannot be inferred away from console success. |
| 0Eh/19h, 1Ah/2Fh, 25h/35h, 2Ah–2Dh, 30h, 32h/33h | Drive, DTA, vectors, dates/time, version, DPB and break state present. | Format/grammar/oracle and target-machine fidelity debt; AH=34h is still described as PM-ratified-pending-operator in locked data. |
| 36h, 39h–43h, 45h–47h, 4Eh/4Fh, 56h/57h, 5Bh | Free space, directories, handle files, dup, searches, rename/time/new-file present. | Search H/S filtering (`g8x3`), rename scope (`ycb3`), fragmentation (`9wx9`), rollback and FAT16 free-space behavior. |
| 44h, 59h | IOCTL and extended errors have handlers. | `jplu` documents wrong drive-vs-handle semantics in a subfunction; `vd9n` requests a file-exists classifier. Presence is not correctness. |
| 48h–4Ah | Low MCB and tagged high-owner allocation paths present. | Newly expanded high memory/stack guard; single high owner is deliberate, not multi-process isolation; RAM-boundary acceptance outstanding. |
| 0Fh–24h legacy FCB range | Concrete open/create/find/read/write/delete/rename/random-file routines, DPB/allocation selectors and reserved no-ops present. | File I/O/format limitations and independent golden choice still matter; this is native flat-32 software, not actual 16-bit binary compatibility. |

`command.c` has real built-in handlers and `batch.c` implements real control flow. The open umbrella `f9z4` already overlaps implemented VOL/BREAK and is not evidence that those words are missing. Conversely, accepting `CONFIG.SYS` entries is not enough: `sysinit.c:461` explicitly logs arbitrary DEVICE, INSTALL and BUFFERS as **`accepted(deferred)`**. `DEVICE=ANSI.SYS` toggles a resident interpreter flag; it does not load an installable SYS driver. FILES/BREAK/LASTDRIVE are applied. SHELL is parsed/reported; this audit did not establish a general configurable replacement-shell loader.

Utility-by-utility source findings:

- **FORMAT/SYS:** real core/program source under `diskutil*`, with system-transfer and refusal tests. Need final-medium installation rather than only fixture transfers. The tracer's missing physical COMMAND.COM is separately tracked.
- **CHKDSK:** real checker and executable wrapper, with filesystem/oracle checks. Keep its deliberate low-memory contract distinct from newly tagged SAMIR.
- **EDLIN:** real line editor/core/executable and gates. The still-open “EDLIN + DEBUG” record must be split conceptually: it is not proof that EDLIN is absent; DEBUG source was not found.
- **XCOPY/TREE:** real traversal/copy source and wrappers; drive/cwd/timestamp fixes in history.
- **FDISK:** real partition/create/activate/delete source and physical block-service tests; `ur7k` remains open despite its delivery commits. This is definite tracker drift.
- **ATTRIB/LABEL/FIND/SORT/MORE:** source exists; remaining operand/status/grammar differences are tracked.
- **Missing or separately promised expansion:** COMP, FC, REPLACE, APPEND, SUBST, ASSIGN, JOIN, BACKUP, RESTORE, RECOVER, MODE, KEYB, CHCP, NLSFUNC, PRINT, GRAPHICS, FASTOPEN, EXE2BIN, SELECT are open first-pickup tickets. A DOS-5 suite, XMS/HMA/UMB and networking/locking are distinct expansion epics, not prerequisites to claim a bounded DOS-3.3 personality.

The current boot master has a concrete media-fit problem: `tools/mk_boot_system.sh` creates **2,944 sectors = 1,507,328 bytes**, containing a 2,880-sector FAT12 partition after a 63-sector prefix plus one trailing sector. That is **32,768 bytes larger than a standard 1.44 MB floppy**. The separate data volume may fit 1.44 MB, but the boot image is not thereby a standard floppy master. An installer may use differently laid-out media; that final layout is not yet provided by the current flagship recipe.

## 3. M5: the live “Saving tables to disk” reference frame

**Estimate: about 45–55% of the actual reference-frame acceptance, with no current owner sign-off established.** The substrate is farther along than the reconstructed content. The September “M5 untouched” statement is obsolete; claiming M5 finished would be equally wrong.

What is real now:

- Disk-launched Calc paints two genuine windows, with region/damage/window management; it uses the common I123 model rather than a second fake spreadsheet engine.
- `film_scene.h` moves/resizes those real windows; `kmain.c:6498` loads CALC.EXE and composes the scene. The normal startup model also has a real Finder folder/volume surface and the Photoshop-canon menu band.
- Code explicitly preserves 40/35/18/14/9 pie values, trailing-minus formatting and the $112,276.87 footer literal. The pie geometry normalizes drawing to the sum while keeping the visibly nonsensical percentages.
- The normal Calc File > Save routes through a real copy with the canonical caption and `michael_bolton.conf` rate. Rate parsing and byte-writing are not just an animated bar.
- Hourglass/busy cursor and Platinum chrome code exist. New startup errors distinguish data-volume/APPS/CALC failures instead of an unexplained panic.

What is specifically missing or not yet accepted:

1. **Recovered data:** `spec/assets/calc_ledger.json` has `TODO_FRAME` for row/column count, column headings, the three known labels' amounts, remaining rows, pie labels and the `-570` cell address. `calc.c:104` can render literal `TODO_FRAME`. Known values are preserved honestly, but this is not the complete ledger.
2. **Recovered visual composition:** comments expressly call positions, two-column view and grayscale pie **authored product policy**, not recovered film art. `film_scene.h` uses two rectangle placements, not a measured full composition. Grid, font, chart, folder contents, z-order and menu/dialog placement need a common source and a live final review.
3. **Staged vs operational modal:** the film policy explicitly says Escape dismisses the staged Saving tableau and no file I/O is claimed there. That differs from the normal real FILE COPY request. The production scene needs an agreed live operation demonstrating the same arrangement without implying the staged modal has copied data.
4. **Faithful ledger interaction:** Return/commit and footer visibility issues remain (`hnhy`, `j89c`). Safe failed copy and cancellation (`rp8d`, `ddyn`) are product blockers even if the screenshot looks plausible.
5. **General multi-app arrangement:** the disk-tenant host remains essentially a single disk slot (`smp0`); built-in cooperative tenants co-reside and Calc can own two windows, but independently keeping several disk apps live is not established. The film build can compose a shell Photoshop band without a live Paint tenant. Decide which frame elements need independent apps and deliver that contract.
6. **Fidelity evidence:** SSIM is now a real tool, so old “SSIM stub” claims are stale. Its known frame selector currently lists desktop/grow-box/scroll-look/alert/finder/icons/controls; it is not a whole-film-frame sign-off. PRD deliberately makes SSIM advisory, not a hard threshold. `tdnl.7` whole-desktop composite and `wlu` M5 acceptance remain open; `c7u1` records owner feedback “requires work, but is getting there.” The final frame/clip acceptance must be explicit.
7. **Release exposure:** the dedicated film image must boot with the final companion data media. October audit's standalone panic was on an older tree; newer error handling is visible, but a successful current final-film boot was not verified here.

For GM-1, finishing M5 means the owner accepts one final **live, draggable, editable and safely saving** arrangement and clip against the original still, with provenance for every recovered/unknown detail. It does not require a full Photoshop clone.

## 4. Scope history and proposed release contracts

The current PRD is mutable: its “2026-06-08 / v0.3” header is not a clean historical baseline. The dates below distinguish an operator/record date from the first relevant commit when they differ. “Every major addition” here means major product, architecture or release streams visible in the specified documents/history, not each new command/bug fix.

| Date | Major addition/change beyond the small original boot + DOS + frame + database + self-host sketch | Evidence; GM relevance |
|---|---|---|
| 2026-06-08 baseline | Bootable DOS personality; Mac-like region/window desktop; Calc/File Manager/Paint-bar/FILE COPY frame; dBASE-like app; seed/resident Pascal + IDE/fixpoint/DDC; VibeMode/sound/real-iron marked stretches. | PRD M0–M8. Self-host/IDE/DDC and printed release must not be confused: first three are original goals; physical manufacture is later. |
| 2026-06-13 | DOS-3.3 feature-parity and kernel-hardening programmes, broader than PRD 6.1's tiny handle/shell core. | `bsy`, `bcg` creation dates. Bounded API/storage reliability essential; exhaustive later-DOS fidelity optional. |
| 2026-06-14 | Broad external-utility programme; future DOS-5 API/utilities, HMA/UMB/XMS/LOADHIGH, locking/networking; physical distribution/SETUP epic. | `8479`, `om2a`, `ppgt`, `ws3x`, `z62a` and `z62a.1` created. SETUP essential for an installable product; DOS-5 expansion optional. Planned XMS is **not** the high-memory implementation delivered in November. |
| 2026-06-20 operator / 06-21 plan commit | Canonical suite expanded to **Initech 123 and InitechWord**, plus native Doom and “Initech Mines,” and never-delete/era layering. | FLAIR comprehensive plan, `fe4d9b1e`, HANDOFF WL-0049/50. Suite optional only in an expressly scoped GM-1; full-suite GM-2 requires it. Games optional. |
| 2026-06-20 | InitechMZ flat-32 executable format, beyond initial “flat COM; MZ deferred.” | DEC-08a (`6dee9142`). Now load-bearing implementation, essential; does not add true 16-bit EXE execution. |
| 2026-06-21 policy; 2026-09-25 ratification | CPU floor moved from 386 aspiration to 486-class; higher memory/GUI capacity became explicit. | PRD section 5, ADR-0001 DEC-02 and HANDOFF. Support contract change, not new user functionality. |
| 2026-06-27 | Fourth North Star: credible early-1990s **application platform**, App Contract, cooperative co-residency, Phase 4.5 resources/Scrap/TE/List/common dialogs/printing. | Ratified ADR-0012/0013 (`1619d62d`). Basic subset needed by shipped apps essential; complete bar GM-2. Binary compatibility and real iron remain accepted shortfalls. |
| 2026-07-11 | Turbo compiler scope ratified as a minimal Pascal subset, with real FPC differentials; units/heap/enums/sets/reals/inline assembly deferred after fixpoint. | ADR-0007, TPS M7 plan. A scope restriction, not a reason to postpone original M8 behind TP-completeness. |
| 2026-08-17 | Separate graphical dBASE-Mac-shaped SAMIR frontend added to full-console product. | `e7qc` ratification/title. Optional GM-1; not needed to ship class-2 SAMIR. |
| 2026-08-23 | “Sellable Platinum desktop” remediation programme: Finder file ops, app platform delivery, fonts, accessories, stronger visual acceptance. | `tdnl` P0 epic; GUI-remediation plan. Essential reliable shipped desktop; every optional Finder/era affordance need not be GM-1. |
| 2026-09-25 directive / 09-26 plans | Canonical suite restated as **full-featured** R2.2/WP5.1 reimplementations, each with classic Face A and later Mac-windowed Face B over one core. | `68iw`, `fdxa`, plans committed `b347b7fa`/`3473d966`. Escalates the June names into major compatibility products. GM-2; explicit deferral required for GM-1. |
| 2026-10-03 | Hardware x87 for I123; Word dictionary companion-volume strategy and Face B gated on Font Manager. | DEC-01a, both plans (`edbfed62`). x87 support essential to current 123; complete dictionary/Face B optional in GM-1. |
| 2026-10-04 | Physical edition research, package/manual families/design system, original-manual acquisition and sourcing. | `docs/packaging/*`, `dffc0212`. Manufacturing optional for a digital GM; essential for an actual boxed SKU. Research prices were not reverified in this repo audit. |
| 2026-10-15–17 | Font specimen, SimpleText-like InitechText, Apple Calculator, Note Pad, Scrapbook and five Control Panels delivered/expanded. | `628e9c65`, `5e05d2f7`, `38f1e6fa`, `a7c3955d`, `91f8088c`. Discretionary additions; once included in GM-1, they require safety/docs, not automatic full historical parity. |
| 2026-10-16 | Comprehensive print-quality **manuals for OS and every app**, not merely developer docs; LuaLaTeX/repro/screens/lockstep pipeline. | `ipu5`, `11d87206`, `491795a2`. Correct essentials mandatory; all planned volumes/physical print production GM-2. |
| 2026-10-16 | Real FCB-based **TPS Report Generator** utility. | `fb9a16ef`, `8479.1`. Optional small bundled utility, independent of compiler TPS. |
| 2026-10-17 | **Initech SideKick** product directive; TSR engine and later actual popup accessory suite. | `6fdt`; October 20 period grounding; `vx9d` UI remains open. Optional GM-1, named-suite GM-2. |
| 2026-10-17 | Physical block-device service/FDISK, FORMAT/SYS and real mounted DOS volumes, significantly strengthening system administration beyond a baked boot fixture. | DEC-17/18 and October utility commits. Essential substrate for installable media; does not itself implement SETUP. |
| 2026-10-24 decision / 10-25 ratification | **Kernel overlays**, FLAIR.OVL cold code/data preload, to escape resident kernel ceiling; retirement of older arrival caps. | ADR-0014, `cdd1d248`/`e01f4530`; WL-0181/0189. Architectural survival work; essential now, not optional user scope. |
| 2026-10-24 ship definition / 10-25 plan | InitechBase expanded from small M6 acceptance corpus to every III+ 1.1 verb/function/SET delivered or explicit omission, reachable catalogue, W1–W5 including forms, designers, ASSIST/HELP. | InitechBase ship plan, `1x6x2`. A major product expansion, GM-2 unless owner retains this full definition for GM-1. |
| 2026-10-25–11-01 | SAMIR overlays/packaging B and resident shrink, then real forms slice 1 and more SET behavior. | WL-0185–0190. Delivered mechanisms retained; the remaining packaging/shrink programme was explicitly retired November 6. Do not re-add it to the critical path. |
| 2026-11-06 | **Tagged extended-memory InitechMZ**: 8 MiB floor, SAMIR at 6–8 MiB, native high-owner allocation/stack, 256 KiB module ceiling. W4 screens regraded to output files + structural traces + clips. | Ratified DEC-03a, `1e29e4e8`; W4 oracle `5ae79486`. Essential compatibility/support change; not XMS/EMS, paging or multiple high apps. W3 remains byte-exact. |
| 2026-10-24–11-06 process stream | Parallel certificates, registries, merge gate map, proof packs, reverts, sentinel/culprit workflow. | F1–F11, WL-0180/0190/0195; `0uphq` closed. Enabling process, not additional product completion. Remaining real correctness defects still need fixes. |

### Proposed GM-1: a small, honest, reproducible digital master

This is a **proposal**, not existing ratification. It preserves the original fidelity/self-host commitments and requires explicit deferral of later “full suite” commitments. It must not be marketed as satisfying all current expanded scope.

- A frozen version/manifest of boot, system and data/install media; target **486 + 8 MiB + specified supported VBE/ATA emulation profiles**. Rebuilds produce identical release bytes. QEMU/Bochs/86Box boot and core workflows are certified with no undeclared skips.
- Safe bounded DOS-3.3 personality and advertised core utilities. No silent acceptance of promised-but-inert behavior; document deliberate limits. Complete SETUP-to-clean-disk/reboot operation on supported media and correct start/exit/re-enter desktop behavior, or explicitly change the installation/startup contract before branding a release.
- M4/M5 accepted live frame, complete recovered ledger or explicitly agreed unknown detail, real safe Save/Copy, stable Finder basics and included small accessories. Paint may remain a clearly described frame-bar carrier; a full paint system is outside GM-1.
- InitechBase core database/interpreter as an expressly bounded, golden-backed product with a frozen omission list. For GM-1, missing full-screen designers, ASSIST, QUERY/SCREEN/VIEW and remaining advanced III+ parity are deferred. If the owner refuses to relax the October 24 InitechBase ship definition, **W2–W5 all become GM-1 blockers** instead.
- Complete the resident Turbo subset + blue IDE, **native K2==K3** and **DDC**, without host assembler/linker assistance in the claimed self-host loop. Later TP5.5 whole-product parity and language families explicitly deferred after fixpoint remain optional.
- Zero unresolved data-loss/crash/hang/install blockers on included surfaces, current full included-feature certificate, matched documentation and essential user/programmer/install manuals. All DRAFT material is removed from released promises or proved; unrelated future manuals are not a gate to this scoped release.
- Initech 123/Word as full products, SideKick UI, elaborate box/manual families, games, VibeMode, sound and real hardware are excluded or clearly labeled previews. An unvalidated hardware claim is not included merely because disk images are writable to a real floppy.

The reduced database contract also explicitly defers GET/READ forms named in the original M6 row; GM-1 therefore cannot be claimed as completion of every original PRD acceptance item.

The master must be suitable for duplication, but printed boxes/binders and a manufactured stock are separate acceptance from the software bytes.

### Proposed GM-2: the full currently expanded product

GM-1 plus: complete InitechBase W1–W5 with current omission rulings; full promised I123/IWord MUST features and both faces (or an explicitly amended scope); usable Paint; SideKick's actual popup suite; completed ADR-0012 drawing/resource/multi-app/printing bar; every included accessory and promised manual; production diskette set, versioned package artwork, verified duplicated media, printed proof copies and assembled pilot box/BOM/production instructions. Preserve the sanctioned absence of 16-bit binary compatibility, preemptive isolation, paging and real-iron support unless separately ratified. Doom/Initech Mines, VibeMode and other explicitly deferred games/eggs remain optional even for GM-2.

**A physical SKU claiming period-PC usability requires an added hardware acceptance contract and real installation evidence.** “Real iron is a stretch” can coexist with a novelty/emulator physical edition, but cannot substantiate a promise that the boxed OS runs on customers' unspecified period PCs.

## 5. Open-issue landscape

These are exclusive, manually reviewed primary themes for **all 417 active issues**. Priority vectors include epics and duplicated work. This is a navigation map, not a clean release backlog; exact issue allocations are in Appendix A.

| Primary theme | Active | P0 | P1 | P2 | P3 | P4 |
|---|---:|---:|---:|---:|---:|---:|---:|
| Boot and foundations | 3 | 2 | 0 | 1 | 0 | 0 |
| Bundled apps and reference frame | 19 | 0 | 0 | 15 | 4 | 0 |
| DOS kernel, shell and utilities | 79 | 1 | 2 | 23 | 50 | 3 |
| Emulator and hardware validation | 13 | 0 | 1 | 9 | 2 | 1 |
| FLAIR platform and shared UI | 37 | 0 | 4 | 19 | 14 | 0 |
| Factory, proof process and tracker hygiene | 24 | 0 | 3 | 10 | 11 | 0 |
| Finder and desktop | 26 | 1 | 5 | 16 | 4 | 0 |
| Initech 123 | 26 | 0 | 1 | 15 | 10 | 0 |
| InitechBase | 88 | 0 | 8 | 52 | 25 | 3 |
| InitechWord | 57 | 0 | 1 | 35 | 21 | 0 |
| Release, publications and physical edition | 18 | 0 | 4 | 8 | 6 | 0 |
| SideKick | 2 | 0 | 2 | 0 | 0 | 0 |
| Stretch, games and easter eggs | 5 | 0 | 0 | 1 | 0 | 4 |
| Turbo Initech and compiler finale | 20 | 0 | 1 | 9 | 1 | 9 |
| **Total** | **417** | **4** | **32** | **213** | **148** | **20** |

Further composition: **153 features, 135 tasks, 104 bugs, 23 epics, 2 chores**. Thus “417 bugs remain” is false. Many are optional feature families or stale umbrellas; conversely one M8 task can represent far more work than dozens of polish issues.

### 5.1 Staleness and dependency debt

Using `updated_at` date relative to 2026-11-13: **163 active records were last updated before 2026-10-14**, 100 before 2026-09-14, and 84 before 2026-08-15. Age alone does not imply invalidity: original M8 records are old precisely because the work remains undone. However, the stale bookkeeping does mechanically obscure priorities.

- All **four P0s are epics**: `njv` M0, `2rb` M1, `509` M2, and `tdnl` sellable-GUI remediation. The first three conflict with implemented foundations/boot/DOS and multiple completion claims. Their remaining child requirements should be reconciled and the release blockers promoted individually. `tdnl` still represents a real unfinished programme; it should not be called done merely because its earliest defects were fixed.
- `krfn` already asks for milestone reconciliation. Its M0.5 reference is itself stale: **`f8v` is closed**, as of October 24. **M7 `rnh` is closed**; **M6 `586` remains open**. The Handoff September assertion “M6 closed” therefore conflicts with the export; some of the semantic scope expansion explains why this cannot be solved by blindly closing the epic.
- In-progress flags do not mean a worker is running. Last handoff says nothing is running. `re30` was last updated June 21; `tdnl.11` August 25; `tdnl.12` October 3; `bf2` and `udr1` October 16. These flags should not be used as staffing/ETA evidence.
- Obvious delivered-but-open cases: `ur7k` FDISK (implementation/October 17 commit), portions of `udr1` EDLIN+DEBUG (EDLIN delivered, DEBUG absent), `509.10` startup baseline (implementation exists, final shipped-media evidence still owed), and `ipu5.13` policy proposal (lockstep machinery and policy are already in use).
- `lo66` asks to add SET FIXED when the current source and newer residual ticket `1x6x2.15` show it has progressed. `h0ss`, `o9fo` and other ledger tickets can remain valid as documentary closure even with small effort, so do not infer product absence from their titles.

The JSONL contains explicit dependency fields on **127 active issues**. There are **100 `blocks` edges from active issues**, of which **31 point to active issues**, affecting **27 active issues**. Missing edges and `parent-child` relationships make this a poor complete schedule. Stale M0/M1 dependencies can falsely block work already possible. `vx9d`'s description says SideKick is blocked on `h4bk`, but `h4bk` is now closed. Plan Sec 7 is a better starting critical path than an unfiltered export DAG.

### 5.2 Duplicates and overlapping scope worth reconciling

These are **candidate overlaps, not automatic duplicate closures**:

| Records | Why they overlap / needed disposition |
|---|---|
| `jkhg` and `tjh1p` | Both describe the III+ unhandled program error “Called from / Cancel, Ignore, Suspend” experience. Consolidate one implementation/acceptance owner, preserve both evidence sets. |
| `0tl`, `9t8c`, `p3nn0` | Original forms task, audit bounded-form request and ship-plan W3 umbrella. Scope them into completed SAY/slice 1 and unimplemented GET/READ/picture slices rather than count three separate form engines. |
| `qw9a`, `vda2`, `w0ej`, `jz16`, `oxte` vs `9x8cn`/`j7lld` | Old BROWSE/ASSIST/structure/report/editor records overlap the newer full-screen kit/apps programme; some runtime-vs-designer scopes differ. Preserve those distinctions, unify acceptance. |
| `hikn` and `1x6x2.13` | UNIQUE creation/toggle vs incremental duplicate-key behavior. Related, but not identical; the latter is residual correctness, not a reason to reimplement the former. |
| `lo66` and `1x6x2.15` | SET FIXED feature request vs newer numeric-picture residuals; mark delivered subset, retain residuals. |
| `j0x4o` and `z8lca` | Both include gs-dir.txt remint; two other z8lca failures have reported fixes. One recapture and one verification owner. |
| `ipu5.11` and `gtzi4` | FLAIR guide delivery vs 12 DRAFT entries' outstanding live proofs; guide source exists, but release proof is incomplete. |
| `jc5s` and closed `bkmv`/earlier date lanes | Old blank-date-low question contradicts newer real captures and shipped coercion decision. Reconcile the question and approved rule, do not relitigate by title alone. |
| `tdnl.62/.72/.88`, `qg22`, `ucch`, old GUI findings vs later alert/control fixes | Several old symptoms overlap reviewed later changes; source and target-specific retests decide closure. No claim that every old GUI defect remains reproducible. |

### 5.3 Process-about-process and real factory reliability

The **24-issue factory/proof/tracker theme** includes three P1s and many practical test defects, so it should not all be dismissed as bureaucracy. Open process/coordination records include `z17ji` (selector uses PM tree instead of proposed head), `krfn` (tracker cleanup), `jk8t` (handoff archive/restructure), `3gynk` (capacity reallocation), and `1x6x2.1/.2` (parallel producer safety and scratch serialization). Other members are missed captures, nondeterministic traces, stale fixtures and coverage holes.

The gate-map/proof-pack/revert/sentinel **programme `0uphq` is closed**, with F8/F9/F10/F11 source delivered. It should not be presented as another unfinished product epic, nor as evidence that user functionality has shipped. A selector bug can genuinely block trustworthy integration; another elaborate dashboard is not automatically on the GM critical path. The final instruction “run sentinel once on main” is outstanding in the handoff, not a certificate already obtained.

## 6. Critical path to GM-1

The order below includes dependencies and explicit parallel work; it is not a claim that every item must be done serially. It assumes the proposed bounded GM-1 is accepted. If the owner retains the full III+ ship definition, insert the W3 → kit → W4 apps → W2/W5 branch described afterward.

1. **Freeze the actual release contract and image inventory.** Decide what ships and which promises are formally deferred. Promote real release blockers out of P0 umbrellas. Pin minimum machine/video/disk profiles and final master contents; name one owner for M5, M8 and release-media acceptance.
2. **Recover kernel headroom before more kernel-growing merges.** `xo63b` is the latest handoff's first prerequisite. Inspect what Lane R already moved to COLD, obtain the remaining capacity with measured size/overlay proofs, preserve SYS/OVL matching and pointer lifetimes. The issue's “52 B” snapshot is stale; latest reported tree has 120 B, still critically tight. This is structural capacity management, not an excuse to revive retired SAMIR shrink rounds.
3. **Close data-preservation and control-flow blockers on included workflows.** The FILE COPY AH=3Ch pre-truncation (`rp8d`) remains visible; commit/rename failure (`vtl9`) and interrupted/failed save paths need a release policy and real recovery. Resolve crash/stack/key-flow risks such as `i92wq`/`drm4v` before they can invalidate included scenarios; determine which are only injected mutants and which affect clean production. Verify batch control, install-system-file transfer and error reporting. Priorities P2/P3 should not conceal a real data-loss severity.
4. **Complete the original resident compiler loop.** Preserve working M7 seed/TPS/FPC rails, provide binary emission or on-OS assembler/linker path, compile full compiler source natively, obtain K2 and K3, compare, and run a genuinely diverse independent path. IDE work and minimal TP5.5 grounding can proceed alongside this. Resolve integer-width/diagnostic policy without expanding the post-fixpoint language backlog. No host NASM/linker may be hidden inside the claimed finale.
5. **Complete M5 content and owner acceptance.** Acquire/lock missing frame data, align measured composition and fonts, keep the joke constants, integrate safe real copy with the live scene, and record the exact final master frame/clip. A full Paint app is not required for the narrow PRD carrier promise; unlabelled invented ledger cells are not acceptable.
6. **Finish the user journey and installable media.** SETUP, copy/system/config/boot files, reboot into the installed system; enter/exit/re-enter FLAIR (`tdnl.106`), run included apps, persist data, restart. The archived FLAIR-command lane is unverified integration work, not a ready-to-merge feature. Prove final disk contents fit and boot through the actual intended medium path.
7. **Complete third-emulator and minimum-memory evidence.** Build the absent 86Box leg and validate period video/media; set explicit 8 MiB QEMU/Bochs profiles, negative boundaries (`r2lqd`) and high-memory lifetimes. This work should begin early in parallel because it can invalidate boot assumptions; sign-off is a late gate. Real hardware remains optional unless the physical SKU claims it.
8. **Bring included manuals and installation instructions into exact lockstep.** `j0x4o`, remaining gs-dir recapture, `gtzi4`, file/save/drive/startup claims and essential compiler manuals. Prove or remove DRAFT release claims; do not force unfinished GM-2 books into GM-1's distribution.
9. **Obtain one current, reproducible final-tree certificate and immutable master.** After the other builder releases ownership, use isolated outputs and a full included-feature gate vector, explicit Bochs/86Box workflows, media install/reboot, M5 sign-off, M8 fixpoint/DDC and publications gates. Fix actual producer/selector defects; a serial vector is sufficient if parallelism is unreliable. Record skipped/excluded cases as exclusions, never green. Hash/tag the exact master and documentation, then verify copied/duplicated bytes.

If the full InitechBase ship definition is mandatory for GM-1, its added critical branch is **W3 slices 2–4 → W4.1 shared kit → widest W4 apps (likely designers/ASSIST/QUERY-SCREEN-VIEW) → per-app L1/L2/L3 → W5**. W2a/b and independent error/format fixes can run alongside; W2c follows UI consumers. DEC-03a runtime has already landed, so it is no longer a future design prerequisite. “Packaging B → resident shrink” is explicitly retired and must not reappear as a blocker. W5 then joins manuals/third-emulator/final certificate before the master.

## 7. Contradictions, stale claims and gate caveats

| Claim/document | Conflicting evidence | Audit interpretation |
|---|---|---|
| `os/README.md`: entire artifact Pascal, no C, dirs/READMEs only | Hundreds of real C production files; PRD/ADR-0002 explicitly C OS, Pascal compiler. | Completely obsolete overview. Do not use it for product status. |
| README: supplied install diskettes; `A:SETUP`; reboot prompt; type WIN | SETUP source/target not found; `z62a.1` open; real flagship directly enters desktop; `tdnl.106` DOS/FLAIR round trip unmerged. | End-user onboarding promise is ahead of implementation. WIN wording also conflicts with planned FLAIR command. |
| README: InitechBase `.MDX` indexes | `os/samir/fs/ndx.c`, specs and gates are `.NDX`; no MDX codec found. | Incorrect file-format claim. |
| README: serial pointing device recommended; VGA-compatible display | Hardware/input implementation is PS/2; GUI requires supported VBE/LFB and 640x480 guard, not arbitrary VGA/serial mouse. | Requirements too loose in one dimension and wrong in another. |
| README: installable character-device drivers | SYSINIT accepts arbitrary DEVICE as deferred; built-in device chain is real. | Built-in devices are not an installable driver loader. |
| README: unified fully conformant platform/full-screen database/IDE | W3/W4 outstanding, platform Print/verbs/co-residency incomplete; IDE absent; M8 stub. | In-universe marketing voice, not release evidence. |
| PRD section 5/15: MZ deferred | Same table appends a reconciliation that InitechMZ shipped; loader/MZ source and tests exist. | Self-contradictory historical text with correct newer caveat; read final amendment. |
| PRD risk table: compiler miscompiles OS; old finale task `ag4`: compile OS source/new bootable image | PRD architecture and section 7 say C OS is factory-built; only Turbo Pascal self-hosts. | Original wording leaks the abandoned all-Pascal architecture. Retarget M8 finale to compiler, not kernel. |
| PRD section 15: target CPU 386+ | Ratified section 5/ADR-0001 floor 486; 386 desirable/nonblocking. | Loose historical decision row; ship support claims must use ratified 486 floor. |
| HANDOFF section 4: small host/emu counts and stale foundation next-work lists | Newest section 5/WL-0195 supersede these; many targets long implemented. | Do not quote historical “green” counts as today's certificate. |
| HANDOFF Sept 26: M0–M3, M6/M7 closed; M5/M8 untouched | Export M0/M1/M2/M6 still open; M5 Calc/scene source now exists, M7 closed; M8 indeed incomplete. | Mixed stale tracker and product progress. Neither opening nor closing an epic proves current release readiness. |
| HANDOFF latest main block: lanes running; 1,220 B margin; read SENTINEL “once F11 lands” | Immediately later addendum says all landed, 120 B, none running, sentinel delivered. | Read addendum before its own superseded header. There is no current whole-tree certificate in those claims. |
| HANDOFF preceding block: `0spyf`/`104j3` parked/unverified | Nov 6 merge/worklog addendum records them merged; source confirms relevant work. | Historic archived-lane warning, not current absence. Other archived lanes (FLAIR command, I123 manuals) remain unlanded. |
| InitechBase plan section 0: 17 SET, 64 commands, 94 DRAFT; overlays block all; section 7 “draft pending ratification” | Later SET/SAY work exists; DEC-03a ratified and high runtime merged; section 7 itself supersedes packaging dependence. | Dated snapshot and stale status header. Do not publish those old coverage fractions as today's counts. The 94 unreachable manual entries can still be counted directly. |
| W3/W4 inventory refers to GET/READ and app groups | Query source only implements forms slice 1; all ten W4 groups not-started; macro never instantiated. | A locked future acceptance contract is not implementation evidence. |
| `selfhost`/`ddc` names appear among project commands; M7 considered closed | Makefile deliberately calls `stub_fail` for both. Hosted `test-tps-gen-fpc` does generate and assemble/link the compiler's own source deterministically; in-OS generation still hands assembly to host tooling. Neither proves native compiler reproduction. | Real M7 execution differential; absent M8 proof. “Compiler runs on OS” is true in bounded form and does not imply self-host. |
| PRD sections 8/11 and older handoff say SSIM stub/absent | `harness/ssim.c`, tests, mutants and `ssim` recipe exist. | No longer missing infrastructure; still advisory and not whole-film acceptance. |
| Tri-emulator/“cannot boot Bochs yet” Makefile comments | `harness/emu/bochs.c` and many boot/pixel legs exist; no 86Box driver. | Some comments are stale; **tri-emulator completion remains false**. Boot-only legs are explicitly labeled and must not be described as complete workflow replay. |
| `hgmx` cursor-shadow issue still open | Latest Lane E1 reports steady-cursor kernels and exact Bochs boot; source has that work. | Candidate stale issue, not proven current failure. Suspend/replay issue must be evaluated separately. |
| Every green test means full product complete | Default vector intentionally excludes pending SAMIR cases and M8; W4 framework/tool tests pass independently of app delivery; print/drawing/86Box remain absent. | A certificate proves its named vector at its exact tree, not all PRD milestones. |
| `samir.allow` is the promised “omissions” vehicle in ship-plan introduction | Current `harness/pubs/samir.allow` is an empty **manual coverage debt list**, with its own shrinking policy, not a full III+ feature-omission register. | Do not use an empty documentation allowlist to claim every III+ verb is implemented. Locate/freeze a real product compatibility inventory for W5. |
| GC-12 / DRAFT entries “block release” | `pubs_gate.c`, `tps_inventory.c`, `turbo_gate.c` print DRAFT warnings and can still print green. No dedicated final GM release target was established in the inspected recipes. | This is a real documented release condition, but do not assume ordinary `make test-pubs` mechanically enforces zero DRAFT. Add/identify final release enforcement in the scoped contract. |
| “Physical distribution” and `flair_data.img` imply floppy operation | `make run` attaches boot and data with IDE-style drives; release research has no manufactured pilot evidence. | Bootable emulator disk artifacts are real; `tools/mk_boot_system.sh` emits 2,944 sectors (1,507,328 bytes), 32,768 bytes larger than a standard 2,880-sector/1,474,560-byte 1.44 MB floppy. BIOS floppy/controller/install path and physical media QA are separate. |
| Older blank dates compare low question/ruling | Newer `bkmv` and HANDOFF say one blank operand makes all comparisons false, two blanks true, with index blank-last unchanged. | Older rule is superseded by fresh real captures; issue and docs still need reconciliation, not ad hoc expected-output editing. |

### Evidence quality conclusions

The repo has more rigor than its promotional summaries suggest: independent file readers, real-software mints, fault injection and named mutants exist. However, there are three recurring overstatement patterns: **implementation presence → compatibility**, **harness/inventory delivery → app delivery**, and **one lane's green subset → current whole-tree release certification**. This audit rejects all three substitutions.

The October audits concern commit `e67b8311…` and other dated images. Their summaries are valuable evidence that systematic hands-on testing found serious data loss and many refused features; later October/November commits fixed many findings. They are **not a current 10,569-file bug count**, and this report did not enumerate or revalidate every old finding. Consult current issue/source state before reopening or closing any one.

No hardware support, clean install, native fixpoint, DDC, current full-vector result, print-ready physical pilot or final-owner visual sign-off was established by running it in this audit. Claims labeled “reported” need the exact source tree, command line, outputs and exclusion vector reproduced later.

## 8. Primary sources and reproducibility of this audit

All links point into the audited checkout. Source and recipe observations can be re-read without executing a build.

- [PRD](/home/tobiasosborne/Projects/InitechOS/InitechOS-PRD.md): sections 1–8, M0–M8 in section 11, reference frame Appendix A, original stretch scope.
- [HANDOFF newest state](/home/tobiasosborne/Projects/InitechOS/docs/HANDOFF.md:204): section 5 newest blocks/addendum first; section 4 is historical.
- [ADR-0012 application-platform bar](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0012-North-Star-Expansion-App-Platform-Bar.md); [ADR-0003 DOS service appendix](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0003-InitechDOS-Base-OS-Personality.md:295); [AH register](/home/tobiasosborne/Projects/InitechOS/spec/int21h_register.json).
- [InitechBase ship plan](/home/tobiasosborne/Projects/InitechOS/docs/plans/INITECHBASE-ship-plan.md), especially section 7; [DEC-03a](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0009-AMENDMENT-DEC-03a-InitechMZ-Extended-Memory.md); [kernel overlay ADR](/home/tobiasosborne/Projects/InitechOS/docs/adr/ADR-0014-Kernel-Overlays.md).
- [FLAIR comprehensive plan](/home/tobiasosborne/Projects/InitechOS/docs/plans/FLAIR-implementation-plan.md); [I123 plan](/home/tobiasosborne/Projects/InitechOS/docs/plans/INITECH-123-plan.md); [IWord plan](/home/tobiasosborne/Projects/InitechOS/docs/plans/INITECH-WORD-plan.md); [TPS M7 scope](/home/tobiasosborne/Projects/InitechOS/docs/plans/TPS-M7-subset-plan.md).
- [Latest worklog](/home/tobiasosborne/Projects/InitechOS/docs/worklog/WL-0195-dec03a-workflow-v2-oneshot.md), [day-1 ship expansion](/home/tobiasosborne/Projects/InitechOS/docs/worklog/WL-0188-initechbase-ship-day1.md), [November wave state](/home/tobiasosborne/Projects/InitechOS/docs/worklog/WL-0190-merge-velocity-fanout.md). Latest 40 shards by numeric WL identifier were skimmed, including duplicate WL numbers; historical unmerged-lane prose was not accepted as delivery.
- [M8 failing stubs](/home/tobiasosborne/Projects/InitechOS/Makefile:31932); [compiler and on-OS gates](/home/tobiasosborne/Projects/InitechOS/Makefile:16765); [host-assisted generator](/home/tobiasosborne/Projects/InitechOS/Makefile:8740); [resident Pascal source](/home/tobiasosborne/Projects/InitechOS/os/tps/tps.pas); [seed](/home/tobiasosborne/Projects/InitechOS/seed/initechc.c).
- [Frame ledger placeholders](/home/tobiasosborne/Projects/InitechOS/spec/assets/calc_ledger.json); [Calc](/home/tobiasosborne/Projects/InitechOS/os/apps/calc/calc.c); [Paint](/home/tobiasosborne/Projects/InitechOS/os/apps/paint/paint.c); [Fonts specimen](/home/tobiasosborne/Projects/InitechOS/os/apps/fonts/fonts.c); [scene policy](/home/tobiasosborne/Projects/InitechOS/os/flair/film_scene.h); [live film load](/home/tobiasosborne/Projects/InitechOS/os/milton/kmain.c:6498).
- [Destructive copy-open path](/home/tobiasosborne/Projects/InitechOS/os/flair/tbxgate.c:1747); [SYSINIT directive effects](/home/tobiasosborne/Projects/InitechOS/os/milton/sysinit.c:461); [INT 21h dispatch](/home/tobiasosborne/Projects/InitechOS/os/milton/int21.c:5612).
- [W4 inventory](/home/tobiasosborne/Projects/InitechOS/spec/samir/screen_regrade.json); [W4 uninstantiated acceptance macro](/home/tobiasosborne/Projects/InitechOS/harness/diff/dbf_diff/w4.mk); [forms source](/home/tobiasosborne/Projects/InitechOS/os/samir/cmd/query.c:1724); [94 unreachable catalogue entries](/home/tobiasosborne/Projects/InitechOS/pubs/manuals/initechbase-volume-one/u7-entries.tex).
- [Image/run recipes](/home/tobiasosborne/Projects/InitechOS/Makefile:12290); [Bochs leg limitations](/home/tobiasosborne/Projects/InitechOS/harness/bochs_legs.mk); [publications Makefile](/home/tobiasosborne/Projects/InitechOS/pubs/pubs.mk); [physical design](/home/tobiasosborne/Projects/InitechOS/docs/packaging/DESIGN-SYSTEM.md); [proposed contents](/home/tobiasosborne/Projects/InitechOS/docs/packaging/INITECH-DOCUMENTATION-SET.md); [production research](/home/tobiasosborne/Projects/InitechOS/docs/packaging/PRODUCTION-SOURCING.md).
- [October six-audit integration record](/home/tobiasosborne/Projects/InitechOS/docs/worklog/WL-0176-six-audits-and-the-fix-wave.md); summary sections of [DOS](/home/tobiasosborne/Projects/InitechOS/docs/audits/2026-10-23-dos/REPORT.md), [desktop](/home/tobiasosborne/Projects/InitechOS/docs/audits/2026-10-23-desktop/REPORT.md), [accessories](/home/tobiasosborne/Projects/InitechOS/docs/audits/2026-10-23-das/REPORT.md), [SAMIR](/home/tobiasosborne/Projects/InitechOS/docs/audits/2026-10-23-samir/REPORT.md), [I123](/home/tobiasosborne/Projects/InitechOS/docs/audits/2026-10-23-i123/REPORT.md), [IWord](/home/tobiasosborne/Projects/InitechOS/docs/audits/2026-10-23-iword/REPORT.md), plus earlier top-level REPORT/TRIAGE files. Audit evidence subtrees were not exhaustively inspected.
- [Tracker export](/home/tobiasosborne/Projects/InitechOS/.beads/issues.jsonl): source for counts, dates and issue descriptions, parsed directly because bd is broken. No statuses changed.

## Appendix A. Full primary-theme allocation of active issues

Each issue appears once here, retaining exported status and priority. This supports the theme counts; it is not a release task list and does not change tracker state.

### Boot and foundations

- **initech-2rb** · P0 · open · M1 — Boot to text (MBR → protected → LFB → console)
- **initech-njv** · P0 · open · M0 — Foundations (factory, harness, seed, assets)
- **initech-ta2** · P2 · open · M1 boot robustness: LBA fallback, multi-bpp/mode, LinBytesPerScanLine, tri-emulator VBE

### Bundled apps and reference frame

- **initech-bf2** · P2 · in_progress · FILE COPY dialog: 'Saving tables to disk…' + michael_bolton.conf rate
- **initech-c7u1** · P2 · open · film_scene clip: operator 2026-10-17 'requires work, but is getting there' -- next fidelity pass against the Office Space frame (fixtures/, PRD Appendix A)
- **initech-ddyn** · P2 · open · Add cancellation to the real modal FILE COPY operation
- **initech-fxrq** · P2 · open · Connect Calculator Copy and Paste to the shared Scrap
- **initech-hjgn** · P2 · open · Add anchored Shift-arrow selection to the shared text applications
- **initech-hnhy** · P2 · open · Commit ledger Return input and display file-copy failures
- **initech-ib6** · P2 · open · M5 — Desktop apps + the full reference frame
- **initech-j2ts** · P2 · open · Activate an already-running accessory and report slot capacity clearly
- **initech-j89c** · P2 · open · Keep the film ledger total within its reachable content area
- **initech-lclc** · P2 · open · Build a bounded usable InitechPaint drawing surface
- **initech-qutz** · P2 · open · Add Paint picture Open and Save through one verified native format
- **initech-rp8d** · P2 · open · Kernel FILECOPY dialog may truncate the destination before copying (CALC.EXE saves through it; TABLES.TXT on a full disk)
- **initech-ulqb** · P2 · open · Use current text-window geometry and repaint every scroll change
- **initech-wlu** · P2 · open · Reproduce the entire reference frame as a live arrangement (M5 gate)
- **initech-yhqb** · P2 · open · Make Scrapbook capacity notices fit the alert contract
- **initech-669c** · P3 · open · Offer mouse reading of long Scrapbook text clips
- **initech-8vdjj** · P3 · open · MILTON text console colours: the film's DOS terminal shows a BLACK background with DARK BLUE foreground (operator observation 2026-10-24); today both consoles paint #C0C0C0 on seafoam #6FA08E (look-pass M014) -- decide and re-key when convenient
- **initech-m6v0** · P3 · open · Render Calculator held-key feedback through release delivery
- **initech-tgt0** · P3 · open · Make Date and Time edit-snapshot behavior clear

### DOS kernel, shell and utilities

- **initech-509** · P0 · open · M2 — DOS personality (MILTON): FAT, INT 21h, loader, shell
- **initech-bsy** · P1 · open · [epic] DOS 3.3 feature-parity push (MILTON): close the Appendix-A INT 21h + shell gap
- **initech-xo63b** · P1 · open · Kernel COLD round (DEC-03a.7 capacity): move the pre-existing EXEC prologue (loader_prepare_core ~241 B, in-place/MZ wrappers, the staging half of load_program_from_fat with loader_run_plan as a cold import, loader_load_high staging ~0.3 KiB) to section COLD to recover >= 1 KiB; margin is 52 B after Lane R
- **initech-509.10** · P2 · open · Appendix D baseline: CONFIG.SYS + AUTOEXEC.BAT (DEC-11/12)
- **initech-8479** · P2 · open · [EPIC] Core DOS external utilities (FORMAT/FDISK/CHKDSK/SYS/filters/...)
- **initech-chowu** · P2 · open · tools/mk_ovl_tables.sh stubs every public COLD symbol at 11 B even when the production kernel never references it (mutant-only verbs); a per-kernel export set reclaims bytes
- **initech-drm4v** · P2 · open · int21 CON: output between the two reads of an extended 00,scan pair can consume the scan byte as Ctrl-P/Ctrl-S flow control (pre-existing; now reachable via Alt-Q/Alt-R after initech-wuxo0)
- **initech-euvd** · P2 · open · Bounce-policy redesign gate: kernel growth past 192KiB (KERNEL_SECTORS=384 is the ratified ceiling)
- **initech-f6p0** · P2 · open · Add DATE and TIME entry prompts and TIME fractional-field parsing
- **initech-g8x3** · P2 · open · Apply DOS hidden and system attribute filtering in handle searches
- **initech-hdyz** · P2 · open · Ship a real COMMAND.COM file on the tracer boot volume so FORMAT /S and SYS can transfer it (today the tracer kernel has COMMAND.COM built in) -- coordinate with initech-13ig's transient-overlay EXEC guard
- **initech-hgeq** · P2 · open · Resolve qualified executable suffixes and preserve meaningful EXEC failures
- **initech-i4dq** · P2 · open · test-formatsys-emu-mutant RUNNING leg RED on the base tree: the quit-after harness misses a DU-RETURN that is in the serial log (quit_after_found=0)
- **initech-ijom** · P2 · open · Transfer batch control on a plain invocation and reserve return for CALL
- **initech-j97r** · P2 · open · Implement prompt-level ECHO state and the blank-line shorthand
- **initech-p00t** · P2 · open · Complete FIND file operands, quoted strings, validation and result status
- **initech-p5u7** · P2 · open · test-diskcopy-emu RED: QEMU's fixed ATA medium cannot be swapped (DISK2-SWAP-RED 'Device initech_disk2 is not removable'); the DISKCOPY/DISKCOMP swapped-media replay needs a removable disk2 or a two-image drive setup
- **initech-r7gf** · P2 · open · Execute complete batch files or reject an oversized script before execution
- **initech-r8hv** · P2 · open · FO-3 completeness: flip the MILTON text-console background seafoam->teal (kmain/console.c + re-key test-tracer-boot/ppm_text_check)
- **initech-shkp** · P2 · open · Honor SORT starting columns and reject unsupported options
- **initech-sosa** · P2 · open · Preserve the last completed command template after Ctrl-C
- **initech-tyvt** · P2 · open · Keep batch arguments beyond percent-9 and correct the SHIFT manual
- **initech-we2** · P2 · open · INT 21h gate DPL=0 -> DPL=3 upgrade for ring-3 callers (DEC-04a C-4)
- **initech-wel7** · P2 · open · Expose the child exit status after COMMAND /C
- **initech-xfg8** · P2 · open · Use one correct environment-expansion path for prompt and batch input
- **initech-z3hr** · P2 · open · Provide a secondary interactive COMMAND session with EXIT back to its caller
- **initech-07yn** · P3 · open · Show and clear the MORE indicator at interactive page waits
- **initech-0xg1** · P3 · open · Validate BREAK operands against an independently captured DOS grammar
- **initech-3l4b** · P3 · open · B50: SUBST: native utility first pickup
- **initech-3sr8** · P3 · open · B62: FASTOPEN: native utility first pickup
- **initech-5uet** · P3 · open · B49: APPEND: native utility first pickup
- **initech-61rq** · P3 · open · In-emulator OPEN-by-name device gate: a .COM that OPENs NUL/CON/PRN by name on QEMU
- **initech-6yld** · P3 · open · DOS 4.0/5.0 large disk: >32MB FAT16-big + INT 25/26 packet form
- **initech-7mjc** · P3 · open · test-fat-corrupt-fuzz scratch-image mint flake under concurrent/stale-build runs (Rule 11)
- **initech-8s1e** · P3 · open · B47: COMP: native utility first pickup
- **initech-8zen** · P3 · open · B64: SELECT: native utility first pickup
- **initech-96gj** · P3 · open · Expand PROMPT $V using the native DOS version contract
- **initech-9dbgc** · P3 · open · spec/initechmz_extended.h: state the two parser placement rules mz.c enforces (e_lfarlc >= 0x38 even with zero relocs; relocation table must end within the header paragraphs) so an emitter working from the locked header alone cannot produce a refused image (Muse review note 4 on Lane R)
- **initech-9g9n** · P3 · open · B60: PRINT: native utility first pickup
- **initech-9wx9** · P3 · open · FAT12 allocator: free-cluster search hints from prev+1, can falsely report NO_SPACE under fragmentation
- **initech-a3pl** · P3 · open · In-emulator env-inheritance leg: child .COM echoes an inherited var over serial (1i0x follow-up)
- **initech-axgm** · P3 · open · B57: KEYB: native utility first pickup
- **initech-bcg.14** · P3 · open · P3 robustness + test-gap sweep (audit 2026-06-13)
- **initech-bcg.15** · P3 · open · 8bpp DAC index->seafoam end-to-end oracle (mode-0x13 screendump)
- **initech-bcg.17** · P3 · open · FAT16 AH=36h GET-FREE-SPACE walks all clusters through the 2-sector FAT window (O(clusters) device reads)
- **initech-bsy.3** · P3 · open · AH=3Fh on AUX/PRN read returns EOF (no serial/parallel input driver)
- **initech-bsy.6** · P3 · open · Defensive: conin_cooked_line should bound against a non-terminating BOUND conin source (not just unbound)
- **initech-cg4b** · P3 · open · B46: REPLACE: native utility first pickup
- **initech-ckz8** · P3 · open · B61: GRAPHICS: native utility first pickup
- **initech-cor0** · P3 · open · B56: MODE: native utility first pickup
- **initech-f8aa** · P3 · open · B54: RESTORE: native utility first pickup
- **initech-f9z4** · P3 · open · Shell built-ins: VOL, VERIFY, BREAK, CTTY, TRUENAME
- **initech-ihv9** · P3 · open · B52: JOIN: native utility first pickup
- **initech-irzn** · P3 · open · Mutation-proof the loader EXEC directory-attr guard + tighten test-zs24-exec-mutant leg A to n==1
- **initech-iz5t** · P3 · open · B58: CHCP: native utility first pickup
- **initech-jplu** · P3 · open · IOCTL AH=44h fidelity (4nbn): AL=08 takes BL=drive not a handle; AL=01 asserts an invented DX output
- **initech-mmlc** · P3 · open · B63: EXE2BIN: native utility first pickup
- **initech-n7bq** · P3 · open · MZ .EXE: harness/ parse-only genuine-16-bit MZ differential reader (DEC-08a.6, dtw.3)
- **initech-nd9a** · P3 · open · B48: FC: native utility first pickup
- **initech-nvvo** · P3 · open · EXEC: honor a caller-provided env block at an arbitrary address (currently degraded to inherit-empty)
- **initech-om2a** · P3 · open · [EPIC] DOS 5.0 coverage beyond ADR-0003 Appendix A (phased; ADR amendment required)
- **initech-rcgm** · P3 · open · MEM.COM: report the MCB memory map (DOS 4.0)
- **initech-rxcp** · P3 · open · B55: RECOVER: native utility first pickup
- **initech-t1hl** · P3 · open · DOS 5.0 INT 21h surface: 3306h true-ver, 4B05h, 65xx country, 6Ch ext-open, 69h serial
- **initech-u6x8** · P3 · open · COPY onto a full disk leaves a 0-byte partial target; real DOS 3.3 behaviour unverified (delete the partial target?) -- mint the ground truth first
- **initech-udr1** · P3 · in_progress · EDLIN + DEBUG: line editor and the assembler/debugger
- **initech-ur7k** · P3 · open · FDISK: create/display/activate MBR partitions
- **initech-vd9n** · P3 · open · do_geterr: dedicated AH=59h classifier arm for 0x0050 ERROR_FILE_EXISTS
- **initech-w9ah** · P3 · open · B59: NLSFUNC: native utility first pickup
- **initech-wqhi** · P3 · open · Installable DEVICE= driver loading: load a .SYS device driver named in CONFIG.SYS into the chain
- **initech-xlqij** · P3 · open · loader: test-tsr-exec flat/overlap mutants are caught by marker-count derailment, not a reason-specific assert; add named asserts (Muse review note 3 on initech-h4bk)
- **initech-xv7h** · P3 · open · B53: BACKUP: native utility first pickup
- **initech-ycb3** · P3 · open · RENAME AH=56h: cross-directory MOVE (old_dir != new_dir)
- **initech-yg5r** · P3 · open · INT 26h on A: still enabled for a FAT16 A: volume (kmain.c ~706 pre-existing binding lacks the FAT-type guard DEC-15a.2 claims for every drive)
- **initech-zdiq** · P3 · open · Flagship COMSPEC reads A:\COMMAND.COM while the file lives on C: (command_repl single-floppy seed)
- **initech-zks4** · P3 · open · B51: ASSIGN: native utility first pickup
- **initech-5g73** · P4 · open · DOS 5.0 utility suite: EDIT, QBASIC, DOSKEY, UNDELETE, UNFORMAT, SETVER, HELP, EXPAND, MIRROR, DOSSHELL
- **initech-ppgt** · P4 · open · DOS 5.0 memory: HMA/UMB/XMS (HIMEM/EMM386) + LOADHIGH + DOS=HIGH,UMB
- **initech-ws3x** · P4 · open · DOS 5.0 SHARE/record-locking (5Ch) + network redirector (5D-5F)

### Emulator and hardware validation

- **initech-bcg** · P1 · open · [epic] Kernel hardening: robustness, fuzzing, edge-cases, dual/tri-emulator
- **initech-44m** · P2 · open · harness/emu/86box.c headless driver (flatpak + Qt offscreen)
- **initech-9pzd** · P2 · open · test-flair-samir-suspend-bochs RED on main: expects the stale VBE-ENOMODE marker (Bochs takes VBE 0101h since WL-0170); the leg has no key replay and its bochsrc is still sync=none
- **initech-cqej** · P2 · open · Restart/Shut Down: emulator leg for an app that DECLINES the quit, Bochs leg, and the shutdown clip on main
- **initech-dzp9** · P2 · open · I123/IWORD corpora: replay Lotus 2.2 INIT.EXE personalization under emulation (86Box or DISASM of INIT's diskette test) so the PRISTINE media is the mint runtime, not the third-party initialized tree
- **initech-hgmx** · P2 · open · test-samir-boot-bochs: the RFB BGR233 shadow frame catches the console cursor in the other blink phase (8x2 cells at (16..23,158..159), golden #C0C0C0 ON vs Bochs 0xAB OFF) while the exact snapshot matches all 307200 px -- leg moved to test-bochs-pending
- **initech-k8o5.11** · P2 · open · test-fb-agree: host dual-target render (32bpp + 8bpp-palette) + per-emulator serial content-digest vs HOST model for its own mode
- **initech-q0gy** · P2 · open · 86Box period-VGA headless pixel-capture + host-model DAC calibration (DEC-04; blocks M4 sign-off)
- **initech-r2lqd** · P2 · open · DEC-03a minimum-RAM factory: explicit emulator memory profiles (QEMU -m 8M, Bochs megs: 8) in harness/emu + Makefile, boundary negatives (5 MiB and 7167 KiB refuse, 7168 KiB boots), QEMU + Bochs legs; period run owed to initech-x0i (86Box)
- **initech-xi0q** · P2 · open · Acquire local Intel references for the x87 init (SDM Vol 3A ch 2 and 9, Vol 1 ch 8, AP-485) under docs/ and verify cited section numbers
- **initech-3f13** · P3 · open · Add Bochs legs for test-fpu-absent and test-fpu-mf (QEMU-only today)
- **initech-bykl** · P3 · open · Exercise FPU-absent path and CR0.NE write on a 386-class machine under 86Box
- **initech-bsy.5** · P4 · open · Confirm DOS 3.3 boot default BREAK=ON against an 86Box golden (DEC-16 C-7)

### FLAIR platform and shared UI

- **initech-49ez** · P1 · open · [EPIC] FLAIR Phase 4.5 -- Platform Services (the app-platform bar's table-stakes)
- **initech-k8o5** · P1 · open · [epic] FLAIR Toolbox architecture (ADR-0004/0005) -- region-first System-7 Toolbox
- **initech-re30** · P1 · in_progress · FLAIR Phase 3 -- boot integration / in-OS arc (verb layer, link Managers, live desktop, emu screendump gate)
- **initech-tdnl.106** · P1 · open · FLAIR as a command: boot to InitechDOS, type FLAIR (or let AUTOEXEC.BAT do it) to start the desktop, and Special > Exit to InitechDOS RETURNS to the prompt without a reboot -- the Windows 3.x / GEM-over-DOS flow the PRD names (operator question 2026-10-17: 'we can boot to dos, then start flair right?')
- **initech-0lko** · P2 · open · FLAIR Resource Manager: resource->Toolbox-record instantiation + per-type decoders (NFNT/FOND/CURS/PAT) as apps demand
- **initech-57o4** · P2 · open · Define and render the extended MacRoman text repertoire
- **initech-81ft.4** · P2 · open · FLAIR Platinum alert: mint sampled pink canon rows and icon truth
- **initech-mox1** · P2 · open · Give Standard File filename fields caret and selection editing
- **initech-nc5c** · P2 · open · FLAIR: text moves between two tenant windows through the shell Scrap -- needs a per-tenant window cap above 1 or a second disk-tenant slot; today the Scrap outlives a launch (routing-gated) but no emu gate crosses tenants
- **initech-ncfu** · P2 · open · FLAIR TextEdit + List Manager: full TERec lineStarts + List scroll/multi-column/drag-select, as apps demand
- **initech-o5vm** · P2 · open · FLAIR Print Manager (device-independent draw-to-printer + PC LOAD LETTER)
- **initech-qg22** · P2 · open · All five DrawControl draw paths ignore contrlHilite==255 — disabled controls render pixel-identical to active ones
- **initech-re30.5** · P2 · open · re30 verb-track 2: GrafPort verb layer (Std rect/rgn) + grafProcs dispatch + SetPort/GetPort on the blitter
- **initech-re30.6** · P2 · open · re30 verb-track 3: flip chrome/control/dialog/menu through the verbs; delete cfill/crect/cframe; re-green each Manager oracle
- **initech-re30.7** · P2 · open · re30 verb-track 4: CopyBits + 8 src transfer modes + maskRgn + colorize + one offscreen GWorld
- **initech-smp0** · P2 · open · R3.7 V2: multi-slot disk tenants (array of g_disk_tenant_slot) + multiple windows per tenant; DOS-heap-per-tenant explicitly deferred
- **initech-t4hp** · P2 · open · FLAIR Phase 6 -- app-hosting arc (Initech-versions): canonical suite + Doom + Initech Mines
- **initech-tdnl.32** · P2 · open · Test coverage for unexercised SETMBAR refusal reasons
- **initech-tdnl.69** · P2 · open · Charcoal 12 strike and the cloverleaf command-key glyph (D3-3 row 9; Sys 8 look)
- **initech-tdnl.88** · P2 · open · Dialog push buttons fire on mouseDown; period behaviour is release-inside (TrackControl gets one point)
- **initech-ucch** · P2 · open · draw_check_box and draw_radio_button never read contrlHilite — no pressed (hilited) feedback during TrackControl
- **initech-z0od** · P2 · open · ModalDialog's updateEvt -> DrawDialog case is unreachable — event.c WNE never synthesizes updateEvt; covered dialog regions never repaint in the modal loop
- **initech-z1f5** · P2 · open · FLAIR live menus: close+restore the pull-down on mouse-up (flash-the-selection then desktop_paint_damage), and route the panel clip through CombineRgn (rmsr/FO-D2-8)
- **initech-0lhh7** · P3 · open · Eviction guard: resident DialogItem state holds pointers into COLD .rodata (labels[], replace[], locked_text[]); valid under 5A never-evict, ovl_ptr_check audits static relocations only; these dangle if eviction is ever authorised
- **initech-322p** · P3 · open · Platinum pressed-widget golden (mouse-down capture) to grade the authored close/zoom/collapse pressed art
- **initech-6grv** · P3 · open · FLAIR fonts: spec/assets/geneva9.h does NOT match real Geneva 9 (slashed zero, footed one) -- found by the Calculator DA lane, which authored its own strike; re-author the shared Geneva 9 strike against the Mac OS 8 NFNT / captures and re-key every gate that paints Geneva 9 text (ledger, Finder labels, InitechText)
- **initech-7n9b** · P3 · open · Event Manager autoKey (key auto-repeat after hold across PIT ticks)
- **initech-81ft.1** · P3 · open · FLAIR Platinum unchecked checkbox: mint missing System 8.1 capture
- **initech-81ft.2** · P3 · open · FLAIR Control Manager: add sampled Platinum group-box surface
- **initech-81ft.3** · P3 · open · FLAIR Control Manager: add sampled Platinum popup-button surface
- **initech-81ft.5** · P3 · open · FLAIR Control Manager: sampled Platinum round help button
- **initech-c2x5** · P3 · open · Provide text-surface I-beam feedback and timed caret blinking
- **initech-krnx** · P3 · open · FLAIR Apple menu: About This Computer is the shell alert (three lines + OK), not the Mac OS 8 window with banner art and the per-application memory bars; menu items lack 16x16 icons; the tracked row inverts where Platinum fills it with the accent (GAPS #2 closed by mint 007)
- **initech-rmsr** · P3 · open · FO-D2-8: exercise the GDI CombineRgn facade ON METAL (Photoshop menu-bar clip)
- **initech-tdnl.89** · P3 · open · Empty Trash: 'item in use' purge protection needs the Toolbox Gate to expose the launched file's identity
- **initech-vtl9** · P3 · open · tbx_file_save: no rollback if the final rename fails after the delete (the complete replacement survives only under NAME.$$$) -- decide whether to retry/rename-back or document the EDLIN precedent in the user manuals
- **initech-wrlb** · P3 · open · MenuKey route does not flash the menu title; a tenant menu choice is only un-highlighted by the shell's panel restore

### Factory, proof process and tracker hygiene

- **initech-1x6x2.1** · P1 · in_progress · F7b: make test-par -j safe -- gates must not each $(MAKE) shared kernel/image intermediates (objcopy empty kernel_shell.elf, mformat flair_data.img, kernel_*_mut*.bin races seen in make -j32 test-par, 1261 s rc=2)
- **initech-4sny** · P1 · open · Finder alerts bundle: four record-flair scripts still miss their marker or fixture on main -- finder_demo (FLAIR-MENU menu=256 item=0), dialog_pressed (FINDER-TRASH-ALERT), finder_replace (FINDER-MOVE name=README.TXT), finder_diskfull (record-flair_data.img has no fixture contract); finder_refused and itext_edit repro differ even at load 0.3
- **initech-croo** · P1 · open · [EPIC] 2026-07-09 whole-repo audit improvement plan (docs/plans/AUDIT-2026-07-09-improvement-plan.md)
- **initech-1x6x2.2** · P2 · in_progress · F2b: key test_samir_audit_emu scratch dir per COM (audit/input/undefined emu legs queue on one scratch lock; ~66 s of the 86 s test-merge-samir critical path)
- **initech-3gynk** · P2 · open · DEC-03a capacity allocation: re-point initech-1x6x2 per ship-plan Sec 7; release packaging/shrink effort to Turbo Initech M7/M8 (initech-v872 ground truth, TPS fixpoint) and FLAIR fidelity (initech-tdnl); kernel-cap work stays separate (initech-x8gsi)
- **initech-b9oa** · P2 · open · test-samir-dml-*-emu legs collide on the shared build/tracer_secondary.img under make -j (QEMU exits at launch, serial_len=0); serial runs are green -- key the scratch image per case
- **initech-jk8t** · P2 · open · docs: HANDOFF.md restructure -- archive PRIOR-STATE blocks, fix stale Sec 6/7 (claims WL-0029 latest; 68 shards exist)
- **initech-krfn** · P2 · open · Tracker debt: milestone epics M0/M0.5/M1/M2/M3/M6 (njv, f8v, 2rb, 509, ib6? , 586) are OPEN in beads though closed by their worklog shards (WL-0004/0016/0035/0032); reconcile children and close
- **initech-mm8vz** · P2 · open · Factory: remaining shared-BUILD sub-make in harness/proptest/overlay.mk test-kernel-size-ovl (parallel-producer risk left after initech-lroek)
- **initech-tdnl.92** · P2 · open · record-flair capture races under host load: scroll_arrow and trash_window are not byte-identical at load ~6.5, identical at load ~1
- **initech-ytsm** · P2 · open · test-flair-trash-purge-views RED on main under load: the default 250-tick image ends after the three FINDER-TRASH-VIEW-CLOSED lines, before kctrl-n (the guest never reaches FINDER-NEW-FOLDER)
- **initech-z17ji** · P2 · open · cert-check.sh must run the gate selector against the PROPOSED HEAD's tree (temp worktree at head_sha), not the PM checkout's map -- a lane that adds spec/ paths is refused on main ('unmapped path spec/initechmz_extended.h') although its own cert-check passed
- **initech-z1gxb** · P2 · open · record-flair lockstep: guest tick values at sync points still jitter across captures (REC-SYNC t=/after= differ by up to 7 ticks in disk-using clips, m1 vs m2), pixels identical in every run so far
- **initech-05rz** · P3 · open · Harness mouse injection: shift/modifier token so emu traces can exercise shift-click extend
- **initech-1sdv** · P3 · open · Fixture volumes: a dd/mformat/mcopy failure BEFORE the verifier still leaves the half-built image for the next make
- **initech-2h6j** · P3 · open · spec: chimera_element_map.json has zero consumers -- wire an oracle or demote out of spec/ (Rule 8)
- **initech-6pm** · P3 · open · PREREQ: provision i686-elf-gcc cross-toolchain (ADR-0002)
- **initech-aaan** · P3 · open · lpf3 fault-rollback oracle coverage gaps: mkdir set_entry-EOC-fail leg uncovered; scenario [A] on-disk asserts always-true on fault path
- **initech-b1q0** · P3 · open · Makefile: duplicate recipe for build/mouse.o and build/mouse_mut_eoi.o (lines ~8835 vs ~8837, from 6e4fcf2b and c1c750b1 on 2026-10-17) -- make warns 'overriding recipe' on every kernel build
- **initech-fgdz** · P3 · open · test-absdisk-emu (8403) serial-capture race: flaky under concurrent QEMU relaunch (Rule 11 reproducibility)
- **initech-fjl2** · P3 · open · Makefile: test-samir-fns-pending names a missing target test-samir-fns-century (make stops: No rule to make target)
- **initech-psk9** · P3 · open · Re-record trash_window and every clip whose stored serial ends in FLAIR-LIVE-OK under the new record-end-marker check
- **initech-xcg** · P3 · open · Harness: requested --screendump silently skipped if guest exits before QMP connects
- **initech-zwo8** · P3 · open · record-flair follow-up: auto-capture-on-red frame ring + icount/guest-tick hardening

### Finder and desktop

- **initech-tdnl** · P0 · open · [EPIC] GUI remediation: sellable Platinum desktop (operator directive 2026-08-23)
- **initech-p6st** · P1 · open · R3.4c-UI: Duplicate + Get Info + rename-in-place (tdnl.11 reslice 3/3)
- **initech-tdnl.11** · P1 · in_progress · R3.4 File ops: drag-move, drag-to-Trash + Empty Trash, Duplicate, Get Info, rename-in-place -- mtools-differential
- **initech-tdnl.12** · P1 · in_progress · R3.5 The real Finder menu bar: Apple/File/Edit/View/Special/Help, every item functional or period-grayed, data-driven dispatch
- **initech-tdnl.62** · P1 · open · Disk-full failures are silent and the Finder shows DESKTOP.DB although it was not written
- **initech-tdnl.7** · P1 · open · R2.5 Whole-desktop composite golden: locked scene screendump diffed pixel-exact vs sys8-derived golden
- **initech-tdnl.41** · P2 · open · Dragging a multi-selection moves only the grabbed icon (group icon drag)
- **initech-tdnl.42** · P2 · open · Icon positions inside a folder are not remembered across close/reopen (no kind=3 item-pos persistence)
- **initech-tdnl.43** · P2 · open · Folder window SIZE is not persisted (only origin); resized window reopens at default 360x220
- **initech-tdnl.45** · P2 · open · Finder File menu: Find, Make Alias, Put Away, Print, Page Setup, Sharing, Eject are all disabled stubs; Ctrl-F opens nothing
- **initech-tdnl.46** · P2 · open · Finder Edit menu: Undo/Cut/Copy/Paste/Clear/Show Clipboard/Preferences all disabled; no file copy/cut/paste or Undo
- **initech-tdnl.48** · P2 · open · Finder Help menu: About Help and Show Balloons present but disabled; no help or balloon system
- **initech-tdnl.49** · P2 · open · No application switcher / running-app menu at the right end of the menu bar; Alt-Tab is delivered to the tenant
- **initech-tdnl.54** · P2 · open · Finder window has no item count / available-space header strip; title reads 'Drive A Files' not the volume name
- **initech-tdnl.55** · P2 · open · No contextual menu on right-click or Control-click; Ctrl-click just extends selection
- **initech-tdnl.57** · P2 · open · Finder: 'already exists' Replace alert on a refused move + Put Away from the kind=5 origin records (keep the original name across a Trash rename)
- **initech-tdnl.72** · P2 · open · Refused moves (exists / cycle / err) give no on-screen explanation
- **initech-tdnl.77** · P2 · open · Icon labels of wide 8.3 names overlap their neighbours in the default grid
- **initech-tdnl.79** · P2 · open · Built-in HELLO and NOTES menu items (New, Open, ...) are enabled and do nothing
- **initech-tdnl.80** · P2 · open · Scroll bars follow-ups: horizontal scrolling on emu, Page Up/Down/Home/End, reference-backed thumb and steps, 4-px content left rail, unify control.c with winscroll.h
- **initech-tdnl.83** · P2 · open · Icon labels are clamped into the viewport instead of clipped with their icon when scrolled or narrowed
- **initech-xkr6** · P2 · open · Present and reveal Finder keyboard selection
- **initech-67iug** · P3 · open · kov2-r docs cleanup on merge: WL-0183-finder-kernel-overlay.md collides with two existing WL-0183 shards (renumber); new docs cite gitignored build/lane-kov2-* evidence
- **initech-kkht** · P3 · open · K14: Give unlabeled mounted desktop volumes a readable fallback name
- **initech-syf0** · P3 · open · B84: Count allocated Trash space including directories
- **initech-tdnl.85** · P3 · open · Clicking the scroll thumb without moving changes the scroll value (16 -> 14)

### Initech 123

- **initech-68iw** · P1 · open · [EPIC] Initech 123 -- Lotus 1-2-3 Release 2.2 Initech-version (native FLAIR tenant, two faces over one recalc/.WK1 core)
- **initech-0c0t** · P2 · open · I123 B10: Complete inline editing and F4 reference-form cycling
- **initech-1z3o** · P2 · open · I123 P4 /Data (Fill/Table/Sort/Query/Dist/Matrix/Regression/Parse) + /Print + macros + /Graph text-mode charts
- **initech-3o9o** · P2 · open · I123 B16: Implement END status and block navigation
- **initech-b406** · P2 · open · I123 P5 Face B Mac-windowed view + InitechCalc (R4.6, 116% pie) convergence decision
- **initech-blx5** · P2 · open · B80: Add reached worksheet context help
- **initech-bnps** · P2 · open · I123 B09: Complete the remaining Worksheet branch and its Undo workflow
- **initech-k9st** · P2 · open · Initech 123 /File Save Backup (.BAK) is 'Command not implemented' -- buildable now on TBX_FILE_RENAME (0x0089)
- **initech-li7r** · P2 · open · I123 B04: Resume Finder power actions after an asynchronous worksheet Save
- **initech-ri6x** · P2 · open · I123 corpus: resolve WK1 RPN opcode 0x18 (string concat?), record 0x2E, NNAME trailing byte, General-format 0xFF bit-7 reading (lotus123-decomp GAPS F-1..F-5) by targeted mints
- **initech-rl2g** · P2 · open · I123 B13: Complete the remaining Range menu branch
- **initech-s5c8** · P2 · open · I123 B15: Connect the worksheet vertical scroll bar to the retained view
- **initech-sd7r** · P2 · open · I123 B05: Return prompts and ordinary error notices to their proper caller
- **initech-tx41** · P2 · open · I123 B12: Complete named-range reassignment, formula display and F3 selection
- **initech-umts** · P2 · open · I123 B14: Complete the remaining File branch and worksheet file context
- **initech-vdgd** · P2 · open · B15: Explain worksheet storage exhaustion without blaming valid input
- **initech-0zm4** · P3 · open · I123 B02 emulator replay + clip: capacity/pool scenarios on a data volume (CAP2047 sheet, POOL135.WK1)
- **initech-3cl2** · P3 · open · I123 B19: Explain invalid 8.3 names and no-space saves accurately
- **initech-68iw.1** · P3 · open · Initech 123 Face A: line-1 descriptors ((F3), [W12]), status-line clock, formula text on line 1 (with P2)
- **initech-68iw.2** · P3 · open · lotus123-decomp: mint the display cases Initech 123 authored without a reference
- **initech-h0ss** · P3 · open · I123 P6 era-shortfall ledger + competitive statement (spec/i123/era_shortfalls.md locked)
- **initech-i4x7** · P3 · open · I123 B20: Keep complete selected menu descriptions readable
- **initech-j5ob** · P3 · open · I123 B21: Expose and repaint CAPS and NUM worksheet status
- **initech-q3oh** · P3 · open · I123 B18: Keep moved worksheet column-span metadata consistent with Lotus files
- **initech-suht** · P3 · open · I123 B17: Provide the native Add-In branch lifecycle
- **initech-uyi1** · P3 · open · I123: /Range Name Labels and Name Table write without a protection check

### InitechBase

- **initech-1x6x2** · P1 · in_progress · [EPIC] InitechBase ship: work DAG per docs/plans/INITECHBASE-ship-plan.md (factory F1-F7, overlays, waves 2a/2b/2c/3/4/5, completion gate)
- **initech-9x8cn** · P1 · open · W4.1 full-screen kit: key loop, menu bar + pulldowns, field editor (from READ), status/scoreboard line, screen save/restore -- Opus, Sol review
- **initech-ebwsw** · P1 · open · SAMIR: SET STATUS real layout and @/CLEAR interplay (guidance row 24, double-line separators, <C:>, Command during programs)
- **initech-ipu5.8.1** · P1 · in_progress · InitechBase defects found by the manuals lane (WL-0133 Frictions 1-12): wrong catalogue message for an unknown SET option ('7 File already exists.'), SET INDEX / SET RELATION / SET CONSOLE / SET TALK accepted but inert, ON ESCAPE / ON KEY never fire, DO ... WITH a character constant not passed intact, REPLACE into a memo field stores nothing and empties the memo, LIST OFF ignores its field list, status bar says 'Enter a dBASE III PLUS command.'
- **initech-j7lld** · P1 · open · [EPIC] W4 full-screen apps: BROWSE, EDIT/CHANGE/APPEND, CREATE/MODIFY STRUCTURE, MODIFY COMMAND editor, REPORT designer, LABEL designer, QUERY/SCREEN/VIEW, ASSIST, HELP, CONFIG.DB -- Opus (glut: Sol), Sol review each
- **initech-p3nn0** · P1 · in_progress · W3 forms: @...GET, READ, PICTURE/FUNCTION templates, @...TO, @...CLEAR, CLEAR GETS (SAY already done) -- Opus, Sol review
- **initech-x8gsi** · P1 · in_progress · [EPIC] Kernel overlays: move cold Toolbox managers, strikes and cold data out of the resident kernel into disk-resident overlay modules loaded on demand (operator ruling 2026-10-24: period-correct answer to the 196,608 B bounce ceiling; the DBASE.OVL precedent)
- **initech-y29t4** · P1 · open · W5 ship: bug sweep (18 open), catalogue-message reachability (94 DRAFT), the mechanical completion gate, -j full vector + Bochs + 86Box + full pubs
- **initech-1x6x2.10** · P2 · open · Document CALL, LOAD, IMPORT, EXPORT as omissions (operator ruling 2026-11-01): w2-inventory classes, manuals, named refusal; IMPORT/EXPORT may reopen post-ship
- **initech-1x6x2.12** · P2 · open · EJECT: grade against verb-mint/eject; behaviour rests on one probe (SET PRINT OFF, margin 3); also printer @ SAY with margin + extra CR before row advance, and ? PCOL() after the leading CRLF
- **initech-1x6x2.13** · P2 · open · SET UNIQUE: duplicate-key APPEND/REPLACE into an open unique index must be omitted from the index (real probe); ndx_insert_key still inserts; composite and numeric keys unminted
- **initech-1x6x2.15** · P2 · open · SET FIXED and numeric picture residuals vs real III+: ROUND ABS MAX MIN pictures, decimal*decimal width, round-half-even at 0 decimals (1234.5 -> 1234), -0.04 at 1 decimal prints no minus, >10 integer digits under FIXED
- **initech-1x6x2.16** · P2 · open · SAMIR exponentiation precision: 10^49 scientific display differs from real III+ (.9999E+49 vs .1000E+50); captures in fns-mint/numdisplay + numfmt (PENDING)
- **initech-1x6x2.17** · P2 · open · SAMIR STR(): scientific fallback, range validation and precision differ from real III+; captures in corpus/fns/numfmt.prg, numfmtwide.prg + fns-mint (PENDING), numround PENDING
- **initech-1x6x2.8** · P2 · open · SET ECHO + SET DEBUG: implement against real III+ captures (DEBUG as raw LPT bytes) -- operator ruling 2026-11-01
- **initech-2e6lp** · P2 · open · W2a ~18 pure SET options -- Luna fan-out, Sonnet block reviews, merge trains
- **initech-2mbay** · P2 · open · samir/ndx: mint the two [inferred] incremental series -- duplicate-key insert descent and empty-root reset (Muse review note 2 on initech-7tj3)
- **initech-2trr** · P2 · open · SAMIR control flow vs real III+: a lone ENDIF/ENDCASE is silently ignored by III+ but SAMIR fails loud (#96); a lone ELSE skips the rest of the file; a lone EXIT left no capture
- **initech-4zpa** · P2 · open · SAMIR DISPLAY with two or three STATUS/DISPLAY areas pauses once more than real dBASE III+ (extra pause after area 2) -- paging boundary
- **initech-51yc** · P2 · open · SAMIR: preserve typed precondition and malformed-index errors at command boundaries
- **initech-60hn** · P2 · open · SAMIR: add a common printer output route for LIST and print settings
- **initech-6mhb** · P2 · open · SPEC QUESTION: CTOD of a calendar-invalid date -- real III+ rolls it over (02/30/92 -> 03/01/92, 13/45/92 -> 02/14/93, no error), SAMIR blanks it per initech-9u0f (test-xbase-fn-a I5, CALVALID mutant)
- **initech-8rrq** · P2 · open · SAMIR: support literal TEXT blocks and program command echo in the flow executor
- **initech-9dyu** · P2 · open · B65: Load a SET PROCEDURE library through the existing procedure engine
- **initech-9t8c** · P2 · open · SAMIR: add a bounded SAY, GET and READ form workflow
- **initech-9uqp** · P2 · open · SAMIR desktop launch: provide its terminal controls and consume BEL correctly
- **initech-a2il** · P2 · open · B71: Add function-key definitions and line-input expansion
- **initech-a7rwc** · P2 · open · SAMIR: CLEAR ALL / CLEAR FIELDS / CLEAR TYPEAHEAD not implemented (36); at-clear-c DC proves CLEAR ALL closes databases and releases memvars
- **initech-aql0** · P2 · open · SAMIR: run programs within the advertised byte limit regardless of short-line count
- **initech-byc5** · P2 · open · B19: Reconcile numeric overflow values with the independent III+ result
- **initech-dbag** · P2 · open · test-samir-dml-* has no make prerequisite on build/CLIENTS.DBF (red on a fresh build dir; works after a hand build)
- **initech-fiwr9** · P2 · open · SAMIR: 1E20 numeric literal raises syntax error; @ r,c SAY coordinates do not use the III+ low-16-bit narrowing
- **initech-gmt0** · P2 · open · SAMIR: set both BOF and EOF for physically empty work areas
- **initech-hikn** · P2 · open · SAMIR: connect UNIQUE index creation and SET UNIQUE to the NDX engine
- **initech-i92wq** · P2 · open · SAMIR: with the DO nesting cap disabled, RECINT triple-faults with ESP=0x00008A00 (below the declared 0x00070000 stack bound) -- the stack-bound guard does not hold; the unregistered test-samir-ctl-emu-mutant probe cannot meet its guard-HALT contract (found by lane t3/samir-dotprompt-cursor25-fidelity, 2026-11-06)
- **initech-itcd** · P2 · open · B69: Add ESCAPE and BELL input policies
- **initech-j83c** · P2 · open · B76: Expose the real console cursor position
- **initech-jc5s** · P2 · open · SPEC QUESTION: a blank date compares LOW in expressions in real III+ (blank > CTOD("12/31/99") is .F.), contradicting locked spec R_blankdate_high; index ordering (blank last) unaffected
- **initech-jkhg** · P2 · open · SAMIR: an untrapped program error shows the III+ 'Called from - <drive>:<prog>.prg' trace and the Cancel/Ignore/Suspend prompt; today SAMIR prints the message and drops to the dot prompt
- **initech-kirm** · P2 · open · B74: Expose actual time and free disk space to database functions
- **initech-klim** · P2 · open · B70: Add CONFIRM behavior to field input
- **initech-lo66** · P2 · open · SAMIR: apply SET FIXED through the existing numeric formatter
- **initech-mgzg** · P2 · open · B77: Expose printer position through real printer output state
- **initech-miu6** · P2 · open · SAMIR: ON KEY trap not carried out (accepted, never fires)
- **initech-n76o** · P2 · open · B66: Add database default-directory and search-path settings
- **initech-nbpj** · P2 · open · B18: Repair live-date century transport and blank-date display
- **initech-o9l9** · P2 · open · SAMIR: remove failed new INDEX, SORT and COPY FILE outputs so retry works
- **initech-oxte** · P2 · open · SAMIR: open and edit PRG files through MODIFY COMMAND
- **initech-pdj1** · P2 · open · SAMIR: retain, recall and list command history
- **initech-qugk9** · P2 · open · W2c ~12 UI-coupled SET options (CONFIRM DELIMITERS INTENSITY FORMAT MENUS SCOREBOARD FUNCTION MESSAGE TYPEAHEAD HISTORY DOHISTORY PRINT MARGIN ...)
- **initech-qvpw** · P2 · open · SAMIR: infer INDEX key geometry independently of the current record and enforce its limit
- **initech-ra46** · P2 · open · SAMIR: resolve nested DO calls to disk PRGs
- **initech-s872** · P2 · open · SAMIR: accept complete 254-byte console commands with STATUS both off and on
- **initech-tjh1p** · P2 · open · SAMIR: unhandled program error screen vs real Called from / Cancel, Ignore, or Suspend (at-to-a TC)
- **initech-u78k** · P2 · open · K08 and K06 diagnostics: SAMIR workflows, shared dispatch and errors
- **initech-ugpsk** · P2 · open · InitechBase VERSION() and OS() wording: VERSION() = 'InitechBase version 3.30', OS() = 'InitechDOS 3.30' (operator ruling 2026-10-24); DEC-13 Appendix C controlled-vocabulary rows
- **initech-ujx8** · P2 · open · B75: Add database record-size and product-identification functions
- **initech-vq9oc** · P2 · open · W2b batch verbs (AVERAGE COUNT SUM JOIN UPDATE INSERT EJECT TEXT/ENDTEXT DIR SUSPEND) -- Luna/Sonnet lanes
- **initech-x84d** · P2 · open · B67: Add alternate transcript output
- **initech-ywqf** · P2 · open · SAMIR: refresh the live memo reader after a successful replacement
- **initech-0tl** · P3 · open · @row,col SAY..GET../READ full-screen forms (CP437/Turbo-Vision)
- **initech-1x6x2.11** · P3 · open · SET HISTORY residual divergences vs real III+: SET HISTORY ON while ON grows the ring (hx7 P2); IF/ELSE/DO WHILE/CASE lines not recorded when executed; NOTE lines, leading blanks and && comments not kept; DISPLAY HISTORY page pause; dot-prompt recording
- **initech-1x6x2.14** · P3 · open · SET MEMOWIDTH residuals: CR/LF inside memo text unminted; heading wider than width 0/1 wraps vertically in real III+; character-literal column after memo has no trailing padding; continuation columns misalign after q_putn flush on records wider than Q_LINEBUF
- **initech-1x6x2.9** · P3 · open · SET STEP: implement after the W4.1 key loop -- operator ruling 2026-11-01
- **initech-39zi** · P3 · open · B85: Put the next database prompt on a fresh line
- **initech-586** · P3 · open · M6 — InitechBase (SAMIR): dBASE-alike that really works
- **initech-5sl4** · P3 · open · SAMIR: aggregate + batch verbs (COUNT/SUM/AVERAGE/TOTAL, SORT, COPY TO, COPY STRUCTURE, JOIN, UPDATE)
- **initech-7az.13** · P3 · open · S3.6c transcendental fns SQRT/LOG/EXP (freestanding x87/approx strategy -- GATED)
- **initech-7az.17** · P3 · open · Consolidate command-module register prototypes into samir/commands.h
- **initech-7lgl** · P3 · open · SAMIR exponent width: real III+ prints 2.5^2 as 11-wide '       6.25'; SAMIR always prints 10+d+1
- **initech-8h8m** · P3 · open · SAMIR in the guest: a 500-line recursive procedure stops at about DO level 4 with #43 (PAL heap exhausted) where real III+ reaches #103 at 20 levels -- capacity gap, not a crash
- **initech-8jpa** · P3 · open · SAMIR LIST/DISPLAY: the deleted-record asterisk sits in column 0 while live rows start at column 1 (rows misaligned by one column after the Record# re-layout)
- **initech-bwbt** · P3 · open · samir prog-diff corpus: flow.prg exercises exactly one true CASE -- DOCASE_ALL-class bugs invisible to flow.out
- **initech-cq0j** · P3 · open · Oracle-verify LOCATE NEXT n EOF()/RECNO() semantics on a bounded (non-EOF) miss vs real dBASE III+
- **initech-dzfq** · P3 · open · SAMIR unminted: USE ... INDEX under SET DELETED ON (pointer placement) and SET INDEX TO (default extension + pointer); mint, then align
- **initech-e7qc** · P3 · open · SAMIR graphical frontend: dBASE-Mac-shaped FLAIR windows (RATIFIED 2026-08-17)
- **initech-eaav** · P3 · open · SAMIR: QUIT prints '*** END RUN dBASE III PLUS' in real III+; SAMIR prints nothing
- **initech-f3q5** · P3 · open · SAMIR STR(999.95,4,1) gives '****' where real III+ gives '1000' (STR does not share the storage rounding-before-stars rule)
- **initech-febzx** · P3 · open · samir/ndx: assert .NDX header reserved bytes 8-11 are zero in test-ndx-incremental (Muse review note 1 on initech-7tj3)
- **initech-fjuh** · P3 · open · SAMIR engine follow-up: move flow_state/memvar arena out of static BSS into the PAL arena (retire the static registry)
- **initech-kd41** · P3 · open · SAMIR REPLACE / DELETE / RECALL NEXT n use their own scope loop in mutate.c and probably leave the pointer on n+1 (LIST/DISPLAY fixed by 5vmn); mint first
- **initech-w0ej** · P3 · open · SAMIR: CREATE / MODIFY STRUCTURE (table DDL incl. CREATE FROM ... STRUCTURE EXTENDED)
- **initech-wqhr** · P3 · open · SAMIR: RETRY outside ON ERROR (re-runs the calling DO) is built but has no mint or test
- **initech-ws0b** · P3 · open · SET INDEX with no database in USE: real III+ prompts 'No database is in USE.' rather than raising a trappable error; SAMIR raises #52 with no golden behind the number
- **initech-xd2yn** · P3 · open · SAMIR dates: mint the '#' (not-equal alias) date comparison and the blank/blank >, >=, <> cases in VERIFY.PRG too (Muse review notes 1 and 4 on initech-bkmv)
- **initech-jz16** · P4 · open · SAMIR: REPORT FORM / LABEL FORM runtime (+ MODIFY REPORT/LABEL designers later)
- **initech-qw9a** · P4 · open · SAMIR: full-screen BROWSE / EDIT / full-screen APPEND
- **initech-vda2** · P4 · open · SAMIR: ASSIST menu system (the III+ menu shell)

### InitechWord

- **initech-fdxa** · P1 · open · [EPIC] InitechWord -- WordPerfect 5.1 Initech-version (native FLAIR tenant, two faces over one piece-table document/.WP5 core)
- **initech-25ak** · P2 · open · IWORD: add confirmed delete-to-line and delete-to-page commands
- **initech-5nnt** · P2 · open · IWORD Block: save the selected stream as an independent WP5 file
- **initech-7bsm** · P2 · open · IWORD: add timed document recovery through the save lifecycle
- **initech-8454** · P2 · open · IWORD Format Line: add line-height setting and vertical layout support
- **initech-8q6j** · P2 · open · IWORD P4 print to PRN + headers/footers + footnotes/endnotes + columns + WP 5.1 tables
- **initech-a092** · P2 · open · IWORD Block: connect paragraph extension and code-aware Search
- **initech-aucx** · P2 · open · IWORD: keep status and modal prompts reachable after window resize
- **initech-bp4s** · P2 · open · IWORD Switch: add secondary-document switching and block case conversion
- **initech-c9v4** · P2 · open · IWORD: implement Tab Set and Tab Align through one tab-stop model
- **initech-ej9b** · P2 · open · IWORD: connect document hit testing and scrollbars to its viewport
- **initech-eo91** · P2 · open · IWORD Format: build the Page submenu over the document page model
- **initech-f4xa** · P2 · open · IWORD: consume the selected entry and status measurement units
- **initech-g5fr** · P2 · open · IWORD Format Line: implement widow and orphan pagination policy
- **initech-gj7z** · P2 · open · IWORD: honor Original Document Backup before replacing a saved version
- **initech-gv6l** · P2 · open · IWORD: replace the Help placeholder with reachable context and key help
- **initech-gycm** · P2 · open · IWORD: implement Insert/typeover as an editor mode
- **initech-hiai** · P2 · open · IWORD List Files: implement typed, default and Other Directory paths
- **initech-hift** · P2 · open · IWORD Look: restore cancellation, text scrolling and adjacent-document navigation
- **initech-ht5v** · P2 · open · IWORD: implement the Date/Outline menu with a real dated or numbered result
- **initech-jxn8** · P2 · open · IWORD List Files: implement Find and Name Search over the directory model
- **initech-mxv5** · P2 · open · IWORD: share word boundaries for Ctrl-arrow movement and Delete Word
- **initech-ndqm** · P2 · open · IWORD Setup: connect Location of Files to a shared path resolver
- **initech-nto1** · P2 · open · IWORD Format: build the Document submenu with a real document setting
- **initech-o3py** · P2 · open · IWORD Search: translate Ctrl-M from the actual FLAIR key event
- **initech-q7fu** · P2 · open · IWORD: implement inline Center and Flush Right codes and their entry commands
- **initech-qiy0** · P2 · open · IWORD: support the verified soft-page code variants in real multipage WP5 files
- **initech-qv7t** · P2 · open · FLAIR/IWORD: deliver Finder power requests to Word Exit and expose the save decision
- **initech-ri4c** · P2 · open · IWORD: distinguish zero margins from invalid numeric input
- **initech-tbgk** · P2 · open · Toolbox/IWORD: detect or eliminate silent directory enumeration truncation
- **initech-tu3d** · P2 · open · IWORD corpus: decode WP5 prefix packets 0x0F/0x07/0x0C/0x06 and the D4/00 page code; clone libwpd as oracle (wordperfect51-decomp GAPS F-1..F-3)
- **initech-x6wv** · P2 · open · IWORD: implement bounded Text In/Out conversion through the WP5 core
- **initech-xp4k** · P2 · open · IWORD P5 Face B Mac-windowed view (InitechText lineage) + spell-check/thesaurus data strategy
- **initech-xuq9** · P2 · open · IWORD Format Line: implement hyphenation and its zone as one line-break policy
- **initech-y19x** · P2 · open · IWORD: implement Repeat Value and Home-Space prefix actions
- **initech-yma5** · P2 · open · IWORD: implement reverse Indent through the existing paragraph geometry engine
- **initech-1xzf** · P3 · open · IWORD List Files: add Short/Long Display mode
- **initech-2lke** · P3 · open · IWORD Format: build the Other submenu from independently captured behavior
- **initech-3vhb** · P3 · open · IWORD: test-flair-iword-goto emulator leg (page/cursor markers + unchanged disk bytes) for PgUp/PgDn/Go To
- **initech-599k** · P3 · open · Audit the host route-suite mocks (notepad, scrapbook, iword, i123, calc) for slot-indexed fixture planting now that saves rename -- the itext mock had two LF.TXT entries after an atomic save
- **initech-5rr7** · P3 · open · IWORD: add merge field codes and a bounded Merge/Sort execution path
- **initech-5vnb** · P3 · open · IWORD Setup: implement keyboard layout selection through actual key dispatch
- **initech-9m65** · P3 · open · IWORD P2 remainder: Size/Base Font + Print Color screens (shots MINT28 25/27 exist), Ctrl-F4 without block Tabular Column / Rectangle / Append, footnotes and extended characters; GAPS W-17/W-18/W-19 + No at Delete Block? + F1 at prompts need mints
- **initech-ctql** · P3 · open · IWORD: implement Screen Rewrite and split-view commands
- **initech-doxv** · P3 · open · IWORD: implement Mark Text with an independently verified marking consumer
- **initech-dpvm** · P3 · open · IWORD Format Line: implement line numbering with page-aware output
- **initech-eum2** · P3 · open · IWORD Setup: add the Display submenu and one verified display consumer
- **initech-mpim** · P3 · open · IWORD: show unavailable Font and Format choices consistently
- **initech-muo5** · P3 · open · IWORD Go To completions: Enter (next hard return), Go To + Block (Alt-F4), Go To in columns, Ctrl-PgDn delete-to-end-of-page
- **initech-naoz** · P3 · open · IWORD: Tab/Indent after a word longer than the line -- slice-2 geometry model and slice-3 format reference disagree on the column (5000-session soak found it; layout follows the format reference); GAPS W-16 unminted: mint in wordperfect51-decomp, then align wrap
- **initech-o6vb** · P3 · open · IWORD: implement the Face A Style command over persisted style definitions
- **initech-o9fo** · P3 · open · IWORD P6 era-shortfall ledger + competitive statement (spec/word/era_shortfalls.md locked)
- **initech-pmz9** · P3 · open · IWORD: implement bounded macro definition and playback as one subsystem
- **initech-ppsb** · P3 · open · IWORD Setup: add initial document settings and their creation-time consumer
- **initech-u2to** · P3 · open · IWORD: provide a safe Shell command handoff and return
- **initech-vz4e** · P3 · open · IWORD: sentence detection treats '.' followed by a code (not a space) as not ending the sentence -- unminted; mint Ctrl-F4 Sentence at a code boundary
- **initech-z9vj** · P3 · open · IWORD: implement Graphics insertion and a bounded native document renderer

### Release, publications and physical edition

- **initech-ipu5** · P1 · open · [EPIC] Initech Publications: print-quality manuals for the OS and every app, mirrored on the originals in content/presentation/completeness, correctness enforced by local gates, updated in lockstep with the software (operator directive 2026-10-16)
- **initech-ipu5.11** · P1 · open · FLAIR desktop User's Guide (Macintosh system-software model) with real emulator screendumps, menus and dialogs gated against the FLAIR menu/dialog data
- **initech-ipu5.13** · P1 · open · Manuals-in-lockstep policy: CLAUDE.md Rule 15 proposal + bead template -- any bead that changes user-visible behaviour carries a pubs delta; test-pubs RED on undocumented shipped command
- **initech-ipu5.9** · P1 · open · Initech 123 manuals: Reference + Setting Up/Tutorial, mirrored on Lotus 1-2-3 R2.2, slash-menu tree and @functions gated against the shipped 123.MNU/engine
- **initech-36ih** · P2 · open · Office Space breadcrumb hunt: catalogue every on-screen Initech artifact in the film (TPS report cover sheet, the 'Is this good for the company?' banner, the 'Initiative + Technology = Initech' banner, and every other prop) as ground truth for the OS, the pubs and the physical edition
- **initech-bc2j** · P2 · open · pubs: InitechDOS Reference COPY entry (and User's Guide copy lessons) to the shipped DOS 3.3 COPY of lane doscmds
- **initech-f8b1** · P2 · open · OPERATOR DECISION (Law 4, pubs): back cover policy -- main's initech-borland.sty gives every Turbo Initech book the Borland booklet back cover; the pubsrecap lane intended the family cover on full-size books and the Borland cover only on the booklet (PK-05)
- **initech-gtzi4** · P2 · open · FLAIR manuals: prove the 12 DRAFT entries in the emulator -- Apple menu Control Panels / Note Pad / Scrapbook, the 5 View modes (Small Icon, Name, Size, Kind, Date), upper-bar Quit no-ops -- captures + operation rows so GC-12 stops blocking release
- **initech-j0x4o** · P2 · open · DEC-03a publications lockstep: README system requirements, PRD Sec 5 reference, spec/hardware.json-cited manuals to 8 MB; fix the 4 MB-vs-5120 KiB drift; re-mint gs-dir.txt once (also owed by initech-z8lca); preserve CHKDSK captures (high window is MZ-only); run test-pubs
- **initech-sx8y** · P2 · open · pubs: InitechDOS books to the DEC-18 real drive volumes of lane dosvolumes (0b429bc7) -- A:, C:, D: as mounted drives, drive selection, LASTDRIVE
- **initech-z8lca** · P2 · open · Pubs lockstep drift on main (3 gates RED alone): test-pubs-tps DOC-NOT-SHIPPED main.mentions HEADING|set; test-pubs-examples TRANSCRIPT-DRIFT l1-display.txt byte 239; test-pubs-screens SCREEN-DRIFT gs-dir.txt bytes free 1294336 -> 1282048
- **initech-zwuy** · P2 · open · pubs/screens/capture.sh: the forwild screen (FOR %F IN (*.TXT) DO ECHO %F) does not settle on a prompt under make -j3 (twice tonight), green under -j1 -- the settle wait should key on the prompt echo with a load-proportional budget, not a fixed sleep
- **initech-4nxn** · P3 · open · Correct the TPS manual's shipped exit-to-DOS procedure
- **initech-7t9e** · P3 · open · IWORD manual: correct Esc instructions for leaving List Files
- **initech-lstk** · P3 · open · pubs: mono faces of initech-ops/xbase/borland still set the font's fi ligature (Ligatures=TeXOff only)
- **initech-pian** · P3 · open · pubs: Reference Appendix A lists the FORMAT/SYS/DISKCOPY/DISKCOMP/FDISK messages that are not in the catalogue
- **initech-z62a** · P3 · open · [EPIC] Period-correct physical distribution (the final straight build)
- **initech-z62a.1** · P3 · open · A:SETUP install program (period-style installer)

### SideKick

- **initech-6fdt** · P1 · in_progress · Initech SideKick: the Borland SideKick (1984-ish, v1.5x) Initech-version -- a DOS TSR desk-accessory set (notepad, calculator, calendar/appointments, ASCII table, phone dialer) popped by a hotkey over any InitechDOS program
- **initech-vx9d** · P1 · open · Initech SideKick part (3): SK.COM -- the resident + pop-up menu bar (F1-F7 + Esc), Notepad (WordStar keys, 4000 chars), Calculator (dec/hex/bin, no paper tape in 1.5x), Calendar + Appointments, ASCII table, Dialer; screens and attributes per docs/research/SIDEKICK-period-grounding.md captures

### Stretch, games and easter eggs

- **initech-2ter** · P2 · in_progress · Easter egg: the Jump to Conclusions Mat (Tom Smykowski's invention) -- hidden in InitechOS, discoverable but never advertised
- **initech-225** · P4 · open · S2 — Sound: PC-speaker / Sound Blaster boot chime
- **initech-2eb** · P4 · open · Stretch / Easter eggs (S1 VibeMode, S2 Sound, S3 real-iron)
- **initech-4aw** · P4 · open · S3 — Real-iron boot on period hardware
- **initech-69z** · P4 · open · S1 — VibeMode: unimplemented app hallucinated over UART by a host LLM proxy

### Turbo Initech and compiler finale

- **initech-v872** · P1 · open · turbopascal-decomp: a real Turbo Pascal 5.5 golden master + dosbox-x mint harness + first goldens (IDE screens, error catalogue, runtime error formats, Writeln formatting, shared-corpus outputs) -- the Law-1 footing M8 Turbo Initech lacks
- **initech-590o** · P2 · open · TP5.5 IDE chrome geometry for Turbo Initech blue IDE: menu bar cols, attributes, frames, dialog rects (measured)
- **initech-9sie** · P2 · open · TP5.5 runtime errors: 'Runtime error NNN at SSSS:OOOO.' on stdout, exit code == NNN; 200/201/202/203/204/205/207/210 + file errors 2,3,100,102-106 verified
- **initech-cu0f** · P2 · open · TP5.5 Write/Real/Str/Val formatting: Real default ' 3.1415900000E+00', round half away from zero, string concat truncates at 255, Val no range check
- **initech-k65i** · P2 · open · TP5.5 divergence: Integer is 16-bit and expression arithmetic is NOT widened (200*200=-25536; 32767+1 var=-32768; i shl 16=0; abs(-32768)=-32768)
- **initech-lrqy** · P2 · open · Shared corpus portability to real TP: TP rejects {$MODE}/{$H-} (Error 17), empty parens F() (Error 2), lines >127 chars (Error 11); adapt_for_tp.py documents the minimal edits
- **initech-obk9** · P2 · open · TP5.5 vs shared corpus: {$MODE DELPHI} 32-bit Integer makes REV=54321 where real TP prints -11215 (array_shared, gen_array_shared, gen_array_deep)
- **initech-tdnl.22** · P2 · open · R5.2 FLAIR.PAS binding file + trap RTL stubs in the TPS runtime
- **initech-tdnl.23** · P2 · open · R5.3 First shipped app authored in TPS Pascal (Calculator or Note Pad), seed- AND TPS-compiled, disk-launched
- **initech-x2fd** · P2 · open · TP5.5 compile-error catalogue: adopt real error numbers/texts + 'FILE(line): Error N: text.' format in Turbo Initech (70 numbers verified)
- **initech-iqe** · P3 · open · inline asm support
- **initech-4iq** · P4 · open · Port Turbo Initech onto InitechOS (resident; InitechDOS file I/O)
- **initech-9xu5** · P4 · open · TPS scope guard: pointers/heap, enums, sets, units, inline-asm are POST-fixpoint (do not pull into the self-host critical path)
- **initech-ag4** · P4 · open · Finale demo: boot → IDE → load OS source → Compile → new bootable image
- **initech-dh6** · P4 · open · Compile the compiler from inside the OS (K2 = K1(src))
- **initech-foy** · P4 · open · DDC: diverse double-compilation against an independent seed
- **initech-kyh** · P4 · open · Turbo-Vision blue IDE: editor + compile + run
- **initech-ls4** · P4 · open · M8 — Resident IDE + self-host (the finale)
- **initech-rgd** · P4 · open · Seed lexer: string-literal scratch pool wraps at LEXER_STRBUF_CAP (1024)
- **initech-ui3** · P4 · open · Two-stage fixpoint: assert K2 == K3 bit-identical

## Appendix B. Focused component count subsets

Major table rows use Appendix A domains. The following are named issue subsets for smaller rows; overlaps with parent domains and other shared-component rows are intentional. Untracked work and shared risks are described in the tables even where the focused count is zero.

| Table key | P0/P1/P2/P3/P4 | Issue IDs |
|---|---|---|
| FAT | 0/0/1/6/0 | initech-9wx9, initech-g8x3, initech-ycb3, initech-bcg.17, initech-yg5r, initech-7mjc, initech-aaan |
| INT | 0/0/2/10/0 | initech-drm4v, initech-we2, initech-nvvo, initech-a3pl, initech-n7bq, initech-jplu, initech-vd9n, initech-irzn, initech-xlqij, initech-9dbgc, initech-bsy.3, initech-61rq |
| SHELL | 0/0/10/5/0 | initech-j97r, initech-f6p0, initech-tyvt, initech-z3hr, initech-xfg8, initech-sosa, initech-wel7, initech-hgeq, initech-r7gf, initech-ijom, initech-zdiq, initech-0xg1, initech-96gj, initech-u6x8, initech-f9z4 |
| CONFIG | 0/0/1/1/0 | initech-509.10, initech-wqhi |
| UTIL | 0/0/6/23/1 | initech-8479, initech-p5u7, initech-i4dq, initech-hdyz, initech-shkp, initech-p00t, initech-07yn, initech-8zen, initech-mmlc, initech-3sr8, initech-9g9n, initech-ckz8, initech-w9ah, initech-iz5t, initech-axgm, initech-cor0, initech-rxcp, initech-f8aa, initech-xv7h, initech-ihv9, initech-zks4, initech-3l4b, initech-5uet, initech-nd9a, initech-8s1e, initech-cg4b, initech-udr1, initech-ur7k, initech-rcgm, initech-5g73 |
| OVL | 0/2/2/1/0 | initech-x8gsi, initech-xo63b, initech-chowu, initech-0lhh7, initech-euvd |
| FM | 0/0/7/1/0 | initech-tdnl.41, initech-tdnl.42, initech-tdnl.43, initech-tdnl.54, initech-tdnl.77, initech-tdnl.83, initech-xkr6, initech-kkht |
| CALC | 0/0/2/0/0 | initech-hnhy, initech-j89c |
| CALCDA | 0/0/1/1/0 | initech-fxrq, initech-m6v0 |
| CP | 0/0/0/1/0 | initech-tgt0 |
| NOTE | 0/0/3/0/0 | initech-hjgn, initech-ulqb, initech-j2ts |
| SCRAP | 0/0/2/1/0 | initech-yhqb, initech-669c, initech-j2ts |
| PAINT | 0/0/2/0/0 | initech-qutz, initech-lclc |
| FONTS | 0/0/2/1/0 | initech-6grv, initech-tdnl.69, initech-57o4 |
| ITEXT | 0/0/2/2/0 | initech-hjgn, initech-ulqb, initech-c2x5, initech-vtl9 |
| COPY | 0/0/3/0/0 | initech-bf2, initech-ddyn, initech-rp8d |
| W1 | 0/0/0/2/0 | initech-7az.13, initech-7az.17 |
| W2 | 0/0/20/4/0 | initech-2e6lp, initech-vq9oc, initech-qugk9, initech-1x6x2.8, initech-1x6x2.9, initech-1x6x2.10, initech-1x6x2.11, initech-1x6x2.12, initech-1x6x2.13, initech-1x6x2.14, initech-1x6x2.15, initech-lo66, initech-hikn, initech-miu6, initech-5sl4, initech-8rrq, initech-9dyu, initech-a2il, initech-itcd, initech-klim, initech-n76o, initech-x84d, initech-pdj1, initech-a7rwc |
| W3 | 0/1/1/1/0 | initech-p3nn0, initech-0tl, initech-9t8c |
| W4 | 0/2/1/1/3 | initech-9x8cn, initech-j7lld, initech-qw9a, initech-vda2, initech-w0ej, initech-jz16, initech-oxte |
| W5 | 0/1/0/0/0 | initech-y29t4 |
| SEED | 0/0/0/0/1 | initech-rgd |
| RESIDENT | 0/1/6/0/1 | initech-4iq, initech-v872, initech-k65i, initech-obk9, initech-cu0f, initech-x2fd, initech-9sie, initech-lrqy |
| IDE | 0/0/1/0/1 | initech-kyh, initech-590o |
| FIX | 0/0/0/0/4 | initech-ls4, initech-dh6, initech-ui3, initech-ag4 |
| DDC | 0/0/0/0/1 | initech-foy |
| PACK | 0/0/1/1/0 | initech-z62a, initech-36ih |
| SETUP | 0/0/0/1/0 | initech-z62a.1 |
| DIST | 0/0/1/3/0 | initech-z62a, initech-z62a.1, initech-hdyz, initech-zdiq |
| IRON | 0/0/0/0/1 | initech-4aw |
| PUBS | 0/4/7/4/0 | initech-ipu5.13, initech-ipu5.11, initech-ipu5.9, initech-ipu5, initech-j0x4o, initech-gtzi4, initech-z8lca, initech-zwuy, initech-f8b1, initech-sx8y, initech-bc2j, initech-4nxn, initech-7t9e, initech-pian, initech-lstk |
