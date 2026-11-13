program States;
const
  Idle = 0;
  Printing = 1;
  Empty = 2;
var
  State, Sheet: integer;

procedure Show(S: integer);
begin
  if S = Idle then writeln('idle')
  else if S = Printing then writeln('printing')
  else writeln('out of paper')
end;

begin
  State := Idle;
  Show(State);
  for Sheet := 1 to 3 do
  begin
    if Sheet = 3 then
      State := Empty
    else
      State := Printing;
    Show(State)
  end
end.
