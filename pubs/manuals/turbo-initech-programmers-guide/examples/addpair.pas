program AddPair;
var
  Total: integer;

function Add(A, B: integer): integer;
begin
  Add := A + B
end;

begin
  Total := Add(2, 3);
  writeln(Total)
end.
