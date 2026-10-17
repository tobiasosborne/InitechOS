program SameTextEx;
var
  A, B: string;

function Lower(C: char): char;
begin
  Lower := C;
  if C >= 'A' then
    if C <= 'Z' then Lower := Chr(Ord(C) + 32)
end;

{ True when A and B differ only in case. }
function SameText(var A, B: string): boolean;
var
  I: integer;
  Same: boolean;
begin
  Same := Length(A) = Length(B);
  for I := 1 to Length(A) do
    if Same then
      if Lower(A[I]) <> Lower(B[I]) then
        Same := false;
  SameText := Same
end;

begin
  A := 'Initech';
  B := 'INITECH';
  writeln(SameText(A, B), ' ', A = B);
  B := 'Initrode';
  writeln(SameText(A, B))
end.
