program Search;
const
  Size = 6;
var
  Badge: array[1..Size] of integer;
  I, Wanted, Where: integer;
  Done: boolean;
begin
  Badge[1] := 2504; Badge[2] := 1001;
  Badge[3] := 3300; Badge[4] := 1992;
  Badge[5] := 4501; Badge[6] := 1077;
  Wanted := 1992;
  Where := 0;
  I := 1;
  Done := false;
  while not Done do
    if I > Size then
      Done := true
    else if Badge[I] = Wanted then
    begin
      Where := I;
      Done := true
    end
    else
      I := I + 1;
  if Where = 0 then
    writeln(Wanted, ' not found')
  else
    writeln(Wanted, ' found at ', Where)
end.
