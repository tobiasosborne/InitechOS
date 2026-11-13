program Guard;
const
  Size = 5;
var
  Table: array[1..Size] of integer;
  I, Wanted: integer;
  Found: boolean;
begin
  for I := 1 to Size do Table[I] := I * I;
  Wanted := 16;
  Found := false;
  I := 1;
  while not Found do
  begin
    if I > Size then
      Found := true
    else
      if Table[I] = Wanted then
        Found := true
      else
        I := I + 1
  end;
  if I <= Size then
    writeln(Wanted, ' is entry ', I)
  else
    writeln(Wanted, ' is not in the table')
end.
