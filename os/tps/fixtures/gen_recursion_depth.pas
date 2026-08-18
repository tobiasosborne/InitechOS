{$MODE DELPHI}{$B+}{$H-}
{ B9.5 focused execution fixture for the B4 recursion-depth hole reported by
  B9.4. DepthSum(12) creates thirteen live activations (12 through 0), and
  each activation keeps its local Held live across the recursive call.

  The two boolean expressions also make ADR-0007 DEC-03 complete evaluation
  observable now that a side-effecting function exists: Touch must run even
  behind false AND and true OR. The explicit dollar-B-plus pin gives FPC the
  same complete-evaluation semantics as Turbo Initech.

  Golden provenance: HAND-COMPUTED from Pascal semantics, not FPC-minted.
    DepthSum(12) = 12 + 11 + ... + 1 + 0 = 78.
    false and true = FALSE; true or true = TRUE.
    Complete evaluation calls Touch(5) and Touch(7), so Touches = 12.
    Pascal div truncates toward zero: (-7) div 2 = -3; the remainder keeps
    the dividend sign, so (-7) mod 2 = -1.
  Expected exact stdout:
    DEPTH=78 AND=FALSE OR=TRUE TOUCH=12 DIV=-3 MOD=-1
}
program RecursionDepth;
var
  Touches: integer;
  AndResult, OrResult: boolean;

function DepthSum(N: integer): integer;
var
  Held: integer;
begin
  Held := N;
  if N = 0 then
    DepthSum := Held
  else
    DepthSum := Held + DepthSum(N - 1)
end;

function Touch(Delta: integer): boolean;
begin
  Touches := Touches + Delta;
  Touch := true
end;

begin
  Touches := 0;
  AndResult := false and Touch(5);
  OrResult := true or Touch(7);
  write('DEPTH='); write(DepthSum(12));
  write(' AND='); write(AndResult);
  write(' OR='); write(OrResult);
  write(' TOUCH='); write(Touches);
  write(' DIV='); write((-7) div 2);
  write(' MOD='); writeln((-7) mod 2)
end.
