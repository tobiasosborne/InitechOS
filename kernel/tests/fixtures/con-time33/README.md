# Console handle timestamps on DOS 3.30

Packard Bell MS-DOS 3.30, medium SHA-256
`3612b044c7c1c0160768a26e18c2c7e2d68ecd67b375aae9f5ec43bb8548cc30`.
Boot a scratch copy in QEMU, 486/8 MiB, with RTC base 1992-03-02T09:30:00.
Assemble probe.asm with NASM, then run PROBE.COM from AUTOEXEC.BAT with
an isa-debug-exit device at F4h. `od -An -t x2 RESULT.BIN` records packed
time and date for each successful AH=57h GET.

| Handle and operation | Time | Date |
|---|---|---|
| stdout before SET | 4BC0 | 1862 |
| stdout after SET | 1800 | 2221 |
| stderr after SET on stdout | 1800 | 2221 |
| CON opened after setting the date to 1993-03-04 | 4BC0 | 1A64 |

Repeating the third query with BX=0 gave 1800:2221 as well. An inherited
CON handle retained 1800:2221 after the date changed.
