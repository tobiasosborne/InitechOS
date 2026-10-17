program Relations;
var
  N: integer;
  C: char;
  B: boolean;
begin
  N := 3;
  C := 'm';
  B := N >= 3;
  writeln(B, ' ', N <> 3, ' ', (C > 'a'));
  writeln((false < true), ' ', (B = true));
  writeln(not (N = 3) or (C = 'm'))
end.
