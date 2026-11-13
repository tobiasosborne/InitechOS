# Real Lotus R2.2 Copy / POINT evidence (B11, initech-fyji)

These WK1 files were saved by the real Lotus 1-2-3 Release 2.2 executable
under DOSBox-X 2024.03.01, from a scratch COPY of lotus123-decomp/mint/work_ref.
The decomp tree was not modified. SHA256SUMS identifies the executable and
saves. The PNGs are unmodified frames extracted from DX-CAPTURE's AVI output.
The .ref files are independent lotus123-decomp/tools/wk1_ref.py decodes.

Ground truth: Lotus R2.2 Reference /Copy pp.3-4/5 (PDF 72/73), Specifying a
Range pp.1-24/25 (PDF 40/41), and these real-executable results:

- SINGLE.WK1: A1=7 copied to D1..D3. All three destination cells are 7.
  Originally saved as TILE1.WK1 and renamed here; single.steps reproduces it.
- ROW.WK1: A1=11, B1=22, FROM A1..B1, TO F1..H3. F/G repeat down three
  rows. H remains blank. A non-singleton source axis does NOT tile across
  the requested TO extent. tile.steps captures this nonmultiple case.
- OVER.WK1: continuing ROW, copy A1..B1 to A1..D2. A/B repeat down to row 2;
  C/D remain blank, F/G remain as in ROW. Overlap reads the original source.
- BLOCK.WK1: A1=1, B1=2, A2=3, B2=4; copy A1..B2 to F1..I4. Only F1..G2
  is written: a multi-row, multi-column source copies once (block.steps).
- point.steps: Copy automatically shows A1..A1 (auto_anchor.png). Escape
  unanchors (unanchor.png). Right, period, Down, Right highlights B1..C2
  (point_range.png); Escape returns to B1 (escape.png), despite the manual's
  ambiguous "without moving it" wording. Period, Down, Right, Backspace
  restores the original POINT entry cell A1 (backspace.png).

Reproduce with the corpus tools/mint_drive.sh and these steps on a private
Xvfb display, as fixtures/i123-width does. This sandbox cannot create X11
sockets, so the actual mint used SDL's dummy driver and DX-CAPTURE instead:

1. Copy mint/work_ref into a scratch directory with work/ and capture/.
2. Compile harness/diff/wk1_diff/mint_i123_copy_steps.c (factory C).
3. Run it with a steps file and the ABSOLUTE scratch directory; redirect
   stdout to a .conf file. Run each steps file in a separate DOSBox-X launch:
   SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy dosbox-x -conf FILE -nogui -exit
4. Extract stable PNG frames with ffmpeg. The driver's stderr times are
   approximate; key press/release and video startup add delay. For the
   point run, stable frames at seconds 5,7,9,11,13 supplied the five PNGs.
   Fresh work directories avoid Lotus Replace prompts. Run tile.steps once
   for both ROW and OVER. The input builder does not generate golden cells.

The implementation is graded with test-i123-copy-tile and the shared
independent saved-cell reader in test-flair-i123-copy[-cpu/-bochs]. Existing
COPY01/MOVE01/WRAP01 differentials still grade mixed/absolute references.
No Initech output was used as an expected file or cell value.
