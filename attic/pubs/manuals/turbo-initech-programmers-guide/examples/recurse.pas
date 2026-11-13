program Recurse;
var
  I: integer;

function Factorial(N: integer): integer;
begin
  if N <= 1 then
    Factorial := 1
  else
    Factorial := N * Factorial(N - 1)
end;

function Digits(N: integer): integer;
var
  Count: integer;
begin
  Count := 1;
  if N >= 10 then
    Count := Count + Digits(N div 10);
  Digits := Count
end;

begin
  for I := 1 to 6 do write(Factorial(I), ' ');
  writeln;
  write(Digits(7), ' ', Digits(2504));
  writeln(' ', Digits(1992))
end.
