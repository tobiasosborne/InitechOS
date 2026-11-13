{$MODE DELPHI}{$B+}
{ array_shared.pas -- DEC-07 Rung 2 SHARED-SUBSET corpus, the B5 backfill
  (beads initech-4yvg, retrofitting initech-54uu; ADR-0007 Sec 4.7 Rung 2).
  Compiled by BOTH Turbo Initech's seed AND Free Pascal (`fpc`);
  test-seed-fpc-diff asserts their stdout is BYTE-IDENTICAL.

  Line 1 directives (ordinary brace comments to the seed): dollar-MODE-DELPHI
  pins fpc's `integer` to 32-BIT signed per ADR-0007 DEC-02 dialect pin 1 --
  fpc's DEFAULT mode makes `integer` 16-bit, and THIS fixture is the one that
  exposed it (REV=54321 wrapped to -11215 unpinned; the earlier corpus never
  exceeded 32767, so the divergence was latent). dollar-B-plus (AFTER the
  mode directive, which resets it) pins COMPLETE boolean evaluation (DEC-03).

  SHARED-OUTPUT ENVELOPE (the Makefile's test-seed-fpc-diff header): only
  plain-decimal integers via write(int), booleans as fpc's default uppercase
  TRUE/FALSE, and string literals. NO width formatting, NO reals, NO char
  output (chars are used internally only, never written). ONE output line,
  final item via writeln.

  B5 constructs exercised (mirroring the Rung-1 fixture
  seed/examples/arith/array.pas): a global array with a CONST-FOLDED bound
  (fill + sum), in-place reversal via a var-param swap whose arguments are
  INDEXED ELEMENTS (element address passed, not a copy), a function-call
  INDEX EXPRESSION, a lower-bound-0 array and a NEGATIVE-lower-bound array
  (the (index - lo) rebase both directions), a boolean array, and a LOCAL
  array inside a function (frame-resident element addressing).

  Expected stdout (both compilers, hand-derivable):
    a = [1..5] -> SUM=15; reversed -> 54321; Idx(2)=4 -> a[4]=2;
    b[i]=100+i, i=0..4 -> BSUM=510; c[i]=i+4, i=-2..2 -> CSUM=20;
    flags -> F1=TRUE F2=FALSE; LocalSum(3)=3*(1+..+5) -> LSUM=45.
  "SUM=15 REV=54321 NESTED=2 BSUM=510 CSUM=20 F1=TRUE F2=FALSE LSUM=45\n" }
program ArrayShared;
const
  N = 5;
var
  a: array[1..N] of integer;
  b: array[0..4] of integer;
  c: array[-2..2] of integer;
  flags: array[1..3] of boolean;
  i, s, rev, bsum, csum: integer;

function Idx(x: integer): integer;
begin
  Idx := x + 2
end;

procedure SwapElem(var x: integer; var y: integer);
var
  t: integer;
begin
  t := x;
  x := y;
  y := t
end;

function LocalSum(n: integer): integer;
var
  la: array[1..5] of integer;
  k, t: integer;
begin
  k := 1;
  while k <= 5 do
  begin
    la[k] := k * n;
    k := k + 1
  end;
  t := 0;
  k := 1;
  while k <= 5 do
  begin
    t := t + la[k];
    k := k + 1
  end;
  LocalSum := t
end;

begin
  i := 1;
  s := 0;
  while i <= N do
  begin
    a[i] := i;
    s := s + a[i];
    i := i + 1
  end;
  write('SUM=');
  write(s);

  SwapElem(a[1], a[5]);
  SwapElem(a[2], a[4]);
  rev := a[1] * 10000 + a[2] * 1000 + a[3] * 100 + a[4] * 10 + a[5];
  write(' REV=');
  write(rev);

  write(' NESTED=');
  write(a[Idx(2)]);

  i := 0;
  bsum := 0;
  while i <= 4 do
  begin
    b[i] := 100 + i;
    bsum := bsum + b[i];
    i := i + 1
  end;
  write(' BSUM=');
  write(bsum);

  i := -2;
  csum := 0;
  while i <= 2 do
  begin
    c[i] := i + 4;
    csum := csum + c[i];
    i := i + 1
  end;
  write(' CSUM=');
  write(csum);

  flags[1] := true;
  flags[2] := false;
  flags[3] := true;
  write(' F1=');
  write(flags[1]);
  write(' F2=');
  write(flags[2]);

  write(' LSUM=');
  writeln(LocalSum(3))
end.
