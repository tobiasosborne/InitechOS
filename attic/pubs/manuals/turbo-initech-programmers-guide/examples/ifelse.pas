program IfElse;
var
  Score: integer;

procedure Rate(Score: integer);
begin
  if Score >= 90 then
    writeln(Score, ': excellent')
  else if Score >= 70 then
    writeln(Score, ': satisfactory')
  else
    writeln(Score, ': see your supervisor')
end;

begin
  Rate(95);
  Rate(70);
  Rate(12);
  Score := 5;
  if Score > 0 then
    if Score > 10 then writeln('large')
    else writeln('small')
end.
