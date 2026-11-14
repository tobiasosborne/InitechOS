# InitechDOS / MS-DOS 3.30 compatibility study

2026-11-14. Branch `t3/dos330-compatibility-study`, reviewed product base
`09d52a32173911debc13035096135143fefb4b2c`. Test fixtures and evidence only;
no product code, shared factory/build files, STATUS.md, or reference media changed.

## Result and authority

18 new defects were filed: one P0 and 17 P1. Existing expansion, SHIFT,
COPY-full-disk, and physical-line issues receive evidence rather than duplicate
issues. Remaining capture work is `initech-tnesd`; this is not a parity certificate.

The oracle is the pinned Packard Bell OEM MS-DOS 3.30 disk already used by
`goldens/dataloss/dos-copy.sh` and `command/tests/batch_break_mint/`.
`expected/media.sha256`, `command.sha256`, and every captured VERSION.TXT identify
it. Fresh captures run genuine COMMAND.COM in QEMU on a 486 with 8 MiB, using
`goldens/dataloss/drive.c` and mtools disk extraction. The ordinary product image
runs on a 486 with 8 MiB, ACPI off, through `factory/emu/qemu_harness`; its hashes
are in `observed/product.sha256`. No Bochs compatibility differential was run.

An important correction to the brief: **this genuine DOS 3.30 COMMAND.COM supports
CALL**. BATCH.BAT prints CHILD-RAN, AFTER-CALL, CHILD-RAN, AFTER-COMMAND under both
systems. The existing hash-pinned batch-break captures also use CALL. Removing
CALL based on the parenthetical in the brief would contradict these captures.

All paths below are relative to this directory unless prefixed by a component.
`expected/<case>/` contains actual DOS captures; `observed/<case>/` contains actual
product results. Exact CRLF inputs are in each compressed data disk, with the
boot AUTOEXEC/CONFIG retained separately. `generate.c` supplies portable recipes.
VGA bytes plus decoded text preserve messages which real DOS writes to CON even
when stdout is redirected. Directory inventories and disk images establish file
existence independently of the shell transcript. Host-created dates, inventory
size/free bytes and reference B: versus product A: are environmental differences;
they are not defects. The report does not normalize meaningful formatting or
%0's added drive/path.

## Case table: shell and batch

| Case / input | Genuine MS-DOS 3.30 | InitechDOS | Disposition |
|---|---|---|---|
| LIMIT: ECHO physical lines of 125 through 132 characters | Outputs 120,121,122,122,122,122,122,122 x characters; full physical line is consumed | Exact captured transcript match | Matching 127-character physical-line boundary |
| LIMIT: 512-character REM followed by a sentinel | REM is skipped; sentinel executes | Exact match | Matching control |
| TCASE: command tails requested at 126/127/128 characters through an overlong physical line | TAIL.COM receives length 119, including the leading space: the *whole command line* already hit its boundary | Exact binary trace match | Matching shell boundary; this does **not** prove raw EXEC tails of 126/127/128 bytes |
| PARAMS: repeat three 26-character positional arguments until expanded command exceeds 127 | Executes a shortened first ECHO, then SHIFT and the next ECHO; captured prefix ends `-abcd` | Silently drops the first ECHO, then executes SHIFT/next ECHO | P1, existing `initech-xfg8`; `command.c:4562` continues on expansion overflow |
| EXPAND: set an 80-character V, then ECHO %V%%V% | Separate reference run did not complete; final VGA says Bad command or file name; trace remains empty | Drops the expanded ECHO silently; following sentinels execute | Reference **pending**; retained as EXPAND-PENDING, not an expected golden |
| BATCH: %0, %1, %2, %9 | `[BATCH.BAT][one][two][nine]` | `[A:\BATCH.BAT][one][two][nine]` | P1, `initech-lb371` |
| BATCH: SHIFT with a tenth supplied argument | `%0=one`, `%1=two`, `%2=three`, `%9=ten` | First three match; `%9` is empty | P1, existing `initech-tyvt` (tracker still has its earlier P3/docs classification) |
| BATCH: unset ABSENT then ECHO [%ABSENT%] | `MISSING=[]` | `MISSING=[%ABSENT%]` | P1, existing `initech-xfg8` |
| BATCH: %% and a trailing unmatched percent | `PERCENT=[%][trailing]` | `PERCENT=[%][trailing%]` | P1, existing `initech-xfg8` |
| BATCH: IF x==X / IF NOT x==X | Case-sensitive comparison; CASE-OK only | Same | Matching control |
| BATCH: IF EXIST *.TXT | Prints WILDCARD-EXISTS | False despite matching files | P1, `initech-upa66` |
| BATCH: IF EXIST SUB for a bare directory | False | False | Matching control; do not invent a directory-exists expectation |
| BATCH: FOR list `(one,two;three=four)` | Four members | Two members: `one`, `two;three=four` | P1, `initech-etdo6` |
| BATCH: FOR unmatched NO*.ZZZ | No iterations | Same | Matching control |
| BATCH: FOR `(A.TXT B.TXT)` | Both members execute | Same | Matching control |
| BATCH: FOR `(*.TXT literal)` | Matching files, then literal member | Matching files; literal omitted | P1, `initech-etdo6` |
| BATCH: `IF nonsense ECHO BAD-IF`, `FOR broken` | Two Syntax error messages on CON | Both silently skipped | P1, `initech-f6ygt` |
| BATCH: GOTO :COLON | Branches to the colon-prefixed target | Same | Matching control |
| BATCH: GOTO missing | Label not found on CON; ends this batch | Same control flow; message goes to redirected stdout | Stream difference, `initech-pnjw6` |
| NEST: AUTOEXEC invokes COMMAND /C BATCH.BAT, then ECHO CAPTURE-DONE | Child returns; parent marker executes | Shared batch buffer overwritten; Bad command or file name, then prompt, no parent marker | P1, `initech-if6qo`; fresh 8 MiB reproduction retained |
| Ctrl-C during nested PAUSE / EXEC child | Existing `batch_break_mint` Y cancels every active frame, N resumes, invalid answer asks again; EXEC child dies and question uses CON | Existing batch-break host gate passes those cases | Existing genuine captures, not freshly re-minted here; redirected child's ^C stream difference is explicitly excluded by that older gate |
| BREAK: PAUSE, QEMU `sendkey ctrl-pause`, then y | ^C and Terminate batch job (Y/N)?; y cancels, no AFTER.OUT | Chord ignored; y releases PAUSE, creates AFTER.OUT and runs the completion marker | P1, `initech-w7y7u` |
| ENV: twelve 78-character SET values, default secondary environment; delete V00 then SET NEW=ok | Out of environment space on CON; V00 complete, V01 partially stored (39 x); NEW works after deletion | Five complete values, then refusal messages in stdout; failed additions leave no partial variable; NEW works | Captured characterization; allocation/default seeds differ. Identical-capacity SET mutation needs a separate capture |
| CFG2: root SHELL=/E:1024, twelve SET values in direct AUTOEXEC | V00 through V11 all complete | Still only V00 through V04 | P1, `initech-vx2ko` |

## Case table: switches, names, files and streams

| Case / input | Genuine DOS 3.30 | InitechDOS | Disposition |
|---|---|---|---|
| SWITCH: DIR/W A.TXT | Adjacent switch accepted; 8.3 fields rendered separately | Adjacent switch accepted; dotted name and different framing | Recognition matches; layout P1 `initech-hcoi3` |
| SWITCH: COPY A.TXT CP.TXT/V | Copies seven bytes | Same seven bytes and success count | Matching control; this does not inject a verify-read failure |
| SWITCH: DIR /Z | Invalid parameter on CON | Too many parameters in stdout | P1 `initech-pnjw6` |
| SWITCH: COPY A.TXT BAD.TXT/Z | Invalid parameter; no destination | Too many parameters; no destination | P1 diagnostic/stream, `initech-pnjw6` |
| SWITCH: DEL DEL.TXT /Z | Invalid parameter; DEL.TXT remains seven bytes | Deletes DEL.TXT | **P0 `initech-2vkuq`** |
| SWITCH: REN B.TXT R.TXT /Z | Invalid parameter; B.TXT remains | Renames B.TXT to R.TXT | P1 `initech-y5t9p` |
| SWITCH: CD /Z | Invalid parameter | Invalid directory | P1 `initech-pnjw6` |
| NAMES: COPY A.TXT LONGFILENAME.LONG | Creates LONGFILE.LON, seven unchanged bytes | Access denied; no destination | P1 `initech-mzll9` |
| NAMES: DIR A?.TXT | Selects A.TXT | Same selection | Matching wildcard selection; layout differs |
| NAMES: redirect to NUL.TXT, SUB\NUL.TXT, SUB\..\NUL | Device, no disk entries | Same | Matching controls |
| DEVICES: OPEN/IOCTL/CLOSE CON, SUB\CON.TXT, SUB\PRN, SUB\NUL, SUB\AUX | All devices | All devices | Matching; no device read/write performed |
| DEVICES: MISSING\NUL and Z:\NUL | Both refused | Both resolve to NUL device | P1 `initech-905b8` |
| DEVICES: CON: | Refused | Refused | Matching control |
| NAMES: CD SUB, CD ., CD .. | Ordinary directory navigation | Same current directories | Matching controls |
| NAMES: CD.. | Executes directory command | Bad command or file name | P1 `initech-e70f4` |
| NAMES: Z: with no such drive | Invalid drive specification on CON | Same message, stdout | Stream P1 `initech-pnjw6` |
| REDIR: ECHO abc>R.OUT; ECHO def>>R.OUT | `abc\r\ndef\r\n` | Byte-identical | Matching output/append control |
| STREAM: PASS <A.TXT >IN.OUT | `alpha\r\n` | Byte-identical | Matching real child input-redirection control |
| STREAM: TYPE A.TXT\|PASS >PIPE.OUT | `alpha\r\n` | Byte-identical | Matching real producer/consumer pipe control |
| REDIR: TYPE<R.OUT with no filename | Invalid number of parameters on CON | Required parameter missing in stdout | P1 `initech-pnjw6`; TYPE is not an stdin filter |
| REDIR: BADNAME>BAD.OUT | Error on CON; BAD.OUT absent | BAD.OUT created with 26 error bytes | P1 `initech-tm15k` |
| REDIR: pre-existing PIPE1.$$$, ECHO hello\|DIR >PIPES.OUT | Stranger preserved; two generated extensionless hex names visible during pipeline; owned temps deleted afterward | Stranger preserved; PIPE1001.$$$ visible during pipeline; owned temp deleted afterward | P1 namespace `initech-6rmfn`; cleanup controls match |
| REDIR: SET PP=parent; SET PP=child\|REM; ECHO %PP% | child | child | Matching shared environment; do not assume modern separate pipeline shells |
| FULL: TYPE BIG.TXT\|ECHO CONSUMER-RAN, with 4096 bytes initially free | Intermediate file error during pipe on CON; consumer skipped; FULL-DONE executes; temps removed; 3584 bytes free afterward | Insufficient disk space on CON; same continuation, cleanup and free-space result | Diagnostic P1, evidence added to existing `initech-u6x8` |
| Known COPY onto a full disk | Existing genuine captures delete failed destination, including an overwritten old destination; exact 0 File(s) copied message | Existing issue identifies retained partial destination and /A diagnostic mismatch | Skipped as requested; evidence `goldens/dataloss/dos/` and `initech-u6x8` |
| Known truncation changes `DEL ` + 122 spaces + `*.TMP` | Non-destructive LIMIT confirms the same 127-character truncation boundary; destructive mask not re-run | Source still executes the shortened mask | Existing `initech-dt7i6`; do not assert a reference-safe deletion mask without its exact capture |

## CONFIG.SYS: bad values and application

The frozen baseline/parser lists FILES, BUFFERS, LASTDRIVE, DEVICE, INSTALL,
SHELL and BREAK; the component README also admits deferred handling. INSTALL
is not accepted by this genuine DOS 3.30 processor. Its presence in a later
DOS manual is not authority for silently accepting it here.

| Input / case | Genuine DOS capture | Product observation |
|---|---|---|
| FILES=banana / CFGA | Unrecognized command in CONFIG.SYS | Parser returns zero recognized, no user diagnostic |
| BUFFERS=-1 / CFGB | Same period refusal | Same silent nonrecognition |
| LASTDRIVE=9 / CFGC | Same period refusal | Same silent nonrecognition |
| BREAK=MAYBE / CFGD | Same period refusal | Same silent nonrecognition, BREAK defaults ON |
| DEVICE=MISSING.SYS /Z / CFGE | Bad or missing MISSING.SYS | Name retained, switch discarded; serial says accepted(deferred) |
| INSTALL=MISSING.EXE /Z / CFGF | Unrecognized command in CONFIG.SYS | Name retained; serial says accepted(deferred) |
| SHELL=MISSING.COM /P / CFG3 | Bad or missing Command Interpreter; no AUTOEXEC / completion | Value logged; normal interpreter and AUTOEXEC run |
| SHELL=COMMAND.COM /P /E:1024 / CFG2 | Root environment holds all twelve values | /E ignored; fixed 512-byte store |
| DEVICE=ANSI.SYS /Z, BREAK=OFF, FILES=20, BUFFERS=20, LASTDRIVE=Z / CFG4 | Boots without an error in this capture | ANSI enabled even though no ANSI.SYS file is required; BREAK OFF applied, BUFFERS deferred, LASTDRIVE applied; memory/cache effects not measured |
| CFG1 aggregate also has FILES=-1,0,256,12junk; BUFFERS=banana,0,100,12junk; LASTDRIVE=Zjunk; BOGUS=1 | Multiple period refusals in raw VGA, but older lines scroll off; exact attribution of every numeric boundary is **pending** | `observed/config-parser.txt`: negative/text absent; FILES 0 clamps to 8, 256 to 255, 12junk to 12; BUFFERS 0 clamps to 1, 100 to 99, 12junk to 12; LASTDRIVE Zjunk becomes Z; unknown line absent |

Malformed/deferred behavior is P1 `initech-hp0tb`; ignoring SHELL and its
options is P1 `initech-vx2ko`. Pure parser observations are **not** proof that
FILES/BUFFERS/last-drive effects work at boot. The report does not infer an
exact DOS clamp from the product's old host expectations. CONFIG over 1024
bytes and partial directives need their own reference capture.

## External utilities

`U01` through `U15` are concrete generated batch fixtures: `<utility> /Z <NUL
>UTIL.OUT`. Each runs on a separate disposable image. Reference U04/U08/U09/U10/
U13 genuinely execute the utilities on the pinned disk. Other missing utilities
are explicitly refused by the capture recipe as PENDING (exit 2).

| Utility / case | Product actual behavior | Genuine DOS 3.30 / state |
|---|---|---|
| MORE / U01 | Silently ignores /Z; empty input gives empty output | Needs supplementary media |
| FIND / U02 | Invalid number of parameters | Needs supplementary media; invalid-switch-with-quoted-string is in FILTER |
| SORT / U03 | Silently ignores /Z; empty output | Needs supplementary media |
| CHKDSK / U04 | Invalid parameter(s); stops | `/z Invalid parameter` then disk/memory report; P1 `initech-opz31` |
| EDLIN / U05 | Entry error | Needs supplementary media |
| FDISK / U06 | Opens interactive Disk Options despite /Z; no completion in the 5-second observation | Needs supplementary media; not evidence of an infinite loop |
| FORMAT / U07 | Invalid parameter | Needs supplementary media |
| SYS / U08 | Invalid parameter CRLF | Byte-identical reference control |
| DISKCOPY / U09 | Invalid parameter CRLF | Additional leading blank line, filename refusal, syntax, BEL; P1 `initech-opz31` |
| DISKCOMP / U10 | Invalid parameter CRLF | Additional leading blank line, filename refusal, syntax, BEL; P1 `initech-opz31` |
| XCOPY / U11 | Invalid parameter(s) | Needs supplementary media |
| TREE / U12 | Invalid parameter(s) | Needs supplementary media |
| ATTRIB / U13 | Invalid number of parameters in stdout | Syntax error on CON, stdout empty; P1 `initech-opz31` |
| LABEL / U14 | Invalid characters in volume label | Needs supplementary media |
| TPSREP / U15 | Input fixture provided; not run in this comparison | Initech-specific extension, no MS-DOS counterpart |

FILTER uses an 18-byte file whose final line has no newline, a 619-byte file
with a 606-character first line and unterminated final line, and BIG.TXT with
123000 bytes / 3000 lines. The completed 8 MiB product run gives:

| Input | Product actual bytes | Reference state |
|---|---|---|
| SORT, FIND on NOEND.TXT | Each adds final CRLF: 20 bytes | Needs genuine utilities |
| MORE on NOEND.TXT | Exact 18-byte passthrough | Needs genuine utility |
| FIND "needle" NOEND.TXT <A.TXT | Empty output: filename ignored, stdin used | Existing `initech-p00t`; needs capture |
| FIND /Z "needle" and SORT /Z on NOEND | Both produce the same 20 bytes as unswitched execution | Manual says FIND invalid switch has a diagnostic (printed Reference p.331); exact run pending. `initech-p00t`, `initech-shkp` |
| MORE /Z on NOEND | 18-byte passthrough | Needs capture |
| SORT on BIG.TXT | 21-byte `Insufficient memory` diagnostic | Needs capture of real buffer boundary and exit status |
| FIND on BIG.TXT | Completes with 65538 output bytes, omitting the tail; source caps input at 64 KiB | Needs capture; not promoted to a proved 3.30 differential |
| FIND and SORT on LONG.TXT | Each writes 621 bytes, final CRLF added | Needs capture of period line-length behavior |
| MORE on BIG and LONG | Exact 123000 and 619-byte output files, completes | Needs capture of period pagination/redirect behavior |

These are product observations, not invented period expected values. In
particular, the original medium cannot answer whether redirected real MORE
pauses on a large file. The old product comment claiming it does not pause is
not an independent oracle. All completed filter bytes are retained, and the
five-second exploratory timeout was replaced by a completed 60-second-deadline
run before reporting sizes. FDISK remains a bounded interactive observation.

## Defects and source locations

| Severity / bd | Source anchor | Required owner action |
|---|---|---|
| P0 `initech-2vkuq` | `dos/command/command.c:3175` | Validate trailing DEL operands/switches before unlink |
| P1 `initech-y5t9p` | `dos/command/command.c:506`, `:3281` | Refuse extra REN switch before rename |
| P1 `initech-etdo6` | `dos/command/batch.c:743`, `dos/command/command.c:4408` | Correct FOR separators and continue after wildcard member |
| P1 `initech-upa66` | `dos/command/command.c:4137` | IF EXIST must use wildcard-aware existence |
| P1 `initech-f6ygt` | `dos/command/command.c:4380`, `:4452`; `batch.c:629` | Distinguish malformed control syntax from false/no-match |
| P1 `initech-lb371` | `dos/command/command.c:4590` | Keep invocation spelling separate from opened batch path |
| P1 `initech-pnjw6` | `dos/command/command.c:2547`, `:3007`, `:2742`, `:2662`, `:1668` | Match captured wording and CON/stdout destination |
| P1 `initech-mzll9` | `dos/command/command.c:2934`, `kernel/fileio_fat.c:251`, `kernel/fat12.c:1408` | Ground the 8.3 truncation boundary; do not change raw-name tests blindly |
| P1 `initech-e70f4` | `dos/command/command.c:159` | Recognize adjacent CD syntax |
| P1 `initech-tm15k` | `dos/command/command.c:3825` | Unknown command must not create an error-output file |
| P1 `initech-if6qo` | `dos/command/command.c:4644` | Restore/reload parent batch text after secondary COMMAND |
| P1 `initech-hp0tb` | `kernel/config_sys.c:147`, `:190`; `kernel/sysinit.c:438` | Implement or visibly refuse every parsed directive and bad value |
| P1 `initech-vx2ko` | `kernel/sysinit.c:433`; `dos/command/env.h:46` | Apply or refuse SHELL path and /E option |
| P1 `initech-w7y7u` | `kernel/kbd.c:177`, `:213` | Route physical Ctrl-Break into batch cancellation |
| P1 `initech-905b8` | `kernel/int21.c:2238`, `:2345` | Validate drive and directory before accepting device basename |
| P1 `initech-6rmfn` | `dos/command/command.c:3950` | Replace assumed pipe-name/count behavior with captured DOS behavior |
| P1 `initech-hcoi3` | `dos/command/command.c:2625`, `:2564`, `:2657` | Render captured DIR/W fields, framing and line endings |
| P1 `initech-opz31` | `dos/chkdsk/chkdsk.c:279`; `dos/diskutil/diskutil.c:243`; `dos/common/dosutil.c:130` | Match genuine utility validation output and continuation |

Existing `initech-xfg8` has new undefined/unmatched/overflow captures;
`initech-tyvt` has new SHIFT evidence; `initech-u6x8` has existing genuine COPY
captures plus the new pipe-full message/control capture; `initech-dt7i6` has
the matching non-destructive physical-limit control and an explicit remaining
mask-capture requirement. No issue is closed and no tracker export is committed
from this worktree.

## Remaining captures and owner handoff

`initech-tnesd` tracks all the following. Use README commands and `capture.sh`;
`DOS33_UTIL_DIR` supplies authenticated supplementary executables, whose hashes
are recorded. Never copy an observed product file into expected/.

1. Genuine 3.30 filters and the missing external utilities: run FILTER, MORECASE
   and U01/U02/U03/U05/U06/U07/U11/U12/U14. Include exit code probes after each
   command; current aggregate completed cases establish return/continuation,
   not exact AH=4Dh codes. More than the supplied /Z spot checks are required
   for an exhaustive switch grammar.
2. Raw EXEC tails: a 16-bit parent should shrink its allocation, call TAIL.COM
   through AH=4B00 with independently constructed 126/127/128 length blocks,
   record CF/AX and the child's PSP bytes; use a corresponding flat parent in
   the product. Current TCASE is explicitly a physical-line test. Add prompt
   keyboard input at the maximum length separately.
3. Repeat %VAR% overflow in a reduced direct-prompt experiment and keep the
   reference noncompletion separate from a normal expected result. Exercise
   expansion adjacent to redirection targets. For identical-capacity SET,
   start from identical environment strings, then measure each failed insertion
   and failed replacement (including whether an old value is partially changed).
4. Isolate CONFIG numeric values one per boot: FILES -1/0/256/12junk; BUFFERS
   banana/0/100/12junk; LASTDRIVE Zjunk; valid boundary effects and CONFIG beyond
   1024 bytes. Do not derive period bounds from product constants.
5. Exact destructive long-line mask and cut redirection-target cases in a
   disposable image with a captured answer/confirmation sequence. Existing
   `dt7i6` is not grounds to invent a non-DOS refusal policy.
6. Ctrl-C/Ctrl-Break during an active large filter, IF/FOR body and tight GOTO
   loop, with BREAK both OFF and ON; queue N/invalid/Y at the actual question.
   The fresh physical-break test covers PAUSE, not every asynchronous boundary.
   Existing Ctrl-C captures/host tests cover their documented narrower paths.

The DOS/kernel owners should integrate these fixtures into their existing
component gates, select independent semantic assertions, show them red against
unmodified source, then fix product code. Shared factory wiring remains with
its owner. No STATUS.md edit or push is made under BRIEF-COMMON.

## Verification

- `nice -n 19 make -j2 image`: succeeded; ordinary image used for every product run.
- `nice -n 19 make -j2 -C dos test`: initial shared-load run hit five 20-second
  deadlines and the 30-second budget (213 seconds). It did not change tests.
- `nice -n 19 make -j2 test`: ran all 19; five components red from timeout/budget
  failures. DOS itself passed all 32 in 27 seconds in this run. Retained summary:
  `observed/checks/top-default.log`.
- `nice -n 19 make -j2 test TEST_TIMEOUT_S=60 TEST_BUDGET_S=300`: **ALL GREEN**, all
  19 components, 97 seconds; DOS 32/32 in 12 seconds. Kernel 91/91, compiler
  40/40. Only command-line limits were relaxed. Retained summary:
  `observed/checks/top-relaxed.log`. Default-budget green is not claimed.
- Final normal product captures: 486/8 MiB, complete markers, no timeout,
  no triple fault, two initial CPU resets, no guest errors. Pending utility
  FDISK /Z is explicitly the bounded noncompletion exception.
- `sh .../compare.sh`: intentionally **red**, eight independently captured
  mismatches, seven green controls; retained `observed/checks/compare.log`.
  No fixes were made, so these mismatches have not turned green.
- `sh .../verify.sh`: retained capture hashes green; a deliberately corrupted
  copy of the BATCH transcript is rejected. This proves integrity checking,
  not product parity.
- No product/interface/smoke driver changed; full `make smoke` and mutation
  sweep were not required or run. These external QEMU observations are
  compatibility experiments on the normal image.
