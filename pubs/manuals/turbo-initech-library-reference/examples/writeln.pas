program WritelnEx;
var
  I: integer;
begin
  writeln('Line one');
  writeln;
  for I := 1 to 3 do
    writeln('Line ', I + 1, ': ', I * I);
  writeln(true)
end.
