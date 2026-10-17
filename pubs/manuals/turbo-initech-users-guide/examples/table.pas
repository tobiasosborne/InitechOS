program Table;
var
  Row, Col, Count: integer;
begin
  for Row := 1 to 4 do
  begin
    for Col := 1 to 4 do
      write(Row * Col, ' ');
    writeln
  end;
  Count := 3;
  while Count > 0 do
  begin
    write(Count, '...');
    Count := Count - 1
  end;
  writeln('go')
end.
