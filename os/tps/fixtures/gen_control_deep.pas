{$MODE DELPHI}{$B+}{$H-}
{ control.pas -- if/then/else + while/do (primitive), plus for/repeat (sugar,
  desugared entirely in seed/parser.c). Beads initech-80iw, B2 of
  docs/plans/TPS-M7-subset-plan.md; ADR-0007 DEC-02/DEC-04.

  Golden provenance: HAND-COMPUTED from the Pascal definitions (ISO 7185 /
  Turbo Pascal), per the bool.pas / negative_divmod.pas precedent -- not
  FPC-minted. Each check is a distinctive TAG=<value> pair with write() (no
  newline) so the whole fixture is one line and the QEMU harness's --expect
  substring match is unambiguous.

  Sections below (each TAG hand-computed):

  1. Counted while-loop sum: sum of 1..5.
       SUM = 1+2+3+4+5 = 15

  2. if/else branch selection (a=3, b=5):
       IF1 = (a<b) ? "LT" : "GE"   ; a<b is TRUE   -> LT
       IF2 = (a>b) ? "GT" : "LE"   ; a>b is FALSE  -> LE

  3. Dangling-else (standard: binds to the NEAREST unmatched 'if'):
       if a<b then
         if a>b then write('DE=X') else write('DE=Y')
       -- no else on the OUTER if --
     a<b is TRUE (enter outer), a>b is FALSE (inner's else fires) -> DE=Y.
     If the else had wrongly bound to the OUTER if instead, the inner if
     (a>b -> FALSE, no else) would print nothing at all -- a materially
     different (and wrong) result, so this is a real proof, not a
     coincidence.

  4. Nested while loops: prod = sum over o=1..3 of (sum over j=1..2 of o*j)
       o=1: 1*1 + 1*2 = 3   (running prod=3)
       o=2: 2*1 + 2*2 = 6   (running prod=9)
       o=3: 3*1 + 3*2 = 9   (running prod=18)
       NEST = 18

  5. for ... to: x = 1+2+3+4 = 10           -> FORTO=10
  6. for ... downto: y = 4+3+2+1 = 10       -> FORDOWN=10

  7. for-bounds-evaluated-ONCE (ISO 7185 Sec 6.8.3.9 / Turbo Pascal Language
     Guide "for statement": initial/final expressions are evaluated once, at
     loop entry -- see parser.h's for-stmt grammar note): the loop mutates
     the variable (n) the bound expression referenced, from inside the
     body. If the bound were (wrongly) re-evaluated every iteration instead
     of being captured once, this loop would never terminate normally in
     any small bounded way (n grows by 100 each pass) and rc would NOT be
     3. Bound captured once at entry (n=3 at that instant) -> exactly 3
     iterations regardless of n's later mutation.
       FORONCE = 3

  8. repeat-until (body is a statement LIST, no begin/end): sum of 1..3,
     tested AFTER the body (so it always runs at least once).
       REPEAT = 1+2+3 = 6
}
program ControlFixture;
var
  i, n, sum : integer;
  a, b : integer;
  outer, inner, prod : integer;
  x, y : integer;
  rc : integer;
begin
  { 1. counted while-loop sum }
  i := 1; sum := 0; n := 5;
  while i <= n do
  begin
    sum := sum + i;
    i := i + 1
  end;
  write('SUM='); write(sum); write(' ');

  { 2. if/else branch selection }
  a := 3; b := 5;
  if a < b then
    write('IF1=LT')
  else
    write('IF1=GE');
  write(' ');

  if a > b then
    write('IF2=GT')
  else
    write('IF2=LE');
  write(' ');

  { 3. dangling else binds to the nearest unmatched if }
  if a < b then
    if a > b then
      write('DE=X')
    else
      write('DE=Y');
  write(' ');

  { 4. nested while loops }
  outer := 1; prod := 0;
  while outer <= 3 do
  begin
    inner := 1;
    while inner <= 2 do
    begin
      prod := prod + outer * inner;
      inner := inner + 1
    end;
    outer := outer + 1
  end;
  write('NEST='); write(prod); write(' ');

  { 5. for .. to }
  x := 0;
  for i := 1 to 4 do
    x := x + i;
  write('FORTO='); write(x); write(' ');

  { 6. for .. downto }
  y := 0;
  for i := 4 downto 1 do
    y := y + i;
  write('FORDOWN='); write(y); write(' ');

  { 7. for-bounds-evaluated-once: mutating n inside the body must not
        change the number of iterations }
  rc := 0; n := 3;
  for i := 1 to n do
  begin
    rc := rc + 1;
    n := n + 100
  end;
  write('FORONCE='); write(rc); write(' ');

  { 8. repeat-until: statement list, no begin/end, guard tested after }
  i := 1; sum := 0;
  repeat
    sum := sum + i;
    i := i + 1
  until i > 3;
  write('REPEAT='); writeln(sum)
end.
