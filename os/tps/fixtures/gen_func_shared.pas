{$MODE DELPHI}{$B+}
{ func_shared.pas -- DEC-07 Rung 2 SHARED-SUBSET corpus (beads initech-63ce;
  ADR-0007 Sec 4.7 Rung 2). Compiled by BOTH Turbo Initech's seed AND Free
  Pascal (`fpc`); test-seed-fpc-diff asserts their stdout is BYTE-IDENTICAL.

  Line 1 directives (the seed treats a brace-delimited region as an ordinary
  comment, so both are harmless no-ops on the seed side): dollar-MODE-DELPHI
  pins fpc's `integer` to 32-BIT signed per ADR-0007 DEC-02 dialect pin 1 --
  fpc's DEFAULT mode makes `integer` 16-bit, a latent divergence the B5
  backfill fixture exposed (array_shared.pas has the story); dollar-B-plus
  (AFTER the mode directive, which resets boolean-eval state) turns on
  COMPLETE boolean evaluation (BOOLEVAL ON), so fpc implements ADR-0007
  DEC-03's semantics like the seed does (this program uses only simple
  guards anyway).

  SHARED-OUTPUT ENVELOPE (documented in the Makefile's test-seed-fpc-diff
  header): this program emits ONLY values whose textual form is identical
  between the seed's serial RTL (seed/rt/start.asm) and fpc's writeln --
  plain-decimal integers (write(int), no field width), booleans as
  TRUE/FALSE (fpc's default uppercase), and string literals. It deliberately
  avoids width/precision formatting (write(x:n)), reals (not in the subset),
  and char output (CP437-vs-console divergence risk). Everything is written on
  ONE line ending in a single writeln so the expected output is a single line
  ending in exactly one newline.

  B4 constructs exercised: recursion (Fact), mutual recursion via `forward`
  (IsEven/IsOdd), a two-`var`-parameter swap, and function-call-in-expression
  and boolean results.

  Expected stdout (both compilers): "120 TRUE FALSE 2 1\n" }
program FuncShared;
var
  a, b: integer;

function Fact(n: integer): integer;
begin
  if n <= 1 then
    Fact := 1
  else
    Fact := n * Fact(n - 1)
end;

function IsEven(n: integer): boolean; forward;

function IsOdd(n: integer): boolean;
begin
  if n = 0 then
    IsOdd := false
  else
    IsOdd := IsEven(n - 1)
end;

function IsEven(n: integer): boolean;
begin
  if n = 0 then
    IsEven := true
  else
    IsEven := IsOdd(n - 1)
end;

procedure Swap(var x: integer; var y: integer);
var
  t: integer;
begin
  t := x;
  x := y;
  y := t
end;

begin
  write(Fact(5)); write(' ');
  write(IsEven(6)); write(' ');
  write(IsOdd(6)); write(' ');
  a := 1; b := 2;
  Swap(a, b);
  write(a); write(' ');
  writeln(b)
end.
