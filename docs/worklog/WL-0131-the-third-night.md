# WL-0131 -- The Third Night: Going Wide (2026-10-16 22:00 -> 2026-10-17)

**Issuing Body:** Initech Systems Corporation -- Programme Office (STAPLER)
**Document Class:** Worklog Shard (session record)
**Programme:** STAPLER -- InitechOS

## Context

Operator directive 2026-10-16 22:00 (bd memory `operator-directives-2026-10-16`):
go WIDE toward the north star (a full corporate OS with a productivity
suite, incl. the TPS Report Generator which must require the DOS legacy FCB
API to run); open a MANUALS programme (print quality, mirrored on the
originals, correctness enforced by local gates, updated in lockstep); burn
the whole Claude and Codex weekly quota, Fable and Claude at the same rate;
Opus codes, Sonnet busywork, Codex gpt-6.1-sol for review/coding/computer
use; smaller apps one-shot by Opus/sol subagents; ZENO WARNING against
review loops. Added mid-session: the TPS Report Generator needs a dedicated
manual replete with busywork rituals (SPACE exactly 7 times etc.), and the
rituals are specified program behaviour.

Starting state: main e203631 (certified 12f62a0 ALL GREEN 450 host + 241
emu); jojk slice 3 WIP recovered on origin; a5sr merged.

## What changed (merge order on main)

| Lane | Model | Landed | Beads |
|---|---|---|---|
| alert-chrome gate honesty | PM | `test-flair-alert-chrome` (+mutant) appended to TEST_EMU_GATES (never in the vector; the certificate had not run them) + Bochs leg `test-flair-alert-chrome-bochs` | tdnl.99, tdnl.100, rgz1 closed |
| CHKDSK.COM | Codex sol | volume report, lost chains /F -> FILEnnnn.CHK, cross-links, /V, fragmentation; INT 26h FAT cache coherence hook; 12 mutants | kd4a |
| Menu-bar clock | Codex sol | R3.6 RTC via AH=2Ch seam, Mac OS 8 menubar capture + independent Chicago NFNT; DELIBERATE locked-spec change chicago12.art (11 px, WL-0115) | tdnl.13 |
| ATTRIB / LABEL / VOL | Codex sol | mattrib/mlabel differential, 16 mutants | gyvx, f88h |
| SAMIR findings | Opus | USE INDEX .NDX default, index-order start record, NEXT n pointer, SET-state leak; mint session 005 vs real dBASE III PLUS 1.1 | vfys, k7lc, 5vmn, x5cc |
| XCOPY / TREE | Codex sol | mcopy -s / mdir differential, injected prompts, Ctrl-C cancel, 24+3 mutants; runtime renamed xcopy_tree.* (PM) | zuvo |
| Pubs toolchain | Opus | `initech-manual.cls`, `make pubs`, byte-reproducible PDF (SOURCE_DATE_EPOCH + suppressoptionalinfo), `test-pubs-build/-honesty/-repro/-screens`, console text decoder `harness/console_text.c`, 32-page InitechDOS User's Guide specimen with 7 real screens | ipu5.5 |
| InitechCalc | Codex sol | CALC.EXE over the shared i123 core; LOCKED spec/assets/calc_ledger.json (TODO_FRAME cells); 570-, $112,276.87, the 116% pie; 7 mutants | tdnl.20 (a) |
| FCB API + TPSREP.COM | Opus | INT 21h 0Fh-24h in full, default FCBs at PSP 5Ch/6Ch; TPS Report Generator with the five rituals as spec/tps_ritual.json rows; goldens from REAL MS-DOS 6.22 under dosbox-x (`make fcb-mint-realdos`) | 509.9, 8479.1 |
| Design reference | Opus | docs/pubs/DESIGN-REFERENCE.md (PUB-REF-0001, 998 lines) from seven page-cited structural annexes of the originals (docs/pubs/originals/) | ipu5.1-.4 |
| EDLIN.COM | Codex sol | manual model replayed host + QEMU, 4 mutants; kbd extended-key pairs; harness --keys >1023 fails loud | udr1 (EDLIN half) |
| MBR partition mount | Codex sol | DEC-07a contract spec/mbr_partition_contract.json, hidden_sectors offset, superfloppy fallback, partitioned fixture at LBA 63 | kzfs |
| DOS 3.3 parity | Codex sol | file/dir date-time stamps, DIR drive/path args, PATH command, startup date/time prompts | dwbl, e47v, sy5f, qskz |
| InitechWord P2 slice 3 | Opus | line spacing, Home prefix keys, block attributes, Ctrl-F4 without block, Format screens; MINT21..32; `test-flair-iword-format` | jojk |
| SAMIR LIST/DISPLAY layout | Codex sol | byte-exact to real dBASE (mint 005: 7+2 prefix, unpadded last heading, program-opening CR LF); audit stops collapsing spaces; boot screendump band re-keyed | 31d1 |
| SAMIR error 12 | Codex sol | undefined variable -> `12  Variable not found.` (dot prompt, ON ERROR, ERROR()) | kdna |
| Arrow cursor | Codex sol | strike matches the Mac OS 8 capture 0/256 cells (was 48/256) | tdnl.98 |
| IO.SYS + INITDOS.SYS | Codex sol | two-file kernel split, FAT-walking stage2, Non-System disk path, Bochs legs green; then EVERY other boot image (76 recipes, ~130 targets) moved to the boot-system tool (Sonnet) | kod |
| DOS 3.3 parity batch 2 | Codex sol | IF/FOR dispatch any internal command (the IF...GOTO loop), CD root/parent/absolute, PROMPT \$D/\$\$, FOR wildcards, console ^Z/^C | s4e8 33t2 iffk lqdc |
| Kernel size round 3 | Codex sol | part 1 section GC (+10.3 KB), part 2 -Oz / -flto cold TUs (+11.1 KB): binding headroom 3,288 -> 24,632 B; mutant kernels go through the house macros | tode (open: round 4 if needed) |
| InitechText | Opus | shared TextEdit surface, desk Scrap, Standard File over the FAT, TEXT.EXE; 15 Mac OS 8.1 captures (mint 007) | tdnl.16 |
| Film scene | Codex sol | the real InitechCalc windows in the authored arrangement, hourglass during Saving | tdnl.102-.104 |
| Harness RTC | PM | the QEMU harness and record-flair pin the guest RTC by default (`--rtc-wall` opts out): the clock made unpinned runs/clips non-reproducible | - |
| Bochs legs | PM | generic `bochs-flair-leg` macro (harness/bochs_legs.mk), legs for the clock and InitechWord gate kernels | cqej |
| InitechDOS manuals pilot | Opus | Reference (108 pp, an entry for every shipped command, generated Messages appendix) + User's Guide (88 pp), 66 live screens, reconciled with the pubs gates: ONE command-registration mechanism fed by the real batch/CONFIG.SYS parsers; DRAFT markers 35 -> 20; allow-list 104 -> 81 | ipu5.7 |
| COPY keeps the source stamp | Codex sol | AH=57h after the final write; XCOPY proven; manual screens re-captured | qqrz |
| FILE COPY + InitechPaint | Opus | the Saving tables dialog over a real FAT copy (MICHAEL.CFG rate), the Platinum progress bar minted from a real Mac OS 8.1 Finder copy (PM ruled teal per OQ-2); PAINT.EXE the bar carrier; clips file_copy, paint_bar | bf2, tdnl.20 |
| Exit to InitechDOS + the TPS journey | Codex sol + PM | Special > Exit to InitechDOS (alert, put-away, DESKTOP.DB commit, console rebuilt) hands the console to a TRANSIENT COMMAND.COM relinked at 0x580000; TPSREP.COM/TPSPARM.DAT/TPS.DBF ship in APPS; the journey gate types the ritual EXACTLY as test-tps-report leg A and diffs the console transcript against the real-DOS golden span; 3 mutants (SIX, NOFILES, CWDRESET). PM relocation: the transient lives in the system area AFTER the 46-cylinder FAT boot image (TRANSIENT_LBA 2944, image 48 cylinders) because LBA 401 fell inside INITDOS.SYS under the two-file layout; LTO internalised the transient's imports, so a generated response file of --undefined flags (transient imports intersected with the kernel's definitions) rides every flagship-family link; exit/journey traces answer the startup date/time prompts (no AUTOEXEC.BAT on the flagship volume); the exit screen floor moved under the longer listing; test-boot-system-repro compares the 2944-sector FAT boot image | 3p1a |
| Menu divider + InitechText scroll | Codex sol | divider starts at panel left + 1 like Mac OS 8; vertical scroll routed through the bar; test-flair-itext-scroll(+mutant) in the vector; itext_scroll clip | r78f, ehq4 |
| DOS messages + Ctrl-Break + not-ready | Codex sol | MSG-DOS-0006/0008/0013/0014/0015 wired to real conditions (Ctrl-Break under a program, drive not ready through the harness fault injector); test-qemu-paths; README cites spec/hardware.json + DEC-02 | vlhp, nstg |
| DEC-13 source purity | Codex sol | CHKDSK, XCOPY/TREE, ATTRIB/LABEL, EDLIN print through the MSG_DOS_* catalogue macros, binaries byte-identical; test-dosmsg [3/3] green | 6png |
| Calculator DA + Apple menu | Opus (renamed by Sonnet: calcda / CALCULAT.EXE) | the rounded rDocProc window (NEWWINDOW WDEF bits 8..15), About This Computer from the banner + the live MCB chain, desk accessories launched through the ONE launch verb; test-calcda/-apple-menu host + test-flair-apple-menu/-calcda(+mutants); clips apple_menu, calculator | tdnl.17, tdnl.47 |

PM re-keys at each merge (all stated in the files): the APPS volume grew
from 5 to 12 files (CHKDSK, ATTRIB, LABEL, XCOPY, TREE, CALC.EXE,
EDLIN.COM), so the stranded-TRASH cell in `FLAIR_SVC_DAMAGED_SPEC` moved
8 -> 10 -> 11 -> 12 (cell n centre = 75+68*(n%4), 122+52*(n/4); parks keep
absolute targets), `locked_trash.sh` purged= 8 -> 10 -> 11 -> 12,
`FLAIR_FIXTURES` grew, and `FLAIR_CALC_SPEC` moved CALC.EXE to cell 10.
`record-flair` now pins the RTC for every clip (`RECORD_RTC_DEFAULT`):
the clock made unpinned clips non-reproducible (6x9 px patch at
563..568, 5..13). The menubar_clock clip records from a 3000-tick image.
Manual screens re-captured after the startup change (every capture
answers the date/time prompts first).

## Why

The north star is a believable corporate OS; the frontier is widened by
shipping the DOS externals and small apps as one-shot lanes, the flagship
vestigial feature (FCB + TPS) with real-DOS goldens, and the manuals
programme as a first-class product line with its own oracles.

## Frictions

- Gate honesty hole: a lane defined gates without appending them to the
  vector; the certificate did not run them. Checked at every merge now.
- The reference frame still `spec/assets/preview.webp` is gitignored, so
  clones and worktrees do not get it; the first frame-apps pass stopped
  on it. ADR-0010 HER-11 forbids pixel grading against it anyway.
- Codex lanes stop honestly on missing goldens (Platinum progress bar,
  floppy icon) or precedence questions (EDLIN manual rule vs printed
  example); the PM decides and relaunches or re-routes to an Opus lane
  that can mint.
- Two Codex lanes both created os/milton/dosutil.*; the XCOPY runtime was
  renamed xcopy_tree.* before merging.
- `git add -A` on main while a writing lane wrote in the main tree swept
  a half-written file into a merge (bd memory). Explicit paths now.
- The menu-bar clock silently broke clip reproducibility for every script, and
  made gates flaky at minute boundaries: the harness now pins the RTC.
- The two-file split converted only the flagship/tracer images; every mutant and
  fixture image still wrote raw sectors at LBA 17 and the strict loader refused
  them ("Non-System disk") -- a latent red across the whole mutant vector, found
  on the first mutant that rebuilt. All 76 recipes now use the boot-system tool.
- Merge-resolver lessons (bd memories): keeping both sides of a RECIPE hunk
  duplicates link/compile lines (the second runs in a fresh shell, or the plain
  ld rule beats the LTO one); a lane committed in a sibling clone must be FETCHED
  before `git merge` (it fails silently and the gates run on unchanged main); a
  queued chain must not wait on a log pattern another chain can print.
- The transient COMMAND.COM: its sector slot (LBA 401) was swallowed by the two-file split, and
  -flto dropped the kernel symbols only the transient imports (EXTERN() is invisible to the
  plugin). Relocated to the system area past the FAT boot image; a response file of
  --undefined flags keeps the imports. make does not track recipe edits: a mutant kernel bin
  whose rule gained the 384-sector pad stayed stale until deleted ("Non-System disk").
- The DOS console prints CR LF: record-flair's exact-line marker grep missed the journey's end
  line until the serial was CR-stripped; the journey clip needed a 700 ms settle (polled keys,
  ~50 rows of report output after the last key).
- The APPS volume grew from 5 to 20 files in one night; launch traces and the
  stranded-TRASH key moved with every file until the damaged fixture put TRASH
  at entry 0 (bead zc68 for the \\DOS folder layout).

## Acceptance

Every lane's family + mutants re-run first-person on main after its merge,
plus the volume re-key gates (finder-service, locked-purge, empty-trash,
app-launch) and the function-key tenants after the kbd change. Clips
recorded and repro-proven on main: calc_ledger, menubar_clock, the four InitechWord
clips, itext_edit, itext_savefile, film_scene, tps_journey, file_copy, paint_bar,
itext_scroll, apple_menu, calculator. Clip determinism: record-image kernels keep a
steady console cursor and step the FILE COPY bar per injected event (shipped kernel
untouched). Full certificate: NOT yet run
tonight (next session's first action, `make clean && make test` in
../initech-os-cert). Bochs legs owed under initech-cqej for every new emu
gate.

## Pointers

- Beads: epic initech-ipu5 (manuals), initech-3p1a (exit FLAIR to DOS +
  ship TPS on the flagship + journey clip), initech-ipu5.14 (TPS manual),
  initech-dam note (the still), initech-cqej (Bochs legs owed).
- PM scratchpad logs: merge*-gates.log, clips*.log, codex/*.out.
- Reference repos touched: dbase3-decomp (mint 005), wordperfect51-decomp
  (MINT21..32, not pushed), lotus123-decomp/mint/dos622.img (real DOS).
