program Sort;
const
  Size = 7;
var
  Key: array[1..Size] of integer;
  I, J, Hold: integer;
  Placed: boolean;
begin
  Key[1] := 42; Key[2] := 7; Key[3] := 19;
  Key[4] := 3;  Key[5] := 88; Key[6] := 19;
  Key[7] := -5;
  { insertion sort }
  for I := 2 to Size do
  begin
    Hold := Key[I];
    J := I - 1;
    Placed := false;
    while not Placed do
      if J < 1 then
        Placed := true
      else if Key[J] > Hold then
      begin
        Key[J + 1] := Key[J];
        J := J - 1
      end
      else
        Placed := true;
    Key[J + 1] := Hold
  end;
  for I := 1 to Size do write(Key[I], ' ');
  writeln
end.
