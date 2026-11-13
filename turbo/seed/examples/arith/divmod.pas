{ divmod.pas -- Pascal div/mod ; expect "D=3 M=2" (17 div 5 = 3, 17 mod 5 = 2).
  beads initech-znb. div/mod truncate toward zero (Turbo-Pascal semantics). }
program DivMod;
begin
  write('D=');
  write(17 div 5);
  write(' M=');
  writeln(17 mod 5)
end.
