program Arith;
var
  A, B: integer;
begin
  A := 17;
  B := 5;
  writeln(A + B, ' ', A - B, ' ', A * B);
  writeln(A div B, ' ', A mod B);
  writeln(-A div B, ' ', -A mod B);
  writeln(A div -B, ' ', A mod -B);
  write(2 + 3 * 4, ' ', (2 + 3) * 4);
  writeln(' ', -2 * -3);
  writeln(10 - 4 - 3, ' ', 100 div 10 div 5)
end.
