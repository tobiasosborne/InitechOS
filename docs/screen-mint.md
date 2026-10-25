# Full-screen differential capture

The 20-line [case contract](../harness/diff/dbf_diff/xbase_prog_diff/screen-mint/README.md)
locks the screen and case formats. `make mint-screen-seeds` writes fresh reviewed
candidates under `build/screen-mint/seeds`; it never overwrites the stored goldens.
Copy the `.screen` files and all three manifests after inspection. No runtime
Python, GUI, full test vector, Bochs, or manual typesetting is involved.

The reference helper extends `samirctl-mint/scrdump.asm`'s video-page copy into
a small guest TSR. `MINT.PRG` executes `SETUP.PRG`, then `RUN SCREEN`, then
returns to the dot prompt. Installing after dBASE initializes matters: dBASE
replaces INT 1Ch during startup. The helper chains that timer vector, feeds
the BIOS keyboard ring only when the previous key has been consumed, and
executes relative waits and named capture events. It verifies the 80x25
geometry and copies the active B800 page (including page offset) and BIOS
cursor into resident memory. The interrupt handler performs no DOS file I/O.
After scripted QUIT, `SCREEN /S` writes each captured 4002-byte page. Only after
all writes and closes succeed does it write `CAPTURE.OK`, containing the saved
dump mask. The driver requires the exact declared mask. Missing, partial, or
unfinished captures fail the mint. DOS time queries are frozen
at noon so ASSIST's clock is real program output with deterministic input.

SETUP is validated recursively before copying: only DOS 8.3 regular files and
directories are allowed, and reserved oracle/driver/capture names are rejected
case-insensitively. Capture files and the save marker are also removed before
launch. Each run mounts a separate guest-only directory; logs and configuration
stay outside it. All staged timestamps are fixed to 1986-07-24 12:00 UTC.
DOSBox-X `freesizecap=fixed`, MOUNT `-size 512,32,2048,1024 -freesize 16`, and
the fixed SCREEN volume label provide 32 MiB total and 16 MiB free. The helper
checks all four INT 21h/AH=36h geometry values before installation and refuses
to run if the drive differs. This follows the installed DOSBox-X reference
configuration's fixed-space contract, independently of host disk availability.

The old `mint_setup.py` and `mint_run.py` were read. They generate and run
control-flow transcript cases and trim a post-exit screen preview, losing
attributes and cursor coordinates. They should be ported to C plus thin
shell when maintaining that older suite; this harness replaces neither its
existing transcript goldens nor its historical provenance.

`test-pubs-screens` uses `pubs/screens/capture.sh`, QMP PPM screenshots and
`harness/console_text.c`. Its decoder trims ASCII rows and does not recover
the full attribute plane or a hidden cursor. SAMIR emu legs likewise take
PPMs and check cells with `ppm_text_check`. For exact full-screen comparison,
the kernel already maintains `g_console_page` (4000 interleaved character/
attribute bytes), and `g_int21_con` points to its `console_t`, including
cursor row and column. This is the same data used by the shipped
`console_save_page` service (`spec/tsr_contract.h`), independent of the goldens.

Set `INITECH_TEXT_SCREEN_ELF` to the matching **unstripped kernel ELF** and
`INITECH_TEXT_SCREEN_TOOL=build/screen_capture` on a normal `qemu_harness`
screen leg. Beside each `name.ppm` it writes `name.ppm.screen`. The helper
inherits the established QMP connection, pauses the guest, reads its real
console state with `pmemsave`, and restores the previous running/paused
state. It also works with `INITECH_QMP_STDIO=1`. The ordinary pixel capture
path has no new dependency when the environment variables are absent.
The helper is polled against the harness's absolute wall deadline; expiry
kills its process group and reaps the direct child. Helper failure removes
its partial output and fails the QMP session and harness verdict, including
record-mode and marker-gated capture paths.
The ELF must match the booted image exactly; symbol presence and the live
80x25 geometry are checked, but an unrelated ELF is not a supported input.

Alternatively, `build/screen_capture QMP-SOCKET KERNEL.elf actual.screen`
captures an idle guest directly. Run `build/screen_tool compare golden.screen
actual.screen`, or `compare-raw golden.screen page.bin` for a 4002-byte
`console_save_page` capture. Exit 0 means identical, 1 means different, and 2
means malformed input, capture failure or I/O error. The first differing
cell reports zero-based row/column, character or attribute, and both hex
bytes; cursor differences name row or column and both values. No trimming,
clock masking, attribute defaults, or cursor inference occurs in comparison.

`make test-screen-mint` checks all stored hashes, codec round trips, same-file
GREEN, one-cell/attribute/cursor RED, malformed-file rejection and key-script
validation. `make test-screen-mint-repro` performs two fresh real-program
runs per case, compares bytes and provenance manifests, checks the stored
goldens, and prints both runs' hashes. Its uniquely named evidence directory
is retained under `build/screen-mint/repro.*`. Logs contain host paths and are
diagnostics, not reproducible golden artifacts. `screen_mint_oracle.sh` rebuilds
the event stream and hashes the current independent dBASE binaries, DOSBox-X,
SCREEN.COM, screen_tool and both mint/provenance scripts. Both gates compare
that result to the stored ORACLE.sha256; the repro gate also compares every
fresh manifest against the stored manifest. A source/tool change requires an
explicitly reviewed provenance refresh; no gate silently re-pins its oracle.

`make test-screen-mint-mutants` runs the original stale-page reproducer,
reserved-name checks, early QUIT and failed-save checks, altered hash/binary/
event/helper checks, stalled-helper unit and full-harness checks, and completed-
then-failed or incomplete SAMIR helper checks. Its RUN DIR CLIENTS.DBF and native
DIR LIKE *.PRG reproducer runs twice after changing host mtimes and disk usage;
both dumps and all three manifests must remain identical. Evidence stays in
`build/screen-mint/review-tests.*`.

`make test-screen-mint-samir` boots the actual SAMIR emulator leg, captures
`USE CLIENTS` / `DISPLAY STRUCTURE`, and runs the differential comparator.
Integration GREEN requires harness exit 0, OK=1, a completed pixel dump, the
helper's completion record and a valid nonempty screen. This is a capture/
integration gate: the parity verdict is reported separately
and can be RED while waves 3-4 remain unimplemented. It must never be read as
a claim that SAMIR's full-screen behavior already matches real III+.
