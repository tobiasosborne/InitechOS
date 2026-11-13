program Mutual;
var
  N: integer;

function IsOdd(N: integer): boolean; forward;

function IsEven(N: integer): boolean;
begin
  if N = 0 then
    IsEven := true
  else
    IsEven := IsOdd(N - 1)
end;

function IsOdd(N: integer): boolean;
begin
  if N = 0 then
    IsOdd := false
  else
    IsOdd := IsEven(N - 1)
end;

begin
  for N := 0 to 3 do
    writeln(N, ' even: ', IsEven(N))
end.
