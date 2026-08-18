{$MODE DELPHI}{$B+}{$H-}
{ array.pas -- B5: static arrays (array[lo..hi] of T) + indexed access as
  both l-value and r-value (beads initech-54uu, B5 of
  docs/plans/TPS-M7-subset-plan.md; ADR-0007 DEC-02 "static arrays
  (array[lo..hi] of T), indexed as both l-value and r-value").

  Golden provenance: HAND-COMPUTED from Pascal semantics (ISO 7185 / Turbo
  Pascal), per the bool.pas/control.pas/char.pas/func.pas precedent -- not
  FPC-minted. Each check is a distinctive TAG=<value> pair with write() (no
  newline) so the whole fixture is one line and the QEMU harness's --expect
  substring match is unambiguous.

  CODEGEN UNDER TEST (codegen.c gen_elem_addr/gen_index_byteoff): element
  address = base + (index - lo) * stride, stride a UNIFORM 4 bytes for every
  element type (integer/boolean/char alike -- see codegen.c's DECISION note
  on why char arrays do NOT byte-pack here, and the forward-looking note
  toward B7 strings).

  Coverage (one clause per required B5 construct):
    a[]        -- a GLOBAL array (.bss block), CONST-FOLDED bound (N), fill
                  + sum (SUM), then REVERSED in place via a `var`-parameter
                  SWAP whose arguments are INDEXED ELEMENTS (SwapElem(a[1],
                  a[5]) etc.) -- proves "passing a[i] to a VAR param passes
                  its element address" (gen_addr_of's AST_INDEX branch).
    NESTED     -- a[Idx(2)]: the INDEX EXPRESSION is itself a function call
                  (nested indexing per the bead: "i is any integer expr incl.
                  function calls").
    local_arr[] (inside FillSum) -- a LOCAL array: frame-resident, exercises
                  the extended frame-slot allocator (cg_build_scope) and the
                  [ebp-...] element addressing math that composes with B4's
                  frame-offset deep-bug locus (SEED_MUT_CODEGEN_FRAME_OFF4
                  still corrupts an array local's base exactly as it
                  corrupts a scalar local's -- see codegen.c's gen_elem_addr
                  comment).
    b[]        -- a GLOBAL array with LOWER BOUND 0 (proves the (index - lo)
                  subtraction is not a no-op skipped when lo happens to be 1).
    c[]        -- a GLOBAL array with a NEGATIVE lower bound (-2..2), proving
                  `sub eax, lo` with a negative immediate (equivalent to an
                  add) still addresses correctly.
    flags[]    -- a BOOLEAN array (uniform 4-byte stride, same as integer/
                  char -- codegen does not special-case element type).
    letters[]  -- a CHAR array, scanned with a B3 char relational compare
                  (`letters[sidx] = 'B'`) inside a boolean-flag while-loop
                  (the DEC-03 canonical idiom -- ADR-0007 DEC-03 -- avoiding
                  any compound short-circuit-shaped guard).

  Expected exact serial:
    FILLSUM=55 SUM=15 REV=54321 NESTED=4 BSUM=510 CSUM=10 FLAGSUM=2 SCANPOS=4
}
program ArrayTest;
const
  N = 5; { const-folded array bound -- parser.c's parse_array_bound }
var
  a: array[1..N] of integer;   { global; const-folded bound }
  b: array[0..4] of integer;   { global; lower bound 0 }
  c: array[-2..2] of integer;  { global; NEGATIVE lower bound }
  letters: array[1..6] of char;
  flags: array[1..3] of boolean;
  i: integer;
  s, rev, nested, bsum, csum, flagsum, scanpos: integer;
  sidx: integer;
  found: boolean;

{ Nested-indexing helper: the INDEX EXPRESSION a[Idx(2)] is a function call,
  not just a bare variable/literal. }
function Idx(n: integer): integer;
begin
  Idx := n
end;

{ A two-`var`-parameter swap (identical shape to func.pas's Swap), called
  below with INDEXED ARRAY ELEMENTS as its arguments -- proves a `var`
  parameter can bind to an array element's address, not just a plain
  variable. }
procedure SwapElem(var x: integer; var y: integer);
var
  t: integer;
begin
  t := x;
  x := y;
  y := t
end;

{ A function with a LOCAL array on its own frame ([ebp-...], contiguous
  slots) -- fills it with squares, then sums it. Exercises the extended
  frame-slot allocator (cg_build_scope) for an array local. }
function FillSum(): integer;
var
  j: integer;
  local_arr: array[1..5] of integer;
  total: integer;
begin
  for j := 1 to 5 do
    local_arr[j] := j * j;
  total := 0;
  for j := 1 to 5 do
    total := total + local_arr[j];
  FillSum := total
end;

begin
  s := FillSum();
  write('FILLSUM='); write(s); write(' ');

  { Fill + sum the GLOBAL array a (const-folded bound N). }
  for i := 1 to N do
    a[i] := i;
  s := 0;
  for i := 1 to N do
    s := s + a[i];
  write('SUM='); write(s); write(' ');

  { Reverse a[1..5] in place via var-parameter SWAPS on INDEXED ELEMENTS. }
  SwapElem(a[1], a[5]);
  SwapElem(a[2], a[4]);
  rev := a[1] * 10000 + a[2] * 1000 + a[3] * 100 + a[4] * 10 + a[5];
  write('REV='); write(rev); write(' ');

  { Nested indexing: the index expression is a function CALL. }
  nested := a[Idx(2)];
  write('NESTED='); write(nested); write(' ');

  { GLOBAL array with lower bound 0. }
  for i := 0 to 4 do
    b[i] := i + 100;
  bsum := 0;
  for i := 0 to 4 do
    bsum := bsum + b[i];
  write('BSUM='); write(bsum); write(' ');

  { GLOBAL array with a NEGATIVE lower bound. }
  for i := -2 to 2 do
    c[i] := i * i;
  csum := 0;
  for i := -2 to 2 do
    csum := csum + c[i];
  write('CSUM='); write(csum); write(' ');

  { BOOLEAN array: uniform 4-byte stride, same addressing as integer/char. }
  flags[1] := true;
  flags[2] := false;
  flags[3] := true;
  flagsum := ord(flags[1]) + ord(flags[2]) + ord(flags[3]);
  write('FLAGSUM='); write(flagsum); write(' ');

  { CHAR array scan with a B3 char relational compare, boolean-flag while
    idiom (DEC-03 canonical -- no compound short-circuit-shaped guard). }
  letters[1] := 'T'; letters[2] := 'U'; letters[3] := 'R';
  letters[4] := 'B'; letters[5] := 'O'; letters[6] := '!';
  sidx := 1;
  found := false;
  while not found do
  begin
    if letters[sidx] = 'B' then
      found := true
    else
      sidx := sidx + 1
  end;
  scanpos := sidx;
  write('SCANPOS='); writeln(scanpos)
end.
