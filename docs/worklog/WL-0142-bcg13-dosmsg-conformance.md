# initech-bcg.13: catalogue extraction and DIR matcher conformance

One-shot lane, 2026-10-17. No commits, pushes, git operations, tracker writes,
CLAUDE.md edits or HANDOFF.md edits. `bd show initech-bcg.13` found no database;
read the issue and initech-bsy.1 in .beads/issues.jsonl instead. Read CLAUDE.md
top to bottom, PRD 6.1/8, ADR-0003 Appendix A/C and compliance item 7.4,
WL-0130 and the relevant WL-0131 rows before implementation. No subagents.

## Result and stop condition

The new family is GREEN, including its mutants. Required regression acceptance
is INCOMPLETE: test-edlin-emu stops at `RED EDLIN-output case=new`. The injected
Ctrl-C produces `INT23-BREAK`, terminates EDLIN before saving IN.TXT, and leaves
the subsequent L/e keys at the shell. This conflicts with the existing EDLIN
trace's expectation that its AH=07h raw input handles Ctrl-C locally.

The non-console BREAK ON entry check at os/milton/int21.c:4992 also covers
EDLIN's redirected AH=40h output. That is the inferred interception point;
the serial trace proves the INT23 termination, not the exact interrupted call.
No BREAK or EDLIN oracle changes were made to hide this disagreement. Following
the lane's explicit stop-on-blocker instruction, remaining gates were not run.

To distinguish a string migration regression from the pre-existing conflict,
reconstructed pre-lane edlin.c literals from build/dosmsg_unique.tsv and
/tmp/dosconf_rawmap.json, compiled with the same freestanding flags, and linked
with the same start/adapter objects. The resulting complete flat program is
byte-identical to the current EDLIN program:

```
10768ad974ccb3098320cad442e06b821f56142fd5852d6c6d68d44ee209f037  build/dosconf_edlin_original.bin
10768ad974ccb3098320cad442e06b821f56142fd5852d6c6d68d44ee209f037  build/edlin_program.bin
```

Evidence: build/dosconf_test-edlin-emu.log,
build/edlin_edlin_program.bin_new.serial,
build/edlin-emu-edlin_program.bin/new/{report,got.txt,model/expected.txt}.

## Changes

Extended the existing test-dosmsg mechanism; no parallel message linter.
Its third tooth now uses a factory C lexer over command.c, batch.c,
config_sys.c, sysinit.c, dosutil.c, chkdsk.c, xcopy_tree.c, edlin.c,
edlin_program.c, FIND/MORE/SORT and TPSREP NASM. The previous all-MILTON
inline-catalogue check remains over all 34 non-test C files. Adjacent C
literals are decoded together; both C and NASM catalogue identifiers must
exist. External message presence is checked in each program image, or the
ATTRIB/LABEL and XCOPY/TREE image pairs where their shared TU is specialized
and garbage-collected. Header equality still grades against the locked JSON.

The dated exact exceptions distinguish non-diagnostic UI from parser/ABI/FCB
tokens, assembly and internal boot tracing. All literals are inventoried,
including non-output strings, rather than guessed by diagnostic keywords.
dosmsg.allow.baseline freezes the initial 313 exception keys; allow rows may
only shrink, duplicates and unused rows fail. Do not regenerate that baseline
from current sources when adding new text. The two FCB macro arguments at the
end of the initial baseline were discovered by extending NASM coverage beyond
db strings, before this first baseline was finalized.

Catalogue/header coverage is now 74 rows, with 48 additions and matching
ADR Appendix C / citation changes. CD already used MSG_DOS_0018; DIR operands
already worked. command.c changes are local diagnostic substitutions and one
stale diagnostic comment, leaving PATH implementation untouched. NASM consumes
a deterministic generated dos_messages.inc from the same generated C header.
TPS application-specific refusal fragments retain spec/tps_ritual.json and
the independent real-DOS TPS transcripts as their authority; they are not
misrepresented as MS-DOS originals.

Four inherited diagnostics now use exact manual message-directory wording:
batch nesting refusal and SORT OOM use `Insufficient memory` (p.337);
CHKDSK invalid arguments use `Invalid parameter(s)` and FIND's missing quoted
parameter uses `Invalid number of parameters` (p.341). Numeric alias rows
preserve the new extraction inventory without renumbering IDs. Other migrated
strings retain their bytes. ECHO spelling is retained from the existing shell;
p.143 specifies current-setting behavior but does not print that exact sentence.
No new real-DOS 3.3 literal golden was minted for that spelling or the inherited
SET-query extension.

Added host and live DIR membership checks for *.TXT, A*.* and FOO?.* on an
identical mformat image, comparing the actual test-80k matcher with independent
mdir inventories and serial-asserted shell output. mtools 4.0.43 uses host glob
semantics: it excludes FOO.TXT for FOO?.* and extensionless A for A*.*.
The reference adapter explicitly unions FOO?.* with exact-stem FOO.* and uses
A* for the latter query. It does not call the artifact matcher to construct
the reference. Pad-space matching and extensionless A have explicit controls.
Read-only images must remain identical and fsck.fat -n must accept them.

## Red before implementation and final GREEN evidence

Before catalogue substitutions, build/dosmsg_source_red.log reports unlisted
SET, ECHO, date/time and command-specific diagnostics. Both the host and
in-emulator DIR oracles were authored and exercised before implementation;
the existing DIR code passed after accounting explicitly for the mtools glob
difference. Their implementation mutants then proved that the gates bite.

All five new targets are in TEST_UNIT_GATES / TEST_EMU_GATES via dosmsg.mk:

| New target | GREEN tail (build/dosconf_family_final.log) |
| --- | --- |
| test-dosmsg-source-mutant | GREEN dosmsg mutant NASM_DIAGNOSTIC: named assertion RED |
| test-dir-match-host | GREEN dir-match-host: *.TXT / A*.* / FOO?.* == mdir + test-80k matcher; identical image, fsck.fat clean |
| test-dir-match-host-mutant | GREEN test-dir-match-host-mutant: INT21_MUTATE_WILD_QMARK_LITERAL caught RED |
| test-dir-match-emu | GREEN dir-match-emu: *.TXT / A*.* / FOO?.* == mdir + test-80k matcher; identical image, fsck.fat clean |
| test-dir-match-emu-mutant | GREEN test-dir-match-emu-mutant: CMD_MUTATE_DIR_ARGS caught RED |

The same final log records `>>> test-dosmsg: green`,
`>>> test-dosmsg-mutant: green` and test-spec PASS. Extraction reports
706 literals, 148 catalogue references, 313 dated exceptions.

| Mutant | Exact RED assertion (source locations omitted here) |
| --- | --- |
| NEW_DIAGNOSTIC | unlisted literal "Directory name rejected" |
| INLINE_CATALOGUE | inline MSG_DOS_0018 (use macro) |
| SPLIT_CATALOGUE | inline MSG_DOS_0018 (use macro) |
| UNKNOWN_MACRO | unknown macro MSG_DOS_9999 |
| ALLOW_GROWTH | allow-list growth beyond frozen baseline |
| STALE_ALLOW | stale allow row os/milton/command.c "Removed UI" |
| NASM_DIAGNOSTIC | unlisted literal "FIND: broken parameter" |
| REWORD_0002 (existing image mutant) | RED dosmsg-image: REWORD_0002 canonical message absent |
| INT21_MUTATE_WILD_QMARK_LITERAL | RED dir-match: *.TXT host != mdir |
| CMD_MUTATE_DIR_ARGS | RED dir-match: TXT shell membership != mdir/test-80k matcher |

Source mutants live under build/dosmsg_source_mutants; the existing header/image
mutant restores the header and shell image. No shipped TU is edited by the new
mutant script. New/modified source is ASCII; both new shell scripts pass sh -n.

Required regression tails:

| Target | Tail / evidence |
| --- | --- |
| test-unit with only command/batch/wildcard gates | test-unit: ALL GREEN (8 host gates), build/dosconf_command_batch.log |
| test-shell | VERDICT : PASS -- booted InitechDOS, got an A:\> prompt, ran DIR/TYPE/a program/EXIT via COMMAND.COM |
| test-dosutil-emu | PASS: ATTRIB/LABEL/VOL QEMU differential, dialogue, disk and clean return |
| test-chkdsk-emu | VERDICT: PASS CHKDSK emu |
| test-xcopy-tree-emu | XCOPY/TREE EMU GREEN: external flat EXEC; injected /W /P keys; FAT differential; TREE byte-exact; SHELL-EXIT |
| test-xcopy-tree-emu-cancel | XCOPY/TREE CANCEL GREEN: injected Ctrl-C; exit=2; destination unchanged; SHELL-EXIT |
| test-edlin-emu | RED EDLIN-output case=new; stopped here |

Per-target logs are build/dosconf_test-<target suffix>.log. The constrained
test-unit vector was test-command, test-command-mutant, test-batch,
test-batch-mutant, test-batch-exec, test-batch-exec-mutant, test-80k,
test-80k-mutant. Every QEMU gate exported INITECH_QMP_STDIO=1. Checked free -g
and df -h / before heavy runs (30 GiB RAM, about 20 GiB available; more than
120 GiB disk free). No full make test and no Bochs.

Not run after the blocker: test-find-filter, test-more-filter,
test-sort-filter, test-tps-report, make image, test-flair-app-launch,
test-flair-locked-purge, test-flair-finder-service. Those remain PM acceptance
work after the BREAK/EDLIN conflict is dispositioned. Beads were not changed.

## Law-1 references

Rendered the following scanned MS-DOS_3.3_Users_Guide_198707.pdf pages at 110 dpi
with pdftoppm and inspected the PNGs. All printed numbers below are within
the User's Reference section (PDF page = printed page +85 except Guide pages).

* DIR/wildcards: printed pp.10,57 (PDF95,142); host contract also
  os/milton/test_int21_wildcard.c/test-80k and ADR-0003 Appendix A 4Eh/4Fh.
* ECHO status behavior: p.143 (PDF228); EDLIN prompt/report UI: pp.162,164
  (PDF247,249), independent harness/edlin/model.c.
* CHKDSK messages/report/prompt: pp.318-319,323,334 (PDF403-404,408,419).
* Entry error: p.326 (PDF411). Insufficient memory: p.337 (PDF422).
* Invalid volume label/date/arguments/time, missing GOTO label, line too long:
  pp.338-339,341-344 (PDF423-424,426-429).
* LABEL root full and no-path status: p.348 (PDF433).
* Out of environment space and EDLIN O.K. prompt: p.351 (PDF436).
* TPS refusal strings: spec/tps_ritual.json and harness/diff/tps/*_expected.txt,
  with its application golden exercised by the existing test-tps-report gate
  (not rerun in this stopped session).

No claims that a page prints a literal it only describes, or that TPS is an
original MS-DOS utility. Remaining byte-spelling verification for inherited
ECHO/SET extensions is explicitly distinguished above from the structural
catalogue-conformance result.
