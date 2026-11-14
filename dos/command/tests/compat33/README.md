# DOS 3.30 compatibility study inputs and evidence

Review fixtures, not an acceptance gate. Product code is unchanged. The owner
can turn captured cases into red-first component tests; pending expectations
must be captured before they are asserted. `review-dos.md` is the case table,
tracker disposition, limits, and verification record.

`generate.c` writes CRLF batch/config inputs and deterministic data under
`build/dos-compat33/inputs-<mode>`. `devices.asm`, `stream.asm`, and `tail.asm`
are test-only programs, assembled in genuine 16-bit DOS and product flat 32-bit
forms. They do not enter a product image. The device probe opens, queries, and
closes: no printer or serial device read/write can hang the experiment.

From repository root, with mtools, QEMU, NASM, cc, and fsck.fat installed:

```
nice -n 19 make -j2 image
nice -n 19 sh dos/command/tests/compat33/capture.sh reference BATCH LIMIT PARAMS ENV SWITCH NAMES REDIR TCASE DEVICES STREAM FULL
nice -n 19 sh dos/command/tests/compat33/capture.sh reference CFG1 CFG2 CFG3 CFG4 CFGA CFGB CFGC CFGD CFGE CFGF
nice -n 19 sh dos/command/tests/compat33/capture.sh reference U04 U08 U09 U10 U13
nice -n 19 sh dos/command/tests/compat33/capture.sh product
nice -n 19 sh dos/command/tests/compat33/capture.sh product FILTER MORECASE STREAM U01 U02 U03 U04 U05 U06 U07 U08 U09 U10 U11 U12 U13 U14
sh dos/command/tests/compat33/break-reference.sh
sh dos/command/tests/compat33/break-product.sh
```

`DOS33_BOOT` overrides the local original medium, whose required hash is
3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30.
The default is `$HOME/Projects/initech-manuals-ref/sidekick/dosboot/` followed
by `PB_MSDOS33_Sys_and_Util.img`. Original media is read only. The existing
`goldens/dataloss/drive.c` supplies HMP completion barriers and raw VGA captures.
Product runs use the built factory harness, a 486, 8 MiB, and the ordinary
boot image with a disposable working disk. `DOS33_TIMEOUT_MS` overrides its
60-second capture timeout; this is an observation deadline, not an assertion
that a command can never finish. Each capture records `driver.rc`, logs,
post-disk bytes, and independent mtools extraction. A successful capture
command records observations; it does not declare parity. Inspect its rc.

The original boot disk lacks FIND, SORT, MORE, EDLIN, FDISK, FORMAT, XCOPY,
TREE, and LABEL. Their unminted outputs are **pending**, not “Bad command or
file name” expected fixtures. For supplementary utilities, set
`DOS33_UTIL_DIR` to a directory containing authentic 3.30 executables; the
capture script copies and hashes them on the disposable data disk. Obtain
and authenticate that media before using the results. TPSREP is an Initech
extension and has no MS-DOS counterpart. No boot media or proprietary
executable is committed here. The post-data images contain only test inputs,
helper programs and their outputs. DOS 3.30's CHKDSK output is a captured
observation, not a copy of its executable.

`expected/` contains only genuine DOS results. Exact input bytes are recoverable
from each `data.img.gz`, and the actual boot AUTOEXEC/CONFIG files are retained
separately. Older capture disks include an unused MORE.BAT and TAIL.BAT: do not
run those names. The final generator uses MORECASE and TCASE so command search
cannot select the wrong same-stem program. Replay outputs need not have an
identical complete directory inventory. Compare the requested operand/files,
normalize reference B: versus product A:, host-created input dates and free
space where appropriate, and preserve meaningful bytes and output streams.
Do not normalize away %0's added path, diagnostics, CRLF, or missing files.

`observed/` is product evidence, never expected data. The final retained normal
product runs use 8 MiB and report `timed_out=0`, `OK=1`; U06 is the explicitly
bounded FDISK noncompletion exception. `observed/NEST` is a completed 8 MiB
reproduction ending at the prompt after the parent batch-buffer overwrite.
`expected/CFG3` deliberately has no completion marker: DOS refuses its
missing shell. `expected/EXPAND-PENDING` retains an inconclusive reference
noncompletion and is not an expected output contract.

```
sh dos/command/tests/compat33/verify.sh
sh dos/command/tests/compat33/compare.sh
```

The first command checks retained evidence hashes. The second intentionally
returns red against the unfixed product observations, while also showing green
controls for line limits and stream bytes. It does not run the OS. No source
from `attic/` is built. Nothing is added to shared build wiring or STATUS.md.
