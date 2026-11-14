# Column-range width captures

Real R2.2, initialized runtime on DOS33/QEMU 486/8MiB, driven by
`goldens/dataloss/drive.c`. Raw VGA, transcriptions, input steps, extracted
WK1s, independent dumps and SHA256SUMS are retained. `invalid-241/` isolates
that input from the multi-command run; only its isolated diagnostic is used.

Set-Width asks for a cell range, then a width 1..240; the rows select the
inclusive columns. B1..C4 changes B/C and preserves A/D. Reset removes those
overrides so a subsequent global width applies. IV1 permits width 240.
Zero, 241 and fractional widths refuse with `Invalid number input`;
Esc restores the default width prompt. Cancel preserves the saved widths.
