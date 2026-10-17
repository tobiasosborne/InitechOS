program Staff;
type
  Person = record
    Badge, Desk: integer;
    Initial: char;
    Exempt: boolean;
  end;
var
  Team: array[1..3] of Person;
  I: integer;
begin
  Team[1].Badge := 101; Team[1].Initial := 'P';
  Team[2].Badge := 102; Team[2].Initial := 'S';
  Team[3].Badge := 103; Team[3].Initial := 'M';
  for I := 1 to 3 do
  begin
    Team[I].Desk := I * 10;
    Team[I].Exempt := Team[I].Initial = 'M'
  end;
  for I := 1 to 3 do
    writeln(Team[I].Badge, ' ', Team[I].Initial,
            ' desk ', Team[I].Desk, ' ',
            Team[I].Exempt)
end.
