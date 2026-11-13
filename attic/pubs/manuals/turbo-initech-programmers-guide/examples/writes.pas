program Writes;
var
  N: integer;
  Ok: boolean;
  C: char;
  S: string;
begin
  N := -25;
  Ok := true;
  C := '!';
  S := 'TPS';
  write('N=', N, ' ');
  writeln(Ok, C, S);
  writeln;
  writeln('done');
  writeln(('A' < 'B'), ' ', ('A' + S))
end.
