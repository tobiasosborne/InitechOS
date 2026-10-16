program VarArg;
var
  A: integer;
procedure Double(var N: integer);
begin
  N := N * 2
end;
begin
  A := 4;
  Double(A + 1)
end.
