# Desktop app file-reader conformance study

Baseline: 09d52a32173911debc13035096135143fefb4b2c, branch
t3/desktop-file-conformance-study, 2026-11-14. Product code is unchanged.
This report is also delivered to the requested phase2/review-app-readers.md.

## Result and evidence boundaries

There are **47 committed byte fixtures and three explicitly pending
later-version inputs (50 case entries)** across the seven components.
Nine initial load captures and four edit/save/reopen follow-ups run actual
Lotus R2.2 or WordPerfect 5.1 under real DOS 3.30. Five new defects were filed, plus evidence on existing
initech-pmw9. The two data-loss findings are InitechText's short read/save
and the existing 240-character label edit loss.

These are a study and handoff, not 50 passing tests. Each cases.tsv separates
product observations from a captured reference or a stated property.
OBSERVED.txt is not expected output. Uncaptured original-software behavior,
missing later-version files, and unexecuted boot/save phases stay pending.
The new material is not wired into make test; owners can add red-first
regressions without making an unknown oracle look green.

All product observations use real host-compiled sources. The Lotus and Word
drivers call the same Retrieve and Save helpers used by their faces; they
also do a direct model edit. Text and accessories run against real TextEdit
and SDK file storage. Calc records a requested copy only. Control Panels
and Desktop DB observers deliberately exercise only their decoders/header
validator. No claim about their booted disk paths follows from those probes.
Host processes finished within the 30-second observation timeout; this is
not a booted cooperative-scheduling proof.

## What was read

Read CLAUDE.md, GOLDEN-MASTER.md, STATUS.md and BRIEF-COMMON first, then all
seven component READMEs and Makefiles. Relevant harnesses include
Initech 123 tests/run.sh and diff/test_i123_wk1_roundtrip.c; InitechWord
tests/run.sh, face_driver.h and diff/test_iword_wp5_roundtrip.c; Text's
test_itext_route.c; accessories' test_notepad_route.c; Calc's
test_calc_file_copy.c; Control Panels test_route.c/test_cpanels.c; and the
Finder desktop persistence tests.

Loader/writer review covered apps/initech123/fs/wk1.c and app/face_a.c;
apps/initechword/fs/wp5.c, core/codes.c and app/face_a.c;
apps/initechtext/itext.c; apps/notepad/notepad.c and store.json;
apps/initechcalc/calc.c and shipped ledger; Control Panels cpanels.c,
core.c, record.json and the real preference loader flair/cp_service.h;
flair/desktop/desktop_db.c/.h and finder_desktop.c/.h.

Reference authorities: goldens/README.md, CORPORA.lock,
goldens/dataloss/{README.md,runtime.sh,drive.c,lotus.sh,wordperfect.sh};
the pinned lotus123-decomp specs/file-formats/wk1-format.md and real
MINT01.WK1; the pinned wordperfect51-decomp WP5 header/body specifications
and real MINT02.WP5. Small-app expectations are the study's explicit
completion/conservation property and the authored store/record contracts,
not inferred Mac file formats. Existing data-loss captures were read but
gj7z, uyi1, 1tuwf and vtl9 are not refiled or fixed.

## User journeys and case matrix

Fixtures live in each component's tests/conformance/fixtures. Each directory
has cases.tsv, generation source, SHA256SUMS, a host observer, run.sh,
OBSERVED.txt and a README with exact steps and limits.

Common journey: open/retrieve on a disposable copy, inspect first and last
data and any notice, save unchanged, edit one value, save to a second name,
close/reopen, stop the emulator and extract bytes independently. A refusal
stops scripted typing; verify the prior document and file remain intact.
For every row below, only the phases explicitly reported have been observed.

Lotus: /File Retrieve CASE.WK1, inspect A1 and the boundary address, save,
edit A1=73/save. Additional steps: L07 F5 IV2048; L08 F5 IV8; L09 F5 A9;
L10 F2/Return without editing; L11 /Range Name Table at A10; L12 F9.
The host observer does not execute the boundary navigation, Name Table
or F9 steps; those remain pending. The requested 2048-row coordinate is
not conflated with a 2048-populated-cell limit: the app documents an
8192-row, 256-column address space, 2048 populated cells, a 32KiB byte pool
and 4096-byte carry area.

Word: Shift-F10 Retrieve on a clear document, inspect top, Home/Home/Down
and Alt-F3 Reveal Codes; save unchanged, insert Z at start, F10 Save to
AFTER.WP5, close/reopen. Reference captures establish initial readable content/refusal. W06 and W11
also have independently checked edit/save and fresh-runtime reopen evidence;
the remaining cases do not establish all later navigation or saved-body fidelity.

### apps/initech123

| Case | Input | Product observation | Independent result / property |
|---|---|---|---|
| L01 | MINT01 unchanged | retrieve 8 cells; unchanged save 1609 bytes, identical | existing real R2.2 MINT01 capture; edit/save replay pending |
| L02 | zero-byte worksheet | refuse | needs capture |
| L03 | 5-byte BOF fragment | refuse | needs capture |
| L04 | MINT01 cut inside final cell record | refuse | needs capture |
| L05 | MINT01 missing EOF | refuse: Not a valid WK1 file | real R2.2: Part of file is missing; captured |
| L06 | MINT01 plus DOS EOF byte 1A | refuse | needs capture |
| L07 | IV2048=42, sparse extent | open one cell; byte-identical save | needs capture; requested coordinate boundary |
| L08 | 2048 populated INTEGER cells A1:IV8 | open 2048; byte-identical save | real R2.2 opens, A1=42; captured |
| L09 | 2049 populated INTEGER cells | refuse: Not enough worksheet memory | needs capture; authored populated-cell capacity, not format limit |
| L10 | 240-character label A1 | open/save identical; F2+Return shortens 240 text chars to 239 | real R2.2 opens label; captured; full-length edit/save pending |
| L11 | 128 NNAME records | open; 3722-byte identical save | needs capture; range table and rebinding pending |
| L12 | formula with 128 PAREN tokens | open; 161-byte identical save | real R2.2 refuses: Formula too long; captured |
| L13 | real R3 WK3 renamed WK1 | PENDING input and observation | needs R3 runtime/native sample; recipe below |

### apps/initechword

| Case | Input | Product observation | Independent result / property |
|---|---|---|---|
| W01 | real MINT02.WP5 | open body 17; save 344; cursor-prefix byte changes | existing real WP5.1 capture; no-op save cursor replay pending |
| W02 | zero bytes | refuse: Incompatible File Format | needs capture |
| W03 | 15-byte header fragment | refuse: Incompatible File Format | needs capture |
| W04 | 327-byte prefix cut at byte 326 | refuse: Incompatible File Format | needs capture |
| W05 | MINT02 loses final text period | open body 16; save 343 | real WP5.1 opens Plain text only; captured |
| W06 | Readable plus incomplete Bold-on code | refuse: Incompatible File Format | real WP5.1 recovers Readable; appends Z, saves a complete 582-byte file with text ReadableZ and reopens |
| W07 | Bold-on code with wrong mirror | refuse: Incompatible File Format | needs capture |
| W08 | 65000 plain body bytes | open; save 65327; insert/save works | needs capture |
| W09 | 1000 Bold-on/b/Bold-off/HRt runs | open body 8000; save 8327; insert/save works | real WP5.1 opens b lines; captured; tail/code inventory pending |
| W10 | total file length 65536 | open; unchanged save works; insertion refuses | real WP5.1 opens x lines; captured; edit/save pending |
| W11 | total file length 65537 | refuse: Document too large | real WP5.1 preserves all 65210 source x characters, appends Z, saves a complete 65784-byte file and reopens |
| W12 | genuine WP5.0 document | PENDING input and observation | needs WP5.0 runtime/sample; recipe below |
| W13 | genuine WP6.0 document | PENDING input and observation | needs WP6.0 runtime/sample; recipe below |

### apps/initechtext

| Case | Input | Product observation | Independent result / property |
|---|---|---|---|
| T01 | CRLF, LF blank line, CR | open 21 normalized bytes; Save=21 | property: exact TEXT, CRLF/LF normalize to CR; all line breaks retained |
| T02 | 4096 printable x bytes | open/save 4096; excess insert refused visibly | property: exact capacity or visible refusal; no unread suffix discarded |
| T03 | 4097 printable x bytes | visible too-large refusal | property: preserve old document/source on refusal |
| T04 | 20-byte readable text, short reads of 4 | normal open/save=20; short read silently open/save=4: RED | property: complete readable bytes or visible safe recovery; no suffix loss |
| T05 | 4097 bytes: 2048 CRLF pairs plus z | visible too-large refusal | property: refusal is permitted; preserves source (normalization does not excuse silent loss) |

### apps/notepad

| Case | Input | Product observation | Independent result / property |
|---|---|---|---|
| N01 | 8 full 1024-byte pages, last current | open page 8, edit first byte/save | property: preserve all other pages; exact fixed store contract |
| N02 | store cut one byte short | empty pages; damaged original retained on edit/save | property: finish; refuse/notice; do not replace damaged source |
| N03 | bad FNV checksum | empty pages; damaged original retained | property: finish; preserve original |
| N04 | page length 1025 with valid checksum | empty pages; damaged original retained | property: length validation independent of checksum; preserve original |
| S01 | 32 full 1024-byte clips, last current | open clip 32; clear last/paste Z/save | property: preserve other clips and valid framing |
| S02 | store cut one byte short | empty scrapbook; damaged original retained | property: finish; preserve original |
| S03 | bad FNV checksum | empty scrapbook; damaged original retained | property: finish; preserve original |
| S04 | clip length 1025, valid checksum | empty scrapbook; damaged original retained | property: enforce length even when checksum agrees |

### apps/initechcalc

| Case | Input | Product observation | Independent result / property |
|---|---|---|---|
| C01 | shipped 150-byte ledger | launch built-in model; Save requests source 150 bytes | property: saved model agrees with visible content; host does not execute FILE COPY |
| C02 | ledger cut after 23 bytes | same built-in display; Save requests source 23 bytes | property: never replace a good destination with damage; booted copy/readback pending |
| C03 | 8192-byte ledger source | same display; Save requests source 8192 bytes | property: complete/refuse; actual copy/readback pending; no import command |

### apps/controlpanels

| Case | Input | Product observation | Independent result / property |
|---|---|---|---|
| P01 | literal valid 16-byte settings | decoder accepts pattern 3 | property: valid settings decode/apply; boot and save pending |
| P02 | 15-byte settings | disk loader PENDING (decoder has no length arg) | property: reject wrong length/defaults, keep original until explicit valid save |
| P03 | future version 2, valid checksum | decoder refuses; defaults retained | property: reject unknown version; do not silently replay/repair |

### flair/desktop

| Case | Input | Product observation | Independent result / property |
|---|---|---|---|
| D01 | empty 8-byte DB | header validator accepts | property: bootstrap completes; boot/read/write preservation pending |
| D02 | 7-byte DB header | validator rejects length | property: bounded refusal/regeneration with notice; no damaged commit |
| D03 | one record, last byte missing | validator rejects size | property: do not read partial record; boot regeneration pending |
| D04 | 170 records, 4088 bytes | structural validator accepts | property: header size bound only; Finder semantic capacity/record validation pending |
| D05 | 171 records, 4112 bytes | structural validator accepts | property: bootstrap rejects >4096; not an observed validator defect (caller checks bound) |

For accessories, install N cases as NOTEPAD.DAT and S cases as SCRAPBK.DAT,
launch and inspect the selected final page/clip. N01 replaces its first byte
with Z; S01 clears clip 32 and pastes Z. Corrupt cases attempt an edit/save;
all six damaged originals remain byte-identical. Full valid-store contents
after editing and UI turn/relaunch remain follow-up work.

Calc has no ledger Open command. Install a case as APPS/LEDGER.TXT and a
known good APPS/TABLES.TXT; launch, inspect, File Save/confirm replacement,
then edit A1=73/save. Launch always paints its built-in model. The observer
only proves that an unedited save requests a copy of the altered source;
it does not prove a damaged TABLES.TXT was written. C02/C03 need a booted
readback before a new data-loss finding is justified.

Control Panels receives SETTINGS GET from FLAIR, not file bytes. For P
cases install CPANELS.DAT before boot, inspect defaults/notice, Cancel and
compare original bytes; reopen, choose Stipple/OK, reboot and extract.
The SDK fake does not do boot loading; P02 remains pending. The actual boot
loader reads 17 bytes and checks exact length; do not pad 15 bytes and
pretend cp_decode tested the real loader.

For D cases install DESKTOP.DB, boot, observe regeneration and shell
responsiveness, move an icon, exit/restart FLAIR and read back. D04/D05
zero-filled records are structural stress, not valid icon metadata.
desktop_db_validate has no maximum-size check, but bootstrap and write do:
D05 helper acceptance alone is not a defect. Finder has an additional
semantic count/type path. Existing failed DB save loss is initech-yeznl.

## Defects and tracker evidence

| Severity | bd | Source | Case and both behaviors |
|---|---|---|---|
| P0 data loss | initech-gxb4z (new) | apps/initechtext/itext.c:308; publication at :330; save at :350 | T04, 20 readable bytes, SDK legal read limit 4: opens four bytes with no alert, Save succeeds and replaces all 20 with four. Normal-read control preserves 20. Property: finish reading or visibly refuse/recover without authorizing suffix destruction. |
| P0 data loss in this rubric | initech-pmw9 (existing; evidence comment) | apps/initech123/app/face_a.c:1072 (i123_cell_content into the entry buffer); finish at :955 | L10 loads/saves all 240 characters; F2/Return without typing leaves 239. Saved F2 derivative is independently readable. Existing issue records real R2.2 retaining all 240; this study independently captures successful real load but does not claim a new real no-op edit/save capture. Existing issue priority metadata left unchanged. |
| P1 visible diagnostic | initech-bcngg (new) | apps/initech123/app/face_a.c:666 | L05: Initech says Not a valid WK1 file; real R2.2 says Part of file is missing. Both refuse at the captured point; no partial-recovery policy inferred. |
| P1 visible compatibility | initech-un7g8 (new) | apps/initech123/fs/wk1.c:139 | L12: Initech loads/saves 132-byte formula RPN; real R2.2 refuses Formula too long. Capture neighboring sizes before selecting a production bound. This is an over-limit/corrupt compatibility input, not evidence that 128 groups are inside Lotus's entry limit. |
| P1 visible compatibility | initech-xn9zw (new) | apps/initechword/fs/wp5.c:56; app/face_a.c:1265 | W06: Initech refuses the incomplete Bold code as Incompatible File Format. Real WP5.1 recovers Readable, appends Z, saves and reopens ReadableZ; independent full text and zero trailer errors verified. |
| P1 explicit capacity gap | initech-2d41v (new) | apps/initechword/app/face_a.c:253, :1263 | W11: Initech refuses total file size 65537 as Document too large; real WP5.1 preserves all 65210 source x characters, appends Z, saves 65211 text characters with zero trailer errors and reopens. Full WP parity is Release 2; owner/operator must decide whether this documented safe refusal stays a frozen omission. |

No new product fixes, no issue closures and no duplicated excluded issues.
The WP truncated-code issue's precise failure line is fs/wp5.c:56 (the
original filing describes the nearby 53-54 boundary too broadly).

## Captures and replay

Nine initial captures: Lotus L05,L08,L10,L12; Word W05,W06,W09,W10,W11.
Four follow-up sessions are W06-save/W06-reopen/W11-save/W11-reopen.
Each retains exact input.steps, raw 4000-byte VGA, 80x25 transcription,
lossless unretouched PNG, gzip-n monitor log and post-load data floppy.
CAPTURES.sha256 pins them. The load-only floppy contains the exact case bytes; mcopy extraction was
compared with the committed fixture after QEMU stopped. Save captures retain
the actual AFTER.WP5, its independent reader dump and the changed data disk.
Fresh-runtime reopen captures load those saved bytes. W06's exact ReadableZ
and W11's exact 65210 x characters plus Z were checked in the dumps;
both readers reach filesize with zero trailer errors.
No application/system binary or runtime image is committed.

Runtime/provenance is exactly goldens/dataloss/README.md: pinned initialized
Lotus R2.2 and WP5.1 03-09-92 under Packard Bell DOS3.30, QEMU 8.2.2,
486, 8MiB, no network, fixed RTC base. Capture runners use the existing
independent goldens/dataloss/drive.c. Run one reference emulator at a time:

    sh goldens/dataloss/runtime.sh ABS_RUNTIME
    sh apps/initech123/tests/conformance/capture.sh ABS_RUNTIME ABS_CASE.WK1 ABS_NEW_OUT
    sh apps/initechword/tests/conformance/capture.sh ABS_RUNTIME ABS_CASE.WP5 ABS_NEW_OUT

Lotus runner installs CASE in its disposable C:\\LOTUS to avoid appending a
drive name to the prefilled directory. No file-not-found capture from that
early setup error is retained as evidence. W06/W11 initially sampled too
early at 4s; the retained captures wait for the Retrieve prompt and sample
after 20s. A prompt-only early screen is neither a refusal nor a freeze.

## Pending work and exact recipes

The component READMEs/capture.steps supply concrete replay instructions for
every unobserved phase. Remaining work is tracked as initech-stc9m.

* Lotus load captures pending: L02-L04,L06-L07,L09,L11; for successful loads,
  F5 the specified boundary, inspect names, F9 formulas, then unchanged and
  edited Save/reopen. L10 needs a fresh no-op F2/Return saved-byte reference.
* WP load captures pending: W02-W04,W07-W08. W05/W09/W10 still need end
  navigation, Reveal Codes, edit/Save/reopen and independent full-body/trailer
  inventory. W06/W11 full saved text and ordinary reopen are now checked;
  general corrupt-code recovery and every prefix packet variant are not.
* L13: obtain a genuine R3 runtime/sample; fresh R3 sheet A1=123,
  B2='LATER, native File Save LATER.WK3. Stop/extract/hash; rename a COPY
  CASE.WK1 without byte changes. Run R2.2 and Initech Retrieve, record any
  conversion prompt before editing/saving/reopening.
* W12/W13: obtain genuine WP5.0/WP6.0 runtime/sample. Type VERSION TEST,
  Return, F6 BOLD F6; F10 save in each version's native format. Do not choose
  WP5.1 export in WP6. Stop/extract/hash and rename a copy CASE.WP5; retrieve
  in the pinned real WP5.1 and InitechWord, capture prompts/codes and
  edit/Save/reopen. No such native files/runtimes were found locally.
  Altering a header byte is not substituted for these three inputs.
* Booted Calc copy/model conservation, Control Panels boot/defaults/Cancel/
  Save, Desktop DB regeneration/write/Finder semantic records, and accessory
  valid-edit full-store/relaunch checks remain with their owners. Those
  probes cannot be honestly replaced by a fake that never runs those paths.

## Validation

The fixture/capture foundation is commit 4fb96275b7e2739ba8c113d2206c5f7512f55323;
this report commit adds four WP follow-ups and validation evidence.

The complete default command, nice -n 15 make -j2 test, finished all 19
registered components in 1104s and returned 2: five components RED. Kernel
timed out test-qekc-oracle and test-zs24 at 20s and exceeded its suite
budget (411s); Initech 123 had zero assertion failures but exceeded 30s
(67s); Word timed out test_iword_codes and exceeded its budget (54s);
DOS had zero assertion failures but exceeded its 30s budget (74s);
Turbo compiler reported self-protocols/self-generation failures and exceeded
its budget (73s). No full-gate green is claimed and no unrelated code was
changed. The default log is validation/default-top-test.log.gz.

The six app suites were rerun with the BRIEF-COMMON permitted command-line
limits, without editing any budget:

    nice -n 15 make -j2 test COMPONENTS='apps/initech123 apps/initechword apps/initechtext apps/notepad apps/initechcalc apps/controlpanels' TEST_TIMEOUT_S=120 TEST_BUDGET_S=900
    nice -n 15 make -j2 -C flair/desktop test TEST_TIMEOUT_S=120 TEST_BUDGET_S=900

All six passed: 19 Lotus, 13 Word, 1 Text, 2 accessory, 2 Calc and 2 Control
Panels programs; top-level summary explicitly says SUBSET GREEN (6 of 19),
71s. Finder separately passed all 17 programs (2s); it also passed at
default limits in the full run (3s). Logs are validation/components-test.log.gz
and desktop-test.log.gz. These seven component results do not certify the
unrelated kernel, DOS or Turbo failures.

All 47 inputs regenerate byte-for-byte using the committed C generators and
independent baseline inputs. Each component run verifies its input hashes.
A disposable T04 byte mutation is rejected by SHA256SUMS (T04.TXT: FAILED);
the original fixture is unchanged. All 13 retained capture sessions verify against their component manifests.

The Text study deliberately returns 1 on the original product:
T04 normal limit=0 -> loaded=1 shown=20 save=1 saved=20 property=GREEN;
limit=4 -> loaded=1 shown=4 alerts=0 save=1 saved=4 property=RED.
No green fixed-product claim is made. The captured Lotus/WP differences
and the existing label loss are observations for owners to turn into
mechanical red-first assertions.

No kernel, Toolbox, interface, smoke driver or product source changed, so no
product smoke is claimed. All work stayed in component test directories;
STATUS.md, shared build files, corpora locks and product code were not edited.
The orchestrator owns landing/status/push per BRIEF-COMMON.
