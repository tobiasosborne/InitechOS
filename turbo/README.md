# Turbo Initech

Two compilers: `seed/` is the C host bootstrap; `compiler/turbo.pas` is the
Pascal compiler. The seed emits assembly for NASM and binutils on the host.
The Pascal compiler emits native flat COM and extended InitechMZ executables,
plus assembly retained for independent factory byte comparisons. No host
assembler or linker finishes the programs compiled inside InitechOS.

`make` builds the seed, the FPC host compiler and resident `TURBO.COM`.
Despite its name, TURBO.COM is an extended InitechMZ container: a flat 32-bit
module linked at zero, relocated to 0x600000 by the content-dispatch loader.
Startup retains the loader's separate 64 KiB stack; descriptor reserve covers
BSS including alignment. Files and exit use INT 21h. Inherited serial helpers
now poll transmitter readiness. No private kernel header/code is included.

`make test` composes 6 seed, 41 compiler and 5 IDE host entries, including
18 FPC execution references and independent native byte comparisons (18 flat,
19 extended). The two compiler placements are packaged with factory/mz/mzlink
and checked by the independent mzcheck against the actual extended limits:
262144 module bytes, aligned module+reserve before 0x7ef000, native heap
header, separate guard and stack. The old conventional compiler placement
gates are replaced by these real limits. Small programs retain their flat
runtime/link route. The warm complete Turbo suite takes about 20 seconds.
Runtime is a checked-in NASM byte table with a tier-0 regeneration comparison;
see compiler/NATIVE.md. The 15 compiler patches include five native
encoder/fixup/relocation/layout and three generated-execution patches.

`make smoke` builds the product image and uses separate FAT12 TEST disks.
QEMU and Bochs each execute all 17 corpus programs and 19 regressions through
direct seed output, hosted Pascal native output, and resident Pascal native
compilation. Hosted and resident Pascal output also runs as flat COM:
370 executions (gen_ioresult adds 10), preserving all prior 330 and the 30 comparisons for
computed characters at the start of concatenations, including higher temporary
bases. The earlier probes cover minimum-Integer div/mod and string temporaries
in Length and record indices.
Every registered case fits the flat arena; missing flat output
fails. Complete stdout and extracted files match FPC;
the original corpus also matches its authored goldens. mzcheck validates the
emitted containers. See tests/execution/README.md for the red-first probes and
the explicit 32-bit endpoint reference normalization.
The guest TIMER test utility brackets compilation through DOS GET TIME at its
1-second resolution. Compilation timing remains recorded per resident case.
No test material is placed on the product boot disk. Images/logs remain under
build/turbo/smoke. The old placement-only check is tests/smoke/placement.sh.

`make selfhost` builds K1 on the host, then performs K1(S) -> K2 and
K2(S) -> K3 on both QEMU and Bochs at 8 MiB. Every compilation uses a fresh
TEST disk and the ordinary product boot image; identical source/input names
are reset each time. The QEMU invocation is unchanged. mtools extracts whole
containers; K2==K3 must hold on each emulator, and both emulator results must
agree. QEMU artifacts remain under build/turbo/selfhost; Bochs artifacts,
configuration, boot-image identity and timing are under its bochs/ subdirectory.

Factory gives emulator runs independent resources. Bochs uses an explicit
3600000ms acceptance bound for each complete harness invocation, including
startup and cleanup. Calibration at 600s and 1200s expired during
native emission. A monotonic acceptance check also rejects late completion
regardless of the harness verdict, and timed_out must be zero. Override only
with TURBO_BOCHS_TIMEOUT_MS. Ordered per-phase waits are a separate factory
follow-up, initech-b8vxr; the operator authorized this bounded whole-run route
until that interface is available. No factory default or QEMU limit is changed.

Controls reject a corrupted QEMU candidate, a corrupted Bochs K2 attempted in
a real guest boot, and an impossible one-millisecond Bochs deadline. The Bochs
corruption preserves MZ/tag but changes the descriptor version, so its red is
a clean loader refusal rather than raw-COM execution or a timeout.

`make ddc` first refreshes that seed-path certificate, then uses the same
compiler source built with Free Pascal as the independent host bootstrap.
That compiler emits D1 directly; D1(S) runs in the OS on both emulators to
produce D2. Each D2 must equal its K2/K3 and the other emulator's D2.
Contamination and changed-source controls must fail. Evidence, FPC version
and bootstrap hashes remain under build/turbo/ddc, with a separate bochs/ tree.

The certified fixed point is K2==K3==D2: 170607 bytes, SHA-256
a4ddd89971dfe232ccf0f43602fa5fba33ba4966d0bf7aeb7371aea2701ffe47.
Stage B (initech-n6r9v) moved it from 164263 bytes (2c33ed0c...): the
751-byte runtime with recoverable I/O, the new builtins and the 2,048-row
symbol tables. Fresh QEMU DOS-clock times are 103s/93s for K2/K3 and 107s
for D1(S). Bochs K2/K3/D2 agree with QEMU at this hash; measured host
harness times are 2534.245s/2671.097s/1560.722s, each below 3600s, with
other agents' Bochs runs on the host at the same time. The combined
command was stopped by the agent's 2-hour background limit during the
Bochs DDC leg, which was then rerun alone (bochs-ddc.sh) against the same
source hash; all negative controls are red. Runtime-encoder authorship
remains open separately as initech-cd2yi.
The checked-in NASM tables regenerate
byte-for-byte. Times use the guest clock with 1-second resolution. DDC removes sole
reliance on the C seed's translation and its bootstrap assembler pipeline.
It still trusts the compiler source (including runtime byte tables), the
independent FPC path, host tools, loader, emulator and hardware. The controls
show comparison sensitivity, not resistance to every possible shared attack.

FPC, NASM, GNU binutils and mtools are required; missing tools fail. No Pascal
language feature was added; seed string-character var arguments now accept
the documented form. Authored corpus/protocol goldens are unchanged. Emulator
work never runs in tier 0. See ROADMAP.md for remaining
GOLDEN-MASTER.md item-7 work, including the IDE.

## Runtime for the IDE (stage B, initech-n6r9v)

Both compilers accept TP 5.5's recoverable I/O: a comment holding exactly
`$I-` or `$I+` switches I/O checking (default on); `IOResult()` returns and
clears the pending DOS error; `Close(f)` (103 when not open), `Erase(f)`
(AH=41h) and `Rename(f, name)` (AH=56h, f takes the new name). While an
error is pending, I/O does nothing; checked code calls `rtl_io_check`,
which prints the old `FILEIO-ERROR AX=n` line and exits. `ReadKey()`
returns AH=07h's character, 0 then the scan code for extended keys. The
switch is sampled after a call's closing parenthesis, so it belongs
between statements. Reference: TP 5.5 Reference Guide (IOResult, the I
directive); Free Pascal is the independent oracle: `gen_ioresult` matches
its hand-computed golden under FPC and, through all five paths, in the
image on QEMU and Bochs; `ide/tests` checks ReadKey's codes and the abort
in the image. The runtime is 751 bytes (padded to 752); its error word
sits in the code and is addressed position-independently, so neither
output form gains a relocation. `tests/runtime.sh` also checks that the
compiler's fixed runtime entry offsets equal the linker's.

**Capacity.** One TURBO.COM must hold the compiler and the IDE. Measured:
the compiler alone uses 904 symbol rows, 576 names and 5,679 pool
characters; with the IDE's editor 1,292, 795 and 7,216, before its compile
and run commands. Limits are now 2,048 rows, 1,536 names and 16,384 pool
characters (were 1,024, 704 and 6,144): 57,856 more bytes of BSS (ten
4-byte columns x 1,024 rows, 832 names x 8 bytes, 10,240 characters),
inside the extended module's reserve and small beside 8 MiB. The seed's
tables grew the same way on the host (1,024 globals, 512 routines, 256
global arrays). `ide/tests` `one-program` compiles the compiler's
declarations with the IDE as one program with both compilers; at the old
limits it failed with `char pool exceeds POOLMAX=6144` and the seed's
`TC_MAX_SYMBOLS exceeded`.

## IDE (GOLDEN-MASTER item 7): the frozen surface

The reference is real Turbo Pascal 5.5 under real MS-DOS 3.30:
turbopascal-decomp `goldens/minted/MINT06` (raw B800h cells, CRTC cursor,
disk read-back; distilled in its `specs/ide-journeys.md`) and MINT01/02,
pinned by `goldens/CORPORA.lock`. Geometry, attributes, cursor and
behaviour come from those captures; their wording that names the original
vendor or product does not appear on any Initech screen.

- **Program.** One `TURBO.COM`: no arguments opens the IDE, a file argument
  compiles from the command line (operator ruling on the IDE design). See
  "Packaging" below for what that still needs.
- **Layout.** The editor is always TP 5.5's zoomed editor (MINT06
  `03_zoom`): row 0 menu bar, row 1 info line (0x17), rows 2-23 text
  (0x1E), row 24 status line (0x70, keys 0x74). The Watch window does not
  exist, so the framed two-window layout is never shown and F5 is absent.
- **Menu bar.** `File` `Edit` `Run` `Compile` at columns 4, 12, 20, 27, as
  in TP 5.5; no other titles. At start the bar is active with File selected
  and the cursor hidden (`02_editor`); Alt-E or Esc enters the editor.
  Edit is not a pulldown (Alt-E enters the editor). F10 activates the bar
  on the last used title.
- **File:** New; Load (F3); Save (F2); Write to; Quit (Alt-X). Pulldown
  geometry and item positions as TP 5.5's File menu with the absent items
  removed.
- **Run:** Run (Ctrl-F9); User screen (Alt-F5).
- **Compile:** Compile (Alt-F9); Make (F9). With one file and no units
  they do the same work. The executable always goes to disk: the box title
  is TP's "Compiling to Disk" (`13c`) form.
- **Editing:** printable characters, Enter, Backspace (joins at column 1),
  Del, Ins (Insert/Overwrite; only the info-line word changes), arrows,
  Home/End, PgUp/PgDn (21 lines, cursor keeps its screen row),
  Ctrl-Home/Ctrl-End (top/bottom of window), Ctrl-PgUp/Ctrl-PgDn
  (start/end of file), Ctrl-Y (delete line). Tabs in a file are kept and
  shown at 8-column stops; horizontal scrolling as `37`. The Tab key and
  auto-indent are absent (the Tab key's TP rule is not established by the
  captures), and so are the info line's `Indent` and `Unindent` words.
- **Info line:** `Line n` at column 6, `Col n` at 17, `Insert` at 25,
  modified star at 57, drive and name at 59, as `03_zoom`/`04_typed`.
- **Status line:** only keys that exist, in TP's format (one leading
  space, two spaces between items), the captured legend's surviving
  items: ` F9-Make  F10-Menu`;
  inside a prompt ` Esc-Abort`.
- **Errors:** the first error replaces the info line, attribute 0x4E
  across the row, TP's number and text where the error is one TP has
  (`Error 85: ";" expected`); Initech's own limits keep honest Initech
  wording. The cursor goes to the token; an off-screen error line is
  placed on screen row 13. The next key restores the info line and keeps
  the position (`07`, `08`, `34`).
- **Files and prompts:** F2 on an unnamed buffer asks for a name (TP's
  "Rename" box, `05`); Write to asks for a new name and confirms an
  overwrite (`48`, `49`); New/Load/Quit on a modified buffer ask
  "NAME not saved.  Save? (Y/N)" in the Verify box, Esc cancels (`43`,
  `46`, `56`); failures show the Error box and leave the buffer modified
  (`51`, `54`). Saving keeps the previous version as `NAME.BAK` and writes
  lines joined by CRLF followed by one 0x1A, as TP 5.5 does
  (`disk/DEMO.PAS`, `disk/DEMO.BAK`).
- **Run and user screen:** Ctrl-F9 makes first if needed, never runs an
  old executable after an error, and returns straight to the editor;
  Alt-F5 shows the screen the program left until a key is pressed (`14`
  to `16`).
- **Absent, not refused:** Help/F1, Pick, Directory, Change dir, OS shell,
  Options and Debug menus, Break/watch, Watch window, Zoom/Switch, trace,
  step, breakpoints, Program reset, Go to cursor, Build, Destination,
  Find error, Primary file, Get info, search, blocks, clipboard, undo,
  multiple windows, mouse, syntax colour, 43/50-line modes, the splash
  box. No greyed placeholder or "not available" message exists for them.

The editor, chrome and paged buffer are in `ide/` (its README): tier 0
compares 47 screens with MINT06 and the saved bytes with TP 5.5's; `make
smoke` repeats both journeys with the Turbo-compiled IDE in the booted
image on QEMU and Bochs.

### Packaging (feasibility, 2026-11-14)

`tests/ide/probe/` shows in the booted product that an extended
`TURBO.COM` stays resident across EXEC and gets the child's exit code back,
but it cannot run what Turbo compiles: the loader refuses an extended
child while an extended program is live (error 8), and places a nested
flat child after the parent's PSP block (0x41220), where Turbo's
fixed-base flat output runs wrongly while EXEC reports success. Running
compiled programs from the IDE therefore waits for a kernel change (see
the probe README); the IDE's editor, chrome and runtime do not depend on
it.

<!-- interface-declarations -->
These interface declarations for CLAUDE.md "Two interfaces". Pure contracts operate
on caller-owned data; resident bindings are unavailable to disk applications.
Records grant type/layout access only. The audit checks the target make graph,
compiled includes, external symbols and interrupt vectors against these lines.
Interface role: dos
Interface product-elf: build/turbo/compiler/extended/zero.elf
The package recipe links this ELF into shipped TURBO.COM; Multiboot startup
objects in all are judging fixtures, not DOS application inputs.
<!-- end-interface-declarations -->
