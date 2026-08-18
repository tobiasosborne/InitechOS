{$MODE DELPHI}{$B+}{$H-}
{ bool.pas -- relational operators (= <> < <= > >=), boolean type, and/or/not
  under COMPLETE evaluation (beads initech-f0uc, B1 of
  docs/plans/TPS-M7-subset-plan.md; ADR-0007 DEC-02/DEC-03).

  Golden provenance: HAND-COMPUTED from the Pascal definitions (ISO 7185 /
  Turbo Pascal), per the negative_divmod.pas precedent -- not FPC-minted.
  writeln of a boolean prints "TRUE"/"FALSE" (Turbo Pascal semantics; see
  seed/codegen.c gen_write's DECISION comment). Each check is emitted as a
  distinctive TAG=<TRUE|FALSE> pair with write() (no newline) so the whole
  fixture is one line and the QEMU harness's --expect substring match is
  unambiguous (the negative_divmod.pas / "A=-3 B=-2 ..." idiom).

  COMPLETE-EVALUATION NOTE (ADR-0007 DEC-03): this subset has no
  procedures/functions yet (those land at B4), so a dynamically-observable
  "operand X traps unless operand Y already short-circuited" expression is
  NOT constructible here -- there is no side-effecting operand to trap.
  The proof that both operands of and/or always evaluate is therefore
  STRUCTURAL, not dynamic: seed/codegen.c's gen_binop is the ONE shared
  code path for every binary operator (arithmetic, relational, and/or), and
  it is branch-free (no jump-mnemonic path exists in it, so no branch could
  ever skip evaluating the rhs); test-seed-codegen mechanically greps
  gen_binop's SOURCE for the absence of any jump mnemonic as a check of
  this invariant. See seed/codegen.c's file header for the full argument.

  Sections below (each TAG hand-computed):

  1. RELATIONAL (a=3, b=5): all six operators, a true case and a false case.
       EQT=(a=a)=T   EQF=(a=b)=F
       NET=(a<>b)=T  NEF=(a<>a)=F
       LTT=(a<b)=T   LTF=(b<a)=F
       LET=(a<=a)=T  LEF=(b<=a)=F
       GTT=(b>a)=T   GTF=(a>b)=F
       GET=(b>=b)=T  GEF=(a>=b)=F

  2. AND/OR/NOT truth table (p=true, q=true, r=false):
       ANDTT=(p and q)=T   ANDTF=(p and r)=F   ANDFF=(r and r)=F
       ORTT =(p or q) =T   ORTF =(p or r) =T   ORFF =(r or r) =F
       NOTT =(not p)  =F   NOTF =(not r)  =T

  3. De Morgan spot checks (p=true, r=false; p and r=F, p or r=T):
       DM1=not(p and r)         = not F = T
       DM2=(not p) or (not r)   = F or T = T     (agrees with DM1)
       DM3=not(p or r)          = not T = F
       DM4=(not p) and (not r)  = F and T = F    (agrees with DM3)

  4. Precedence proofs (ISO 7185 / TP: relational lowest and non-chaining;
     'or' joins +/-; 'and' joins */div/mod; 'not' binds tighter than 'and'):
       PARITH1=(1+2 < 2*2)      : (1+2)=3, (2*2)=4, 3<4        = T
       PARITH2=(2*2 < 1+2)      : 4<3                          = F
       PAND (q or r and r), q=true,r=false: 'and' binds tighter than 'or',
         so this is q or (r and r) = T or (F and F) = T or F   = T.
         (If 'or' wrongly bound tighter: (q or r) and r
          = (T or F) and F = T and F = F -- DIFFERENT, so this case is a
          real proof, not a coincidence.)
       PNOT (not r and r), r=false: 'not' binds tighter than 'and', so this
         is (not r) and r = T and F                            = F.
         (If 'not' wrongly bound looser: not (r and r) = not F = T --
          DIFFERENT.)
       PREL ((u or v) = w), u=true,v=false,w=false: relational is the
         OUTERMOST (lowest-precedence) level, so this is (u or v) = w
         = T = F                                                = F.
         (If relational wrongly bound tighter than 'or': u or (v = w)
         = T or (F=F) = T or T = T -- DIFFERENT.)

  Expected exact stdout (the four sections above, in emission order):
    EQT=TRUE EQF=FALSE NET=TRUE NEF=FALSE LTT=TRUE LTF=FALSE LET=TRUE LEF=FALSE GTT=TRUE GTF=FALSE GET=TRUE GEF=FALSE ANDTT=TRUE ANDTF=FALSE ANDFF=FALSE ORTT=TRUE ORTF=TRUE ORFF=FALSE NOTT=FALSE NOTF=TRUE DM1=TRUE DM2=TRUE DM3=FALSE DM4=FALSE PARITH1=TRUE PARITH2=FALSE PAND=TRUE PNOT=FALSE PREL=FALSE
}
program BoolFixture;
var a, b : integer;
    p, q, r : boolean;
    u, v, w : boolean;
begin
  a := 3;
  b := 5;

  { 1. relational: all six operators, true and false cases }
  write('EQT='); write(a = a); write(' ');
  write('EQF='); write(a = b); write(' ');
  write('NET='); write(a <> b); write(' ');
  write('NEF='); write(a <> a); write(' ');
  write('LTT='); write(a < b); write(' ');
  write('LTF='); write(b < a); write(' ');
  write('LET='); write(a <= a); write(' ');
  write('LEF='); write(b <= a); write(' ');
  write('GTT='); write(b > a); write(' ');
  write('GTF='); write(a > b); write(' ');
  write('GET='); write(b >= b); write(' ');
  write('GEF='); write(a >= b); write(' ');

  { 2. and/or/not truth table }
  p := true; q := true; r := false;
  write('ANDTT='); write(p and q); write(' ');
  write('ANDTF='); write(p and r); write(' ');
  write('ANDFF='); write(r and r); write(' ');
  write('ORTT='); write(p or q); write(' ');
  write('ORTF='); write(p or r); write(' ');
  write('ORFF='); write(r or r); write(' ');
  write('NOTT='); write(not p); write(' ');
  write('NOTF='); write(not r); write(' ');

  { 3. De Morgan spot checks }
  write('DM1='); write(not (p and r)); write(' ');
  write('DM2='); write((not p) or (not r)); write(' ');
  write('DM3='); write(not (p or r)); write(' ');
  write('DM4='); write((not p) and (not r)); write(' ');

  { 4. precedence proofs }
  write('PARITH1='); write(1 + 2 < 2 * 2); write(' ');
  write('PARITH2='); write(2 * 2 < 1 + 2); write(' ');
  write('PAND='); write(q or r and r); write(' ');
  write('PNOT='); write(not r and r); write(' ');
  u := true; v := false; w := false;
  write('PREL='); writeln((u or v) = w)
end.
