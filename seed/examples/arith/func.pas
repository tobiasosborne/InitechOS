{ func.pas -- B4: procedures/functions, stack frames, value + var params,
  recursion, and forward/mutual recursion (beads initech-63ce; ADR-0007
  DEC-02 "procedures/functions: nested scopes, both value and var
  parameters, recursion, results"; the `forward` directive per the DEC-02
  ratification amendment). THE CODEGEN PIVOT: the first fixture that stands
  up real cdecl stack frames (push ebp / mov ebp,esp / sub esp / leave /
  ret), left-to-right params at [ebp+8+4i], locals at [ebp-4k], eax return.

  CALLING CONVENTION (DECISION, report): cdecl. The caller pushes arguments
  RIGHT-TO-LEFT so the FIRST (leftmost) parameter lands at the lowest
  address [ebp+8], parameter i at [ebp+8+4i]; the caller cleans the stack
  after the call (add esp, 4*nargs). A `var` parameter passes the ADDRESS of
  the argument variable; loads/stores through it dereference the pointer
  EVERY time (no cached-in-register staleness). Value parameters pass a
  copy. See seed/codegen.c's frame-model header for the full contract.

  Golden provenance: HAND-COMPUTED from Pascal semantics (ISO 7185 / Turbo
  Pascal), not FPC-minted -- this seed's Rung-1 fixtures are hand-computed
  exact-serial goldens (ADR-0007 DEC-07 Rung 1). The DEC-07 Rung 2 Free
  Pascal differential (test-seed-fpc-diff) grades this SAME source against
  fpc when fpc is installed; the two rungs must agree.

  Expected exact serial:
    FACT5=120 SUMTO=15 EVEN7=FALSE ODD7=TRUE SWAPA=8 SWAPB=3 ALIASG=41 ALIASG2=42 VALINDEP=5 COMPOSE=10

  Coverage (one clause per required B4 construct):
    FACT5    -- direct recursion (recursive factorial), function-in-expr.
    SUMTO    -- a function with LOCAL variables (i, acc) on its frame
                (exercises [ebp-4k] locals; the FRAME_OFF4 mutant target).
    EVEN7/ODD7 -- MUTUAL recursion via a `forward` declaration.
    SWAPA/SWAPB -- a two-`var`-parameter swap (var params write back).
    ALIASG/ALIASG2 -- var-param ALIASING: a GLOBAL is modified THROUGH a var
                parameter and then read directly, twice; proves the callee
                reads/writes through the pointer with no register staleness
                (the VARPARAM_COPY mutant target -- if a var param degraded
                to a value copy, ALIASG would stay 40).
    VALINDEP -- value-parameter independence: the callee mutates its own
                copy; the caller's variable is UNCHANGED.
    COMPOSE  -- function-call composition inside an expression
                (Add(Add(1,2), Add(3,4)) = 10). }

{ BOOLEVAL directive: complete (non-short-circuit) boolean evaluation is
  ADR-0007 DEC-03; this fixture uses only simple guards (no compound
  and/or), so the directive is not load-bearing here, but the DEC-07 Rung 2
  fpc differential compiles the shared corpus under a dollar-B-plus /
  BOOLEVAL-ON directive so both compilers implement DEC-03's semantics --
  documented in the fixture that needs it. }
program FuncTest;
var
  g: integer;       { global -- aliased through a var parameter below }
  a, b: integer;
  r: integer;

{ Direct recursion + function used inside an expression. }
function Fact(n: integer): integer;
begin
  if n <= 1 then
    Fact := 1
  else
    Fact := n * Fact(n - 1)
end;

{ A function with LOCAL variables on its own frame ([ebp-4k]). The `for`
  loop's synthesized limit variable lands as a LOCAL of THIS routine (not a
  program global) so it is frame-resident and re-entrant -- exercising the
  for-loop-inside-a-routine path, not just the top-level for-loop control.pas
  already covers. }
function SumTo(n: integer): integer;
var
  i, acc: integer;
begin
  acc := 0;
  for i := 1 to n do
    acc := acc + i;
  SumTo := acc
end;

{ Mutual recursion via a forward declaration (required by ADR-0007 DEC-02's
  ratification amendment: the parser's own block<->statement mutual recursion
  is inexpressible single-pass without it). }
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

{ Two var parameters + a local temporary: a classic swap. }
procedure Swap(var x: integer; var y: integer);
var
  t: integer;
begin
  t := x;
  x := y;
  y := t
end;

{ One var parameter: increments its argument in place (aliasing proof). }
procedure Bump(var v: integer);
begin
  v := v + 1
end;

{ One value parameter: mutating the copy must NOT affect the caller. }
procedure AddHundredToCopy(n: integer);
begin
  n := n + 100
end;

{ Plain two-argument value-parameter function (composition building block). }
function Add(x: integer; y: integer): integer;
begin
  Add := x + y
end;

begin
  write('FACT5='); write(Fact(5)); write(' ');

  write('SUMTO='); write(SumTo(5)); write(' ');

  write('EVEN7='); write(IsEven(7)); write(' ');
  write('ODD7='); write(IsOdd(7)); write(' ');

  a := 3; b := 8;
  Swap(a, b);
  write('SWAPA='); write(a); write(' ');
  write('SWAPB='); write(b); write(' ');

  g := 40;
  Bump(g);
  write('ALIASG='); write(g); write(' ');
  Bump(g);
  write('ALIASG2='); write(g); write(' ');

  a := 5;
  AddHundredToCopy(a);
  write('VALINDEP='); write(a); write(' ');

  r := Add(Add(1, 2), Add(3, 4));
  write('COMPOSE='); writeln(r)
end.
