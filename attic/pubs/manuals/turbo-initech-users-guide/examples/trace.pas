program Trace;
var
  N, Steps: integer;
  Debug: boolean;
begin
  Debug := true;
  N := 27;
  Steps := 0;
  while N <> 1 do
  begin
    if N mod 2 = 0 then
      N := N div 2
    else
      N := 3 * N + 1;
    Steps := Steps + 1;
    if Debug then
      if Steps <= 3 then
        writeln('step ', Steps, ': N = ', N)
  end;
  writeln(Steps, ' steps')
end.
