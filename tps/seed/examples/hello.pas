{ hello.pas -- the seed front-end smoke sample (beads initech-znb).
  Ref: PRD Sec 6.7 -- the eventual smoke program writes "InitechOS seed OK".
  This exercises the whole minimal subset: program header, a var section,
  assignment, integer expressions with precedence, and write/writeln with
  mixed string + integer arguments. (* both comment forms appear too *) }
program Hello;
var
  x, y : integer;
  sum : integer;
begin
  x := 2 + 3 * 4;        (* 14: * binds tighter than + *)
  y := (2 + 3) * 4;      { 20: parens override precedence }
  sum := x + y;
  write('x = ', x, ', y = ', y);
  writeln(' sum = ', sum);
  writeln('InitechOS seed OK')
end.
