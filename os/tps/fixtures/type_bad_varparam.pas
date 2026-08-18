program BadVarParam;
procedure SetOne(var N: integer);
begin
  N := 1
end;
begin
  SetOne(1)
end.
