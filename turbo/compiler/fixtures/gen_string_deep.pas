{$MODE DELPHI}{$B+}{$H-}
{ string.pas -- B7: fixed/ShortString strings (length/index/compare/concat)
  + frame-resident temporaries (beads initech-39k2, B7 of
  docs/plans/TPS-M7-subset-plan.md; ADR-0007 DEC-02 "minimal fixed/ShortString-
  style strings (length/index/compare/concat) -- not dynamic/heap strings",
  DEC-05 "int-to-decimal-string is hand-written in-subset Pascal ... the RTL
  stays byte/block I/O only").

  Binding design: B7-design-record.md (committee 2026-07-14, 3 seats + chair
  synthesis). THE CHAIR'S CORRECTION (D4, binding): in THIS subset NO string
  temp is ever live across a call (string function results are rejected and a
  concat is never a var-arg), so the temp-lifetime deep bug is
  TWO-LIVE-TEMPS-IN-ONE-STATEMENT (a RIGHT-NESTED concat a+(b+c)), NOT a
  clobber-across-recursion. The RNEST clause below is that load-bearing bite:
  its golden differs the moment the two simultaneously-live temps collapse to
  one. A recursive clause (RECUR) is included as belt (frame locals + temps
  under activation depth), not as the load-bearing bite.

  DEEP-BUG LOCUS (the committee's own framing, D4/D10): string-temporary
  lifetime (right-nested concat needs a SECOND simultaneously-live temp) and
  the __str_cmp length tiebreak (a prefix-equal compare decides on length).

  Golden provenance: HAND-COMPUTED from ShortString semantics (ISO 7185 /
  Turbo Pascal: silent truncation to the declared capacity on assignment,
  concat clamped at 255, unsigned bytewise compare with the shorter string
  ordering before the longer when one is a prefix of the other) and this
  seed's own documented layout/codegen -- NOT fpc-minted (the fpc leg is
  string_shared.pas). Each check is a distinctive TAG=<value> pair with
  write() (no newline) so the whole fixture is one line and the QEMU
  harness's --expect substring match is unambiguous.

  Coverage (D10):
    LEN0      -- literal assign of '' (empty string) + length() == 0.
    CONSTSTR  -- a string CONST declaration (D1) folded to a literal at use.
    TRUNC/TLEN-- silent truncation assigning 'abcdef' to a string[3].
    LDEEP     -- a LEFT-DEEP concat chain p1+p2+p3 (one accumulator temp).
    RNEST     -- the RIGHT-NESTED concat q1+(q2+q3): the load-bearing
                 two-live-temps clause (collapse-to-one corrupts it).
    CCAT      -- write() of a concat EXPRESSION directly (temp -> __str_write).
    SELFA     -- self-alias s := s + c (char coercion in a concat).
    PRE       -- prepend 'a' + s (leftmost char coercion inits the accumulator).
    EQ..GE    -- all six relops incl. prefix ('ab' vs 'abc') + equal + a FALSE.
    CMPCH     -- a string = 'a' compare (char coercion of a compare operand).
    LENV/LENP -- length() on a var and on a var-string PARAMETER.
    IDXR/IDXW -- s[i] read (char r-value) AND write (byte l-value), in a
                 program that ALSO indexes a B5 integer array (IDXA -- the
                 cross-talk guard: a string byte-index must NOT dword-load and
                 an array index must NOT byte-load).
    RECUR     -- recursive var-string accumulation (belt): frame locals +
                 temps under activation depth (BuildDown appends a digit and
                 recurses).
    MUT       -- var-string-parameter mutation through a call (AppendBang
                 appends, the caller observes the change).

  Expected exact serial:
    LEN0=0 CONSTSTR=Hi TRUNC=abc TLEN=3 LDEEP=abcdef RNEST=ghijkl CCAT=abcd
    SELFA=abc PRE=aZ EQ=TRUE EQF=FALSE NE=TRUE LTP=TRUE LTB=TRUE LE=TRUE
    GT=TRUE GE=TRUE CMPCH=TRUE LENV=3 LENP=3 IDXR=Y IDXW=WYZ IDXA=77
    RECUR=321 MUT=go!
}
program StringTest;

const
  Greeting = 'Hi';

var
  e0: string;
  t3: string[3];
  p1, p2, p3: string;
  q1, q2, q3: string;
  ld, rn: string;
  sa, pre: string;
  u_ab, u_ab2, u_abc, u_abd: string;
  onech: string;
  si: string;
  iarr: array[1..3] of integer;
  acc, mm, gc: string;

{ Recursive var-string accumulation (belt -- D4). Appends the digit chr('0'+n)
  then recurses; string function results are rejected, so accumulation is
  through a `var string` parameter (the D6 idiom). No string temp is ever live
  across the recursive call -- the concat completes into acc before BuildDown
  is called again. }
procedure BuildDown(n: integer; var s: string);
begin
  if n > 0 then
  begin
    s := s + chr(ord('0') + n);
    BuildDown(n - 1, s)
  end
end;

{ length() on a var-string PARAMETER (the address is passed; length reads its
  byte 0). A `var string` param + an integer result is legal; a string RESULT
  would be rejected. }
function StrLen(var s: string): integer;
begin
  StrLen := length(s)
end;

{ var-string-parameter mutation through a call: the callee appends '!' and the
  caller observes it (the record.pas Bump precedent, for strings). }
procedure AppendBang(var s: string);
begin
  s := s + '!'
end;

begin
  { empty-string literal assign + length. }
  e0 := '';
  write('LEN0='); write(length(e0)); write(' ');

  { a string CONST folded to a literal at its use site (D1). }
  gc := Greeting;
  write('CONSTSTR='); write(gc); write(' ');

  { silent truncation to a string[3]. }
  t3 := 'abcdef';
  write('TRUNC='); write(t3); write(' ');
  write('TLEN='); write(length(t3)); write(' ');

  p1 := 'ab'; p2 := 'cd'; p3 := 'ef';
  q1 := 'gh'; q2 := 'ij'; q3 := 'kl';

  { LEFT-DEEP chain: one accumulator temp (in-place accumulation). }
  ld := p1 + p2 + p3;
  write('LDEEP='); write(ld); write(' ');

  { RIGHT-NESTED: the load-bearing two-simultaneously-live-temps clause. A
    collapse-to-one-temp mutant evaluates (q2+q3) into the SAME temp holding
    q1's copy, so the golden 'ghijkl' becomes 'ijklijkl'. }
  rn := q1 + (q2 + q3);
  write('RNEST='); write(rn); write(' ');

  { write() of a concat EXPRESSION directly (materialized temp -> __str_write). }
  write('CCAT='); write(p1 + p2); write(' ');

  { self-alias s := s + c (char coercion of the concat operand). }
  sa := 'ab';
  sa := sa + 'c';
  write('SELFA='); write(sa); write(' ');

  { prepend: 'a' + s -- the leftmost char coercion initializes the accumulator. }
  pre := 'Z';
  pre := 'a' + pre;
  write('PRE='); write(pre); write(' ');

  { all six relops incl. a prefix case ('ab' vs 'abc') + equal + a FALSE. }
  u_ab := 'ab'; u_ab2 := 'ab'; u_abc := 'abc'; u_abd := 'abd';
  write('EQ='); write(u_ab = u_ab2); write(' ');
  write('EQF='); write(u_ab = u_abc); write(' ');
  write('NE='); write(u_ab <> u_abc); write(' ');
  write('LTP='); write(u_ab < u_abc); write(' ');
  write('LTB='); write(u_abc < u_abd); write(' ');
  write('LE='); write(u_ab <= u_ab2); write(' ');
  write('GT='); write(u_abd > u_abc); write(' ');
  write('GE='); write(u_abc >= u_abc); write(' ');

  { a string = 'a' compare (char coercion of a compare operand). }
  onech := 'q';
  write('CMPCH='); write(onech = 'q'); write(' ');

  { length() on a var and on a var-string parameter. }
  write('LENV='); write(length(u_abc)); write(' ');
  write('LENP='); write(StrLen(u_abc)); write(' ');

  { s[i] read (char) + write (byte l-value), with an integer array indexed in
    the SAME program (cross-talk guard). }
  si := 'XYZ';
  write('IDXR='); write(si[2]); write(' ');
  si[1] := 'W';
  write('IDXW='); write(si); write(' ');
  iarr[2] := 77;
  write('IDXA='); write(iarr[2]); write(' ');

  { recursive var-string accumulation (belt). }
  acc := '';
  BuildDown(3, acc);
  write('RECUR='); write(acc); write(' ');

  { var-string-parameter mutation observed by the caller. }
  mm := 'go';
  AppendBang(mm);
  write('MUT='); writeln(mm)
end.
