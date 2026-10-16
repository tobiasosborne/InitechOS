# WL-0118: SAMIR LIST/DISPLAY layout (initech-31d1)

The one-shot SAMIR lane corrects the shared LIST/DISPLAY renderer against real
dBASE III PLUS 1.1 mint session 005. No mint, commit, push, bead edit, or change
to CLAUDE.md/HANDOFF.md was performed. `bd show initech-31d1` was attempted:
the clone has no usable beads database; the sibling database cannot acquire
its lock under read-only access. The exported issue snapshot lacks this bead.
The operator's lane description supplied the scope.

## Ground truth

The authority for exact bytes is the read-only sibling corpus:

- `dbase3-decomp/re/mint-results-005.md`, "LIST layout" and "NEXT n".
- `dbase3-decomp/mint/work/M005U.PRG`, `M005ULOG.TXT`: one indexed LIST table.
- `dbase3-decomp/mint/work/M005N.PRG`, `M005NLOG.TXT`: seven LIST tables and
  one DISPLAY NEXT 2 table, including natural/index order, single-field and
  two-field headings. Each record number occupies seven columns followed by
  two spaces; the final heading has no padding; final values retain padding;
  a table in a program opens with CR LF; all table lines end CR LF.
- Existing `harness/diff/dbf_diff/use_mint005.md5` pins all four files and the
  six DBF/DBT/NDX inputs. The integrity check passed again after all runs.
- *Using dBase III Plus*, U5-144 (PDF page 256): LIST/OFF and DISPLAY ALL;
  U5-235 (PDF page 347): SET HEADING and column width = max(title, field).
  Both pages were rendered at 110 dpi and visually inspected. These pages
  support the command/width rules, not the exact incidental spacing bytes.
- PRD 6.6; ADR-0003 DEC-08 and Appendix A (existing flat app/PAL boundary).

No DOS/FAT behavior was implemented. mtools stages the same corpus bytes,
compares the copied inputs, and extracts guest stdout; fsck.fat checks the
image before and after execution. No SAMIR codec generates the expected text.
Deleted-marker placement has no M005 case and remains unchanged/authored.

## Change and deliberate re-keys

- `os/samir/cmd/query.c`: 7+2 record prefix; strip only the final heading's
  padding; program-opening CR LF; literal CR LF table line endings, including
  redirected stdout. Dot commands use the existing typed Enter boundary.
- `test_samir_audit.c`: preserve every space, including final value padding,
  in M005 comparison. Run each original LIST/DISPLAY command verbatim through
  a single-command PRG, retaining program context. Only the pre-existing
  prompt/error-capture adaptations and CR-to-LF transport normalization remain.
  K06 marker/RECALL expectations use 7+2 with an explicit M005 citation. K05's
  numeric rows now expect literal CR LF.
- `test_interp_list.c`: re-key record prefixes, final heading, program opening
  and line endings/line counts; remove the obsolete claim that all layout is
  ungraded. Synthetic values/scopes remain authored controls.
- `test_samir_repl.c`: re-key its three record prefixes.
- `test_samir_screen_emu.c`: re-key numeric LIST/DISPLAY prefixes and final
  heading. STATUS serial consists of positioned text spans, so its assertion
  joins the unpadded heading to the following record span, without inventing
  serial newline bytes.
- `xbase_prog_diff/golden/query.out` and `corpus/query.prg`: re-key the
  synthetic program's prefixes and opening line breaks; document M005 rules.
  This authored fixture is not claimed to be a historical transcript.
- `numeric_format_driver.c`: explicitly select dot-command context for its
  U5-144 field-format oracle; program-opening bytes have the separate M005 gate.
- `mint_clients_dbf.c`: correct the explanatory layout example only; DBF bytes
  are unchanged.
- `Makefile`: strengthen test-samir-boot with complete heading/record-line
  assertions. Re-key the USE-NDX-extension mutant's diagnostic from one to two
  spaces because the replay no longer squeezes whitespace. Add the four gates
  below to TEST_UNIT_GATES / TEST_EMU_GATES.

No FLAIR golden or purge count changed: SAMIR is on its separate volume, and
the FLAIR launch trace/heap availability remains unchanged.

## Red before implementation

The strengthened host replay failed on both M005U and M005N. The guest gate
failed all 18 table comparisons (nine serial plus nine extracted stdout):

```
test-samir-audit: 132 checks, 2 failures
test-samir-list-layout-emu (files): 106 checks, 18 failures
```

These runs preceded the query.c implementation change. Logs are archived in
`build/samir-list-layout-cert/initech-31d1-{host,emu}-red.log`.

## New gates and final GREEN tails

```
make test-samir-list-layout
test-samir-audit: 132 checks, 0 failures
>>> test-samir-list-layout: green (M005U/M005N spaces and program-opening CR LF graded)

make test-samir-list-layout-mutant
>>> test-samir-list-layout-mutant: green

make test-samir-list-layout-emu
test-samir-list-layout-emu (files): 107 checks, 0 failures
ppm_text_check: CELLS PASS -- 240 explicit ink/blank cells
test-samir-list-layout-emu (screen): 94 checks, 0 failures
>>> test-samir-list-layout-emu: green (9 real tables on serial/disk; M005U console cells)

make test-samir-list-layout-emu-mutant
>>> test-samir-list-layout-emu-mutant: green (three clean guest returns; wrong bytes rejected)
```

The emulator uses a locked injected trace and verifies every key was sent,
clean completion, no triple fault, and no panic. Its screen projection reads
the real M005U heading and rows directly from the log; 240 cells are graded.
The captured screen was also viewed (`build/samir-list-layout-cert/screen.png`).

Each compile-time implementation mutant fails both host M005 replays and
the SAME guest serial/stdout oracle. The named guest RED lines are:

```
SAMIR_LIST_MUTATE_COLUMNS: LIST M005U table 0 stdout file differs at byte 37
SAMIR_LIST_MUTATE_HEADING: LIST M005U table 0 stdout file differs at byte 30
SAMIR_LIST_MUTATE_OPENING: LIST M005U table 0 stdout file differs at byte 0
```

The host RED for each is `FAIL ...test_samir_audit.c:319: M005U real-dBASE
golden: USE INDEX defaults .NDX and starts at the index top`. Its first
differing normalized byte is 207 / 201 / 172 respectively. Mutation logs and
clean guest-return reports are `build/samir-list-layout-{emu-}mut-*`.

## Existing gate acceptance

All commands exported INITECH_QMP_STDIO=1 and absolute DBASE3_DECOMP,
SYSTEM7_DECOMP, WIN31_DECOMP, LOTUS123_DECOMP, WORDPERFECT51_DECOMP roots.
Memory/disk checks showed 25 GiB available memory and 132 GiB free disk.
Neither `make test` nor Bochs ran.

| make target | GREEN tail / result |
| --- | --- |
| test-interp-list | 108 checks, 0 failures; `>>> test-interp-list: green` |
| test-interp-list-mutant | `green (CONTINUE re-scope correctly RED)` |
| test-samir-repl | 47 checks, 0 failures; `>>> test-samir-repl: green` |
| test-samir-repl-mutant | `green (REPLACE-unregistered correctly RED)` |
| test-samir-query | 30 checks, 0 failures; `>>> test-samir-query: green` |
| test-samir-view | 136 checks, 0 failures; `>>> test-samir-view: green` |
| test-samir-view-mutant | filter/deleted/marker correctly RED; `green` |
| test-samir-use | 132 checks, 0 failures; `green (incl. mint 005 USE INDEX / NEXT n replays vs real dBASE III PLUS 1.1)` |
| test-samir-use-mutant | all seven mutants correctly RED; `green` |
| test-samir-report | 1056 checks, 0 failures; `green (111 REPORT FORM files byte-identical to real dBASE III PLUS 1.1)` |
| test-samir-report-mutant | subtotal/column/page correctly RED; `green` |
| test-dbase-diff | 8 checks, 0 failures; `>>> test-dbase-diff: green` |
| test-dbase-diff-mutants | `green (ALL SIX corpus goldens mutation-proven)` |
| test-samir-numeric-format | `8957 cells PASS (manual-derived, no LIST golden)` |
| test-samir-numeric-format-fraction-mutant | `PASS (implementation mutant RED: descriptor differential)` |
| test-samir-numeric-format-left-mutant | `PASS (implementation mutant RED: descriptor differential)` |
| test-interp-list-heading-mutant | `PASS (implementation mutant RED: screen controls)` |
| test-interp-set-console-mutant | `PASS (implementation mutant RED: screen controls)` |
| test-samir-boot | `[5/5]: screendump shows SAMIR's dot prompt + LIST table on the seafoam desktop`; `VERDICT: PASS` |
| test-samir-boot-mutant | `green (short-read mutant correctly RED -- LIST rows garbled/absent, no crash)` |
| test-samir-audit-emu | 570 checks, 0 failures; `green` |
| test-samir-input-emu | 113 checks, 0 failures; `green` |
| test-samir-status-emu | 17 checks, 0 failures |
| test-samir-status-off-emu | 16 checks, 0 failures |
| test-samir-numeric-format-emu | 14 checks, 0 failures |
| test-samir-console-emu | 17 checks, 0 failures |
| test-samir-report-emu | files 48 checks, screen 132 checks, 0 failures; 1920 screen cells PASS; `green` |
| test-flair-samir-suspend | `[4/4]: pre.ppm == post.ppm (desktop rebuilt BIT-IDENTICAL)`; `VERDICT: PASS` |
| image | `>>> image: build/initech.img (flagship alias of build/flair_tenants.img -- FLAIR desktop + HELLO/NOTES co-resident tenants)`; data image built |
| test-flair-app-launch | `[10/10] MENU ... clean exit after`; `>>> test-flair-app-launch: green` (RTC detail below) |
| test-flair-locked-purge | `VERDICT: PASS -- test-flair-locked-purge (locked bytes/attributes intact, visible alert pixels, mtools + fsck.fat)` |
| test-flair-finder-service | `>>> test-flair-finder-service: green` |

App-launch's initial unpinned RTC run crossed a menu-bar minute: exactly 96
bytes differed, confined to x=563..568/y=5..13, while its locked launch trace
passed. Final acceptance pins only the guest RTC using this temporary wrapper:

```
#!/bin/sh
exec /home/tobias/Projects/initech-os-samir2/build/qemu_harness --rtc-base 1986-07-24T12:00:00 "$@"
```

The exact make invocation was `make test-flair-app-launch
HARNESS_BIN=/tmp/initech-31d1-qemu`. The wrapper and environment exports are
archived in `build/samir-list-layout-cert/`; no unrelated RTC code/golden was
changed. A preliminary wrapper with --rtc-deterministic changed mouse timing
and was discarded; the accepted run uses ordinary QEMU timing.

Complete original logs, including initial REDs and successful final reruns,
are archived in `build/samir-list-layout-cert/`. No requested implementation
remains. Unminted cases retain their pre-existing limitations; bead access
and the unrelated unpinned clock sensitivity are reported for PM context.
