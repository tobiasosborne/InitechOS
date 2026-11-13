# Real Lotus numeric evidence

These are real Lotus 1-2-3 Release 2.2 saves, not files emitted by the
Initech implementation. The input builder only creates an automatic macro;
real `123.EXE` retrieves it, types every case, and saves the output. The
`.ref` files are independent `lotus123-decomp/tools/wk1_ref.py` decodes.
The PNGs are unmodified frames extracted from DOSBox-X's video capture.

Reference executable SHA256:
`6c870af82bda3bf7962ec28dd89230aff3effb5dab617434a5e1f2231dd86f8b`.

`NUMENTRY.WK1` SHA256:
`61d5e9e1589c47c52bff29733021f50beae55bcb94e4d448dbb8e0ab61a5fa4f`.
Column A rows 1..14 correspond to the `entry` cases in
`harness/diff/wk1_diff/mint_i123_numeric.c`. `$-12.34` and `$+12.34` are
numbers; `-$12.34`, `+$12.34`, `$$12`, `12$34`, and `$` leave no cell.
`$A$1` and `+$A$1` produce absolute-reference formulas, not numeric literals.

`NUMFMT.WK1` SHA256:
`25bcc53570551a3d00b534e5a3e0ebde6e3a2bc2d412c175308d5e87e1e9629e`.
Column A holds the formatted numbers at width 30; column B holds the
corresponding real `@STRING` results, evaluated by Lotus. The reader uses
the older name `@FIXED` for the token that R2.2 calls `@STRING`.
Rows 1..4 are F12..15; 5..8 C12..15; 9..12 comma12..15; 13..18 P10..15;
19..26 are negative, zero, near-carry, negative tiny, half-rounding and
large-number controls. Macro source labels remain in column K.

**B03 requires a scope decision.** Real Lotus itself shows stars for
F/C/comma15 and P13..15 for 1234.5678912345, and caches ERR for
`@STRING(1234.5678912345,15)`. It succeeds at F/C/comma13..14 and P11..12,
where Initech currently refuses. At 14 decimals it produces
`1234.56789123450005`; it also preserves a minus on tiny negative zero.
The requested blanket 0..15 success conflicts with these real goldens.
No B03 implementation or oracle was changed pending that decision.

Reproduce in a scratch directory inside the worktree:

1. Build `mint_i123_numeric.c` with `-Ios/i123/core -Ispec`, the C files in
   `os/i123/core`, and `os/i123/fs/wk1.c`. Run it with `entry SEEDENT.WK1`
   and `format SEEDFMT.WK1` into a copy of the corpus `mint/work_ref`.
2. Start DOSBox-X with `videodriver=dummy`, `output=surface`, `machine=svga_s3`,
   `cputype=386`, `cycles=max`, a captures directory, and mount that copy as C.
3. Before `dx-capture /V 123` in autoexec, schedule
   `autotype -w 4 -p 0.2 esc slash f r s e e d e n t enter`
   (use `s e e d f m t` for formatting). The real automatic macro saves
   NUMENTRY or NUMFMT. Add comma pauses, PageDown/PageUp, and `/qy` as needed
   to capture both pages and end the program. Use `SDL_VIDEODRIVER=dummy`
   and `SDL_AUDIODRIVER=dummy`; no X server or socket is needed.
