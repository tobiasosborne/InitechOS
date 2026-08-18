{$MODE DELPHI}{$B+}{$H-}
{ char.pas -- const declarations, the char type, char literals, and ord/chr
  (beads initech-7mo3, B3 of docs/plans/TPS-M7-subset-plan.md; ADR-0007
  DEC-02: "const declarations", "char, ord, chr").

  Golden provenance: HAND-COMPUTED from the Pascal definitions (ISO 7185 /
  Turbo Pascal), per the bool.pas / control.pas / negative_divmod.pas
  precedent -- not FPC-minted. Each check is a distinctive TAG=<value> pair
  with write() (no newline) so the whole fixture is one line and the QEMU
  harness's --expect substring match is unambiguous.

  Sections below (each TAG hand-computed):

  1. ord()/chr() value no-ops + round trips (ASCII: 'A'=65, 'B'=66):
       ORDA     = ord('A')        = 65
       CHRA     = chr(65)         = 'A'
       ORDCHR65 = ord(chr(66))    = 66   (chr then ord: round trip)
       CHRORDB  = chr(ord('B'))   = 'B'  (ord then chr: round trip)

  2. chr() out-of-range TRUNCATES to 8 bits (DECISION: Borland Turbo Pascal
     7.0 Language Guide "Chr", default range-checking-off mode -- see
     codegen.c's OP_CHR comment; NOTE the dollar-R-minus compiler directive
     is deliberately NOT written literally here, braces do not nest in
     this lexer's comment scanner, see lexer.c):
       CHRWRAP = chr(65 + 256) = chr(321); 321 = 0x141, low byte 0x41 = 65
               = 'A'

  3. const folding (front-end only, no runtime storage -- see ast.h/
     parser.c's B3 comments): an int const used directly in an expression,
     plus a negative int const, a char const, and boolean consts. None of
     MAXCOUNT/NEGONE/LETTERA/FLAGON/FLAGOFF gets a .bss slot -- verify by
     inspecting the emitted .bss section (test-seed-codegen's structural
     check, mirroring bool.pas's gen_binop grep).
       CONSTUSE   = MAXCOUNT + 1 = 10 + 1        = 11
       NEGC       = NEGONE                       = -1
       CHRCONST   = LETTERA                      = 'A'
       BOOLCONSTT = FLAGON                       = TRUE
       BOOLCONSTF = FLAGOFF                       = FALSE

  4. char comparisons driving if/then/else (relational ops on char yield
     boolean -- needed by every lexer):
       CMPIF = (LETTERA = 'A') ? "EQ" : "NE"   ; 'A'='A' is TRUE -> EQ
       CMPLT = ('a' < 'b') ? "LT" : "GE"       ; ascii 97 < 98 -> LT

  5. char compares driving a WHILE loop -- a mini scan loop over a const
     char (the exact shape a lexer's char-scanner uses): start c='A',
     advance c via chr(ord(c)+1) each iteration, count while c <= ENDCH
     (const char 'E'=69).
       c=65<=69 count=1 c->66   c=66<=69 count=2 c->67
       c=67<=69 count=3 c->68   c=68<=69 count=4 c->69
       c=69<=69 count=5 c->70   c=70<=69 FALSE, stop
       SCAN = 5   (A,B,C,D,E)

  6. ord(boolean): Turbo Pascal ALLOWS Ord on Boolean, giving 0/1 --
     DECISION: IMPLEMENTED, not rejected (see typecheck.c check_unop's
     OP_ORD case for the citation).
       ORDBOOLT = ord(true)  = 1
       ORDBOOLF = ord(false) = 0

  7. write(char) emits the RAW BYTE, never decimal or TRUE/FALSE:
       WCH = chr(90) printed literally as 'Z', not "90".
}
program CharFixture;
const
  MAXCOUNT = 10;
  NEGONE = -1;
  LETTERA = 'A';
  ENDCH = 'E';
  FLAGON = true;
  FLAGOFF = false;
var
  c : char;
  count, x : integer;
begin
  { 1. ord/chr value no-ops + round trips }
  write('ORDA='); write(ord('A')); write(' ');
  write('CHRA='); write(chr(65)); write(' ');
  write('ORDCHR65='); write(ord(chr(66))); write(' ');
  write('CHRORDB='); write(chr(ord('B'))); write(' ');

  { 2. chr() 8-bit truncation }
  write('CHRWRAP='); write(chr(65 + 256)); write(' ');

  { 3. const folding -- no runtime storage for MAXCOUNT/NEGONE/LETTERA/
        FLAGON/FLAGOFF }
  x := MAXCOUNT + 1;
  write('CONSTUSE='); write(x); write(' ');
  write('NEGC='); write(NEGONE); write(' ');
  write('CHRCONST='); write(LETTERA); write(' ');
  write('BOOLCONSTT='); write(FLAGON); write(' ');
  write('BOOLCONSTF='); write(FLAGOFF); write(' ');

  { 4. char comparisons driving if/else }
  if LETTERA = 'A' then write('CMPIF=EQ') else write('CMPIF=NE');
  write(' ');
  if 'a' < 'b' then write('CMPLT=LT') else write('CMPLT=GE');
  write(' ');

  { 5. char compare driving a while loop -- mini scan over a const char }
  c := 'A'; count := 0;
  while c <= ENDCH do
  begin
    count := count + 1;
    c := chr(ord(c) + 1)
  end;
  write('SCAN='); write(count); write(' ');

  { 6. ord(boolean) }
  write('ORDBOOLT='); write(ord(true)); write(' ');
  write('ORDBOOLF='); write(ord(false)); write(' ');

  { 7. write(char) emits the raw byte, not decimal }
  write('WCH=');
  writeln(chr(90))
end.
