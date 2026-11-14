# Active program descriptors

This supplements genuine III+ C33 before its implementation fix. The same
hash-pinned DOS 3.30/III+ runtime and input files are used. Only ON ERROR is
changed to WAIT 'FD-READY', letting the independent capture driver save memory
while the exhausted program stack is still active. Enter then unwinds and
completes the original session. `capture.sh ABSOLUTE_EMPTY_OUTPUT` reproduces
the observation; raw executable memory stays in the disposable output.

`reference/handles.txt` is an independent PSP/JFT/SFT metadata projection by
read.c, not an InitechBase observation. It shows a twenty-entry child JFT,
all twenty occupied, and these sixteen file descriptors owned by that PSP:

- DBASE.OVL, one;
- MINT.PRG, one;
- RESULT.OUT and BASE.DBF, one each;
- PROBE.PRG, twelve separate entries, each with reference count one.

The parent JFT's AUX slot (handle 3, SFT 0) becomes the child's SFT 3; the
DBASE.OVL SFT entry is at the corresponding first-table position. Thus III+
keeps four standard descriptors and reuses AUX for the resident overlay.
The next disk DO fails because the JFT is full, not because twelve is a
language nesting limit. Original C33 supplies the exact #6 transcript; the
independent library-recursion corpus still supplies nineteen/#103.

InitechBase likewise closes unused AUX before opening INITBASE.OVL, retains
each active disk source until return, and preserves DOS error 4 through its
PAL as the #6 command error. Its host PAL has no real overlay descriptor;
its available table therefore models twenty JFT entries minus four standard
handles minus the product's one resident overlay. This models resources,
not a twelve-frame program cap.

The retained commands, metadata, completed result and VGA text are hash-pinned.
The capture's raw memory is at the directory recorded in the Phase 2 report;
no reference executable or one-MiB executable memory dump is checked in.
