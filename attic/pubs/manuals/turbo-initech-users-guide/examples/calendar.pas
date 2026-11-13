program Calendar;
var
  Month, Year, Days, Total: integer;

function IsLeap(Y: integer): boolean;
begin
  IsLeap := false;
  if Y mod 4 = 0 then IsLeap := true;
  if Y mod 100 = 0 then IsLeap := false;
  if Y mod 400 = 0 then IsLeap := true
end;

function DaysIn(M, Y: integer): integer;
begin
  DaysIn := 31;
  if (M = 4) or (M = 6) then DaysIn := 30;
  if (M = 9) or (M = 11) then DaysIn := 30;
  if M = 2 then
    if IsLeap(Y) then DaysIn := 29
    else DaysIn := 28
end;

begin
  Year := 1992;
  Total := 0;
  for Month := 1 to 12 do
    Total := Total + DaysIn(Month, Year);
  writeln(Year, ' has ', Total, ' days');
  write('1900 leap: ', IsLeap(1900));
  writeln(', 1600 leap: ', IsLeap(1600))
end.
