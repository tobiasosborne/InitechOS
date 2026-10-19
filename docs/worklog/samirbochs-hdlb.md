# samirbochs -- initech-hdlb Bochs remainder

## Context and change

Lane `lane/samirbochs`; no commits, stashes, branch changes or beads writes.
Read the main-tree CLAUDE.md, PRD sections 5/6.6/8, locked hardware and DBF
data, ADR-0009 DEC-08 and WL-0170. `bd show initech-hdlb` returned no issue;
the lane's checked-in `.beads/issues.jsonl` contains the full bead and its
QEMU+Bochs acceptance clause.

The new `test-samir-boot-bochs` refreshes the original QEMU still, copies
the SAMIR data volume, waits for no `pgrep -x bochs` process (30-second
polls, at most 30 minutes), and drives the original keys after SHELL-READY.
It uses WL-0170's video/milestone verdict and exact RGB plus displayed
BGR233 checker against that QEMU still. The serial check asserts the dot
prompt, both command echoes and the exact three known CLIENTS rows.

The existing Bochs driver had no input replay. Added the narrow RFB key
vocabulary needed by this trace, with both edges, complete-line gating
and capture held until the final release. The independent wire-byte test
replaces only send(), including short writes, EINTR and transport failure.
The Return-to-BackSpace mutant proves that host check bites.

The independent roster now has 73 rows and a seventh wiring mutant that
removes SAMIR input. Both Bochs targets are beside test-boot-bochs in the
emulator vector. The live short-read mutant requires a clean Bochs pass
first, then BOTH serial and pixel wrong-data REDs with a valid snapshot
and no crash; infrastructure failure cannot count as a mutation proof.

## Files

- Makefile
- harness/samir_boot_bochs.mk
- harness/emu/bochs.c
- harness/emu/bochs.h
- harness/emu/bochs_main.c
- harness/emu/test_bochs_legs.c
- harness/emu/test_bochs_legs.sh
- harness/emu/samir_boot_check.sh
- harness/emu/test_samir_bochs_keys.c
- docs/research/samir-bochs-rfb.md
- docs/worklog/samirbochs-hdlb.md

## Acceptance evidence

Every make invocation used `-j3`; every QEMU run used
`INITECH_QMP_STDIO=1`. No full test vector or other Bochs leg was run.

Counts used a read-only extra makefile on stdin, printing
`$(words $(TEST_UNIT_GATES))` and `$(words $(TEST_EMU_GATES))`:

    >>> samir-lane-count: 696 host + 569 emu gates
    >>> samir-lane-count: 698 host + 571 emu gates

First RED, before adding the target:

    RED bochs-legs MISSING: test-samir-boot-bochs

`make -j3 test-samir-boot` (also refreshed as the new leg's prerequisite):

    >>> test-samir-boot [1/5]: no triple-fault (SAMIR ran without crashing)
    >>> test-samir-boot [2/5]: SHELL-READY (COMMAND.COM REPL entered)
    >>> test-samir-boot [3/5]: SAMIR EXEC ran and the dBASE dot prompt appeared
    >>> test-samir-boot [4/5]: LIST rendered all 3 records on serial (PESTON / WADDAMS / LUMBERGH)
    >>> test-samir-boot [5/5]: screendump shows SAMIR's dot prompt + LIST table on the seafoam desktop

`make -j3 test-samir-bochs-keys test-samir-bochs-keys-mutant test-makefile-vars
build/bochs_harness build/ppm_bochs_check`:

    >>> test-samir-bochs-keys: green (RFC KeyEvent bytes, both edges, partial writes, EINTR, broken transport)
    RED samir-bochs-keys: Return keysym must be ff0d
    >>> test-samir-bochs-keys-mutant: green (Return-to-BackSpace branch correctly RED)
    >>> test-makefile-vars: green

The two explicit build targets exited zero (no oracle green banner):

    cc -std=c11 -Wall -Wextra -Werror -Iharness/emu -o build/bochs_harness harness/emu/bochs_main.c harness/emu/bochs.c
    cc -std=c11 -Wall -Wextra -Werror tools/ppm_bochs_check.c -o build/ppm_bochs_check

`make -j3 test-bochs-legs` and `make -j3 test-bochs-legs-mutant`, separately:

    GREEN test-bochs-legs: 73 named boot legs; vector, images, boundaries, video paths, the padded DAC row
    BOCHS_LEGS_MISSING: RED bochs-legs MISSING: test-command-file-emu-bochs
    BOCHS_LEGS_VECTOR: RED bochs-legs VECTOR: test-command-file-emu-bochs
    BOCHS_LEGS_IMAGE: RED bochs-legs IMAGE: test-command-file-emu-bochs
    BOCHS_LEGS_BOUNDARY: RED bochs-legs BOUNDARY: test-block-device-emu-bochs
    BOCHS_LEGS_VIDEO: RED bochs-legs VIDEO: test-boot-vga13-bochs
    BOCHS_LEGS_DAC: RED bochs-legs DAC: test-flair-desktop-8bpp-bochs
    BOCHS_LEGS_SAMIR: RED bochs-legs SAMIR: test-samir-boot-bochs
    GREEN test-bochs-legs-mutant: all seven factory wiring mutants caught

The first combined parallel roster invocation raced their shared
`build/bochs2/legs.make-db`, producing spurious IMAGE failures; isolated
reruns above are green. No unrelated roster entry was changed.

`make -j3 test-samir-boot-bochs`, twice, with no Bochs process present:

    bochs_run: RFB socket: Operation not permitted
    [bochs] launched=1 rfb_unblocked=0 timed_out=0 exit=1 signal=0
    [bochs] serial_len=0 marker_found=0 (expect=SHELL-READY)
    [bochs] triple_fault=0
    [bochs] quit_marker_found=0 quit_markers_seen=0 (want=1, quit_after=SHELL-READY)
    [bochs] serial=build/samir_boot_bochs.serial
    [bochs] screendump_after=SHELL-READY marker_found=0 screendump_ok=0 rfb=0x0 full_frames=0 rfb_error=(none)
    [bochs] rfb_pgm=(none)
    [bochs] snapshot_ok=0 snapshot_tries=0 snap_ppm=(none)
    [bochs] OK=0

Bochs itself agrees on the infrastructure failure:

    [RFB   ] >>PANIC<< could not create socket.

Also checked shell syntax, ASCII of changed/new code and `git diff --check`.
The new serial checker accepts the real QEMU serial capture; that is a
checker check, not evidence of Bochs acceptance.

## Not accepted / handoff

Live Bochs serial, pixel equality and the pal_milton short-read mutant are
UNVERIFIED. Socket creation is forbidden by this execution sandbox, and
approval escalation is unavailable. No timeout, tolerance or golden was
weakened. The short-read Bochs mutant was not run after the blocked clean
leg. PM must run the two new Bochs targets serially in a socket-enabled
environment. Bead closure is not justified by the available evidence.

Full logs are in `build/samir-bochs/`, the blocked Bochs report in
`build/samir_boot_bochs.report`, and the refreshed independent still in
`build/samir_boot_scrn.ppm`. No Bochs process remains.
