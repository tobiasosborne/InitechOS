{ precedence.pas -- * binds tighter than + ; expect "R=14" (beads initech-znb). }
program Precedence;
begin
  write('R=');
  writeln(2 + 3 * 4)
end.
