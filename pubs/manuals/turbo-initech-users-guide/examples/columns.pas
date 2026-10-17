program Columns;
var
  Item: integer;

{ Writes N right-aligned in a field of Width
  characters: Write has no field widths. }
procedure WriteRight(N, Width: integer);
var
  Digits, Rest: integer;
begin
  Digits := 1;
  Rest := N;
  if Rest < 0 then
  begin
    Digits := 2;
    Rest := -Rest
  end;
  while Rest >= 10 do
  begin
    Digits := Digits + 1;
    Rest := Rest div 10
  end;
  while Digits < Width do
  begin
    write(' ');
    Digits := Digits + 1
  end;
  write(N)
end;

begin
  for Item := 1 to 4 do
  begin
    WriteRight(Item, 3);
    WriteRight(Item * Item * Item * 7, 8);
    writeln
  end;
  WriteRight(-570, 11);
  writeln
end.
