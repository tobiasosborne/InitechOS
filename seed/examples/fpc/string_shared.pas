{$B+}{$H-}
{ string_shared.pas -- DEC-07 Rung 2 SHARED-SUBSET corpus for B7 strings
  (beads initech-39k2; ADR-0007 Sec 4.7 Rung 2). Compiled by BOTH Turbo
  Initech's seed AND Free Pascal (`fpc`); test-seed-fpc-diff asserts their
  stdout is BYTE-IDENTICAL.

  Line 1 carries BOTH directives (harmless brace comments to the seed, which
  treats any brace-delimited region as an ordinary comment):
    the dollar-B-plus directive -- COMPLETE boolean evaluation (BOOLEVAL ON),
      so fpc matches ADR-0007 DEC-03 like the seed does.
    the dollar-H-minus directive -- PIN fpc's ShortString semantics explicitly
      (string = ShortString, NOT AnsiString), so string[N] / length / index /
      compare / concat mean the same fixed-capacity type both compilers
      implement (Seat C, unanimous adopt). Without it a bare `string` under
      fpc's default could be an AnsiString and the differential would compare
      two different types.

  SHARED-OUTPUT ENVELOPE (D10): ASCII only; plain write/writeln (no width/
  precision, no reals); explicit string[N] capacities; ONLY in-range s[i]; NO
  char+char (every concat keeps >= 1 string operand); NO s[0]; ONE output line
  ending in exactly one writeln. Everything emitted here has a textual form
  identical between the seed's serial RTL (__str_write / serial_put_int /
  serial_putc) and fpc's writeln.

  Expected stdout (both compilers): "Hello, World! 13 H EQ LT" + newline. }
program StringShared;
var
  a, b, c: string[16];
  r: string[64];
begin
  a := 'Hello';
  b := ', ';
  c := 'World';
  r := a + b + c + '!';         { concat chain, every step has a string operand }
  write(r); write(' ');          { whole-string write }
  write(length(r)); write(' ');  { length -> integer }
  write(r[1]); write(' ');       { in-range s[i] -> ASCII char 'H' }
  if a = 'Hello' then            { string equality compare }
    write('EQ')
  else
    write('NE');
  write(' ');
  if a < c then                  { string ordering compare ('H' < 'W') }
    write('LT')
  else
    write('GE');
  writeln
end.
