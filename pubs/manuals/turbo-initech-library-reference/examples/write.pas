program WriteEx;
var
  Total: integer;
  Late: boolean;
  Grade: char;
  Name: string;
begin
  Total := -570;
  Late := false;
  Grade := 'C';
  Name := 'Samir';
  write('Total: ', Total);
  write(' Late: ', Late);
  write(' Grade: ', Grade, ' ', Name);
  writeln
end.
