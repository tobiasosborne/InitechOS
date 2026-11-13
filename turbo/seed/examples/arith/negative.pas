{ negative.pas -- unary-minus / negative decimal ; expect "N=-5" (0 - 5).
  beads initech-znb. Exercises signed decimal formatting in serial_put_int. }
program Negative;
begin
  write('N=');
  writeln(0 - 5)
end.
