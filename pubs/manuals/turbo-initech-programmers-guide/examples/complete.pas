program Complete;
var
  Calls: integer;

function Check(Answer: boolean): boolean;
begin
  Calls := Calls + 1;
  Check := Answer
end;

begin
  Calls := 0;
  if Check(false) and Check(true) then
    writeln('both');
  writeln('calls: ', Calls);
  if Check(true) or Check(false) then
    writeln('either');
  writeln('calls: ', Calls)
end.
