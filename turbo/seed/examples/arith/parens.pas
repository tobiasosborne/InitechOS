{ parens.pas -- parens override precedence ; expect "R=20" (beads initech-znb). }
program Parens;
begin
  write('R=');
  writeln((2 + 3) * 4)
end.
