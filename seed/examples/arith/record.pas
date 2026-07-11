{ record.pas -- B6: records + field access (incl. array-of-record),
  deterministic layout (beads initech-rug7, B6 of
  docs/plans/TPS-M7-subset-plan.md; ADR-0007 DEC-02 "records (record ... end),
  field access, arrays of records, with a deterministic field layout").

  DEEP-BUG LOCUS (the committee's own framing, beads initech-rug7): field-
  offset/padding math, and its composition with B5's array-element stride
  when the array's element type IS a record (array-of-record).

  LAYOUT RULE UNDER TEST (seed/codegen.c's file-header comment has the full
  worked story; seed/ast.h's B6 AST_TYPEDECL comment has the design note):
  fields occupy CONSECUTIVE UNIFORM 4-byte slots in DECLARATION ORDER --
  field k is at byte offset 4*k from the record's base address, and
  sizeof(record) = 4 * field-count (the identical "one dword per scalar, no
  packing" convention B1/B3/B5 already use for booleans/chars/array
  elements, extended one level). An array-of-record ELEMENT then uses the
  record's TOTAL SIZE (4*field-count) as its element STRIDE -- B5's existing
  "base + (index-lo)*stride" formula composes UNCHANGED, just fed a bigger
  stride.

  Golden provenance: HAND-COMPUTED from Pascal semantics (ISO 7185 / Turbo
  Pascal) and this seed's own documented layout rule, per the bool.pas/
  control.pas/char.pas/func.pas/array.pas precedent -- not FPC-minted. Each
  check is a distinctive TAG=<value> pair with write() (no newline) so the
  whole fixture is one line and the QEMU harness's --expect substring match
  is unambiguous.

  Coverage (one clause per required B6 construct):
    Token record  -- a `type Token = record kind: integer; ch: char;
                  ok: boolean; end;` (the ONLY type-constructor in this
                  subset, ADR-0007 DEC-02's own text; a Token-like record
                  mirroring what the compiler's own future source needs).
    KIND/CH/OK    -- pack (field assignment, r.f := v) then unpack (field
                  read, r.f) a single scalar record variable.
    LOCALREC      -- a FUNCTION with a LOCAL scalar record (lt: Token) AND a
                  LOCAL array-of-record (larr: array[1..3] of Token),
                  exercising the frame-offset math (composes with B4's own
                  deep-bug locus -- cg_build_scope's contiguous slot
                  allocation for a record-typed local).
    BKIND/BOK     -- a `var`-parameter of RECORD type (Bump(var tk: Token))
                  mutating the CALLER's record in place (the record's base
                  ADDRESS is passed, not a copy -- needed for symbol-table-
                  style helpers).
    T2KIND/T2CH   -- WHOLE-RECORD assignment (t2 := t) -- a member-wise copy,
                  not aliasing.
    TKIND/T2KIND2 -- mutating t2 AFTER the whole-record copy leaves t
                  UNCHANGED (proves member-wise COPY semantics, not a shared
                  reference).
    NESTED        -- a DIRECT nested l-value/r-value: `toks[1].kind := ...`
                  (array-element-then-field), NOT via a var-parameter helper
                  -- the deep-bug intersection itself (B5 stride composing
                  with B6 field offset).
    SUMKIND       -- array-of-record FILL (SetTok, a `var`-parameter of
                  RECORD type called with an INDEXED ELEMENT argument
                  `toks[i]` -- composing B5's indexed-var-param-argument
                  addressing with B6's record var-param addressing) + SCAN
                  (`toks[i].kind` read in a B2 for-loop).
    FIRSTOK       -- a boolean-flag while-loop (DEC-03 canonical idiom) that
                  scans `toks[i].ok` to find the first true index.
    SCANCH        -- a char field read off an array-of-record element.

  Expected exact serial:
    LOCALREC=607 KIND=42 CH=X OK=TRUE BKIND=43 BOK=FALSE T2KIND=43 T2CH=X
    TKIND=43 T2KIND2=999 NESTED=1010 SUMKIND=1150 FIRSTOK=2 SCANCH=C
}
program RecordTest;

type
  Token = record
    kind: integer;
    ch: char;
    ok: boolean;
  end;

var
  t, t2: Token;
  toks: array[1..5] of Token;
  i, sumkind, firstok: integer;
  scanch: char;

{ A LOCAL scalar record (lt) AND a LOCAL array-of-record (larr) on the SAME
  frame -- exercises the extended frame-slot allocator for BOTH record
  shapes at once (composes with B4's frame-offset deep-bug locus). }
function LocalRecTest(): integer;
var
  lt: Token;
  larr: array[1..3] of Token;
  j, total: integer;
begin
  lt.kind := 7;
  lt.ch := 'Z';
  lt.ok := false;
  for j := 1 to 3 do
  begin
    larr[j].kind := j * 100;
    larr[j].ch := chr(ord('P') + j);
    larr[j].ok := true
  end;
  total := lt.kind;
  for j := 1 to 3 do
    total := total + larr[j].kind;
  LocalRecTest := total
end;

{ A `var` parameter of RECORD type: mutates the CALLER's record in place
  (the record's base ADDRESS is passed -- proves "var parameters of record
  type" works, not just var parameters of scalar type). }
procedure Bump(var tk: Token);
begin
  tk.kind := tk.kind + 1;
  tk.ok := not tk.ok
end;

{ A `var` parameter of RECORD type, called with an INDEXED ARRAY ELEMENT
  argument (SetTok(toks[i], ...)) -- the deep-bug intersection the bead
  flags: B5's "pass a[i] to a var param" composed with B6's "var param of
  record type". }
procedure SetTok(var tk: Token; k: integer; c: char; o: boolean);
begin
  tk.kind := k;
  tk.ch := c;
  tk.ok := o
end;

begin
  write('LOCALREC='); write(LocalRecTest()); write(' ');

  { pack/unpack a single scalar record. }
  t.kind := 42;
  t.ch := 'X';
  t.ok := true;
  write('KIND='); write(t.kind); write(' ');
  write('CH='); write(t.ch); write(' ');
  write('OK='); write(t.ok); write(' ');

  { var-param (record) mutation. }
  Bump(t);
  write('BKIND='); write(t.kind); write(' ');
  write('BOK='); write(t.ok); write(' ');

  { whole-record assignment (member-wise copy). }
  t2 := t;
  write('T2KIND='); write(t2.kind); write(' ');
  write('T2CH='); write(t2.ch); write(' ');

  { mutate t2 independently; t must stay unchanged (copy, not aliasing). }
  t2.kind := 999;
  write('TKIND='); write(t.kind); write(' ');
  write('T2KIND2='); write(t2.kind); write(' ');

  { array-of-record: fill via a var-param-of-record call with an INDEXED
    element argument, B2 for-loop. }
  for i := 1 to 5 do
    SetTok(toks[i], i * 10, chr(ord('A') + i - 1), (i mod 2 = 0));

  { direct nested l-value/r-value: arr[i].field := expr (no var-param
    helper) -- the deep-bug intersection itself. }
  toks[1].kind := toks[1].kind + 1000;
  write('NESTED='); write(toks[1].kind); write(' ');

  { scan: sum kinds, find first index where ok = true. }
  sumkind := 0;
  for i := 1 to 5 do
    sumkind := sumkind + toks[i].kind;
  write('SUMKIND='); write(sumkind); write(' ');

  firstok := 0;
  i := 1;
  while (firstok = 0) do
  begin
    if toks[i].ok then
      firstok := i
    else
      i := i + 1
  end;
  write('FIRSTOK='); write(firstok); write(' ');

  scanch := toks[3].ch;
  write('SCANCH='); writeln(scanch)
end.
