program Sales;
const
  Quarters = 4;
var
  Units: array[1..Quarters] of integer;
  Q, Total, Best: integer;
begin
  Units[1] := 120;
  Units[2] := 95;
  Units[3] := 140;
  Units[4] := 88;
  Total := 0;
  Best := 1;
  for Q := 1 to Quarters do
  begin
    Total := Total + Units[Q];
    if Units[Q] > Units[Best] then Best := Q
  end;
  writeln('Total units: ', Total);
  write('Best quarter: ', Best);
  writeln(' (', Units[Best], ')');
  writeln('Average: ', Total div Quarters)
end.
