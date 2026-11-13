program Loops;
var
  I, Sum: integer;
begin
  Sum := 0;
  I := 1;
  while I <= 10 do
  begin
    Sum := Sum + I;
    I := I + 1
  end;
  writeln('while: ', Sum);
  I := 1;
  repeat
    I := I * 2
  until I > 100;
  writeln('repeat: ', I);
  for I := 1 to 5 do write(I, ' ');
  writeln;
  for I := 5 downto 1 do write(I, ' ');
  writeln;
  for I := 3 to 1 do writeln('never')
end.
