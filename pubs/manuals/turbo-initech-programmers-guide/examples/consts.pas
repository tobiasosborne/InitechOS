program Consts;
const
  Limit = 10;
  Floor = -3;
  Title = 'Quarterly';
  Initial = 'Q';
  Verbose = true;
var
  Row: array[Floor..Limit] of integer;
  Name: string[Limit];
begin
  Name := Title;
  Row[Floor] := Limit * 2;
  writeln(Name, ' ', Initial, ' ', Row[-3]);
  write(Verbose, ' ');
  writeln(Limit - Floor + 1, ' rows')
end.
