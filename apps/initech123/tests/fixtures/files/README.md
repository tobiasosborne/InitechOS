Real Lotus 1-2-3 R2.2 file-command probes, Reference 5-13..18.

All sessions use goldens/dataloss/drive.c, the hash-pinned initialized runtime
from goldens/dataloss/runtime.sh, DOS33/QEMU 486/8MiB. input.steps records exact
keys, .vga the raw text/attribute screen, .txt its character transcription.
Unsteady initial experiments are excluded. steady3 and boundaries use settled
FILES screens and explicit READY waits. Literal sample files contain eight
bytes `sample\r\n`, dated 2 March 1992 09:30 with mcopy -m. mtools listings are
independent FAT directory evidence before/after confirmed deletion. Directories
in the disposable reference volume carry their actual captured FAT dates.

Worksheet filters are *.WK? (including WK2), Print *.PRN, Graph *.PIC, Other
*.*. Files sort lexically before directories. Hidden files are omitted;
read-only files remain eligible. Full List displays selected name/date/time/size,
Home/End/arrows and paging; Return dismisses a file selection, selecting a
directory browses locally. Browsing does not change the session Directory.
Erase has an inline chooser and F3 full-list view, followed by No/Yes, default
No. A bare worksheet basename matches its WK? file; wildcard stems retain the
category extension. Read-only and absent deletions are refused with the exact
captured messages. No/Cancel preserve files and worksheet. Empty folders leave
an editable filename field without a fabricated entry. Relative/parent and
absolute Directory paths work; bad paths/drives say Directory does not exist.
Native drive A substitutes for reference C; the prefix is session state over
root-resolved SDK paths, never a kernel chdir or persisted default.

import/ contains literal INPUT.PRN and independently saved RESULT.WK1 for each
probe. Seed is Fill A1:A20 with 99, Fixed2. CR separates rows, LF and NUL are
ignored, Ctrl-Z ends input; final unterminated data commits. Blank lines advance
without overwriting. Tabs expand to eight-column stops, counted toward the
240-character limit. Other controls are retained as raw label bytes. Text
imports literal content with a left prefix, irrespective of global alignment.
240 characters fit, 241 or tab-expanded 241 fail. Completed earlier lines
persist on protection/length failure even though the reference error screen
still displays old values; the saved WK1 is the authority. At row 8192 the
remaining input is silently truncated, READY. FULL saves 8192 X labels; native
capacity tests compare the complete documented 2048-cell prefix instead.

Native listing capacity is 511 entries; an SDK result reaching the 512-entry
sentinel must report a real limit rather than display a silently incomplete
list. This is a documented native capacity, not an R2.2 capacity claim.

File Save's default after Retrieve retains the retrieved file's complete old
path even after Directory changes; default-path captures this explicitly.
Typed names for all five file operations use the selected session prefix.

Further attribute/drive captures confirm that an explicitly typed hidden file
can be erased after Yes although it is omitted from listings. Yes's description
is exactly Erase the file. An explicit name on a missing drive still gets
No/Yes; Yes reports File does not exist, and No does not attempt deletion.
