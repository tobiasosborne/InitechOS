# WL-0138 -- TPS rituals re-grounded: R2 top-of-form alignment (tacit count), R5 names the form

Lane tpsr2 (Opus coding lane, branch lane/tpsr2), bead initech-5igf. A
DELIBERATE change to locked spec-data (CLAUDE.md Rule 8): spec/tps_ritual.json
rows R2 and R5. R1, R3, R4 unchanged.

## Context

Operator Law-4 eyeball of build/clips/tps_journey.gif, 2026-10-17: the TPS
Report Generator must not tell the operator to hit the space bar or how many
times; that is known by the operator and written in the TPS Report Generator
manual. The research brief docs/research/TPS-REPORT-GENERATOR-PERIOD-GROUNDING.md
(initech-9yw6) laid out period options per ritual; the PM adopted its
recommendations (bd initech-5igf notes, 2026-10-17 09:20; operator may veto).

## What changed

| Item | Before | After |
|---|---|---|
| R2 name | form feed alignment | top of form alignment |
| R2 prompt | `PRESS THE SPACE BAR EXACTLY 7 TIMES TO ALIGN THE FORM FEED,` / `THEN PRESS ENTER.` | `TOP OF FORM ALIGNMENT` / `----- ALIGNMENT TEST LINE ----- FORM TPS-1 (REV. 10/89) -----` / `ADVANCE THE FORM TO THE TEST LINE, THEN PRESS ENTER.` |
| R2 refusal | message, count reset, prompt NOT reprinted | message (unchanged text), count reset, the three prompt lines reprinted |
| R2 behaviour | SPACE x7 + ENTER, period per press, other keys ignored | unchanged; `"count": 7` kept |
| R2 new fields | -- | `"tacit"`: the count and the key are stated only in the TPS Report Generator Operating Procedures (OP-7.4); `"grounding"` |
| R5 prompt | `DID YOU REMEMBER TO ATTACH THE NEW COVER SHEET? (Y/N) ` | `DID YOU REMEMBER TO ATTACH THE NEW COVER SHEET, FORM TPS-1 (REV. 10/89)? (Y/N) ` |
| R5 refusal / exit | TPS-005 ... / return code 5 | unchanged |

Files: spec/tps_ritual.json, os/milton/tpsrep_program.asm (m_r2a/m_r2b/m_r2t,
`jmp ritual` on a refused count, m_r5a; new mutant knob TPS_MUTATE_SAY7),
harness/emu/fcb_tps.mk ([tacit] legs + SAY7 mutant),
harness/fixture_volumes.mk (contract for build/tps_say7.img),
harness/diff/tps/tps_a_expected.txt + tps_c_expected.txt (re-minted).

## Why (citations, all from the research brief's sourced survey)

- R2 option B, the alignment test line + accept loop: IBM 4230 Printer User's
  Guide SA40-0593-04 pp. 1-46, 5-4..5-5 (Top of Form Alignment test, reprinted
  until accepted; `TOP OF FORM SET`); Using dBASE III PLUS p. U5-143 (the sample
  line loop, `Do you want more samples?`). The number of line-spaces is
  training knowledge, as the 4230's numeric steps are (p. 1-46), so it lives in
  the manual (OP-7.4, bead initech-ipu5.14) and nowhere in the program's output.
- R5 option B keeping the canon sentence: a special-forms mount attestation
  (IBM IEC163A/164A mount requests; WordPerfect 5.1 Reference p. 751 `Insert
  Paper-Press "G" to Continue`); the form id is the one already printed on the
  cover sheet (`FORM TPS-1 (REV. 10/89)`).

## Frictions

- The brief names `make fcb-mint-realdos`; the TPS goldens are minted by its
  sibling `make tps-mint-realdos` (same file, same realdos_run.sh). The default
  DOS622_IMG path is relative (`../lotus123-decomp/...`) and does not resolve
  from a worktree; passed DOS622_IMG=/home/tobias/Projects/lotus123-decomp/mint/dos622.img.
- First [tacit/spec] extraction counted the `"prompt"` key as a fourth row;
  fixed (be2e5f3).
- The new R5 prompt is 79 columns; with the echoed answer the line fills all
  80. Byte transcripts are unaffected; on an 80-column screen the cursor wraps
  before the CR LF, so the clip may show one blank line after the answer (real
  DOS behaves the same on its console). Watch for it when re-recording.

## Acceptance (first-person, QEMU)

- Golden: `make tps-mint-realdos` under REAL MS-DOS 6.22 (dosbox-x): legs A and
  C differ from the old golden exactly in the R2/R5 lines; minted twice, the
  second mint reproduces the committed bytes; tpsrept_expected.txt and
  tps_b_expected.txt unchanged. No golden was hand-edited.
- `make test-tps-report`: [spec] 22 literals; [tacit/spec]; [A] 90 lines +
  2432 B == real DOS; [tacit/A] 3 R2 prompts, one per attempt; [B]; [C]. PASS.
- `make test-tps-report-mutant`: COUNT6 RED, seq RED, SAY7 RED on [tacit]
  (`ADVANCE THE FORM 7 LINE-SPACES TO THE TEST LINE, THEN PRESS ENTER.`). The
  static [tacit/spec] lint was separately proven RED on a spec copy stating the
  count (TPS_SPEC/TPSREP_BIN overridden to the SAY7 pair).
- `make test-flair-tps-journey` green (trace unchanged: still 6/8 refused, 7
  accepted, 60 keystrokes == keys_a.txt; 88-line span == real DOS);
  `-mutant` 3/3 RED for the named reason. `make image`, `make
  test-fixture-volumes`, `make test-makefile-vars` green.
- NOT done here: the Rule-14 clip `record-flair SCRIPT=tps_journey` -- the PM
  re-records it after merge. Bochs leg of the journey still owed (as before).

## Pointers

bd initech-5igf (this), initech-9yw6 (research), initech-ipu5.14 (the manual,
which must state the 7 presses of the SPACE BAR in OP-7.4), initech-8479.1.
