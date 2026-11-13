{$MODE DELPHI}{$B+}
{ record_shared.pas -- DEC-07 Rung 2 SHARED-SUBSET corpus, the B6 backfill
  (beads initech-4yvg, retrofitting initech-rug7; ADR-0007 Sec 4.7 Rung 2).
  Compiled by BOTH Turbo Initech's seed AND Free Pascal (`fpc`);
  test-seed-fpc-diff asserts their stdout is BYTE-IDENTICAL.

  Line 1 directives (ordinary brace comments to the seed): dollar-MODE-DELPHI
  pins fpc's `integer` to 32-BIT signed per ADR-0007 DEC-02 dialect pin 1
  (fpc's DEFAULT mode is 16-bit -- see array_shared.pas for the latent
  divergence this corpus-wide pin closes); dollar-B-plus (AFTER the mode
  directive, which resets it) pins COMPLETE boolean evaluation (DEC-03).

  SHARED-OUTPUT ENVELOPE: plain-decimal integers, uppercase TRUE/FALSE
  booleans, string literals only; no width formatting, no reals, no char
  output. The record deliberately carries integer/boolean fields only (the
  Rung-1 fixture's char field is covered there; char OUTPUT is outside the
  shared envelope). ONE output line, final item via writeln.

  NOTE the two compilers' record LAYOUTS differ (the seed packs one uniform
  dword per scalar field; fpc packs natively) -- that is fine and exactly the
  point of a stdout differential: the OBSERVABLE semantics (field
  access/copy/aliasing), not the byte layout, must agree.

  B6 constructs exercised (mirroring the Rung-1 fixture
  seed/examples/arith/record.pas): a named record type (the subset's only
  type constructor), field assignment + read on a scalar record, a
  var-parameter of record type mutating the CALLER's record in place,
  WHOLE-RECORD assignment with member-wise-copy semantics (mutating the copy
  leaves the source untouched), an array-of-record with a DIRECT nested
  l-value/r-value (toks[i].field on both sides), and a record-array walk
  inside a function.

  Expected stdout (both compilers, hand-derivable):
    t = (7,40,FALSE); Bump adds 10, sets ok -> BKIND=17 BOK=TRUE;
    t2 := t then t2.kind := 99 -> T2KIND=99, t untouched TKIND=17;
    toks[i].kind = 3,6,9; toks[2].val := 3+9 -> T2VAL=12;
    KindSum(100) = 100+3+6+9 -> KSUM=118.
  "KIND=7 VAL=40 OK=FALSE BKIND=17 BOK=TRUE T2KIND=99 TKIND=17 T2VAL=12 KSUM=118\n" }
program RecordShared;
type
  Token = record
    kind: integer;
    val: integer;
    ok: boolean;
  end;
var
  t, t2: Token;
  toks: array[1..3] of Token;
  i: integer;

procedure Bump(var tk: Token);
begin
  tk.kind := tk.kind + 10;
  tk.ok := true
end;

function KindSum(base: integer): integer;
var
  k, s: integer;
begin
  s := base;
  k := 1;
  while k <= 3 do
  begin
    s := s + toks[k].kind;
    k := k + 1
  end;
  KindSum := s
end;

begin
  t.kind := 7;
  t.val := 40;
  t.ok := false;
  write('KIND=');
  write(t.kind);
  write(' VAL=');
  write(t.val);
  write(' OK=');
  write(t.ok);

  Bump(t);
  write(' BKIND=');
  write(t.kind);
  write(' BOK=');
  write(t.ok);

  t2 := t;
  t2.kind := 99;
  write(' T2KIND=');
  write(t2.kind);
  write(' TKIND=');
  write(t.kind);

  i := 1;
  while i <= 3 do
  begin
    toks[i].kind := i * 3;
    toks[i].val := 0;
    toks[i].ok := false;
    i := i + 1
  end;
  toks[2].val := toks[1].kind + toks[3].kind;
  write(' T2VAL=');
  write(toks[2].val);

  write(' KSUM=');
  writeln(KindSum(100))
end.
