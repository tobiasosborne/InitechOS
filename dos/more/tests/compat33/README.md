# MORE under MS-DOS 3.30

The reference is Disk 1 of Microsoft MS-DOS 3.30, MD5
`5c2c58602d688c636ea1c9801e6106f4`. MORE.COM SHA-256 is
`9a71219dbcc0ebe9b082a1ee8450ca4ed7150ce069dfdd7dcdc15a641465bb06`.
The [acquisition record](../../../command/tests/compat33/UTILITY-MEDIA.md)
identifies the archive. The fixture contains observations and no DOS binary.

The first VGA capture stops after line 24 and shows `-- More --` in row 25.
The capture after a further idle interval is byte-identical. Enter advances
past the cue and leaves a blank row above the next lines. The second wait is
after line 47. An ordinary key is echoed beside the cue as the pager moves
down. The old marker scrolls with the page. At a wait, Ctrl-C puts `^C` after
the cue and returns to COMMAND before line 25.

In the genuine redirected pipe, the VGA capture shows the console waiting
without a visible cue. The raw PIPE.OUT includes the cue at two waits and
the continuation bytes from the Enter keys. The short pipe capture includes
an initial CRLF. InitechDOS keeps its file and pipe output byte-exact, with
no input wait. The product replay grades that path against the input fixture
and grades the interactive cue against the genuine VGA rows.

Run the acquisition during an idle emulator slot with
`sh dos/more/tests/compat33/capture.sh <disk1.img> <new-output-directory>`.
Each VGA observation is saved as 80 columns by 25 rows. The checked-in text
has blank cells trimmed from the right of each row. The native replay uses
the QEMU harness and its text-screen tool, compares all rows at the waits,
then compares the pager-owned rows and the return to the prompt after
continuing or cancelling.
